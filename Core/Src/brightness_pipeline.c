/**
 * @file brightness_pipeline.c
 * @brief Brightness mapping and smoothing utilities for render pipeline.
 */
#include "brightness_pipeline.h"

#include "ambient_tuning.h"
#include <math.h>

/* Clamp float into normalized [0..1] range. */
float brightness_pipeline_clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

/* Per-zone brightness boost coefficients from tuning config. */
static float brightness_zone_boost(zone_id_t z)
{
    if (z == ZONE_STRIP) return AMB_ZONE_STRIP_BRIGHTNESS_BOOST;
    if (z == ZONE_HANDLE) return AMB_ZONE_HANDLE_BRIGHTNESS_BOOST;
    if (z == ZONE_STORAGE) return AMB_ZONE_STORAGE_BRIGHTNESS_BOOST;
    if (z == ZONE_FOOTWELL) return AMB_ZONE_FOOTWELL_BRIGHTNESS_BOOST;
    return 1.0f;
}

/* Map OEM brightness to perceptual curve with floor/ceiling constraints. */
float brightness_pipeline_map_oem_brightness(float oem)
{
    float x = oem;
    float y;

    if (x <= 0.0f) return 0.0f;
    if (x > 1.0f) x = 1.0f;

    y = powf(x, AMB_BRIGHTNESS_EXP);
    y = AMB_BRIGHTNESS_FLOOR + (AMB_BRIGHTNESS_CEIL - AMB_BRIGHTNESS_FLOOR) * y;
    return brightness_pipeline_clamp01(y);
}

/* First-order slew limiter for dimming transitions. */
float brightness_pipeline_slew_dimming(float current, float target, uint32_t dt_ms)
{
    uint32_t tau_ms;
    float alpha;

    if (dt_ms > 100u) dt_ms = 100u;
    tau_ms = (target >= current) ? AMB_DIMMING_ATTACK_MS : AMB_DIMMING_RELEASE_MS;
    if (tau_ms == 0u) return target;

    alpha = (float)dt_ms / ((float)tau_ms + (float)dt_ms);
    alpha = brightness_pipeline_clamp01(alpha);
    current += (target - current) * alpha;

    if (fabsf(target - current) < 0.0005f) {
        current = target;
    }
    return brightness_pipeline_clamp01(current);
}

/* Post-filter used to reduce flicker and stage-boundary jitter. */
float brightness_pipeline_post_smooth(float prev, float current, uint8_t stage_active)
{
    float k = AMB_DIMMING_POST_SMOOTH;
    if (!stage_active) {
        k += 0.08f;
    }
    if (k < 0.0f) k = 0.0f;
    if (k > 0.98f) k = 0.98f;
    return brightness_pipeline_clamp01(prev * k + current * (1.0f - k));
}

/* Evaluate final per-zone brightness factors for current frame. */
void brightness_pipeline_eval_zone(float calc_brightness,
                                   zone_id_t zone_id,
                                   float rel_brightness,
                                   float comfort_dim,
                                   uint8_t night_mode,
                                   brightness_zone_eval_t *out)
{
    brightness_zone_eval_t e;
    float v;

    if (!out) return;

    e.base = brightness_pipeline_clamp01(calc_brightness);
    e.rel = (rel_brightness > 0.0f) ? rel_brightness : 1.0f;
    e.boost = brightness_zone_boost(zone_id);
    e.comfort_dim = (comfort_dim > 0.0f) ? comfort_dim : 1.0f;

    v = e.base * e.rel * e.boost;
    if (night_mode) {
        v *= AMB_NIGHT_BRIGHTNESS_SCALE;
    } else if (zone_id != ZONE_STRIP) {
        v *= AMB_DAY_NONSTRIP_GAIN;
    }
    v *= e.comfort_dim;

    e.value = brightness_pipeline_clamp01(v);
    *out = e;
}
