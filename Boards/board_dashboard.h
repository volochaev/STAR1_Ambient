#pragma once
/**
 * @file board_dashboard.h
 * @brief Hardware mapping for Dashboard/Instrument Panel ambient zones.
 *
 * Тут описываем:
 *  - какие физические WS2812-линии есть на модуле
 *  - сколько в них диодов
 *  - какие TIM-каналы за них отвечают
 *  - extern g_zone_map[] для zones.c
 */

#include "driver.h"
#include "zones.h"

/* Определение типа платы для CAN discovery */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_DASHBOARD
#endif

/* Количество диодов в каждой физической линии (подправь под себя) */
#define DASHBOARD_STRIP_LEDS      150u   // основная линия по торпедо
#define DASHBOARD_CENTER_LEDS     60u   // центральная консоль
#define DASHBOARD_AC_VENTS_LEDS    40u   // подсветка дефлекторов кондиционера
#define DASHBOARD_FOOTWELL_LEDS    12u   // подсветка ног водителя/пассажира

/* Экземпляры драйвера для каждой линии */
extern ws2812_t g_dashboard_strip;
extern ws2812_t g_dashboard_center;
extern ws2812_t g_dashboard_ac_vents;
extern ws2812_t g_dashboard_footwell;

/* Маппинг логических зон для zones.c */
extern const zone_map_t g_zone_map[WS_ZONE_MAX];

/* Инициализация железа ленты на этом борде (TIM, DMA, буферы) */
void board_dashboard_led_init(void);

/* Удобный хелпер: отрендерить все линии этого борда */
void board_dashboard_led_render_all(void);
