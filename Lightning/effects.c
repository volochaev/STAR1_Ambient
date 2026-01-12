#include "driver.h"
#include "effects.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint32_t idx3(uint16_t i) { return (uint32_t)i * 3u; }

static inline void set_grb(ws2812_t *ws, uint16_t i,
                           uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb) return;
    if (i >= ws->led_count) return;
//    uint32_t p = idx3(i);
//    ws->grb[p + 0] = g;
//    ws->grb[p + 1] = r;
//    ws->grb[p + 2] = b;

    ws_set_pixel_rgb(ws, i, r, g, b);
}

/* === Continuous FX ==================================================== */

static void fx_solid_gradient(ws2812_t *ws,
                              const ws_palette_t *pal,
                              fx_state_t *st)
{
    (void)st;
    if (!ws || !ws->grb || !pal) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;
    for (uint16_t i = 0; i < n; ++i) {
        float u = (n == 1) ? 0.5f : (float)i / (float)(n - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        set_grb(ws, i, r, g, b);
    }
}

static void fx_gradient_flow(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms)
{
    if (!ws || !ws->grb || !pal || !st) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float speed = (st->speed > 0.0f) ? st->speed : 0.03f;
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

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

    float freq = (st->speed > 0.0f) ? st->speed : 0.07f;
    st->t += (float)delta_ms * 0.001f * freq;
    float s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f;
    float br = 0.25f + 0.35f * s;

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
    (void)st;
    if (!ws || !ws->grb || !pal) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    for (uint16_t i = 0; i < n; ++i) {
        float v = (float)i / (float)(n - 1);
        float u = (v < 0.5f)
                  ? (v * 2.0f * 0.5f)
                  : (0.5f + (v - 0.5f) * 2.0f * 0.5f);
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

    float speed = (st->speed > 0.0f) ? st->speed : 0.12f;
    st->t += (float)delta_ms * 0.001f * speed;

    for (uint16_t i = 0; i < n; ++i) {
        float x = (float)i / (float)(n - 1);
        float w1 = 0.5f * (sinf((x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w2 = 0.5f * (sinf((1.0f - x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w = clamp01f(w1 + w2);

        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        float k = 0.2f + 0.8f * w;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, i, r, g, b);
    }
}

static void fx_mb_ocean_flow(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms)
{
    if (!ws || !ws->grb || !pal || !st) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    /* 1) Очень медленная скорость – чтобы шаг между кадрами был крошечный */
    float speed = (st->speed > 0.0f) ? st->speed : 0.008f;  // было 0.01, можно даже 0.005
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

    /* 2) Сглаживание по ленте (между соседними диодами) */
    const float spatial_smooth  = 0.70f;  // 0.7–0.8 даёт мягкий "водяной" эффект

    /* 3) Лёгкое временное сглаживание (между кадрами) */
    const float temporal_smooth = 0.70f;  // 0.7 – новый кадр только на 30% сразу

    uint8_t prev_r = 0;
    uint8_t prev_g = 0;
    uint8_t prev_b = 0;

    for (uint16_t i = 0; i < n; ++i) {
        float u  = (n == 1) ? 0.5f : (float)i / (float)(n - 1);
        float uu = u + offset;
        uu -= floorf(uu);

        uint8_t r_raw, g_raw, b_raw;
        ws_palette_sample_rgb8(pal, uu, &r_raw, &g_raw, &b_raw);

        /* --- пространственное сглаживание --- */
        uint8_t r_sp = r_raw;
        uint8_t g_sp = g_raw;
        uint8_t b_sp = b_raw;

        if (i > 0) {
            float inv = 1.0f - spatial_smooth;
            r_sp = (uint8_t)(prev_r * spatial_smooth + r_raw * inv + 0.5f);
            g_sp = (uint8_t)(prev_g * spatial_smooth + g_raw * inv + 0.5f);
            b_sp = (uint8_t)(prev_b * spatial_smooth + b_raw * inv + 0.5f);
        }

        /* --- временное сглаживание: подмешиваем предыдущий кадр --- */
        uint32_t idx = (uint32_t)i * 3u;
        uint8_t old_g = ws->grb[idx + 0];
        uint8_t old_r = ws->grb[idx + 1];
        uint8_t old_b = ws->grb[idx + 2];

        float inv_t = 1.0f - temporal_smooth;

        uint8_t r = (uint8_t)(old_r * temporal_smooth + r_sp * inv_t + 0.5f);
        uint8_t g = (uint8_t)(old_g * temporal_smooth + g_sp * inv_t + 0.5f);
        uint8_t b = (uint8_t)(old_b * temporal_smooth + b_sp * inv_t + 0.5f);
//
//        ws->grb[idx + 0] = g;
//        ws->grb[idx + 1] = r;
//        ws->grb[idx + 2] = b;
        ws_set_pixel_rgb(ws, i, r, g, b);
        prev_r = r_sp;
        prev_g = g_sp;
        prev_b = b_sp;
    }
}

static void fx_mb_two_tone_wave(ws2812_t *ws,
                                const ws_palette_t *pal,
                                fx_state_t *st,
                                uint32_t delta_ms)
{
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

    float freq = (st->speed > 0.0f) ? st->speed : 0.1f;
    st->t += (float)delta_ms * 0.001f * freq;
    float s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f;

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

/* === One-shot FX ====================================================== */

static void fx_welcome_sweep(ws2812_t *ws,
                             const ws_palette_t *pal,
                             float t_norm)
{
    if (!ws || !ws->grb || !pal) return;
    uint16_t n = ws->led_count;
    if (n == 0) return;

    float head  = t_norm * (float)n;
    float width = 0.25f * (float)n;

    for (uint16_t i = 0; i < n; ++i) {
        float pos = (float)i;
        float d   = head - pos;
        float w   = clamp01f(1.0f - fabsf(d) / width);
        float u   = (n == 1) ? 0.5f : (float)i / (float)(n - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        float k = 0.15f + 0.85f * w;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, i, r, g, b);
    }
}

void fx_welcome(ws2812_t *ws,
                   const ws_palette_t *pal,
                   float t_norm)
{
    if (!ws || !pal) return;

    uint16_t n = ws->led_count;
    if (n == 0) return;

    float t = t_norm * 0.7f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* --- 1. Глобальный fade-in от 0 до 1, с плавной кривой --- */
    float fade = t * t * (3.0f - 2.0f * t);  // smoothstep(0..1)

    /* --- 2. Фаза волн (0.15..0.85) --- */
    float wave_phase = 0.0f;
    if (t > 0.15f && t < 0.85f) {
        wave_phase = (t - 0.15f) / 0.70f;   // 0..1
    }

    /* --- 3. Лёгкий pulse в конце (0.8..1.0) --- */
    float pulse = 1.0f;
    if (t > 0.80f) {
        float tp = (t - 0.80f) / 0.20f;
        pulse = 1.0f + 0.10f * sinf(tp * 3.14159f);
    }

    /* Центр с небольшим смещением, как у MB */
    float center = (float)n * 0.47f;
    float half   = (float)((n > 1) ? (n - 1) : 1);

    for (uint16_t i = 0; i < n; ++i)
    {
        uint8_t r0, g0, b0;
        ws_palette_sample_rgb8(pal, 0.5f, &r0, &g0, &b0);

        float fi = (float)i;
        float x  = (fi - center) / half;     // ~ -1..1
        if (x < -1.0f) x = -1.0f;
        if (x >  1.0f) x =  1.0f;

        float dist = fabsf(x);               // 0 в центре, 1 по краям

        /* --- Базовая яркость: центр ярче, края чуть темнее --- */
        float base = fade * (1.0f - 0.4f * dist);  // 0..1, но без нуля на краях
        if (base < 0.0f) base = 0.0f;

        float br = base;

        /* --- Две волны: добавляем НЕ “плюсом 0.8”, а небольшим усилением --- */
        if (wave_phase > 0.0f) {
            float c1 = -1.0f + 2.0f * wave_phase;  // -1 → +1
            float c2 =  1.0f - 2.0f * wave_phase;  // +1 → -1

            float d1 = x - c1;
            float d2 = x - c2;

            float w1 = expf(-d1 * d1 * 4.0f);
            float w2 = expf(-d2 * d2 * 4.0f);

            float w  = (w1 + w2);              // 0..~2

            /* Максимальное усиление ~ +30% от base */
            float wave_gain = 1.0f + 0.3f * w; // в центре волны ~1.3
            br *= wave_gain;
        }

        /* --- Pulse в конце --- */
        br *= pulse;

        if (br > 1.0f) br = 1.0f;

        ws_set_pixel_rgb(ws, i,
            (uint8_t)(r0 * br),
            (uint8_t)(g0 * br),
            (uint8_t)(b0 * br));
    }
}

void fx_goodbye(ws2812_t *ws,
                   const ws_palette_t *pal,
                   float t_norm)
{
    if (!ws || !pal) return;

    uint16_t n = ws->led_count;
    if (n == 0) return;

    float t = t_norm;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* MB S-Class: сверху вниз / от краёв к центру гаснет,
       плюс общее плавное затухание. */

    /* Общий fade (глобальное затемнение) */
    float global = 1.0f - t;
    if (global < 0.0f) global = 0.0f;

    /* "Занавес" с двух краёв к центру */
    float curtain_pos = t;   // 0..1
    float half   = (float)((n > 1) ? (n - 1) : 1);
    float center = half * 0.5f;

    for (uint16_t i = 0; i < n; ++i) {
        float fi = (float)i;

        /* Нормированная позиция [-1..1] */
        float x = (fi - center) / center;
        if (x < -1.0f) x = -1.0f;
        if (x >  1.0f) x =  1.0f;
        float dist_edge = fabsf(x);  // 0 в центре, 1 по краям

        /* "Шторка": края гаснут раньше центра */
        float k_curtain = 1.0f - (dist_edge * 1.2f * curtain_pos);
        if (k_curtain < 0.0f) k_curtain = 0.0f;

        float br = global * k_curtain;

        if (br <= 0.0f) {
        	ws_set_pixel_rgb(ws, i, 0, 0, 0);
            continue;
        }

        /* Берём исходный цвет из палитры по центру */
        uint8_t r0, g0, b0;
        ws_palette_sample_rgb8(pal, 0.5f, &r0, &g0, &b0);

        ws_set_pixel_rgb(ws, i,
                    (uint8_t)(r0 * br),
                    (uint8_t)(g0 * br),
                    (uint8_t)(b0 * br));
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

/* === Public API ======================================================= */

void ws_oneshot_start(ws2812_t        *ws,
                      oneshot_t       *os,
                      fx_id_t          fx_id,
                      const ws_palette_t *pal,
                      float            base_br,
                      uint32_t         duration_ms)
{
    (void)ws;
    if (!os || !pal || duration_ms == 0) return;

    os->active      = 1u;
    os->fx_id       = fx_id;
    os->pal         = pal;
    os->base_br     = clamp01f(base_br);
    os->start_ms    = HAL_GetTick();
    os->duration_ms = duration_ms;
}

uint8_t ws_oneshot_tick(ws2812_t *ws, oneshot_t *os)
{
    if (!ws || !os || !os->active || !os->pal)
        return 1u;

    uint32_t now = HAL_GetTick();
    uint32_t dt  = now - os->start_ms;
    float t = (os->duration_ms > 0)
              ? (float)dt / (float)os->duration_ms
              : 1.0f;
    if (t >= 1.0f) t = 1.0f;

    switch (os->fx_id) {
    case FX_WELCOME_SWEEP:
    case FX_MB_WELCOME:
        fx_welcome_sweep(ws, os->pal, t);
        break;
    case FX_GOODBYE_FADE:
    case FX_MB_GOODBYE:
        fx_goodbye_fade(ws, os->pal, os->base_br, t);
        break;
    case FX_WELCOME:
        fx_welcome(ws, os->pal, t);
        break;
    case FX_GOODBYE:
        fx_goodbye(ws, os->pal, t);
        break;
    default:
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

    switch (fx_id) {
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
        fx_solid_gradient(ws, pal, st);
        break;
    }
}
