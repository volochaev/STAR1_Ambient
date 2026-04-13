/**
 ******************************************************************************
 * @file    zones.h
 * @brief   Zone mapping system for ambient lighting
 * @details Maps logical zones (strip, handle, storage, footwell) to physical
 *          LED strips and applies theme effects to each zone independently.
 *
 * @section Zone System
 * The selected board profile provides a single zone map (g_zone_map) in
 * Boards/board_zone_map.c. It maps logical zones to physical LED strips.
 * Zones can have different effects, palettes, and brightness scales.
 *
 * @section Zone Operations
 * - zones_apply_scene(): Apply continuous scene effects to all zones
 * - zones_apply_intro(): Apply intro animation (0.0 to 1.0 progress)
 * - zones_apply_outro(): Apply outro animation (0.0 to 1.0 progress)
 *
 * @note g_zone_map is defined once in Boards/board_zone_map.c
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include "types.h"
#include "led_runtime.h"
#include "palette.h"
#include "themes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    led_runtime_strip_t *strip;
    uint16_t  first;
    uint16_t  count;
} zone_map_t;

/* Implemented in Boards/board_zone_map.c */
extern const zone_map_t g_zone_map[ZONE_MAX];

void zones_apply_scene(const base_scene_t *pl);
void zones_apply_intro(const base_scene_t *pl, float t_norm);
void zones_apply_outro(const base_scene_t *pl, float t_norm);
void zones_apply_bridge(const base_scene_t *pl, float t_norm);
void zones_apply_interrupt_overlay(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
