#include "board_door_rr.h"
#include "main.h"       // htim1
#include <string.h>

/* TIM из CubeMX */
extern TIM_HandleTypeDef htim1;

/* Макрос длины DMA-буфера для конкретного количества диодов */
#define ZDMA_LEN(leds)   ((leds) * BYTES_PER_LED * 8u + WS_RESET_SLOTS)

/* === GRB и DMA буферы для всех линий этой двери ===================== */

/* STRIP */
static uint8_t  rr_strip_fb[RR_STRIP_LEDS * BYTES_PER_LED];
static uint16_t rr_strip_dmaA[ZDMA_LEN(RR_STRIP_LEDS)];
static uint16_t rr_strip_dmaB[ZDMA_LEN(RR_STRIP_LEDS)];

/* HANDLE */
static uint8_t  rr_handle_fb[RR_HANDLE_LEDS * BYTES_PER_LED];
static uint16_t rr_handle_dmaA[ZDMA_LEN(RR_HANDLE_LEDS)];
static uint16_t rr_handle_dmaB[ZDMA_LEN(RR_HANDLE_LEDS)];

/* STORAGE */
static uint8_t  rr_storage_fb[RR_STORAGE_LEDS * BYTES_PER_LED];
static uint16_t rr_storage_dmaA[ZDMA_LEN(RR_STORAGE_LEDS)];
static uint16_t rr_storage_dmaB[ZDMA_LEN(RR_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t  rr_footwell_fb[RR_FOOTWELL_LEDS * BYTES_PER_LED];
static uint16_t rr_footwell_dmaA[ZDMA_LEN(RR_FOOTWELL_LEDS)];
static uint16_t rr_footwell_dmaB[ZDMA_LEN(RR_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_rr_strip;
ws2812_t g_rr_handle;
ws2812_t g_rr_storage;
ws2812_t g_rr_footwell;

/* === Маппинг логических зон → физические линии ======================= */
/* zones.c опирается на g_zone_map[WS_ZONE_*] */
#if defined(BOARD_TYPE) && BOARD_TYPE == BOARD_TYPE_RR

__attribute__((weak)) const zone_map_t g_zone_map[WS_ZONE_MAX] = {
    [WS_ZONE_STRIP] = {
        .ws    = &g_rr_strip,
        .first = 0,
        .count = RR_STRIP_LEDS,
    },
    [WS_ZONE_HANDLE] = {
        .ws    = &g_rr_handle,
        .first = 0,
        .count = RR_HANDLE_LEDS,
    },
    [WS_ZONE_STORAGE] = {
        .ws    = &g_rr_storage,
        .first = 0,
        .count = RR_STORAGE_LEDS,
    },
    [WS_ZONE_FOOTWELL] = {
        .ws    = &g_rr_footwell,
        .first = 0,
        .count = RR_FOOTWELL_LEDS,
    },
};
#endif /* BOARD_TYPE == BOARD_TYPE_RR */

/* === Инициализация ==================================================== */

void board_rr_led_init(void)
{
    /* TIM1 должен быть настроен в CubeMX:
     * - PWM mode
     * - период под 800 кГц биттайминга WS2812
     * - включены каналы CH1..CH4 (где надо)
     * - DMA для соответствующих каналов
     */

    /* Основная линия (STRIP) на TIM1_CH1 */
    ws_init(&g_rr_strip,
            &htim1,
            TIM_CHANNEL_1,
            rr_strip_fb,
            rr_strip_dmaA,
            rr_strip_dmaB,
            RR_STRIP_LEDS);

    /* Ручка (HANDLE) на TIM1_CH2 */
    ws_init(&g_rr_handle,
            &htim1,
            TIM_CHANNEL_2,
            rr_handle_fb,
            rr_handle_dmaA,
            rr_handle_dmaB,
            RR_HANDLE_LEDS);

    /* Ниша (STORAGE) на TIM1_CH3 */
    ws_init(&g_rr_storage,
            &htim1,
            TIM_CHANNEL_3,
            rr_storage_fb,
            rr_storage_dmaA,
            rr_storage_dmaB,
            RR_STORAGE_LEDS);

    /* Подсветка ног (FOOTWELL) на TIM1_CH4 */
    ws_init(&g_rr_footwell,
            &htim1,
            TIM_CHANNEL_4,
            rr_footwell_fb,
            rr_footwell_dmaA,
            rr_footwell_dmaB,
            RR_FOOTWELL_LEDS);

    /* Стартовое состояние: выкл, яркость 0 */
    ws_power_set(0);
}

/* === Рендер всех линий борда ========================================= */

void board_rr_led_render_all(void)
{
    /* Обычно свет рисует scene_player + zones.c,
     * а тут мы просто отправляем содержимое всех ws->grb в DMA.
     */

	// Главную (g_rr_strip) рендерит только player_tick
	// ws_render(&g_rr_strip);
    ws_render(&g_rr_handle);
    ws_render(&g_rr_storage);
    ws_render(&g_rr_footwell);
}
