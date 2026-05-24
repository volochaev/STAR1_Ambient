#include "runtime_events.h"

#include "ambient.h"
#include "ambient_config.h"
#include "event_layer.h"
#include "led_runtime.h"

#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
static uint8_t s_unlock_signature_armed = 0u;
static uint32_t s_last_unlock_signature_ms = 0u;
#endif
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
static uint8_t s_lock_sleep_forced = 0u;
#endif

void runtime_events_reset(void)
{
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
    s_unlock_signature_armed = 0u;
    s_last_unlock_signature_ms = 0u;
#endif
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    s_lock_sleep_forced = 0u;
#endif
}

uint8_t runtime_events_lock_sleep_forced(void)
{
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    return s_lock_sleep_forced;
#else
    return 0u;
#endif
}

void runtime_events_clear_lock_sleep_forced(void)
{
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    s_lock_sleep_forced = 0u;
#endif
}

void runtime_events_clear_unlock_signature_cooldown(void)
{
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
    s_last_unlock_signature_ms = 0u;
#endif
}

void runtime_events_arm_unlock_signature(uint8_t trigger, uint32_t now)
{
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
    if (!trigger) return;
    if (s_last_unlock_signature_ms != 0u &&
        (now - s_last_unlock_signature_ms) < AMB_UNLOCK_SIGNATURE_COOLDOWN_MS) {
        return;
    }
    s_unlock_signature_armed = 1u;
#else
    (void)trigger;
    (void)now;
#endif
}

void runtime_events_maybe_arm_unlock_from_can(uint32_t now)
{
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE && AMB_ENABLE_UNLOCK_TRIGGER_EIS301
#if AMB_ENABLE_START_GATE_DOOR_OPEN
    (void)now;
#else
    if (can_ambient_consume_unlock_event()) {
        runtime_events_arm_unlock_signature(1u, now);
    }
#endif
#else
    (void)now;
#endif
}

void runtime_events_maybe_trigger_unlock_signature(runtime_flow_ctx_t *ctx, uint32_t now)
{
#if AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
    if (!ctx || !ctx->base_scene) return;
    if (!s_unlock_signature_armed) return;
    if (ctx->base_scene->stage != BASE_SCENE_ACTIVE || ctx->base_scene->crossfade_active) return;
    if (s_last_unlock_signature_ms != 0u &&
        (now - s_last_unlock_signature_ms) < AMB_UNLOCK_SIGNATURE_COOLDOWN_MS) {
        s_unlock_signature_armed = 0u;
        return;
    }

    {
        event_scene_t scene;
        event_scene_build_unlock_signature(&scene);
        if (event_layer_start(&scene, now)) {
            s_last_unlock_signature_ms = now;
            s_unlock_signature_armed = 0u;
        }
    }
#else
    (void)ctx;
    (void)now;
#endif
}

void runtime_events_maybe_trigger_door_open(uint32_t now)
{
#if AMB_ENABLE_DOOR_OPEN_EVENT_SCENE
    uint8_t door_ev = can_ambient_consume_door_open_event_for_board();
    if (!door_ev) return;
    {
        event_scene_t scene;
        /* Door event already has dedicated choreography scene.
         * Additional context-hold on strip causes perceived fade/restart on door-open. */
        event_scene_build_door_open(&scene, (uint8_t)(door_ev == 2u));
        (void)event_layer_start(&scene, now);
    }
#else
    (void)now;
#endif
}

void runtime_events_maybe_trigger_lock_goodbye(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip)
{
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    if (!ctx || !ctx->base_scene || !main_strip) return;
    /* Defensive gate: goodbye only from fresh EIS_A2 lock source.
     * Drops stale/ghost lock pending states that are not tied to recent EIS traffic. */
    if (!can_ambient_is_lock_source_recent()) {
        (void)can_ambient_consume_lock_event();
        return;
    }
    if (!can_ambient_consume_lock_event()) return;
    if (can_ambient_is_any_door_open_for_board()) return;
    s_lock_sleep_forced = 1u;
    can_ambient_request_sleep_lock();
    if (ctx->base_scene->stage == BASE_SCENE_ACTIVE || ctx->base_scene->stage == BASE_SCENE_BRIDGE) {
        base_scene_start_outro(main_strip, ctx->base_scene);
    }
#else
    (void)ctx;
    (void)main_strip;
#endif
}

void runtime_events_maybe_trigger_hvac_temp(uint32_t now)
{
#if AMB_ENABLE_HVAC_TEMP_EVENT_SCENE
    int8_t trend = can_ambient_consume_hvac_temp_trend_for_board();

    if (trend == 0) return;
    if (trend > 0) {
        event_layer_start_hvac_wave_auto(1u, now);
    } else {
        event_layer_start_hvac_wave_auto(0u, now);
    }
#else
    (void)now;
#endif
}
