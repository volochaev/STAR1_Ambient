#include "runtime_state_machine.h"

static runtime_state_machine_diag_t g_sm_diag = {0u, 0u};

/* Return stable text name for diagnostics/logging. */
const char *runtime_state_machine_name(runtime_mode_t state)
{
    switch (state) {
    case RUNTIME_BOOT: return "BOOT";
    case RUNTIME_WAIT_OEM: return "WAIT_OEM";
    case RUNTIME_ACTIVE: return "ACTIVE";
    case RUNTIME_SLEEP_PREP: return "SLEEP_PREP";
    case RUNTIME_STOP: return "STOP";
    case RUNTIME_WAKE_RECOVER: return "WAKE_RECOVER";
    default: return "UNKNOWN";
    }
}

/* Validate transition against allowed runtime graph. */
uint8_t runtime_state_machine_can_transition(runtime_mode_t from, runtime_mode_t to)
{
    if (from == to) return 1u;

    switch (from) {
    case RUNTIME_BOOT:
        return (uint8_t)(to == RUNTIME_WAIT_OEM);
    case RUNTIME_WAIT_OEM:
        return (uint8_t)(to == RUNTIME_ACTIVE || to == RUNTIME_STOP);
    case RUNTIME_ACTIVE:
        return (uint8_t)(to == RUNTIME_SLEEP_PREP || to == RUNTIME_WAIT_OEM || to == RUNTIME_STOP);
    case RUNTIME_SLEEP_PREP:
        return (uint8_t)(to == RUNTIME_ACTIVE || to == RUNTIME_STOP || to == RUNTIME_WAIT_OEM);
    case RUNTIME_STOP:
        return (uint8_t)(to == RUNTIME_WAKE_RECOVER || to == RUNTIME_WAIT_OEM);
    case RUNTIME_WAKE_RECOVER:
        return (uint8_t)(to == RUNTIME_WAIT_OEM);
    default:
        return 0u;
    }
}

/* Apply transition and update diagnostics counters. */
uint8_t runtime_state_machine_transition(runtime_mode_t *state, runtime_mode_t to)
{
    runtime_mode_t from;

    if (!state) return 0u;
    from = *state;
    if (!runtime_state_machine_can_transition(from, to)) {
        g_sm_diag.invalid_transitions++;
        return 0u;
    }

    *state = to;
    g_sm_diag.valid_transitions++;
    return 1u;
}

/* Export transition counters for debug/telemetry. */
void runtime_state_machine_get_diag(runtime_state_machine_diag_t *out)
{
    if (!out) return;
    *out = g_sm_diag;
}
