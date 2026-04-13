/**
 ******************************************************************************
 * @file    board_door_fl.h
 * @brief   Hardware configuration for Front-Left door ambient lighting board
 * @details Defines LED strip configuration, zone mapping, and hardware
 *          initialization for the front-left door ambient lighting module.
 *
 * @section Hardware Configuration
 * - LED strips: Main strip (120 LEDs), Handle (8 LEDs), Storage (6 LEDs), Footwell (10 LEDs)
 * - TIM channels: TIM1_CH1 (strip), TIM1_CH2 (handle), TIM1_CH3 (storage), TIM1_CH4 (footwell)
 * - Board type: BOARD_TYPE_FL (used for CAN discovery)
 *
 * @section Zone Mapping
 * Defines g_zone_map[] mapping logical zones (ZONE_STRIP, ZONE_HANDLE, etc.)
 * to physical LED strips for use by zones.c.
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include "driver.h"
#include "zones.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* Инициализация железа ленты на этом борде (TIM, DMA, буферы) */
void board_fl_led_init(void);

/* Удобный хелпер: отрендерить все линии этого борда */
void board_fl_led_render_all(void);
void board_fl_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif /* BOARD_DOOR_FL_H */
