#include "board_dashboard.h"
#include "main.h"       // htim1
#include <string.h>

/* TIM из CubeMX */
extern TIM_HandleTypeDef htim1;

/* Макрос длины DMA-буфера для конкретного количества диодов */
#define ZDMA_LEN(leds)   ((leds) * BYTES_PER_LED * 8u + WS_RESET_SLOTS)

/* === GRB и DMA буферы для всех линий этого модуля ===================== */

/* STRIP - основная линия по торпедо */
static uint8_t  dashboard_strip_fb[DASHBOARD_STRIP_LEDS * BYTES_PER_LED];
static uint16_t dashboard_strip_dmaA[ZDMA_LEN(DASHBOARD_STRIP_LEDS)];
static uint16_t dashboard_strip_dmaB[ZDMA_LEN(DASHBOARD_STRIP_LEDS)];

/* CENTER - центральная консоль */
static uint8_t  dashboard_center_fb[DASHBOARD_CENTER_LEDS * BYTES_PER_LED];
static uint16_t dashboard_center_dmaA[ZDMA_LEN(DASHBOARD_CENTER_LEDS)];
static uint16_t dashboard_center_dmaB[ZDMA_LEN(DASHBOARD_CENTER_LEDS)];

/* AC_VENTS - дефлекторы кондиционера */
static uint8_t  dashboard_ac_vents_fb[DASHBOARD_AC_VENTS_LEDS * BYTES_PER_LED];
static uint16_t dashboard_ac_vents_dmaA[ZDMA_LEN(DASHBOARD_AC_VENTS_LEDS)];
static uint16_t dashboard_ac_vents_dmaB[ZDMA_LEN(DASHBOARD_AC_VENTS_LEDS)];

/* FOOTWELL - подсветка ног */
static uint8_t  dashboard_footwell_fb[DASHBOARD_FOOTWELL_LEDS * BYTES_PER_LED];
static uint16_t dashboard_footwell_dmaA[ZDMA_LEN(DASHBOARD_FOOTWELL_LEDS)];
static uint16_t dashboard_footwell_dmaB[ZDMA_LEN(DASHBOARD_FOOTWELL_LEDS)];

/* === Экземпляры ws2812_t (фактически физические линии/zones) ========= */

ws2812_t g_dashboard_strip;
ws2812_t g_dashboard_center;
ws2812_t g_dashboard_ac_vents;
ws2812_t g_dashboard_footwell;

/* === Маппинг логических зон → физические линии ======================= */
/* zones.c опирается на g_zone_map[WS_ZONE_*] */
/* Для dashboard используем: STRIP, HANDLE->CENTER, STORAGE->AC_VENTS, FOOTWELL */

const zone_map_t g_zone_map[WS_ZONE_MAX] = {
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

/* === Инициализация ==================================================== */

void board_dashboard_led_init(void)
{
    /* TIM1 должен быть настроен в CubeMX:
     * - PWM mode
     * - период под 800 кГц биттайминга WS2812
     * - включены каналы CH1..CH4 (где надо)
     * - DMA для соответствующих каналов
     */

    /* Основная линия (STRIP) на TIM1_CH1 */
    ws_init(&g_dashboard_strip,
            &htim1,
            TIM_CHANNEL_1,
            dashboard_strip_fb,
            dashboard_strip_dmaA,
            dashboard_strip_dmaB,
            DASHBOARD_STRIP_LEDS);

    /* Центральная консоль (CENTER) на TIM1_CH2 */
    ws_init(&g_dashboard_center,
            &htim1,
            TIM_CHANNEL_2,
            dashboard_center_fb,
            dashboard_center_dmaA,
            dashboard_center_dmaB,
            DASHBOARD_CENTER_LEDS);

    /* Дефлекторы кондиционера (AC_VENTS) на TIM1_CH3 */
    ws_init(&g_dashboard_ac_vents,
            &htim1,
            TIM_CHANNEL_3,
            dashboard_ac_vents_fb,
            dashboard_ac_vents_dmaA,
            dashboard_ac_vents_dmaB,
            DASHBOARD_AC_VENTS_LEDS);

    /* Подсветка ног (FOOTWELL) на TIM1_CH4 */
    ws_init(&g_dashboard_footwell,
            &htim1,
            TIM_CHANNEL_4,
            dashboard_footwell_fb,
            dashboard_footwell_dmaA,
            dashboard_footwell_dmaB,
            DASHBOARD_FOOTWELL_LEDS);

    /* Стартовое состояние: выкл, яркость 0 */
    ws_power_set(0);
}

/* === Рендер всех линий борда ========================================= */

void board_dashboard_led_render_all(void)
{
    /* Обычно свет рисует scene_player + zones.c,
     * а тут мы просто отправляем содержимое всех ws->grb в DMA.
     */

	// Главную (g_dashboard_strip) рендерит только player_tick
	// ws_render(&g_dashboard_strip);
    ws_render(&g_dashboard_center);
    ws_render(&g_dashboard_ac_vents);
    ws_render(&g_dashboard_footwell);
}
