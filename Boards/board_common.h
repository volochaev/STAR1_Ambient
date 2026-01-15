/**
 ******************************************************************************
 * @file    board_common.h
 * @brief   Common macros and utilities for board implementations
 * @details Provides DMA buffer macros with proper alignment and common
 *          initialization patterns for all board types.
 *
 * @section Usage
 * Include this header in all board_xxx.c files before defining buffers.
 * Use DECLARE_ZONE_BUFFERS() macro to declare properly aligned buffers.
 *
 * @version 1.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include "driver.h"
#include "zones.h"
#include "main.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== DMA BUFFER ALIGNMENT ============================================ */
/* DMA buffers must be aligned to 4 bytes for optimal DMA performance.
 * Using __ALIGNED(4) ensures proper alignment on all compilers.
 */

#ifndef __ALIGNED
#define __ALIGNED(x) __attribute__((aligned(x)))
#endif

/* ====== BUFFER SIZE CALCULATION ========================================= */
/* DMA buffer length: (LEDs * 3 bytes/LED * 8 bits/byte) + reset slots */
#define BOARD_DMA_BUF_LEN(leds)   ((leds) * BYTES_PER_LED * 8u + WS_RESET_SLOTS)

/* ====== ZONE BUFFER DECLARATION MACROS ================================== */
/* These macros declare properly aligned DMA buffers for each zone.
 * Usage: DECLARE_ZONE_BUFFERS(prefix, STRIP_LEDS, HANDLE_LEDS, STORAGE_LEDS, FOOTWELL_LEDS)
 */

#define DECLARE_ZONE_FB(prefix, zone, leds) \
    static uint8_t prefix##_##zone##_fb[(leds) * BYTES_PER_LED]

#define DECLARE_ZONE_DMA(prefix, zone, leds) \
    __ALIGNED(4) static uint32_t prefix##_##zone##_dma[BOARD_DMA_BUF_LEN(leds)]

#define DECLARE_ZONE_BUFFERS_SINGLE(prefix, zone, leds) \
    DECLARE_ZONE_FB(prefix, zone, leds); \
    DECLARE_ZONE_DMA(prefix, zone, leds)

/* Full buffer declaration for all 4 zones */
#define DECLARE_ALL_ZONE_BUFFERS(prefix, strip_leds, handle_leds, storage_leds, footwell_leds) \
    DECLARE_ZONE_BUFFERS_SINGLE(prefix, strip, strip_leds); \
    DECLARE_ZONE_BUFFERS_SINGLE(prefix, handle, handle_leds); \
    DECLARE_ZONE_BUFFERS_SINGLE(prefix, storage, storage_leds); \
    DECLARE_ZONE_BUFFERS_SINGLE(prefix, footwell, footwell_leds)

/* ====== ZONE MAP DECLARATION MACRO ====================================== */
/* Declares g_zone_map array for zones.c */

#define DECLARE_ZONE_MAP(prefix, board_type, strip_leds, handle_leds, storage_leds, footwell_leds) \
    __attribute__((weak)) const zone_map_t g_zone_map[WS_ZONE_MAX] = { \
        [WS_ZONE_STRIP] = { \
            .ws    = &g_##prefix##_strip, \
            .first = 0, \
            .count = strip_leds, \
        }, \
        [WS_ZONE_HANDLE] = { \
            .ws    = &g_##prefix##_handle, \
            .first = 0, \
            .count = handle_leds, \
        }, \
        [WS_ZONE_STORAGE] = { \
            .ws    = &g_##prefix##_storage, \
            .first = 0, \
            .count = storage_leds, \
        }, \
        [WS_ZONE_FOOTWELL] = { \
            .ws    = &g_##prefix##_footwell, \
            .first = 0, \
            .count = footwell_leds, \
        }, \
    }

/* ====== ZONE INIT HELPER ================================================ */
/* Initializes a single zone with ws_init() */

#define INIT_ZONE(htim, prefix, zone, channel, leds) \
    ws_init(&g_##prefix##_##zone, \
            htim, \
            channel, \
            prefix##_##zone##_fb, \
            prefix##_##zone##_dmaA, \
            prefix##_##zone##_dmaB, \
            leds)

#ifdef __cplusplus
}
#endif
