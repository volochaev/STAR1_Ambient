/**
 ******************************************************************************
 * @file    ambient_feature_flags.h
 * @brief   Compile-time feature switches for ambient firmware
 * @details This file contains only enable/disable style flags and profile
 *          selection. Numeric tuning constants are defined in
 *          `ambient_tuning.h`.
 ******************************************************************************
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== DEPLOYMENT PROFILE ============================================== */
#define AMB_PROFILE_BENCH      1u
#define AMB_PROFILE_PRODUCTION 2u

#ifndef AMB_PROFILE
#ifdef DEBUG
#define AMB_PROFILE AMB_PROFILE_BENCH
#else
#define AMB_PROFILE AMB_PROFILE_PRODUCTION
#endif
#endif

/* ====== CORE PIPELINE ==================================================== */
/* Apply gamma correction before output (color/brightness linearization). */
#ifndef AMB_ENABLE_GAMMA
#define AMB_ENABLE_GAMMA 1u
#endif
/* Temporal/spatial dithering to reduce low-level banding/flicker artifacts. */
#ifndef AMB_ENABLE_DITHERING
#define AMB_ENABLE_DITHERING 1u
#endif
/* Enable per-zone FX engine (non-strip zones can animate independently). */
#ifndef AMB_ENABLE_ZONE_FX
#define AMB_ENABLE_ZONE_FX 1u
#endif
/* Feed IWDG from runtime loop; disable only for deep debug sessions. */
#ifndef AMB_ENABLE_WATCHDOG
#define AMB_ENABLE_WATCHDOG 1u
#endif

/* ====== OVERLAYS / EVENTS =============================================== */
/* Render blind-spot warning overlay when BSM indicates active warning. */
#ifndef AMB_ENABLE_BSM_OVERLAY
#define AMB_ENABLE_BSM_OVERLAY 1u
#endif
/* Accept any 0x17E frame as BSM activity (bench fallback, less strict decode). */
#ifndef AMB_BSM_ACCEPT_ANY_17E
#define AMB_BSM_ACCEPT_ANY_17E 0u
#endif
/* Toggle debug pulse/indicator path for BSM RX diagnostics. */
#ifndef AMB_DEBUG_BSM_RX_PULSE
#define AMB_DEBUG_BSM_RX_PULSE 0u
#endif

/* Allow decorative welcome event layer scene during startup/theme entry. */
#ifndef AMB_ENABLE_WELCOME_EVENT_SCENE
#define AMB_ENABLE_WELCOME_EVENT_SCENE 1u
#endif
/* Enable dedicated unlock accent signature scene (extra cosmetic event). */
#ifndef AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE
#define AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE 0u
#endif
/* Trigger unlock events from EIS_A2 explicit unlock fields. */
#ifndef AMB_ENABLE_UNLOCK_TRIGGER_EIS301
#define AMB_ENABLE_UNLOCK_TRIGGER_EIS301 0u
#endif
/* Trigger unlock/welcome after wake recovery path (sleep->run). */
#ifndef AMB_ENABLE_UNLOCK_TRIGGER_WAKE_RECOVER
#define AMB_ENABLE_UNLOCK_TRIGGER_WAKE_RECOVER 0u
#endif
/* Startup gate: require door-open condition to arm/start ambient scene. */
#ifndef AMB_ENABLE_START_GATE_DOOR_OPEN
#define AMB_ENABLE_START_GATE_DOOR_OPEN 1u
#endif
/* Bypass intro animation and jump directly to scene/welcome state. */
#ifndef AMB_DISABLE_WELCOME_INTRO
#define AMB_DISABLE_WELCOME_INTRO 0u
#endif
/* Enable door-open event choreography scene. */
#ifndef AMB_ENABLE_DOOR_OPEN_EVENT_SCENE
#define AMB_ENABLE_DOOR_OPEN_EVENT_SCENE 1u
#endif
/* Enable lock/goodbye event scene on lock transition. */
#ifndef AMB_ENABLE_LOCK_GOODBYE_EVENT
#define AMB_ENABLE_LOCK_GOODBYE_EVENT 1u
#endif
/* Use EIS turn-lamp indicators as trigger sources (generic gate). */
#ifndef AMB_ENABLE_EIS_TURNLMP_TRIGGERS
#define AMB_ENABLE_EIS_TURNLMP_TRIGGERS 1u
#endif
/* Use EIS turn-lamp unlock indication as unlock trigger source. */
#ifndef AMB_ENABLE_EIS_TURNLMP_UNLOCK_TRIGGER
#define AMB_ENABLE_EIS_TURNLMP_UNLOCK_TRIGGER 1u
#endif
/* Use EIS turn-lamp lock indication as lock trigger source. */
#ifndef AMB_ENABLE_EIS_TURNLMP_LOCK_TRIGGER
#define AMB_ENABLE_EIS_TURNLMP_LOCK_TRIGGER 0u
#endif
/* Fallback unlock/lock detection via EIS door-request bits. */
#ifndef AMB_ENABLE_EIS_DOOR_REQ_FALLBACK
#define AMB_ENABLE_EIS_DOOR_REQ_FALLBACK 0u
#endif
/* Enable HVAC temperature change event scene (warm/cool accent). */
#ifndef AMB_ENABLE_HVAC_TEMP_EVENT_SCENE
#define AMB_ENABLE_HVAC_TEMP_EVENT_SCENE 1u
#endif
/* Add moving trail component to HVAC event scene. */
#ifndef AMB_ENABLE_HVAC_DRAG_TRAIL
#define AMB_ENABLE_HVAC_DRAG_TRAIL 1u
#endif
/* Keep short-lived climate context memory after event completion. */
#ifndef AMB_ENABLE_CLIMATE_MEMORY
#define AMB_ENABLE_CLIMATE_MEMORY 1u
#endif
/* Split left/right climate response instead of single combined trend. */
#ifndef AMB_ENABLE_HVAC_DUAL_SPLIT
#define AMB_ENABLE_HVAC_DUAL_SPLIT 1u
#endif
/* Modulate motion by HVAC fan intensity. */
#ifndef AMB_ENABLE_HVAC_FAN_MOTION_MOD
#define AMB_ENABLE_HVAC_FAN_MOTION_MOD 1u
#endif
/* Enable comfort auto-dim logic:
 * applies context dim shaping (night/HVAC/scene conditions) to keep
 * perceived comfort and avoid over-bright secondary zones. */
#ifndef AMB_ENABLE_COMFORT_AUTO_DIM
#define AMB_ENABLE_COMFORT_AUTO_DIM 1u
#endif
/* Apply energy-aware trim:
 * perceptual balancing of saturation/brightness to keep palette readable
 * and avoid harsh clipping at bright/high-energy moments. */
#ifndef AMB_ENABLE_ENERGY_AWARE_TRIM
#define AMB_ENABLE_ENERGY_AWARE_TRIM 1u
#endif
/* Night calm profile:
 * extra nighttime softening (energy/chroma behavior) on top of base night dim. */
#ifndef AMB_ENABLE_NIGHT_CALM
#define AMB_ENABLE_NIGHT_CALM 1u
#endif
/* Couple seat-heater status into ambient modulation/event accents. */
#ifndef AMB_ENABLE_SEAT_HEAT_COUPLING
#define AMB_ENABLE_SEAT_HEAT_COUPLING 0u
#endif
/* Respect ILM dim requests from vehicle CAN (can reduce apparent daytime output). */
#ifndef AMB_ENABLE_ILM_DIM_REQUESTS
#define AMB_ENABLE_ILM_DIM_REQUESTS 1u
#endif
/* Handle source policy constants (door-handle/ambient request source selection). */
#define AMB_HANDLE_SOURCE_NSI          1u
#define AMB_HANDLE_SOURCE_IL_R_ON_RQ   2u
#define AMB_HANDLE_SOURCE_NS_ILL_ACTV  3u
#define AMB_HANDLE_SOURCE_PDDLLMP_ANY  4u
/* Active handle request source policy. */
#ifndef AMB_HANDLE_SOURCE_POLICY
#define AMB_HANDLE_SOURCE_POLICY AMB_HANDLE_SOURCE_PDDLLMP_ANY
#endif
/* Debug override: ignore reverse suppression for PDDLLMP handle requests. */
#ifndef AMB_PDDLLMP_IGNORE_REVERSE_FOR_DEBUG
#define AMB_PDDLLMP_IGNORE_REVERSE_FOR_DEBUG 0u
#endif
/* Use HU/TGW day-night signal as fallback night source when primary unavailable. */
#ifndef AMB_ENABLE_HU_DAYNIGHT_FALLBACK
#define AMB_ENABLE_HU_DAYNIGHT_FALLBACK 1u
#endif
/* Use LGTSENS 0x025 as primary/active night source path. */
#ifndef AMB_ENABLE_LGTSENS_NIGHT_SOURCE
#define AMB_ENABLE_LGTSENS_NIGHT_SOURCE 1u
#endif
/* Enable parking warning overlay from PTS signals. */
#ifndef AMB_ENABLE_PARKING_WARN_OVERLAY
#define AMB_ENABLE_PARKING_WARN_OVERLAY 1u
#endif
/* Enable reverse-scene postprocess:
 * while reverse is active, runtime applies dedicated tint+gain blend
 * (configured in ambient_color_brightness.h). */
#ifndef AMB_ENABLE_REVERSE_SCENE
#define AMB_ENABLE_REVERSE_SCENE 1u
#endif
/* Allow reverse signal contribution from SAM_F decode path. */
#ifndef AMB_ENABLE_REVERSE_SIGNAL_FROM_SAMF
#define AMB_ENABLE_REVERSE_SIGNAL_FROM_SAMF 0u
#endif
/* Allow reverse signal contribution from CHASSIS (0x10C) decode path. */
#ifndef AMB_ENABLE_REVERSE_SIGNAL_FROM_CHASSIS
#define AMB_ENABLE_REVERSE_SIGNAL_FROM_CHASSIS 1u
#endif
/* Enable solar intensity compensation (daylight gain). */
#ifndef AMB_ENABLE_SUN_INTENSITY_COMP
#define AMB_ENABLE_SUN_INTENSITY_COMP 1u
#endif
/* Enable Mercedes-like dual-color breathing overlay over active theme. */
#ifndef AMB_ENABLE_DUAL_COLOR_BREATH
#define AMB_ENABLE_DUAL_COLOR_BREATH 0u
#endif

/* ====== MOTION / THEMES ================================================== */
/* Auto-select motion profile from CAN/vehicle cues. */
#ifndef AMB_ENABLE_DRIVE_MODE_AUTOPROFILE
#define AMB_ENABLE_DRIVE_MODE_AUTOPROFILE 1u
#endif
/* Periodic automatic theme rotation inside active bank. */
#ifndef AMB_ENABLE_AUTO_ROTATE
#define AMB_ENABLE_AUTO_ROTATE 0u
#endif
/* Fast intra-bank theme switches (reduced transitional overhead). */
#ifndef AMB_ENABLE_FAST_BANK_SWITCH
#define AMB_ENABLE_FAST_BANK_SWITCH 1u
#endif
/* Unified theme transition model:
 * both strip and zone layers use shared crossfade/bridge logic,
 * reducing abrupt mismatches during bank/theme changes. */
#ifndef AMB_ENABLE_UNIFIED_THEME_TRANSITION
#define AMB_ENABLE_UNIFIED_THEME_TRANSITION 1u
#endif
/* Theme personality layer:
 * each theme carries its own motion "character" (phase offsets, speed bias,
 * depth feel), so same FX family looks different per theme. */
#ifndef AMB_ENABLE_THEME_PERSONALITY
#define AMB_ENABLE_THEME_PERSONALITY 1u
#endif
/* Cabin depth model (complex feature):
 * computes virtual cabin pose/depth and applies zone-dependent modulation:
 * - palette position shift (u-shift),
 * - phase shift,
 * - slight brightness delta by zone depth weight.
 * Goal: make door/storage/footwell feel spatially separated instead of flat.
 * Implemented in zones/base-scene path; does not require extra CAN signals. */
#ifndef AMB_ENABLE_CABIN_DEPTH_MODEL
#define AMB_ENABLE_CABIN_DEPTH_MODEL 1u
#endif
/* Bank character memory:
 * stores per-bank motion-speed adaptation derived from runtime observations,
 * then reuses it on future entries to keep bank behavior consistent. */
#ifndef AMB_ENABLE_BANK_CHARACTER_MEMORY
#define AMB_ENABLE_BANK_CHARACTER_MEMORY 1u
#endif

/* ====== POWER ============================================================ */
/* Enable sleep/stop mode logic for low-power behavior. */
#ifndef AMB_ENABLE_SLEEP_MODE
#define AMB_ENABLE_SLEEP_MODE 1u
#endif

/* Door-handle surrounding illumination source policy */
/* Trust request field from OEM 0x325 only. */
#define AMB_SURRILL_SOURCE_REQUEST_325 1u
/* Trust state from 0x006 only. */
#define AMB_SURRILL_SOURCE_STATE_006   2u
/* Hybrid reconciliation between request/state sources. */
#define AMB_SURRILL_SOURCE_HYBRID      3u
/* Active surrounding illumination source policy. */
#ifndef AMB_SURRILL_SOURCE_POLICY
#define AMB_SURRILL_SOURCE_POLICY AMB_SURRILL_SOURCE_HYBRID
#endif

/* OEM color field source policy for IC_A8 (0x325) */
/* Read OEM color from bits [3:2]. */
#define AMB_OEM_COLOR_SRC_BITS_3_2 1u
/* Read OEM color from bits [5:4]. */
#define AMB_OEM_COLOR_SRC_BITS_5_4 2u
/* Auto-detect and lock best-matching color bit source. */
#define AMB_OEM_COLOR_SRC_AUTO     3u
/* Active OEM color source policy. */
#ifndef AMB_OEM_COLOR_SOURCE_POLICY
#define AMB_OEM_COLOR_SOURCE_POLICY AMB_OEM_COLOR_SRC_BITS_3_2
#endif

/* Bench wake policy: permissive wake behavior for testing. */
#define AMB_CAN_WAKE_BENCH       0u
/* Production wake policy: stricter wake semantics. */
#define AMB_CAN_WAKE_PRODUCTION  1u

/* Active CAN wake policy (selected by profile unless overridden). */
#ifndef AMB_CAN_WAKE_POLICY
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_CAN_WAKE_POLICY AMB_CAN_WAKE_BENCH
#else
#define AMB_CAN_WAKE_POLICY AMB_CAN_WAKE_PRODUCTION
#endif
#endif

/* Enable dedicated transceiver WAKE/INT pin (PB7) as STOP wake source. */
#ifndef AMB_ENABLE_TRANSCEIVER_WAKE_PIN
#define AMB_ENABLE_TRANSCEIVER_WAKE_PIN 0u
#endif

#ifdef __cplusplus
}
#endif
