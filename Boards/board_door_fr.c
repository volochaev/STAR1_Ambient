/**
 ******************************************************************************
 * @file    board_door_fr.c
 * @brief   Front-Right door board implementation
 * @version 2.1
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"        /* BOARD_TYPE_* definitions */
#include "board_door_fr.h"
#include "board_led_backend.h"
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim2;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */

/* STRIP */
static uint8_t fr_strip_fb[FR_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fr_strip_dma[BOARD_DMA_BUF_LEN(FR_STRIP_LEDS)];

/* HANDLE */
static uint8_t fr_handle_fb[FR_HANDLE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fr_handle_dma[BOARD_DMA_BUF_LEN(FR_HANDLE_LEDS)];

/* STORAGE */
static uint8_t fr_storage_fb[FR_STORAGE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fr_storage_dma[BOARD_DMA_BUF_LEN(FR_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t fr_footwell_fb[FR_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fr_footwell_dma[BOARD_DMA_BUF_LEN(FR_FOOTWELL_LEDS)];

/* WS2812 driver instances (physical board lines/zones). */

ws2812_t g_fr_strip;
ws2812_t g_fr_handle;
ws2812_t g_fr_storage;
ws2812_t g_fr_footwell;

static const board_led_line_t g_fr_lines[] = {
    { &g_fr_strip, TIM_CHANNEL_1, fr_strip_fb, fr_strip_dma, FR_STRIP_LEDS, FR_STRIP_ORDER, 1u },
    { &g_fr_handle, TIM_CHANNEL_2, fr_handle_fb, fr_handle_dma, FR_HANDLE_LEDS, FR_HANDLE_ORDER, 1u },
    { &g_fr_storage, TIM_CHANNEL_3, fr_storage_fb, fr_storage_dma, FR_STORAGE_LEDS, FR_STORAGE_ORDER, 1u },
    { &g_fr_footwell, TIM_CHANNEL_4, fr_footwell_fb, fr_footwell_dma, FR_FOOTWELL_LEDS, FR_FOOTWELL_ORDER, 1u },
};

/* Initialization. */

void board_fr_led_init(void)
{
    board_led_backend_init_lines(&htim2,
                                 g_fr_lines,
                                 (uint8_t)(sizeof(g_fr_lines) / sizeof(g_fr_lines[0])),
                                 1u);
}

/* Render all board lines. */

void board_fr_led_render_all(void)
{
    board_led_backend_render_lines(g_fr_lines, (uint8_t)(sizeof(g_fr_lines) / sizeof(g_fr_lines[0])));
}

void board_fr_dma_tc(TIM_HandleTypeDef *htim)
{
    board_led_backend_dma_tc_lines(g_fr_lines,
                                   (uint8_t)(sizeof(g_fr_lines) / sizeof(g_fr_lines[0])),
                                   htim);
}
