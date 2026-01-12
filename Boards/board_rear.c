#include "board_rear.h"
#include "main.h"       // htim1
#include <string.h>

/* TIM из CubeMX */
extern TIM_HandleTypeDef htim1;

/* Макрос длины DMA-буфера для конкретного количества диодов */
#define ZDMA_LEN(leds)   ((leds) * BYTES_PER_LED * 8u + WS_RESET_SLOTS)

/* === GRB и DMA буферы для всех линий этого модуля ===================== */

/* STRIP */
static uint8_t  rear_strip_fb[REAR_STRIP_LEDS * BYTES_PER_LED];
static uint16_t rear_strip_dmaA[ZDMA_LEN(REAR_STRIP_LEDS)];
static uint16_t rear_strip_dmaB[ZDMA_LEN(REAR_STRIP_LEDS)];

/* HANDLE - может отсутствовать, но буферы нужны для совместимости */
static uint8_t  rear_handle_fb[1 * BYTES_PER_LED];
static uint16_t rear_handle_dmaA[ZDMA_LEN(1)];
static uint16_t rear_handle_dmaB[ZDMA_LEN(1)];

/* STORAGE - может отсутствовать, но буферы нужны для совместимости */
static uint8_t  rear_storage_fb[1 * BYTES_PER_LED];
static uint16_t rear_storage_dmaA[ZDMA_LEN(1)];
static uint16_t rear_storage_dmaB[ZDMA_LEN(1)];

/* FOOTWELL */
static uint8_t  rear_footwell_fb[REAR_FOOTWELL_LEDS * BYTES_PER_LED];
static uint16_t rear_footwell_dmaA[ZDMA_LEN(REAR_FOOTWELL_LEDS)];
static uint16_t rear_footwell_dmaB[ZDMA_LEN(REAR_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_rear_strip;
ws2812_t g_rear_handle;
ws2812_t g_rear_storage;
ws2812_t g_rear_footwell;

/* === Маппинг логических зон → физические линии ======================= */
/* zones.c опирается на g_zone_map[WS_ZONE_*] */
#if defined(BOARD_TYPE) && BOARD_TYPE == BOARD_TYPE_REAR

__attribute__((weak)) const zone_map_t g_zone_map[WS_ZONE_MAX] = {
    [WS_ZONE_STRIP] = {
        .ws    = &g_rear_strip,
        .first = 0,
        .count = REAR_STRIP_LEDS,
    },
    [WS_ZONE_HANDLE] = {
        .ws    = (REAR_HANDLE_LEDS > 0) ? &g_rear_handle : NULL,
        .first = 0,
        .count = REAR_HANDLE_LEDS,
    },
    [WS_ZONE_STORAGE] = {
        .ws    = (REAR_STORAGE_LEDS > 0) ? &g_rear_storage : NULL,
        .first = 0,
        .count = REAR_STORAGE_LEDS,
    },
    [WS_ZONE_FOOTWELL] = {
        .ws    = &g_rear_footwell,
        .first = 0,
        .count = REAR_FOOTWELL_LEDS,
    },
};
#endif /* BOARD_TYPE == BOARD_TYPE_REAR */

/* === Инициализация ==================================================== */

void board_rear_led_init(void)
{
    /* TIM1 должен быть настроен в CubeMX:
     * - PWM mode
     * - период под 800 кГц биттайминга WS2812
     * - включены каналы CH1..CH4 (где надо)
     * - DMA для соответствующих каналов
     */

    /* Основная линия (STRIP) на TIM1_CH1 */
    ws_init(&g_rear_strip,
            &htim1,
            TIM_CHANNEL_1,
            rear_strip_fb,
            rear_strip_dmaA,
            rear_strip_dmaB,
            REAR_STRIP_LEDS);

    /* Ручка (HANDLE) на TIM1_CH2 - инициализируем всегда (count=0 если нет) */
    ws_init(&g_rear_handle,
            &htim1,
            TIM_CHANNEL_2,
            rear_handle_fb,
            rear_handle_dmaA,
            rear_handle_dmaB,
            REAR_HANDLE_LEDS);

    /* Ниша (STORAGE) на TIM1_CH3 - инициализируем всегда (count=0 если нет) */
    ws_init(&g_rear_storage,
            &htim1,
            TIM_CHANNEL_3,
            rear_storage_fb,
            rear_storage_dmaA,
            rear_storage_dmaB,
            REAR_STORAGE_LEDS);

    /* Подсветка ног (FOOTWELL) на TIM1_CH4 */
    ws_init(&g_rear_footwell,
            &htim1,
            TIM_CHANNEL_4,
            rear_footwell_fb,
            rear_footwell_dmaA,
            rear_footwell_dmaB,
            REAR_FOOTWELL_LEDS);

    /* Стартовое состояние: выкл, яркость 0 */
    ws_power_set(0);
}

/* === Рендер всех линий борда ========================================= */

void board_rear_led_render_all(void)
{
    /* Обычно свет рисует scene_player + zones.c,
     * а тут мы просто отправляем содержимое всех ws->grb в DMA.
     */

	// Главную (g_rear_strip) рендерит только player_tick
	// ws_render(&g_rear_strip);
    if (REAR_HANDLE_LEDS > 0) {
        ws_render(&g_rear_handle);
    }
    if (REAR_STORAGE_LEDS > 0) {
        ws_render(&g_rear_storage);
    }
    ws_render(&g_rear_footwell);
}
