#pragma once

#include "scene_color_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Overlay palette derived from current scene entity. */
typedef struct {
    scene_rgb8_t warm;
    scene_rgb8_t cool;
    scene_rgb8_t parking;
    scene_rgb8_t safety;
    scene_rgb8_t guidance;
} overlay_palette_t;

/** Build overlay palette from scene entity (with safe fallbacks). */
void overlay_palette_build(const scene_color_entity_t *entity, overlay_palette_t *out);

#ifdef __cplusplus
}
#endif
