#include "board_door_fr.h"
#include "main.h"       // htim1
#include <string.h>

/* TIM из CubeMX */
extern TIM_HandleTypeDef htim1;

/* Макрос длины DMA-буфера для конкретного количества диодов */
#define ZDMA_LEN(leds)   ((leds) * BYTES_PER_LED * 8u + WS_RESET_SLOTS)

/* === GRB и DMA буферы для всех линий этой двери ===================== */

/* STRIP */
static uint8_t  fr_strip_fb[FR_STRIP_LEDS * BYTES_PER_LED];
static uint16_t fr_strip_dmaA[ZDMA_LEN(FR_STRIP_LEDS)];
static uint16_t fr_strip_dmaB[ZDMA_LEN(FR_STRIP_LEDS)];

/* HANDLE */
static uint8_t  fr_handle_fb[FR_HANDLE_LEDS * BYTES_PER_LED];
static uint16_t fr_handle_dmaA[ZDMA_LEN(FR_HANDLE_LEDS)];
static uint16_t fr_handle_dmaB[ZDMA_LEN(FR_HANDLE_LEDS)];

/* STORAGE */
static uint8_t  fr_storage_fb[FR_STORAGE_LEDS * BYTES_PER_LED];
static uint16_t fr_storage_dmaA[ZDMA_LEN(FR_STORAGE_LEDS)];
static uint16_t fr_storage_dmaB[ZDMA_LEN(FR_STORAGE_LEDS)];

/* FOOTWELL */
static uint8_t  fr_footwell_fb[FR_FOOTWELL_LEDS * BYTES_PER_LED];
static uint16_t fr_footwell_dmaA[ZDMA_LEN(FR_FOOTWELL_LEDS)];
static uint16_t fr_footwell_dmaB[ZDMA_LEN(FR_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_fr_strip;
ws2812_t g_fr_handle;
ws2812_t g_fr_storage;
ws2812_t g_fr_footwell;

/* === Маппинг логических зон → физические линии ======================= */
/* zones.c опирается на g_zone_map[WS_ZONE_*] */
#if defined(BOARD_TYPE) && BOARD_TYPE == BOARD_TYPE_FR

__attribute__((weak)) const zone_map_t g_zone_map[WS_ZONE_MAX] = {
    [WS_ZONE_STRIP] = {
        .ws    = &g_fr_strip,
        .first = 0,
        .count = FR_STRIP_LEDS,
    },
    [WS_ZONE_HANDLE] = {
        .ws    = &g_fr_handle,
        .first = 0,
        .count = FR_HANDLE_LEDS,
    },
    [WS_ZONE_STORAGE] = {
        .ws    = &g_fr_storage,
        .first = 0,
        .count = FR_STORAGE_LEDS,
    },
    [WS_ZONE_FOOTWELL] = {
        .ws    = &g_fr_footwell,
        .first = 0,
        .count = FR_FOOTWELL_LEDS,
    },
};
#endif /* BOARD_TYPE == BOARD_TYPE_FR */

/* === Инициализация ==================================================== */

void board_fr_led_init(void)
{
    /* TIM1 должен быть настроен в CubeMX:
     * - PWM mode
     * - период под 800 кГц биттайминга WS2812
     * - включены каналы CH1..CH4 (где надо)
     * - DMA для соответствующих каналов
     */

    /* Основная линия (STRIP) на TIM1_CH1 */
    ws_init(&g_fr_strip,
            &htim1,
            TIM_CHANNEL_1,
            fr_strip_fb,
            fr_strip_dmaA,
            fr_strip_dmaB,
            FR_STRIP_LEDS);

    /* Ручка (HANDLE) на TIM1_CH2 */
    ws_init(&g_fr_handle,
            &htim1,
            TIM_CHANNEL_2,
            fr_handle_fb,
            fr_handle_dmaA,
            fr_handle_dmaB,
            FR_HANDLE_LEDS);

    /* Ниша (STORAGE) на TIM1_CH3 */
    ws_init(&g_fr_storage,
            &htim1,
            TIM_CHANNEL_3,
            fr_storage_fb,
            fr_storage_dmaA,
            fr_storage_dmaB,
            FR_STORAGE_LEDS);

    /* Подсветка ног (FOOTWELL) на TIM1_CH4 */
    ws_init(&g_fr_footwell,
            &htim1,
            TIM_CHANNEL_4,
            fr_footwell_fb,
            fr_footwell_dmaA,
            fr_footwell_dmaB,
            FR_FOOTWELL_LEDS);

    /* Стартовое состояние: выкл, яркость 0 */
    ws_power_set(0);
}

/* === Рендер всех линий борда ========================================= */

void board_fr_led_render_all(void)
{
    /* Обычно свет рисует scene_player + zones.c,
     * а тут мы просто отправляем содержимое всех ws->grb в DMA.
     */

	// Главную (g_fr_strip) рендерит только player_tick
	// ws_render(&g_fr_strip);
    ws_render(&g_fr_handle);
    ws_render(&g_fr_storage);
    ws_render(&g_fr_footwell);
}
