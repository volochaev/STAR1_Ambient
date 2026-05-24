#pragma once

#include <stdint.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float base;
    float rel;
    float boost;
    float comfort_dim;
    float value;
} brightness_zone_eval_t;

/** Clamp float to `[0..1]`. */
float brightness_pipeline_clamp01(float x);
/** Map normalized OEM brightness input to runtime dimming curve. */
float brightness_pipeline_map_oem_brightness(float oem);
/** Slew-limit dimming changes using time-based rate limits. */
float brightness_pipeline_slew_dimming(float current, float target, uint32_t dt_ms);
/** Post-smooth dimming to reduce visible stepping in active scene. */
float brightness_pipeline_post_smooth(float prev, float current, uint8_t stage_active);
/** Evaluate final per-zone brightness after all policy multipliers. */
void brightness_pipeline_eval_zone(float calc_brightness,
                                   zone_id_t zone_id,
                                   float rel_brightness,
                                   float comfort_dim,
                                   uint8_t night_mode,
                                   brightness_zone_eval_t *out);

#ifdef __cplusplus
}
#endif
