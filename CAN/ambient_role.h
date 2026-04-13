#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void can_role_init(void);
void can_role_reset(void);

int8_t can_role_get(void);
uint8_t can_role_is_master(void);

void can_role_handle_discovery(uint8_t board_type, uint32_t now_ms);
void can_role_note_master_heartbeat(uint32_t now_ms);
void can_role_note_sync_from_master(uint32_t now_ms);
void can_role_update(uint32_t now_ms);

uint32_t can_role_get_last_master_heartbeat_ms(void);

#ifdef __cplusplus
}
#endif
