/**
 ******************************************************************************
 * @file    features.h
 * @brief   Feature flags and configuration for ambient lighting
 * @details Compile-time and runtime feature toggles for advanced lighting
 *          features including gamma correction, dithering, and night mode.
 *
 * @section Features
 * Compile-time features (via defines):
 * - AMB_ENABLE_GAMMA: Gamma correction for better color accuracy
 * - AMB_ENABLE_DITHERING: Temporal dithering for smoother gradients
 * - AMB_ENABLE_ZONE_FX: Enable zone-specific effects
 * - AMB_ENABLE_WATCHDOG: Independent watchdog for system safety
 *
 * Runtime features:
 * - Night mode: Reduces brightness via AMB_NIGHT_BRIGHTNESS_SCALE
 *
 * @version 2.1
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== FEATURE TOGGLES (compile-time) ================================= */

/* ====== DEPLOYMENT PROFILE ============================================== */
/* Set via compiler define:
 * -DAMB_PROFILE=1  (BENCH)
 * -DAMB_PROFILE=2  (PRODUCTION)
 */
#define AMB_PROFILE_BENCH      1u
#define AMB_PROFILE_PRODUCTION 2u

#ifndef AMB_PROFILE
#ifdef DEBUG
#define AMB_PROFILE AMB_PROFILE_BENCH
#else
#define AMB_PROFILE AMB_PROFILE_PRODUCTION
#endif
#endif

#define AMB_ENABLE_GAMMA        1       /* 1 = enable gamma correction */
#define AMB_GAMMA_EXP           2.2f    /* 2.2 = ближе к реальному восприятию, меньше "выцветания" на макс. яркости */
#define AMB_GAMMA_MODE_LEGACY   0u      /* per-channel pow(x, gamma) */
#define AMB_GAMMA_MODE_LINEAR   1u      /* per-channel pow(x, 1/gamma) */
#define AMB_GAMMA_MODE          AMB_GAMMA_MODE_LEGACY
#define AMB_SATURATION_BOOST    1.18f   /* >1.0 increases color richness after gamma */
#define AMB_ENABLE_DITHERING    1       /* 1 = enable temporal dithering */
#define AMB_ENABLE_ZONE_FX      1       /* 1 = enable zone effects (breath/flow) */
#define AMB_ENABLE_WATCHDOG     1       /* 1 = enable IWDG watchdog */
#ifndef AMB_WATCHDOG_TIMEOUT_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_WATCHDOG_TIMEOUT_MS 8000u   /* Bench: меньше ложных reset/яркостных скачков при отладке */
#else
#define AMB_WATCHDOG_TIMEOUT_MS 8000u   /* Production: консервативно, устойчиво к коротким задержкам */
#endif
#endif

/* ====== HANDLE PWM (PT4211) ============================================= */
/* 1 = active-low DIM (sink-to-GND control), 0 = active-high DIM */
#define AMB_HANDLE_PWM_ACTIVE_LOW 0u
/* Perceptual slew for handle PWM (ms) */
#ifndef AMB_HANDLE_PWM_ATTACK_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_HANDLE_PWM_ATTACK_MS  900u   /* Bench: быстрее отклик ручки для отладки */
#else
#define AMB_HANDLE_PWM_ATTACK_MS  1400u  /* Production: мягкий premium разгон */
#endif
#endif
#ifndef AMB_HANDLE_PWM_RELEASE_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_HANDLE_PWM_RELEASE_MS 1500u  /* Bench: ускоренный цикл проверки */
#else
#define AMB_HANDLE_PWM_RELEASE_MS 2200u  /* Production: длинный плавный спад */
#endif
#endif

/* ====== BSM INTERRUPT OVERLAY =========================================== */
/* W223/EQS-style blind-spot warning pulse above ambient scene */
#define AMB_ENABLE_BSM_OVERLAY      1u
#define AMB_BSM_BLINK_HZ            1.8f
#define AMB_BSM_OVERLAY_GAIN_STRIP  0.92f
#define AMB_BSM_OVERLAY_GAIN_HANDLE 1.00f
#define AMB_BSM_ENV_ATTACK_ALPHA    0.26f  /* Плавный вход в BSM */
#define AMB_BSM_ENV_RELEASE_ALPHA   0.12f  /* Мягкий спад */
/* Bench helper: any received 0x17E frame forces visible BSM overlay.
 * Use for bring-up when payload mapping is unknown; disable for production. */
#ifndef AMB_BSM_ACCEPT_ANY_17E
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_BSM_ACCEPT_ANY_17E      1u
#else
#define AMB_BSM_ACCEPT_ANY_17E      0u
#endif
#endif
/* Bench diagnostic: hard pulse strip on every received 0x17E.
 * This bypasses overlay logic and helps verify RX path quickly. */
#ifndef AMB_DEBUG_BSM_RX_PULSE
#define AMB_DEBUG_BSM_RX_PULSE      0u
#endif
/* BSM frame hold timeout: if no new 0x17E within this time,
 * overlay state is treated as inactive. */
#ifndef AMB_BSM_RX_TIMEOUT_MS
#define AMB_BSM_RX_TIMEOUT_MS       220u
#endif

/* ====== EVENT SCENES ==================================================== */
#ifndef AMB_ENABLE_WELCOME_EVENT_SCENE
#define AMB_ENABLE_WELCOME_EVENT_SCENE 1u
#endif
#define AMB_WELCOME_EVENT_HOLD_MS      900u

#ifndef AMB_ENABLE_HVAC_TEMP_EVENT_SCENE
#define AMB_ENABLE_HVAC_TEMP_EVENT_SCENE 1u
#endif
#define AMB_HVAC_TEMP_EVENT_HOLD_MS      420u
#define AMB_HVAC_TEMP_EVENT_COOLDOWN_MS  700u
#define AMB_HVAC_TEMP_EVENT_BURST_WINDOW_MS 1100u
#define AMB_HVAC_TEMP_EVENT_BURST_CYCLES    3u
#define AMB_HVAC_TEMP_WARM_R             255u
#define AMB_HVAC_TEMP_WARM_G             120u
#define AMB_HVAC_TEMP_WARM_B             24u
#define AMB_HVAC_TEMP_COOL_R             86u
#define AMB_HVAC_TEMP_COOL_G             156u
#define AMB_HVAC_TEMP_COOL_B             255u

#ifndef AMB_ENABLE_HVAC_DRAG_TRAIL
#define AMB_ENABLE_HVAC_DRAG_TRAIL       1u
#endif
#define AMB_HVAC_TRAIL_SPREAD_NORM       0.16f
#define AMB_HVAC_TRAIL_GAIN              0.72f

#ifndef AMB_ENABLE_CLIMATE_MEMORY
#define AMB_ENABLE_CLIMATE_MEMORY        1u
#endif
#define AMB_CLIMATE_MEMORY_HOLD_MS       7000u
#define AMB_CLIMATE_MEMORY_GAIN          0.10f

#ifndef AMB_ENABLE_HVAC_DUAL_SPLIT
#define AMB_ENABLE_HVAC_DUAL_SPLIT       1u
#endif
#define AMB_HVAC_SPLIT_MIN_DELTA_RAW     3u
#define AMB_HVAC_SPLIT_FULL_DELTA_RAW    16u
#define AMB_HVAC_SPLIT_GAIN              0.12f
#define AMB_HVAC_SPLIT_WARM_R            255u
#define AMB_HVAC_SPLIT_WARM_G            142u
#define AMB_HVAC_SPLIT_WARM_B            40u
#define AMB_HVAC_SPLIT_COOL_R            96u
#define AMB_HVAC_SPLIT_COOL_G            172u
#define AMB_HVAC_SPLIT_COOL_B            255u

#ifndef AMB_ENABLE_HVAC_FAN_MOTION_MOD
#define AMB_ENABLE_HVAC_FAN_MOTION_MOD   1u
#endif
#define AMB_HVAC_FAN_MOTION_MIN          0.86f
#define AMB_HVAC_FAN_MOTION_MAX          1.30f

#ifndef AMB_ENABLE_COMFORT_AUTO_DIM
#define AMB_ENABLE_COMFORT_AUTO_DIM      1u
#endif
#define AMB_COMFORT_DIM_FOOTWELL_NIGHT_HVAC 0.76f
#define AMB_COMFORT_DIM_STORAGE_NIGHT_HVAC  0.82f

#ifndef AMB_ENABLE_ENERGY_AWARE_TRIM
#define AMB_ENABLE_ENERGY_AWARE_TRIM     1u
#endif
#define AMB_ENERGY_SAT_LUXURY            1.00f
#define AMB_ENERGY_SAT_CALM              0.94f
#define AMB_ENERGY_SAT_SPORT             1.06f
#define AMB_ENERGY_WHITEPOINT_NIGHT      0.96f

#ifndef AMB_ENABLE_NIGHT_CALM
#define AMB_ENABLE_NIGHT_CALM            1u
#endif
#define AMB_NIGHT_MOTION_SPEED_SCALE     0.82f

#ifndef AMB_ENABLE_SEAT_HEAT_COUPLING
#define AMB_ENABLE_SEAT_HEAT_COUPLING    1u
#endif
#define AMB_SEAT_HEAT_OVERLAY_GAIN_STRIP    0.10f
#define AMB_SEAT_HEAT_OVERLAY_GAIN_FOOTWELL 0.18f
#define AMB_SEAT_HEAT_OVERLAY_GAIN_STORAGE  0.12f
#define AMB_SEAT_HEAT_WARM_R                255u
#define AMB_SEAT_HEAT_WARM_G                128u
#define AMB_SEAT_HEAT_WARM_B                34u

/* ====== MOTION PROFILE (Luxury/Calm/Sport) ============================== */
#ifndef AMB_DEFAULT_MOTION_PROFILE
#define AMB_DEFAULT_MOTION_PROFILE 0u
#endif
#define AMB_MOTION_PROFILE_SCALE_LUXURY 1.00f
#define AMB_MOTION_PROFILE_SCALE_CALM   0.84f
#define AMB_MOTION_PROFILE_SCALE_SPORT  1.20f
#define AMB_MOTION_PROFILE_TINT_LUXURY_R 1.00f
#define AMB_MOTION_PROFILE_TINT_LUXURY_G 1.00f
#define AMB_MOTION_PROFILE_TINT_LUXURY_B 1.00f
#define AMB_MOTION_PROFILE_TINT_CALM_R   1.00f
#define AMB_MOTION_PROFILE_TINT_CALM_G   1.00f
#define AMB_MOTION_PROFILE_TINT_CALM_B   1.00f
#define AMB_MOTION_PROFILE_TINT_SPORT_R  1.03f
#define AMB_MOTION_PROFILE_TINT_SPORT_G  1.00f
#define AMB_MOTION_PROFILE_TINT_SPORT_B  0.97f
#define AMB_DRIVE_MODE_BOOST_DURATION_MS 900u
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

/* Auto profile switch from vehicle drive mode CAN signal.
 * Set AMB_DRIVE_MODE_CAN_ID to real frame ID from DBC to enable. */
#ifndef AMB_ENABLE_DRIVE_MODE_AUTOPROFILE
#define AMB_ENABLE_DRIVE_MODE_AUTOPROFILE 1u
#endif
#ifndef AMB_DRIVE_MODE_CAN_ID
#define AMB_DRIVE_MODE_CAN_ID 0x38Eu   /* DBC: BO_910 SPP_STAT_R1 */
#endif
#ifndef AMB_DRIVE_MODE_BYTE_INDEX
#define AMB_DRIVE_MODE_BYTE_INDEX 1u    /* SG SPP_DrvProg_Rq: start bit 10 -> byte1 */
#endif
#ifndef AMB_DRIVE_MODE_BIT_SHIFT
#define AMB_DRIVE_MODE_BIT_SHIFT 2u     /* SG SPP_DrvProg_Rq: bit2..4 of byte1 */
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
#define AMB_DRIVE_MODE_VALUE_CALM 5u     /* ECO -> calm */
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

/* ====== AUTO-ROTATE (theme cycling within bank) ======================== */

#define AMB_ENABLE_AUTO_ROTATE          0       /* 1 = auto-rotate themes within bank */
#define AMB_AUTO_ROTATE_INTERVAL_SEC    10u     /* Theme change interval (seconds) */
#define AMB_CROSSFADE_DURATION_MS       1600u   /* Crossfade duration (ms) — вернули исходную плавность */
#define AMB_ENABLE_FAST_BANK_SWITCH     1u      /* 1 = active cross-bank switch uses crossfade path */
#define AMB_FAST_BANK_SWITCH_DEBOUNCE_MS 280u   /* Ignore overly dense bank-switch bursts */
#define AMB_MAX_CROSSFADE_LEDS          256u    /* Max LEDs for crossfade temp buffer */
#define AMB_CROSSFADE_SCREEN_MIX        0.35f   /* Доля screen mix в середине кроссфейда (0..1) */

/* ====== THEME PERSONALITY / CABIN DEPTH ================================= */
#define AMB_ENABLE_THEME_PERSONALITY     1u     /* Theme-specific motion signature: speed/phase/depth */
#define AMB_ENABLE_CABIN_DEPTH_MODEL     1u     /* Pseudo-3D front/rear + left/right parallax */
#define AMB_CABIN_DEPTH_LR_GAIN          0.85f  /* Left/right contribution in board pose */
#define AMB_CABIN_DEPTH_FR_GAIN          1.00f  /* Front/rear contribution in board pose */
#define AMB_CABIN_DEPTH_U_SPREAD         0.10f  /* Palette-space offset from depth model */
#define AMB_CABIN_DEPTH_PHASE_SHIFT      1.35f  /* Additional phase shift for dynamic zone FX */
#define AMB_CABIN_DEPTH_BRIGHTNESS_DELTA 0.08f  /* Subtle zone brightness parallax from depth */

/* ====== BANK CHARACTER MEMORY ============================================ */
#define AMB_ENABLE_BANK_CHARACTER_MEMORY   1u   /* Remember per-bank motion characteristic and restore on return */
#define AMB_BANK_CHARACTER_LERP_ALPHA      0.78f/* Higher = slower memory adaptation */
#define AMB_BANK_CHARACTER_SPEED_MIN        0.84f
#define AMB_BANK_CHARACTER_SPEED_MAX        1.22f

/* ====== NIGHT MODE (runtime) =========================================== */
/* Enabled/disabled via CAN. Default: off. */

/* Runtime flag synchronized from CAN and consumed by scene/zones rendering. */
extern uint8_t g_night_mode_state;

#define AMB_NIGHT_BRIGHTNESS_SCALE   0.40f  /* Brightness coefficient in night mode (0..1) */

/* ====== OEM BRIGHTNESS CURVE =========================================== */
/* Формирует кривую из шага 0..5 OEM в линейную яркость 0..1 */
#define AMB_BRIGHTNESS_FLOOR   0.10f   /* Минимум даже на шаге 1 (видимость низов) */
#define AMB_BRIGHTNESS_CEIL    0.95f   /* Верхний предел (оставляем запас цвета) */
#define AMB_BRIGHTNESS_EXP     0.55f   /* Экспонента кривой ( <1 приподнимает низы ) */
#ifndef AMB_DIMMING_ATTACK_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_DIMMING_ATTACK_MS  380u    /* Bench: быстрее отклик при калибровке */
#else
#define AMB_DIMMING_ATTACK_MS  450u    /* Production: мягкий премиальный разгон */
#endif
#endif
#ifndef AMB_DIMMING_RELEASE_MS
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_DIMMING_RELEASE_MS 760u    /* Bench: быстрее итерации на столе */
#else
#define AMB_DIMMING_RELEASE_MS 900u    /* Production: более мягкий спад */
#endif
#endif
#ifndef AMB_DIMMING_POST_SMOOTH
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_DIMMING_POST_SMOOTH 0.70f  /* Bench: меньше инерции, легче ловить изменения */
#else
#define AMB_DIMMING_POST_SMOOTH 0.82f  /* Production: максимум плавности */
#endif
#endif

/* ====== SLEEP MODE (low power) ========================================== */
/* Automatic sleep mode when no CAN activity. Wake up via CAN RX EXTI. */

#ifndef AMB_ENABLE_SLEEP_MODE
#define AMB_ENABLE_SLEEP_MODE       1      /* 1 = enable sleep mode */
#endif
#ifndef AMB_SLEEP_TIMEOUT_SEC
#define AMB_SLEEP_TIMEOUT_SEC       4u     /* Sleep timeout (seconds) */
#endif
#ifndef AMB_SLEEP_FADE_OUT_MS
#define AMB_SLEEP_FADE_OUT_MS       2000u  /* Fade-out duration before sleep (ms) */
#endif
#ifndef AMB_SLEEP_CANCEL_IDLE_DIV
#define AMB_SLEEP_CANCEL_IDLE_DIV   2u     /* Cancel sleep/outro if idle less than timeout/div */
#endif
#ifndef AMB_WAIT_OEM_RESLEEP_MS
#define AMB_WAIT_OEM_RESLEEP_MS     1200u  /* After STOP wake, return to STOP if no OEM (0x325) */
#endif

/* ====== CAN WAKE POLICY =================================================== */
/* Bench profile: keep transceiver in Normal mode during MCU STOP
 * to wake from any regular CAN traffic (higher current).
 * Production profile: put transceiver to Standby in sleep (lower current),
 * wake depends on WUP/WAKE behavior of the vehicle network. */
#define AMB_CAN_WAKE_BENCH       0u
#define AMB_CAN_WAKE_PRODUCTION  1u

#ifndef AMB_CAN_WAKE_POLICY
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_CAN_WAKE_POLICY AMB_CAN_WAKE_BENCH
#else
#define AMB_CAN_WAKE_POLICY AMB_CAN_WAKE_PRODUCTION
#endif
#endif

/* ====== FLASH STORAGE =================================================== */
/* Settings are saved to Flash with delay to minimize wear. */

#define AMB_FLASH_SAVE_DELAY_MS     10000u  /* Delay before saving to Flash (10 sec) */

/* ====== WELCOME/GOODBYE EFFECT PARAMETERS =============================== */
/* Tunable parameters for intro/outro effects */

#define FX_WELCOME_TIME_SCALE       0.92f   /* Больше времени на фазу проявления и settle */
#define FX_WELCOME_WAVE_START       0.10f   /* When waves begin */
#define FX_WELCOME_WAVE_DURATION    0.50f   /* Waves duration in normalized time */
#define FX_WELCOME_WAVE_GAIN_MAX    0.11f   /* Более читаемая премиальная волна */
#define FX_WELCOME_PULSE_START      0.55f   /* When pulse begins */
#define FX_WELCOME_PULSE_DURATION   0.20f   /* Pulse duration */
#define FX_WELCOME_PULSE_AMPLITUDE  0.06f   /* Выразительнее центральный акцент */
#define FX_WELCOME_CENTER_OFFSET    0.47f   /* Center position offset */
#define FX_WELCOME_DIST_SCALE       0.25f   /* Edge dimming scale */
#define FX_WELCOME_SETTLE_START     0.75f   /* When settle phase begins */
#define FX_WELCOME_FINAL_SCALE      0.82f   /* Меньше провал по яркости при входе в сцену */
#define FX_GOODBYE_CURTAIN_SCALE    1.2f    /* Curtain closing speed */

/* ====== THEME / FX GLOBAL TUNING ======================================= */
#define AMB_THEME_MIN_BRIGHTNESS    0.70f   /* Универсальная база для тем (чтобы даже тёмные не гасли) */
#define AMB_BRIDGE_DURATION_MS       400u   /* Плавный переход intro->scene (быстрее заезд в сцену) */
#define AMB_BRIDGE_SCREEN_MIX        0.22f  /* Лёгкий anti-dim в середине intro->scene */
#ifndef AMB_TRANSITION_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_TRANSITION_SMOOTH_ALPHA  0.62f  /* Bench: быстрее переходы при отладке */
#else
#define AMB_TRANSITION_SMOOTH_ALPHA  0.76f  /* Production: мягче переходы */
#endif
#endif
#ifndef AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA 0.55f /* Bench: меньше инерции, быстрее диагностика */
#else
#define AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA 0.78f /* Production: более шелковистая яркость */
#endif
#endif
#ifndef AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA 0.64f /* Bench: чуть мягче на intro/bridge */
#else
#define AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA 0.84f /* Production: premium-переходы без ступеней */
#endif
#endif
#ifndef AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA 0.72f /* Bench: быстрее финальный спад */
#else
#define AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA 0.90f /* Production: максимально мягкий хвост */
#endif
#endif
#ifndef AMB_IDLE_MICRO_MOTION_AMPLITUDE
#if (AMB_PROFILE == AMB_PROFILE_BENCH)
#define AMB_IDLE_MICRO_MOTION_AMPLITUDE 0.010f /* 1.0% в bench */
#else
#define AMB_IDLE_MICRO_MOTION_AMPLITUDE 0.018f /* 1.8% в production */
#endif
#endif
#ifndef AMB_IDLE_MICRO_MOTION_HZ
#define AMB_IDLE_MICRO_MOTION_HZ 0.08f         /* Один длинный цикл ~12.5 сек */
#endif
#define AMB_FX_TEMPORAL_SMOOTH       0.86f  /* Межкадровое сглаживание для premium FX */
#define FX_SPATIAL_GROUP            4u      /* Сколько физических LED объединяем в один "виртуальный" пиксель */
#define AMB_INTRO_DURATION_MS       2200u   /* Intro анимация при смене темы */
#define AMB_OUTRO_DURATION_MS       1800u   /* Outro анимация при смене темы */

/* ====== CAN PROTOCOL TIMINGS ============================================ */

#define AMB_CAN_MASTER_TX_INTERVAL_MS   200u    /* Master packet interval */
#define AMB_CAN_SYNC_INTERVAL_MS        500u    /* Sync/heartbeat interval */
#define AMB_CAN_DISCOVERY_INTERVAL_MS   1500u   /* Discovery packet interval */
#define AMB_CAN_STARTUP_DISCOVERY_MS    1500u   /* Discovery-only window after first OEM packet */
#define AMB_CAN_ACTIVE_TIMEOUT_MS       2000u   /* Stop TX if no CAN RX within this window */

#ifdef __cplusplus
}
#endif
