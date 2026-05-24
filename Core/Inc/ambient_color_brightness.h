/**
 ******************************************************************************
 * @file    ambient_color_brightness.h
 * @brief   Colorimetry and brightness transfer tuning
 ******************************************************************************
 */

#pragma once

#include <stdint.h>
#include "ambient_feature_flags.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Gamma and saturation */
#define AMB_GAMMA_EXP           2.2f
#define AMB_GAMMA_MODE_LEGACY   0u
#define AMB_GAMMA_MODE_LINEAR   1u
#define AMB_GAMMA_MODE          AMB_GAMMA_MODE_LEGACY
#define AMB_SATURATION_BOOST    1.18f

/* Theme/scene brightness baseline */
#define AMB_THEME_MIN_BRIGHTNESS    0.70f
#define AMB_THEME_GLOBAL_BRIGHTNESS_BOOST 1.18f
#define AMB_ZONE_STRIP_BRIGHTNESS_BOOST   1.00f
#define AMB_ZONE_HANDLE_BRIGHTNESS_BOOST  1.00f
#define AMB_ZONE_STORAGE_BRIGHTNESS_BOOST 1.00f
#define AMB_ZONE_FOOTWELL_BRIGHTNESS_BOOST 1.00f
/* Additional daytime lift for secondary zones (handle/storage/footwell). */
#define AMB_DAY_NONSTRIP_GAIN             1.25f

/* HVAC/seat accent colors */
#define AMB_DOOR_OPEN_ACCENT_R             255u
#define AMB_DOOR_OPEN_ACCENT_G             178u
#define AMB_DOOR_OPEN_ACCENT_B             74u

/* Profile tint/energy mapping */
#define AMB_MOTION_PROFILE_TINT_LUXURY_R 1.00f
#define AMB_MOTION_PROFILE_TINT_LUXURY_G 1.00f
#define AMB_MOTION_PROFILE_TINT_LUXURY_B 1.00f
#define AMB_MOTION_PROFILE_TINT_CALM_R   1.00f
#define AMB_MOTION_PROFILE_TINT_CALM_G   1.00f
#define AMB_MOTION_PROFILE_TINT_CALM_B   1.00f
#define AMB_MOTION_PROFILE_TINT_SPORT_R  1.03f
#define AMB_MOTION_PROFILE_TINT_SPORT_G  1.00f
#define AMB_MOTION_PROFILE_TINT_SPORT_B  0.97f
#define AMB_ENERGY_SAT_LUXURY            1.00f
#define AMB_ENERGY_SAT_CALM              0.94f
#define AMB_ENERGY_SAT_SPORT             1.06f
#define AMB_ENERGY_WHITEPOINT_NIGHT      0.96f
/* Low-level chroma stabilization (cheap-strip hue drift compensation at low PWM). */
#define AMB_LOW_LEVEL_CHROMA_ENABLE      1u
#define AMB_LOW_LEVEL_CHROMA_START       0.24f
#define AMB_LOW_LEVEL_CHROMA_END         0.06f
#define AMB_LOW_LEVEL_SAT_BOOST          1.18f
#define AMB_LOW_LEVEL_CODE_FLOOR         3u

/* DBC-driven dim/scene modifiers */
#define AMB_ILM_DIM_SCALE_FRONT          0.98f
#define AMB_ILM_DIM_SCALE_REAR           0.98f
#define AMB_ILM_DIM_SCALE_BG             1.00f
#define AMB_ILM_DIM_SMOOTH_ALPHA         0.86f
#define AMB_REVERSE_SCENE_GAIN           1.03f
#define AMB_REVERSE_SCENE_TINT_R         0.97f
#define AMB_REVERSE_SCENE_TINT_G         0.99f
#define AMB_REVERSE_SCENE_TINT_B         1.05f
#define AMB_REVERSE_SCENE_SMOOTH_ALPHA   0.84f
#define AMB_SUN_GAIN_MAX                 0.20f
#define AMB_SUN_GAIN_SMOOTH_ALPHA        0.88f
#define AMB_PARKING_OVERLAY_GAIN_STRIP   0.72f
#define AMB_PARKING_OVERLAY_GAIN_HANDLE  0.86f
#define AMB_DOOR_PRE_GLOW_GAIN_HANDLE    1.12f
#define AMB_DOOR_PRE_GLOW_GAIN_FOOTWELL  0.88f
#define AMB_DOOR_CHOREO_STRIP_GAIN       0.82f
#define AMB_CONTEXT_HOLD_GAIN_CLIMATE    0.12f
#define AMB_CONTEXT_HOLD_GAIN_DOOR       0.10f
#define AMB_CONTEXT_HOLD_GAIN_PARKING    0.13f
#define AMB_CONTEXT_HOLD_GAIN_REVERSE    0.06f
/* Dual-color breathing overlay (premium dynamic accent) */
#define AMB_DUAL_BREATH_PERIOD_SEC       60.0f
#define AMB_DUAL_BREATH_MIX_STRIP        0.16f
#define AMB_DUAL_BREATH_MIX_HANDLE       0.30f
#define AMB_DUAL_BREATH_MIX_STORAGE      0.36f
#define AMB_DUAL_BREATH_MIX_FOOTWELL     0.32f

/* Comfort and night brightness */
#define AMB_NIGHT_BRIGHTNESS_SCALE        0.70f
#define AMB_COMFORT_DIM_FOOTWELL_NIGHT_HVAC 1.00f
#define AMB_COMFORT_DIM_STORAGE_NIGHT_HVAC  1.00f

/* OEM brightness transfer curve */
#define AMB_BRIGHTNESS_FLOOR   0.10f
#define AMB_BRIGHTNESS_CEIL    1.00f
#define AMB_BRIGHTNESS_EXP     1.00f
#ifndef AMB_DIMMING_ATTACK_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_DIMMING_ATTACK_MS  380u
#else
#define AMB_DIMMING_ATTACK_MS  450u
#endif
#endif
#ifndef AMB_DIMMING_RELEASE_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_DIMMING_RELEASE_MS 760u
#else
#define AMB_DIMMING_RELEASE_MS 900u
#endif
#endif
#ifndef AMB_DIMMING_POST_SMOOTH
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_DIMMING_POST_SMOOTH 0.70f
#else
#define AMB_DIMMING_POST_SMOOTH 0.82f
#endif
#endif

/* Crossfade / bridge brightness blending */
#define AMB_CROSSFADE_SCREEN_MIX        0.18f
#define AMB_BRIDGE_SCREEN_MIX           0.10f
#ifndef AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA 0.55f
#else
#define AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA 0.78f
#endif
#endif
#ifndef AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA 0.64f
#else
#define AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA 0.84f
#endif
#endif
#ifndef AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA 0.72f
#else
#define AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA 0.90f
#endif
#endif

/* Runtime flag synchronized from CAN and consumed in render */
extern uint8_t g_night_mode_state;

#ifdef __cplusplus
}
#endif
