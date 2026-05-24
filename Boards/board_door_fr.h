#pragma once
/**
 ******************************************************************************
 * @file    board_door_fr.h
 * @brief   Front-Right board hardware profile
 * @details Defines LED geometry, color order, and exported strip instances
 *          for the FR door board.
 ******************************************************************************
 */

#include "driver.h"
#include "zones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board type used by CAN discovery/election. */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_FR
#endif

/* LED count per physical line (tune per hardware revision). */
#define FR_STRIP_LEDS      120u   // main decorative strip
#define FR_HANDLE_LEDS     1u     // door handle accent
#define FR_STORAGE_LEDS    1u     // storage pocket accent
#define FR_FOOTWELL_LEDS   1u     // footwell accent

#define FR_STRIP_FIRST_LED     10u
#define FR_HANDLE_FIRST_LED    0u
#define FR_STORAGE_FIRST_LED   0u
#define FR_FOOTWELL_FIRST_LED  0u

#define FR_STRIP_ORDER     WS_COLOR_ORDER_RGB
#define FR_HANDLE_ORDER    WS_COLOR_ORDER_GRB
#define FR_STORAGE_ORDER   WS_COLOR_ORDER_GRB
#define FR_FOOTWELL_ORDER  WS_COLOR_ORDER_GRB

/* Driver instances for each physical line. */
extern ws2812_t g_fr_strip;
extern ws2812_t g_fr_handle;
extern ws2812_t g_fr_storage;
extern ws2812_t g_fr_footwell;

/** Initialize LED hardware resources for FR board. */
void board_fr_led_init(void);

/** Render all FR board LED lines. */
void board_fr_led_render_all(void);
/** Handle DMA transfer-complete interrupt for FR lines. */
void board_fr_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
