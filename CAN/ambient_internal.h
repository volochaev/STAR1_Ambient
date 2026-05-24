#pragma once

#include "ambient_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Global CAN state snapshot shared inside ambient CAN module.
 */
extern volatile can_state_t g_can_state;

#ifdef __cplusplus
}
#endif
