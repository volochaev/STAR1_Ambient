/**
 ******************************************************************************
 * @file    base_scene.h
 * @brief   Base-scene state machine for ambient lighting
 * @details Manages lighting state transitions: intro → scene → outro.
 *          Controls brightness, night mode, and smooth theme transitions.
 *
 * @section State Machine
 * States:
 * - BASE_SCENE_IDLE: No active scene
 * - BASE_SCENE_INTRO: Intro animation (one-shot)
 * - BASE_SCENE_BRIDGE: Transition frame between intro and scene
 * - BASE_SCENE_ACTIVE: Active theme scene (continuous)
 * - BASE_SCENE_OUTRO: Outro animation (one-shot)
 *
 * @section Base-Scene Functions
 * - base_scene_init(): Initialize base scene with initial theme
 * - base_scene_start_theme(): Start new theme (triggers intro)
 * - base_scene_start_intro(): Start intro animation
 * - base_scene_start_outro(): Start outro animation
 * - base_scene_tick(): Update base scene (call periodically)
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
#include "effects.h"
#include "features.h"

#ifdef __cplusplus
extern "C" {
#endif

void base_scene_init(base_scene_t *sc, theme_id_t initial_theme);
void base_scene_start_theme(led_runtime_strip_t *ws, base_scene_t *sc, theme_id_t theme);
void base_scene_start_intro(led_runtime_strip_t *ws, base_scene_t *sc);
void base_scene_start_outro(led_runtime_strip_t *ws, base_scene_t *sc);
void base_scene_tick(led_runtime_strip_t *ws, base_scene_t *sc, uint32_t delta_ms);
void base_scene_refresh_brightness(base_scene_t *sc);
void base_scene_reset_fx_state(base_scene_t *sc);
void base_scene_start_theme_crossfade(base_scene_t *sc, theme_id_t target_theme);
uint8_t base_scene_apply_theme_base(base_scene_t *sc, theme_id_t theme);
uint8_t base_scene_apply_theme_with_intro(base_scene_t *sc, led_runtime_strip_t *ws, theme_id_t theme);
uint8_t base_scene_apply_theme_to_scene(base_scene_t *sc, theme_id_t theme);

#if AMB_ENABLE_AUTO_ROTATE
/* Auto-rotate control (enabled via AMB_ENABLE_AUTO_ROTATE in features.h)
 * Note: Uses global runtime OEM color state (g_oem_color) for bank selection.
 * Auto-rotate is automatically enabled when AMB_ENABLE_AUTO_ROTATE=1.
 * These functions are optional - for manual control if needed. */
void base_scene_set_auto_rotate(base_scene_t *sc, uint8_t enable);
void base_scene_trigger_next_theme(base_scene_t *sc);  /* Manual trigger for next theme with crossfade */
#endif

#ifdef __cplusplus
}
#endif
