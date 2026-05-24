#pragma once

#include <stdint.h>

#include "ambient_state_store.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} scene_rgb8_t;

/** Runtime color model used by zones/event pipeline for one frame. */
typedef struct {
    scene_rgb8_t primary;
    scene_rgb8_t accent_a;
    scene_rgb8_t accent_b;
    scene_rgb8_t neutral;
    float energy;
    uint8_t valid;
} scene_color_model_t;

/** Semantic color entity for M4 composition layer. */
typedef struct {
    scene_rgb8_t base;
    scene_rgb8_t accent_warm;
    scene_rgb8_t accent_cool;
    scene_rgb8_t neutral_soft;
    scene_rgb8_t safety_alert;
    scene_rgb8_t guidance_line;
    float energy;
    uint8_t valid;
} scene_color_entity_t;

/** Build scene color model from normalized ambient state and context. */
void scene_color_model_from_ambient(const ambient_state_snapshot_t *ambient_state,
                                    motion_profile_t motion_profile,
                                    uint8_t night_mode,
                                    scene_color_model_t *out);
/** Build semantic color entity from current scene color model. */
void scene_color_entity_from_model(const scene_color_model_t *model,
                                   scene_color_entity_t *out);
/** Build semantic color entity directly from ambient/runtime context. */
void scene_color_entity_from_ambient(const ambient_state_snapshot_t *ambient_state,
                                     motion_profile_t motion_profile,
                                     uint8_t night_mode,
                                     scene_color_entity_t *out);

#ifdef __cplusplus
}
#endif
