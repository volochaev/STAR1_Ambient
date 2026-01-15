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

#define AMB_ENABLE_GAMMA        1       /* 1 = enable gamma correction */
#define AMB_GAMMA_EXP           2.2f
#define AMB_ENABLE_DITHERING    1       /* 1 = enable temporal dithering */
#define AMB_ENABLE_ZONE_FX      1       /* 1 = enable zone effects (breath/flow) */
#define AMB_ENABLE_WATCHDOG     1       /* 1 = enable IWDG watchdog */
#define AMB_WATCHDOG_TIMEOUT_MS 2000u   /* Watchdog timeout in ms */

/* ====== AUTO-ROTATE (theme cycling within bank) ======================== */

#define AMB_ENABLE_AUTO_ROTATE          0       /* 1 = auto-rotate themes within bank */
#define AMB_AUTO_ROTATE_INTERVAL_SEC    10u     /* Theme change interval (seconds) */
#define AMB_CROSSFADE_DURATION_MS       5000u   /* Crossfade duration (ms) */
#define AMB_MAX_CROSSFADE_LEDS          256u    /* Max LEDs for crossfade temp buffer */

/* ====== NIGHT MODE (runtime) =========================================== */
/* Enabled/disabled via CAN. Default: off. */

extern uint8_t g_amb_night_mode;

#define AMB_NIGHT_BRIGHTNESS_SCALE   0.30f  /* Brightness coefficient in night mode (0..1) */

/* ====== SLEEP MODE (low power) ========================================== */
/* Automatic sleep mode when no CAN activity. Wake up via CAN RX EXTI. */

#define AMB_ENABLE_SLEEP_MODE       1      /* 1 = enable sleep mode */
#define AMB_SLEEP_TIMEOUT_SEC       60u     /* Sleep timeout (seconds) */
#define AMB_SLEEP_FADE_OUT_MS       2000u   /* Fade-out duration before sleep (ms) */

/* ====== FLASH STORAGE =================================================== */
/* Settings are saved to Flash with delay to minimize wear. */

#define AMB_FLASH_SAVE_DELAY_MS     10000u  /* Delay before saving to Flash (10 sec) */

/* ====== WELCOME/GOODBYE EFFECT PARAMETERS =============================== */
/* Tunable parameters for intro/outro effects */

#define FX_WELCOME_TIME_SCALE       0.85f   /* Animation completes at ~0.85, leaving 0.15 for settle */
#define FX_WELCOME_WAVE_START       0.10f   /* When waves begin */
#define FX_WELCOME_WAVE_DURATION    0.50f   /* Waves duration in normalized time */
#define FX_WELCOME_WAVE_GAIN_MAX    0.25f   /* Max brightness boost from waves */
#define FX_WELCOME_PULSE_START      0.55f   /* When pulse begins */
#define FX_WELCOME_PULSE_DURATION   0.20f   /* Pulse duration */
#define FX_WELCOME_PULSE_AMPLITUDE  0.08f   /* Pulse brightness amplitude */
#define FX_WELCOME_CENTER_OFFSET    0.47f   /* Center position offset */
#define FX_WELCOME_DIST_SCALE       0.25f   /* Edge dimming scale */
#define FX_WELCOME_SETTLE_START     0.75f   /* When settle phase begins */
#define FX_GOODBYE_CURTAIN_SCALE    1.2f    /* Curtain closing speed */

/* ====== CAN PROTOCOL TIMINGS ============================================ */

#define AMB_CAN_MASTER_TX_INTERVAL_MS   100u    /* Master packet interval */
#define AMB_CAN_SYNC_INTERVAL_MS        250u    /* Sync/heartbeat interval */
#define AMB_CAN_DISCOVERY_INTERVAL_MS   1000u   /* Discovery packet interval */

#ifdef __cplusplus
}
#endif
