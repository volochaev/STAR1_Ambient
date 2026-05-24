#include "runtime_state.h"

#include "ambient_config.h"
#include "brightness_pipeline.h"
#include "runtime_dimming_policy.h"
#include <string.h>

static float runtime_input_brightness_norm(const runtime_inputs_t *inputs)
{
    float br;

    if (!inputs) return 1.0f;

    if (inputs->ambient_state.valid) {
        br = inputs->ambient_state.brightness_norm;
    } else {
        /* M1 fallback while legacy path is still present in parallel. */
        br = inputs->can_state.oem_brightness;
    }

    if (br < 0.0f) br = 0.0f;
    if (br > 1.0f) br = 1.0f;
    return br;
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
    uint32_t dt_ms;
    runtime_dimming_policy_eval_t policy;

    if (!state || !inputs || !pl) return;

    state->can_state = inputs->can_state;
    state->bsm = inputs->bsm;
    state->motion_profile = inputs->motion_profile;

    runtime_dimming_policy_eval(pl, &policy);

    target_dimming = brightness_pipeline_map_oem_brightness(runtime_input_brightness_norm(inputs));
    state->hold_dimming = policy.hold_dimming;
    if (!state->hold_dimming) {
        state->last_non_outro_dimming = target_dimming;
    } else {
        target_dimming = state->last_non_outro_dimming;
    }

    if (!state->hold_dimming) {
        if (state->smoothed_last_ms == 0u) {
            state->smoothed_last_ms = inputs->now_ms;
            state->smoothed_dimming = target_dimming;
        } else {
            dt_ms = (inputs->now_ms >= state->smoothed_last_ms)
                  ? (inputs->now_ms - state->smoothed_last_ms)
                  : 0u;
            state->smoothed_last_ms = inputs->now_ms;
            state->smoothed_dimming = brightness_pipeline_slew_dimming(state->smoothed_dimming,
                                                                       target_dimming,
                                                                       dt_ms);
        }
    } else {
        state->smoothed_last_ms = inputs->now_ms;
    }

    if (!state->post_smooth_initialized) {
        state->post_smoothed_dimming = state->smoothed_dimming;
        state->post_smooth_initialized = 1u;
    } else {
        state->post_smoothed_dimming = brightness_pipeline_post_smooth(state->post_smoothed_dimming,
                                                                        state->smoothed_dimming,
                                                                        (uint8_t)(pl->stage == BASE_SCENE_ACTIVE));
    }

    state->target_dimming = target_dimming;
    state->active_dimming = state->post_smoothed_dimming;
}
