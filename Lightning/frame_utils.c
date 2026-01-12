
#include "frame_utils.h"
#include <string.h>

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint32_t px3(uint16_t i) { return (uint32_t)i * 3u; }

void frame_clear(ws2812_t *ws)
{
    if (!ws || !ws->grb) return;
    memset(ws->grb, 0, (size_t)ws->led_count * BYTES_PER_LED);
}

void frame_fill(ws2812_t *ws, uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb) return;
    uint16_t n = ws->led_count;
    for (uint16_t i = 0; i < n; ++i) {
        uint32_t p = px3(i);
        ws->grb[p + 0] = g;
        ws->grb[p + 1] = r;
        ws->grb[p + 2] = b;
    }
}

void frame_fill_zone(ws2812_t *ws, uint16_t first, uint16_t count,
                     uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb || count == 0) return;
    if (first >= ws->led_count) return;
    if (first + count > ws->led_count)
        count = ws->led_count - first;

    uint32_t base = px3(first);
    for (uint16_t i = 0; i < count; ++i) {
        uint32_t p = base + px3(i);
        ws->grb[p + 0] = g;
        ws->grb[p + 1] = r;
        ws->grb[p + 2] = b;
    }
}

void frame_copy(ws2812_t *dst, const ws2812_t *src)
{
    if (!dst || !src || !dst->grb || !src->grb) return;
    uint16_t n = dst->led_count < src->led_count ? dst->led_count : src->led_count;
    memcpy(dst->grb, src->grb, (size_t)n * BYTES_PER_LED);
}

void frame_copy_zone(ws2812_t *dst, uint16_t first_dst,
                     const ws2812_t *src, uint16_t first_src,
                     uint16_t count)
{
    if (!dst || !src || !dst->grb || !src->grb || count == 0) return;

    if (first_dst >= dst->led_count || first_src >= src->led_count) return;

    if (first_dst + count > dst->led_count)
        count = dst->led_count - first_dst;
    if (first_src + count > src->led_count)
        count = src->led_count - first_src;

    uint32_t bd = px3(first_dst);
    uint32_t bs = px3(first_src);
    memmove(&dst->grb[bd], &src->grb[bs], (size_t)count * BYTES_PER_LED);
}

void frame_mix(ws2812_t *dst,
               const ws2812_t *a,
               const ws2812_t *b,
               float t)
{
    if (!dst || !a || !b || !dst->grb || !a->grb || !b->grb) return;

    uint16_t n = dst->led_count;
    if (a->led_count < n) n = a->led_count;
    if (b->led_count < n) n = b->led_count;
    float tt = clamp01f(t);

    for (uint32_t i = 0; i < (uint32_t)n * BYTES_PER_LED; ++i) {
        float av = (float)a->grb[i];
        float bv = (float)b->grb[i];
        dst->grb[i] = (uint8_t)(av + (bv - av) * tt + 0.5f);
    }
}

void frame_mix_zone(ws2812_t *dst,
                    uint16_t first,
                    uint16_t count,
                    const ws2812_t *a,
                    const ws2812_t *b,
                    float t)
{
    if (!dst || !a || !b || !dst->grb || !a->grb || !b->grb || count == 0) return;

    if (first >= dst->led_count || first >= a->led_count || first >= b->led_count)
        return;

    if (first + count > dst->led_count)
        count = dst->led_count - first;
    if (first + count > a->led_count)
        count = a->led_count - first;
    if (first + count > b->led_count)
        count = b->led_count - first;

    float tt = clamp01f(t);
    uint32_t base = px3(first);
    uint32_t n3   = (uint32_t)count * BYTES_PER_LED;

    for (uint32_t i = 0; i < n3; ++i) {
        float av = (float)a->grb[base + i];
        float bv = (float)b->grb[base + i];
        dst->grb[base + i] = (uint8_t)(av + (bv - av) * tt + 0.5f);
    }
}

void frame_set_pixel(ws2812_t *ws, uint16_t idx,
                     uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb) return;
    if (idx >= ws->led_count) return;
    uint32_t p = px3(idx);
    ws->grb[p + 0] = g;
    ws->grb[p + 1] = r;
    ws->grb[p + 2] = b;
}

void frame_get_pixel(const ws2812_t *ws, uint16_t idx,
                     uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r) *r = 0;
    if (g) *g = 0;
    if (b) *b = 0;
    if (!ws || !ws->grb) return;
    if (idx >= ws->led_count) return;
    uint32_t p = px3(idx);
    if (g) *g = ws->grb[p + 0];
    if (r) *r = ws->grb[p + 1];
    if (b) *b = ws->grb[p + 2];
}
