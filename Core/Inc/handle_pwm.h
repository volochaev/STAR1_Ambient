#pragma once

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize handle backlight PWM bridge. */
void handle_pwm_init(TIM_HandleTypeDef *htim, uint32_t channel);
/** Enable or disable handle backlight output logic. */
void handle_pwm_set_enabled(uint8_t enabled);
/** Set target handle backlight brightness in percent (0..100). */
void handle_pwm_set_brightness_pct(uint8_t pct);
/** Periodic tick for smooth PWM fades. */
void handle_pwm_tick(uint32_t dt_ms);
/** True when effective PWM output is fully off. */
uint8_t handle_pwm_is_off(void);
/** Prepare PWM bridge for low-power stop entry. */
void handle_pwm_prepare_stop(void);
/** Restore PWM operation after wakeup from stop mode. */
void handle_pwm_resume_after_stop(void);

#ifdef __cplusplus
}
#endif
