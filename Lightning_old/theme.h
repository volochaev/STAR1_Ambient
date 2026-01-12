/*
 * theme.h
 *
 *  Created on: Nov 9, 2025
 *      Author: nv
 */
#pragma once
#include <stdint.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Найти описание темы
const THEME_DESC_T* scene_by_theme(THEME_ID_T id);

// Рендер бесконечной сцены для текущей темы
void ws_theme_render_scene(LED_driver_t *ws,
                           THEME_ID_T theme,
                           FX_state_t *st_scene,
                           float *opt_set_brightness);

// Старт интро (MB-welcome)
void ws_theme_start_intro(LED_driver_t *ws,
						  LED_oneshot_t *intro,
						  THEME_ID_T theme,
                          float brightness);

// Старт аутро (MB-goodbye)
void ws_theme_start_outro(LED_driver_t *ws,
                          LED_oneshot_t *outro,
						  THEME_ID_T theme,
                          float current_br);

#ifdef __cplusplus
}
#endif
