/**
 ******************************************************************************
 * @file    driver.c
 * @brief   WS2812B driver implementation
 * @details Implements PWM+DMA based driver for WS2812B LEDs with double
 *          buffering, brightness control, and power management.
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#include "driver.h"
#include "features.h"
#include <math.h>
#include <string.h>

static uint8_t g_power_state = 0u;

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline float ws_effective_brightness(const ws2812_t *ws)
{
    float br = ws->br_forced ? ws->br_forced_value : ws->global_brightness;
    return clamp01f(br);
}

#if AMB_ENABLE_GAMMA
static uint8_t g_gamma_fwd[256];
static uint8_t g_gamma_inv[256];
static uint8_t g_gamma_ready = 0u;

static void gamma_tables_init(void)
{
    if (g_gamma_ready) return;
    const float inv255 = 1.0f / 255.0f;
    for (uint32_t i = 0; i < 256u; ++i) {
        float v = (float)i * inv255;
        float fwd = powf(v, AMB_GAMMA_EXP) * 255.0f;
        float inv = powf(v, 1.0f / AMB_GAMMA_EXP) * 255.0f;
        if (fwd < 0.0f) fwd = 0.0f; if (fwd > 255.0f) fwd = 255.0f;
        if (inv < 0.0f) inv = 0.0f; if (inv > 255.0f) inv = 255.0f;
        g_gamma_fwd[i] = (uint8_t)(fwd + 0.5f);
        g_gamma_inv[i] = (uint8_t)(inv + 0.5f);
    }
    g_gamma_ready = 1u;
}
#endif

static uint32_t channel_to_active_flag(uint32_t tim_channel)
{
    switch (tim_channel) {
    case TIM_CHANNEL_1: return HAL_TIM_ACTIVE_CHANNEL_1;
    case TIM_CHANNEL_2: return HAL_TIM_ACTIVE_CHANNEL_2;
    case TIM_CHANNEL_3: return HAL_TIM_ACTIVE_CHANNEL_3;
    case TIM_CHANNEL_4: return HAL_TIM_ACTIVE_CHANNEL_4;
    default:            return 0;
    }
}

static void pack_into(ws2812_t *ws, uint32_t *dst)
{
    if (!ws || !dst || !ws->rgb) return;

    const uint32_t one  = WS_T1H;
    const uint32_t zero = WS_T0H;

    float brf  = ws_effective_brightness(ws);
    uint32_t bq = (uint32_t)(brf * 255.f + 0.5f);

    uint32_t bits_payload = (uint32_t)ws->led_count * BYTES_PER_LED * 8u;
    uint32_t max_payload  = ws->dma_len > WS_RESET_SLOTS
                            ? (ws->dma_len - WS_RESET_SLOTS)
                            : 0u;
    if (bits_payload > max_payload)
        bits_payload = max_payload;

    const uint8_t *src = ws->rgb;
    uint32_t bits_written = 0;

    if (bq == 0u) {
        for (; bits_written < bits_payload; ++bits_written)
            *dst++ = zero;
    } else {
#if AMB_ENABLE_GAMMA
        gamma_tables_init();
#endif
#if AMB_ENABLE_DITHERING
        uint32_t t_ms = HAL_GetTick();
#endif
        for (uint16_t i = 0; i < ws->led_count && bits_written < bits_payload; ++i) {
            for (uint32_t c = 0; c < BYTES_PER_LED && bits_written < bits_payload; ++c) {
                uint8_t v = *src++;
                if (bq != 255u) {
#if AMB_ENABLE_GAMMA
                    uint32_t lin = g_gamma_inv[v];
                    uint32_t scaled = lin * bq;
#else
                    uint32_t scaled = (uint32_t)v * bq;
#endif
                    uint32_t vq = scaled / 255u;
#if AMB_ENABLE_DITHERING
                    uint32_t rem = scaled - (vq * 255u);
                    if (rem != 0u) {
                        uint32_t seed = t_ms + (uint32_t)i * 131u + (uint32_t)c * 17u;
                        if ((seed & 0xFFu) < rem) {
                            vq++;
                        }
                    }
#endif
                    if (vq > 255u) vq = 255u;
#if AMB_ENABLE_GAMMA
                    v = g_gamma_fwd[vq];
#else
                    v = (uint8_t)vq;
#endif
                }
                for (int b = 7; b >= 0 && bits_written < bits_payload; --b) {
                    *dst++ = (v & (1u << b)) ? one : zero;
                    ++bits_written;
                }
            }
        }
    }

    uint32_t reset_len = ws->dma_len - bits_written;
    if (reset_len > WS_RESET_SLOTS)
        reset_len = WS_RESET_SLOTS;

    for (uint32_t i = 0; i < reset_len; ++i) {
        *dst++ = 0u;
    }
}

void ws_init(ws2812_t        *ws,
             TIM_HandleTypeDef *htim,
             uint32_t         tim_channel,
             uint8_t         *framebuffer,
             uint32_t        *dma_buf,
             uint16_t         led_count)
{
    if (!ws || !htim || !framebuffer || !dma_buf || led_count == 0)
        return;

    memset(ws, 0, sizeof(*ws));

    ws->htim        = htim;
    ws->tim_channel = tim_channel;
    ws->rgb         = framebuffer;
    ws->led_count   = led_count;
    ws->dma_buf     = dma_buf;
    ws->dma_len     = (uint32_t)led_count * BYTES_PER_LED * 8u + WS_RESET_SLOTS;

    memset(ws->rgb, 0, (size_t)led_count * BYTES_PER_LED);

    ws->global_brightness = 1.0f;
    ws->br_forced         = 0u;
    ws->br_forced_value   = 0.0f;
    ws->dma_busy          = 0u;
}

void ws_set_global_brightness(ws2812_t *ws, float br)
{
    if (!ws) return;
    ws->global_brightness = clamp01f(br);
}

void ws_force_brightness(ws2812_t *ws, float br)
{
    if (!ws) return;
    ws->br_forced       = 1u;
    ws->br_forced_value = clamp01f(br);
}

void ws_release_brightness(ws2812_t *ws)
{
    if (!ws) return;
    ws->br_forced = 0u;
}

void ws_render(ws2812_t *ws)
{
    if (!ws || !ws->htim || !ws->dma_buf || !ws->rgb)
        return;

    if (ws->dma_busy)
        return;  /* Ждём завершения предыдущей передачи */

    pack_into(ws, ws->dma_buf);
    ws->dma_busy = 1u;

    HAL_TIM_PWM_Start_DMA(ws->htim,
                          ws->tim_channel,
                          (uint32_t*)ws->dma_buf,
                          ws->dma_len);
}

void ws_dma_tc_isr(ws2812_t *ws, TIM_HandleTypeDef *htim)
{
    if (!ws || !htim) return;
    if (ws->htim != htim) return;

    uint32_t expected = channel_to_active_flag(ws->tim_channel);
    if (expected == 0 || htim->Channel != expected)
        return;

    ws->dma_busy = 0u;
    HAL_TIM_PWM_Stop_DMA(ws->htim, ws->tim_channel);
}

void ws_power_set(uint8_t on)
{
    g_power_state = on ? 1u : 0u;

#ifdef LED_PWR_EN_Pin
    HAL_GPIO_WritePin(LED_PWR_EN_GPIO_Port,
                      LED_PWR_EN_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif

#ifdef LED_DATA_OE_Pin
    HAL_GPIO_WritePin(LED_DATA_OE_GPIO_Port,
                      LED_DATA_OE_Pin,
                      on ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
}

uint8_t ws_is_power_on(void)
{
    return g_power_state;
}
