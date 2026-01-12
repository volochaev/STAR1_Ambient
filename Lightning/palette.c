#include "features.h"
#include "palette.h"
#include <math.h>
#include "stm32g4xx_hal.h"   // для HAL_GetTick()


/* Simple MB-inspired palettes (very rough approximations) */

static const ws_pal_stop_t PAL_SUNSET_AMBER[] = {
    { 0.00f, 255, 120,  10 },
    { 0.50f, 255,  80,  20 },
    { 1.00f, 255,  40,  10 },
};

static const ws_pal_stop_t PAL_OCEAN_BLUE[] = {
    { 0.00f,   0,  40, 255 },
    { 0.50f,   0, 100, 255 },
    { 1.00f,   0, 180, 255 },
};

static const ws_pal_stop_t PAL_RED_MOON[] = {
    { 0.00f, 255,  20,  20 },
    { 0.50f, 255,  10,  60 },
    { 1.00f, 255,   0,   0 },
};

static const ws_pal_stop_t PAL_PURPLE_SILK[] = {
    { 0.00f, 120,   0, 255 },
    { 0.50f, 180,  40, 255 },
    { 1.00f, 255,  80, 255 },
};

static const ws_pal_stop_t PAL_GLACIER[] = {
    { 0.00f, 200, 230, 255 },
    { 0.50f, 180, 255, 255 },
    { 1.00f, 140, 220, 255 },
};

static const ws_pal_stop_t PAL_ENERGIZE[] = {
    { 0.00f, 255,  40,  40 },
    { 0.25f, 255, 180,   0 },
    { 0.50f,  40, 255,  40 },
    { 0.75f,   0, 180, 255 },
    { 1.00f, 200,  40, 255 },
};

static const ws_pal_stop_t PAL_POLAR_WHITE[] = {
    { 0.00f, 255, 255, 255 },
    { 1.00f, 240, 240, 255 },
};

/* Глубокий ночной синий: от почти чёрно-синего к яркому небесно-синему */
static const ws_pal_stop_t PAL_NIGHT_BLUE[] = {
    { 0.00f,  5,  10,  40 },  // почти чернильный
    { 0.40f,  5,  30, 100 },  // глубокий синий
    { 0.75f,  0,  80, 180 },  // насыщенный ночной
    { 1.00f,  0, 140, 255 },  // акцент ярко-голубой
};

/* Hyacinth – фирменный фиолетово-розовый вечерний */
static const ws_pal_stop_t PAL_HYACINTH[] = {
    { 0.00f, 110,  10, 120 },  // глубокий фиолетовый
    { 0.35f, 160,  30, 180 },  // ярче
    { 0.65f, 210,  60, 200 },  // фиолетово-розовый
    { 1.00f, 255, 120, 210 },  // мягкий розовый свет
};

/* Copper Gold – тёплый медно-золотой; ближе к интерьерам "Exclusive" */
static const ws_pal_stop_t PAL_COPPER_GOLD[] = {
    { 0.00f, 120,  40,   5 },  // тёмная медь
    { 0.30f, 180,  70,  10 },  // ярче
    { 0.60f, 230, 110,  15 },  // медно-золотой
    { 1.00f, 255, 180,  40 },  // золото/шампань
};

/* Rose – мягкий розово-золотой "лаунж" */
static const ws_pal_stop_t PAL_ROSE[] = {
    { 0.00f, 160,  40,  40 },  // тёплый розовый
    { 0.40f, 210,  70,  70 },  // светлее
    { 0.75f, 245, 110,  90 },  // розово-персиковый
    { 1.00f, 255, 170, 120 },  // розово-золотой
};

static const ws_palette_t G_PALS[WSPAL_MAX_] = {
    [WSPAL_MB_SUNSET_AMBER] = { PAL_SUNSET_AMBER,  (uint8_t)(sizeof(PAL_SUNSET_AMBER)/sizeof(PAL_SUNSET_AMBER[0])) },
    [WSPAL_MB_OCEAN_BLUE]   = { PAL_OCEAN_BLUE,    (uint8_t)(sizeof(PAL_OCEAN_BLUE)/sizeof(PAL_OCEAN_BLUE[0])) },
    [WSPAL_MB_RED_MOON]     = { PAL_RED_MOON,      (uint8_t)(sizeof(PAL_RED_MOON)/sizeof(PAL_RED_MOON[0])) },
    [WSPAL_MB_PURPLE_SILK]  = { PAL_PURPLE_SILK,   (uint8_t)(sizeof(PAL_PURPLE_SILK)/sizeof(PAL_PURPLE_SILK[0])) },
    [WSPAL_MB_GLACIER]      = { PAL_GLACIER,       (uint8_t)(sizeof(PAL_GLACIER)/sizeof(PAL_GLACIER[0])) },
    [WSPAL_MB_ENERGIZE]     = { PAL_ENERGIZE,      (uint8_t)(sizeof(PAL_ENERGIZE)/sizeof(PAL_ENERGIZE[0])) },
    [WSPAL_MB_POLAR_WHITE]  = { PAL_POLAR_WHITE,   (uint8_t)(sizeof(PAL_POLAR_WHITE)/sizeof(PAL_POLAR_WHITE[0])) },
    [WSPAL_MB_NIGHT_BLUE] 	= { PAL_NIGHT_BLUE,	   (uint8_t)(sizeof(PAL_NIGHT_BLUE) / sizeof(PAL_NIGHT_BLUE[0])) },
    [WSPAL_MB_HYACINTH] 	= { PAL_HYACINTH,	   (uint8_t)(sizeof(PAL_HYACINTH) / sizeof(PAL_HYACINTH[0])) },
    [WSPAL_MB_COPPER_GOLD] 	= { PAL_COPPER_GOLD,   (uint8_t)(sizeof(PAL_COPPER_GOLD) / sizeof(PAL_COPPER_GOLD[0])) },
    [WSPAL_MB_ROSE] 		= { PAL_ROSE, 		   (uint8_t)(sizeof(PAL_ROSE) / sizeof(PAL_ROSE[0])) },
};

const ws_palette_t* ws_palette_get(ws_palette_id_t id)
{
    if (id < 0 || id >= WSPAL_MAX_) return 0;
    return &G_PALS[id];
}

void ws_palette_sample_rgb8(const ws_palette_t *pal, float u,
                            uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r) *r = 0;
    if (g) *g = 0;
    if (b) *b = 0;
    if (!pal || pal->count == 0) return;

    /* Нормализуем u в 0..1 (поддержим "wrap-around") */
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

    /* Линейная интерполяция в "сырых" RGB */
    float rf = (1.0f - t) * (float)s0->r + t * (float)s1->r;
    float gf = (1.0f - t) * (float)s0->g + t * (float)s1->g;
    float bf = (1.0f - t) * (float)s0->b + t * (float)s1->b;

#if AMB_ENABLE_GAMMA
    /* Нормализуем к 0..1 и применяем gamma, затем обратно к 0..255.
     * Это делает тёмные оттенки более "богатыми".
     */
    const float inv255 = 1.0f / 255.0f;
    float gexp = AMB_GAMMA_EXP;

    rf = powf(rf * inv255, gexp) * 255.0f;
    gf = powf(gf * inv255, gexp) * 255.0f;
    bf = powf(bf * inv255, gexp) * 255.0f;
#endif

    /* Обрезаем к диапазону */
    if (rf < 0.0f) rf = 0.0f; if (rf > 255.0f) rf = 255.0f;
    if (gf < 0.0f) gf = 0.0f; if (gf > 255.0f) gf = 255.0f;
    if (bf < 0.0f) bf = 0.0f; if (bf > 255.0f) bf = 255.0f;

    uint8_t R = (uint8_t)(rf + 0.5f);
    uint8_t G = (uint8_t)(gf + 0.5f);
    uint8_t B = (uint8_t)(bf + 0.5f);

#if AMB_ENABLE_DITHERING
    /* Простейший временной дезеринг по LSB:
     * добавляем +1 к части пикселей, чтобы "имитировать" промежутки между значениями.
     */
    uint32_t t_ms  = HAL_GetTick();
    uint32_t phase = (t_ms >> 1);  // менять каждые ~2 мс

    /* Небольшой pseudo-random из u + времени */
    uint32_t seed = (uint32_t)(u * 10000.0f) ^ phase;

    if (R > 0 && R < 255 && (seed & 0x01)) R++;
    if (G > 0 && G < 255 && (seed & 0x02)) G++;
    if (B > 0 && B < 255 && (seed & 0x04)) B++;
#endif

    if (r) *r = R;
    if (g) *g = G;
    if (b) *b = B;
}
