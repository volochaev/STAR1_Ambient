#pragma once

#include <stdint.h>

#include "ambient_state.h"
#include "base_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    can_state_t       can_state;
    can_bsm_state_t       bsm;
    motion_profile_t  motion_profile;
    theme_id_t         resolved_theme;
    uint32_t              now_ms;
} runtime_inputs_t;

typedef struct {
    can_state_t       can_state;
    can_bsm_state_t       bsm;
    motion_profile_t  motion_profile;
    theme_id_t         resolved_theme;
    float                 active_dimming;
    float                 target_dimming;
    float                 smoothed_dimming;
    float                 post_smoothed_dimming;
    float                 last_non_outro_dimming;
    uint32_t              smoothed_last_ms;
    uint8_t               post_smooth_initialized;
    uint8_t               same_bank_as_player;
    uint8_t               theme_change_requires_outro;
    uint8_t               hold_dimming;
} director_runtime_t;

void runtime_state_init(director_runtime_t *state);
void runtime_state_step(director_runtime_t *state,
                            const runtime_inputs_t *inputs,
                            const base_scene_t *pl);

#ifdef __cplusplus
}
#endif
