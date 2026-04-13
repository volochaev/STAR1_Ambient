#pragma once

#include <stdint.h>

#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ws2812_t led_runtime_strip_t;

/* Power and brightness bridge used by runtime/director layers. */
void led_runtime_power_set(uint8_t on);
uint8_t led_runtime_is_power_on(void);
void led_runtime_set_global_brightness(led_runtime_strip_t *strip, float brightness);
void led_runtime_force_brightness(led_runtime_strip_t *strip, float brightness);
void led_runtime_release_brightness(led_runtime_strip_t *strip);
void led_runtime_dma_tc(led_runtime_strip_t *strip, TIM_HandleTypeDef *htim);

/* Pixel-level bridge to avoid direct rgb-buffer access in orchestration code. */
void led_runtime_set_pixel_rgb(led_runtime_strip_t *strip, uint16_t led_idx, uint8_t r, uint8_t g, uint8_t b);
void led_runtime_get_pixel_rgb(const led_runtime_strip_t *strip,
                               uint16_t led_idx,
                               uint8_t *r,
                               uint8_t *g,
                               uint8_t *b);

/* Contiguous RGB helpers: 3 bytes per LED, RGB order. */
void led_runtime_copy_rgb_from_strip(const led_runtime_strip_t *strip,
                                     uint16_t first_led,
                                     uint16_t led_count,
                                     uint8_t *out_rgb);
void led_runtime_copy_rgb_to_strip(led_runtime_strip_t *strip,
                                   uint16_t first_led,
                                   uint16_t led_count,
                                   const uint8_t *in_rgb);

#ifdef __cplusplus
}
#endif
