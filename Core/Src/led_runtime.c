#include "led_runtime.h"
#include <string.h>

void led_runtime_power_set(uint8_t on)
{
    ws_power_set(on);
}

uint8_t led_runtime_is_power_on(void)
{
    return ws_is_power_on();
}

void led_runtime_set_global_brightness(led_runtime_strip_t *strip, float brightness)
{
    ws_set_global_brightness(strip, brightness);
}

void led_runtime_force_brightness(led_runtime_strip_t *strip, float brightness)
{
    ws_force_brightness(strip, brightness);
}

void led_runtime_release_brightness(led_runtime_strip_t *strip)
{
    ws_release_brightness(strip);
}

void led_runtime_dma_tc(led_runtime_strip_t *strip, TIM_HandleTypeDef *htim)
{
    ws_dma_tc_isr(strip, htim);
}

void led_runtime_set_pixel_rgb(led_runtime_strip_t *strip, uint16_t led_idx, uint8_t r, uint8_t g, uint8_t b)
{
    if (!strip) return;
    ws_set_pixel_rgb(strip, led_idx, r, g, b);
}

void led_runtime_get_pixel_rgb(const led_runtime_strip_t *strip,
                               uint16_t led_idx,
                               uint8_t *r,
                               uint8_t *g,
                               uint8_t *b)
{
    uint32_t idx;
    if (!strip || !r || !g || !b) return;
    /* Driver buffer is tightly packed RGBRGB... */
    idx = (uint32_t)led_idx * 3u;
    *r = strip->rgb[idx + 0u];
    *g = strip->rgb[idx + 1u];
    *b = strip->rgb[idx + 2u];
}

void led_runtime_copy_rgb_from_strip(const led_runtime_strip_t *strip,
                                     uint16_t first_led,
                                     uint16_t led_count,
                                     uint8_t *out_rgb)
{
    uint32_t bytes;
    uint32_t offset;
    if (!strip || !out_rgb || led_count == 0u) return;
    bytes = (uint32_t)led_count * 3u;
    offset = (uint32_t)first_led * 3u;
    memcpy(out_rgb, &strip->rgb[offset], bytes);
}

void led_runtime_copy_rgb_to_strip(led_runtime_strip_t *strip,
                                   uint16_t first_led,
                                   uint16_t led_count,
                                   const uint8_t *in_rgb)
{
    uint32_t bytes;
    uint32_t offset;
    if (!strip || !in_rgb || led_count == 0u) return;
    bytes = (uint32_t)led_count * 3u;
    offset = (uint32_t)first_led * 3u;
    memcpy(&strip->rgb[offset], in_rgb, bytes);
}
