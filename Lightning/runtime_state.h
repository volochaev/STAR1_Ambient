#pragma once

#include <stdint.h>

#include "ambient_state.h"
#include "ambient_state_store.h"
#include "base_scene.h"
#include "scene_color_model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    can_state_t       can_state;
    ambient_state_snapshot_t ambient_state;
    scene_color_model_t scene_colors;
    can_bsm_state_t    bsm;
    motion_profile_t  motion_profile;
    uint32_t          now_ms;
} runtime_inputs_t;

/** Director-local runtime state (filtered dimming + cached CAN slices). */
typedef struct {
    can_state_t       can_state;
    can_bsm_state_t       bsm;
    motion_profile_t  motion_profile;
    float                 active_dimming;
    float                 target_dimming;
    float                 smoothed_dimming;
    float                 post_smoothed_dimming;
    float                 last_non_outro_dimming;
    uint32_t              smoothed_last_ms;
    uint8_t               post_smooth_initialized;
    uint8_t               hold_dimming;
} director_runtime_t;

/** Initialize director runtime state. */
void runtime_state_init(director_runtime_t *state);
/** Advance director runtime filters and derived dimming state. */
void runtime_state_step(director_runtime_t *state,
                            const runtime_inputs_t *inputs,
                            const base_scene_t *pl);

#ifdef __cplusplus
}
#endif
