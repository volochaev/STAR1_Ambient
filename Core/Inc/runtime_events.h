#pragma once

#include <stdint.h>

#include "runtime_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

void runtime_events_reset(void);
/** Return 1 when lock path currently forces sleep transition. */
uint8_t runtime_events_lock_sleep_forced(void);
/** Clear forced-sleep latch raised by lock event path. */
void runtime_events_clear_lock_sleep_forced(void);
/** Reset unlock-signature cooldown to allow immediate retrigger. */
void runtime_events_clear_unlock_signature_cooldown(void);

/** Arm unlock-signature event with trigger source tag. */
void runtime_events_arm_unlock_signature(uint8_t trigger, uint32_t now);
/** Observe CAN-derived unlock sources and arm unlock-signature when eligible. */
void runtime_events_maybe_arm_unlock_from_can(uint32_t now);
/** Start unlock-signature scene when armed and runtime state allows it. */
void runtime_events_maybe_trigger_unlock_signature(runtime_flow_ctx_t *ctx, uint32_t now);
/** Trigger door-open scene from pending CAN door events. */
void runtime_events_maybe_trigger_door_open(uint32_t now);
/** Trigger lock-goodbye transition path when lock conditions are met. */
void runtime_events_maybe_trigger_lock_goodbye(runtime_flow_ctx_t *ctx, led_runtime_strip_t *main_strip);
/** Trigger HVAC temperature trend scene when pending trend exists. */
void runtime_events_maybe_trigger_hvac_temp(uint32_t now);

#ifdef __cplusplus
}
#endif
