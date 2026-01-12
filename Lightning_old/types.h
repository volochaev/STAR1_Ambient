/**
 * @file types.h
 * @brief Common type definitions for LED lighting core.
 *
 * This header defines the base data structures used across
 * lighting, effects, scenes, and rendering layers.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
//
///* === Core LED buffer type ============================================== */
//
///**
// * @struct led_strip_t
// * @brief Basic structure representing one WS2812B-compatible LED strip.
// */
//typedef struct {
//    uint8_t  *grb;        ///< GRB data buffer (3 * led_count bytes)
//    uint16_t  led_count;  ///< number of LEDs
//    uint8_t   power_on;   ///< power state flag (optional)
//} led_strip_t;
//
///* Backward alias if старый код использует ws2812_t */
//typedef led_strip_t ws2812_t;

/* === Scene / FX types ================================================== */

/**
 * @brief Effect ID enumeration (matches effects.c)
 */
typedef enum {
    FX_SOLID_GRADIENT = 0,   // статичный плавный градиент по палитре
    FX_GRADIENT_FLOW,        // медленный сдвиг градиента по палитре
    FX_SOFT_BREATHE,         // лёгкое "дыхание" яркости по всей ленте
    FX_DUAL_ZONE,            // две зоны с разным offset по палитре
    FX_TWIN_WAVE,            // две мягкие бегущие волны
    FX_MB_OCEAN_FLOW,        // глубокий плавный градиент (MB ocean-style)
    FX_MB_TWO_TONE_WAVE,     // две тональные волны с разных сторон
    FX_MB_ENERGIZE_PULSE,    // активный мультицветной пульс
    FX_WELCOME_SWEEP,        // one-shot welcome
    FX_GOODBYE_FADE,         // one-shot goodbye
    FX_MB_WELCOME,           // алиас для MB-style welcome
    FX_MB_GOODBYE,           // алиас для MB-style goodbye
    FX_MAX_
} fx_id_t;

/**
 * @brief State object for continuous effects.
 */
typedef struct {
    float t;        ///< накопленное "время" (сек или абстракция)
    float phase;    ///< доп. фаза/offset
    float speed;    ///< скорость анимации (>0)
    float a, b;     ///< произвольные параметры (яркость, ширина и т.п.)
} fx_state_t;

/* === Scene player structures ========================================== */

/**
 * @brief Playback stages (scene_player.c)
 */
typedef enum {
    PST_IDLE = 0,
    PST_INTRO,
    PST_BRIDGE,
    PST_SCENE,
    PST_OUTRO
} player_stage_t;

/**
 * @brief Generic one-shot effect descriptor (welcome / goodbye)
 */
typedef struct {
    fx_id_t  fx_id;
    uint32_t start_ms;
    uint32_t duration_ms;
    float    progress;   ///< 0..1
    uint8_t  active;
} oneshot_t;

/**
 * @brief Scene player context (used by main.c / player_tick)
 */
typedef struct {
    player_stage_t stage;        ///< current playback stage
    uint32_t t0_ms;              ///< timestamp of stage start
    float    calc_brightness;    ///< resulting brightness (after dimming)
    float    theme_brightness;   ///< current theme brightness (internal)
    float    theme_dimming;      ///< user dimming (0..1)
    uint8_t  theme;              ///< current theme id
    oneshot_t intro;             ///< welcome
    oneshot_t outro;             ///< goodbye
    fx_state_t st_scene;         ///< state for continuous scene
} scene_player_t;

/* === Helper macros ===================================================== */

#ifndef CLAMP01
#define CLAMP01(x)  ((x) < 0.0f ? 0.0f : ((x) > 1.0f ? 1.0f : (x)))
#endif
