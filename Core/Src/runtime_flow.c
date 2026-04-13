#include "runtime_flow.h"

#include <string.h>

#include "ambient.h"
#include "board_dispatch.h"
#include "event_layer.h"
#include "features.h"
#include "handle_pwm.h"
#include "led_runtime.h"
#include "runtime_can.h"
#include "runtime_render.h"
#include "runtime_stop.h"
#include "themes.h"
#include "zones.h"

extern const zone_map_t g_zone_map[ZONE_MAX];

static runtime_mode_t flow_state(const runtime_flow_ctx_t *ctx)
{
    return (ctx && ctx->runtime_state) ? *ctx->runtime_state : RUNTIME_BOOT;
}

void runtime_flow_set_state(runtime_flow_ctx_t *ctx, runtime_mode_t s)
{
    if (!ctx || !ctx->runtime_state) return;
    *ctx->runtime_state = s;
}

static float runtime_ease01(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
static uint32_t runtime_sleep_cancel_idle_threshold_ms(void)
{
    uint32_t div = (AMB_SLEEP_CANCEL_IDLE_DIV == 0u) ? 1u : AMB_SLEEP_CANCEL_IDLE_DIV;
    return (AMB_SLEEP_TIMEOUT_SEC * 1000u) / div;
}
#endif

#if !DEMO_MODE
static void runtime_activate_from_oem(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip)
{
    runtime_inputs_t inputs;
    oem_color_id_t oem_col;
    uint8_t theme_idx;
    const theme_bank_t *bank;

    if (!ctx || !ctx->oem_color || !ctx->current_theme || !ctx->base_scene) return;

    can_ambient_fill_runtime_inputs(&inputs);
    oem_col = (oem_color_id_t)inputs.can_state.oem_color;
    if (oem_col >= OEM_COLOR_MAX) {
        oem_col = OEM_COLOR_AMBER;
    }
    theme_idx = inputs.can_state.oem_theme_indices[oem_col];

    *ctx->oem_color = oem_col;
    bank = theme_get_bank(oem_col);
    if (bank && bank->count > 0) {
        if (theme_idx == 0xFF || theme_idx >= bank->count) {
            theme_idx = 0;
        }
        *ctx->current_theme = bank->themes[theme_idx];
    } else {
        *ctx->current_theme = theme_default_for_oem(oem_col);
    }

    base_scene_init(ctx->base_scene, *ctx->current_theme);
    if (main_strip) {
        runtime_render_push_black_frame();
        base_scene_start_intro(main_strip, ctx->base_scene);
    }

    runtime_flow_set_state(ctx, RUNTIME_ACTIVE);
}
#endif

#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
static void runtime_sleep_cancel(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip)
{
    if (!ctx || !ctx->sleep_fade_active) return;
    *ctx->sleep_fade_active = 0u;
    can_ambient_clear_sleep_request();
    runtime_flow_set_state(ctx, RUNTIME_ACTIVE);
    if (main_strip) {
        led_runtime_release_brightness(main_strip);
    }
}

static uint8_t runtime_sleep_outro_cancel_requested(void)
{
    return (uint8_t)(can_ambient_get_idle_time_ms() < runtime_sleep_cancel_idle_threshold_ms());
}

static uint8_t runtime_sleep_run_outro_until_done(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip)
{
    if (!ctx || !ctx->base_scene || !ctx->last_tick_ms) return 0u;

    if (main_strip && ctx->base_scene->stage == BASE_SCENE_ACTIVE) {
        base_scene_start_outro(main_strip, ctx->base_scene);
    }

    while (ctx->base_scene->stage == BASE_SCENE_OUTRO) {
        uint32_t outro_now;
        uint32_t outro_dt;
        uint32_t outro_elapsed;
        uint32_t outro_duration;
        float outro_t;

        if (runtime_sleep_outro_cancel_requested()) {
            runtime_sleep_cancel(ctx, main_strip);
            return 1u;
        }

        outro_now = HAL_GetTick();
        outro_dt = outro_now - *ctx->last_tick_ms;
        if (outro_dt > 100u) outro_dt = 16u;
        *ctx->last_tick_ms = outro_now;
        base_scene_tick(main_strip, ctx->base_scene, outro_dt);

        outro_elapsed = outro_now - ctx->base_scene->outro.start_ms;
        outro_duration = ctx->base_scene->outro.duration_ms ? ctx->base_scene->outro.duration_ms : 1u;
        if (outro_elapsed > outro_duration) outro_elapsed = outro_duration;
        outro_t = (float)outro_elapsed / (float)outro_duration;
        zones_apply_scene(ctx->base_scene);
        zones_apply_outro(ctx->base_scene, outro_t);
        board_dispatch_led_render_all();

        HAL_Delay(10);
    }

    return 0u;
}

static void runtime_sleep_finish_handle_fade(runtime_flow_ctx_t *ctx)
{
    uint32_t pwm_fade_t0 = HAL_GetTick();

    if (!ctx || !ctx->last_tick_ms) return;

    handle_pwm_set_enabled(0u);
    while (!handle_pwm_is_off()) {
        uint32_t pwm_now = HAL_GetTick();
        uint32_t pwm_dt = pwm_now - *ctx->last_tick_ms;
        if (pwm_dt > 100u) pwm_dt = 16u;
        *ctx->last_tick_ms = pwm_now;
        handle_pwm_tick(pwm_dt);
        if ((pwm_now - pwm_fade_t0) > (AMB_HANDLE_PWM_RELEASE_MS + 200u)) {
            break;
        }
        HAL_Delay(5);
    }
    handle_pwm_prepare_stop();
}

static void runtime_sleep_enter_stop_and_recover(runtime_flow_ctx_t *ctx)
{
    runtime_stop_hooks_t stop_hooks;

    if (!ctx || !ctx->can_protocol_started || !ctx->can_protocol_start_ms) return;

    runtime_render_push_black_frame();
    can_ambient_enter_sleep();
    runtime_flow_set_state(ctx, RUNTIME_STOP);

    memset(&stop_hooks, 0, sizeof(stop_hooks));
    stop_hooks.watchdog = ctx->watchdog;
#if AMB_ENABLE_WATCHDOG
    stop_hooks.watchdog_enabled = 1u;
#else
    stop_hooks.watchdog_enabled = 0u;
#endif
    stop_hooks.prepare_wakeup_io = ctx->prepare_wakeup_io;
    stop_hooks.restore_after_wake = ctx->restore_after_wake;
    stop_hooks.restore_system_clock = ctx->system_clock_restore;
#if AMB_ENABLE_WATCHDOG
    stop_hooks.rtc_wakeup_start = ctx->rtc_wakeup_start;
    stop_hooks.rtc_wakeup_stop = ctx->rtc_wakeup_stop;
#else
    stop_hooks.rtc_wakeup_start = NULL;
    stop_hooks.rtc_wakeup_stop = NULL;
#endif
    runtime_stop_enter_and_wait(&stop_hooks);

    can_ambient_note_stop_wakeup(runtime_stop_get_wakeup_source());
    handle_pwm_resume_after_stop();
    can_ambient_exit_sleep();

    *ctx->can_protocol_started = 0u;
    *ctx->can_protocol_start_ms = 0u;
    can_ambient_reset_oem_received();
    runtime_flow_set_state(ctx, RUNTIME_WAKE_RECOVER);
}

static uint8_t runtime_handle_sleep_state(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip, uint32_t now)
{
    uint32_t fade_elapsed;

    if (!ctx || !ctx->sleep_fade_active || !ctx->sleep_fade_start_ms || !ctx->sleep_fade_start_brightness ||
        !ctx->base_scene) {
        return 0u;
    }

    can_ambient_check_sleep_timeout();

    if (*ctx->sleep_fade_active && runtime_sleep_outro_cancel_requested()) {
        runtime_sleep_cancel(ctx, main_strip);
    }

    if (can_ambient_should_sleep() && !*ctx->sleep_fade_active) {
        *ctx->sleep_fade_active = 1u;
        *ctx->sleep_fade_start_ms = now;
        *ctx->sleep_fade_start_brightness = ctx->base_scene->calc_brightness;
        runtime_flow_set_state(ctx, RUNTIME_SLEEP_PREP);
    }

    if (!*ctx->sleep_fade_active) {
        return 0u;
    }

    fade_elapsed = now - *ctx->sleep_fade_start_ms;
    if (fade_elapsed >= AMB_SLEEP_FADE_OUT_MS) {
        *ctx->sleep_fade_active = 0u;
        can_ambient_clear_sleep_request();

        if (runtime_sleep_run_outro_until_done(ctx, main_strip)) {
            return 0u;
        }

        if (main_strip) {
            led_runtime_release_brightness(main_strip);
        }
        runtime_sleep_finish_handle_fade(ctx);
        runtime_sleep_enter_stop_and_recover(ctx);
        return 1u;
    } else {
        float fade_progress = (float)fade_elapsed / (float)AMB_SLEEP_FADE_OUT_MS;
        float eased = runtime_ease01(fade_progress);
        float fade_brightness = *ctx->sleep_fade_start_brightness * (1.0f - eased);
        if (fade_brightness < 0.0f) fade_brightness = 0.0f;
        if (main_strip) {
            led_runtime_force_brightness(main_strip, fade_brightness);
        }
    }

    return 0u;
}
#endif

static void runtime_render_base_scene_and_zones(runtime_flow_ctx_t *ctx, uint32_t now)
{
    float t_norm = 0.0f;

    if (!ctx || !ctx->base_scene) return;

    switch (ctx->base_scene->stage) {
    case BASE_SCENE_INTRO:
    {
        uint32_t elapsed = now - ctx->base_scene->intro.start_ms;
        uint32_t duration = ctx->base_scene->intro.duration_ms ? ctx->base_scene->intro.duration_ms : 1u;
        if (elapsed > duration) elapsed = duration;
        t_norm = (float)elapsed / (float)duration;
        zones_apply_intro(ctx->base_scene, t_norm);
    }
    break;

    case BASE_SCENE_ACTIVE:
        zones_apply_scene(ctx->base_scene);
        break;

    case BASE_SCENE_BRIDGE:
    {
        uint32_t elapsed = now - ctx->base_scene->t0_ms;
        uint32_t duration = AMB_BRIDGE_DURATION_MS ? AMB_BRIDGE_DURATION_MS : 1u;
        if (elapsed > duration) elapsed = duration;
        t_norm = (float)elapsed / (float)duration;
        zones_apply_bridge(ctx->base_scene, t_norm);
    }
    break;

    case BASE_SCENE_OUTRO:
    {
        uint32_t elapsed = now - ctx->base_scene->outro.start_ms;
        uint32_t duration = ctx->base_scene->outro.duration_ms ? ctx->base_scene->outro.duration_ms : 1u;
        if (elapsed > duration) elapsed = duration;
        t_norm = (float)elapsed / (float)duration;
        zones_apply_outro(ctx->base_scene, t_norm);
    }
    break;

    default:
        break;
    }

    event_layer_apply(now);
    zones_apply_interrupt_overlay(now);
    runtime_render_postprocess_frame(now);
#if AMB_DEBUG_BSM_RX_PULSE
    /* Bench diagnostics: force an obvious strip pulse to verify RX path timing. */
    if (ctx->dbg_bsm_rx_until_ms && (now < *ctx->dbg_bsm_rx_until_ms)) {
        const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
        if (zm && zm->strip && zm->count > 0u) {
            uint16_t i;
            for (i = 0u; i < zm->count; ++i) {
                led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), 255u, 24u, 0u);
            }
        }
    } else if (ctx->dbg_non_oem_rx_until_ms && ctx->dbg_non_oem_rx_id &&
               (now < *ctx->dbg_non_oem_rx_until_ms)) {
        const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
        uint8_t r = 96u, g = 0u, b = 255u;
        if (*ctx->dbg_non_oem_rx_id == CAN_BSM_ID) {
            r = 255u;
            g = 0u;
            b = 0u;
        }
        if (zm && zm->strip && zm->count > 0u) {
            uint16_t i;
            for (i = 0u; i < zm->count; ++i) {
                led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
            }
        }
    }
#endif
    board_dispatch_led_render_all();
}

static void runtime_update_handle_pwm(const runtime_flow_ctx_t *ctx, uint32_t now, uint32_t dt)
{
    uint8_t handle_pwm_enable = 0u;
    uint8_t handle_pwm_pct = 100u;

    if (can_ambient_nsi_active()) {
#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
        if (ctx && ctx->sleep_fade_active &&
            !(can_ambient_is_sleeping() || (can_ambient_should_sleep() && !*ctx->sleep_fade_active))) {
            handle_pwm_enable = 1u;
        }
        if (ctx && ctx->sleep_fade_active && *ctx->sleep_fade_active && ctx->sleep_fade_start_ms) {
            uint32_t fade_elapsed = now - *ctx->sleep_fade_start_ms;
            if (fade_elapsed >= AMB_SLEEP_FADE_OUT_MS) {
                handle_pwm_pct = 0u;
            } else {
                float fade_progress = (float)fade_elapsed / (float)AMB_SLEEP_FADE_OUT_MS;
                float eased = runtime_ease01(fade_progress);
                float lvl = 1.0f - eased;
                if (lvl < 0.0f) lvl = 0.0f;
                if (lvl > 1.0f) lvl = 1.0f;
                handle_pwm_pct = (uint8_t)(lvl * 100.0f + 0.5f);
            }
        }
#else
        (void)ctx;
        handle_pwm_enable = 1u;
#endif
    }

    handle_pwm_set_brightness_pct(handle_pwm_pct);
    handle_pwm_set_enabled(handle_pwm_enable);
    handle_pwm_tick(dt);
}

static void runtime_maybe_trigger_hvac_temp_event(uint32_t now)
{
#if AMB_ENABLE_HVAC_TEMP_EVENT_SCENE
    static uint32_t s_last_hvac_event_ms = 0u;
    int8_t trend = can_ambient_consume_hvac_temp_trend_for_board();
    event_scene_t scene;
    uint8_t warmer;
    uint8_t burst = 0u;

    if (trend == 0) return;
    warmer = (trend > 0) ? 1u : 0u;
    if (s_last_hvac_event_ms != 0u && (now - s_last_hvac_event_ms) <= AMB_HVAC_TEMP_EVENT_BURST_WINDOW_MS) {
        burst = 1u;
    }
    s_last_hvac_event_ms = now;
    if (warmer) {
        event_scene_build_temp_delta(&scene,
                                     1u,
                                     AMB_HVAC_TEMP_WARM_R,
                                     AMB_HVAC_TEMP_WARM_G,
                                     AMB_HVAC_TEMP_WARM_B,
                                     AMB_HVAC_TEMP_EVENT_HOLD_MS);
    } else {
        event_scene_build_temp_delta(&scene,
                                     0u,
                                     AMB_HVAC_TEMP_COOL_R,
                                     AMB_HVAC_TEMP_COOL_G,
                                     AMB_HVAC_TEMP_COOL_B,
                                     AMB_HVAC_TEMP_EVENT_HOLD_MS);
    }
    if (burst && scene.strip_trail) {
        scene.strip_trail_cycles = AMB_HVAC_TEMP_EVENT_BURST_CYCLES;
    }
    event_layer_note_climate_memory(warmer, now);
    (void)event_layer_start(&scene, now);
#else
    (void)now;
#endif
}

uint8_t runtime_flow_dispatch_state(runtime_flow_ctx_t *ctx,
                                    led_runtime_strip_t *main_strip,
                                    uint32_t dt,
                                    uint8_t oem_received)
{
#if !DEMO_MODE
    uint32_t now = HAL_GetTick();

    if (!ctx || !ctx->wait_oem_after_wake || !ctx->wait_oem_enter_ms) return 0u;

    switch (flow_state(ctx)) {
    case RUNTIME_BOOT:
        *ctx->wait_oem_after_wake = 0u;
        *ctx->wait_oem_enter_ms = now;
        runtime_flow_set_state(ctx, RUNTIME_WAIT_OEM);
    /* fallthrough */
    case RUNTIME_WAKE_RECOVER:
        if (flow_state(ctx) == RUNTIME_WAKE_RECOVER) {
            *ctx->wait_oem_after_wake = 1u;
            *ctx->wait_oem_enter_ms = now;
            runtime_flow_set_state(ctx, RUNTIME_WAIT_OEM);
        }
    /* fallthrough */
    case RUNTIME_WAIT_OEM:
        if (oem_received) {
            *ctx->wait_oem_after_wake = 0u;
            *ctx->wait_oem_enter_ms = 0u;
            runtime_activate_from_oem(ctx, main_strip);
            return 0u;
        }
#if AMB_ENABLE_SLEEP_MODE
        if (*ctx->wait_oem_after_wake && *ctx->wait_oem_enter_ms != 0u) {
            uint32_t wait_ms = (now >= *ctx->wait_oem_enter_ms)
                                   ? (now - *ctx->wait_oem_enter_ms)
                                   : ((UINT32_MAX - *ctx->wait_oem_enter_ms) + now + 1u);
            if (wait_ms >= AMB_WAIT_OEM_RESLEEP_MS) {
#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
                runtime_sleep_enter_stop_and_recover(ctx);
                return 1u;
#endif
            }
        }
#endif
        led_runtime_power_set(0u);
        if (main_strip) {
            led_runtime_release_brightness(main_strip);
        }
        handle_pwm_set_enabled(0u);
        handle_pwm_tick(dt);
#if AMB_ENABLE_WATCHDOG
        if (ctx->watchdog) {
            HAL_IWDG_Refresh(ctx->watchdog);
        }
#endif
        __WFI();
        return 1u;

    case RUNTIME_ACTIVE:
    case RUNTIME_SLEEP_PREP:
        return 0u;

    case RUNTIME_STOP:
#if AMB_ENABLE_WATCHDOG
        if (ctx->watchdog) {
            HAL_IWDG_Refresh(ctx->watchdog);
        }
#endif
        __WFI();
        return 1u;

    default:
        __WFI();
        return 1u;
    }
#else
    (void)ctx;
    (void)main_strip;
    (void)dt;
    (void)oem_received;
    return 0u;
#endif
}

uint8_t runtime_flow_handle_active_state(runtime_flow_ctx_t *ctx,
                                         led_runtime_strip_t *main_strip,
                                         uint32_t now,
                                         uint32_t dt,
                                         uint8_t oem_received)
{
    runtime_inputs_t inputs;

    if (!ctx || !ctx->director || !ctx->base_scene || !ctx->oem_color || !ctx->current_theme ||
        !ctx->can_protocol_started || !ctx->can_protocol_start_ms) {
        return 0u;
    }

    if (main_strip) {
        can_ambient_fill_runtime_inputs(&inputs);
#if defined(DEMO_MODE) && DEMO_MODE == 1
        if (can_ambient_is_master()) {
            g_night_mode_state = inputs.can_state.night_mode;
        } else {
            director_update(ctx->director, main_strip, ctx->base_scene, &inputs);
        }
#else
        director_update(ctx->director, main_strip, ctx->base_scene, &inputs);
#endif
        runtime_maybe_trigger_hvac_temp_event(now);

        if (inputs.can_state.oem_color < OEM_COLOR_MAX) {
            *ctx->oem_color = (oem_color_id_t)inputs.can_state.oem_color;
        } else {
            *ctx->oem_color = OEM_COLOR_AMBER;
        }
    }

    if (ctx->demo_mode_update) {
        ctx->demo_mode_update(now);
    }

#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
    if (flow_state(ctx) == RUNTIME_ACTIVE || flow_state(ctx) == RUNTIME_SLEEP_PREP) {
        if (runtime_handle_sleep_state(ctx, main_strip, now)) {
            return 1u;
        }
    }
#endif

    if (main_strip) {
        base_scene_tick(main_strip, ctx->base_scene, dt);
    }

    runtime_update_handle_pwm(ctx, now, dt);
    runtime_render_base_scene_and_zones(ctx, now);
    runtime_can_tx_scheduler_tick(now,
                                  oem_received,
                                  *ctx->can_protocol_started,
                                  *ctx->can_protocol_start_ms,
#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
                                  *ctx->sleep_fade_active
#else
                                  0u
#endif
    );

    return 0u;
}
