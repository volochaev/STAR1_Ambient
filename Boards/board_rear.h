#pragma once
/**
 ******************************************************************************
 * @file    board_rear.h
 * @brief   Rear board hardware profile
 * @details Defines LED geometry, color order, and exported strip instances
 *          for rear cabin ambient lines.
 ******************************************************************************
 */

#include "driver.h"
#include "zones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board type used by CAN discovery/election. */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_REAR
#endif

/* LED count per physical line (tune per hardware revision). */
#define REAR_STRIP_LEDS      52u   // rear main ambient strip
#define REAR_HANDLE_LEDS     0u    // optional/unused on some variants
#define REAR_STORAGE_LEDS    0u    // optional/unused on some variants
#define REAR_FOOTWELL_LEDS   12u   // rear footwell accent

#define REAR_STRIP_FIRST_LED     0u
#define REAR_HANDLE_FIRST_LED    0u
#define REAR_STORAGE_FIRST_LED   0u
#define REAR_FOOTWELL_FIRST_LED  0u

#define REAR_STRIP_ORDER     WS_COLOR_ORDER_GRB
#define REAR_HANDLE_ORDER    WS_COLOR_ORDER_GRB
#define REAR_STORAGE_ORDER   WS_COLOR_ORDER_GRB
#define REAR_FOOTWELL_ORDER  WS_COLOR_ORDER_GRB

/* Driver instances for each physical line. */
extern ws2812_t g_rear_strip;
extern ws2812_t g_rear_handle;
extern ws2812_t g_rear_storage;
extern ws2812_t g_rear_footwell;

/** Initialize LED hardware resources for rear board. */
void board_rear_led_init(void);

/** Render all rear board LED lines. */
void board_rear_led_render_all(void);
/** Handle DMA transfer-complete interrupt for rear lines. */
void board_rear_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
