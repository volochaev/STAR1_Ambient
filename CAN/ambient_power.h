#pragma once

#include <stdint.h>

#include "ambient.h"

#ifdef __cplusplus
extern "C" {
#endif

#if AMB_ENABLE_SLEEP_MODE

/** Initialize CAN power-management state. */
void can_power_init(void);
/** Mark CAN activity (resets idle timer and sleep request path). */
void can_power_note_can_activity(void);

/** Return 1 if power state is currently sleeping. */
uint8_t can_power_is_sleeping(void);
/** Return 1 if sleep has been requested by policy. */
uint8_t can_power_should_sleep(void);
/** Clear pending sleep request. */
void can_power_clear_sleep_request(void);
/** Force sleep request due to lock/event policy. */
void can_power_request_sleep_lock(void);
/** Mark subsystem awake and reset sleep transitions. */
void can_power_mark_awake(void);
/** Return idle time since last accepted activity. */
uint32_t can_power_get_idle_time_ms(void);
/** Evaluate timeout policy and set sleep request when needed. */
void can_power_check_sleep_timeout(void);
/** Enter CAN sleep mode (transceiver/FDCAN low-power path). */
void can_power_enter_sleep(FDCAN_HandleTypeDef *hfdcan);
/** Exit CAN sleep mode and restore normal operation. */
void can_power_exit_sleep(FDCAN_HandleTypeDef *hfdcan);
/** Record STOP wake source in diagnostics. */
void can_power_note_stop_wakeup(uint8_t wake_src);
/** Record periodic RTC wake cycle while still waiting in STOP loop. */
void can_power_note_stop_rtc_wakeup_cycle(void);
/** Return power diagnostics snapshot. */
void can_power_get_diag(can_power_diag_t *out);
/** Reset power diagnostics counters/state. */
void can_power_reset_diag(void);

#endif

#ifdef __cplusplus
}
#endif
