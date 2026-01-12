#include "effects.h"
#include <math.h>
#include <string.h>

/* Вспомогательные */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint32_t idx3(uint16_t i)
{
    return (uint32_t)i * 3u;
}

/* Установка пикселя в GRB-буфер */
static inline void set_grb(ws2812_t *ws,
                           uint16_t i,
                           uint8_t r,
                           uint8_t g,
                           uint8_t b)
{
    if (!ws || !ws->grb) return;
    if (i >= ws->led_count) return;

    uint32_t p = idx3(i);
    ws->grb[p + 0] = g;
    ws->grb[p + 1] = r;
    ws->grb[p + 2] = b;
}

/* ==== Непрерывные эффекты ============================================ */

static void fx_solid_gradient(ws2812_t *ws,
                              const ws_palette_t *pal,
                              fx_state_t *st)
{
    if (!ws || !ws->grb || !pal) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    for (uint16_t i = 0; i < n; ++i) {
        float u = (n == 1) ? 0.5f : (float)i / (float)(n - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        set_grb(ws, i, r, g, b);
    }

    (void)st;
}

static void fx_gradient_flow(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms)
{
    if (!ws || !ws->grb || !pal || !st) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float speed = (st->speed > 0.0f) ? st->speed : 0.03f; // циклов палитры в сек
    st->t += (float)delta_ms * 0.001f * speed;

    float offset = st->t - floorf(st->t); // 0..1

    for (uint16_t i = 0; i < n; ++i) {
        float u = (float)i / (float)(n - 1);
        float uu = u + offset;
        uu -= floorf(uu);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, uu, &r, &g, &b);
        set_grb(ws, i, r, g, b);
    }
}

static void fx_soft_breathe(ws2812_t *ws,
                            const ws_palette_t *pal,
                            fx_state_t *st,
                            uint32_t delta_ms)
{
    if (!ws || !ws->grb || !pal || !st) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float freq = (st->speed > 0.0f) ? st->speed : 0.2f; // Гц
    st->t += (float)delta_ms * 0.001f * freq;
    float s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f; // 0..1
    float br = 0.25f + 0.35f * s; // мягкий диапазон

    for (uint16_t i = 0; i < n; ++i) {
        float u = (n == 1) ? 0.5f : (float)i / (float)(n - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);
        set_grb(ws, i, r, g, b);
    }
}

static void fx_dual_zone(ws2812_t *ws,
                         const ws_palette_t *pal,
                         fx_state_t *st)
{
    if (!ws || !ws->grb || !pal || !st) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    // Левая половина — u в нижней части палитры, правая — верхняя
    for (uint16_t i = 0; i < n; ++i) {
        float v = (float)i / (float)(n - 1);
        float u = (v < 0.5f)
                  ? (v * 2.0f * 0.5f)                  // 0..0.5
                  : (0.5f + (v - 0.5f) * 2.0f * 0.5f); // 0.5..1
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, clamp01f(u), &r, &g, &b);
        set_grb(ws, i, r, g, b);
    }
}

static void fx_twin_wave(ws2812_t *ws,
                         const ws_palette_t *pal,
                         fx_state_t *st,
                         uint32_t delta_ms)
{
    if (!ws || !ws->grb || !pal || !st) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float speed = (st->speed > 0.0f) ? st->speed : 0.8f;
    st->t += (float)delta_ms * 0.001f * speed;

    for (uint16_t i = 0; i < n; ++i) {
        float x = (float)i / (float)(n - 1);
        float w1 = 0.5f * (sinf((x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w2 = 0.5f * (sinf((1.0f - x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w = clamp01f(w1 + w2); // две волны

        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        r = (uint8_t)(r * (0.2f + 0.8f*w));
        g = (uint8_t)(g * (0.2f + 0.8f*w));
        b = (uint8_t)(b * (0.2f + 0.8f*w));
        set_grb(ws, i, r, g, b);
    }
}

static void fx_mb_ocean_flow(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms)
{
    if (!ws || !ws->grb || !pal || !st) return;

    // По сути более медленный/глубокий GRADIENT_FLOW
    if (st->speed <= 0.0f) st->speed = 0.01f;
    fx_gradient_flow(ws, pal, st, delta_ms);
}

static void fx_mb_two_tone_wave(ws2812_t *ws,
                                const ws_palette_t *pal,
                                fx_state_t *st,
                                uint32_t delta_ms)
{
    // Можно реализовать как комбинацию twin_wave + dual_zone,
    // для краткости оставим переиспользование twin_wave.
    fx_twin_wave(ws, pal, st, delta_ms);
}

static void fx_mb_energize_pulse(ws2812_t *ws,
                                 const ws_palette_t *pal,
                                 fx_state_t *st,
                                 uint32_t delta_ms)
{
    if (!ws || !ws->grb || !pal || !st) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float freq = (st->speed > 0.0f) ? st->speed : 1.2f;
    st->t += (float)delta_ms * 0.001f * freq;
    float s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f; // 0..1

    for (uint16_t i = 0; i < n; ++i) {
        float u = (float)i / (float)(n - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        float k = 0.2f + 0.8f * s;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, i, r, g, b);
    }
}

/* ==== One-shot эффекты =============================================== */

static void fx_welcome_sweep(ws2812_t *ws,
                             const ws_palette_t *pal,
                             float t_norm)
{
    if (!ws || !ws->grb || !pal) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float head = t_norm * (float)n;
    float width = 0.25f * (float)n; // ширина "фронта"

    for (uint16_t i = 0; i < n; ++i) {
        float pos = (float)i;
        float d = head - pos;
        float w = clamp01f(1.0f - fabsf(d) / width); // 1 в центре фронта → 0 по краям

        float u = (n == 1) ? 0.5f : (float)i / (float)(n - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);

        float k = 0.15f + 0.85f * w;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, i, r, g, b);
    }
}

static void fx_goodbye_fade(ws2812_t *ws,
                            const ws_palette_t *pal,
                            float base_br,
                            float t_norm)
{
    if (!ws || !ws->grb || !pal) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float k = clamp01f(1.0f - t_norm);
    float br = base_br * k;

    for (uint16_t i = 0; i < n; ++i) {
        float u = (n == 1) ? 0.5f : (float)i / (float)(n - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);
        set_grb(ws, i, r, g, b);
    }
}

/* ==== Реализация публичного API ====================================== */

void ws_oneshot_start(ws2812_t        *ws,
                      ws_oneshot_t    *os,
                      fx_id_t          fx_id,
                      const ws_palette_t *pal,
                      float            base_br,
                      uint32_t         duration_ms)
{
    (void)ws;
    if (!os || !pal || duration_ms == 0) return;

    os->active     = 1u;
    os->fx_id      = fx_id;
    os->pal        = pal;
    os->base_br    = clamp01f(base_br);
    os->start_ms   = HAL_GetTick();
    os->duration_ms= duration_ms;
}

uint8_t ws_oneshot_tick(ws2812_t *ws,
                        ws_oneshot_t *os)
{
    if (!ws || !os || !os->active || !os->pal)
        return 1u;

    uint32_t now = HAL_GetTick();
    uint32_t dt  = now - os->start_ms;
    float t = (os->duration_ms > 0)
              ? (float)dt / (float)os->duration_ms
              : 1.0f;
    if (t >= 1.0f) t = 1.0f;

    switch (os->fx_id)
    {
    case FX_WELCOME_SWEEP:
    case FX_MB_WELCOME:
        fx_welcome_sweep(ws, os->pal, t);
        break;

    case FX_GOODBYE_FADE:
    case FX_MB_GOODBYE:
        fx_goodbye_fade(ws, os->pal, os->base_br, t);
        break;

    default:
        // незнакомый oneshot — считаем завершённым
        os->active = 0u;
        return 1u;
    }

    if (t >= 1.0f) {
        os->active = 0u;
        return 1u;
    }

    return 0u;
}

void ws_fx_apply(ws2812_t           *ws,
                 fx_id_t             fx_id,
                 const ws_palette_t *pal,
                 fx_state_t         *st,
                 uint32_t            delta_ms)
{
    if (!ws || !pal) return;

    switch (fx_id)
    {
    case FX_SOLID_GRADIENT:
        fx_solid_gradient(ws, pal, st);
        break;

    case FX_GRADIENT_FLOW:
        fx_gradient_flow(ws, pal, st, delta_ms);
        break;

    case FX_SOFT_BREATHE:
        fx_soft_breathe(ws, pal, st, delta_ms);
        break;

    case FX_DUAL_ZONE:
        fx_dual_zone(ws, pal, st);
        break;

    case FX_TWIN_WAVE:
        fx_twin_wave(ws, pal, st, delta_ms);
        break;

    case FX_MB_OCEAN_FLOW:
        fx_mb_ocean_flow(ws, pal, st, delta_ms);
        break;

    case FX_MB_TWO_TONE_WAVE:
        fx_mb_two_tone_wave(ws, pal, st, delta_ms);
        break;

    case FX_MB_ENERGIZE_PULSE:
        fx_mb_energize_pulse(ws, pal, st, delta_ms);
        break;

    default:
        // для one-shot FX сюда попадать не должны;
        // если всё же попали, рендерим статичный градиент.
        fx_solid_gradient(ws, pal, st);
        break;
    }
}
