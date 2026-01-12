
#pragma once
/**
 * @file driver.h
 * @brief Multi-channel WS2812B driver on TIM PWM + DMA.
 */

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

    uint8_t           *grb;
    uint16_t           led_count;

    uint16_t          *dma_buf_a;
    uint16_t          *dma_buf_b;
    uint16_t          *active_buf;
    uint16_t          *ready_buf;
    uint32_t           dma_len;

    volatile uint8_t   dma_busy;
    volatile uint8_t   frame_ready;

    float              global_brightness;
    uint8_t            br_forced;
    float              br_forced_value;
} ws2812_t;

typedef ws2812_t led_zone_t;

void ws_init(ws2812_t        *ws,
             TIM_HandleTypeDef *htim,
             uint32_t         tim_channel,
             uint8_t         *framebuffer,
             uint16_t        *dma_buf_a,
             uint16_t        *dma_buf_b,
             uint16_t         led_count);

void ws_set_global_brightness(ws2812_t *ws, float br);
void ws_force_brightness(ws2812_t *ws, float br);
void ws_release_brightness(ws2812_t *ws);
void ws_render(ws2812_t *ws);
void ws_dma_tc_isr(ws2812_t *ws, TIM_HandleTypeDef *htim);

void ws_power_set(uint8_t on);
uint8_t ws_is_power_on(void);

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


static inline void ws_set_pixel_rgb(ws2812_t *ws,
                                    uint16_t i,
                                    uint8_t r,
                                    uint8_t g,
                                    uint8_t b)
{
    uint32_t idx = (uint32_t)i * 3u;

    // RGB
    ws->grb[idx + 0] = r;
    ws->grb[idx + 1] = g;
    ws->grb[idx + 2] = b;

    // GRB
    /*
    ws->grb[idx + 0] = g;
    ws->grb[idx + 1] = r;
    ws->grb[idx + 2] = b;
    */
}

#ifdef __cplusplus
}
#endif
