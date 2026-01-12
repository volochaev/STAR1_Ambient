#pragma once
/**
 * @file lighting_features.h
 * @brief Toggle advanced visual features for ambient lighting.
 *
 * Всё, что можно, включаем через константы.
 * Night Mode — отдельный runtime-флаг.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== FEATURE TOGGLES (compile-time) ================================= */

#define AMB_ENABLE_GAMMA        1   /* 1 = включить гамма-коррекцию */
#define AMB_GAMMA_EXP           2.2f

#define AMB_ENABLE_DITHERING    1   /* 1 = включить лёгкий дезеринг по LSB */

#define AMB_ENABLE_ZONE_FX      1   /* 1 = живые эффекты для зон (breath/flow) */

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
