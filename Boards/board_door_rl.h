#pragma once
/**
 ******************************************************************************
 * @file    board_door_rl.h
 * @brief   Rear-Left board hardware profile
 * @details Defines LED geometry, color order, and exported strip instances
 *          for the RL door board.
 ******************************************************************************
 */

#include "driver.h"
#include "zones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board type used by CAN discovery/election. */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_RL
#endif

/* LED count per physical line (tune per hardware revision). */
#define RL_STRIP_LEDS      100u   // main decorative strip
#define RL_HANDLE_LEDS     8u     // door handle accent
#define RL_STORAGE_LEDS    6u     // storage pocket accent
#define RL_FOOTWELL_LEDS   10u    // footwell accent

#define RL_STRIP_FIRST_LED     0u
#define RL_HANDLE_FIRST_LED    0u
#define RL_STORAGE_FIRST_LED   0u
#define RL_FOOTWELL_FIRST_LED  0u

#define RL_STRIP_ORDER     WS_COLOR_ORDER_GRB
#define RL_HANDLE_ORDER    WS_COLOR_ORDER_GRB
#define RL_STORAGE_ORDER   WS_COLOR_ORDER_GRB
#define RL_FOOTWELL_ORDER  WS_COLOR_ORDER_GRB

/* Driver instances for each physical line. */
extern ws2812_t g_rl_strip;
extern ws2812_t g_rl_handle;
extern ws2812_t g_rl_storage;
extern ws2812_t g_rl_footwell;

/** Initialize LED hardware resources for RL board. */
void board_rl_led_init(void);

/** Render all RL board LED lines. */
void board_rl_led_render_all(void);
/** Handle DMA transfer-complete interrupt for RL lines. */
void board_rl_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
