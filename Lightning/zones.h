
#pragma once
/**
 * @file zones.h
 * @brief Mapping themes to physical LED zones.
 */

#include "types.h"
#include "driver.h"
#include "palette.h"
#include "presets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ws2812_t *ws;
    uint16_t  first;
    uint16_t  count;
} zone_map_t;

/* Implemented in specific board_xxx.c */
extern const zone_map_t g_zone_map[WS_ZONE_MAX];

void zones_apply_scene(const scene_player_t *pl);
void zones_apply_intro(const scene_player_t *pl, float t_norm);
void zones_apply_outro(const scene_player_t *pl, float t_norm);

#ifdef __cplusplus
}
#endif
