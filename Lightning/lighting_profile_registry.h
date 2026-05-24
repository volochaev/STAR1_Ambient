#pragma once

#include <stdint.h>

#include "scene_preset.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Resolve active preset using canonical runtime mapping policy. */
void lighting_profile_registry_resolve_preset(motion_profile_t profile,
                                              uint8_t night_mode,
                                              scene_preset_t *out);

/** Get immutable preset template by id (NULL for invalid id). */
const scene_preset_t *lighting_profile_registry_get_preset(scene_preset_id_t id);

#ifdef __cplusplus
}
#endif
