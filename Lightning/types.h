/**
 ******************************************************************************
 * @file    types.h
 * @brief   Common types and definitions for LED lighting system
 * @details Defines core data structures and enumerations used throughout
 *          the ambient lighting system including zones, effects,
 *          and base-scene states.
 *
 * @section Types Overview
 * - zone_id_t: Logical zone identifiers (strip, handle, storage, footwell)
 * - fx_id_t: Visual effect identifiers (gradient, wave, pulse, etc.)
 * - fx_state_t: Runtime state for effects
 * - oneshot_t: One-shot effect state (intro/outro)
 * - base_scene_t: Base-scene state machine
 * - base_scene_stage_t: Base-scene stage enumeration (idle, intro, scene, outro)
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Logical zones inside door/ambient module */
typedef enum {
    ZONE_STRIP = 0,     // main long strip
    ZONE_HANDLE,        // door handle
    ZONE_STORAGE,       // pocket / storage
    ZONE_FOOTWELL,      // footwell
    ZONE_MAX
} zone_id_t;

/* Effect identifiers */
typedef enum {
    /* Basic effects */
    FX_SOLID_GRADIENT = 0,
    FX_GRADIENT_FLOW,
    FX_SOFT_BREATHE,
    FX_DUAL_ZONE,
    FX_TWIN_WAVE,

    /* Premium continuous effects */
    FX_OCEAN_FLOW,
    FX_TWO_TONE_WAVE,
    FX_ENERGIZE_PULSE,
    FX_VELVET_FLOW,        /* Velvet-style premium smooth flow. */
    FX_GENTLE_PULSE,       /* Soft premium pulse. */

    /* W223/EQS Premium effects */
    FX_AURORA,                /* Multi-phase sinusoidal aurora effect. */
    FX_CASCADE,               /* Falling brightness wave from start to end. */
    FX_SPARKLE,               /* Sparse per-pixel sparkle flashes. */
    FX_BREATHE_WAVE,          /* Traveling breathing-style brightness wave. */
    FX_COLOR_MORPH,           /* Smooth palette morph between zone accents. */

    /* One-shot effects */
    FX_WELCOME_SWEEP,
    FX_GOODBYE_FADE,
    FX_WELCOME_LUXE,
    FX_GOODBYE_LUXE,
    FX_WELCOME,
    FX_GOODBYE,

    FX_MAX_
} fx_id_t;

/* FX state */
typedef struct {
    float t;
    float phase;
    float speed;
    float a, b;
} fx_state_t;

/* One-shot FX state (intro/outro) */
typedef struct {
    uint8_t          active;
    fx_id_t          fx_id;
    float            base_br;
    uint32_t         start_ms;
    uint32_t         duration_ms;
    uint16_t         first;      /* first LED index in zone */
    uint16_t         count;      /* LED count in zone */
} oneshot_t;

/* Base-scene state machine */
typedef enum {
    BASE_SCENE_IDLE = 0,
    BASE_SCENE_INTRO,
    BASE_SCENE_BRIDGE,
    BASE_SCENE_ACTIVE,
    BASE_SCENE_OUTRO
} base_scene_stage_t;

typedef enum {
    OEM_COLOR_AMBER = 0,   /* solar / amber / warm */
    OEM_COLOR_BLUE,        /* polar / blue         */
    OEM_COLOR_WHITE,       /* neutral / white      */
    OEM_COLOR_MAX
} oem_color_id_t;

/* Motion personality profile selected from CAN/user policy. */
typedef enum {
    MOTION_PROFILE_LUXURY = 0u,
    MOTION_PROFILE_CALM   = 1u,
    MOTION_PROFILE_SPORT  = 2u
} motion_profile_t;

/* Runtime base scene container used by director/zones pipeline. */
typedef struct {
    base_scene_stage_t stage;

    uint32_t t0_ms;

    float    calc_brightness;
    float    scene_brightness;
    float    runtime_dimming;

    oneshot_t intro;
    oneshot_t outro;

    fx_state_t st_scene;

    /* --- Auto-rotate / Crossfade support --- */
    uint8_t       auto_rotate_enabled;   /* 0 = manual, 1 = auto-rotate within bank */
    uint8_t       oem_color;             /* current OEM color (bank selector) */
    uint32_t      scene_start_ms;        /* when current scene started (for auto-rotate timer) */
    
    /* Crossfade state */
    uint8_t       crossfade_active;      /* 1 = crossfade in progress */
    fx_state_t    st_scene_next;         /* effect state for next theme */
    uint32_t      crossfade_start_ms;    /* when crossfade started */
    float         crossfade_blend_smooth;/* smoothed crossfade blend (0..1) */
    float         bridge_blend_smooth;   /* smoothed bridge blend (0..1) */
    float         global_brightness_smooth; /* LPF-smoothed output brightness */
} base_scene_t;

#ifndef CLAMP01
#define CLAMP01(x) ((x)<0.0f?0.0f:((x)>1.0f?1.0f:(x)))  /**< Clamp value to [0.0, 1.0] range */
#endif

#ifdef __cplusplus
}
#endif /* TYPES_H */
