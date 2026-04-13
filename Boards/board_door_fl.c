/**
 ******************************************************************************
 * @file    board_door_fl.c
 * @brief   Front-Left door board implementation
 * @details Implements hardware initialization and zone mapping for the
 *          front-left door ambient lighting module.
 *
 * @version 2.1
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"        /* BOARD_TYPE_* definitions */
#include "board_door_fl.h"
#include "board_led_backend.h"
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim2;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */
/* Using macros from board_common.h with __ALIGNED(4) for optimal DMA */

/* STRIP */
static uint8_t fl_strip_fb[FL_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fl_strip_dma[BOARD_DMA_BUF_LEN(FL_STRIP_LEDS)];

/* HANDLE */
static uint8_t fl_handle_fb[FL_HANDLE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fl_handle_dma[BOARD_DMA_BUF_LEN(FL_HANDLE_LEDS)];

/* STORAGE */
static uint8_t fl_storage_fb[FL_STORAGE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fl_storage_dma[BOARD_DMA_BUF_LEN(FL_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t fl_footwell_fb[FL_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t fl_footwell_dma[BOARD_DMA_BUF_LEN(FL_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_fl_strip;
ws2812_t g_fl_handle;
ws2812_t g_fl_storage;
ws2812_t g_fl_footwell;

static const board_led_line_t g_fl_lines[] = {
    { &g_fl_strip, TIM_CHANNEL_1, fl_strip_fb, fl_strip_dma, FL_STRIP_LEDS, 1u },
    { &g_fl_handle, TIM_CHANNEL_2, fl_handle_fb, fl_handle_dma, FL_HANDLE_LEDS, 1u },
    { &g_fl_storage, TIM_CHANNEL_3, fl_storage_fb, fl_storage_dma, FL_STORAGE_LEDS, 1u },
    { &g_fl_footwell, TIM_CHANNEL_4, fl_footwell_fb, fl_footwell_dma, FL_FOOTWELL_LEDS, 1u },
};

/* === Инициализация ==================================================== */

void board_fl_led_init(void)
{
    board_led_backend_init_lines(&htim2,
                                 g_fl_lines,
                                 (uint8_t)(sizeof(g_fl_lines) / sizeof(g_fl_lines[0])),
                                 1u);
}

/* === Рендер всех линий борда ========================================= */

void board_fl_led_render_all(void)
{
    board_led_backend_render_lines(g_fl_lines, (uint8_t)(sizeof(g_fl_lines) / sizeof(g_fl_lines[0])));
}

void board_fl_dma_tc(TIM_HandleTypeDef *htim)
{
    board_led_backend_dma_tc_lines(g_fl_lines,
                                   (uint8_t)(sizeof(g_fl_lines) / sizeof(g_fl_lines[0])),
                                   htim);
}
