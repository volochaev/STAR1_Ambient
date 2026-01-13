#include <stddef.h>
#include "presets.h"

/* -----------------------------------------------------------------------
 * Основная таблица тем
 * ----------------------------------------------------------------------- */

static const ws_theme_desc_t G_THEMES[THEME_MAX_] = {
    /* ================================================================
     * AMBER BANK
     * ================================================================ */

    /* Тёплый янтарный Aurora - магический эффект северного сияния */
    [THEME_AMBER_WARM_LOUNGE] = {
        .fx_main = FX_AURORA,
        .pal_main = WSPAL_MB_SUNSET_AMBER,
        .theme_brightness = 0.55f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_AURORA,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.38f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.24f,
                .accent_u = 0.45f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.30f,
                .accent_u = 0.50f,
            },
        },
    },

    /* Шампань Каскад - элегантный каскадный эффект */
    [THEME_AMBER_SUNSET_FLOW] = {
        .fx_main = FX_CASCADE,
        .pal_main = WSPAL_CHAMPAGNE_GOLD,
        .theme_brightness = 0.58f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_CASCADE,
                .pal_id = WSPAL_CHAMPAGNE_GOLD,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_CHAMPAGNE_GOLD,
                .rel_brightness = 0.42f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_CHAMPAGNE_GOLD,
                .rel_brightness = 0.26f,
                .accent_u = 0.45f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_CHAMPAGNE_GOLD,
                .rel_brightness = 0.30f,
                .accent_u = 0.55f,
            },
        },
    },

    /* Золотое Мерцание - праздничный эффект искр */
    [THEME_AMBER_SOFT_PULSE] = {
        .fx_main = FX_SPARKLE,
        .pal_main = WSPAL_MB_AMBER_GOLD,
        .theme_brightness = 0.55f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_SPARKLE,
                .pal_id = WSPAL_MB_AMBER_GOLD,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_AMBER_GOLD,
                .rel_brightness = 0.43f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_AMBER_GOLD,
                .rel_brightness = 0.24f,
                .accent_u = 0.45f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_AMBER_GOLD,
                .rel_brightness = 0.32f,
                .accent_u = 0.50f,
            },
        },
    },

    [THEME_AMBER_DUO_WAVE] = {
        .fx_main = FX_MB_TWO_TONE_WAVE,
        .pal_main = WSPAL_MB_SUNSET_AMBER,
        .theme_brightness = 0.60f,  // ниже для премиум вида
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_TWO_TONE_WAVE,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,  // дыхание для премиум эффекта
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.42f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.24f,
                .accent_u = 0.45f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.32f,
                .accent_u = 0.50f,
            },
        },
    },

    /* ================================================================
     * BLUE BANK
     * ================================================================ */

    /* Северное Сияние - магический эффект aurora borealis */
    [THEME_BLUE_COOL_LOUNGE] = {
        .fx_main = FX_AURORA,
        .pal_main = WSPAL_AURORA_BOREALIS,
        .theme_brightness = 0.55f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_AURORA,
                .pal_id = WSPAL_AURORA_BOREALIS,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_AURORA_BOREALIS,
                .rel_brightness = 0.38f,
                .accent_u = 0.55f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_AURORA_BOREALIS,
                .rel_brightness = 0.22f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_AURORA_BOREALIS,
                .rel_brightness = 0.30f,
                .accent_u = 0.60f,
            },
        },
    },
    /* Глубокое Дыхание Океана - медитативный эффект */
    [THEME_BLUE_OCEAN_FLOW] = {
        .fx_main = FX_BREATHE_WAVE,
        .pal_main = WSPAL_MB_OCEAN_BLUE,
        .theme_brightness = 0.58f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_BREATHE_WAVE,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.38f,
                .accent_u = 0.60f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.22f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_FLOW_SOFT,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.28f,
                .accent_u = 0.65f,
            },
        },
    },

    /* Ледяной Сапфир - мерцающие льдинки */
    [THEME_BLUE_GLACIER] = {
        .fx_main = FX_SPARKLE,
        .pal_main = WSPAL_ICE_SAPPHIRE,
        .theme_brightness = 0.55f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_SPARKLE,
                .pal_id = WSPAL_ICE_SAPPHIRE,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_ICE_SAPPHIRE,
                .rel_brightness = 0.42f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_ICE_SAPPHIRE,
                .rel_brightness = 0.24f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_ICE_SAPPHIRE,
                .rel_brightness = 0.32f,
                .accent_u = 0.6f,
            },
        },
    },

    /* Ночной Морфинг - гипнотическое перетекание цветов */
    [THEME_BLUE_SKY_NIGHT] = {
        .fx_main = FX_COLOR_MORPH,
        .pal_main = WSPAL_MB_PURPLE_SILK,
        .theme_brightness = 0.52f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_COLOR_MORPH,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 0.42f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 0.24f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 0.32f,
                .accent_u = 0.6f,
            },
        },
    },

    /* ================================================================
     * WHITE BANK
     * ================================================================ */

    /* Бриллиантовый Каскад - элегантный падающий свет */
    [THEME_WHITE_POLAR_BREATHE] = {
        .fx_main = FX_CASCADE,
        .pal_main = WSPAL_MB_DIAMOND_WHITE,
        .theme_brightness = 0.55f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_CASCADE,
                .pal_id = WSPAL_MB_DIAMOND_WHITE,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_DIAMOND_WHITE,
                .rel_brightness = 0.40f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_DIAMOND_WHITE,
                .rel_brightness = 0.25f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_DIAMOND_WHITE,
                .rel_brightness = 0.30f,
                .accent_u = 0.5f,
            },
        },
    },

    /* Кристальное Сияние - холодная aurora */
    [THEME_WHITE_ICE] = {
        .fx_main = FX_AURORA,
        .pal_main = WSPAL_MB_GLACIER,
        .theme_brightness = 0.52f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_AURORA,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 0.42f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 0.25f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 0.35f,
                .accent_u = 0.7f,
            },
        },
    },

    /* Платиновое Мерцание - роскошные искры */
    [THEME_WHITE_SATIN] = {
        .fx_main = FX_SPARKLE,
        .pal_main = WSPAL_MB_PLATINUM,
        .theme_brightness = 0.52f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_SPARKLE,
                .pal_id = WSPAL_MB_PLATINUM,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_PLATINUM,
                .rel_brightness = 0.38f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_PLATINUM,
                .rel_brightness = 0.25f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_PLATINUM,
                .rel_brightness = 0.30f,
                .accent_u = 0.5f,
            },
        },
    },

    [THEME_WHITE_CRYSTAL_WAVE] = {
        .fx_main = FX_MB_TWO_TONE_WAVE,
        .pal_main = WSPAL_MB_POLAR_WHITE,
        .theme_brightness = 0.60f,  // немного ниже для премиум вида
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_TWO_TONE_WAVE,
                .pal_id = WSPAL_MB_POLAR_WHITE,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_POLAR_WHITE,
                .rel_brightness = 0.40f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_POLAR_WHITE,
                .rel_brightness = 0.25f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_POLAR_WHITE,
                .rel_brightness = 0.35f,
                .accent_u = 0.5f,
            },
        },
    },

	/* Изумрудный Лес - природный морфинг зелёных оттенков */
	[THEME_NIGHT_CRUISE] = {
	    .fx_main          = FX_COLOR_MORPH,
	    .pal_main         = WSPAL_EMERALD_FOREST,
	    .theme_brightness = 0.50f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_COLOR_MORPH,
	            .pal_id         = WSPAL_EMERALD_FOREST,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.0f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_EMERALD_FOREST,
	            .rel_brightness = 0.38f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_EMERALD_FOREST,
	            .rel_brightness = 0.22f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_FLOW_SOFT,
	            .pal_id         = WSPAL_EMERALD_FOREST,
	            .rel_brightness = 0.30f,
	            .accent_u       = 0.70f
	        },
	    },
	},

	//	Королевский фиолетово-розовый, с бархатным breathing по зонам.
	[THEME_HYACINTH_DREAM] = {
	    .fx_main          = FX_MB_TWO_TONE_WAVE,
	    .pal_main         = WSPAL_MB_HYACINTH,
	    .theme_brightness = 0.48f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_TWO_TONE_WAVE,
	            .pal_id         = WSPAL_MB_HYACINTH,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_HYACINTH,
	            .rel_brightness = 0.40f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_HYACINTH,
	            .rel_brightness = 0.22f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_FLOW_SOFT,
	            .pal_id         = WSPAL_MB_HYACINTH,
	            .rel_brightness = 0.28f,
	            .accent_u       = 0.75f
	        },
	    },
	},

	//	Дорогой медно-золотой свет — премиум премиумов.
	[THEME_COPPER_LOUNGE] = {
	    .fx_main          = FX_MB_VELVET_FLOW,  // более плавный бархатный поток
	    .pal_main         = WSPAL_MB_COPPER_GOLD,
	    .theme_brightness = 0.48f,  // немного ниже для премиум вида

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_VELVET_FLOW,  // бархатный поток для плавности
	            .pal_id         = WSPAL_MB_COPPER_GOLD,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_COPPER_GOLD,
	            .rel_brightness = 0.45f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_COPPER_GOLD,
	            .rel_brightness = 0.25f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_COPPER_GOLD,
	            .rel_brightness = 0.30f,
	            .accent_u       = 0.70f
	        },
	    },
	},

	//	Мягкий розово-золотой — расслабляет, создаёт уют, “spa mode”.
	[THEME_ROSE_SUITE] = {
	    .fx_main          = FX_MB_OCEAN_FLOW,
	    .pal_main         = WSPAL_MB_ROSE,
	    .theme_brightness = 0.45f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_OCEAN_FLOW,
	            .pal_id         = WSPAL_MB_ROSE,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.45f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_ROSE,
	            .rel_brightness = 0.38f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_ROSE,
	            .rel_brightness = 0.22f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_FLOW_SOFT,
	            .pal_id         = WSPAL_MB_ROSE,
	            .rel_brightness = 0.28f,
	            .accent_u       = 0.75f
	        },
	    },
	},

	[THEME_INDIGO_SKY] = {
	    .fx_main          = FX_MB_VELVET_FLOW,  // бархатный поток для отличия от SKY_NIGHT
	    .pal_main         = WSPAL_MB_PURPLE_SILK,  // индиго / фиолетово-синий
	    .theme_brightness = 0.58f,  // немного выше для различия

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_VELVET_FLOW,  // бархатный поток вместо двухтонной волны
	            .pal_id         = WSPAL_MB_PURPLE_SILK,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.0f,  // 0.0 для плавного потока
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_PURPLE_SILK,
	            .rel_brightness = 0.38f,
	            .accent_u       = 0.55f,
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_PURPLE_SILK,
	            .rel_brightness = 0.22f,
	            .accent_u       = 0.45f,
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_FLOW_SOFT,
	            .pal_id         = WSPAL_MB_PURPLE_SILK,
	            .rel_brightness = 0.30f,
	            .accent_u       = 0.70f,
	        },
	    },
	},

	[THEME_ICE_CRYSTAL] = {
	    .fx_main          = FX_MB_GENTLE_PULSE,  // мягкий пульс вместо энергичного
	    .pal_main         = WSPAL_MB_GLACIER,   // ледяной голубой
	    .theme_brightness = 0.55f,  // немного ниже для премиум вида

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_GENTLE_PULSE,  // мягкий пульс для плавности
	            .pal_id         = WSPAL_MB_GLACIER,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.5f,
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_GLACIER,
	            .rel_brightness = 0.40f,
	            .accent_u       = 0.55f,
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_GLACIER,
	            .rel_brightness = 0.24f,
	            .accent_u       = 0.50f,
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_FLOW_SOFT,
	            .pal_id         = WSPAL_MB_GLACIER,
	            .rel_brightness = 0.30f,
	            .accent_u       = 0.70f,
	        },
	    },
	},

	/* Глубокий Бордо - роскошный винный оттенок с дыханием волны */
	[THEME_RED_MOON_PREMIUM] = {
	    .fx_main          = FX_BREATHE_WAVE,
	    .pal_main         = WSPAL_DEEP_BURGUNDY,
	    .theme_brightness = 0.50f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_BREATHE_WAVE,
	            .pal_id         = WSPAL_DEEP_BURGUNDY,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.0f,
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_DEEP_BURGUNDY,
	            .rel_brightness = 0.40f,
	            .accent_u       = 0.50f,
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_DEEP_BURGUNDY,
	            .rel_brightness = 0.24f,
	            .accent_u       = 0.50f,
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_DEEP_BURGUNDY,
	            .rel_brightness = 0.30f,
	            .accent_u       = 0.50f,
	        },
	    },
	},

	/* ================================================================
	 * PREMIUM BANK (W223/EQS стиль)
	 * ================================================================ */

	/* Бриллиантовая Волна Дыхания - премиальная медитативность */
	[THEME_DIAMOND_PREMIUM] = {
	    .fx_main          = FX_BREATHE_WAVE,
	    .pal_main         = WSPAL_MB_DIAMOND_WHITE,
	    .theme_brightness = 0.52f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_BREATHE_WAVE,
	            .pal_id         = WSPAL_MB_DIAMOND_WHITE,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.0f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_DIAMOND_WHITE,
	            .rel_brightness = 0.42f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_DIAMOND_WHITE,
	            .rel_brightness = 0.25f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_DIAMOND_WHITE,
	            .rel_brightness = 0.32f,
	            .accent_u       = 0.5f
	        },
	    },
	},

	// Серебряный лаунж с мягким пульсом - нейтральная роскошь
	[THEME_SILVER_LOUNGE] = {
	    .fx_main          = FX_MB_GENTLE_PULSE,
	    .pal_main         = WSPAL_MB_SILVER_MIST,
	    .theme_brightness = 0.58f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_GENTLE_PULSE,
	            .pal_id         = WSPAL_MB_SILVER_MIST,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_SILVER_MIST,
	            .rel_brightness = 0.40f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_SILVER_MIST,
	            .rel_brightness = 0.24f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_SILVER_MIST,
	            .rel_brightness = 0.30f,
	            .accent_u       = 0.5f
	        },
	    },
	},

	// Платиновая элегантность с бархатным потоком - теплая роскошь
	[THEME_PLATINUM_ELEGANCE] = {
	    .fx_main          = FX_MB_VELVET_FLOW,
	    .pal_main         = WSPAL_MB_PLATINUM,
	    .theme_brightness = 0.52f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_VELVET_FLOW,
	            .pal_id         = WSPAL_MB_PLATINUM,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.0f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_PLATINUM,
	            .rel_brightness = 0.38f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_PLATINUM,
	            .rel_brightness = 0.23f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_PLATINUM,
	            .rel_brightness = 0.28f,
	            .accent_u       = 0.5f
	        },
	    },
	},

	// Янтарное золото люкс с мягким пульсом - теплая премиум роскошь
	[THEME_AMBER_GOLD_LUXURY] = {
	    .fx_main          = FX_MB_GENTLE_PULSE,
	    .pal_main         = WSPAL_MB_AMBER_GOLD,
	    .theme_brightness = 0.50f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_GENTLE_PULSE,
	            .pal_id         = WSPAL_MB_AMBER_GOLD,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.5f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_AMBER_GOLD,
	            .rel_brightness = 0.43f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_AMBER_GOLD,
	            .rel_brightness = 0.25f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_AMBER_GOLD,
	            .rel_brightness = 0.32f,
	            .accent_u       = 0.60f
	        },
	    },
	},

	/* Королевская Aurora - магический пурпур с северным сиянием */
	[THEME_ROYAL_MAGENTA] = {
	    .fx_main          = FX_AURORA,
	    .pal_main         = WSPAL_MB_MAGENTA_ROYAL,
	    .theme_brightness = 0.48f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_AURORA,
	            .pal_id         = WSPAL_MB_MAGENTA_ROYAL,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.0f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_MAGENTA_ROYAL,
	            .rel_brightness = 0.40f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_MAGENTA_ROYAL,
	            .rel_brightness = 0.22f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_FLOW_SOFT,
	            .pal_id         = WSPAL_MB_MAGENTA_ROYAL,
	            .rel_brightness = 0.28f,
	            .accent_u       = 0.70f
	        },
	    },
	},
};

/* -----------------------------------------------------------------------
 * Доступ к описанию темы
 * ----------------------------------------------------------------------- */

const ws_theme_desc_t* ws_theme_get(ws_theme_id_t id)
{
    if (id >= THEME_MAX_) {
        id = 0;
    }
    return &G_THEMES[(ws_theme_enum_t)id];
}

/* -----------------------------------------------------------------------
 * OEM color → theme banks
 * ----------------------------------------------------------------------- */

static const ws_theme_id_t G_AMBER_THEMES[] = {
    THEME_AMBER_WARM_LOUNGE,     // тёплый янтарный лаунж
    THEME_AMBER_SUNSET_FLOW,     // золотистый шампань
    THEME_AMBER_SOFT_PULSE,      // янтарное золото с искрами
    THEME_AMBER_DUO_WAVE,        // янтарная двухтоновая волна
    THEME_COPPER_LOUNGE,         // медно-золотой премиум
    THEME_AMBER_GOLD_LUXURY,     // янтарное золото люкс
};

static const ws_theme_id_t G_BLUE_THEMES[] = {
    THEME_BLUE_COOL_LOUNGE,
    THEME_BLUE_OCEAN_FLOW,
    THEME_BLUE_GLACIER,
    THEME_BLUE_SKY_NIGHT,
    THEME_NIGHT_CRUISE,   // глубокий ночной синий
    THEME_INDIGO_SKY,     // сине-фиолетовое небо
    THEME_ROYAL_MAGENTA,  // королевский пурпур
};

static const ws_theme_id_t G_WHITE_THEMES[] = {
    THEME_WHITE_POLAR_BREATHE,
    THEME_WHITE_ICE,
    THEME_WHITE_SATIN,
    THEME_WHITE_CRYSTAL_WAVE,
    THEME_HYACINTH_DREAM,    // фиолетово-розовый премиум
    THEME_ICE_CRYSTAL,       // ледяной кристалл
    THEME_DIAMOND_PREMIUM,   // бриллиантово-белый премиум
    THEME_SILVER_LOUNGE,     // серебряный лаунж
    THEME_PLATINUM_ELEGANCE, // платиновая элегантность
    THEME_ROSE_SUITE,        // розово-золотой lounge (перенесён из AMBER)
    THEME_RED_MOON_PREMIUM,  // бордо «красная луна» (перенесён из AMBER)
};

const ws_theme_bank_t g_theme_banks[OEM_COLOR_MAX] = {
    [OEM_COLOR_AMBER] = {
        .themes = G_AMBER_THEMES,
        .count  = (uint8_t)(sizeof(G_AMBER_THEMES)/sizeof(G_AMBER_THEMES[0])),
    },
    [OEM_COLOR_BLUE] = {
        .themes = G_BLUE_THEMES,
        .count  = (uint8_t)(sizeof(G_BLUE_THEMES)/sizeof(G_BLUE_THEMES[0])),
    },
    [OEM_COLOR_WHITE] = {
        .themes = G_WHITE_THEMES,
        .count  = (uint8_t)(sizeof(G_WHITE_THEMES)/sizeof(G_WHITE_THEMES[0])),
    },
};

/* -----------------------------------------------------------------------
 * Helpers для OEM/Extended режимов
 * ----------------------------------------------------------------------- */

const ws_theme_bank_t* ws_theme_get_bank(oem_color_id_t id)
{
    if (id < 0 || id >= OEM_COLOR_MAX)
        return NULL;

    const ws_theme_bank_t *bank = &g_theme_banks[id];
    if (!bank->themes || bank->count == 0)
        return NULL;

    return bank;
}

ws_theme_id_t ws_theme_bank_next(const ws_theme_bank_t *bank,
                                 ws_theme_id_t          current)
{
    if (!bank || !bank->themes || bank->count == 0) {
        return (ws_theme_id_t)0;
    }

    for (uint8_t i = 0; i < bank->count; ++i) {
        if (bank->themes[i] == current) {
            uint8_t j = (uint8_t)((i + 1u) % bank->count);
            return bank->themes[j];
        }
    }

    /* Если текущая тема не найдена — вернём первую */
    return bank->themes[0];
}

ws_theme_id_t ws_theme_default_for_oem(oem_color_id_t id)
{
    const ws_theme_bank_t *bank = ws_theme_get_bank(id);
    if (!bank || !bank->themes || bank->count == 0) {
        return (ws_theme_id_t)0;
    }
    return bank->themes[0];
}

/* Helper: find which bank contains a theme, returns -1 if not found */
static int find_theme_bank(ws_theme_id_t theme)
{
    for (int b = 0; b < OEM_COLOR_MAX; ++b) {
        const ws_theme_bank_t *bank = &g_theme_banks[b];
        if (!bank->themes) continue;
        for (uint8_t i = 0; i < bank->count; ++i) {
            if (bank->themes[i] == theme) {
                return b;
            }
        }
    }
    return -1;
}

uint8_t ws_theme_same_bank(ws_theme_id_t theme_a, ws_theme_id_t theme_b)
{
    if (theme_a == theme_b) return 1;
    
    int bank_a = find_theme_bank(theme_a);
    int bank_b = find_theme_bank(theme_b);
    
    /* If either not found, or in different banks */
    if (bank_a < 0 || bank_b < 0) return 0;
    
    return (bank_a == bank_b) ? 1 : 0;
}
