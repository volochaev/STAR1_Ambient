#pragma once

#include <stdint.h>

#include "ambient_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize backend state and persistent store linkage. */
void ambient_backend_init(void);
/** Apply CAN-derived state update through active backend. */
void ambient_backend_on_can_state(const can_state_t *can_state, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
