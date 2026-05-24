#pragma once

#include <stdint.h>

#include "ambient_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize deferred persistence state machine. */
void can_persist_state_init(void);
/** Record state loaded from flash as persistence baseline. */
void can_persist_state_note_flash_loaded(const can_state_t *state);
/** Notify persistence layer that runtime state changed at `now_ms`. */
void can_persist_state_note_state_change(uint32_t now_ms);
/** Run deferred save policy and commit when debounce rules allow. */
void can_persist_state_process_deferred_save(volatile can_state_t *state,
                                           uint8_t is_master,
                                           uint32_t now_ms);

#ifdef __cplusplus
}
#endif
