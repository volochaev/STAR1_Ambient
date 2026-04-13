#include "driver.h"
#include "effects.h"
#include "features.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Effect parameters are now in features.h for easy tuning:
 * FX_WELCOME_TIME_SCALE, FX_WELCOME_WAVE_START, FX_WELCOME_WAVE_DURATION,
 * FX_WELCOME_WAVE_GAIN_MAX, FX_WELCOME_PULSE_START, FX_WELCOME_PULSE_DURATION,
 * FX_WELCOME_PULSE_AMPLITUDE, FX_WELCOME_CENTER_OFFSET, FX_WELCOME_DIST_SCALE,
 * FX_WELCOME_SETTLE_START, FX_GOODBYE_CURTAIN_SCALE
 */

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float fractf(float x)
{
    return x - floorf(x);
}

/* Коэффициент позиции с укрупнением шага: несколько LED = один виртуальный пиксель */
static float fx_pos_norm(uint16_t i, uint16_t count)
{
    if (count <= 1u) return 0.5f;
#if FX_SPATIAL_GROUP > 1
    uint16_t group = i / FX_SPATIAL_GROUP;
    uint16_t groups = (count + FX_SPATIAL_GROUP - 1u) / FX_SPATIAL_GROUP;
    return (groups <= 1u) ? 0.5f : (float)group / (float)(groups - 1u);
#else
    return (float)i / (float)(count - 1u);
#endif
}

/* =========================================================================
 * EASING FUNCTIONS для премиальной плавности W223/EQS
 * ========================================================================= */

/* Smoothstep: плавный переход 0->1 с нулевыми производными на концах */
static float smoothstep(float t)
{
    t = clamp01f(t);
    return t * t * (3.0f - 2.0f * t);
}

/* Ease-in-out cubic: ещё более плавный переход */
static float ease_in_out_cubic(float t)
{
    t = clamp01f(t);
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = 2.0f * t - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }
}

/* Ease-out quad: быстрый старт, плавное замедление */
static float ease_out_quad(float t)
{
    t = clamp01f(t);
    return t * (2.0f - t);
}

/* Soft-knee limiter in 0..1 range. Keeps low/mid untouched, compresses near peak. */
static float soft_clip01(float x, float knee)
{
    float y;
    x = clamp01f(x);
    if (knee < 0.0f) knee = 0.0f;
    if (knee > 0.98f) knee = 0.98f;
    if (x <= knee) return x;
    y = (x - knee) / (1.0f - knee);
    y = smoothstep(y);
    return knee + (1.0f - knee) * y * 0.72f;
}

/* Deterministic hash (0..1) for stable per-LED micro-variation. */
static float hash01_u32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return (float)(x & 0x00FFFFFFu) / 16777215.0f;
}

static inline uint32_t idx3(uint16_t i) { return (uint32_t)i * 3u; }

static inline void set_rgb(ws2812_t *ws, uint16_t i,
                           uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->rgb) return;
    if (i >= ws->led_count) return;

    ws_set_pixel_rgb(ws, i, r, g, b);
}

static inline void set_rgb_temporal(ws2812_t *ws,
                                    uint16_t i,
                                    uint8_t r_new,
                                    uint8_t g_new,
                                    uint8_t b_new,
                                    float temporal_smooth,
                                    uint8_t first_frame)
{
    uint32_t idx;
    float inv_t;
    uint8_t r_old, g_old, b_old;

    if (!ws || !ws->rgb) return;
    if (i >= ws->led_count) return;
    if (first_frame || temporal_smooth <= 0.0f) {
        ws_set_pixel_rgb(ws, i, r_new, g_new, b_new);
        return;
    }

    if (temporal_smooth > 0.98f) temporal_smooth = 0.98f;
    inv_t = 1.0f - temporal_smooth;
    idx = idx3(i);
    r_old = ws->rgb[idx + 0];
    g_old = ws->rgb[idx + 1];
    b_old = ws->rgb[idx + 2];

    ws_set_pixel_rgb(ws, i,
                     (uint8_t)(r_old * temporal_smooth + r_new * inv_t + 0.5f),
                     (uint8_t)(g_old * temporal_smooth + g_new * inv_t + 0.5f),
                     (uint8_t)(b_old * temporal_smooth + b_new * inv_t + 0.5f));
}

/* === Continuous FX ==================================================== */

static void fx_solid_gradient(ws2812_t *ws,
                              const ws_palette_t *pal,
                              fx_state_t *st,
                              uint16_t first,
                              uint16_t count)
{
    (void)st;
    if (!ws || !ws->rgb || !pal || count == 0) return;
    for (uint16_t i = 0; i < count; ++i) {
        float u = fx_pos_norm(i, count);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        set_rgb(ws, first + i, r, g, b);
    }
}

static void fx_gradient_flow(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms,
                             uint16_t first,
                             uint16_t count)
{
    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    float speed = (st->speed > 0.0f) ? st->speed : 0.024f;
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

    for (uint16_t i = 0; i < count; ++i) {
        float u = fx_pos_norm(i, count);
        float uu = u + offset;
        uu -= floorf(uu);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, uu, &r, &g, &b);
        set_rgb(ws, first + i, r, g, b);
    }
}

static void fx_soft_breathe(ws2812_t *ws,
                            const ws_palette_t *pal,
                            fx_state_t *st,
                            uint32_t delta_ms,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    float freq = (st->speed > 0.0f) ? st->speed : 0.05f;
    st->t += (float)delta_ms * 0.001f * freq;
    float s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f;
    float br = 0.30f + 0.30f * s;

    for (uint16_t i = 0; i < count; ++i) {
        float u = fx_pos_norm(i, count);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);
        set_rgb(ws, first + i, r, g, b);
    }
}

static void fx_dual_zone(ws2812_t *ws,
                         const ws_palette_t *pal,
                         fx_state_t *st,
                         uint16_t first,
                         uint16_t count)
{
    (void)st;
    if (!ws || !ws->rgb || !pal || count == 0) return;

    for (uint16_t i = 0; i < count; ++i) {
        float v = fx_pos_norm(i, count);
        /* Создаём два градиента: от начала до середины и от середины до конца */
        float u = (v < 0.5f)
                  ? (v * 2.0f)           /* первая половина: 0..0.5 -> 0..1 */
                  : ((v - 0.5f) * 2.0f); /* вторая половина: 0.5..1 -> 0..1 */
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, clamp01f(u), &r, &g, &b);
        set_rgb(ws, first + i, r, g, b);
    }
}

static void fx_twin_wave(ws2812_t *ws,
                         const ws_palette_t *pal,
                         fx_state_t *st,
                         uint32_t delta_ms,
                         uint16_t first,
                         uint16_t count)
{
    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    float speed = (st->speed > 0.0f) ? st->speed : 0.12f;
    st->t += (float)delta_ms * 0.001f * speed;

    for (uint16_t i = 0; i < count; ++i) {
        float x = fx_pos_norm(i, count);
        float w1 = 0.5f * (sinf((x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w2 = 0.5f * (sinf((1.0f - x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w = clamp01f(w1 + w2);

        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        float k = 0.2f + 0.8f * w;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_rgb(ws, first + i, r, g, b);
    }
}

static void fx_ocean_flow(ws2812_t *ws,
                          const ws_palette_t *pal,
                          fx_state_t *st,
                          uint32_t delta_ms,
                          uint16_t first,
                          uint16_t count)
{
    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* 1) Очень медленная скорость – чтобы шаг между кадрами был крошечный */
    float speed = (st->speed > 0.0f) ? st->speed : 0.006f;  // мягче движение
    float t_prev = st->t;  /* нужен, чтобы на первом кадре не мешать интро-цвет */
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

    /* 2) Сглаживание по ленте (между соседними диодами) */
    const float spatial_smooth  = 0.85f;  // сильнее размазываем соседей

    /* 3) Лёгкое временное сглаживание (между кадрами) */
    /* На первом кадре после смены темы/интро не подмешиваем старый кадр,
     * иначе в новых палитрах видно «мигание» от старых цветов. */
    uint8_t first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                          (st->a == 0.0f)  && (st->b == 0.0f);
    const float temporal_smooth = first_frame ? 0.0f : 0.92f;  // LPF для плавных шагов

    uint8_t prev_r = 0;
    uint8_t prev_g = 0;
    uint8_t prev_b = 0;

    for (uint16_t i = 0; i < count; ++i) {
        float u  = fx_pos_norm(i, count);
        float uu = u + offset * 0.55f;  // уменьшаем силу смещения, шаги мягче
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
        /* Читаем предыдущий кадр в RGB формате (ws_set_pixel_rgb использует RGB) */
        uint32_t idx = (uint32_t)(first + i) * 3u;
        uint8_t old_r = first_frame ? 0 : ws->rgb[idx + 0];
        uint8_t old_g = first_frame ? 0 : ws->rgb[idx + 1];
        uint8_t old_b = first_frame ? 0 : ws->rgb[idx + 2];

        float inv_t = 1.0f - temporal_smooth;

        uint8_t r = (uint8_t)(old_r * temporal_smooth + r_sp * inv_t + 0.5f);
        uint8_t g = (uint8_t)(old_g * temporal_smooth + g_sp * inv_t + 0.5f);
        uint8_t b = (uint8_t)(old_b * temporal_smooth + b_sp * inv_t + 0.5f);
        
        ws_set_pixel_rgb(ws, first + i, r, g, b);
        prev_r = r_sp;
        prev_g = g_sp;
        prev_b = b_sp;
    }
}

static void fx_two_tone_wave(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms,
                             uint16_t first,
                             uint16_t count)
{
    float t_prev;
    uint8_t first_frame;
    float temporal_smooth;

    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Плавная двухтонная волна с лёгким сдвигом фаз — ближе к EQS */
    float speed = (st->speed > 0.0f) ? st->speed : 0.028f; /* замедлили для более плавного слайда */
    t_prev = st->t;
    st->t += (float)delta_ms * 0.001f * speed;
    first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                  (st->a == 0.0f)  && (st->b == 0.0f);
    temporal_smooth = first_frame ? 0.0f : 0.76f;
    float phase_a = st->t * 2.0f * (float)M_PI;
    float phase_b = phase_a + (float)M_PI * 0.35f; /* небольшой сдвиг для richer look */

    for (uint16_t i = 0; i < count; ++i) {
        float pos = fx_pos_norm(i, count);

        /* Две выборки палитры с осциллирующим смещением */
        float u1 = pos + 0.07f * sinf(phase_a + pos * (float)M_PI);
        float u2 = pos + 0.07f * sinf(phase_b + pos * (float)M_PI);
        u1 -= floorf(u1);
        u2 -= floorf(u2);

        uint8_t r1,g1,b1, r2,g2,b2;
        ws_palette_sample_rgb8(pal, u1, &r1, &g1, &b1);
        ws_palette_sample_rgb8(pal, u2, &r2, &g2, &b2);

        /* Crossfade между двумя цветами волны */
        float mix_raw = 0.5f + 0.5f * sinf(phase_a + pos * 2.5f * (float)M_PI);
        float mix = smoothstep(mix_raw);  /* сглаживаем пики, чтобы волна была шелковистой */
        float inv = 1.0f - mix;

        uint8_t r = (uint8_t)(r1 * inv + r2 * mix + 0.5f);
        uint8_t g = (uint8_t)(g1 * inv + g2 * mix + 0.5f);
        uint8_t b = (uint8_t)(b1 * inv + b2 * mix + 0.5f);

        set_rgb_temporal(ws, first + i, r, g, b, temporal_smooth, first_frame);
    }
}

static void fx_energize_pulse(ws2812_t *ws,
                              const ws_palette_t *pal,
                              fx_state_t *st,
                              uint32_t delta_ms,
                              uint16_t first,
                              uint16_t count)
{
    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Медленный вдох-выдох + редкий быстрый акцент — как «энергетический» */
    float base_freq = (st->speed > 0.0f) ? st->speed : 0.055f;
    st->t += (float)delta_ms * 0.001f * base_freq;
    float envelope = 0.35f + 0.65f * (0.5f + 0.5f * sinf(st->t * 2.0f * (float)M_PI));

    /* Быстрый акцент поверх: короткий импульс */
    float accent = 0.0f;
    float pulse_phase = fmodf(st->t * 0.35f, 1.0f); /* раз в ~3 сек */
    if (pulse_phase < 0.20f) {
        float p = pulse_phase / 0.20f;
        accent = 0.35f * sinf(p * (float)M_PI); /* плавный одиночный удар */
    }

    for (uint16_t i = 0; i < count; ++i) {
        float u = fx_pos_norm(i, count);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        float k = clamp01f(envelope + accent);
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_rgb(ws, first + i, r, g, b);
    }
}

static void fx_velvet_flow(ws2812_t *ws,
                           const ws_palette_t *pal,
                           fx_state_t *st,
                           uint32_t delta_ms,
                           uint16_t first,
                           uint16_t count)
{
    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Ещё более медленная скорость для бархатного эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.005f;
    float t_prev = st->t;  /* нужен, чтобы на первом кадре не мешать интро-цвет */
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

    /* Усиленное пространственное сглаживание для бархатного эффекта */
    const float spatial_smooth  = 0.85f;  // 0.85 даёт очень плавный "бархатный" эффект

    /* Более сильное временное сглаживание */
    uint8_t first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                          (st->a == 0.0f)  && (st->b == 0.0f);
    const float temporal_smooth = first_frame ? 0.0f : 0.80f;  // 80% старого кадра

    uint8_t prev_r = 0;
    uint8_t prev_g = 0;
    uint8_t prev_b = 0;

    for (uint16_t i = 0; i < count; ++i) {
        float u  = fx_pos_norm(i, count);
        float uu = u + offset;
        uu -= floorf(uu);

        uint8_t r_raw, g_raw, b_raw;
        ws_palette_sample_rgb8(pal, uu, &r_raw, &g_raw, &b_raw);

        /* Пространственное сглаживание */
        uint8_t r_sp = r_raw;
        uint8_t g_sp = g_raw;
        uint8_t b_sp = b_raw;

        if (i > 0) {
            float inv = 1.0f - spatial_smooth;
            r_sp = (uint8_t)(prev_r * spatial_smooth + r_raw * inv + 0.5f);
            g_sp = (uint8_t)(prev_g * spatial_smooth + g_raw * inv + 0.5f);
            b_sp = (uint8_t)(prev_b * spatial_smooth + b_raw * inv + 0.5f);
        }

        /* Временное сглаживание */
        uint32_t idx = (uint32_t)(first + i) * 3u;
        uint8_t old_r = first_frame ? 0 : ws->rgb[idx + 0];
        uint8_t old_g = first_frame ? 0 : ws->rgb[idx + 1];
        uint8_t old_b = first_frame ? 0 : ws->rgb[idx + 2];

        float inv_t = 1.0f - temporal_smooth;

        uint8_t r = (uint8_t)(old_r * temporal_smooth + r_sp * inv_t + 0.5f);
        uint8_t g = (uint8_t)(old_g * temporal_smooth + g_sp * inv_t + 0.5f);
        uint8_t b = (uint8_t)(old_b * temporal_smooth + b_sp * inv_t + 0.5f);
        
        ws_set_pixel_rgb(ws, first + i, r, g, b);
        prev_r = r_sp;
        prev_g = g_sp;
        prev_b = b_sp;
    }
}

static void fx_gentle_pulse(ws2812_t *ws,
                            const ws_palette_t *pal,
                            fx_state_t *st,
                            uint32_t delta_ms,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Очень медленный, глубокий пульс без резких вершин */
    float freq = (st->speed > 0.0f) ? st->speed : 0.018f;  // ~0.018 Гц ≈ 55 сек на цикл
    st->t += (float)delta_ms * 0.001f * freq;

    float raw_s = 0.5f + 0.5f * sinf(st->t * 2.0f * (float)M_PI);  // 0..1
    float s = smoothstep(raw_s);  // мягкая кривая

    /* Диапазон 50%..80% для деликатного дыхания */
    float k = 0.50f + 0.30f * s;

    for (uint16_t i = 0; i < count; ++i) {
        float u = fx_pos_norm(i, count);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_rgb(ws, first + i, r, g, b);
    }
}

/* =========================================================================
 * W223/EQS PREMIUM EFFECTS
 * ========================================================================= */

/**
 * @brief Aurora / Northern Lights effect
 * Несколько синусоид с разными фазами и скоростями создают эффект северного сияния.
 * Магический, спокойный характер.
 */
static void fx_aurora(ws2812_t *ws,
                      const ws_palette_t *pal,
                      fx_state_t *st,
                      uint32_t delta_ms,
                      uint16_t first,
                      uint16_t count)
{
    float t_prev;
    uint8_t first_frame;
    float temporal_smooth;

    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Очень медленная базовая скорость для магического эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.006f;
    t_prev = st->t;
    st->t += (float)delta_ms * 0.001f * speed;
    first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                  (st->a == 0.0f)  && (st->b == 0.0f);
    temporal_smooth = first_frame ? 0.0f : AMB_FX_TEMPORAL_SMOOTH;
    
    /* Три волны с разными частотами и фазами */
    float wave1_freq = 1.0f;
    float wave2_freq = 1.7f;   /* Золотое сечение-подобное отношение */
    float wave3_freq = 2.3f;
    
    float wave1_amp = 0.4f;
    float wave2_amp = 0.3f;
    float wave3_amp = 0.2f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = fx_pos_norm(i, count);
        
        /* Три синусоиды для aurora эффекта */
        float w1 = sinf((x * wave1_freq + st->t) * 2.0f * (float)M_PI) * wave1_amp;
        float w2 = sinf((x * wave2_freq + st->t * 1.3f + 0.3f) * 2.0f * (float)M_PI) * wave2_amp;
        float w3 = sinf((x * wave3_freq + st->t * 0.7f + 0.7f) * 2.0f * (float)M_PI) * wave3_amp;
        
        /* Комбинированное смещение по палитре */
        float palette_offset = w1 + w2 + w3;
        float u = x + palette_offset;
        u = u - floorf(u);  /* Wrap around */
        
        /* Волнообразное изменение яркости */
        float brightness_wave = 0.8f + 0.2f * sinf((x * 2.0f + st->t * 0.5f) * 2.0f * (float)M_PI);
        brightness_wave = smoothstep(brightness_wave);
        
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        
        r = (uint8_t)(r * brightness_wave);
        g = (uint8_t)(g * brightness_wave);
        b = (uint8_t)(b * brightness_wave);
        
        set_rgb_temporal(ws, first + i, r, g, b, temporal_smooth, first_frame);
    }
}

/**
 * @brief Cascade / Waterfall effect
 * Волна яркости, "падающая" от начала к концу ленты и затухающая.
 * Динамичный, элегантный характер.
 */
static void fx_cascade(ws2812_t *ws,
                       const ws_palette_t *pal,
                       fx_state_t *st,
                       uint32_t delta_ms,
                       uint16_t first,
                       uint16_t count)
{
    uint8_t first_frame;
    float temporal_smooth;

    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Замедлили каскад и усилили сглаживание, чтобы убрать строб */
    float speed = (st->speed > 0.0f) ? st->speed : 0.007f;
    float t_prev = st->t;
    st->t += (float)delta_ms * 0.001f * speed;
    first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                  (st->a == 0.0f)  && (st->b == 0.0f);
    
    /* Позиция "головы" каскада (циклическая) */
    float head_pos = st->t - floorf(st->t);
    
    /* Ширина каскадной волны */
    float cascade_width = 0.42f;

    /* Временное сглаживание */
    temporal_smooth = first_frame ? 0.0f : 0.92f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = fx_pos_norm(i, count);
        
        /* Расстояние до головы каскада (с учётом wrap-around) */
        float dist = x - head_pos;
        if (dist < 0.0f) dist += 1.0f;  /* Wrap around */
        
        /* Хвост каскада: яркость падает с расстоянием */
        float cascade_factor;
        if (dist < cascade_width) {
            /* Голова каскада - максимальная яркость с плавным fade */
            cascade_factor = 1.0f - (dist / cascade_width);
            cascade_factor = ease_out_quad(cascade_factor);
        } else {
            /* За пределами каскада - базовая яркость */
            cascade_factor = 0.0f;
        }
        cascade_factor = smoothstep(cascade_factor);
        
        /* Комбинированная яркость: база + каскад */
        float brightness = 0.58f + 0.36f * cascade_factor;
        brightness = soft_clip01(brightness, 0.88f);
        
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        
        uint8_t r_new = (uint8_t)(r * brightness);
        uint8_t g_new = (uint8_t)(g * brightness);
        uint8_t b_new = (uint8_t)(b * brightness);

        set_rgb_temporal(ws, first + i, r_new, g_new, b_new, temporal_smooth, first_frame);
    }
}

/**
 * @brief Sparkle effect
 * Редкие мерцания отдельных LED поверх базового градиента.
 * Праздничный, роскошный характер.
 */
static void fx_sparkle(ws2812_t *ws,
                       const ws_palette_t *pal,
                       fx_state_t *st,
                       uint32_t delta_ms,
                       uint16_t first,
                       uint16_t count)
{
    float t_prev;
    uint8_t first_frame;
    float temporal_smooth;

    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Базовая скорость и таймер */
    float speed = (st->speed > 0.0f) ? st->speed : 0.02f;
    t_prev = st->t;
    st->t += (float)delta_ms * 0.001f * speed;
    first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                  (st->a == 0.0f)  && (st->b == 0.0f);
    temporal_smooth = first_frame ? 0.0f : 0.86f;
    /* Density kept low for premium restraint: few highlights, no disco look. */
    const float sparkle_density = 0.026f;
    const float twinkle_rate = 0.28f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = fx_pos_norm(i, count);
        
        /* Базовый цвет из палитры */
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        
        /* Stable per-LED profile: no frame-random pops, only smooth twinkle cycles. */
        uint32_t base_seed = (uint32_t)(first + i) * 2654435761u;
        float density_pick = hash01_u32(base_seed ^ 0x9E3779B9u);
        float phase_jitter = hash01_u32(base_seed ^ 0x85EBCA6Bu);
        float amp_jitter = hash01_u32(base_seed ^ 0xC2B2AE35u);
        float sparkle_boost = 0.0f;

        if (density_pick < sparkle_density) {
            float p = fractf(st->t * twinkle_rate + phase_jitter * 2.0f);
            float tri = 1.0f - fabsf(2.0f * p - 1.0f);     /* 0..1 triangle */
            float env = smoothstep(tri);
            env = env * env;                               /* softer peak */
            sparkle_boost = env * (0.55f + 0.45f * amp_jitter);
        }
        
        /* Базовая яркость + искра */
        float base_wave = 0.66f + 0.06f * sinf((st->t * 0.7f + x * 0.8f) * 2.0f * (float)M_PI);
        float brightness = base_wave + 0.24f * sparkle_boost;
        brightness = soft_clip01(brightness, 0.86f);
        
        /* Для искр также слегка сдвигаем к белому */
        if (sparkle_boost > 0.1f) {
            float white_mix = sparkle_boost * 0.12f;
            r = (uint8_t)(r + (255 - r) * white_mix);
            g = (uint8_t)(g + (255 - g) * white_mix);
            b = (uint8_t)(b + (255 - b) * white_mix);
        }
        
        r = (uint8_t)(r * brightness);
        g = (uint8_t)(g * brightness);
        b = (uint8_t)(b * brightness);
        
        set_rgb_temporal(ws, first + i, r, g, b, temporal_smooth, first_frame);
    }
}

/**
 * @brief Breathe Wave effect
 * Волна яркости, проходящая по ленте (не просто пульс, а движущаяся волна).
 * Живой, медитативный характер.
 */
static void fx_breathe_wave(ws2812_t *ws,
                            const ws_palette_t *pal,
                            fx_state_t *st,
                            uint32_t delta_ms,
                            uint16_t first,
                            uint16_t count)
{
    float t_prev;
    uint8_t first_frame;
    float temporal_smooth;

    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Очень медленная скорость для медитативного эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.008f;
    t_prev = st->t;
    st->t += (float)delta_ms * 0.001f * speed;
    first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                  (st->a == 0.0f)  && (st->b == 0.0f);
    temporal_smooth = first_frame ? 0.0f : AMB_FX_TEMPORAL_SMOOTH;
    
    /* Центр волны дыхания движется по ленте */
    float wave_center = st->t - floorf(st->t);
    
    /* Ширина волны дыхания */
    float wave_width = 0.45f;
    
    /* Амплитуда дыхания (минимум - максимум яркости) */
    float min_brightness = 0.62f;
    float max_brightness = 1.0f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = fx_pos_norm(i, count);
        
        /* Расстояние до центра волны (с wrap-around) */
        float dist = fabsf(x - wave_center);
        if (dist > 0.5f) dist = 1.0f - dist;  /* Shortest distance with wrap */
        
        /* Интенсивность дыхания в зависимости от расстояния */
        float wave_intensity;
        if (dist < wave_width) {
            wave_intensity = 1.0f - (dist / wave_width);
            wave_intensity = ease_in_out_cubic(wave_intensity);
        } else {
            wave_intensity = 0.0f;
        }
        
        float brightness = min_brightness + (max_brightness - min_brightness) * wave_intensity;
        
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        
        r = (uint8_t)(r * brightness);
        g = (uint8_t)(g * brightness);
        b = (uint8_t)(b * brightness);
        
        set_rgb_temporal(ws, first + i, r, g, b, temporal_smooth, first_frame);
    }
}

/**
 * @brief Color Morph effect
 * Плавное перетекание между двумя зонами палитры.
 * Гипнотический, премиальный характер.
 */
static void fx_color_morph(ws2812_t *ws,
                           const ws_palette_t *pal,
                           fx_state_t *st,
                           uint32_t delta_ms,
                           uint16_t first,
                           uint16_t count)
{
    float t_prev;
    uint8_t first_frame;
    float temporal_smooth;

    if (!ws || !ws->rgb || !pal || !st || count == 0) return;

    /* Медленная скорость для гипнотического эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.004f;
    t_prev = st->t;
    st->t += (float)delta_ms * 0.001f * speed;
    first_frame = (t_prev == 0.0f) && (st->phase == 0.0f) &&
                  (st->a == 0.0f)  && (st->b == 0.0f);
    temporal_smooth = first_frame ? 0.0f : AMB_FX_TEMPORAL_SMOOTH;
    
    /* Морфинг между двумя позициями палитры */
    float morph_phase = st->t - floorf(st->t);
    
    /* Применяем smoothstep для плавного перехода между зонами */
    float zone_blend = sinf(morph_phase * 2.0f * (float)M_PI) * 0.5f + 0.5f;
    zone_blend = smoothstep(zone_blend);

    /* Лёгкое дыхание всей сцены, чтобы убрать статичность */
    float breath_phase = 0.5f + 0.5f * sinf(st->t * 2.0f * (float)M_PI * 0.12f);
    float breathing = 0.85f + 0.15f * smoothstep(breath_phase);
    
    /* Две "зоны" палитры для морфинга */
    float zone1_center = 0.25f;
    float zone2_center = 0.75f;
    
    for (uint16_t i = 0; i < count; ++i) {
        float x = fx_pos_norm(i, count);
        
        /* Позиция в палитре для каждой зоны */
        float u1 = zone1_center + (x - 0.5f) * 0.4f;  /* Зона 1 */
        float u2 = zone2_center + (x - 0.5f) * 0.4f;  /* Зона 2 */
        
        /* Нормализация позиций */
        u1 = u1 - floorf(u1);
        u2 = u2 - floorf(u2);
        
        /* Получаем цвета из обеих зон */
        uint8_t r1, g1, b1, r2, g2, b2;
        ws_palette_sample_rgb8(pal, u1, &r1, &g1, &b1);
        ws_palette_sample_rgb8(pal, u2, &r2, &g2, &b2);
        
        /* Смешиваем с учётом пространственного смещения */
        float local_blend = zone_blend + (x - 0.5f) * 0.2f;
        local_blend = clamp01f(local_blend);
        local_blend = smoothstep(local_blend);
        
        float inv_blend = 1.0f - local_blend;
        
        uint8_t r = (uint8_t)(r1 * inv_blend + r2 * local_blend);
        uint8_t g = (uint8_t)(g1 * inv_blend + g2 * local_blend);
        uint8_t b = (uint8_t)(b1 * inv_blend + b2 * local_blend);
        
        r = (uint8_t)(r * breathing);
        g = (uint8_t)(g * breathing);
        b = (uint8_t)(b * breathing);

        set_rgb_temporal(ws, first + i, r, g, b, temporal_smooth, first_frame);
    }
}

/* === One-shot FX ====================================================== */

static void fx_welcome_sweep(ws2812_t *ws,
                             const ws_palette_t *pal,
                             float t_norm,
                             uint16_t first,
                             uint16_t count)
{
    if (!ws || !ws->rgb || !pal || count == 0) return;

    float head  = t_norm * (float)count;
    float width = 0.25f * (float)count;

    for (uint16_t i = 0; i < count; ++i) {
        float pos = (float)i;
        float d   = head - pos;
        float w   = clamp01f(1.0f - fabsf(d) / width);
        float u   = fx_pos_norm(i, count);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        float k = 0.15f + 0.85f * w;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_rgb(ws, first + i, r, g, b);
    }
}

static void fx_welcome_zone(ws2812_t *ws,
                            const ws_palette_t *pal,
                            float t_norm,
                            float base_br,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !pal || count == 0) return;

    float t = clamp01f(t_norm * FX_WELCOME_TIME_SCALE);
    float fade = ease_in_out_cubic(t);
    float pre_roll = smoothstep(clamp01f((t - 0.020f) / 0.20f));
    float center = (float)(count - 1u) * FX_WELCOME_CENTER_OFFSET;
    float half = (float)((count > 1u) ? (count - 1u) : 1u) * 0.5f;
    float settle_mix = 0.0f;
    float pulse = 1.0f;
    float wave_phase = 0.0f;
    float wave_gate = 0.0f;
    float travel_pos;
    float center_seed;
    float pulse_end = FX_WELCOME_PULSE_START + FX_WELCOME_PULSE_DURATION;
    float wave_end = FX_WELCOME_WAVE_START + FX_WELCOME_WAVE_DURATION;
    uint8_t first_frame = (t_norm <= 0.01f) ? 1u : 0u;
    float temporal_smooth = first_frame ? 0.0f : 0.74f;
    uint8_t r_mid, g_mid, b_mid;

    ws_palette_sample_rgb8(pal, 0.5f, &r_mid, &g_mid, &b_mid);
    center_seed = smoothstep(clamp01f((t - 0.02f) / 0.22f));

    if (t > FX_WELCOME_SETTLE_START) {
        float p = (t - FX_WELCOME_SETTLE_START) / (1.0f - FX_WELCOME_SETTLE_START);
        settle_mix = smoothstep(p);
    }

    if (t > FX_WELCOME_WAVE_START && t < wave_end) {
        wave_phase = (t - FX_WELCOME_WAVE_START) / FX_WELCOME_WAVE_DURATION;
        wave_phase = clamp01f(wave_phase);
        wave_gate = sinf((float)M_PI * wave_phase);
        wave_gate = wave_gate * wave_gate;
    }
    travel_pos = smoothstep(wave_phase);

    if (t > FX_WELCOME_PULSE_START && t < pulse_end) {
        float tp = (t - FX_WELCOME_PULSE_START) / FX_WELCOME_PULSE_DURATION;
        float shaped = sinf(tp * (float)M_PI);
        pulse = 1.0f + FX_WELCOME_PULSE_AMPLITUDE * shaped * shaped;
    }

    for (uint16_t i = 0; i < count; ++i) {
        float fi = (float)i;
        float x = (half > 0.5f) ? (fi - center) / half : 0.0f; /* -1..1 around visual center */
        float dist = fabsf(x);
        float u = fx_pos_norm(i, count);
        float u_shift = u + 0.08f * (0.5f - dist) * wave_gate + 0.03f * sinf((float)i * 0.29f + t * 2.8f);
        float front = expf(-20.0f * (dist - travel_pos) * (dist - travel_pos)) * wave_gate;
        float tail = expf(-7.5f * dist) * (1.0f - travel_pos) * wave_gate;
        float center_core = expf(-14.0f * x * x) * center_seed * (1.0f - smoothstep((t - 0.58f) / 0.30f));
        float crown = expf(-18.0f * x * x) * smoothstep((t - 0.62f) / 0.24f);
        float warm = expf(-14.0f * x * x) * smoothstep((t - 0.50f) * 2.0f);
        float edge_mask = clamp01f(1.0f - FX_WELCOME_DIST_SCALE * dist * (1.0f - settle_mix));
        float br = fade * edge_mask;
        uint8_t r, g, b;
        float white_mix;
        float r_f;
        float g_f;
        float b_f;

        /* Slight palette motion in intro makes it feel less static and closer to OEM premium FX. */
        u_shift = u_shift - floorf(u_shift);
        ws_palette_sample_rgb8(pal, u_shift, &r, &g, &b);

        br *= (1.0f + FX_WELCOME_WAVE_GAIN_MAX * (1.55f * front + 0.55f * tail));
        br *= pulse;
        br *= FX_WELCOME_FINAL_SCALE;
        br = soft_clip01(br + 0.13f * center_core + 0.05f * crown, 0.80f);
        br *= pre_roll;
        if (br > base_br) br = base_br;

        r_f = (float)r * br;
        g_f = (float)g * br;
        b_f = (float)b * br;

        /* Controlled white-core highlight in middle-late phase, prevents harsh flat peak. */
        white_mix = 0.14f * warm;
        if (white_mix > 0.0f) {
            r_f = r_f + ((float)r_mid * br - r_f) * white_mix;
            g_f = g_f + ((float)g_mid * br - g_f) * white_mix;
            b_f = b_f + ((float)b_mid * br - b_f) * white_mix;
        }

        set_rgb_temporal(ws,
                         first + i,
                         (uint8_t)(r_f + 0.5f),
                         (uint8_t)(g_f + 0.5f),
                         (uint8_t)(b_f + 0.5f),
                         temporal_smooth,
                         first_frame);
    }
}

static void fx_goodbye_zone(ws2812_t *ws,
                            const ws_palette_t *pal,
                            float t_norm,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !pal || count == 0) return;

    float t = clamp01f(t_norm);
    float t_e = ease_in_out_cubic(t);
    float center = (float)(count - 1u) * 0.5f;
    float half = (float)((count > 1u) ? (count - 1u) : 1u) * 0.5f;
    float edge_progress = clamp01f((t_e - 0.06f) / 0.70f);
    float center_hold = 1.0f - smoothstep(clamp01f((t_e - 0.56f) / 0.40f));
    float global = 1.0f - smoothstep(t_e);
    uint8_t first_frame = (t_norm <= 0.01f) ? 1u : 0u;
    float temporal_smooth = first_frame ? 0.0f : 0.80f;

    for (uint16_t i = 0; i < count; ++i) {
        float fi = (float)i;
        float x = (half > 0.5f) ? (fi - center) / half : 0.0f;  /* -1..1 */
        float edge = fabsf(x);
        float u = fx_pos_norm(i, count);
        float u_shift = u - 0.05f * edge_progress + 0.015f * sinf((float)i * 0.19f + t * 2.2f);
        float edge_shape = powf(edge, 0.82f);
        float curtain = 1.0f - edge_progress * edge_shape * FX_GOODBYE_CURTAIN_SCALE;
        float tail = expf(-12.5f * x * x) * center_hold;
        float ember = expf(-16.0f * x * x) * (1.0f - smoothstep((t_e - 0.72f) / 0.24f));
        float br;
        uint8_t r, g, b;
        float r_f;
        float g_f;
        float b_f;

        if (curtain < 0.0f) curtain = 0.0f;
        if (curtain > 1.0f) curtain = 1.0f;

        br = global * curtain;
        br = br + 0.12f * tail * (1.0f - t_e);
        br = br + 0.08f * ember * (1.0f - t_e);
        br = soft_clip01(br, 0.80f);
        if (br < 0.002f) {
            set_rgb_temporal(ws, first + i, 0u, 0u, 0u, temporal_smooth, first_frame);
            continue;
        }

        u_shift = u_shift - floorf(u_shift);
        ws_palette_sample_rgb8(pal, u_shift, &r, &g, &b);

        r_f = (float)r * br;
        g_f = (float)g * br;
        b_f = (float)b * br;

        /* Subtle amber-warm tail at the very end, typical of premium curtain fade. */
        if (t_e > 0.66f) {
            float warm = smoothstep((t_e - 0.66f) / 0.34f) * (0.24f * tail + 0.18f * ember);
            r_f = r_f + ((220.0f * br) - r_f) * warm;
            g_f = g_f + ((150.0f * br) - g_f) * warm;
        }

        set_rgb_temporal(ws,
                         first + i,
                         (uint8_t)(r_f + 0.5f),
                         (uint8_t)(g_f + 0.5f),
                         (uint8_t)(b_f + 0.5f),
                         temporal_smooth,
                         first_frame);
    }
}

static void fx_goodbye_fade(ws2812_t *ws,
                            const ws_palette_t *pal,
                            float base_br,
                            float t_norm,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !ws->rgb || !pal || count == 0) return;

    float k = clamp01f(1.0f - t_norm);
    float br = base_br * k;

    for (uint16_t i = 0; i < count; ++i) {
        float u = fx_pos_norm(i, count);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);
        set_rgb(ws, first + i, r, g, b);
    }
}

/* === Public API ======================================================= */

void ws_oneshot_start(ws2812_t        *ws,
                      oneshot_t       *os,
                      fx_id_t          fx_id,
                      const ws_palette_t *pal,
                      float            base_br,
                      uint32_t         duration_ms,
                      uint16_t         first,
                      uint16_t         count)
{
    (void)ws;
    if (!os || !pal || duration_ms == 0 || count == 0) return;

    os->active      = 1u;
    os->fx_id       = fx_id;
    os->pal         = pal;
    os->base_br     = clamp01f(base_br);
    os->start_ms    = HAL_GetTick();
    os->duration_ms = duration_ms;
    os->first       = first;
    os->count       = count;
}

uint8_t ws_oneshot_tick(ws2812_t *ws, oneshot_t *os)
{
    if (!ws || !os || !os->active || !os->pal || os->count == 0)
        return 1u;

    uint32_t now = HAL_GetTick();
    uint32_t dt  = now - os->start_ms;
    float t = (os->duration_ms > 0)
              ? (float)dt / (float)os->duration_ms
              : 1.0f;
    if (t >= 1.0f) t = 1.0f;

    switch (os->fx_id) {
    case FX_WELCOME_SWEEP:
    case FX_WELCOME_LUXE:
        fx_welcome_sweep(ws, os->pal, t, os->first, os->count);
        break;
    case FX_GOODBYE_FADE:
    case FX_GOODBYE_LUXE:
        fx_goodbye_fade(ws, os->pal, os->base_br, t, os->first, os->count);
        break;
    case FX_WELCOME:
        fx_welcome_zone(ws, os->pal, t, os->base_br, os->first, os->count);
        break;
    case FX_GOODBYE:
        fx_goodbye_zone(ws, os->pal, t, os->first, os->count);
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
                 uint32_t            delta_ms,
                 uint16_t            first,
                 uint16_t            count)
{
    if (!ws || !pal || count == 0) return;

    switch (fx_id) {
    /* Basic effects */
    case FX_SOLID_GRADIENT:
        fx_solid_gradient(ws, pal, st, first, count);
        break;
    case FX_GRADIENT_FLOW:
        fx_gradient_flow(ws, pal, st, delta_ms, first, count);
        break;
    case FX_SOFT_BREATHE:
        fx_soft_breathe(ws, pal, st, delta_ms, first, count);
        break;
    case FX_DUAL_ZONE:
        fx_dual_zone(ws, pal, st, first, count);
        break;
    case FX_TWIN_WAVE:
        fx_twin_wave(ws, pal, st, delta_ms, first, count);
        break;

    /* Mercedes-inspired effects */
    case FX_OCEAN_FLOW:
        fx_ocean_flow(ws, pal, st, delta_ms, first, count);
        break;
    case FX_TWO_TONE_WAVE:
        fx_two_tone_wave(ws, pal, st, delta_ms, first, count);
        break;
    case FX_ENERGIZE_PULSE:
        fx_energize_pulse(ws, pal, st, delta_ms, first, count);
        break;
    case FX_VELVET_FLOW:
        fx_velvet_flow(ws, pal, st, delta_ms, first, count);
        break;
    case FX_GENTLE_PULSE:
        fx_gentle_pulse(ws, pal, st, delta_ms, first, count);
        break;

    /* W223/EQS Premium effects */
    case FX_AURORA:
        fx_aurora(ws, pal, st, delta_ms, first, count);
        break;
    case FX_CASCADE:
        fx_cascade(ws, pal, st, delta_ms, first, count);
        break;
    case FX_SPARKLE:
        fx_sparkle(ws, pal, st, delta_ms, first, count);
        break;
    case FX_BREATHE_WAVE:
        fx_breathe_wave(ws, pal, st, delta_ms, first, count);
        break;
    case FX_COLOR_MORPH:
        fx_color_morph(ws, pal, st, delta_ms, first, count);
        break;

    default:
        fx_solid_gradient(ws, pal, st, first, count);
        break;
    }
}
