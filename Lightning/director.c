#include "director.h"

#include "features.h"

static uint8_t director_can_fast_switch_bank(director_t *director, const base_scene_t *pl)
{
#if AMB_ENABLE_FAST_BANK_SWITCH
    uint32_t now_ms;
    if (!director || !pl) return 0u;
    if (pl->stage != BASE_SCENE_ACTIVE) return 0u;
    if (pl->crossfade_active) return 0u;
    now_ms = HAL_GetTick();
    if ((now_ms - director->last_fast_bank_switch_ms) < AMB_FAST_BANK_SWITCH_DEBOUNCE_MS) {
        return 0u;
    }
    director->last_fast_bank_switch_ms = now_ms;
    return 1u;
#else
    (void)director;
    (void)pl;
    return 0u;
#endif
}

static void director_update_stage_bookkeeping(director_t *director, const base_scene_t *pl)
{
    if (!director || !pl) return;
    director->last_stage = pl->stage;
}

static void director_arm_welcome_scene(director_t *director)
{
#if AMB_ENABLE_WELCOME_EVENT_SCENE
    if (!director) return;
    director->welcome_scene_armed = 1u;
#else
    (void)director;
#endif
}

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

    event_scene_build_dual_accent(&scene,
                                      60u, 122u, 255u,
                                      255u, 124u, 18u,
                                      AMB_WELCOME_EVENT_HOLD_MS);
    (void)event_layer_start(&scene, inputs->now_ms);
    director->welcome_scene_armed = 0u;
#else
    (void)director;
    (void)inputs;
    (void)pl;
#endif
}

static void queue_pending_theme(director_t *director, theme_id_t theme)
{
    if (!director) return;
    director->pending_theme = theme;
    director->has_pending_theme = 1u;
}

static uint8_t process_pending_theme_idle(director_t *director, base_scene_t *pl, led_runtime_strip_t *ws)
{
    if (!director || !pl) return 0u;
    if (!director->has_pending_theme || pl->stage != BASE_SCENE_IDLE) return 0u;

    if (ws) {
        (void)base_scene_apply_theme_with_intro(pl, ws, director->pending_theme);
        director_arm_welcome_scene(director);
    } else {
        (void)base_scene_apply_theme_to_scene(pl, director->pending_theme);
    }
    director->has_pending_theme = 0u;
    return 1u;
}

static uint8_t process_pending_theme_scene_conflict(director_t *director,
                                                    base_scene_t *pl,
                                                    led_runtime_strip_t *ws)
{
    uint8_t same_bank;
    uint8_t skip_outro;

    if (!director || !pl) return 0u;
    if (!director->has_pending_theme || pl->stage != BASE_SCENE_ACTIVE || pl->theme == director->pending_theme) {
        return 0u;
    }

    same_bank = theme_same_bank(pl->theme, director->pending_theme);
#if (AMB_ENABLE_AUTO_ROTATE || AMB_ENABLE_FAST_BANK_SWITCH)
    skip_outro = (uint8_t)(same_bank || pl->crossfade_active);
#else
    skip_outro = same_bank;
#endif
    if (skip_outro) return 0u;

    if (ws) {
        base_scene_reset_fx_state(pl);
        base_scene_start_outro(ws, pl);
    }
    return 1u;
}

static void process_cross_bank_theme_change(director_t *director,
                                            base_scene_t *pl,
                                            led_runtime_strip_t *ws,
                                            theme_id_t theme_now)
{
    if (!director || !pl) return;
    if (!ws) {
        (void)base_scene_apply_theme_to_scene(pl, theme_now);
        return;
    }

    if (pl->stage == BASE_SCENE_ACTIVE) {
#if AMB_ENABLE_FAST_BANK_SWITCH
        if (director_can_fast_switch_bank(director, pl)) {
            director->has_pending_theme = 0u;
            base_scene_start_theme_crossfade(pl, theme_now);
            return;
        }
#endif
        queue_pending_theme(director, theme_now);
        base_scene_reset_fx_state(pl);
        base_scene_start_outro(ws, pl);
    } else if (pl->stage == BASE_SCENE_IDLE) {
        (void)base_scene_apply_theme_with_intro(pl, ws, theme_now);
        director_arm_welcome_scene(director);
    } else if (pl->stage == BASE_SCENE_OUTRO || pl->stage == BASE_SCENE_INTRO) {
        queue_pending_theme(director, theme_now);
    } else {
        base_scene_start_theme(ws, pl, theme_now);
    }
}

static void process_same_bank_theme_change(base_scene_t *pl, led_runtime_strip_t *ws, theme_id_t theme_now)
{
    if (!pl || !ws) return;
#if (AMB_ENABLE_AUTO_ROTATE || AMB_ENABLE_FAST_BANK_SWITCH)
    if (pl->stage == BASE_SCENE_ACTIVE && !pl->crossfade_active) {
        base_scene_reset_fx_state(pl);
        base_scene_start_theme(ws, pl, theme_now);
    }
#else
    if (pl->stage == BASE_SCENE_ACTIVE) {
        base_scene_reset_fx_state(pl);
        base_scene_start_theme(ws, pl, theme_now);
    }
#endif
}

void director_init(director_t *director)
{
    if (!director) return;
    director->pending_theme = 0u;
    director->has_pending_theme = 0u;
    director->welcome_scene_armed = 0u;
    director->last_fast_bank_switch_ms = 0u;
    director->last_stage = BASE_SCENE_IDLE;
    runtime_state_init(&director->runtime);
}

void director_update(director_t *director,
                         led_runtime_strip_t *ws,
                         base_scene_t *pl,
                         const runtime_inputs_t *inputs)
{
    uint8_t theme_really_changed;
    theme_id_t theme_now;

    if (!director || !pl || !inputs) return;

    runtime_state_step(&director->runtime, inputs, pl);
    theme_now = director->runtime.resolved_theme;

    pl->theme_dimming = director->runtime.active_dimming;
    base_scene_refresh_brightness(pl);

    if (process_pending_theme_idle(director, pl, ws)) {
        g_night_mode_state = director->runtime.can_state.night_mode;
        director_update_stage_bookkeeping(director, pl);
        return;
    }

    if (process_pending_theme_scene_conflict(director, pl, ws)) {
        g_night_mode_state = director->runtime.can_state.night_mode;
        director_update_stage_bookkeeping(director, pl);
        return;
    }

#if (AMB_ENABLE_AUTO_ROTATE || AMB_ENABLE_FAST_BANK_SWITCH)
    theme_really_changed = (uint8_t)((pl->theme != theme_now)
                                  && !pl->crossfade_active
                                  && !director->runtime.same_bank_as_player);
#else
    theme_really_changed = (uint8_t)((pl->theme != theme_now)
                                  && !director->runtime.same_bank_as_player);
#endif

    if (theme_really_changed) {
        process_cross_bank_theme_change(director, pl, ws, theme_now);
    } else if (pl->theme != theme_now && director->runtime.same_bank_as_player) {
        process_same_bank_theme_change(pl, ws, theme_now);
    }

    director_maybe_trigger_welcome_scene(director, inputs, pl);
    g_night_mode_state = director->runtime.can_state.night_mode;
    director_update_stage_bookkeeping(director, pl);
}
