#pragma once

#include <stdint.h>

#include "ambient_state.h"
#include "types.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

void can_tx_send_master(FDCAN_HandleTypeDef *hfdcan,
                        const can_state_t *state,
                        motion_profile_t motion_profile);
void can_tx_send_sync(FDCAN_HandleTypeDef *hfdcan,
                      uint8_t bank_id,
                      uint8_t theme_index);
void can_tx_send_discovery(FDCAN_HandleTypeDef *hfdcan,
                           uint32_t unique_id,
                           uint8_t is_master);
void can_tx_send_test(FDCAN_HandleTypeDef *hfdcan,
                      uint32_t id,
                      const uint8_t *data,
                      uint8_t len);

#ifdef __cplusplus
}
#endif
