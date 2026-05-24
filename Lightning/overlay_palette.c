#include "overlay_palette.h"

static scene_rgb8_t rgb_mix(scene_rgb8_t a, scene_rgb8_t b, float t)
{
    scene_rgb8_t out;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    out.r = (uint8_t)((float)a.r * (1.0f - t) + (float)b.r * t + 0.5f);
    out.g = (uint8_t)((float)a.g * (1.0f - t) + (float)b.g * t + 0.5f);
    out.b = (uint8_t)((float)a.b * (1.0f - t) + (float)b.b * t + 0.5f);
    return out;
}

void overlay_palette_build(const scene_color_entity_t *entity, overlay_palette_t *out)
{
    scene_rgb8_t fallback_warm = {255u, 128u, 34u};
    scene_rgb8_t fallback_cool = {32u, 120u, 255u};
    scene_rgb8_t fallback_safety = {255u, 24u, 0u};
    scene_rgb8_t fallback_guidance = {96u, 180u, 255u};

    if (!out) return;
    out->warm = fallback_warm;
    out->cool = fallback_cool;
    out->parking = rgb_mix(fallback_warm, fallback_safety, 0.25f);
    out->safety = fallback_safety;
    out->guidance = fallback_guidance;

    if (!entity || !entity->valid) return;

    out->warm = rgb_mix(entity->accent_warm, entity->base, 0.30f);
    out->cool = rgb_mix(entity->accent_cool, entity->guidance_line, 0.30f);
    out->parking = rgb_mix(entity->accent_warm, entity->neutral_soft, 0.20f);
    out->safety = entity->safety_alert;
    out->guidance = entity->guidance_line;
}
