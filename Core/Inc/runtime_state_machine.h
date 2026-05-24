#pragma once

#include <stdint.h>

#include "runtime_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t valid_transitions;
    uint32_t invalid_transitions;
} runtime_state_machine_diag_t;

/** Return short text name for runtime mode (debug/telemetry helper). */
const char *runtime_state_machine_name(runtime_mode_t state);
/** Check transition validity without mutating runtime state. */
uint8_t runtime_state_machine_can_transition(runtime_mode_t from, runtime_mode_t to);
/** Perform transition with validation/diagnostic counters. */
uint8_t runtime_state_machine_transition(runtime_mode_t *state, runtime_mode_t to);
/** Return transition diagnostics snapshot. */
void runtime_state_machine_get_diag(runtime_state_machine_diag_t *out);

#ifdef __cplusplus
}
#endif
