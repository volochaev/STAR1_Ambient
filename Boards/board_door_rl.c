/**
 ******************************************************************************
 * @file    board_door_rl.c
 * @brief   Rear-Left door board implementation
 * @version 2.1
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"        /* BOARD_TYPE_* definitions */
#include "board_door_rl.h"
#include "board_led_backend.h"
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim2;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */

/* STRIP */
static uint8_t rl_strip_fb[RL_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rl_strip_dma[BOARD_DMA_BUF_LEN(RL_STRIP_LEDS)];

/* HANDLE */
static uint8_t rl_handle_fb[RL_HANDLE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rl_handle_dma[BOARD_DMA_BUF_LEN(RL_HANDLE_LEDS)];

/* STORAGE */
static uint8_t rl_storage_fb[RL_STORAGE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rl_storage_dma[BOARD_DMA_BUF_LEN(RL_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t rl_footwell_fb[RL_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t rl_footwell_dma[BOARD_DMA_BUF_LEN(RL_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_rl_strip;
ws2812_t g_rl_handle;
ws2812_t g_rl_storage;
ws2812_t g_rl_footwell;

static const board_led_line_t g_rl_lines[] = {
    { &g_rl_strip, TIM_CHANNEL_1, rl_strip_fb, rl_strip_dma, RL_STRIP_LEDS, 1u },
    { &g_rl_handle, TIM_CHANNEL_2, rl_handle_fb, rl_handle_dma, RL_HANDLE_LEDS, 1u },
    { &g_rl_storage, TIM_CHANNEL_3, rl_storage_fb, rl_storage_dma, RL_STORAGE_LEDS, 1u },
    { &g_rl_footwell, TIM_CHANNEL_4, rl_footwell_fb, rl_footwell_dma, RL_FOOTWELL_LEDS, 1u },
};

/* === Инициализация ==================================================== */

void board_rl_led_init(void)
{
    board_led_backend_init_lines(&htim2,
                                 g_rl_lines,
                                 (uint8_t)(sizeof(g_rl_lines) / sizeof(g_rl_lines[0])),
                                 1u);
}

/* === Рендер всех линий борда ========================================= */

void board_rl_led_render_all(void)
{
    board_led_backend_render_lines(g_rl_lines, (uint8_t)(sizeof(g_rl_lines) / sizeof(g_rl_lines[0])));
}

void board_rl_dma_tc(TIM_HandleTypeDef *htim)
{
    board_led_backend_dma_tc_lines(g_rl_lines,
                                   (uint8_t)(sizeof(g_rl_lines) / sizeof(g_rl_lines[0])),
                                   htim);
}
