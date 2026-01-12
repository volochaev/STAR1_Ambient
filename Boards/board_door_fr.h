#pragma once
/**
 * @file board_door_fr.h
 * @brief Hardware mapping for Front-Right door ambient zones.
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
#define BOARD_TYPE  BOARD_TYPE_FR
#endif

/* Количество диодов в каждой физической линии (подправь под себя) */
#define FR_STRIP_LEDS      120u   // длинная линия по торпедо/вставке
#define FR_HANDLE_LEDS     8u     // ручка открывания
#define FR_STORAGE_LEDS    6u     // ниша/карман
#define FR_FOOTWELL_LEDS   10u    // подсветка ног

/* Экземпляры драйвера для каждой линии */
extern ws2812_t g_fr_strip;
extern ws2812_t g_fr_handle;
extern ws2812_t g_fr_storage;
extern ws2812_t g_fr_footwell;

/* Маппинг логических зон для zones.c */
extern const zone_map_t g_zone_map[WS_ZONE_MAX];

/* Инициализация железа ленты на этом борде (TIM, DMA, буферы) */
void board_fr_led_init(void);

/* Удобный хелпер: отрендерить все линии этого борда */
void board_fr_led_render_all(void);
