#pragma once
/*
 * scene_player.h
 *
 * Стейт-машина поверх эффектов/тем:
 *   INTRO (welcome) -> BRIDGE -> SCENE -> OUTRO (goodbye) -> IDLE
 *
 * Особенности:
 *  - Управляет только сценарием и яркостью.
 *  - Не знает про DMA-внутренности: вывод кадра делает ws_render().
 *  - Яркость: theme_brightness (из темы) * user_brightness (ручка).
 *  - Welcome/Goodbye используют палитру темы (через presets).
 */

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "types.h"
//#include "effects.h"
//#include "presets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PST_IDLE = 0,
    PST_INTRO,
    PST_BRIDGE,
    PST_SCENE,
    PST_OUTRO
} player_stage_t;

typedef struct {
    player_stage_t stage;

    ws_theme_id_t  theme;

    ws_fx_state_t  st_scene;   // состояние основной сцены
    ws_oneshot_t   intro;      // welcome
    ws_oneshot_t   outro;      // goodbye

    float          calc_brightness;  // итоговая яркость темы
    float          theme_brightness; // базовая яркость темы
    float          theme_dimming;    // пользователь 0..1

    uint32_t       t0_ms;      // timestamp для bridge / стадий
} scene_player_t;

/* Инициализация структуры состояния (один раз при старте). */
void player_init(scene_player_t *pl);

/* Запуск темы:
 *  - запускает welcome-анимацию (ws_theme_start_intro)
 *  - сохраняет base_br и user_dim
 *  - включает форсированную яркость base_br*user_dim
 */
void player_start_theme(ws2812_t *ws,
                        scene_player_t *pl,
                        ws_theme_id_t theme,
                        float theme_dimming);

/* Запуск goodbye:
 *  - если играла SCENE/INTRO/BRIDGE — запускает outro (ws_theme_start_outro)
 *  - по завершении OUTRO переходит в IDLE и гасит ленту
 */
void player_start_outro(ws2812_t *ws, scene_player_t *pl);

/* Установка "ручки" яркости 0..1.
 * Применяется на следующем тике (через ws_force_brightness).
 */
void player_set_user_dim(ws2812_t *ws,
                         scene_player_t *pl,
                         float theme_dimming);

/* Тик стейт-машины:
 *  - вызывается часто (например, каждые 1–5 мс) в основном цикле
 *  - рисует кадр (intro / bridge / scene / outro) в ws->grb
 *  - один раз вызывает ws_render(ws)
 */
void player_tick(ws2812_t *ws,
                 scene_player_t *pl);

/* Хелперы */
static inline player_stage_t player_get_stage(const scene_player_t *pl)
{
    return pl->stage;
}
static inline ws_theme_id_t player_get_theme(const scene_player_t *pl)
{
    return pl->theme;
}
static inline float player_get_base_br(const scene_player_t *pl)
{
    return pl->theme_brightness;
}
static inline float player_get_user_dim(const scene_player_t *pl)
{
    return pl->theme_dimming;
}
static inline uint8_t player_is_idle(const scene_player_t *pl)
{
    return (pl->stage == PST_IDLE);
}

#ifdef __cplusplus
}
#endif
