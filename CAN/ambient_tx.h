#pragma once

#include <stdint.h>

#include "ambient_state.h"
#include "types.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Send master state packet (`0x353`, master frame format). */
void can_tx_send_master(FDCAN_HandleTypeDef *hfdcan,
                        const can_state_t *state,
                        motion_profile_t motion_profile,
                        uint8_t oem_diag_b3,
                        uint8_t oem_diag_flags);
/** Send sync heartbeat packet (`0x353`, sync frame format). */
void can_tx_send_sync(FDCAN_HandleTypeDef *hfdcan,
                      uint8_t oem_color,
                      uint8_t brightness_raw);
/** Send discovery packet for role election. */
void can_tx_send_discovery(FDCAN_HandleTypeDef *hfdcan,
                           uint32_t unique_id,
                           uint8_t is_master);
/** Send arbitrary test frame (debug/bench only). */
void can_tx_send_test(FDCAN_HandleTypeDef *hfdcan,
                      uint32_t id,
                      const uint8_t *data,
                      uint8_t len);
/** Send modern body ambient status frame (`0x12B`). */
void can_tx_send_body_ambient_status(FDCAN_HandleTypeDef *hfdcan,
                                     uint8_t color_stat,
                                     uint8_t brightness_stat,
                                     uint8_t effect_stat);

#ifdef __cplusplus
}
#endif
