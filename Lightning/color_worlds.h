#pragma once

#include <stdint.h>

#include "scene_color_model.h"
#include "scene_preset.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COLOR_WORLD_DUAL = 0u,
    COLOR_WORLD_SPECTRAL = 1u,
    COLOR_WORLD_LUMA = 2u
} color_world_type_t;

typedef enum {
    COLOR_WORLD_OCEAN_BLUE = 0u,
    COLOR_WORLD_RED_MOON,
    COLOR_WORLD_MIAMI_ROSE,
    COLOR_WORLD_MALIBU_SUNSET,
    COLOR_WORLD_FIERY_WHITE,
    COLOR_WORLD_PURPLE_SKY,
    COLOR_WORLD_JUNGLE_GREEN,
    COLOR_WORLD_GLACIER_BLUE,
    COLOR_WORLD_SUN_YELLOW,
    COLOR_WORLD_SILVER_LIGHT,
    COLOR_WORLD_CITRUS_DAWN,
    COLOR_WORLD_MINT_BREEZE,
    COLOR_WORLD_MIDNIGHT_TIDE,
    COLOR_WORLD_MAX
} color_world_id_t;

typedef struct {
    color_world_id_t id;
    color_world_type_t type;
    scene_rgb8_t dominant;
    uint8_t generated;
    uint16_t period_ms;
    float spatial_freq;
    int8_t direction;
} color_world_desc_t;

typedef struct {
    uint8_t active;
    color_world_id_t id;
    color_world_id_t prev_id;
    scene_rgb8_t dominant;
    float transition_k;
    float selection_distance;
    uint8_t generated;
} color_world_selection_t;

const color_world_desc_t *color_world_get_desc(color_world_id_t id);

void color_world_select(uint8_t effect_id,
                        uint8_t color_id,
                        uint32_t now_ms,
                        color_world_selection_t *out);

void color_world_sample(const color_world_selection_t *sel,
                        zone_id_t zone,
                        float pos01,
                        uint32_t now_ms,
                        const scene_preset_t *preset,
                        uint8_t *r,
                        uint8_t *g,
                        uint8_t *b);

#ifdef __cplusplus
}
#endif
