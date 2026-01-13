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
#include "board_common.h"   /* Common macros with DMA alignment */

/* TIM from CubeMX */
extern TIM_HandleTypeDef htim1;

/* === RGB and DMA buffers for all zones (properly aligned for DMA) ======= */

/* STRIP */
static uint8_t rl_strip_fb[RL_STRIP_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t rl_strip_dmaA[BOARD_DMA_BUF_LEN(RL_STRIP_LEDS)];
__ALIGNED(4) static uint16_t rl_strip_dmaB[BOARD_DMA_BUF_LEN(RL_STRIP_LEDS)];

/* HANDLE */
static uint8_t rl_handle_fb[RL_HANDLE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t rl_handle_dmaA[BOARD_DMA_BUF_LEN(RL_HANDLE_LEDS)];
__ALIGNED(4) static uint16_t rl_handle_dmaB[BOARD_DMA_BUF_LEN(RL_HANDLE_LEDS)];

/* STORAGE */
static uint8_t rl_storage_fb[RL_STORAGE_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t rl_storage_dmaA[BOARD_DMA_BUF_LEN(RL_STORAGE_LEDS)];
__ALIGNED(4) static uint16_t rl_storage_dmaB[BOARD_DMA_BUF_LEN(RL_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t rl_footwell_fb[RL_FOOTWELL_LEDS * BYTES_PER_LED];
__ALIGNED(4) static uint16_t rl_footwell_dmaA[BOARD_DMA_BUF_LEN(RL_FOOTWELL_LEDS)];
__ALIGNED(4) static uint16_t rl_footwell_dmaB[BOARD_DMA_BUF_LEN(RL_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_rl_strip;
ws2812_t g_rl_handle;
ws2812_t g_rl_storage;
ws2812_t g_rl_footwell;

/* === Маппинг логических зон → физические линии ======================= */
/* zones.c опирается на g_zone_map[WS_ZONE_*] */
#if defined(BOARD_TYPE) && BOARD_TYPE == BOARD_TYPE_RL

__attribute__((weak)) const zone_map_t g_zone_map[WS_ZONE_MAX] = {
    [WS_ZONE_STRIP] = {
        .ws    = &g_rl_strip,
        .first = 0,
        .count = RL_STRIP_LEDS,
    },
    [WS_ZONE_HANDLE] = {
        .ws    = &g_rl_handle,
        .first = 0,
        .count = RL_HANDLE_LEDS,
    },
    [WS_ZONE_STORAGE] = {
        .ws    = &g_rl_storage,
        .first = 0,
        .count = RL_STORAGE_LEDS,
    },
    [WS_ZONE_FOOTWELL] = {
        .ws    = &g_rl_footwell,
        .first = 0,
        .count = RL_FOOTWELL_LEDS,
    },
};
#endif /* BOARD_TYPE == BOARD_TYPE_RL */

/* === Инициализация ==================================================== */

void board_rl_led_init(void)
{
    /* TIM1 должен быть настроен в CubeMX:
     * - PWM mode
     * - период под 800 кГц биттайминга WS2812
     * - включены каналы CH1..CH4 (где надо)
     * - DMA для соответствующих каналов
     */

    /* Основная линия (STRIP) на TIM1_CH1 */
    ws_init(&g_rl_strip,
            &htim1,
            TIM_CHANNEL_1,
            rl_strip_fb,
            rl_strip_dmaA,
            rl_strip_dmaB,
            RL_STRIP_LEDS);

    /* Ручка (HANDLE) на TIM1_CH2 */
    ws_init(&g_rl_handle,
            &htim1,
            TIM_CHANNEL_2,
            rl_handle_fb,
            rl_handle_dmaA,
            rl_handle_dmaB,
            RL_HANDLE_LEDS);

    /* Ниша (STORAGE) на TIM1_CH3 */
    ws_init(&g_rl_storage,
            &htim1,
            TIM_CHANNEL_3,
            rl_storage_fb,
            rl_storage_dmaA,
            rl_storage_dmaB,
            RL_STORAGE_LEDS);

    /* Подсветка ног (FOOTWELL) на TIM1_CH4 */
    ws_init(&g_rl_footwell,
            &htim1,
            TIM_CHANNEL_4,
            rl_footwell_fb,
            rl_footwell_dmaA,
            rl_footwell_dmaB,
            RL_FOOTWELL_LEDS);

    /* Стартовое состояние: выкл, яркость 0 */
    ws_power_set(0);
}

/* === Рендер всех линий борда ========================================= */

void board_rl_led_render_all(void)
{
    /* Обычно свет рисует scene_player + zones.c,
     * а тут мы просто отправляем содержимое всех ws->rgb в DMA.
     */

	// Главную (g_rl_strip) рендерит только player_tick
	// ws_render(&g_rl_strip);
    ws_render(&g_rl_handle);
    ws_render(&g_rl_storage);
    ws_render(&g_rl_footwell);
}
