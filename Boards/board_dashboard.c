/**
 ******************************************************************************
 * @file    board_dashboard.c
 * @brief   Dashboard board implementation
 * @version 2.1
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"        /* BOARD_TYPE_* definitions */
#include "board_dashboard.h"
#include "board_led_backend.h"
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim2;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */

/* STRIP - main dashboard strip */
static uint8_t dashboard_strip_fb[DASHBOARD_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t dashboard_strip_dma[BOARD_DMA_BUF_LEN(DASHBOARD_STRIP_LEDS)];

/* CENTER - center console */
static uint8_t dashboard_center_fb[DASHBOARD_CENTER_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t dashboard_center_dma[BOARD_DMA_BUF_LEN(DASHBOARD_CENTER_LEDS)];

/* AC_VENTS - AC vents */
static uint8_t dashboard_ac_vents_fb[DASHBOARD_AC_VENTS_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t dashboard_ac_vents_dma[BOARD_DMA_BUF_LEN(DASHBOARD_AC_VENTS_LEDS)];

/* FOOTWELL */
static uint8_t dashboard_footwell_fb[DASHBOARD_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint32_t dashboard_footwell_dma[BOARD_DMA_BUF_LEN(DASHBOARD_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_dashboard_strip;
ws2812_t g_dashboard_center;
ws2812_t g_dashboard_ac_vents;
ws2812_t g_dashboard_footwell;

static const board_led_line_t g_dashboard_lines[] = {
    { &g_dashboard_strip, TIM_CHANNEL_1, dashboard_strip_fb, dashboard_strip_dma, DASHBOARD_STRIP_LEDS, 1u },
    { &g_dashboard_center, TIM_CHANNEL_2, dashboard_center_fb, dashboard_center_dma, DASHBOARD_CENTER_LEDS, 1u },
    { &g_dashboard_ac_vents, TIM_CHANNEL_3, dashboard_ac_vents_fb, dashboard_ac_vents_dma, DASHBOARD_AC_VENTS_LEDS, 1u },
    { &g_dashboard_footwell, TIM_CHANNEL_4, dashboard_footwell_fb, dashboard_footwell_dma, DASHBOARD_FOOTWELL_LEDS, 1u },
};

/* === Инициализация ==================================================== */

void board_dashboard_led_init(void)
{
    board_led_backend_init_lines(&htim2,
                                 g_dashboard_lines,
                                 (uint8_t)(sizeof(g_dashboard_lines) / sizeof(g_dashboard_lines[0])),
                                 1u);
}

/* === Рендер всех линий борда ========================================= */

void board_dashboard_led_render_all(void)
{
    board_led_backend_render_lines(g_dashboard_lines,
                                   (uint8_t)(sizeof(g_dashboard_lines) / sizeof(g_dashboard_lines[0])));
}

void board_dashboard_dma_tc(TIM_HandleTypeDef *htim)
{
    board_led_backend_dma_tc_lines(g_dashboard_lines,
                                   (uint8_t)(sizeof(g_dashboard_lines) / sizeof(g_dashboard_lines[0])),
                                   htim);
}
