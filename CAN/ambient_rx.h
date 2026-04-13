#pragma once

#include <stdint.h>

#include "ambient.h"

#ifdef __cplusplus
extern "C" {
#endif

void can_rx_init(void);
void can_rx_process(uint32_t id, uint8_t *data, uint8_t len);

uint8_t can_rx_oem_received(void);
void can_rx_reset_oem_received(void);
uint8_t can_rx_nsi_active(void);

can_bsm_state_t can_rx_get_bsm_state(void);
motion_profile_t can_rx_get_motion_profile(void);
void can_rx_set_motion_profile(motion_profile_t profile);
int8_t can_rx_consume_hvac_temp_trend(void);
int8_t can_rx_consume_hvac_temp_trend_side(uint8_t right_side);
int8_t can_rx_consume_hvac_temp_trend_combined(void);
float can_rx_get_hvac_split_bias_side(uint8_t right_side);
float can_rx_get_hvac_split_bias_combined(void);
float can_rx_get_hvac_fan_level(void);
float can_rx_get_seat_heat_level_side(uint8_t right_side);
float can_rx_get_seat_heat_level_combined(void);
float can_rx_get_bank_character_speed_scale(void);

#ifdef __cplusplus
}
#endif
