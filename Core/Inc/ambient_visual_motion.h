/**
 ******************************************************************************
 * @file    ambient_visual_motion.h
 * @brief   Motion dynamics, smoothing and spatial behavior tuning
 ******************************************************************************
 */

#pragma once

#include <stdint.h>
#include "ambient_feature_flags.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Handle PWM polarity */
#define AMB_HANDLE_PWM_ACTIVE_LOW 0u

/* BSM dynamics */
#define AMB_BSM_BLINK_HZ            1.8f
#define AMB_BSM_OVERLAY_GAIN_STRIP  0.92f
#define AMB_BSM_OVERLAY_GAIN_HANDLE 1.00f
#define AMB_BSM_ENV_ATTACK_ALPHA    0.26f
#define AMB_BSM_ENV_RELEASE_ALPHA   0.12f
#define AMB_PTS_ENV_ATTACK_ALPHA    1.00f
#define AMB_PTS_ENV_RELEASE_ALPHA   1.00f

/* Unlock welcome signature */
#define AMB_UNLOCK_SIGNATURE_GAIN           0.72f
#define AMB_UNLOCK_SIGNATURE_NIGHT_GAIN     0.65f
#define AMB_UNLOCK_SIGNATURE_SPREAD_NORM    0.13f
#define AMB_UNLOCK_SIGNATURE_CENTER_BOOST   0.34f

/* HVAC-derived dynamics */
#define AMB_HVAC_TRAIL_SPREAD_NORM       0.24f
#define AMB_HVAC_TRAIL_GAIN              1.45f
#define AMB_HVAC_TEMP_EVENT_STRIP_GAIN_WARM 1.45f
#define AMB_HVAC_TEMP_EVENT_STRIP_GAIN_COOL 1.45f
#define AMB_HVAC_WAVE_BG_DIM             0.94f
#define AMB_HVAC_WAVE_OVERLAY_GAIN_WARM  0.92f
#define AMB_HVAC_WAVE_OVERLAY_GAIN_COOL  0.96f
#define AMB_HVAC_WAVE_EDGE_ENVELOPE      0.20f
#define AMB_CLIMATE_MEMORY_GAIN          0.00f
#define AMB_HVAC_SPLIT_MIN_DELTA_RAW     3u
#define AMB_HVAC_SPLIT_FULL_DELTA_RAW    16u
#define AMB_HVAC_SPLIT_GAIN              0.00f
#define AMB_HVAC_FAN_MOTION_MIN          0.86f
#define AMB_HVAC_FAN_MOTION_MAX          1.30f
#define AMB_HVAC_FAN_JITTER_DEADBAND     0.015f
#define AMB_NIGHT_MOTION_SPEED_SCALE     0.82f

/* Seat heat overlay amplitudes */
#define AMB_SEAT_HEAT_OVERLAY_GAIN_STRIP    0.10f
#define AMB_SEAT_HEAT_OVERLAY_GAIN_FOOTWELL 0.18f
#define AMB_SEAT_HEAT_OVERLAY_GAIN_STORAGE  0.12f

/* Motion profile system */
#ifndef AMB_DEFAULT_MOTION_PROFILE
#define AMB_DEFAULT_MOTION_PROFILE 0u
#endif
#define AMB_MOTION_PROFILE_SCALE_LUXURY 1.00f
#define AMB_MOTION_PROFILE_SCALE_CALM   0.84f
#define AMB_MOTION_PROFILE_SCALE_SPORT  1.20f
#define AMB_DRIVE_MODE_BOOST_GAIN_LUXURY 0.08f
#define AMB_DRIVE_MODE_BOOST_GAIN_CALM   0.05f
#define AMB_DRIVE_MODE_BOOST_GAIN_SPORT  0.14f
#define AMB_FRAME_SLEW_STEP_BASE         5u
#define AMB_FRAME_SLEW_SCALE_LUXURY      1.00f
#define AMB_FRAME_SLEW_SCALE_CALM        0.82f
#define AMB_FRAME_SLEW_SCALE_SPORT       1.28f
#define AMB_PROFILE_BLEND_GAMMA_LUXURY   1.00f
#define AMB_PROFILE_BLEND_GAMMA_CALM     1.16f
#define AMB_PROFILE_BLEND_GAMMA_SPORT    0.84f
#define AMB_PROFILE_SMOOTH_ALPHA_MUL_LUXURY 1.00f
#define AMB_PROFILE_SMOOTH_ALPHA_MUL_CALM   1.08f
#define AMB_PROFILE_SMOOTH_ALPHA_MUL_SPORT  0.88f

/* Drive mode CAN mapping */
#ifndef AMB_DRIVE_MODE_CAN_ID
#define AMB_DRIVE_MODE_CAN_ID 0x38Eu
#endif
#ifndef AMB_DRIVE_MODE_BYTE_INDEX
#define AMB_DRIVE_MODE_BYTE_INDEX 1u
#endif
#ifndef AMB_DRIVE_MODE_BIT_SHIFT
#define AMB_DRIVE_MODE_BIT_SHIFT 2u
#endif
#ifndef AMB_DRIVE_MODE_BIT_MASK
#define AMB_DRIVE_MODE_BIT_MASK 0x07u
#endif
#ifndef AMB_DRIVE_MODE_VALUE_COMFORT
#define AMB_DRIVE_MODE_VALUE_COMFORT 0u
#endif
#ifndef AMB_DRIVE_MODE_VALUE_SPORT
#define AMB_DRIVE_MODE_VALUE_SPORT 1u
#endif
#ifndef AMB_DRIVE_MODE_VALUE_CALM
#define AMB_DRIVE_MODE_VALUE_CALM 5u
#endif
#ifndef AMB_DRIVE_MODE_VALUE_SPORT_PLUS
#define AMB_DRIVE_MODE_VALUE_SPORT_PLUS 2u
#endif
#ifndef AMB_DRIVE_MODE_VALUE_SLEEK
#define AMB_DRIVE_MODE_VALUE_SLEEK 4u
#endif
#ifndef AMB_DRIVE_MODE_VALUE_INDIVIDUAL
#define AMB_DRIVE_MODE_VALUE_INDIVIDUAL 3u
#endif

/* Transition and personality dynamics */
#define AMB_CABIN_DEPTH_LR_GAIN          0.85f
#define AMB_CABIN_DEPTH_FR_GAIN          1.00f
#define AMB_CABIN_DEPTH_U_SPREAD         0.10f
#define AMB_CABIN_DEPTH_PHASE_SHIFT      1.35f
#define AMB_CABIN_DEPTH_BRIGHTNESS_DELTA 0.08f
#define AMB_BANK_CHARACTER_LERP_ALPHA      0.78f
#define AMB_BANK_CHARACTER_SPEED_MIN        0.84f
#define AMB_BANK_CHARACTER_SPEED_MAX        1.22f
#define AMB_THEME_DYNAMIC_SCALE_MIN         0.78f
#define AMB_THEME_DYNAMIC_SCALE_MAX         1.35f
#ifndef AMB_TRANSITION_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_TRANSITION_SMOOTH_ALPHA  0.74f
#else
#define AMB_TRANSITION_SMOOTH_ALPHA  0.86f
#endif
#endif
#ifndef AMB_IDLE_MICRO_MOTION_AMPLITUDE
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_IDLE_MICRO_MOTION_AMPLITUDE 0.010f
#else
#define AMB_IDLE_MICRO_MOTION_AMPLITUDE 0.018f
#endif
#endif
#ifndef AMB_IDLE_MICRO_MOTION_HZ
#define AMB_IDLE_MICRO_MOTION_HZ 0.08f
#endif

/* Effect shaping */
#define FX_WELCOME_TIME_SCALE       1.02f
#define FX_WELCOME_WAVE_START       0.08f
#define FX_WELCOME_WAVE_DURATION    0.62f
#define FX_WELCOME_WAVE_GAIN_MAX    0.28f
#define FX_WELCOME_PULSE_START      0.50f
#define FX_WELCOME_PULSE_DURATION   0.28f
#define FX_WELCOME_PULSE_AMPLITUDE  0.20f
#define FX_WELCOME_CENTER_OFFSET    0.50f
#define FX_WELCOME_DIST_SCALE       0.22f
#define FX_WELCOME_SETTLE_START     0.78f
#define FX_WELCOME_FINAL_SCALE      1.00f
#define FX_GOODBYE_CURTAIN_SCALE    1.05f
#define AMB_FX_TEMPORAL_SMOOTH      0.86f
#define FX_SPATIAL_GROUP            4u

#ifdef __cplusplus
}
#endif
