/*
 * palette.c
 *
 *  Created on: Nov 9, 2025
 *      Author: nv
 */

#include "palette.h"
#include <math.h>

static inline float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint8_t lerp8(uint8_t a, uint8_t b, float t)
{
    return (uint8_t)(a + (b - a) * t + 0.5f);
}

/* Линейная интерполяция по stop-палитре */
void palette_sample_rgb8(const AMB_palette_t *pal, float u, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (!r || !g || !b) return;

    if (!pal || pal->count == 0 || !pal->stops) {
        *r = *g = *b = 0;
        return;
    }

    if (pal->count == 1) {
        *r = pal->stops[0].c.r;
        *g = pal->stops[0].c.g;
        *b = pal->stops[0].c.b;
        return;
    }

    float uu = clamp01f(u);

    /* Если левее первого стопа */
    if (uu <= pal->stops[0].pos) {
        *r = pal->stops[0].c.r;
        *g = pal->stops[0].c.g;
        *b = pal->stops[0].c.b;
        return;
    }

    /* Если правее последнего стопа */
    const AMB_pal_stop_t *last = &pal->stops[pal->count - 1];
    if (uu >= last->pos) {
        *r = last->c.r;
        *g = last->c.g;
        *b = last->c.b;
        return;
    }

    /* Ищем интервал [i, i+1] такой, что pos_i <= u <= pos_{i+1} */
    const AMB_pal_stop_t *s = pal->stops;
    for (uint8_t i = 0; i < pal->count - 1; ++i) {
        const AMB_pal_stop_t *a = &s[i];
        const AMB_pal_stop_t *b_s = &s[i + 1];

        if (uu >= a->pos && uu <= b_s->pos) {
            float span = (b_s->pos - a->pos);
            float t = (span > 0.0f) ? (uu - a->pos) / span : 0.0f;
            *r = lerp8(a->c.r, b_s->c.r, t);
            *g = lerp8(a->c.g, b_s->c.g, t);
            *b = lerp8(a->c.b, b_s->c.b, t);
            return;
        }
    }

    /* Fallback — последний стоп */
    *r = last->c.r;
    *g = last->c.g;
    *b = last->c.b;
}

/* Блендинг двух палитр поверх ws_palette_sample_rgb8 */
void ws_palette_sample_blend_rgb8(const AMB_palette_t *a, const AMB_palette_t *b, float mix, float u, uint8_t *r, uint8_t *g, uint8_t *b_out)
{
    if (!r || !g || !b_out) return;

    if (!a && !b) {
        *r = *g = *b_out = 0;
        return;
    }

    float m = clamp01f(mix);

    if (!b || m <= 0.0f) {
        palette_sample_rgb8(a, u, r, g, b_out);
        return;
    }

    if (!a || m >= 1.0f) {
        palette_sample_rgb8(b, u, r, g, b_out);
        return;
    }

    uint8_t r0, g0, b0;
    uint8_t r1, g1, b1;
    palette_sample_rgb8(a, u, &r0, &g0, &b0);
    palette_sample_rgb8(b, u, &r1, &g1, &b1);

    *r     = lerp8(r0, r1, m);
    *g     = lerp8(g0, g1, m);
    *b_out = lerp8(b0, b1, m);
}

/* ===== Mercedes-style палитры (приближены к OEM по ощущению) ===== */

/* 1. Sunset Amber / Solar — тёплый янтарный контур */
static const AMB_pal_stop_t pal_sunset_amber_stops[] = {
    {0.00f, {255, 150,  20}},
    {0.30f, {255, 130,  10}},
    {0.65f, {255, 100,   0}},
    {1.00f, {220,  70,   0}},
};
const AMB_palette_t PAL_MB_SUNSET_AMBER = {
    .stops = pal_sunset_amber_stops,
    .count = sizeof(pal_sunset_amber_stops)/sizeof(pal_sunset_amber_stops[0]),
};

/* 2. Fire Red — чистый насыщенный красный */
static const AMB_pal_stop_t pal_fire_red_stops[] = {
    {0.00f, {255,   0,   0}},
    {0.50f, {255,  20,  10}},
    {1.00f, {200,   0,   0}},
};
const AMB_palette_t PAL_MB_FIRE_RED = {
    .stops = pal_fire_red_stops,
    .count = sizeof(pal_fire_red_stops)/sizeof(pal_fire_red_stops[0]),
};

/* 3. Red Moon — глубокий рубиновый */
static const AMB_pal_stop_t pal_red_moon_stops[] = {
    {0.00f, {255,   0,  40}},
    {0.35f, {255,   0,  70}},
    {0.70f, {210,   0, 110}},
    {1.00f, {120,   0,  60}},
};
const AMB_palette_t PAL_MB_RED_MOON = {
    .stops = pal_red_moon_stops,
    .count = sizeof(pal_red_moon_stops)/sizeof(pal_red_moon_stops[0]),
};

/* 4. Warm Lounge — мягкий янтарный “Lounge” */
static const AMB_pal_stop_t pal_warm_lounge_stops[] = {
    {0.00f, {255, 190, 100}},
    {0.40f, {255, 170,  80}},
    {0.75f, {255, 150,  60}},
    {1.00f, {210, 120,  40}},
};
const AMB_palette_t PAL_MB_WARM_LOUNGE = {
    .stops = pal_warm_lounge_stops,
    .count = sizeof(pal_warm_lounge_stops)/sizeof(pal_warm_lounge_stops[0]),
};

/* 5. Ocean Blue — классический синий контур */
static const AMB_pal_stop_t pal_ocean_blue_stops[] = {
    {0.00f, {  0,  40, 255}},
    {0.35f, {  0, 110, 255}},
    {0.70f, {  0, 180, 255}},
    {1.00f, { 60, 230, 255}},
};
const AMB_palette_t PAL_MB_OCEAN_BLUE = {
    .stops = pal_ocean_blue_stops,
    .count = sizeof(pal_ocean_blue_stops)/sizeof(pal_ocean_blue_stops[0]),
};

/* 6. Dawn Blue — нежный голубой с лёгким розоватым оттенком */
static const AMB_pal_stop_t pal_dawn_blue_stops[] = {
    {0.00f, {150, 210, 255}},
    {0.50f, {180, 230, 255}},
    {1.00f, {255, 220, 255}},
};
const AMB_palette_t PAL_MB_DAWN_BLUE = {
    .stops = pal_dawn_blue_stops,
    .count = sizeof(pal_dawn_blue_stops)/sizeof(pal_dawn_blue_stops[0]),
};

/* 7. Purple Sky — фиолетово-сиреневая тема */
static const AMB_pal_stop_t pal_purple_sky_stops[] = {
    {0.00f, {170,   0, 255}},
    {0.40f, {130,   0, 255}},
    {0.80f, { 90,   0, 210}},
    {1.00f, { 60,   0, 140}},
};
const AMB_palette_t PAL_MB_PURPLE_SKY = {
    .stops = pal_purple_sky_stops,
    .count = sizeof(pal_purple_sky_stops)/sizeof(pal_purple_sky_stops[0]),
};

/* 8. Jungle Green — бирюзово-зелёный (типа "Jungle Green") */
static const AMB_pal_stop_t pal_jungle_green_stops[] = {
    {0.00f, {  0, 180, 120}},
    {0.40f, {  0, 210, 140}},
    {0.80f, {  0, 240, 170}},
    {1.00f, { 40, 255, 200}},
};
const AMB_palette_t PAL_MB_JUNGLE_GREEN = {
    .stops = pal_jungle_green_stops,
    .count = sizeof(pal_jungle_green_stops)/sizeof(pal_jungle_green_stops[0]),
};

/* 9. Glacier — ледяной бело-голубой (Polar / Ice Blue) */
static const AMB_pal_stop_t pal_glacier_stops[] = {
    {0.00f, {160, 255, 255}},
    {0.40f, {120, 225, 255}},
    {0.75f, { 80, 190, 255}},
    {1.00f, { 40, 150, 255}},
};
const AMB_palette_t PAL_MB_GLACIER = {
    .stops = pal_glacier_stops,
    .count = sizeof(pal_glacier_stops)/sizeof(pal_glacier_stops[0]),
};

/* 10. Neutral Ambient — мягкий нейтральный белый */
static const AMB_pal_stop_t pal_neutral_ambient_stops[] = {
    {0.00f, {255, 255, 255}},
    {0.50f, {235, 235, 235}},
    {1.00f, {215, 215, 215}},
};
const AMB_palette_t PAL_MB_NEUTRAL_AMBIENT = {
    .stops = pal_neutral_ambient_stops,
    .count = sizeof(pal_neutral_ambient_stops)/sizeof(pal_neutral_ambient_stops[0]),
};

/* 11. Polar White — холодный бело-голубой */
static const AMB_pal_stop_t pal_polar_white_stops[] = {
    {0.00f, {210, 255, 255}},
    {0.50f, {235, 255, 255}},
    {1.00f, {255, 255, 255}},
};
const AMB_palette_t PAL_MB_POLAR_WHITE = {
    .stops = pal_polar_white_stops,
    .count = sizeof(pal_polar_white_stops)/sizeof(pal_polar_white_stops[0]),
};

/* 12. Energizing Duo — янтарь + фиолет (Energizing Comfort) */
static const AMB_pal_stop_t pal_energizing_duo_stops[] = {
    {0.00f, {185,   0, 255}},
    {0.33f, {255,  70,   0}},
    {0.66f, {255, 150,   0}},
    {1.00f, { 80,   0, 190}},
};
const AMB_palette_t PAL_MB_ENERGIZING_DUO = {
    .stops = pal_energizing_duo_stops,
    .count = sizeof(pal_energizing_duo_stops)/sizeof(pal_energizing_duo_stops[0]),
};

/* 13. Miami — бирюза + розовый (типичный "wow"-режим) */
static const AMB_pal_stop_t pal_miami_stops[] = {
    {0.00f, {  0, 220, 200}},
    {0.40f, {  0, 255, 230}},
    {0.70f, {255,  80, 180}},
    {1.00f, {255, 120, 220}},
};
const AMB_palette_t PAL_MB_MIAMI = {
    .stops = pal_miami_stops,
    .count = sizeof(pal_miami_stops)/sizeof(pal_miami_stops[0]),
};

/* 14. Sky Night — ночной сине-фиолетовый (EQS Night) */
static const AMB_pal_stop_t pal_sky_night_stops[] = {
    {0.00f, {  0,  30, 255}},
    {0.35f, { 60,   0, 210}},
    {0.70f, {130,   0, 190}},
    {1.00f, {200,   0, 255}},
};
const AMB_palette_t PAL_MB_SKY_NIGHT = {
    .stops = pal_sky_night_stops,
    .count = sizeof(pal_sky_night_stops)/sizeof(pal_sky_night_stops[0]),
};

/* Таблица для ws_palette_get */

static const AMB_palette_t* const G_PALS[PAL_MAX_] = {
    [PAL_SUNSET_AMBER]    = &PAL_MB_SUNSET_AMBER,
    [PAL_FIRE_RED]        = &PAL_MB_FIRE_RED,
    [PAL_RED_MOON]        = &PAL_MB_RED_MOON,
    [PAL_WARM_LOUNGE]     = &PAL_MB_WARM_LOUNGE,
    [PAL_OCEAN_BLUE]      = &PAL_MB_OCEAN_BLUE,
    [PAL_DAWN_BLUE]       = &PAL_MB_DAWN_BLUE,
    [PAL_PURPLE_SKY]      = &PAL_MB_PURPLE_SKY,
    [PAL_JUNGLE_GREEN]    = &PAL_MB_JUNGLE_GREEN,
    [PAL_GLACIER]         = &PAL_MB_GLACIER,
    [PAL_NEUTRAL_AMBIENT] = &PAL_MB_NEUTRAL_AMBIENT,
    [PAL_POLAR_WHITE]     = &PAL_MB_POLAR_WHITE,
    [PAL_ENERGIZING_DUO]  = &PAL_MB_ENERGIZING_DUO,
    [PAL_MIAMI]           = &PAL_MB_MIAMI,
    [PAL_SKY_NIGHT]       = &PAL_MB_SKY_NIGHT,
};

const AMB_palette_t* palette_get(AMB_palette_id_t id)
{
    if ((int)id < 0 || id >= PAL_MAX_) {
        return NULL;
    }
    return G_PALS[id];
}
