#pragma once

#include <stdint.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCENE_PRESET_CLASSIC = 0u,
    SCENE_PRESET_CALM = 1u,
    SCENE_PRESET_SPORT = 2u,
    SCENE_PRESET_LOUNGE = 3u,
} scene_preset_id_t;

typedef struct {
    scene_preset_id_t id;
    float accent_gain;
    float hvac_wave_gain_mul;
    float welcome_hold_mul;
    float temporal_scale;
    float contrast_gain;
    float neutral_mix_bias;
    float overlay_gain_mul;
    float motion_tint_mul;
    float energy_sat_mul;
} scene_preset_t;

/** Resolve active scene preset from current runtime profile/night context. */
void scene_preset_resolve(motion_profile_t profile, uint8_t night_mode, scene_preset_t *out);

#ifdef __cplusplus
}
#endif
