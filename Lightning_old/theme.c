/*
 * theme.c
 *
 *  Created on: Nov 9, 2025
 *      Author: nv
 */

#include "theme.h"

static const THEME_DESC_T G_THEMES[THEME_MAX_] = {

    // 1. SUNSET AMBER (Solar) — тёплый премиум
    [THEME_SUNSET_AMBER] = {
        .id   = THEME_SUNSET_AMBER,
        .fx_main = FX_FLOW_SOFT,
        .pal_main = PAL_SUNSET_AMBER,
        .default_brightness = 0.35f,
        .zone = {
            [ZONE_STRIP] = { FX_FLOW_SOFT,  PAL_SUNSET_AMBER, 1.00f, 0.00f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_SUNSET_AMBER, 0.70f, 0.45f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_SUNSET_AMBER, 0.25f, 0.35f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_SUNSET_AMBER, 0.20f, 0.30f },
        },
    },

    // 2. FIRE RED — чистый красный, драматичный
    [THEME_FIRE_RED] = {
        .id   = THEME_FIRE_RED,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_FIRE_RED,
        .default_brightness = 0.30f,
        .zone = {
            [ZONE_STRIP] = { FX_SOFT_SOLID,  PAL_FIRE_RED, 1.00f, 0.50f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_FIRE_RED, 0.60f, 0.40f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_FIRE_RED, 0.18f, 0.35f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_FIRE_RED, 0.16f, 0.30f },
        },
    },

    // 3. RED MOON — глубокий рубин, ночной
    [THEME_RED_MOON] = {
        .id   = THEME_RED_MOON,
        .fx_main = FX_FLOW_SOFT,
        .pal_main = PAL_RED_MOON,
        .default_brightness = 0.32f,
        .zone = {
            [ZONE_STRIP] = { FX_FLOW_SOFT,   PAL_RED_MOON, 1.00f, 0.10f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_RED_MOON, 0.65f, 0.35f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_RED_MOON, 0.20f, 0.30f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_RED_MOON, 0.18f, 0.25f },
        },
    },

    // 4. WARM LOUNGE — мягкий янтарный премиум
    [THEME_WARM_LOUNGE] = {
        .id   = THEME_WARM_LOUNGE,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_WARM_LOUNGE,
        .default_brightness = 0.28f,
        .zone = {
            [ZONE_STRIP] = { FX_SOFT_SOLID,  PAL_WARM_LOUNGE, 1.00f, 0.40f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_WARM_LOUNGE, 0.70f, 0.50f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_WARM_LOUNGE, 0.25f, 0.35f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_WARM_LOUNGE, 0.22f, 0.30f },
        },
    },

    // 5. OCEAN BLUE — классический MB голубой
    [THEME_OCEAN_BLUE] = {
        .id   = THEME_OCEAN_BLUE,
        .fx_main = FX_FLOW_SOFT,
        .pal_main = PAL_OCEAN_BLUE,
        .default_brightness = 0.40f,
        .zone = {
            [ZONE_STRIP] = { FX_FLOW_SOFT,   PAL_OCEAN_BLUE, 1.00f, 0.10f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_OCEAN_BLUE, 0.75f, 0.40f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_OCEAN_BLUE, 0.28f, 0.30f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_OCEAN_BLUE, 0.24f, 0.25f },
        },
    },

    // 6. DAWN BLUE — нежный холодный/розоватый
    [THEME_DAWN_BLUE] = {
        .id   = THEME_DAWN_BLUE,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_DAWN_BLUE,
        .default_brightness = 0.32f,
        .zone = {
            [ZONE_STRIP] = { FX_SOFT_SOLID,  PAL_DAWN_BLUE, 1.00f, 0.60f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_DAWN_BLUE, 0.65f, 0.75f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_DAWN_BLUE, 0.24f, 0.50f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_DAWN_BLUE, 0.20f, 0.45f },
        },
    },

    // 7. PURPLE SKY — фиолетовый ночной
    [THEME_PURPLE_SKY] = {
        .id   = THEME_PURPLE_SKY,
        .fx_main = FX_FLOW_SOFT,
        .pal_main = PAL_PURPLE_SKY,
        .default_brightness = 0.35f,
        .zone = {
            [ZONE_STRIP] = { FX_FLOW_SOFT,   PAL_PURPLE_SKY, 1.00f, 0.20f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_PURPLE_SKY, 0.70f, 0.50f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_PURPLE_SKY, 0.22f, 0.40f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_PURPLE_SKY, 0.18f, 0.35f },
        },
    },

    // 8. JUNGLE GREEN — бирюзово-зелёный
    [THEME_JUNGLE_GREEN] = {
        .id   = THEME_JUNGLE_GREEN,
        .fx_main = FX_FLOW_SOFT,
        .pal_main = PAL_JUNGLE_GREEN,
        .default_brightness = 0.35f,
        .zone = {
            [ZONE_STRIP] = { FX_FLOW_SOFT,   PAL_JUNGLE_GREEN, 1.00f, 0.15f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_JUNGLE_GREEN, 0.70f, 0.35f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_JUNGLE_GREEN, 0.24f, 0.30f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_JUNGLE_GREEN, 0.20f, 0.25f },
        },
    },

    // 9. GLACIER — ледяной polar/ice
    [THEME_GLACIER] = {
        .id   = THEME_GLACIER,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_GLACIER,
        .default_brightness = 0.34f,
        .zone = {
            [ZONE_STRIP] = { FX_SOFT_SOLID,  PAL_GLACIER, 1.00f, 0.40f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_GLACIER, 0.75f, 0.55f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_GLACIER, 0.26f, 0.35f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_GLACIER, 0.22f, 0.30f },
        },
    },

    // 10. NEUTRAL AMBIENT — мягкий белый
    [THEME_NEUTRAL_AMBIENT] = {
        .id   = THEME_NEUTRAL_AMBIENT,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_NEUTRAL_AMBIENT,
        .default_brightness = 0.26f,
        .zone = {
            [ZONE_STRIP] = { FX_SOFT_SOLID,  PAL_NEUTRAL_AMBIENT, 1.00f, 0.50f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_NEUTRAL_AMBIENT, 0.70f, 0.50f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_NEUTRAL_AMBIENT, 0.22f, 0.50f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_NEUTRAL_AMBIENT, 0.20f, 0.45f },
        },
    },

    // 11. POLAR WHITE — холодный чистый
    [THEME_POLAR_WHITE] = {
        .id   = THEME_POLAR_WHITE,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_POLAR_WHITE,
        .default_brightness = 0.30f,
        .zone = {
            [ZONE_STRIP] = { FX_SOFT_SOLID,  PAL_POLAR_WHITE, 1.00f, 0.60f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_POLAR_WHITE, 0.75f, 0.60f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_POLAR_WHITE, 0.24f, 0.55f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_POLAR_WHITE, 0.22f, 0.50f },
        },
    },

    // 12. ENERGIZING_DUO — янтарь + фиолет, динамично
    [THEME_ENERGIZING_DUO] = {
        .id   = THEME_ENERGIZING_DUO,
        .fx_main = FX_DUO_FLOW,
        .pal_main = PAL_ENERGIZING_DUO,
        .default_brightness = 0.40f,
        .zone = {
            [ZONE_STRIP] = { FX_DUO_FLOW,    PAL_ENERGIZING_DUO, 1.00f, 0.00f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_ENERGIZING_DUO, 0.80f, 0.05f }, // фиолет или янтарь
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_ENERGIZING_DUO, 0.30f, 0.65f }, // тёплый
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_ENERGIZING_DUO, 0.24f, 0.65f },
        },
    },

    // 13. MIAMI — бирюза + розовый, показушный режим
    [THEME_MIAMI] = {
        .id   = THEME_MIAMI,
        .fx_main = FX_DUO_FLOW,
        .pal_main = PAL_MIAMI,
        .default_brightness = 0.40f,
        .zone = {
            [ZONE_STRIP] = { FX_DUO_FLOW,    PAL_MIAMI, 1.00f, 0.00f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_MIAMI, 0.80f, 0.75f }, // розовый акцент
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_MIAMI, 0.28f, 0.10f }, // бирюза снизу
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_MIAMI, 0.24f, 0.10f },
        },
    },

    // 14. SKY_NIGHT — глубокий сине-фиолетовый (EQS Night)
    [THEME_SKY_NIGHT] = {
        .id   = THEME_SKY_NIGHT,
        .fx_main = FX_FLOW_DEEP,
        .pal_main = PAL_SKY_NIGHT,
        .default_brightness = 0.36f,
        .zone = {
            [ZONE_STRIP] = { FX_FLOW_DEEP,   PAL_SKY_NIGHT, 1.00f, 0.15f },
            [ZONE_HANDLE] = { FX_SOFT_SOLID, PAL_SKY_NIGHT, 0.75f, 0.50f },
            [ZONE_STORAGE] = { FX_SOFT_SOLID, PAL_SKY_NIGHT, 0.26f, 0.35f },
            [ZONE_FOOTWELL] = { FX_SOFT_SOLID, PAL_SKY_NIGHT, 0.22f, 0.30f },
        },
    },

    // OEM-совместимые (если используешь напрямую с 0x351)
    [THEME_OEM_SOLAR] = {
        .id   = THEME_OEM_SOLAR,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_SUNSET_AMBER,
        .default_brightness = 0.30f,
        .zone = { [ZONE_STRIP] = { FX_SOFT_SOLID, PAL_SUNSET_AMBER, 1.0f, 0.4f }, },
    },
    [THEME_OEM_POLAR] = {
        .id   = THEME_OEM_POLAR,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_POLAR_WHITE,
        .default_brightness = 0.30f,
        .zone = { [ZONE_STRIP] = { FX_SOFT_SOLID, PAL_POLAR_WHITE, 1.0f, 0.6f }, },
    },
    [THEME_OEM_NEUTRAL] = {
        .id   = THEME_OEM_NEUTRAL,
        .fx_main = FX_SOFT_SOLID,
        .pal_main = PAL_NEUTRAL_AMBIENT,
        .default_brightness = 0.26f,
        .zone = { [ZONE_STRIP] = { FX_SOFT_SOLID, PAL_NEUTRAL_AMBIENT, 1.0f, 0.5f }, },
    },
};
