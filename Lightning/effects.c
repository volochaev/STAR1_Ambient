#include "driver.h"
#include "effects.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Константы для эффектов */
/* Intro timing: scaled so animation completes before t_norm=1.0 */
#define FX_WELCOME_TIME_SCALE       0.85f   /* Animation completes at ~0.85, leaving 0.15 for settle */
#define FX_WELCOME_WAVE_START       0.10f
#define FX_WELCOME_WAVE_DURATION    0.50f   /* Waves end at 0.60 of scaled time */
#define FX_WELCOME_WAVE_GAIN_MAX    0.25f
#define FX_WELCOME_PULSE_START      0.55f   /* Pulse starts earlier */
#define FX_WELCOME_PULSE_DURATION   0.20f
#define FX_WELCOME_PULSE_AMPLITUDE  0.08f
#define FX_WELCOME_CENTER_OFFSET    0.47f
#define FX_WELCOME_DIST_SCALE       0.25f   /* Less edge dimming for smoother transition */
#define FX_WELCOME_SETTLE_START     0.75f   /* Final settle phase: equalize brightness */
#define FX_GOODBYE_CURTAIN_SCALE    1.2f
#define OEM_BRIGHTNESS_MAX          5u

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
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

/* Простой pseudo-random генератор для sparkle эффекта */
static uint32_t fx_rand_seed = 12345u;
static uint32_t fx_rand(void)
{
    fx_rand_seed = fx_rand_seed * 1103515245u + 12345u;
    return (fx_rand_seed >> 16) & 0x7FFF;
}

static inline uint32_t idx3(uint16_t i) { return (uint32_t)i * 3u; }

static inline void set_grb(ws2812_t *ws, uint16_t i,
                           uint8_t r, uint8_t g, uint8_t b)
{
    if (!ws || !ws->grb) return;
    if (i >= ws->led_count) return;

    ws_set_pixel_rgb(ws, i, r, g, b);
}

/* === Continuous FX ==================================================== */

static void fx_solid_gradient(ws2812_t *ws,
                              const ws_palette_t *pal,
                              fx_state_t *st,
                              uint16_t first,
                              uint16_t count)
{
    (void)st;
    if (!ws || !ws->grb || !pal || count == 0) return;
    for (uint16_t i = 0; i < count; ++i) {
        float u = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        set_grb(ws, first + i, r, g, b);
    }
}

static void fx_gradient_flow(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms,
                             uint16_t first,
                             uint16_t count)
{
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    float speed = (st->speed > 0.0f) ? st->speed : 0.03f;
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

    for (uint16_t i = 0; i < count; ++i) {
        float u = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        float uu = u + offset;
        uu -= floorf(uu);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, uu, &r, &g, &b);
        set_grb(ws, first + i, r, g, b);
    }
}

static void fx_soft_breathe(ws2812_t *ws,
                            const ws_palette_t *pal,
                            fx_state_t *st,
                            uint32_t delta_ms,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    float freq = (st->speed > 0.0f) ? st->speed : 0.07f;
    st->t += (float)delta_ms * 0.001f * freq;
    float s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f;
    float br = 0.25f + 0.35f * s;

    for (uint16_t i = 0; i < count; ++i) {
        float u = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);
        set_grb(ws, first + i, r, g, b);
    }
}

static void fx_dual_zone(ws2812_t *ws,
                         const ws_palette_t *pal,
                         fx_state_t *st,
                         uint16_t first,
                         uint16_t count)
{
    (void)st;
    if (!ws || !ws->grb || !pal || count == 0) return;

    for (uint16_t i = 0; i < count; ++i) {
        float v = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        /* Создаём два градиента: от начала до середины и от середины до конца */
        float u = (v < 0.5f)
                  ? (v * 2.0f)           /* первая половина: 0..0.5 -> 0..1 */
                  : ((v - 0.5f) * 2.0f); /* вторая половина: 0.5..1 -> 0..1 */
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, clamp01f(u), &r, &g, &b);
        set_grb(ws, first + i, r, g, b);
    }
}

static void fx_twin_wave(ws2812_t *ws,
                         const ws_palette_t *pal,
                         fx_state_t *st,
                         uint32_t delta_ms,
                         uint16_t first,
                         uint16_t count)
{
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    float speed = (st->speed > 0.0f) ? st->speed : 0.12f;
    st->t += (float)delta_ms * 0.001f * speed;

    for (uint16_t i = 0; i < count; ++i) {
        float x = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        float w1 = 0.5f * (sinf((x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w2 = 0.5f * (sinf((1.0f - x - st->t) * 6.2831f) * 0.5f + 0.5f);
        float w = clamp01f(w1 + w2);

        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        float k = 0.2f + 0.8f * w;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, first + i, r, g, b);
    }
}

static void fx_mb_ocean_flow(ws2812_t *ws,
                             const ws_palette_t *pal,
                             fx_state_t *st,
                             uint32_t delta_ms,
                             uint16_t first,
                             uint16_t count)
{
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* 1) Очень медленная скорость – чтобы шаг между кадрами был крошечный */
    float speed = (st->speed > 0.0f) ? st->speed : 0.008f;  // было 0.01, можно даже 0.005
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

    /* 2) Сглаживание по ленте (между соседними диодами) */
    const float spatial_smooth  = 0.70f;  // 0.7–0.8 даёт мягкий "водяной" эффект

    /* 3) Лёгкое временное сглаживание (между кадрами) */
    /* На первом кадре после смены темы/интро не подмешиваем старый кадр,
     * иначе в новых палитрах видно «мигание» от старых цветов. */
    uint8_t first_frame = (st->t == 0.0f) && (st->phase == 0.0f) &&
                          (st->a == 0.0f)  && (st->b == 0.0f);
    const float temporal_smooth = first_frame ? 0.0f : 0.70f;  // 0.7 – новый кадр только на 30% сразу

    uint8_t prev_r = 0;
    uint8_t prev_g = 0;
    uint8_t prev_b = 0;

    for (uint16_t i = 0; i < count; ++i) {
        float u  = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
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
        /* Читаем предыдущий кадр в RGB формате (ws_set_pixel_rgb использует RGB) */
        uint32_t idx = (uint32_t)(first + i) * 3u;
        uint8_t old_r = first_frame ? 0 : ws->grb[idx + 0];
        uint8_t old_g = first_frame ? 0 : ws->grb[idx + 1];
        uint8_t old_b = first_frame ? 0 : ws->grb[idx + 2];

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

static void fx_mb_two_tone_wave(ws2812_t *ws,
                                const ws_palette_t *pal,
                                fx_state_t *st,
                                uint32_t delta_ms,
                                uint16_t first,
                                uint16_t count)
{
    fx_twin_wave(ws, pal, st, delta_ms, first, count);
}

static void fx_mb_energize_pulse(ws2812_t *ws,
                                 const ws_palette_t *pal,
                                 fx_state_t *st,
                                 uint32_t delta_ms,
                                 uint16_t first,
                                 uint16_t count)
{
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    float freq = (st->speed > 0.0f) ? st->speed : 0.1f;
    st->t += (float)delta_ms * 0.001f * freq;
    float s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f;

    for (uint16_t i = 0; i < count; ++i) {
        float u = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        float k = 0.2f + 0.8f * s;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, first + i, r, g, b);
    }
}

static void fx_mb_velvet_flow(ws2812_t *ws,
                              const ws_palette_t *pal,
                              fx_state_t *st,
                              uint32_t delta_ms,
                              uint16_t first,
                              uint16_t count)
{
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* Ещё более медленная скорость для бархатного эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.005f;
    st->t += (float)delta_ms * 0.001f * speed;
    float offset = st->t - floorf(st->t);

    /* Усиленное пространственное сглаживание для бархатного эффекта */
    const float spatial_smooth  = 0.85f;  // 0.85 даёт очень плавный "бархатный" эффект

    /* Более сильное временное сглаживание */
    uint8_t first_frame = (st->t == 0.0f) && (st->phase == 0.0f) &&
                          (st->a == 0.0f)  && (st->b == 0.0f);
    const float temporal_smooth = first_frame ? 0.0f : 0.80f;  // 80% старого кадра

    uint8_t prev_r = 0;
    uint8_t prev_g = 0;
    uint8_t prev_b = 0;

    for (uint16_t i = 0; i < count; ++i) {
        float u  = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
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
        uint8_t old_r = first_frame ? 0 : ws->grb[idx + 0];
        uint8_t old_g = first_frame ? 0 : ws->grb[idx + 1];
        uint8_t old_b = first_frame ? 0 : ws->grb[idx + 2];

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

static void fx_mb_gentle_pulse(ws2812_t *ws,
                               const ws_palette_t *pal,
                               fx_state_t *st,
                               uint32_t delta_ms,
                               uint16_t first,
                               uint16_t count)
{
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* Медленная частота для мягкого пульса */
    float freq = (st->speed > 0.0f) ? st->speed : 0.04f;  // ~0.04 Гц = 25 сек на цикл
    st->t += (float)delta_ms * 0.001f * freq;
    
    /* Плавная синусоидальная пульсация с easing */
    float raw_s = sinf(st->t * 2.0f * (float)M_PI) * 0.5f + 0.5f;  // 0..1
    float s = smoothstep(raw_s);  // Применяем easing для более премиальной плавности
    
    /* Более мягкий диапазон пульсации (60% - 100%) */
    float k = 0.60f + 0.40f * s;

    for (uint16_t i = 0; i < count; ++i) {
        float u = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, first + i, r, g, b);
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
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* Очень медленная базовая скорость для магического эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.006f;
    st->t += (float)delta_ms * 0.001f * speed;
    
    /* Три волны с разными частотами и фазами */
    float wave1_freq = 1.0f;
    float wave2_freq = 1.7f;   /* Золотое сечение-подобное отношение */
    float wave3_freq = 2.3f;
    
    float wave1_amp = 0.4f;
    float wave2_amp = 0.3f;
    float wave3_amp = 0.2f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        
        /* Три синусоиды для aurora эффекта */
        float w1 = sinf((x * wave1_freq + st->t) * 2.0f * (float)M_PI) * wave1_amp;
        float w2 = sinf((x * wave2_freq + st->t * 1.3f + 0.3f) * 2.0f * (float)M_PI) * wave2_amp;
        float w3 = sinf((x * wave3_freq + st->t * 0.7f + 0.7f) * 2.0f * (float)M_PI) * wave3_amp;
        
        /* Комбинированное смещение по палитре */
        float palette_offset = w1 + w2 + w3;
        float u = x + palette_offset;
        u = u - floorf(u);  /* Wrap around */
        
        /* Волнообразное изменение яркости */
        float brightness_wave = 0.7f + 0.3f * sinf((x * 2.0f + st->t * 0.5f) * 2.0f * (float)M_PI);
        brightness_wave = smoothstep(brightness_wave);
        
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        
        r = (uint8_t)(r * brightness_wave);
        g = (uint8_t)(g * brightness_wave);
        b = (uint8_t)(b * brightness_wave);
        
        set_grb(ws, first + i, r, g, b);
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
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* Умеренная скорость для каскадного эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.015f;
    st->t += (float)delta_ms * 0.001f * speed;
    
    /* Позиция "головы" каскада (циклическая) */
    float head_pos = st->t - floorf(st->t);
    
    /* Ширина каскадной волны */
    float cascade_width = 0.35f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        
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
        
        /* Комбинированная яркость: база + каскад */
        float brightness = 0.3f + 0.7f * cascade_factor;
        
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        
        r = (uint8_t)(r * brightness);
        g = (uint8_t)(g * brightness);
        b = (uint8_t)(b * brightness);
        
        set_grb(ws, first + i, r, g, b);
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
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* Базовая скорость и таймер */
    float speed = (st->speed > 0.0f) ? st->speed : 0.02f;
    st->t += (float)delta_ms * 0.001f * speed;
    
    /* Фаза для детерминированной случайности (обновляется периодически) */
    uint32_t phase = (uint32_t)(st->t * 10.0f);
    
    /* Частота появления искр (0.0 - 1.0) */
    float sparkle_density = 0.08f;
    
    /* Длительность вспышки */
    float sparkle_duration = 0.15f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        
        /* Базовый цвет из палитры */
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, x, &r, &g, &b);
        
        /* Определяем, есть ли искра на этом LED */
        /* Используем pseudo-random на основе позиции и фазы */
        uint32_t seed = (uint32_t)(x * 1000.0f) + phase * 7919u;
        seed = seed * 1103515245u + 12345u;
        float rand_val = (float)((seed >> 16) & 0x7FFF) / 32767.0f;
        
        float sparkle_boost = 0.0f;
        
        if (rand_val < sparkle_density) {
            /* Вычисляем фазу искры (0..1 внутри её жизненного цикла) */
            float sparkle_t = fmodf(st->t * 5.0f + rand_val * 10.0f, 1.0f);
            
            if (sparkle_t < sparkle_duration) {
                /* Искра активна: яркость поднимается и падает */
                float sparkle_phase = sparkle_t / sparkle_duration;
                sparkle_boost = sinf(sparkle_phase * (float)M_PI);
                sparkle_boost = sparkle_boost * sparkle_boost;  /* Квадратичное усиление */
            }
        }
        
        /* Базовая яркость + искра */
        float brightness = 0.6f + 0.4f * sparkle_boost;
        
        /* Для искр также слегка сдвигаем к белому */
        if (sparkle_boost > 0.1f) {
            float white_mix = sparkle_boost * 0.3f;
            r = (uint8_t)(r + (255 - r) * white_mix);
            g = (uint8_t)(g + (255 - g) * white_mix);
            b = (uint8_t)(b + (255 - b) * white_mix);
        }
        
        r = (uint8_t)(r * brightness);
        g = (uint8_t)(g * brightness);
        b = (uint8_t)(b * brightness);
        
        set_grb(ws, first + i, r, g, b);
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
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* Очень медленная скорость для медитативного эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.008f;
    st->t += (float)delta_ms * 0.001f * speed;
    
    /* Центр волны дыхания движется по ленте */
    float wave_center = st->t - floorf(st->t);
    
    /* Ширина волны дыхания */
    float wave_width = 0.4f;
    
    /* Амплитуда дыхания (минимум - максимум яркости) */
    float min_brightness = 0.35f;
    float max_brightness = 1.0f;

    for (uint16_t i = 0; i < count; ++i) {
        float x = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        
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
        
        set_grb(ws, first + i, r, g, b);
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
    if (!ws || !ws->grb || !pal || !st || count == 0) return;

    /* Медленная скорость для гипнотического эффекта */
    float speed = (st->speed > 0.0f) ? st->speed : 0.004f;
    st->t += (float)delta_ms * 0.001f * speed;
    
    /* Морфинг между двумя позициями палитры */
    float morph_phase = st->t - floorf(st->t);
    
    /* Применяем smoothstep для плавного перехода между зонами */
    float zone_blend = sinf(morph_phase * 2.0f * (float)M_PI) * 0.5f + 0.5f;
    zone_blend = smoothstep(zone_blend);
    
    /* Две "зоны" палитры для морфинга */
    float zone1_center = 0.25f;
    float zone2_center = 0.75f;
    
    for (uint16_t i = 0; i < count; ++i) {
        float x = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        
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
        
        set_grb(ws, first + i, r, g, b);
    }
}

/* === One-shot FX ====================================================== */

static void fx_welcome_sweep(ws2812_t *ws,
                             const ws_palette_t *pal,
                             float t_norm,
                             uint16_t first,
                             uint16_t count)
{
    if (!ws || !ws->grb || !pal || count == 0) return;

    float head  = t_norm * (float)count;
    float width = 0.25f * (float)count;

    for (uint16_t i = 0; i < count; ++i) {
        float pos = (float)i;
        float d   = head - pos;
        float w   = clamp01f(1.0f - fabsf(d) / width);
        float u   = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        float k = 0.15f + 0.85f * w;
        r = (uint8_t)(r * k);
        g = (uint8_t)(g * k);
        b = (uint8_t)(b * k);
        set_grb(ws, first + i, r, g, b);
    }
}

static void fx_welcome_zone(ws2812_t *ws,
                            const ws_palette_t *pal,
                            float t_norm,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !pal || count == 0) return;

    float t = t_norm * FX_WELCOME_TIME_SCALE;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* --- 1. Глобальный fade-in от 0 до 1, с плавной кривой --- */
    float fade = t * t * (3.0f - 2.0f * t);  // smoothstep(0..1)
    if (fade > 1.0f) fade = 1.0f;

    /* --- 2. Фаза волн (FX_WELCOME_WAVE_START..wave_end) --- */
    float wave_phase = 0.0f;
    float wave_end = FX_WELCOME_WAVE_START + FX_WELCOME_WAVE_DURATION;
    if (t > FX_WELCOME_WAVE_START && t < wave_end) {
        wave_phase = (t - FX_WELCOME_WAVE_START) / FX_WELCOME_WAVE_DURATION;   // 0..1
    }

    /* --- 3. Лёгкий pulse (FX_WELCOME_PULSE_START..PULSE_END) --- */
    float pulse = 1.0f;
    float pulse_end = FX_WELCOME_PULSE_START + FX_WELCOME_PULSE_DURATION;
    if (t > FX_WELCOME_PULSE_START && t < pulse_end) {
        float tp = (t - FX_WELCOME_PULSE_START) / FX_WELCOME_PULSE_DURATION;
        pulse = 1.0f + FX_WELCOME_PULSE_AMPLITUDE * sinf(tp * (float)M_PI);
    }

    /* --- 4. Финальная фаза стабилизации (settle) --- */
    /* Плавно уменьшаем влияние dist_scale, чтобы к концу яркость была равномерной */
    float settle_factor = 1.0f;  /* 1.0 = полный dist_scale, 0.0 = нет dist_scale */
    if (t_norm > FX_WELCOME_SETTLE_START) {
        float settle_progress = (t_norm - FX_WELCOME_SETTLE_START) / (1.0f - FX_WELCOME_SETTLE_START);
        settle_progress = settle_progress * settle_progress * (3.0f - 2.0f * settle_progress); /* smoothstep */
        settle_factor = 1.0f - settle_progress;  /* 1.0 -> 0.0 */
    }

    /* Центр с небольшим смещением, как у MB */
    float center = (float)count * FX_WELCOME_CENTER_OFFSET;
    float half   = (float)((count > 1) ? (count - 1) : 1);

    for (uint16_t i = 0; i < count; ++i)
    {
        uint8_t r0, g0, b0;
        ws_palette_sample_rgb8(pal, 0.5f, &r0, &g0, &b0);

        float fi = (float)i;
        float x  = (fi - center) / half;     // ~ -1..1
        if (x < -1.0f) x = -1.0f;
        if (x >  1.0f) x =  1.0f;

        float dist = fabsf(x);               // 0 в центре, 1 по краям

        /* --- Базовая яркость: центр ярче, края чуть темнее (уменьшается к концу) --- */
        float dist_effect = FX_WELCOME_DIST_SCALE * dist * settle_factor;
        float base = fade * (1.0f - dist_effect);
        if (base < 0.0f) base = 0.0f;

        float br = base;

        /* --- Две волны: добавляем небольшое усиление --- */
        if (wave_phase > 0.0f) {
            float c1 = -1.0f + 2.0f * wave_phase;  // -1 → +1
            float c2 =  1.0f - 2.0f * wave_phase;  // +1 → -1

            float d1 = x - c1;
            float d2 = x - c2;

            float w1 = expf(-d1 * d1 * 4.0f);
            float w2 = expf(-d2 * d2 * 4.0f);

            float w  = (w1 + w2);              // 0..~2

            /* Максимальное усиление ~ +FX_WELCOME_WAVE_GAIN_MAX% от base */
            float wave_gain = 1.0f + FX_WELCOME_WAVE_GAIN_MAX * w;
            br *= wave_gain;
        }

        /* --- Pulse --- */
        br *= pulse;

        if (br > 1.0f) br = 1.0f;

        ws_set_pixel_rgb(ws, first + i,
            (uint8_t)(r0 * br),
            (uint8_t)(g0 * br),
            (uint8_t)(b0 * br));
    }
}

static void fx_goodbye_zone(ws2812_t *ws,
                            const ws_palette_t *pal,
                            float t_norm,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !pal || count == 0) return;

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
    float half   = (float)((count > 1) ? (count - 1) : 1);
    float center = half * 0.5f;

    for (uint16_t i = 0; i < count; ++i) {
        float fi = (float)i;

        /* Нормированная позиция [-1..1] */
        float x = (fi - center) / center;
        if (x < -1.0f) x = -1.0f;
        if (x >  1.0f) x =  1.0f;
        float dist_edge = fabsf(x);  // 0 в центре, 1 по краям

        /* "Шторка": края гаснут раньше центра */
        float k_curtain = 1.0f - (dist_edge * FX_GOODBYE_CURTAIN_SCALE * curtain_pos);
        if (k_curtain < 0.0f) k_curtain = 0.0f;

        float br = global * k_curtain;

        if (br <= 0.0f) {
        	ws_set_pixel_rgb(ws, first + i, 0, 0, 0);
            continue;
        }

        /* Берём исходный цвет из палитры по центру */
        uint8_t r0, g0, b0;
        ws_palette_sample_rgb8(pal, 0.5f, &r0, &g0, &b0);

        ws_set_pixel_rgb(ws, first + i,
                    (uint8_t)(r0 * br),
                    (uint8_t)(g0 * br),
                    (uint8_t)(b0 * br));
    }
}

static void fx_goodbye_fade(ws2812_t *ws,
                            const ws_palette_t *pal,
                            float base_br,
                            float t_norm,
                            uint16_t first,
                            uint16_t count)
{
    if (!ws || !ws->grb || !pal || count == 0) return;

    float k = clamp01f(1.0f - t_norm);
    float br = base_br * k;

    for (uint16_t i = 0; i < count; ++i) {
        float u = (count == 1) ? 0.5f : (float)i / (float)(count - 1);
        uint8_t r,g,b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);
        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);
        set_grb(ws, first + i, r, g, b);
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
    case FX_MB_WELCOME:
        fx_welcome_sweep(ws, os->pal, t, os->first, os->count);
        break;
    case FX_GOODBYE_FADE:
    case FX_MB_GOODBYE:
        fx_goodbye_fade(ws, os->pal, os->base_br, t, os->first, os->count);
        break;
    case FX_WELCOME:
        fx_welcome_zone(ws, os->pal, t, os->first, os->count);
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
    case FX_MB_OCEAN_FLOW:
        fx_mb_ocean_flow(ws, pal, st, delta_ms, first, count);
        break;
    case FX_MB_TWO_TONE_WAVE:
        fx_mb_two_tone_wave(ws, pal, st, delta_ms, first, count);
        break;
    case FX_MB_ENERGIZE_PULSE:
        fx_mb_energize_pulse(ws, pal, st, delta_ms, first, count);
        break;
    case FX_MB_VELVET_FLOW:
        fx_mb_velvet_flow(ws, pal, st, delta_ms, first, count);
        break;
    case FX_MB_GENTLE_PULSE:
        fx_mb_gentle_pulse(ws, pal, st, delta_ms, first, count);
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
