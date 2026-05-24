#pragma once

#include <stdint.h>

#include "ambient_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize source arbitration runtime state. */
void ambient_source_arbiter_init(void);
/** Apply CAN ingress arbitration (463 preferred, 325 fallback) into state store. */
void ambient_source_arbiter_apply(const can_state_t *can_state, uint32_t now_ms);
/** Internal logic self-test for source arbitration and legacy bank cycler (non-runtime critical). */
uint8_t ambient_source_arbiter_selftest(void);

#ifdef __cplusplus
}
#endif
