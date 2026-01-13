#include "features.h"
#include "palette.h"
#include <math.h>
#include "stm32g4xx_hal.h"   // для HAL_GetTick()


/* Simple MB-inspired palettes (very rough approximations) */

/* Sunset Amber – мягкий янтарный закат (премиум версия) */
static const ws_pal_stop_t PAL_SUNSET_AMBER[] = {
    { 0.00f, 240, 130,  30 },  // мягкий тёплый янтарь
    { 0.40f, 250, 140,  50 },  // светлее, более золотистый
    { 0.75f, 255, 160,  60 },  // мягкое золото
    { 1.00f, 255, 180,  80 },  // светлый янтарный
};

/* Ocean Blue – мягкий океанский синий (премиум версия) */
static const ws_pal_stop_t PAL_OCEAN_BLUE[] = {
    { 0.00f,  20,  60, 240 },  // глубокий, но не чёрный
    { 0.40f,  40, 100, 250 },  // мягкий синий
    { 0.75f,  60, 140, 255 },  // светлый океанский
    { 1.00f,  80, 170, 255 },  // небесно-голубой
};

/* Red Moon – мягкий красный закат (премиум версия) */
static const ws_pal_stop_t PAL_RED_MOON[] = {
    { 0.00f, 220,  60,  50 },  // мягкий тёплый красный
    { 0.40f, 240,  80,  70 },  // более светлый
    { 0.75f, 250, 100,  90 },  // розово-красный
    { 1.00f, 255, 120, 110 },  // светлый коралловый
};

/* Purple Silk – мягкий фиолетовый шёлк (премиум версия) */
static const ws_pal_stop_t PAL_PURPLE_SILK[] = {
    { 0.00f, 100,  40, 200 },  // глубокий фиолетовый с мягкостью
    { 0.40f, 140,  60, 230 },  // более светлый
    { 0.75f, 180,  90, 250 },  // яркий, но мягкий
    { 1.00f, 220, 130, 255 },  // светлый лавандовый
};

/* Glacier – ледяной голубой (премиум версия) */
static const ws_pal_stop_t PAL_GLACIER[] = {
    { 0.00f, 210, 235, 255 },  // мягкий холодный белый
    { 0.40f, 220, 245, 255 },  // светлее
    { 0.75f, 230, 250, 255 },  // очень светлый голубой
    { 1.00f, 240, 252, 255 },  // почти белый с голубым
};

/* Energize – мягкая радужная палитра (премиум версия) */
static const ws_pal_stop_t PAL_ENERGIZE[] = {
    { 0.00f, 220,  80,  80 },  // мягкий красный
    { 0.20f, 250, 150,  60 },  // тёплый оранжевый
    { 0.40f, 240, 200,  80 },  // мягкий жёлтый
    { 0.60f, 120, 220, 140 },  // мягкий зелёный
    { 0.80f,  80, 160, 240 },  // мягкий синий
    { 1.00f, 180, 100, 230 },  // мягкий фиолетовый
};

/* Polar White – полярный белый (премиум версия) */
static const ws_pal_stop_t PAL_POLAR_WHITE[] = {
    { 0.00f, 250, 250, 255 },  // мягкий белый с лёгким голубым
    { 0.50f, 255, 255, 255 },  // чистый белый
    { 1.00f, 245, 245, 250 },  // мягкий белый
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

/* Diamond White – бриллиантово-белый с холодными оттенками (W223/EQS стиль) */
static const ws_pal_stop_t PAL_DIAMOND_WHITE[] = {
    { 0.00f, 245, 250, 255 },  // холодный белый с голубым оттенком
    { 0.35f, 255, 255, 255 },  // чистый белый
    { 0.70f, 250, 248, 255 },  // слегка фиолетовый оттенок
    { 1.00f, 240, 245, 255 },  // мягкий голубовато-белый
};

/* Silver Mist – серебряный туман, нейтральный металлик */
static const ws_pal_stop_t PAL_SILVER_MIST[] = {
    { 0.00f, 180, 185, 190 },  // тёмное серебро
    { 0.40f, 220, 225, 230 },  // среднее серебро
    { 0.75f, 240, 245, 250 },  // светлое серебро
    { 1.00f, 255, 255, 255 },  // почти белый металлик
};

/* Platinum – платина, элегантный серый с теплым оттенком */
static const ws_pal_stop_t PAL_PLATINUM[] = {
    { 0.00f, 200, 200, 205 },  // тёплый серый
    { 0.40f, 230, 230, 235 },  // светлый платиновый
    { 0.75f, 245, 245, 248 },  // почти белый с теплом
    { 1.00f, 255, 252, 248 },  // платиново-белый с легким кремом
};

/* Amber Gold – янтарное золото, более насыщенное чем COPPER_GOLD */
static const ws_pal_stop_t PAL_AMBER_GOLD[] = {
    { 0.00f, 180, 100,  20 },  // глубокое янтарное золото
    { 0.35f, 220, 140,  40 },  // ярче
    { 0.70f, 255, 180,  60 },  // насыщенное золото
    { 1.00f, 255, 220, 120 },  // светлое янтарное золото
};

/* Magenta Royal – королевский пурпур, глубокий и насыщенный */
static const ws_pal_stop_t PAL_MAGENTA_ROYAL[] = {
    { 0.00f,  80,   0,  80 },  // глубокий фиолетовый
    { 0.35f, 140,  20, 140 },  // насыщенный пурпур
    { 0.70f, 200,  60, 200 },  // яркий королевский пурпур
    { 1.00f, 255, 120, 255 },  // светлый маджента-розовый
};

/* =========================================================================
 * W223/EQS PREMIUM PALETTES
 * ========================================================================= */

/* Aurora Borealis – северное сияние с магическими переливами
 * Вдохновлено реальным северным сиянием: зелёный -> голубой -> фиолетовый
 */
static const ws_pal_stop_t PAL_AURORA_BOREALIS[] = {
    { 0.00f,  20, 180,  80 },  // мягкий изумрудный
    { 0.20f,  40, 220, 140 },  // яркий аврора-зелёный
    { 0.40f,  60, 200, 200 },  // бирюзовый переход
    { 0.60f,  80, 160, 240 },  // небесно-голубой
    { 0.80f, 140, 100, 220 },  // лавандово-синий
    { 1.00f, 180,  60, 200 },  // мягкий фиолетовый
};

/* Champagne Gold – тёплый перламутровый золотистый
 * Элегантный шампань с розовыми переливами, как пузырьки в бокале
 */
static const ws_pal_stop_t PAL_CHAMPAGNE_GOLD[] = {
    { 0.00f, 255, 220, 180 },  // светлый шампань
    { 0.25f, 255, 200, 150 },  // золотистый шампань
    { 0.50f, 250, 190, 160 },  // розовый оттенок
    { 0.75f, 255, 210, 170 },  // перламутровый
    { 1.00f, 255, 230, 200 },  // кремовый шампань
};

/* Deep Burgundy – глубокий бордо с винным оттенком
 * Насыщенный винный цвет для роскошного ощущения
 */
static const ws_pal_stop_t PAL_DEEP_BURGUNDY[] = {
    { 0.00f,  80,  10,  20 },  // глубокий бордо
    { 0.30f, 140,  25,  40 },  // насыщенный винный
    { 0.60f, 180,  40,  60 },  // светлый бордо
    { 0.85f, 200,  60,  80 },  // розово-винный
    { 1.00f, 220,  80, 100 },  // мягкий коралловый акцент
};

/* Emerald Forest – изумрудный лес с тёплыми акцентами
 * Глубокий зелёный с золотистыми отблесками
 */
static const ws_pal_stop_t PAL_EMERALD_FOREST[] = {
    { 0.00f,  10,  80,  40 },  // глубокий лесной
    { 0.25f,  30, 140,  60 },  // насыщенный изумруд
    { 0.50f,  50, 180,  80 },  // яркий изумрудный
    { 0.75f,  80, 160,  70 },  // с золотистым оттенком
    { 1.00f, 120, 180, 100 },  // мягкий лаймово-золотой
};

/* Ice Sapphire – ледяной сапфировый синий
 * Холодный, кристально чистый синий с ледяными акцентами
 */
static const ws_pal_stop_t PAL_ICE_SAPPHIRE[] = {
    { 0.00f,  20,  60, 180 },  // глубокий сапфир
    { 0.25f,  40, 100, 220 },  // насыщенный синий
    { 0.50f,  80, 150, 255 },  // яркий сапфировый
    { 0.75f, 140, 200, 255 },  // ледяной голубой
    { 1.00f, 200, 230, 255 },  // кристальный белый-голубой
};

static const ws_palette_t G_PALS[WSPAL_MAX_] = {
    /* Classic Mercedes palettes */
    [WSPAL_MB_SUNSET_AMBER]  = { PAL_SUNSET_AMBER,    (uint8_t)(sizeof(PAL_SUNSET_AMBER)/sizeof(PAL_SUNSET_AMBER[0])) },
    [WSPAL_MB_OCEAN_BLUE]    = { PAL_OCEAN_BLUE,      (uint8_t)(sizeof(PAL_OCEAN_BLUE)/sizeof(PAL_OCEAN_BLUE[0])) },
    [WSPAL_MB_RED_MOON]      = { PAL_RED_MOON,        (uint8_t)(sizeof(PAL_RED_MOON)/sizeof(PAL_RED_MOON[0])) },
    [WSPAL_MB_PURPLE_SILK]   = { PAL_PURPLE_SILK,     (uint8_t)(sizeof(PAL_PURPLE_SILK)/sizeof(PAL_PURPLE_SILK[0])) },
    [WSPAL_MB_GLACIER]       = { PAL_GLACIER,         (uint8_t)(sizeof(PAL_GLACIER)/sizeof(PAL_GLACIER[0])) },
    [WSPAL_MB_ENERGIZE]      = { PAL_ENERGIZE,        (uint8_t)(sizeof(PAL_ENERGIZE)/sizeof(PAL_ENERGIZE[0])) },
    [WSPAL_MB_POLAR_WHITE]   = { PAL_POLAR_WHITE,     (uint8_t)(sizeof(PAL_POLAR_WHITE)/sizeof(PAL_POLAR_WHITE[0])) },
    [WSPAL_MB_NIGHT_BLUE]    = { PAL_NIGHT_BLUE,      (uint8_t)(sizeof(PAL_NIGHT_BLUE)/sizeof(PAL_NIGHT_BLUE[0])) },
    [WSPAL_MB_HYACINTH]      = { PAL_HYACINTH,        (uint8_t)(sizeof(PAL_HYACINTH)/sizeof(PAL_HYACINTH[0])) },
    [WSPAL_MB_COPPER_GOLD]   = { PAL_COPPER_GOLD,     (uint8_t)(sizeof(PAL_COPPER_GOLD)/sizeof(PAL_COPPER_GOLD[0])) },
    [WSPAL_MB_ROSE]          = { PAL_ROSE,            (uint8_t)(sizeof(PAL_ROSE)/sizeof(PAL_ROSE[0])) },

    /* Premium palettes */
    [WSPAL_MB_DIAMOND_WHITE] = { PAL_DIAMOND_WHITE,   (uint8_t)(sizeof(PAL_DIAMOND_WHITE)/sizeof(PAL_DIAMOND_WHITE[0])) },
    [WSPAL_MB_SILVER_MIST]   = { PAL_SILVER_MIST,     (uint8_t)(sizeof(PAL_SILVER_MIST)/sizeof(PAL_SILVER_MIST[0])) },
    [WSPAL_MB_PLATINUM]      = { PAL_PLATINUM,        (uint8_t)(sizeof(PAL_PLATINUM)/sizeof(PAL_PLATINUM[0])) },
    [WSPAL_MB_AMBER_GOLD]    = { PAL_AMBER_GOLD,      (uint8_t)(sizeof(PAL_AMBER_GOLD)/sizeof(PAL_AMBER_GOLD[0])) },
    [WSPAL_MB_MAGENTA_ROYAL] = { PAL_MAGENTA_ROYAL,   (uint8_t)(sizeof(PAL_MAGENTA_ROYAL)/sizeof(PAL_MAGENTA_ROYAL[0])) },

    /* W223/EQS Premium palettes */
    [WSPAL_AURORA_BOREALIS]  = { PAL_AURORA_BOREALIS, (uint8_t)(sizeof(PAL_AURORA_BOREALIS)/sizeof(PAL_AURORA_BOREALIS[0])) },
    [WSPAL_CHAMPAGNE_GOLD]   = { PAL_CHAMPAGNE_GOLD,  (uint8_t)(sizeof(PAL_CHAMPAGNE_GOLD)/sizeof(PAL_CHAMPAGNE_GOLD[0])) },
    [WSPAL_DEEP_BURGUNDY]    = { PAL_DEEP_BURGUNDY,   (uint8_t)(sizeof(PAL_DEEP_BURGUNDY)/sizeof(PAL_DEEP_BURGUNDY[0])) },
    [WSPAL_EMERALD_FOREST]   = { PAL_EMERALD_FOREST,  (uint8_t)(sizeof(PAL_EMERALD_FOREST)/sizeof(PAL_EMERALD_FOREST[0])) },
    [WSPAL_ICE_SAPPHIRE]     = { PAL_ICE_SAPPHIRE,    (uint8_t)(sizeof(PAL_ICE_SAPPHIRE)/sizeof(PAL_ICE_SAPPHIRE[0])) },
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
