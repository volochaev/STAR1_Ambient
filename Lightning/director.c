#include "director.h"

#include "ambient_config.h"

/* Keep last scene stage for transition-triggered effects. */
static void director_update_stage_bookkeeping(director_t *director, const base_scene_t *pl)
{
    if (!director || !pl) return;
    director->last_stage = pl->stage;
}

/* Arm one-shot welcome event after idle->active pipeline startup. */
static void director_arm_welcome_scene(director_t *director)
{
#if AMB_ENABLE_WELCOME_EVENT_SCENE
    if (!director) return;
    director->welcome_scene_armed = 1u;
#else
    (void)director;
#endif
}

/* Start welcome event once base scene bridge is fully completed. */
static void director_maybe_trigger_welcome_scene(director_t *director,
                                                 const runtime_inputs_t *inputs,
                                                 const base_scene_t *pl)
{
#if AMB_ENABLE_WELCOME_EVENT_SCENE
    event_scene_t scene;
    if (!director || !inputs || !pl) return;
    if (!director->welcome_scene_armed) return;
    if (director->last_stage != BASE_SCENE_BRIDGE || pl->stage != BASE_SCENE_ACTIVE) return;
    if (pl->crossfade_active) return;

    event_scene_build_dual_accent_auto(&scene, AMB_WELCOME_EVENT_HOLD_MS);
    (void)event_layer_start(&scene, inputs->now_ms);
    director->welcome_scene_armed = 0u;
#else
    (void)director;
    (void)inputs;
    (void)pl;
#endif
}

/* Initialize director orchestration state and runtime state machine. */
void director_init(director_t *director)
{
    if (!director) return;
    director->welcome_scene_armed = 0u;
    director->last_stage = BASE_SCENE_IDLE;
    runtime_state_init(&director->runtime);
}

/* Main orchestrator step: runtime policy, scene prep, and event triggers. */
void director_update(director_t *director,
                     led_runtime_strip_t *ws,
                     base_scene_t *pl,
                     const runtime_inputs_t *inputs)
{
    if (!director || !pl || !inputs) return;

    runtime_state_step(&director->runtime, inputs, pl);

    pl->runtime_dimming = director->runtime.active_dimming;
    base_scene_refresh_brightness(pl);

    if (ws && pl->stage == BASE_SCENE_IDLE) {
        base_scene_start_intro(ws, pl);
        director_arm_welcome_scene(director);
    }

    director_maybe_trigger_welcome_scene(director, inputs, pl);
    g_night_mode_state = director->runtime.can_state.night_mode;
    director_update_stage_bookkeeping(director, pl);
}
