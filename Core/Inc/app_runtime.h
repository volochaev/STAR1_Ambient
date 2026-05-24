#pragma once

#include <stdint.h>

#include "runtime_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Execute one application runtime tick (CAN -> flow -> render orchestration). */
void app_runtime_tick(runtime_flow_ctx_t *ctx,
                      uint32_t *last_tick_ms,
                      uint8_t *can_protocol_started,
                      uint32_t *can_protocol_start_ms);
/** Return 1 if main loop may enter WFI idle this cycle. */
uint8_t app_runtime_should_idle_wfi(void);

#ifdef __cplusplus
}
#endif
