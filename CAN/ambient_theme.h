#pragma once

#include <stdint.h>

#include "ambient_state.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void can_theme_state_init(void);
void can_theme_state_note_flash_loaded(const can_state_t *state);
void can_theme_state_note_state_change(uint32_t now_ms);

theme_id_t can_theme_state_resolve(volatile can_state_t *state, uint32_t now_ms);
void can_theme_state_process_deferred_save(volatile can_state_t *state,
                                           uint8_t is_master,
                                           uint32_t now_ms);

#ifdef __cplusplus
}
#endif
