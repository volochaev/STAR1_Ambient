#pragma once
/**
 * @file driver.h
 * @brief Multi-channel WS2812B driver on TIM PWM + DMA.
 *
 * Поддерживает несколько независимых физических линий (zones),
 * каждая линия = отдельный ws2812_t, привязанный к:
 *   - одному TIM (например TIM1)
 *   - одному PWM-каналу (CH1..CH4)
 *   - своему GRB-буферу и DMA-буферам.
 *
 * Логика:
 *  - верхние уровни (effects / scene_player / zones) рисуют в ws->grb;
 *  - ws_render() упаковывает в PWM-формат;
 *  - TIM+DMA выстреливает данные на конкретную линию;
 *  - один TIM может обслуживать до 4 каналов одновременно.
 */

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* === Настройки таймингов и размеров =================================== */

#ifndef BYTES_PER_LED
#define BYTES_PER_LED 3u          // WS2812: GRB, 3 байта на LED
#endif

#ifndef WS_RESET_SLOTS
#define WS_RESET_SLOTS 300u       // длина reset-паузы (кол-во "0" импульсов)
#endif

#ifndef WS_T0H
#define WS_T0H  60u               // таймер ticks для лог.0 high
#endif

#ifndef WS_T1H
#define WS_T1H  120u              // таймер ticks для лог.1 high
#endif

/* === Структура канала (zone) ========================================== */

/**
 * @brief Один физический выход WS2812 (одна лента / зона).
 *
 * Каждый такой объект привязан к:
 *   - конкретному TIM (htim)
 *   - конкретному PWM-каналу (tim_channel: TIM_CHANNEL_x)
 *   - своим GRB- и DMA-буферам.
 *
 * Верхние уровни знают только про ws->grb и ws->led_count.
 */
typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t           tim_channel;

    uint8_t           *grb;         ///< GRB буфер [led_count * 3]
    uint16_t           led_count;   ///< число светодиодов в этой зоне

    uint16_t          *dma_buf_a;   ///< первый DMA буфер (PWM-код)
    uint16_t          *dma_buf_b;   ///< второй DMA буфер (double-buffer)
    uint16_t          *active_buf;  ///< сейчас крутится DMA
    uint16_t          *ready_buf;   ///< следующий кадр
    uint32_t           dma_len;     ///< длина DMA буфера (uint16_t)

    volatile uint8_t   dma_busy;    ///< DMA сейчас активен
    volatile uint8_t   frame_ready; ///< есть упакованный кадр в ready_buf

    float              global_brightness; ///< 0..1
    uint8_t            br_forced;         ///< 1 = форс яркости
    float              br_forced_value;   ///< 0..1
} ws2812_t;

/* Для единообразия можно думать об этом как о "zone" */
typedef ws2812_t led_zone_t;

/* === API =============================================================== */

/**
 * @brief Инициализация одного физического канала (зоны).
 *
 * @param ws          объект зоны
 * @param htim        TIM, сконфигурированный под WS2812 (например TIM1)
 * @param tim_channel TIM_CHANNEL_1..TIM_CHANNEL_4
 * @param framebuffer GRB-буфер [led_count * 3]
 * @param dma_buf_a   буфер PWM-кода [led_count*24 + WS_RESET_SLOTS]
 * @param dma_buf_b   второй буфер той же длины
 * @param led_count   число светодиодов на этой линии
 */
void ws_init(ws2812_t        *ws,
             TIM_HandleTypeDef *htim,
             uint32_t         tim_channel,
             uint8_t         *framebuffer,
             uint16_t        *dma_buf_a,
             uint16_t        *dma_buf_b,
             uint16_t         led_count);

/* Глобальная яркость 0..1 (мультипликативно, поверх содержимого grb). */
void ws_set_global_brightness(ws2812_t *ws, float br);

/* Форсировать яркость 0..1 (игнорирует global_brightness). */
void ws_force_brightness(ws2812_t *ws, float br);

/* Снять форс. */
void ws_release_brightness(ws2812_t *ws);

/**
 * @brief Подготовить и запустить (или поставить в очередь) вывод кадра.
 *
 * Действия:
 *  - упаковать ws->grb в ws->ready_buf как PWM-формат;
 *  - если DMA свободен — сразу запустить;
 *  - если занят — следующий кадр уйдёт при следующем ws_dma_tc_isr().
 */
void ws_render(ws2812_t *ws);

/**
 * @brief Обработчик окончания PWM+DMA для конкретного канала.
 *
 * Вызывать из HAL_TIM_PWM_PulseFinishedCallback():
 *
 *   void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
 *   {
 *       ws_dma_tc_isr(&zone_strip, htim);
 *       ws_dma_tc_isr(&zone_handle, htim);
 *       ws_dma_tc_isr(&zone_storage, htim);
 *       ws_dma_tc_isr(&zone_footwell, htim);
 *   }
 */
void ws_dma_tc_isr(ws2812_t *ws, TIM_HandleTypeDef *htim);

/* === Питание и OE (общие для всех зон на модуле) ====================== */

/* Включить/выключить питание и выходы данных (если ноги определены). */
void ws_power_set(uint8_t on);
uint8_t ws_is_power_on(void);

/* Алиасы под "новый" нейминг, если захочешь: */
static inline void led_zone_init(led_zone_t       *z,
                                 TIM_HandleTypeDef *htim,
                                 uint32_t          ch,
                                 uint8_t          *fb,
                                 uint16_t         *dma_a,
                                 uint16_t         *dma_b,
                                 uint16_t          n)
{
    ws_init(z, htim, ch, fb, dma_a, dma_b, n);
}

static inline void led_zone_render(led_zone_t *z)
{
    ws_render(z);
}

#ifdef __cplusplus
}
#endif
