#pragma once

#include <stdint.h>

#include "ambient.h"

#ifdef __cplusplus
extern "C" {
#endif

#if AMB_ENABLE_SLEEP_MODE

void can_power_init(void);
void can_power_note_can_activity(void);

uint8_t can_power_is_sleeping(void);
uint8_t can_power_should_sleep(void);
void can_power_clear_sleep_request(void);
void can_power_mark_awake(void);
uint32_t can_power_get_idle_time_ms(void);
void can_power_check_sleep_timeout(void);
void can_power_enter_sleep(FDCAN_HandleTypeDef *hfdcan);
void can_power_exit_sleep(FDCAN_HandleTypeDef *hfdcan);
void can_power_note_stop_wakeup(uint8_t wake_src);
void can_power_get_diag(can_power_diag_t *out);
void can_power_reset_diag(void);

#endif

#ifdef __cplusplus
}
#endif
