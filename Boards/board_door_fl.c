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
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim1;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */
/* Using macros from board_common.h with __ALIGNED(4) for optimal DMA */

/* STRIP */
static uint8_t fl_strip_fb[FL_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t fl_strip_dmaA[BOARD_DMA_BUF_LEN(FL_STRIP_LEDS)];
__ALIGNED(4) static uint16_t fl_strip_dmaB[BOARD_DMA_BUF_LEN(FL_STRIP_LEDS)];

/* HANDLE */
static uint8_t fl_handle_fb[FL_HANDLE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t fl_handle_dmaA[BOARD_DMA_BUF_LEN(FL_HANDLE_LEDS)];
__ALIGNED(4) static uint16_t fl_handle_dmaB[BOARD_DMA_BUF_LEN(FL_HANDLE_LEDS)];

/* STORAGE */
static uint8_t fl_storage_fb[FL_STORAGE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t fl_storage_dmaA[BOARD_DMA_BUF_LEN(FL_STORAGE_LEDS)];
__ALIGNED(4) static uint16_t fl_storage_dmaB[BOARD_DMA_BUF_LEN(FL_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t fl_footwell_fb[FL_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t fl_footwell_dmaA[BOARD_DMA_BUF_LEN(FL_FOOTWELL_LEDS)];
__ALIGNED(4) static uint16_t fl_footwell_dmaB[BOARD_DMA_BUF_LEN(FL_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_fl_strip;
ws2812_t g_fl_handle;
ws2812_t g_fl_storage;
ws2812_t g_fl_footwell;

/* === Маппинг логических зон → физические линии ======================= */
/* zones.c опирается на g_zone_map[WS_ZONE_*] */
/* Определяем только если BOARD_TYPE соответствует этому файлу */
#if defined(BOARD_TYPE) && BOARD_TYPE == BOARD_TYPE_FL

__attribute__((weak)) const zone_map_t g_zone_map[WS_ZONE_MAX] = {
    [WS_ZONE_STRIP] = {
        .ws    = &g_fl_strip,
        .first = 0,
        .count = FL_STRIP_LEDS,
    },
    [WS_ZONE_HANDLE] = {
        .ws    = &g_fl_handle,
        .first = 0,
        .count = FL_HANDLE_LEDS,
    },
    [WS_ZONE_STORAGE] = {
        .ws    = &g_fl_storage,
        .first = 0,
        .count = FL_STORAGE_LEDS,
    },
    [WS_ZONE_FOOTWELL] = {
        .ws    = &g_fl_footwell,
        .first = 0,
        .count = FL_FOOTWELL_LEDS,
    },
};
#endif /* BOARD_TYPE == BOARD_TYPE_FL */

/* === Инициализация ==================================================== */

void board_fl_led_init(void)
{
    /* TIM1 должен быть настроен в CubeMX:
     * - PWM mode
     * - период под 800 кГц биттайминга WS2812
     * - включены каналы CH1..CH4 (где надо)
     * - DMA для соответствующих каналов
     */

    /* Основная линия (STRIP) на TIM1_CH1 */
    ws_init(&g_fl_strip,
            &htim1,
            TIM_CHANNEL_1,
            fl_strip_fb,
            fl_strip_dmaA,
            fl_strip_dmaB,
            FL_STRIP_LEDS);

    /* Ручка (HANDLE) на TIM1_CH2 */
    ws_init(&g_fl_handle,
            &htim1,
            TIM_CHANNEL_2,
            fl_handle_fb,
            fl_handle_dmaA,
            fl_handle_dmaB,
            FL_HANDLE_LEDS);

    /* Ниша (STORAGE) на TIM1_CH3 */
    ws_init(&g_fl_storage,
            &htim1,
            TIM_CHANNEL_3,
            fl_storage_fb,
            fl_storage_dmaA,
            fl_storage_dmaB,
            FL_STORAGE_LEDS);

    /* Подсветка ног (FOOTWELL) на TIM1_CH4 */
    ws_init(&g_fl_footwell,
            &htim1,
            TIM_CHANNEL_4,
            fl_footwell_fb,
            fl_footwell_dmaA,
            fl_footwell_dmaB,
            FL_FOOTWELL_LEDS);

    /* Стартовое состояние: выкл, яркость 0 */
    ws_power_set(0);
}

/* === Рендер всех линий борда ========================================= */

void board_fl_led_render_all(void)
{
    /* Обычно свет рисует scene_player + zones.c,
     * а тут мы просто отправляем содержимое всех ws->rgb в DMA.
     */

	// Главную (g_fl_strip) рендерит только player_tick
	ws_render(&g_fl_strip);
    ws_render(&g_fl_handle);
    ws_render(&g_fl_storage);
    ws_render(&g_fl_footwell);
}
