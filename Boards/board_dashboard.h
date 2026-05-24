#pragma once
/**
 ******************************************************************************
 * @file    board_dashboard.h
 * @brief   Dashboard board hardware profile
 * @details Defines LED geometry, color order, and exported strip instances
 *          for dashboard/center/vent/footwell lines.
 ******************************************************************************
 */

#include "driver.h"
#include "zones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board type used by CAN discovery/election. */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_DASHBOARD
#endif

/* LED count per physical line (tune per hardware revision). */
#define DASHBOARD_STRIP_LEDS      150u   // main dashboard strip
#define DASHBOARD_CENTER_LEDS     60u    // center console strip
#define DASHBOARD_AC_VENTS_LEDS   40u    // AC vent accent strip
#define DASHBOARD_FOOTWELL_LEDS   12u    // front footwell accent

#define DASHBOARD_STRIP_FIRST_LED      0u
#define DASHBOARD_CENTER_FIRST_LED     0u
#define DASHBOARD_AC_VENTS_FIRST_LED   0u
#define DASHBOARD_FOOTWELL_FIRST_LED   0u

#define DASHBOARD_STRIP_ORDER      WS_COLOR_ORDER_GRB
#define DASHBOARD_CENTER_ORDER     WS_COLOR_ORDER_GRB
#define DASHBOARD_AC_VENTS_ORDER   WS_COLOR_ORDER_GRB
#define DASHBOARD_FOOTWELL_ORDER   WS_COLOR_ORDER_GRB

/* Driver instances for each physical line. */
extern ws2812_t g_dashboard_strip;
extern ws2812_t g_dashboard_center;
extern ws2812_t g_dashboard_ac_vents;
extern ws2812_t g_dashboard_footwell;

/** Initialize LED hardware resources for dashboard board. */
void board_dashboard_led_init(void);

/** Render all dashboard board LED lines. */
void board_dashboard_led_render_all(void);
/** Handle DMA transfer-complete interrupt for dashboard lines. */
void board_dashboard_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
