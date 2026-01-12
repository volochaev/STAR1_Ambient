#pragma once
/**
 * @file board_door_fl.h
 * @brief Hardware mapping for Front-Left door ambient zones.
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
#define BOARD_TYPE  BOARD_TYPE_FL
#endif

/* Количество диодов в каждой физической линии (подправь под себя) */
#define FL_STRIP_LEDS      120u   // длинная линия по торпедо/вставке
#define FL_HANDLE_LEDS     8u     // ручка открывания
#define FL_STORAGE_LEDS    6u     // ниша/карман
#define FL_FOOTWELL_LEDS   10u    // подсветка ног

/* Экземпляры драйвера для каждой линии */
extern ws2812_t g_fl_strip;
extern ws2812_t g_fl_handle;
extern ws2812_t g_fl_storage;
extern ws2812_t g_fl_footwell;

/* Маппинг логических зон для zones.c */
extern const zone_map_t g_zone_map[WS_ZONE_MAX];

/* Инициализация железа ленты на этом борде (TIM, DMA, буферы) */
void board_fl_led_init(void);

/* Удобный хелпер: отрендерить все линии этого борда */
void board_fl_led_render_all(void);
