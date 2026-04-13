#pragma once
/**
 * @file board_rear.h
 * @brief Hardware mapping for Rear ambient zones.
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
#define BOARD_TYPE  BOARD_TYPE_REAR
#endif

/* Количество диодов в каждой физической линии (подправь под себя) */
#define REAR_STRIP_LEDS      100u   // основная линия задней подсветки
#define REAR_HANDLE_LEDS     0u     // может отсутствовать
#define REAR_STORAGE_LEDS    0u     // может отсутствовать
#define REAR_FOOTWELL_LEDS   12u    // подсветка ног задних пассажиров

/* Экземпляры драйвера для каждой линии */
extern ws2812_t g_rear_strip;
extern ws2812_t g_rear_handle;
extern ws2812_t g_rear_storage;
extern ws2812_t g_rear_footwell;

/* Инициализация железа ленты на этом борде (TIM, DMA, буферы) */
void board_rear_led_init(void);

/* Удобный хелпер: отрендерить все линии этого борда */
void board_rear_led_render_all(void);
void board_rear_dma_tc(TIM_HandleTypeDef *htim);
