/**
 * @file scene_color_model.c
 * @brief Map ambient color IDs to scene primary/accent color model.
 */
#include "scene_color_model.h"
#include "oem_color_wheel.h"

/* Linear RGB interpolation helper. */
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

/* Convert to luma-neutral and blend back to preserve some chroma. */
static scene_rgb8_t rgb_luma(scene_rgb8_t c, float keep_color)
{
    float y;
    scene_rgb8_t gray;
    if (keep_color < 0.0f) keep_color = 0.0f;
    if (keep_color > 1.0f) keep_color = 1.0f;
    y = 0.299f * (float)c.r + 0.587f * (float)c.g + 0.114f * (float)c.b;
    gray.r = (uint8_t)(y + 0.5f);
    gray.g = (uint8_t)(y + 0.5f);
    gray.b = (uint8_t)(y + 0.5f);
    return rgb_mix(gray, c, keep_color);
}

/* Scale RGB color by normalized factor. */
static scene_rgb8_t rgb_scale(scene_rgb8_t c, float k)
{
    scene_rgb8_t out;
    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;
    out.r = (uint8_t)((float)c.r * k + 0.5f);
    out.g = (uint8_t)((float)c.g * k + 0.5f);
    out.b = (uint8_t)((float)c.b * k + 0.5f);
    return out;
}

/* Build runtime scene color model from unified ambient state snapshot. */
void scene_color_model_from_ambient(const ambient_state_snapshot_t *ambient_state,
                                    motion_profile_t motion_profile,
                                    uint8_t night_mode,
                                    scene_color_model_t *out)
{
    uint8_t color_id = 0u;
    uint8_t idx;
    float energy = 0.55f;
    scene_rgb8_t primary;
    scene_rgb8_t a, b;

    if (!out) return;
    out->valid = 0u;
    out->energy = 0.0f;
    out->primary = (scene_rgb8_t){0u, 0u, 0u};
    out->accent_a = (scene_rgb8_t){0u, 0u, 0u};
    out->accent_b = (scene_rgb8_t){0u, 0u, 0u};
    out->neutral = (scene_rgb8_t){0u, 0u, 0u};

    if (!ambient_state || !ambient_state->valid) {
        return;
    }

    color_id = ambient_state->color_id;
    idx = (uint8_t)(color_id % oem_color_wheel_count());
    primary = oem_color_wheel_at(idx);
    a = oem_color_wheel_at((uint8_t)(idx + 1u));
    b = oem_color_wheel_at((uint8_t)(idx + oem_color_wheel_count() - 1u));

    if (motion_profile == MOTION_PROFILE_CALM) energy = 0.35f;
    else if (motion_profile == MOTION_PROFILE_SPORT) energy = 0.82f;
    if (night_mode) energy *= 0.78f;

    out->primary = primary;
    out->accent_a = rgb_mix(primary, a, 0.42f);
    out->accent_b = rgb_mix(primary, b, 0.42f);
    out->neutral = rgb_luma(primary, 0.22f);
    out->energy = energy;
    out->valid = 1u;
}

void scene_color_entity_from_model(const scene_color_model_t *model,
                                   scene_color_entity_t *out)
{
    if (!out) return;

    out->base = (scene_rgb8_t){0u, 0u, 0u};
    out->accent_warm = (scene_rgb8_t){0u, 0u, 0u};
    out->accent_cool = (scene_rgb8_t){0u, 0u, 0u};
    out->neutral_soft = (scene_rgb8_t){0u, 0u, 0u};
    out->safety_alert = (scene_rgb8_t){255u, 40u, 0u};
    out->guidance_line = (scene_rgb8_t){96u, 180u, 255u};
    out->energy = 0.0f;
    out->valid = 0u;

    if (!model || !model->valid) {
        return;
    }

    out->base = model->primary;
    out->accent_warm = rgb_mix(model->primary, model->accent_a, 0.55f);
    out->accent_cool = rgb_mix(model->primary, model->accent_b, 0.55f);
    out->neutral_soft = rgb_scale(model->neutral, 0.85f);
    out->energy = model->energy;
    out->valid = 1u;
}

void scene_color_entity_from_ambient(const ambient_state_snapshot_t *ambient_state,
                                     motion_profile_t motion_profile,
                                     uint8_t night_mode,
                                     scene_color_entity_t *out)
{
    scene_color_model_t model;

    if (!out) return;
    scene_color_model_from_ambient(ambient_state, motion_profile, night_mode, &model);
    scene_color_entity_from_model(&model, out);
}
