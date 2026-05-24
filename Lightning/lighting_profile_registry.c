#include "lighting_profile_registry.h"

static const scene_preset_t k_preset_table[] = {
    [SCENE_PRESET_CLASSIC] = {
        .id = SCENE_PRESET_CLASSIC,
        .accent_gain = 1.00f,
        .hvac_wave_gain_mul = 1.00f,
        .welcome_hold_mul = 1.00f,
        .temporal_scale = 1.00f,
        .contrast_gain = 1.00f,
        .neutral_mix_bias = 0.00f,
        .overlay_gain_mul = 1.00f,
        .motion_tint_mul = 1.00f,
        .energy_sat_mul = 1.00f,
    },
    [SCENE_PRESET_CALM] = {
        .id = SCENE_PRESET_CALM,
        .accent_gain = 0.90f,
        .hvac_wave_gain_mul = 0.88f,
        .welcome_hold_mul = 1.10f,
        .temporal_scale = 0.88f,
        .contrast_gain = 0.94f,
        .neutral_mix_bias = 0.05f,
        .overlay_gain_mul = 0.92f,
        .motion_tint_mul = 0.97f,
        .energy_sat_mul = 0.94f,
    },
    [SCENE_PRESET_SPORT] = {
        .id = SCENE_PRESET_SPORT,
        .accent_gain = 1.10f,
        .hvac_wave_gain_mul = 1.15f,
        .welcome_hold_mul = 0.92f,
        .temporal_scale = 1.16f,
        .contrast_gain = 1.10f,
        .neutral_mix_bias = -0.05f,
        .overlay_gain_mul = 1.10f,
        .motion_tint_mul = 1.04f,
        .energy_sat_mul = 1.08f,
    },
    [SCENE_PRESET_LOUNGE] = {
        .id = SCENE_PRESET_LOUNGE,
        .accent_gain = 0.86f,
        .hvac_wave_gain_mul = 0.85f,
        .welcome_hold_mul = 1.18f,
        .temporal_scale = 0.84f,
        .contrast_gain = 0.92f,
        .neutral_mix_bias = 0.08f,
        .overlay_gain_mul = 0.88f,
        .motion_tint_mul = 0.96f,
        .energy_sat_mul = 0.92f,
    },
};

const scene_preset_t *lighting_profile_registry_get_preset(scene_preset_id_t id)
{
    const uint32_t count = (uint32_t)(sizeof(k_preset_table) / sizeof(k_preset_table[0]));
    if ((uint32_t)id >= count) return 0;
    return &k_preset_table[(uint32_t)id];
}

void lighting_profile_registry_resolve_preset(motion_profile_t profile,
                                              uint8_t night_mode,
                                              scene_preset_t *out)
{
    const scene_preset_t *preset;

    if (!out) return;

    if (night_mode && profile != MOTION_PROFILE_SPORT) {
        preset = lighting_profile_registry_get_preset(SCENE_PRESET_LOUNGE);
    } else if (profile == MOTION_PROFILE_CALM) {
        preset = lighting_profile_registry_get_preset(SCENE_PRESET_CALM);
    } else if (profile == MOTION_PROFILE_SPORT) {
        preset = lighting_profile_registry_get_preset(SCENE_PRESET_SPORT);
    } else {
        preset = lighting_profile_registry_get_preset(SCENE_PRESET_CLASSIC);
    }

    if (!preset) {
        *out = k_preset_table[SCENE_PRESET_CLASSIC];
        return;
    }
    *out = *preset;
}
