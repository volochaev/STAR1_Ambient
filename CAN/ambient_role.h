#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize role-election subsystem and discovery cache. */
void can_role_init(void);
/** Reset role subsystem to initial state. */
void can_role_reset(void);

/** Return current role (`-1` auto/unknown, `0` slave, `1` master). */
int8_t can_role_get(void);
/** Return 1 when current board acts as master. */
uint8_t can_role_is_master(void);

/** Process discovery frame from remote board. */
void can_role_handle_discovery(uint8_t board_type, uint32_t now_ms);
/** Update master heartbeat timestamp from received master packet. */
void can_role_note_master_heartbeat(uint32_t now_ms);
/** Update sync heartbeat timestamp from received sync packet. */
void can_role_note_sync_from_master(uint32_t now_ms);
/** Run periodic role-election/failover logic. */
void can_role_update(uint32_t now_ms);

/** Return last observed master heartbeat timestamp. */
uint32_t can_role_get_last_master_heartbeat_ms(void);

#ifdef __cplusplus
}
#endif
