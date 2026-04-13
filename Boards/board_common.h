/**
 ******************************************************************************
 * @file    board_common.h
 * @brief   Shared constants/macros for board buffer declarations
 * @details Keeps only low-level DMA alignment and WS DMA length helpers
 *          used by board implementations.
 ******************************************************************************
 */

#pragma once

#include "driver.h"

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

#ifdef __cplusplus
}
#endif
