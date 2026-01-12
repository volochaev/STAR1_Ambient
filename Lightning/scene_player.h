
#pragma once
/**
 * @file scene_player.h
 * @brief Scene player (intro / bridge / scene / outro) for main strip.
 */

#include "types.h"
#include "driver.h"
#include "palette.h"
#include "presets.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" {
#endif

void player_init(scene_player_t *pl, ws_theme_id_t initial_theme);
void player_start_theme(ws2812_t *ws, scene_player_t *pl, ws_theme_id_t theme);
void player_start_intro(ws2812_t *ws, scene_player_t *pl);
void player_start_outro(ws2812_t *ws, scene_player_t *pl);
void player_tick(ws2812_t *ws, scene_player_t *pl, uint32_t delta_ms);

#ifdef __cplusplus
}
#endif
