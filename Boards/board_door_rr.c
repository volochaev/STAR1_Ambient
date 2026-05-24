/**
 ******************************************************************************
 * @file    board_door_rr.c
 * @brief   Rear-Right door board implementation
 * @version 2.1
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"        /* BOARD_TYPE_* definitions */
#include "board_door_rr.h"
#include "board_led_backend.h"
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim2;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */

/* STRIP */
static uint8_t rr_strip_fb[RR_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rr_strip_dma[BOARD_DMA_BUF_LEN(RR_STRIP_LEDS)];

/* HANDLE */
static uint8_t rr_handle_fb[RR_HANDLE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rr_handle_dma[BOARD_DMA_BUF_LEN(RR_HANDLE_LEDS)];

/* STORAGE */
static uint8_t rr_storage_fb[RR_STORAGE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rr_storage_dma[BOARD_DMA_BUF_LEN(RR_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t rr_footwell_fb[RR_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rr_footwell_dma[BOARD_DMA_BUF_LEN(RR_FOOTWELL_LEDS)];

/* WS2812 driver instances (physical board lines/zones). */

ws2812_t g_rr_strip;
ws2812_t g_rr_handle;
ws2812_t g_rr_storage;
ws2812_t g_rr_footwell;

static const board_led_line_t g_rr_lines[] = {
    { &g_rr_strip, TIM_CHANNEL_1, rr_strip_fb, rr_strip_dma, RR_STRIP_LEDS, RR_STRIP_ORDER, 1u },
    { &g_rr_handle, TIM_CHANNEL_2, rr_handle_fb, rr_handle_dma, RR_HANDLE_LEDS, RR_HANDLE_ORDER, 1u },
    { &g_rr_storage, TIM_CHANNEL_3, rr_storage_fb, rr_storage_dma, RR_STORAGE_LEDS, RR_STORAGE_ORDER, 1u },
    { &g_rr_footwell, TIM_CHANNEL_4, rr_footwell_fb, rr_footwell_dma, RR_FOOTWELL_LEDS, RR_FOOTWELL_ORDER, 1u },
};

/* Initialization. */

void board_rr_led_init(void)
{
    board_led_backend_init_lines(&htim2,
                                 g_rr_lines,
                                 (uint8_t)(sizeof(g_rr_lines) / sizeof(g_rr_lines[0])),
                                 1u);
}

/* Render all board lines. */

void board_rr_led_render_all(void)
{
    board_led_backend_render_lines(g_rr_lines, (uint8_t)(sizeof(g_rr_lines) / sizeof(g_rr_lines[0])));
}

void board_rr_dma_tc(TIM_HandleTypeDef *htim)
{
    board_led_backend_dma_tc_lines(g_rr_lines,
                                   (uint8_t)(sizeof(g_rr_lines) / sizeof(g_rr_lines[0])),
                                   htim);
}
