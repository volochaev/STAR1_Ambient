#include "runtime_flow.h"

#include <string.h>

#include "ambient.h"
#include "board_dispatch.h"
#include "event_layer.h"
#include "ambient_config.h"
#include "handle_pwm.h"
#include "led_runtime.h"
#include "runtime_can.h"
#include "runtime_events.h"
#include "runtime_render.h"
#include "runtime_debug_hooks.h"
#include "runtime_state_machine.h"
#include "runtime_stop.h"
#include "zones.h"

extern const zone_map_t g_zone_map[ZONE_MAX];

static runtime_mode_t flow_state(const runtime_flow_ctx_t *ctx)
{
    return (ctx && ctx->runtime_state) ? *ctx->runtime_state : RUNTIME_BOOT;
}

void runtime_flow_set_state(runtime_flow_ctx_t *ctx, runtime_mode_t s)
{
    if (!ctx || !ctx->runtime_state) return;
    (void)runtime_state_machine_transition(ctx->runtime_state, s);
}

static float runtime_ease01(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

#if AMB_ENABLE_SLEEP_MODE
static uint32_t runtime_sleep_cancel_idle_threshold_ms(void)
{
    uint32_t div = (AMB_SLEEP_CANCEL_IDLE_DIV == 0u) ? 1u : AMB_SLEEP_CANCEL_IDLE_DIV;
    return (AMB_SLEEP_TIMEOUT_SEC * 1000u) / div;
}
#endif

static void runtime_activate_from_oem(runtime_flow_ctx_t *ctx,
                                      led_runtime_strip_t *main_strip,
                                      uint8_t wake_recover,
                                      uint8_t door_gated_start)
{
    runtime_inputs_t inputs;
    oem_color_id_t oem_col;

    if (!ctx || !ctx->oem_color || !ctx->base_scene) return;

    can_ambient_fill_runtime_inputs(&inputs);
    oem_col = (oem_color_id_t)inputs.can_state.oem_color;
    if (oem_col >= OEM_COLOR_MAX) {
        oem_col = OEM_COLOR_AMBER;
    }
    *ctx->oem_color = oem_col;
    /* Reset director transient runtime (pending-theme / armed events) on each WAIT_OEM activation. */
    if (ctx->director) {
        director_init(ctx->director);
    }
    /* Drop stale event overlays/holds from previous active session before fresh start. */
    event_layer_init();
    base_scene_init(ctx->base_scene);
    if (main_strip) {
        runtime_render_push_black_frame();
#if AMB_DISABLE_WELCOME_INTRO
        (void)wake_recover;
#if AMB_ENABLE_START_GATE_DOOR_OPEN
        if (door_gated_start) {
            /* For first activation via door-gate after sleep, force intro animation
             * even when global intro is disabled. */
            base_scene_start_intro(main_strip, ctx->base_scene);
        } else {
            base_scene_set_active(ctx->base_scene);
        }
#else
        base_scene_set_active(ctx->base_scene);
#endif
#else
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
        if (wake_recover) {
            base_scene_set_active(ctx->base_scene);
        } else {
            base_scene_start_intro(main_strip, ctx->base_scene);
        }
#else
        base_scene_start_intro(main_strip, ctx->base_scene);
#endif
#endif
    }

    runtime_flow_set_state(ctx, RUNTIME_ACTIVE);
}

#if AMB_ENABLE_SLEEP_MODE
static void runtime_sleep_note_periodic_rtc_wakeup_cycle(void)
{
    can_ambient_note_stop_rtc_wakeup_cycle();
    runtime_debug_hooks_note_stop_rtc_cycle(HAL_GetTick());
}

static void runtime_sleep_cancel(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip)
{
    if (!ctx || !ctx->sleep_fade_active) return;
    *ctx->sleep_fade_active = 0u;
    can_ambient_clear_sleep_request();
    runtime_flow_set_state(ctx, RUNTIME_ACTIVE);
    if (main_strip) {
        led_runtime_release_brightness(main_strip);
    }
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    runtime_events_clear_lock_sleep_forced();
#endif
}

static uint8_t runtime_sleep_outro_cancel_requested(void)
{
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    if (runtime_events_lock_sleep_forced()) {
        return 0u;
    }
#endif
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

static uint8_t runtime_sleep_enter_stop_and_recover(runtime_flow_ctx_t *ctx)
{
    runtime_stop_hooks_t stop_hooks;

    if (!ctx || !ctx->can_protocol_started || !ctx->can_protocol_start_ms) return 0u;

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
    stop_hooks.note_periodic_wakeup_cycle = runtime_sleep_note_periodic_rtc_wakeup_cycle;
#else
    stop_hooks.rtc_wakeup_start = NULL;
    stop_hooks.rtc_wakeup_stop = NULL;
    stop_hooks.note_periodic_wakeup_cycle = NULL;
#endif
    runtime_stop_enter_and_wait(&stop_hooks);

    can_ambient_note_stop_wakeup(runtime_stop_get_wakeup_source());
    runtime_debug_hooks_note_stop_wake(runtime_stop_get_wakeup_source(), HAL_GetTick());
    handle_pwm_resume_after_stop();
    can_ambient_exit_sleep();

    *ctx->can_protocol_started = 0u;
    *ctx->can_protocol_start_ms = 0u;
    can_ambient_reset_oem_received();
    runtime_flow_set_state(ctx, RUNTIME_WAKE_RECOVER);
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    runtime_events_clear_lock_sleep_forced();
#endif
    return 1u;
}

static uint8_t runtime_handle_sleep_state(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip, uint32_t now)
{
    uint32_t fade_elapsed;

    if (!ctx || !ctx->sleep_fade_active || !ctx->sleep_fade_start_ms || !ctx->sleep_fade_start_brightness ||
        !ctx->base_scene) {
        return 0u;
    }

#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    if (runtime_events_lock_sleep_forced()) {
        uint8_t unlock_ev = can_ambient_consume_unlock_event();
        uint8_t door_open = can_ambient_is_any_door_open_for_board();
        if (unlock_ev || door_open) {
            runtime_events_clear_lock_sleep_forced();
            runtime_sleep_cancel(ctx, main_strip);
            return 0u;
        }
    }
#endif

    /* Reverse should never trigger or continue sleep fade on bench/driving context.
     * If reverse arrives during fade, immediately restore ACTIVE brightness path. */
    if (can_ambient_is_reverse_active()) {
        if (*ctx->sleep_fade_active) {
            runtime_sleep_cancel(ctx, main_strip);
        }
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
        if (runtime_sleep_enter_stop_and_recover(ctx)) {
            return 1u;
        }
        runtime_flow_set_state(ctx, RUNTIME_ACTIVE);
        return 0u;
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
    {
        uint8_t r, g, b;
        if (runtime_debug_hooks_get_strip_overlay(now, &r, &g, &b)) {
            const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
            if (zm && zm->strip && zm->count > 0u) {
                uint16_t i;
                for (i = 0u; i < zm->count; ++i) {
                    led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
                }
            }
        }
    }
    board_dispatch_led_render_all();
}

static void runtime_update_handle_pwm(const runtime_flow_ctx_t *ctx, uint32_t now, uint32_t dt)
{
    uint8_t handle_pwm_enable = 0u;
    uint8_t handle_pwm_pct = 100u;

    if (can_ambient_handle_request_active()) {
#if AMB_ENABLE_SLEEP_MODE
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

uint8_t runtime_flow_dispatch_state(runtime_flow_ctx_t *ctx,
                                    led_runtime_strip_t *main_strip,
                                    uint32_t dt,
                                    uint8_t oem_received)
{
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
        {
            uint8_t ignition_on = can_ambient_is_ignition_on();
            /* KL15 has startup priority: allow ACTIVE transition even when OEM 0x325
             * is not yet present/fresh. */
            uint8_t startup_source_ready = (uint8_t)(oem_received || ignition_on);
        if (startup_source_ready) {
            uint8_t wake_recover = *ctx->wait_oem_after_wake;
            uint8_t allow_start = 1u;
            uint8_t door_gated_start = 0u;
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE && AMB_ENABLE_START_GATE_DOOR_OPEN
            uint8_t start_unlock_signature_direct = 0u;
#endif
#if AMB_ENABLE_START_GATE_DOOR_OPEN
            if (ignition_on) {
                allow_start = 1u;
                door_gated_start = 0u;
            } else {
                allow_start = can_ambient_is_any_door_open_for_board();
                door_gated_start = allow_start;
            }
#endif
            if (allow_start) {
#if AMB_ENABLE_START_GATE_DOOR_OPEN
                /* This door-open edge was used as startup gate. Do not replay it again
                 * as a separate door-open event in the first ACTIVE tick, otherwise it
                 * immediately overrides welcome signature due to higher event priority. */
                if (door_gated_start) {
                    (void)can_ambient_consume_door_open_event_for_board();
                }
#endif
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
                uint8_t unlock_trigger = 0u;
#if AMB_ENABLE_START_GATE_DOOR_OPEN
                /* Door-gated start means first visible activation happens on door-open.
                 * Re-arm welcome signature here to preserve "first-open after sleep" accent. */
                (void)wake_recover;
                if (door_gated_start) {
                    runtime_events_clear_unlock_signature_cooldown();
                    /* Start unlock signature explicitly after ACTIVE transition, do not rely
                     * on deferred arm-only path for first door-open after sleep. */
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE && AMB_ENABLE_START_GATE_DOOR_OPEN
                    start_unlock_signature_direct = 1u;
#endif
                }
                unlock_trigger = 0u;
#else
#if AMB_ENABLE_UNLOCK_TRIGGER_WAKE_RECOVER
                unlock_trigger = wake_recover;
#endif
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
                if (can_ambient_consume_unlock_event()) {
                    unlock_trigger = 1u;
                }
#endif
#endif
                runtime_events_arm_unlock_signature(unlock_trigger, now);
#endif
                *ctx->wait_oem_after_wake = 0u;
                *ctx->wait_oem_enter_ms = 0u;
                /* Drop stale lock edge collected during WAIT_OEM gate; otherwise
                 * first ACTIVE frame may fire false goodbye/sleep. */
                can_ambient_clear_lock_event_pending();
                runtime_activate_from_oem(ctx, main_strip, wake_recover, door_gated_start);
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE && AMB_ENABLE_START_GATE_DOOR_OPEN
                if (start_unlock_signature_direct) {
                    event_scene_t scene;
                    event_scene_build_unlock_signature(&scene);
                    (void)event_layer_start(&scene, now);
                }
#endif
                return 0u;
            }
        }
        }
#if AMB_ENABLE_SLEEP_MODE
        if (*ctx->wait_oem_after_wake && *ctx->wait_oem_enter_ms != 0u) {
            uint32_t wait_ms = (now >= *ctx->wait_oem_enter_ms)
                                   ? (now - *ctx->wait_oem_enter_ms)
                                   : ((UINT32_MAX - *ctx->wait_oem_enter_ms) + now + 1u);
            /* Keep WAKE_RECOVER->WAIT_OEM resleep timer from kicking in while
             * standalone handle request is active. This avoids cyclic
             * STOP/wake loops with door closed. */
            if (wait_ms >= AMB_WAIT_OEM_RESLEEP_MS && !can_ambient_handle_request_active()) {
                if (runtime_sleep_enter_stop_and_recover(ctx)) {
                    return 1u;
                }
            }
        }
#endif
        led_runtime_power_set(0u);
        if (main_strip) {
            led_runtime_release_brightness(main_strip);
        }
        /* Door-handle PWM is intentionally decoupled from strip ACTIVE and
         * door-open gates: the OEM handle source owns its state. */
        if (can_ambient_handle_request_active() &&
            !can_ambient_is_sleeping()) {
            handle_pwm_set_brightness_pct(100u);
            handle_pwm_set_enabled(1u);
        } else {
            handle_pwm_set_enabled(0u);
        }
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
    return 1u;
}

uint8_t runtime_flow_handle_active_state(runtime_flow_ctx_t *ctx,
                                         led_runtime_strip_t *main_strip,
                                         uint32_t now,
                                         uint32_t dt,
                                         uint8_t oem_received)
{
    runtime_inputs_t inputs;

    if (!ctx || !ctx->director || !ctx->base_scene || !ctx->oem_color ||
        !ctx->can_protocol_started || !ctx->can_protocol_start_ms) {
        return 0u;
    }

    /* Defensive recovery: in ACTIVE path brightness must come from scene/runtime dimming.
     * If a previous sleep fade left forced-brightness latched, OEM brightness appears frozen. */
    if (main_strip) {
#if AMB_ENABLE_SLEEP_MODE
        if (!(ctx->sleep_fade_active && *ctx->sleep_fade_active)) {
            led_runtime_release_brightness(main_strip);
        }
#else
        led_runtime_release_brightness(main_strip);
#endif
    }

    if (main_strip) {
        can_ambient_fill_runtime_inputs(&inputs);
        director_update(ctx->director, main_strip, ctx->base_scene, &inputs);
        runtime_events_maybe_arm_unlock_from_can(now);
        runtime_events_maybe_trigger_unlock_signature(ctx, now);
        runtime_events_maybe_trigger_door_open(now);
        runtime_events_maybe_trigger_lock_goodbye(ctx, main_strip);
        runtime_events_maybe_trigger_hvac_temp(now);

        if (inputs.can_state.oem_color < OEM_COLOR_MAX) {
            *ctx->oem_color = (oem_color_id_t)inputs.can_state.oem_color;
        } else {
            *ctx->oem_color = OEM_COLOR_AMBER;
        }
    }

#if AMB_ENABLE_SLEEP_MODE
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
#if AMB_ENABLE_SLEEP_MODE
                                  *ctx->sleep_fade_active
#else
                                  0u
#endif
    );

    return 0u;
}
