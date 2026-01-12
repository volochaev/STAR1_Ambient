/**
 ******************************************************************************
 * @file    scene_player.h
 * @brief   Scene player state machine for ambient lighting
 * @details Manages lighting state transitions: intro → scene → outro.
 *          Controls brightness, night mode, and smooth theme transitions.
 *
 * @section State Machine
 * States:
 * - PST_IDLE: No active scene
 * - PST_INTRO: Intro animation (one-shot)
 * - PST_BRIDGE: Transition frame between intro and scene
 * - PST_SCENE: Active theme scene (continuous)
 * - PST_OUTRO: Outro animation (one-shot)
 *
 * @section Player Functions
 * - player_init(): Initialize player with initial theme
 * - player_start_theme(): Start new theme (triggers intro)
 * - player_start_intro(): Start intro animation
 * - player_start_outro(): Start outro animation
 * - player_tick(): Update player state (call periodically)
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
