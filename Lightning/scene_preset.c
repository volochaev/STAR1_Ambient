#include "scene_preset.h"
#include "lighting_profile_registry.h"

void scene_preset_resolve(motion_profile_t profile, uint8_t night_mode, scene_preset_t *out)
{
    lighting_profile_registry_resolve_preset(profile, night_mode, out);
}
