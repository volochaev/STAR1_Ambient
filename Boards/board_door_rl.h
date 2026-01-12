#pragma once
/**
 * @file board_door_rl.h
 * @brief Hardware mapping for Rear-Left door ambient zones.
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
#define BOARD_TYPE  BOARD_TYPE_RL
#endif

/* Количество диодов в каждой физической линии (подправь под себя) */
#define RL_STRIP_LEDS      100u   // длинная линия по двери
#define RL_HANDLE_LEDS     8u     // ручка открывания
#define RL_STORAGE_LEDS    6u     // ниша/карман
#define RL_FOOTWELL_LEDS   10u    // подсветка ног

/* Экземпляры драйвера для каждой линии */
extern ws2812_t g_rl_strip;
extern ws2812_t g_rl_handle;
extern ws2812_t g_rl_storage;
extern ws2812_t g_rl_footwell;

/* Маппинг логических зон для zones.c */
extern const zone_map_t g_zone_map[WS_ZONE_MAX];

/* Инициализация железа ленты на этом борде (TIM, DMA, буферы) */
void board_rl_led_init(void);

/* Удобный хелпер: отрендерить все линии этого борда */
void board_rl_led_render_all(void);
