/**
 * @file frame_utils.c
 * @brief Implementation of basic WS2812 frame operations.
 *
 * Contains utility functions for direct manipulation of GRB LED buffers.
 * Includes routines for clearing, filling, copying, mixing, and
 * accessing individual pixels or zones.
 *
 * These utilities are the foundation of all higher-level lighting logic.
 * They operate on raw ws2812_t buffers and make no assumptions
 * about scenes, zones, or playback states.
 *
 * See also: frame_utils.h
 */

#include <frame_utils.h>
#include <string.h>

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint32_t px3(uint16_t i)
{
    return (uint32_t)i * 3u;
}

void frame_clear(ws2812_t *ws)
{
    if (!ws || !ws->grb) return;
    uint16_t n = WS_GET_LED_COUNT(ws);
    memset(ws->grb, 0, (size_t)n * 3u);
}

void frame_fill(ws2812_t *ws,
                uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb) return;
    uint16_t n = WS_GET_LED_COUNT(ws);
    uint8_t *p = ws->grb;
    for (uint16_t i = 0; i < n; ++i) {
        uint32_t idx = px3(i);
        p[idx + 0] = g;
        p[idx + 1] = r;
        p[idx + 2] = b;
    }
}

void frame_fill_zone(ws2812_t *ws,
                     uint16_t first,
                     uint16_t count,
                     uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb || count == 0) return;
    uint16_t n = WS_GET_LED_COUNT(ws);
    if (first >= n) return;
    if (first + count > n) count = (uint16_t)(n - first);

    uint8_t *p = ws->grb;
    uint32_t base = px3(first);
    for (uint16_t i = 0; i < count; ++i) {
        uint32_t idx = base + px3(i);
        p[idx + 0] = g;
        p[idx + 1] = r;
        p[idx + 2] = b;
    }
}

void frame_copy(ws2812_t *dst,
                const ws2812_t *src)
{
    if (!dst || !src || !dst->grb || !src->grb) return;
    uint16_t nd = WS_GET_LED_COUNT(dst);
    uint16_t ns = WS_GET_LED_COUNT(src);
    uint16_t n  = (nd < ns) ? nd : ns;
    if (n == 0) return;

    memcpy(dst->grb, src->grb, (size_t)n * 3u);
}

void frame_copy_zone(ws2812_t *dst,
                     uint16_t first_dst,
                     const ws2812_t *src,
                     uint16_t first_src,
                     uint16_t count)
{
    if (!dst || !src || !dst->grb || !src->grb || count == 0) return;

    uint16_t nd = WS_GET_LED_COUNT(dst);
    uint16_t ns = WS_GET_LED_COUNT(src);
    if (first_dst >= nd || first_src >= ns) return;

    if (first_src + count > ns) count = (uint16_t)(ns - first_src);
    if (first_dst + count > nd) count = (uint16_t)(nd - first_dst);
    if (count == 0) return;

    uint8_t *d = dst->grb;
    const uint8_t *s = src->grb;

    uint32_t bd = px3(first_dst);
    uint32_t bs = px3(first_src);

    memmove(&d[bd], &s[bs], (size_t)count * 3u);
}

void frame_mix(ws2812_t *dst,
               const ws2812_t *a,
               const ws2812_t *b,
               float t)
{
    if (!dst || !a || !b || !dst->grb || !a->grb || !b->grb) return;

    uint16_t nd = WS_GET_LED_COUNT(dst);
    uint16_t na = WS_GET_LED_COUNT(a);
    uint16_t nb = WS_GET_LED_COUNT(b);
    uint16_t n  = nd;
    if (na < n) n = na;
    if (nb < n) n = nb;
    if (n == 0) return;

    float tt = clamp01f(t);
    uint8_t *dstp = dst->grb;
    const uint8_t *ap = a->grb;
    const uint8_t *bp = b->grb;

    for (uint16_t i = 0; i < n * 3u; ++i) {
        float av = (float)ap[i];
        float bv = (float)bp[i];
        dstp[i] = (uint8_t)(av + (bv - av) * tt + 0.5f);
    }
}

void frame_mix_zone(ws2812_t *dst,
                    uint16_t first,
                    uint16_t count,
                    const ws2812_t *a,
                    const ws2812_t *b,
                    float t)
{
    if (!dst || !a || !b || !dst->grb || !a->grb || !b->grb || count == 0)
        return;

    uint16_t nd = WS_GET_LED_COUNT(dst);
    uint16_t na = WS_GET_LED_COUNT(a);
    uint16_t nb = WS_GET_LED_COUNT(b);
    if (first >= nd || first >= na || first >= nb) return;

    if (first + count > nd) count = (uint16_t)(nd - first);
    if (first + count > na) count = (uint16_t)(na - first);
    if (first + count > nb) count = (uint16_t)(nb - first);
    if (count == 0) return;

    float tt = clamp01f(t);
    uint8_t *dp = dst->grb;
    const uint8_t *ap = a->grb;
    const uint8_t *bp = b->grb;

    uint32_t base = px3(first);
    uint32_t n3 = (uint32_t)count * 3u;

    for (uint32_t i = 0; i < n3; ++i) {
        float av = (float)ap[base + i];
        float bv = (float)bp[base + i];
        dp[base + i] = (uint8_t)(av + (bv - av) * tt + 0.5f);
    }
}

void frame_set_pixel(ws2812_t *ws,
                     uint16_t idx,
                     uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb) return;
    uint16_t n = WS_GET_LED_COUNT(ws);
    if (idx >= n) return;

    uint32_t p = px3(idx);
    ws->grb[p + 0] = g;
    ws->grb[p + 1] = r;
    ws->grb[p + 2] = b;
}

void frame_get_pixel(const ws2812_t *ws,
                     uint16_t idx,
                     uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r) *r = 0;
    if (g) *g = 0;
    if (b) *b = 0;

    if (!ws || !ws->grb) return;
    uint16_t n = WS_GET_LED_COUNT(ws);
    if (idx >= n) return;

    uint32_t p = px3(idx);
    if (g) *g = ws->grb[p + 0];
    if (r) *r = ws->grb[p + 1];
    if (b) *b = ws->grb[p + 2];
}
