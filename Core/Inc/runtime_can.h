#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Drive periodic CAN TX scheduling for master/discovery/status frames. */
void runtime_can_tx_scheduler_tick(uint32_t now,
                                   uint8_t oem_received,
                                   uint8_t can_protocol_started,
                                   uint32_t can_protocol_start_ms,
                                   uint8_t sleep_fade_active);

#ifdef __cplusplus
}
#endif
