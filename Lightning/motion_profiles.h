#pragma once

#include "types.h"
#include "themes.h"

/* Centralized motion tuning profiles.
 * Edit values here to tune premium dynamics without touching logic files. */

#define WS_FX_BASE_SPEED_INIT {        \
    [FX_SOLID_GRADIENT] = 0.0f,        \
    [FX_GRADIENT_FLOW]  = 0.024f,      \
    [FX_SOFT_BREATHE]   = 0.050f,      \
    [FX_DUAL_ZONE]      = 0.0f,        \
    [FX_TWIN_WAVE]      = 0.120f,      \
    [FX_OCEAN_FLOW]     = 0.006f,      \
    [FX_TWO_TONE_WAVE]  = 0.028f,      \
    [FX_ENERGIZE_PULSE] = 0.055f,      \
    [FX_VELVET_FLOW]    = 0.005f,      \
    [FX_GENTLE_PULSE]   = 0.018f,      \
    [FX_AURORA]         = 0.006f,      \
    [FX_CASCADE]        = 0.007f,      \
    [FX_SPARKLE]        = 0.020f,      \
    [FX_BREATHE_WAVE]   = 0.008f,      \
    [FX_COLOR_MORPH]    = 0.004f,      \
    [FX_WELCOME_SWEEP]  = 0.0f,        \
    [FX_GOODBYE_FADE]   = 0.0f,        \
    [FX_WELCOME_LUXE]   = 0.0f,        \
    [FX_GOODBYE_LUXE]   = 0.0f,        \
    [FX_WELCOME]        = 0.0f,        \
    [FX_GOODBYE]        = 0.0f,        \
}

#define WS_THEME_MOTION_SCALE_INIT {        \
    [THEME_AMBER_CHAMPAGNE_ARC]   = 0.82f,  \
    [THEME_AMBER_SUNSET]          = 0.88f,  \
    [THEME_AMBER_COPPER_ROSE]     = 0.92f,  \
    [THEME_AMBER_BURGUNDY_VELVET] = 0.78f,  \
                                              \
    [THEME_BLUE_AURORA_GLACIER]   = 0.96f,  \
    [THEME_BLUE_SAPPHIRE_ICE]     = 1.14f,  \
    [THEME_BLUE_NIGHT_OPAL]       = 1.00f,  \
    [THEME_BLUE_EMERALD_STREAM]   = 1.18f,  \
                                              \
    [THEME_WHITE_PLATINUM_CLOUD]  = 0.74f,  \
    [THEME_WHITE_SILVER_SILK]     = 0.82f,  \
    [THEME_WHITE_PEARL_BLUSH]     = 0.72f,  \
    [THEME_WHITE_GLACIER_MIST]    = 0.78f,  \
}

#define WS_THEME_PERSONALITY_SPEED_INIT {   \
    [THEME_AMBER_CHAMPAGNE_ARC]   = 0.92f,  \
    [THEME_AMBER_SUNSET]          = 1.00f,  \
    [THEME_AMBER_COPPER_ROSE]     = 1.06f,  \
    [THEME_AMBER_BURGUNDY_VELVET] = 0.88f,  \
                                              \
    [THEME_BLUE_AURORA_GLACIER]   = 0.98f,  \
    [THEME_BLUE_SAPPHIRE_ICE]     = 1.12f,  \
    [THEME_BLUE_NIGHT_OPAL]       = 0.95f,  \
    [THEME_BLUE_EMERALD_STREAM]   = 1.10f,  \
                                              \
    [THEME_WHITE_PLATINUM_CLOUD]  = 0.90f,  \
    [THEME_WHITE_SILVER_SILK]     = 0.94f,  \
    [THEME_WHITE_PEARL_BLUSH]     = 0.86f,  \
    [THEME_WHITE_GLACIER_MIST]    = 0.92f,  \
}

#define WS_THEME_PERSONALITY_DEPTH_INIT {   \
    [THEME_AMBER_CHAMPAGNE_ARC]   = +0.22f, \
    [THEME_AMBER_SUNSET]          = +0.12f, \
    [THEME_AMBER_COPPER_ROSE]     = +0.18f, \
    [THEME_AMBER_BURGUNDY_VELVET] = +0.28f, \
                                              \
    [THEME_BLUE_AURORA_GLACIER]   = -0.10f, \
    [THEME_BLUE_SAPPHIRE_ICE]     = -0.16f, \
    [THEME_BLUE_NIGHT_OPAL]       = -0.26f, \
    [THEME_BLUE_EMERALD_STREAM]   = -0.14f, \
                                              \
    [THEME_WHITE_PLATINUM_CLOUD]  = +0.04f, \
    [THEME_WHITE_SILVER_SILK]     = -0.04f, \
    [THEME_WHITE_PEARL_BLUSH]     = +0.08f, \
    [THEME_WHITE_GLACIER_MIST]    = -0.06f, \
}

#define WS_THEME_PERSONALITY_PHASE_INIT {   \
    [THEME_AMBER_CHAMPAGNE_ARC]   = 0.11f,  \
    [THEME_AMBER_SUNSET]          = 0.27f,  \
    [THEME_AMBER_COPPER_ROSE]     = 0.39f,  \
    [THEME_AMBER_BURGUNDY_VELVET] = 0.53f,  \
                                              \
    [THEME_BLUE_AURORA_GLACIER]   = 0.18f,  \
    [THEME_BLUE_SAPPHIRE_ICE]     = 0.35f,  \
    [THEME_BLUE_NIGHT_OPAL]       = 0.62f,  \
    [THEME_BLUE_EMERALD_STREAM]   = 0.47f,  \
                                              \
    [THEME_WHITE_PLATINUM_CLOUD]  = 0.08f,  \
    [THEME_WHITE_SILVER_SILK]     = 0.21f,  \
    [THEME_WHITE_PEARL_BLUSH]     = 0.44f,  \
    [THEME_WHITE_GLACIER_MIST]    = 0.57f,  \
}
