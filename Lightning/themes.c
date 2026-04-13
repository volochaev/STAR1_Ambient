#include <stddef.h>
#include "themes.h"
#include "motion_profiles.h"

/* Премиальные темы, собранные вокруг трёх банков OEM (Amber/Blue/White). */

static const theme_definition_t G_THEME_DEFINITIONS[THEME_MAX_] = {
    /* ================= AMBER ================= */
    [THEME_AMBER_CHAMPAGNE_ARC] = {
        .fx_main = FX_VELVET_FLOW,
        .pal_main = WSPAL_CHAMPAGNE_ARC,
        .theme_brightness = 0.76f,
        .zone = {
            [ZONE_STRIP]    = { FX_VELVET_FLOW,    WSPAL_CHAMPAGNE_ARC, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_CHAMPAGNE_ARC, 0.42f, 0.46f },
            [ZONE_STORAGE]  = { FX_GRADIENT_FLOW,  WSPAL_CHAMPAGNE_ARC, 0.30f, 0.44f },
            [ZONE_FOOTWELL] = { FX_SOLID_GRADIENT, WSPAL_CHAMPAGNE_ARC, 0.36f, 0.52f },
        },
    },
    [THEME_AMBER_SUNSET] = {
        .fx_main = FX_TWO_TONE_WAVE,
        .pal_main = WSPAL_AMBER_SUNSET,
        .theme_brightness = 0.74f,
        .zone = {
            [ZONE_STRIP]    = { FX_TWO_TONE_WAVE,  WSPAL_AMBER_SUNSET, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_AMBER_SUNSET, 0.40f, 0.38f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_AMBER_SUNSET, 0.28f, 0.46f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_AMBER_SUNSET, 0.33f, 0.54f },
        },
    },
    [THEME_AMBER_COPPER_ROSE] = {
        .fx_main = FX_CASCADE,
        .pal_main = WSPAL_COPPER_ROSE,
        .theme_brightness = 0.74f,
        .zone = {
            [ZONE_STRIP]    = { FX_CASCADE,        WSPAL_COPPER_ROSE, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_COPPER_ROSE, 0.40f, 0.48f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_COPPER_ROSE, 0.28f, 0.52f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_COPPER_ROSE, 0.34f, 0.60f },
        },
    },
    [THEME_AMBER_BURGUNDY_VELVET] = {
        .fx_main = FX_COLOR_MORPH,
        .pal_main = WSPAL_BURGUNDY_VELVET,
        .theme_brightness = 0.72f,
        .zone = {
            [ZONE_STRIP]    = { FX_COLOR_MORPH,    WSPAL_BURGUNDY_VELVET, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_BURGUNDY_VELVET, 0.38f, 0.56f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_BURGUNDY_VELVET, 0.26f, 0.60f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_BURGUNDY_VELVET, 0.32f, 0.66f },
        },
    },

    /* ================= BLUE ================= */
    [THEME_BLUE_AURORA_GLACIER] = {
        .fx_main = FX_AURORA,
        .pal_main = WSPAL_AURORA_GLACIER,
        .theme_brightness = 0.74f,
        .zone = {
            [ZONE_STRIP]    = { FX_AURORA,         WSPAL_AURORA_GLACIER, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_AURORA_GLACIER, 0.44f, 0.58f },
            [ZONE_STORAGE]  = { FX_GRADIENT_FLOW,  WSPAL_AURORA_GLACIER, 0.30f, 0.52f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_AURORA_GLACIER, 0.36f, 0.62f },
        },
    },
    [THEME_BLUE_SAPPHIRE_ICE] = {
        .fx_main = FX_SPARKLE,
        .pal_main = WSPAL_SAPPHIRE_ICE,
        .theme_brightness = 0.78f,
        .zone = {
            [ZONE_STRIP]    = { FX_SPARKLE,        WSPAL_SAPPHIRE_ICE, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_SAPPHIRE_ICE, 0.46f, 0.64f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_SAPPHIRE_ICE, 0.32f, 0.60f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_SAPPHIRE_ICE, 0.40f, 0.70f },
        },
    },
    [THEME_BLUE_NIGHT_OPAL] = {
        .fx_main = FX_COLOR_MORPH,
        .pal_main = WSPAL_NIGHT_OPAL,
        .theme_brightness = 0.78f,
        .zone = {
            [ZONE_STRIP]    = { FX_COLOR_MORPH,    WSPAL_NIGHT_OPAL, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_NIGHT_OPAL, 0.42f, 0.58f },
            [ZONE_STORAGE]  = { FX_GRADIENT_FLOW,  WSPAL_NIGHT_OPAL, 0.30f, 0.60f },
            [ZONE_FOOTWELL] = { FX_SOLID_GRADIENT, WSPAL_NIGHT_OPAL, 0.36f, 0.62f },
        },
    },
    [THEME_BLUE_EMERALD_STREAM] = {
        .fx_main = FX_CASCADE,
        .pal_main = WSPAL_EMERALD_VEIL,
        .theme_brightness = 0.76f,
        .zone = {
            [ZONE_STRIP]    = { FX_CASCADE,        WSPAL_EMERALD_VEIL, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_EMERALD_VEIL, 0.44f, 0.46f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_EMERALD_VEIL, 0.30f, 0.52f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_EMERALD_VEIL, 0.36f, 0.60f },
        },
    },

    /* ================= WHITE ================= */
    [THEME_WHITE_PLATINUM_CLOUD] = {
        .fx_main = FX_VELVET_FLOW,
        .pal_main = WSPAL_PLATINUM_CLOUD,
        .theme_brightness = 0.72f,
        .zone = {
            [ZONE_STRIP]    = { FX_VELVET_FLOW,    WSPAL_PLATINUM_CLOUD, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_PLATINUM_CLOUD, 0.44f, 0.48f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_PLATINUM_CLOUD, 0.30f, 0.52f },
            [ZONE_FOOTWELL] = { FX_SOLID_GRADIENT, WSPAL_PLATINUM_CLOUD, 0.34f, 0.58f },
        },
    },
    [THEME_WHITE_SILVER_SILK] = {
        .fx_main = FX_VELVET_FLOW,
        .pal_main = WSPAL_SILVER_SILK,
        .theme_brightness = 0.72f,
        .zone = {
            [ZONE_STRIP]    = { FX_VELVET_FLOW,    WSPAL_SILVER_SILK, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_SILVER_SILK, 0.40f, 0.52f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_SILVER_SILK, 0.28f, 0.56f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_SILVER_SILK, 0.32f, 0.62f },
        },
    },
    [THEME_WHITE_PEARL_BLUSH] = {
        .fx_main = FX_GENTLE_PULSE,
        .pal_main = WSPAL_PEARL_BLUSH,
        .theme_brightness = 0.70f,
        .zone = {
            [ZONE_STRIP]    = { FX_GENTLE_PULSE,   WSPAL_PEARL_BLUSH, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_PEARL_BLUSH, 0.46f, 0.54f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_PEARL_BLUSH, 0.30f, 0.58f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_PEARL_BLUSH, 0.34f, 0.62f },
        },
    },
    [THEME_WHITE_GLACIER_MIST] = {
        .fx_main = FX_BREATHE_WAVE,
        .pal_main = WSPAL_GLACIER_MIST,
        .theme_brightness = 0.74f,
        .zone = {
            [ZONE_STRIP]    = { FX_BREATHE_WAVE,   WSPAL_GLACIER_MIST, 1.00f, 0.00f },
            [ZONE_HANDLE]   = { FX_SOFT_BREATHE,   WSPAL_GLACIER_MIST, 0.46f, 0.56f },
            [ZONE_STORAGE]  = { FX_SOLID_GRADIENT, WSPAL_GLACIER_MIST, 0.30f, 0.54f },
            [ZONE_FOOTWELL] = { FX_GRADIENT_FLOW,  WSPAL_GLACIER_MIST, 0.36f, 0.64f },
        },
    },
};

/* Base motion speed per effect (scene layer). */
static const float G_FX_BASE_SPEED[FX_MAX_] = WS_FX_BASE_SPEED_INIT;

/* Motion scale profile per theme (W223/EQS-like dynamics tuning) */
static const float G_THEME_MOTION_SCALE[THEME_MAX_] = WS_THEME_MOTION_SCALE_INIT;
/* Theme personality profile: per-theme speed/depth/phase signature. */
static const float G_THEME_PERSONALITY_SPEED[THEME_MAX_] = WS_THEME_PERSONALITY_SPEED_INIT;
static const float G_THEME_PERSONALITY_DEPTH[THEME_MAX_] = WS_THEME_PERSONALITY_DEPTH_INIT;
static const float G_THEME_PERSONALITY_PHASE[THEME_MAX_] = WS_THEME_PERSONALITY_PHASE_INIT;

const theme_definition_t* theme_get(theme_id_t id)
{
    if (id < 0 || id >= THEME_MAX_) return NULL;
    return &G_THEME_DEFINITIONS[id];
}

float theme_motion_scale(theme_id_t id)
{
    float s;
    if (id >= THEME_MAX_) return 1.0f;
    s = G_THEME_MOTION_SCALE[id];
    if (s <= 0.0f) return 1.0f;
    return s;
}

float fx_base_speed(fx_id_t fx)
{
    float s;
    if (fx < 0 || fx >= FX_MAX_) return 0.01f;
    s = G_FX_BASE_SPEED[fx];
    if (s < 0.0f) return 0.0f;
    return s;
}

float theme_personality_speed(theme_id_t id)
{
    float s;
    if (id >= THEME_MAX_) return 1.0f;
    s = G_THEME_PERSONALITY_SPEED[id];
    if (s < 0.50f) s = 0.50f;
    if (s > 1.60f) s = 1.60f;
    return s;
}

float theme_personality_depth(theme_id_t id)
{
    float d;
    if (id >= THEME_MAX_) return 0.0f;
    d = G_THEME_PERSONALITY_DEPTH[id];
    if (d < -1.0f) d = -1.0f;
    if (d > 1.0f) d = 1.0f;
    return d;
}

float theme_personality_phase(theme_id_t id)
{
    float p;
    if (id >= THEME_MAX_) return 0.0f;
    p = G_THEME_PERSONALITY_PHASE[id];
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    return p;
}

/* Банки тем для OEM цветов */
static const theme_id_t BANK_AMBER[] = {
    THEME_AMBER_CHAMPAGNE_ARC,
    THEME_AMBER_SUNSET,
    THEME_AMBER_COPPER_ROSE,
    THEME_AMBER_BURGUNDY_VELVET,
};
static const theme_id_t BANK_BLUE[] = {
    THEME_BLUE_AURORA_GLACIER,
    THEME_BLUE_SAPPHIRE_ICE,
    THEME_BLUE_NIGHT_OPAL,
    THEME_BLUE_EMERALD_STREAM,
};
static const theme_id_t BANK_WHITE[] = {
    THEME_WHITE_PLATINUM_CLOUD,
    THEME_WHITE_SILVER_SILK,
    THEME_WHITE_PEARL_BLUSH,
    THEME_WHITE_GLACIER_MIST,
};

const theme_bank_t g_theme_banks[OEM_COLOR_MAX] = {
    [OEM_COLOR_AMBER] = { BANK_AMBER, (uint8_t)(sizeof(BANK_AMBER) / sizeof(BANK_AMBER[0])) },
    [OEM_COLOR_BLUE ] = { BANK_BLUE,  (uint8_t)(sizeof(BANK_BLUE)  / sizeof(BANK_BLUE[0]))  },
    [OEM_COLOR_WHITE] = { BANK_WHITE, (uint8_t)(sizeof(BANK_WHITE) / sizeof(BANK_WHITE[0])) },
};

const theme_bank_t* theme_get_bank(oem_color_id_t id)
{
    if (id >= OEM_COLOR_MAX) return NULL;
    return &g_theme_banks[id];
}

theme_id_t theme_bank_next(const theme_bank_t *bank,
                                 theme_id_t          current)
{
    if (!bank || !bank->themes || bank->count == 0)
        return 0;

    /* Найдём current в массиве */
    for (uint8_t i = 0; i < bank->count; ++i) {
        if (bank->themes[i] == current) {
            uint8_t next = (uint8_t)((i + 1) % bank->count);
            return bank->themes[next];
        }
    }

    /* Если не нашли - вернём первый */
    return bank->themes[0];
}

theme_id_t theme_default_for_oem(oem_color_id_t id)
{
    const theme_bank_t *bank = theme_get_bank(id);
    if (!bank || !bank->themes || bank->count == 0)
        return 0;
    return bank->themes[0];
}

uint8_t theme_same_bank(theme_id_t theme_a, theme_id_t theme_b)
{
    for (uint8_t bank = 0; bank < OEM_COLOR_MAX; ++bank) {
        const theme_bank_t *b = &g_theme_banks[bank];
        if (!b || !b->themes || b->count == 0) continue;
        uint8_t found_a = 0, found_b = 0;
        for (uint8_t i = 0; i < b->count; ++i) {
            if (b->themes[i] == theme_a) found_a = 1;
            if (b->themes[i] == theme_b) found_b = 1;
        }
        if (found_a && found_b) return 1;
    }
    return 0;
}
