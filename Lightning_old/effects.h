#pragma once
/**
 * @file effects.h
 * @brief Visual effects for WS2812 ambient lighting.
 *
 * Здесь описываются:
 *  - enum fx_id_t (бывший ws_fx_id_t)
 *  - состояние эффекта fx_state_t (бывший ws_fx_state_t)
 *  - one-shot FX (welcome / goodbye)
 *  - общий вызов ws_fx_apply()
 *
 * Эффекты работают с:
 *  - ws2812_t (см. driver.h / types.h)
 *  - ws_palette_t (см. palette.h)
 *
 * Буфер ws->grb считается GRB-последовательностью:
 *   idx: G,R,B (как в zones.c и driver.c)
 */

#include <stdint.h>
#include "driver.h"
#include "types.h"
#include "palette.h"
#include "main.h"      // для HAL_GetTick

#ifdef __cplusplus
extern "C" {
#endif

/* ==== FX IDs ========================================================== */
/* Список основан на твоём оригинальном ws_fx_id_t. */


/* Для совместимости со старым кодом */
typedef fx_id_t ws_fx_id_t;

/* MB-style логические алиасы (используются в presets/zones) */
#define FX_MB_SOFT_SOLID      FX_SOLID_GRADIENT
#define FX_MB_FLOW_SOFT       FX_GRADIENT_FLOW
#define FX_MB_FLOW_DEEP       FX_MB_OCEAN_FLOW
#define FX_MB_DUO_FLOW        FX_DUAL_ZONE
#define FX_MB_SOFT_BREATHE    FX_SOFT_BREATHE
#define FX_MB_ENERGIZE        FX_MB_ENERGIZE_PULSE
/* FX_MB_WELCOME / FX_MB_GOODBYE уже в enum */

/* ==== FX State ======================================================== */


/* Совместимость */
typedef fx_state_t ws_fx_state_t;

/* ==== One-shot FX (welcome, goodbye) ================================= */

typedef struct {
    uint8_t   active;
    fx_id_t   fx_id;
    float     base_br;       ///< базовая яркость в начале эффекта (0..1)
    const ws_palette_t *pal; ///< палитра эффекта (может совпадать с темой)
    uint32_t  start_ms;
    uint32_t  duration_ms;   ///< длительность в мс
} ws_oneshot_t;

/**
 * @brief Запуск one-shot эффекта.
 *
 * fx_id:
 *   - FX_WELCOME_SWEEP / FX_MB_WELCOME
 *   - FX_GOODBYE_FADE / FX_MB_GOODBYE
 */
void ws_oneshot_start(ws2812_t        *ws,
                      ws_oneshot_t    *os,
                      fx_id_t          fx_id,
                      const ws_palette_t *pal,
                      float            base_br,
                      uint32_t         duration_ms);

/**
 * @brief Выполнить шаг one-shot эффекта.
 *
 * @return 1, если эффект завершён (os->active сброшен), иначе 0.
 *
 * Вызывает соответствующий FX по fx_id и заполняет ws->grb.
 * Вызывается из player_tick() в стадиях INTRO/OUTRO.
 */
uint8_t ws_oneshot_tick(ws2812_t *ws,
                        ws_oneshot_t *os);

/* ==== Основной вызов непрерывных эффектов ============================ */

/**
 * @brief Применить непрерывный эффект к ленте.
 *
 * - fx_id: тип эффекта
 * - pal: палитра (обязательна для всех градиентных FX)
 * - st: состояние эффекта (обновляется внутри)
 *
 * Эффект рисует в ws->grb (GRB порядок).
 * delta_ms — шаг времени в миллисекундах.
 */
void ws_fx_apply(ws2812_t           *ws,
                 fx_id_t             fx_id,
                 const ws_palette_t *pal,
                 fx_state_t         *st,
                 uint32_t            delta_ms);

#ifdef __cplusplus
}
#endif
