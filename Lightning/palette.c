#include <math.h>
#include "palette.h"
#include "features.h"
#include "main.h"

/* Палитры в премиум-стиле (W223/EQS). Максимум плавности и сложные оттенки. */

static const ws_pal_stop_t PAL_CHAMPAGNE_ARC[] = {
    {0.00f, 205, 148,  82},
    {0.28f, 225, 170, 110},
    {0.55f, 242, 192, 140},
    {0.82f, 254, 210, 170},
    {1.00f, 255, 222, 190},
};

static const ws_pal_stop_t PAL_AMBER_SUNSET[] = {
    {0.00f, 188, 110,  24},
    {0.25f, 210, 128,  38},
    {0.50f, 234, 156,  68},
    {0.75f, 255, 186, 108},
    {1.00f, 255, 204, 140},
};

static const ws_pal_stop_t PAL_COPPER_ROSE[] = {
    {0.00f, 150,  70,  34},
    {0.25f, 176,  90,  56},
    {0.55f, 206, 122,  90},
    {0.78f, 228, 150, 116},
    {1.00f, 242, 172, 136},
};

static const ws_pal_stop_t PAL_BURGUNDY_VELVET[] = {
    {0.00f,  92,  10,  12},
    {0.22f, 128,  22,  24},
    {0.48f, 168,  36,  42},
    {0.72f, 198,  54,  62},
    {1.00f, 225,  74,  80},
};

static const ws_pal_stop_t PAL_EMERALD_VEIL[] = {
    {0.00f,  12,  72,  52},
    {0.22f,  22, 110,  88},
    {0.50f,  44, 150, 132},
    {0.78f,  70, 185, 170},
    {1.00f, 110, 195, 175},
};

static const ws_pal_stop_t PAL_AURORA_GLACIER[] = {
    {0.00f,  12,  96,  78},
    {0.24f,  36, 148, 140},
    {0.52f,  72, 198, 214},
    {0.78f,  98, 170, 240},
    {1.00f,  72, 130, 236},
};

static const ws_pal_stop_t PAL_SAPPHIRE_ICE[] = {
    {0.00f,  10,  58, 140},
    {0.30f,  26,  96, 182},
    {0.55f,  60, 150, 224},
    {0.78f,  90, 196, 255},
    {1.00f, 150, 228, 255},
};

static const ws_pal_stop_t PAL_NIGHT_OPAL[] = {
    {0.00f,   6,  10,  36},
    {0.25f,  16,  20,  72},
    {0.50f,  30,  40, 126},
    {0.75f,  52,  62, 176},
    {1.00f,  82, 102, 206},
};

static const ws_pal_stop_t PAL_GLACIER_MIST[] = {
    {0.00f, 182, 196, 220},
    {0.24f, 200, 214, 236},
    {0.52f, 220, 234, 250},
    {0.78f, 238, 246, 255},
    {1.00f, 214, 232, 252},
};

static const ws_pal_stop_t PAL_SILVER_SILK[] = {
    {0.00f, 132, 136, 144},
    {0.22f, 154, 160, 170},
    {0.50f, 182, 192, 204},
    {0.76f, 205, 214, 224},
    {1.00f, 186, 198, 214},
};

static const ws_pal_stop_t PAL_PLATINUM_CLOUD[] = {
    {0.00f, 198, 208, 224},
    {0.26f, 214, 224, 238},
    {0.54f, 234, 244, 252},
    {0.80f, 246, 252, 255},
    {1.00f, 230, 238, 248},
};

static const ws_pal_stop_t PAL_PEARL_BLUSH[] = {
    {0.00f, 216, 202, 210},
    {0.24f, 232, 214, 224},
    {0.52f, 246, 228, 236},
    {0.80f, 252, 242, 248},
    {1.00f, 232, 218, 228},
};

static const ws_palette_t G_PALS[WSPAL_MAX_] = {
    [WSPAL_CHAMPAGNE_ARC]  = { PAL_CHAMPAGNE_ARC,  (uint8_t)(sizeof(PAL_CHAMPAGNE_ARC)/sizeof(PAL_CHAMPAGNE_ARC[0])) },
    [WSPAL_AMBER_SUNSET]   = { PAL_AMBER_SUNSET,   (uint8_t)(sizeof(PAL_AMBER_SUNSET)/sizeof(PAL_AMBER_SUNSET[0])) },
    [WSPAL_COPPER_ROSE]    = { PAL_COPPER_ROSE,    (uint8_t)(sizeof(PAL_COPPER_ROSE)/sizeof(PAL_COPPER_ROSE[0])) },
    [WSPAL_BURGUNDY_VELVET]= { PAL_BURGUNDY_VELVET,(uint8_t)(sizeof(PAL_BURGUNDY_VELVET)/sizeof(PAL_BURGUNDY_VELVET[0])) },
    [WSPAL_EMERALD_VEIL]   = { PAL_EMERALD_VEIL,   (uint8_t)(sizeof(PAL_EMERALD_VEIL)/sizeof(PAL_EMERALD_VEIL[0])) },
    [WSPAL_AURORA_GLACIER] = { PAL_AURORA_GLACIER, (uint8_t)(sizeof(PAL_AURORA_GLACIER)/sizeof(PAL_AURORA_GLACIER[0])) },
    [WSPAL_SAPPHIRE_ICE]   = { PAL_SAPPHIRE_ICE,   (uint8_t)(sizeof(PAL_SAPPHIRE_ICE)/sizeof(PAL_SAPPHIRE_ICE[0])) },
    [WSPAL_NIGHT_OPAL]     = { PAL_NIGHT_OPAL,     (uint8_t)(sizeof(PAL_NIGHT_OPAL)/sizeof(PAL_NIGHT_OPAL[0])) },
    [WSPAL_GLACIER_MIST]   = { PAL_GLACIER_MIST,   (uint8_t)(sizeof(PAL_GLACIER_MIST)/sizeof(PAL_GLACIER_MIST[0])) },
    [WSPAL_SILVER_SILK]    = { PAL_SILVER_SILK,    (uint8_t)(sizeof(PAL_SILVER_SILK)/sizeof(PAL_SILVER_SILK[0])) },
    [WSPAL_PLATINUM_CLOUD] = { PAL_PLATINUM_CLOUD, (uint8_t)(sizeof(PAL_PLATINUM_CLOUD)/sizeof(PAL_PLATINUM_CLOUD[0])) },
    [WSPAL_PEARL_BLUSH]    = { PAL_PEARL_BLUSH,    (uint8_t)(sizeof(PAL_PEARL_BLUSH)/sizeof(PAL_PEARL_BLUSH[0])) },
};

const ws_palette_t* ws_palette_get(ws_palette_id_t id)
{
    if (id < 0 || id >= WSPAL_MAX_) return 0;
    return &G_PALS[id];
}

void ws_palette_sample_rgb8(const ws_palette_t *pal, float u,
                            uint8_t *r, uint8_t *g, uint8_t *b)
{
    static uint32_t s_dither_call_counter = 0u;

    if (r) *r = 0;
    if (g) *g = 0;
    if (b) *b = 0;
    if (!pal || pal->count == 0) return;

    /* Нормализуем u в 0..1 (wrap-around) */
    if (u < 0.0f || u > 1.0f) {
        u = u - floorf(u);
        if (u < 0.0f) u += 1.0f;
    }

    /* Найдём два соседних стопа палитры */
    const ws_pal_stop_t *s0 = &pal->stops[0];
    const ws_pal_stop_t *s1 = &pal->stops[pal->count - 1];

    for (uint8_t i = 1; i < pal->count; ++i) {
        if (u <= pal->stops[i].pos) {
            s1 = &pal->stops[i];
            s0 = &pal->stops[i-1];
            break;
        }
    }

    float span = (s1->pos - s0->pos);
    float t    = span > 0.0f ? (u - s0->pos) / span : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* Линейная интерполяция */
    float rf = (1.0f - t) * (float)s0->r + t * (float)s1->r;
    float gf = (1.0f - t) * (float)s0->g + t * (float)s1->g;
    float bf = (1.0f - t) * (float)s0->b + t * (float)s1->b;

#if AMB_ENABLE_GAMMA
    const float inv255 = 1.0f / 255.0f;
    float gexp = AMB_GAMMA_EXP;
    if (gexp < 0.1f) gexp = 0.1f;
#if (AMB_GAMMA_MODE == AMB_GAMMA_MODE_LINEAR)
    {
        float inv_gamma = 1.0f / gexp;
        rf = powf(rf * inv255, inv_gamma) * 255.0f;
        gf = powf(gf * inv255, inv_gamma) * 255.0f;
        bf = powf(bf * inv255, inv_gamma) * 255.0f;
    }
#else
    rf = powf(rf * inv255, gexp) * 255.0f;
    gf = powf(gf * inv255, gexp) * 255.0f;
    bf = powf(bf * inv255, gexp) * 255.0f;
#endif
#endif

    /* Optional saturation lift after gamma to keep colors punchy on real strips. */
    {
        float sat = AMB_SATURATION_BOOST;
        if (sat > 0.0f && fabsf(sat - 1.0f) > 0.001f) {
            float luma = 0.2126f * rf + 0.7152f * gf + 0.0722f * bf;
            rf = luma + (rf - luma) * sat;
            gf = luma + (gf - luma) * sat;
            bf = luma + (bf - luma) * sat;
        }
    }

    /* Обрезаем к диапазону */
    if (rf < 0.0f) rf = 0.0f;
    if (rf > 255.0f) rf = 255.0f;
    if (gf < 0.0f) gf = 0.0f;
    if (gf > 255.0f) gf = 255.0f;
    if (bf < 0.0f) bf = 0.0f;
    if (bf > 255.0f) bf = 255.0f;

    uint8_t R = (uint8_t)(rf + 0.5f);
    uint8_t G = (uint8_t)(gf + 0.5f);
    uint8_t B = (uint8_t)(bf + 0.5f);

#if AMB_ENABLE_DITHERING
    uint32_t t_ms  = HAL_GetTick();
    uint32_t phase = (t_ms >> 1);
    uint32_t seed = (uint32_t)(u * 10000.0f) ^ phase ^ (s_dither_call_counter * 2654435761u);
    if (R > 0 && R < 255 && (seed & 0x01)) R++;
    if (G > 0 && G < 255 && (seed & 0x02)) G++;
    if (B > 0 && B < 255 && (seed & 0x04)) B++;
#endif

    s_dither_call_counter++;

    if (r) *r = R;
    if (g) *g = G;
    if (b) *b = B;
}
