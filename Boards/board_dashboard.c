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

/* === Маппинг логических зон → физические линии ======================= */
/* zones.c опирается на g_zone_map[WS_ZONE_*] */
/* Для dashboard используем: STRIP, HANDLE->CENTER, STORAGE->AC_VENTS, FOOTWELL */
#if defined(BOARD_TYPE) && BOARD_TYPE == BOARD_TYPE_DASHBOARD

__attribute__((weak)) const zone_map_t g_zone_map[WS_ZONE_MAX] = {
    [WS_ZONE_STRIP] = {
        .ws    = &g_dashboard_strip,
        .first = 0,
        .count = DASHBOARD_STRIP_LEDS,
    },
    [WS_ZONE_HANDLE] = {
        .ws    = &g_dashboard_center,
        .first = 0,
        .count = DASHBOARD_CENTER_LEDS,
    },
    [WS_ZONE_STORAGE] = {
        .ws    = &g_dashboard_ac_vents,
        .first = 0,
        .count = DASHBOARD_AC_VENTS_LEDS,
    },
    [WS_ZONE_FOOTWELL] = {
        .ws    = &g_dashboard_footwell,
        .first = 0,
        .count = DASHBOARD_FOOTWELL_LEDS,
    },
};
#endif /* BOARD_TYPE == BOARD_TYPE_DASHBOARD */

/* === Инициализация ==================================================== */

void board_dashboard_led_init(void)
{
    /* TIM2 должен быть настроен в CubeMX:
     * - PWM mode
     * - период под 800 кГц биттайминга WS2812
     * - включены каналы CH1..CH4 (где надо)
     * - DMA для соответствующих каналов
     */

    /* Основная линия (STRIP) на TIM2_CH1 */
    ws_init(&g_dashboard_strip,
            &htim2,
            TIM_CHANNEL_1,
            dashboard_strip_fb,
            dashboard_strip_dma,
            DASHBOARD_STRIP_LEDS);

    /* Центральная консоль (CENTER) на TIM2_CH2 */
    ws_init(&g_dashboard_center,
            &htim2,
            TIM_CHANNEL_2,
            dashboard_center_fb,
            dashboard_center_dma,
            DASHBOARD_CENTER_LEDS);

    /* Дефлекторы кондиционера (AC_VENTS) на TIM2_CH3 */
    ws_init(&g_dashboard_ac_vents,
            &htim2,
            TIM_CHANNEL_3,
            dashboard_ac_vents_fb,
            dashboard_ac_vents_dma,
            DASHBOARD_AC_VENTS_LEDS);

    /* Подсветка ног (FOOTWELL) на TIM2_CH4 */
    ws_init(&g_dashboard_footwell,
            &htim2,
            TIM_CHANNEL_4,
            dashboard_footwell_fb,
            dashboard_footwell_dma,
            DASHBOARD_FOOTWELL_LEDS);

    /* Стартовое состояние: выкл, яркость 0 */
    ws_power_set(0);
}

/* === Рендер всех линий борда ========================================= */

void board_dashboard_led_render_all(void)
{
    /* Обычно свет рисует scene_player + zones.c,
     * а тут мы просто отправляем содержимое всех ws->rgb в DMA.
     */

	// Главную (g_dashboard_strip) рендерит только player_tick
	// ws_render(&g_dashboard_strip);
    ws_render(&g_dashboard_center);
    ws_render(&g_dashboard_ac_vents);
    ws_render(&g_dashboard_footwell);
}
