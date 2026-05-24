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

/* Board type used by CAN discovery/election. */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_FL
#endif

/* LED count per physical line (tune per hardware revision). */
#define FL_STRIP_LEDS      50u   // main decorative strip
#define FL_HANDLE_LEDS     8u    // door handle accent
#define FL_STORAGE_LEDS    6u    // storage pocket accent
#define FL_FOOTWELL_LEDS   10u   // footwell accent

#define FL_STRIP_FIRST_LED     0u
#define FL_HANDLE_FIRST_LED    0u
#define FL_STORAGE_FIRST_LED   0u
#define FL_FOOTWELL_FIRST_LED  0u

#define FL_STRIP_ORDER     WS_COLOR_ORDER_GRB
#define FL_HANDLE_ORDER    WS_COLOR_ORDER_GRB
#define FL_STORAGE_ORDER   WS_COLOR_ORDER_GRB
#define FL_FOOTWELL_ORDER  WS_COLOR_ORDER_GRB

/* Driver instances for each physical line. */
extern ws2812_t g_fl_strip;
extern ws2812_t g_fl_handle;
extern ws2812_t g_fl_storage;
extern ws2812_t g_fl_footwell;

/* Initialize board LED hardware resources (TIM/DMA/frame buffers). */
void board_fl_led_init(void);

/* Render all LED lines owned by this board. */
void board_fl_led_render_all(void);
void board_fl_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif /* BOARD_DOOR_FL_H */
