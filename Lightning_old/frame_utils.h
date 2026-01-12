/**
 * @file frame_utils.h
 * @brief Low-level frame utilities for WS2812 LED buffers.
 *
 * This module provides helper functions for manipulating LED frame data
 * (GRB buffers) used throughout the lighting system.
 *
 * Typical use cases:
 *   - Clearing or filling entire LED strips or zones
 *   - Copying and mixing frames (e.g., bridge transitions)
 *   - Per-pixel access and simple blending
 *
 * This layer is purely technical — it does NOT know about themes, effects,
 * or animation logic. It is intended to be used by higher-level modules:
 *     - effects.c  → to build temporary patterns
 *     - zones.c    → to colorize specific zones
 *     - scene_player.c → for bridge / intro / outro blending
 *
 * Author: NV & OpenAI
 * Project: STM32 WS2812B Ambient Controller
 */
#pragma once

#include <stdint.h>
#include "types.h"   // или твой LED.h, где объявлен ws2812_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ожидаем, что ws2812_t содержит минимум:
 *   uint16_t led_count;
 *   uint8_t *grb; // 3 * led_count байт
 *
 * Если у тебя поле называется иначе (count / size / num_leds) —
 * просто подправь макрос ниже.
 */
#ifndef WS_GET_LED_COUNT
#define WS_GET_LED_COUNT(ws)  ((ws)->led_count)
#endif

/* Очистить весь буфер ленты (всё в 0). */
void frame_clear(ws2812_t *ws);

/* Заполнить всю ленту одним цветом. */
void frame_fill(ws2812_t *ws,
                uint8_t r, uint8_t g, uint8_t b);

/* Заполнить сегмент [first, first+count) одним цветом. */
void frame_fill_zone(ws2812_t *ws,
                     uint16_t first,
                     uint16_t count,
                     uint8_t r, uint8_t g, uint8_t b);

/* Полное копирование кадров src → dst (по минимальному общему кол-ву диодов). */
void frame_copy(ws2812_t *dst,
                const ws2812_t *src);

/* Копирование сегмента src[first_src..] → dst[first_dst..]. */
void frame_copy_zone(ws2812_t *dst,
                     uint16_t first_dst,
                     const ws2812_t *src,
                     uint16_t first_src,
                     uint16_t count);

/* Смешивание двух кадров a и b в dst:
 * t=0 → dst=a, t=1 → dst=b.
 */
void frame_mix(ws2812_t *dst,
               const ws2812_t *a,
               const ws2812_t *b,
               float t);

/* Смешивание сегмента двух кадров в зоне:
 * t=0 → a, t=1 → b.
 */
void frame_mix_zone(ws2812_t *dst,
                    uint16_t first,
                    uint16_t count,
                    const ws2812_t *a,
                    const ws2812_t *b,
                    float t);

/* Установить один пиксель (с защитой по границам). */
void frame_set_pixel(ws2812_t *ws,
                     uint16_t idx,
                     uint8_t r, uint8_t g, uint8_t b);

/* Прочитать один пиксель (out=0,0,0, если idx вне). */
void frame_get_pixel(const ws2812_t *ws,
                     uint16_t idx,
                     uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif
