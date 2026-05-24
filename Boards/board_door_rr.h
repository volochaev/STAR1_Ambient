#pragma once
/**
 ******************************************************************************
 * @file    board_door_rr.h
 * @brief   Rear-Right board hardware profile
 * @details Defines LED geometry, color order, and exported strip instances
 *          for the RR door board.
 ******************************************************************************
 */

#include "driver.h"
#include "zones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board type used by CAN discovery/election. */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_RR
#endif

/* LED count per physical line (tune per hardware revision). */
#define RR_STRIP_LEDS      100u   // main decorative strip
#define RR_HANDLE_LEDS     8u     // door handle accent
#define RR_STORAGE_LEDS    6u     // storage pocket accent
#define RR_FOOTWELL_LEDS   10u    // footwell accent

#define RR_STRIP_FIRST_LED     0u
#define RR_HANDLE_FIRST_LED    0u
#define RR_STORAGE_FIRST_LED   0u
#define RR_FOOTWELL_FIRST_LED  0u

#define RR_STRIP_ORDER     WS_COLOR_ORDER_GRB
#define RR_HANDLE_ORDER    WS_COLOR_ORDER_GRB
#define RR_STORAGE_ORDER   WS_COLOR_ORDER_GRB
#define RR_FOOTWELL_ORDER  WS_COLOR_ORDER_GRB

/* Driver instances for each physical line. */
extern ws2812_t g_rr_strip;
extern ws2812_t g_rr_handle;
extern ws2812_t g_rr_storage;
extern ws2812_t g_rr_footwell;

/** Initialize LED hardware resources for RR board. */
void board_rr_led_init(void);

/** Render all RR board LED lines. */
void board_rr_led_render_all(void);
/** Handle DMA transfer-complete interrupt for RR lines. */
void board_rr_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
