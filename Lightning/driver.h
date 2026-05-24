/**
 ******************************************************************************
 * @file    driver.h
 * @brief   WS2812B LED driver using TIM PWM + DMA
 * @details Low-level driver for WS2812B RGB LEDs using STM32 timer PWM
 *          with DMA for efficient data transfer. Supports up to 4 independent
 *          channels (zones) using TIM1 channels 1-4.
 *
 * @section Driver Architecture
 * - Uses TIM1 PWM at 800 kHz for WS2812B timing
 * - Double buffered DMA for smooth frame updates
 * - RGB color format (red, green, blue byte order)
 * - Global brightness control with per-frame dimming
 *
 * @section Usage
 * 1. Initialize with ws_init() providing framebuffer and DMA buffers
 * 2. Set pixel colors using ws_set_pixel_rgb()
 * 3. Call ws_render() to start DMA transfer
 * 4. Handle DMA completion in HAL_TIM_PWM_PulseFinishedCallback using ws_dma_tc_isr()
 *
 * @note DMA buffers must be large enough: (led_count * 3 * 8) + WS_RESET_SLOTS words
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BYTES_PER_LED
#define BYTES_PER_LED 3u
#endif

#ifndef WS_RESET_SLOTS
#define WS_RESET_SLOTS 300u
#endif

#ifndef WS_T0H
#define WS_T0H  60u
#endif

#ifndef WS_T1H
#define WS_T1H  120u
#endif

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t           tim_channel;

    uint8_t           *rgb;
    uint16_t           led_count;

    uint32_t          *dma_buf;
    uint32_t           dma_len;

    volatile uint8_t   dma_busy;

    float              global_brightness;
    uint8_t            br_forced;
    float              br_forced_value;
    uint8_t            color_order;
} ws2812_t;

typedef ws2812_t led_zone_t;

/* === POWER CONTROL ======================================================= */
/* Global buck enable is driven by LED_PWR_EN.
 * Optional per-channel switches (CH1_EN..CH4_EN) can be toggled via mask. */
#define WS_CH1   (1u << 0)
#define WS_CH2   (1u << 1)
#define WS_CH3   (1u << 2)
#define WS_CH4   (1u << 3)
#define WS_CH_ALL (WS_CH1 | WS_CH2 | WS_CH3 | WS_CH4)

#define WS_COLOR_ORDER_RGB 0u
#define WS_COLOR_ORDER_GRB 1u

void ws_init(ws2812_t        *ws,
             TIM_HandleTypeDef *htim,
             uint32_t         tim_channel,
             uint8_t         *framebuffer,
             uint32_t        *dma_buf,
             uint16_t         led_count);
void ws_set_color_order(ws2812_t *ws, uint8_t color_order);

void ws_set_global_brightness(ws2812_t *ws, float br);
void ws_force_brightness(ws2812_t *ws, float br);
void ws_release_brightness(ws2812_t *ws);
void ws_render(ws2812_t *ws);
void ws_dma_tc_isr(ws2812_t *ws, TIM_HandleTypeDef *htim);

void ws_power_set(uint8_t on);
uint8_t ws_is_power_on(void);
void ws_power_set_channel_mask(uint8_t mask);
uint8_t ws_power_get_channel_mask(void);

static inline void led_zone_init(led_zone_t       *z,
                                 TIM_HandleTypeDef *htim,
                                 uint32_t          ch,
                                 uint8_t          *fb,
                                 uint32_t         *dma,
                                 uint16_t          n)
{
    ws_init(z, htim, ch, fb, dma, n);
}

static inline void led_zone_render(led_zone_t *z)
{
    ws_render(z);
}


static inline void ws_set_pixel_rgb(ws2812_t *ws,
                                    uint16_t i,
                                    uint8_t r,
                                    uint8_t g,
                                    uint8_t b)
{
    uint32_t idx = (uint32_t)i * 3u;

    ws->rgb[idx + 0] = r;
    ws->rgb[idx + 1] = g;
    ws->rgb[idx + 2] = b;
}

#ifdef __cplusplus
}
#endif
