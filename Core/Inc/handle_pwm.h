#pragma once

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void handle_pwm_init(TIM_HandleTypeDef *htim, uint32_t channel);
void handle_pwm_set_enabled(uint8_t enabled);
void handle_pwm_set_brightness_pct(uint8_t pct);
void handle_pwm_tick(uint32_t dt_ms);
uint8_t handle_pwm_is_off(void);
void handle_pwm_prepare_stop(void);
void handle_pwm_resume_after_stop(void);

#ifdef __cplusplus
}
#endif
