#include <stddef.h>
#include "presets.h"

/* -----------------------------------------------------------------------
 * Основная таблица тем
 * ----------------------------------------------------------------------- */

static const ws_theme_desc_t G_THEMES[THEME_MAX_] = {
    /* ================================================================
     * AMBER BANK
     * ================================================================ */

    [THEME_AMBER_WARM_LOUNGE] = {
        .fx_main = FX_MB_SOFT_SOLID,
        .pal_main = WSPAL_MB_SUNSET_AMBER,
        .theme_brightness = 0.60f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 1.00f,
                .accent_u = 0.40f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.35f,
                .accent_u = 0.45f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.25f,
                .accent_u = 0.40f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.30f,
                .accent_u = 0.50f,
            },
        },
    },

    [THEME_AMBER_SUNSET_FLOW] = {
        .fx_main = FX_MB_FLOW_SOFT,
        .pal_main = WSPAL_MB_SUNSET_AMBER,
        .theme_brightness = 0.70f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_FLOW_SOFT,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.40f,
                .accent_u = 0.55f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.25f,
                .accent_u = 0.40f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.30f,
                .accent_u = 0.60f,
            },
        },
    },

    [THEME_AMBER_SOFT_PULSE] = {
        .fx_main = FX_MB_ENERGIZE_PULSE,
        .pal_main = WSPAL_MB_SUNSET_AMBER,
        .theme_brightness = 0.75f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_ENERGIZE_PULSE,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.45f,
                .accent_u = 0.45f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.25f,
                .accent_u = 0.35f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.35f,
                .accent_u = 0.50f,
            },
        },
    },

    [THEME_AMBER_DUO_WAVE] = {
        .fx_main = FX_MB_TWO_TONE_WAVE,
        .pal_main = WSPAL_MB_SUNSET_AMBER,
        .theme_brightness = 0.75f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_TWO_TONE_WAVE,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.45f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.25f,
                .accent_u = 0.35f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_SUNSET_AMBER,
                .rel_brightness = 0.35f,
                .accent_u = 0.55f,
            },
        },
    },

    /* ================================================================
     * BLUE BANK
     * ================================================================ */

    [THEME_BLUE_COOL_LOUNGE] = {
        .fx_main = FX_MB_SOFT_SOLID,
        .pal_main = WSPAL_MB_OCEAN_BLUE,
        .theme_brightness = 0.65f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 1.00f,
                .accent_u = 0.40f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.35f,
                .accent_u = 0.60f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.20f,
                .accent_u = 0.50f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.30f,
                .accent_u = 0.70f,
            },
        },
    },
    [THEME_BLUE_OCEAN_FLOW] = {
        .fx_main = FX_MB_OCEAN_FLOW,
        .pal_main = WSPAL_MB_OCEAN_BLUE,
        .theme_brightness = 0.75f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_OCEAN_FLOW,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_BREATHE,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.35f,
                .accent_u = 0.65f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.20f,
                .accent_u = 0.55f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_FLOW_SOFT,
                .pal_id = WSPAL_MB_OCEAN_BLUE,
                .rel_brightness = 0.28f,
                .accent_u = 0.75f,
            },
        },
    },

    [THEME_BLUE_GLACIER] = {
        .fx_main = FX_MB_FLOW_SOFT,
        .pal_main = WSPAL_MB_GLACIER,
        .theme_brightness = 0.70f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_FLOW_SOFT,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 0.40f,
                .accent_u = 0.6f,
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

    [THEME_BLUE_SKY_NIGHT] = {
        .fx_main = FX_MB_TWO_TONE_WAVE,
        .pal_main = WSPAL_MB_PURPLE_SILK, /* сине-фиолетовая палитра */
        .theme_brightness = 0.75f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_TWO_TONE_WAVE,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 1.00f,
                .accent_u = 0.0f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 0.40f,
                .accent_u = 0.6f,
            },
            [WS_ZONE_STORAGE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 0.25f,
                .accent_u = 0.5f,
            },
            [WS_ZONE_FOOTWELL] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_PURPLE_SILK,
                .rel_brightness = 0.35f,
                .accent_u = 0.7f,
            },
        },
    },

    /* ================================================================
     * WHITE BANK
     * ================================================================ */

    [THEME_WHITE_POLAR_BREATHE] = {
        .fx_main = FX_MB_SOFT_BREATHE,
        .pal_main = WSPAL_MB_POLAR_WHITE,
        .theme_brightness = 0.60f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_SOFT_BREATHE,
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
                .rel_brightness = 0.30f,
                .accent_u = 0.5f,
            },
        },
    },

    [THEME_WHITE_ICE] = {
        .fx_main = FX_MB_SOFT_SOLID,
        .pal_main = WSPAL_MB_GLACIER,
        .theme_brightness = 0.65f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 1.00f,
                .accent_u = 0.3f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_GLACIER,
                .rel_brightness = 0.40f,
                .accent_u = 0.6f,
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

    [THEME_WHITE_SATIN] = {
        .fx_main = FX_MB_SOFT_SOLID,
        .pal_main = WSPAL_MB_POLAR_WHITE,
        .theme_brightness = 0.55f,
        .zone = {
            [WS_ZONE_STRIP] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_POLAR_WHITE,
                .rel_brightness = 1.00f,
                .accent_u = 0.45f,
            },
            [WS_ZONE_HANDLE] = {
                .fx = FX_MB_SOFT_SOLID,
                .pal_id = WSPAL_MB_POLAR_WHITE,
                .rel_brightness = 0.35f,
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
                .rel_brightness = 0.30f,
                .accent_u = 0.5f,
            },
        },
    },

    [THEME_WHITE_CRYSTAL_WAVE] = {
        .fx_main = FX_MB_TWO_TONE_WAVE,
        .pal_main = WSPAL_MB_POLAR_WHITE,
        .theme_brightness = 0.70f,
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

	//	Глубокий ночной синий + мягкий поток оттенков.
	[THEME_NIGHT_CRUISE] = {
	    .fx_main          = FX_MB_OCEAN_FLOW,
	    .pal_main         = WSPAL_MB_NIGHT_BLUE,
	    .theme_brightness = 0.55f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_OCEAN_FLOW,
	            .pal_id         = WSPAL_MB_NIGHT_BLUE,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.35f
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_NIGHT_BLUE,
	            .rel_brightness = 0.38f,
	            .accent_u       = 0.55f
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_NIGHT_BLUE,
	            .rel_brightness = 0.22f,
	            .accent_u       = 0.50f
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_FLOW_SOFT,
	            .pal_id         = WSPAL_MB_NIGHT_BLUE,
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
	    .fx_main          = FX_GRADIENT_FLOW,
	    .pal_main         = WSPAL_MB_COPPER_GOLD,
	    .theme_brightness = 0.50f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_GRADIENT_FLOW,
	            .pal_id         = WSPAL_MB_COPPER_GOLD,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.45f
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
	    .fx_main          = FX_MB_TWO_TONE_WAVE,
	    .pal_main         = WSPAL_MB_PURPLE_SILK,  // индиго / фиолетово-синий
	    .theme_brightness = 0.55f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_TWO_TONE_WAVE,
	            .pal_id         = WSPAL_MB_PURPLE_SILK,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.50f,
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
	    .fx_main          = FX_MB_ENERGIZE_PULSE,
	    .pal_main         = WSPAL_MB_GLACIER,   // ледяной голубой
	    .theme_brightness = 0.60f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_ENERGIZE_PULSE,
	            .pal_id         = WSPAL_MB_GLACIER,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.45f,
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

	[THEME_RED_MOON_PREMIUM] = {
	    .fx_main          = FX_MB_ENERGIZE_PULSE,
	    .pal_main         = WSPAL_MB_SUNSET_AMBER,
	    .theme_brightness = 0.65f,

	    .zone = {
	        [WS_ZONE_STRIP] = {
	            .fx             = FX_MB_ENERGIZE_PULSE,
	            .pal_id         = WSPAL_MB_SUNSET_AMBER,
	            .rel_brightness = 1.00f,
	            .accent_u       = 0.80f,   // более красный край палитры
	        },
	        [WS_ZONE_HANDLE] = {
	            .fx             = FX_MB_SOFT_BREATHE,
	            .pal_id         = WSPAL_MB_SUNSET_AMBER,
	            .rel_brightness = 0.40f,
	            .accent_u       = 0.75f,
	        },
	        [WS_ZONE_STORAGE] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_SUNSET_AMBER,
	            .rel_brightness = 0.24f,
	            .accent_u       = 0.70f,
	        },
	        [WS_ZONE_FOOTWELL] = {
	            .fx             = FX_MB_SOFT_SOLID,
	            .pal_id         = WSPAL_MB_SUNSET_AMBER,
	            .rel_brightness = 0.30f,
	            .accent_u       = 0.65f,
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
    THEME_AMBER_WARM_LOUNGE,
    THEME_AMBER_SUNSET_FLOW,
    THEME_AMBER_SOFT_PULSE,
    THEME_AMBER_DUO_WAVE,
    THEME_COPPER_LOUNGE,      // новый медно-золотой премиум
    THEME_ROSE_SUITE,         // мягкий розово-золотой lounge
    THEME_RED_MOON_PREMIUM,   // «красная луна»
};

static const ws_theme_id_t G_BLUE_THEMES[] = {
    THEME_BLUE_COOL_LOUNGE,
    THEME_BLUE_OCEAN_FLOW,
    THEME_BLUE_GLACIER,
    THEME_BLUE_SKY_NIGHT,
    THEME_NIGHT_CRUISE,   // глубокий ночной синий
    THEME_INDIGO_SKY,     // сине-фиолетовое небо
};

static const ws_theme_id_t G_WHITE_THEMES[] = {
    THEME_WHITE_POLAR_BREATHE,
    THEME_WHITE_ICE,
    THEME_WHITE_SATIN,
    THEME_WHITE_CRYSTAL_WAVE,
    THEME_HYACINTH_DREAM,  // фиолетово-розовый премиум
    THEME_ICE_CRYSTAL,     // ледяной кристалл
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
