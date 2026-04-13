#include "runtime_state.h"

#include "features.h"
#include "themes.h"
#include <math.h>
#include <string.h>

static float smoothstep5(float x)
{
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

static float slew_dimming(float current, float target, uint32_t now_ms, uint32_t *last_ms)
{
    uint32_t dt;
    uint32_t tau_ms;
    float alpha;
    float eased_alpha;

    if (!last_ms) return target;
    if (*last_ms == 0u) {
        *last_ms = now_ms;
        return target;
    }
    dt = (now_ms >= *last_ms) ? (now_ms - *last_ms) : 0u;
    if (dt > 100u) dt = 100u;
    *last_ms = now_ms;

    tau_ms = (target >= current) ? AMB_DIMMING_ATTACK_MS : AMB_DIMMING_RELEASE_MS;
    if (tau_ms == 0u) return target;

    alpha = (float)dt / ((float)tau_ms + (float)dt);
    eased_alpha = smoothstep5(alpha);
    current += (target - current) * eased_alpha;

    if (fabsf(target - current) < 0.0005f) {
        current = target;
    }

    return current;
}

static float map_oem_brightness(float oem)
{
    float x = oem;
    float y;
    if (x <= 0.0f) return 0.0f;
    if (x > 1.0f) x = 1.0f;

    y = powf(x, AMB_BRIGHTNESS_EXP);
    y = AMB_BRIGHTNESS_FLOOR + (AMB_BRIGHTNESS_CEIL - AMB_BRIGHTNESS_FLOOR) * y;
    if (y < 0.0f) y = 0.0f;
    if (y > 1.0f) y = 1.0f;
    return y;
}

void runtime_state_init(director_runtime_t *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->active_dimming = 1.0f;
    state->target_dimming = 1.0f;
    state->smoothed_dimming = 1.0f;
    state->post_smoothed_dimming = 1.0f;
    state->last_non_outro_dimming = 1.0f;
}

void runtime_state_step(director_runtime_t *state,
                            const runtime_inputs_t *inputs,
                            const base_scene_t *pl)
{
    float target_dimming;
    float k;

    if (!state || !inputs || !pl) return;

    state->can_state = inputs->can_state;
    state->bsm = inputs->bsm;
    state->motion_profile = inputs->motion_profile;
    state->resolved_theme = inputs->resolved_theme;

    state->same_bank_as_player = theme_same_bank(pl->theme, inputs->resolved_theme);
    state->theme_change_requires_outro =
        (uint8_t)((pl->stage == BASE_SCENE_ACTIVE)
               && !state->same_bank_as_player
               && (pl->theme != inputs->resolved_theme));

    target_dimming = map_oem_brightness(inputs->can_state.oem_brightness);
    state->hold_dimming = (uint8_t)((pl->stage == BASE_SCENE_OUTRO) || state->theme_change_requires_outro);
    if (!state->hold_dimming) {
        state->last_non_outro_dimming = target_dimming;
    } else {
        target_dimming = state->last_non_outro_dimming;
    }

    if (!state->hold_dimming) {
        state->smoothed_dimming = slew_dimming(state->smoothed_dimming,
                                               target_dimming,
                                               inputs->now_ms,
                                               &state->smoothed_last_ms);
    } else {
        state->smoothed_last_ms = inputs->now_ms;
    }

    k = AMB_DIMMING_POST_SMOOTH;
    if (pl->stage != BASE_SCENE_ACTIVE) {
        k += 0.08f;
    }
    if (k < 0.0f) k = 0.0f;
    if (k > 0.98f) k = 0.98f;

    if (!state->post_smooth_initialized) {
        state->post_smoothed_dimming = state->smoothed_dimming;
        state->post_smooth_initialized = 1u;
    } else {
        state->post_smoothed_dimming = state->post_smoothed_dimming * k
                                     + state->smoothed_dimming * (1.0f - k);
    }

    state->target_dimming = target_dimming;
    state->active_dimming = state->post_smoothed_dimming;
}
