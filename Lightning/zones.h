/**
 ******************************************************************************
 * @file    zones.h
 * @brief   Zone mapping system for ambient lighting
 * @details Maps logical zones (strip, handle, storage, footwell) to physical
 *          LED strips and applies theme effects to each zone independently.
 *
 * @section Zone System
 * Each board defines a zone map (g_zone_map) that maps logical zones to
 * physical LED strips. Zones can have different effects, palettes, and
 * brightness scales for customized lighting per area.
 *
 * @section Zone Operations
 * - zones_apply_scene(): Apply continuous scene effects to all zones
 * - zones_apply_intro(): Apply intro animation (0.0 to 1.0 progress)
 * - zones_apply_outro(): Apply outro animation (0.0 to 1.0 progress)
 *
 * @note Zone map (g_zone_map) must be defined in board-specific files
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

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
void zones_apply_bridge(const scene_player_t *pl, float t_norm);

#ifdef __cplusplus
}
#endif
