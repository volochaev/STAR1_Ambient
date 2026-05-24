#pragma once

#include <stdint.h>

#include "ambient_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Apply unified CAN state to the modern ambient backend.
 */
void ambient_backend_modern_on_can_state(const can_state_t *can_state, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
