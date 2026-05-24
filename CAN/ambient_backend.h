#pragma once

#include <stdint.h>

#include "ambient_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AMBIENT_BACKEND_MODERN = 0u, /* Modern ambient request/state backend. */
} ambient_backend_kind_t;

/** Initialize backend state and persistent store linkage. */
void ambient_backend_init(void);
/** Set backend kind (currently modern-only; non-modern values are ignored). */
void ambient_backend_set_kind(ambient_backend_kind_t kind);
/** Return currently active backend kind. */
ambient_backend_kind_t ambient_backend_get_kind(void);
/** Apply CAN-derived state update through active backend. */
void ambient_backend_on_can_state(const can_state_t *can_state, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
