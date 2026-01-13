/**
 ******************************************************************************
 * @file    frame_utils.h
 * @brief   Frame manipulation utilities for WS2812 framebuffers
 * @details Provides helper functions for working with LED framebuffers:
 *          clearing, filling, copying, mixing, and pixel manipulation.
 *
 * @section Frame Operations
 * - Clearing and filling frames/zones
 * - Copying frames and zones between strips
 * - Mixing two frames with alpha blending
 * - Pixel-level get/set operations
 *
 * @note All operations work on RGB format framebuffers
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include <stdint.h>
#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif

void frame_clear(ws2812_t *ws);
void frame_fill(ws2812_t *ws, uint8_t r, uint8_t g, uint8_t b);
void frame_fill_zone(ws2812_t *ws, uint16_t first, uint16_t count,
                     uint8_t r, uint8_t g, uint8_t b);
void frame_copy(ws2812_t *dst, const ws2812_t *src);
void frame_copy_zone(ws2812_t *dst, uint16_t first_dst,
                     const ws2812_t *src, uint16_t first_src,
                     uint16_t count);
void frame_mix(ws2812_t *dst,
               const ws2812_t *a,
               const ws2812_t *b,
               float t);
void frame_mix_zone(ws2812_t *dst,
                    uint16_t first,
                    uint16_t count,
                    const ws2812_t *a,
                    const ws2812_t *b,
                    float t);
void frame_set_pixel(ws2812_t *ws, uint16_t idx,
                     uint8_t r, uint8_t g, uint8_t b);
void frame_get_pixel(const ws2812_t *ws, uint16_t idx,
                     uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif
