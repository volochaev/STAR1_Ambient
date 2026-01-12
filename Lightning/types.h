/**
 ******************************************************************************
 * @file    types.h
 * @brief   Common types and definitions for LED lighting system
 * @details Defines core data structures and enumerations used throughout
 *          the ambient lighting system including zones, effects, palettes,
 *          and scene player states.
 *
 * @section Types Overview
 * - ws_zone_id_t: Logical zone identifiers (strip, handle, storage, footwell)
 * - fx_id_t: Visual effect identifiers (gradient, wave, pulse, etc.)
 * - fx_state_t: Runtime state for effects
 * - oneshot_t: One-shot effect state (intro/outro)
 * - scene_player_t: Scene player state machine
 * - player_stage_t: Player stage enumeration (idle, intro, scene, outro)
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
    WS_ZONE_STRIP = 0,     // main long strip
    WS_ZONE_HANDLE,        // door handle
    WS_ZONE_STORAGE,       // pocket / storage
    WS_ZONE_FOOTWELL,      // footwell
    WS_ZONE_MAX
} ws_zone_id_t;

/* Effect identifiers */
typedef enum {
    FX_SOLID_GRADIENT = 0,
    FX_GRADIENT_FLOW,
    FX_SOFT_BREATHE,
    FX_DUAL_ZONE,
    FX_TWIN_WAVE,
    FX_MB_OCEAN_FLOW,
    FX_MB_TWO_TONE_WAVE,
    FX_MB_ENERGIZE_PULSE,
    FX_WELCOME_SWEEP,
    FX_GOODBYE_FADE,
    FX_MB_WELCOME,
    FX_MB_GOODBYE,
	FX_WELCOME,
    FX_GOODBYE,
    FX_MAX_
} fx_id_t;

/* Mercedes-style aliases */
#define FX_MB_SOFT_SOLID      FX_SOLID_GRADIENT
#define FX_MB_FLOW_SOFT       FX_GRADIENT_FLOW
#define FX_MB_FLOW_DEEP       FX_MB_OCEAN_FLOW
#define FX_MB_DUO_FLOW        FX_DUAL_ZONE
#define FX_MB_SOFT_BREATHE    FX_SOFT_BREATHE
#define FX_MB_ENERGIZE        FX_MB_ENERGIZE_PULSE

/* FX state */
typedef struct {
    float t;
    float phase;
    float speed;
    float a, b;
} fx_state_t;

/* Forward decl for palette */
struct ws_palette_s;
typedef struct ws_palette_s ws_palette_t;

/* One-shot FX state (intro/outro) */
typedef struct {
    uint8_t          active;
    fx_id_t          fx_id;
    float            base_br;
    const ws_palette_t *pal;
    uint32_t         start_ms;
    uint32_t         duration_ms;
} oneshot_t;

/* Scene player */
typedef enum {
    PST_IDLE = 0,
    PST_INTRO,
    PST_BRIDGE,
    PST_SCENE,
    PST_OUTRO
} player_stage_t;

typedef uint8_t ws_theme_id_t;

typedef struct {
    player_stage_t stage;

    uint32_t t0_ms;

    float    calc_brightness;
    float    theme_brightness;
    float    theme_dimming;

    ws_theme_id_t theme;

    oneshot_t intro;
    oneshot_t outro;

    fx_state_t st_scene;
} scene_player_t;

#ifndef CLAMP01
#define CLAMP01(x) ((x)<0.0f?0.0f:((x)>1.0f?1.0f:(x)))  /**< Clamp value to [0.0, 1.0] range */
#endif

#ifdef __cplusplus
}
#endif
#endif /* TYPES_H */