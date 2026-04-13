/**
 ******************************************************************************
 * @file    board_rear.c
 * @brief   Rear ambient board implementation
 * @version 2.1
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"        /* BOARD_TYPE_* definitions */
#include "board_rear.h"
#include "board_led_backend.h"
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim2;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */

/* STRIP */
static uint8_t rear_strip_fb[REAR_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rear_strip_dma[BOARD_DMA_BUF_LEN(REAR_STRIP_LEDS)];

/* HANDLE - may be absent, but buffers needed for compatibility */
static uint8_t rear_handle_fb[1 * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rear_handle_dma[BOARD_DMA_BUF_LEN(1)];

/* STORAGE - may be absent, but buffers needed for compatibility */
static uint8_t rear_storage_fb[1 * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rear_storage_dma[BOARD_DMA_BUF_LEN(1)];

/* FOOTWELL */
static uint8_t rear_footwell_fb[REAR_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rear_footwell_dma[BOARD_DMA_BUF_LEN(REAR_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_rear_strip;
ws2812_t g_rear_handle;
ws2812_t g_rear_storage;
ws2812_t g_rear_footwell;

static const board_led_line_t g_rear_lines[] = {
    { &g_rear_strip, TIM_CHANNEL_1, rear_strip_fb, rear_strip_dma, REAR_STRIP_LEDS, 1u },
    { &g_rear_handle, TIM_CHANNEL_2, rear_handle_fb, rear_handle_dma, REAR_HANDLE_LEDS, (REAR_HANDLE_LEDS > 0u) ? 1u : 0u },
    { &g_rear_storage, TIM_CHANNEL_3, rear_storage_fb, rear_storage_dma, REAR_STORAGE_LEDS, (REAR_STORAGE_LEDS > 0u) ? 1u : 0u },
    { &g_rear_footwell, TIM_CHANNEL_4, rear_footwell_fb, rear_footwell_dma, REAR_FOOTWELL_LEDS, 1u },
};

/* === Инициализация ==================================================== */

void board_rear_led_init(void)
{
    board_led_backend_init_lines(&htim2,
                                 g_rear_lines,
                                 (uint8_t)(sizeof(g_rear_lines) / sizeof(g_rear_lines[0])),
                                 1u);
}

/* === Рендер всех линий борда ========================================= */

void board_rear_led_render_all(void)
{
    board_led_backend_render_lines(g_rear_lines, (uint8_t)(sizeof(g_rear_lines) / sizeof(g_rear_lines[0])));
}

void board_rear_dma_tc(TIM_HandleTypeDef *htim)
{
    board_led_backend_dma_tc_lines(g_rear_lines,
                                   (uint8_t)(sizeof(g_rear_lines) / sizeof(g_rear_lines[0])),
                                   htim);
}
