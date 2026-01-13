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
 *
 * Runtime features:
 * - Night mode: Reduces brightness via AMB_NIGHT_BRIGHTNESS_SCALE
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

/* ====== FEATURE TOGGLES (compile-time) ================================= */

#define AMB_ENABLE_GAMMA        1   /* 1 = включить гамма-коррекцию */
#define AMB_GAMMA_EXP           2.2f

#define AMB_ENABLE_DITHERING    1   /* 1 = включить лёгкий дезеринг по LSB */

#define AMB_ENABLE_ZONE_FX      1   /* 1 = живые эффекты для зон (breath/flow) */

/* ====== AUTO-ROTATE (theme cycling within bank) ======================== */

#define AMB_ENABLE_AUTO_ROTATE  1   /* 1 = авто-ротация тем каждые N минут */
#define AMB_AUTO_ROTATE_INTERVAL_SEC   10u   /* интервал смены темы (секунды) */
#define AMB_CROSSFADE_DURATION_MS     5000u   /* длительность кроссфейда (мс) - 5 сек для плавности */

/* ====== NIGHT MODE (runtime) =========================================== */
/* Включается/выключается по CAN.
 * По умолчанию: выкл.
 */

extern uint8_t g_amb_night_mode;

/* Коэффициент для яркости в night mode (0..1) */
#define AMB_NIGHT_BRIGHTNESS_SCALE   0.30f

#ifdef __cplusplus
}
#endif
