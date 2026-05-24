#pragma once

#include <stdint.h>

#include "ambient.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize CAN RX derived-state subsystem. */
void can_rx_init(void);
/** Process one received CAN frame and update derived state/events. */
void can_rx_process(uint32_t id, uint8_t *data, uint8_t len);

/** Return 1 if OEM ambient source is currently considered fresh/valid. */
uint8_t can_rx_oem_received(void);
/** Reset OEM freshness and all CAN-derived volatile latches/state. */
void can_rx_reset_oem_received(void);
/** Return Night Security Illumination active state. */
uint8_t can_rx_nsi_active(void);
/** Return rear ILM request active state. */
uint8_t can_rx_is_ilm_rear_on_request(void);
/** Return LM_A1 night-security illumination active state. */
uint8_t can_rx_is_ns_ill_active(void);
/** Return LM_A4 puddle/entry combined request state. */
uint8_t can_rx_is_pddllmp_any_on_request(void);
/** Return reverse signal state with freshness filtering. */
uint8_t can_rx_is_reverse_signal_active(void);
/** Return reverse handle-block helper state with hold timer logic. */
uint8_t can_rx_is_reverse_handle_block_active(void);
/** Return ignition state with freshness filtering. */
uint8_t can_rx_is_ignition_on(void);

/** Return board-masked BSM state snapshot. */
can_bsm_state_t can_rx_get_bsm_state(void);
/** Return current motion profile derived from drive-mode policy/manual set. */
motion_profile_t can_rx_get_motion_profile(void);
/** Override motion profile at runtime. */
void can_rx_set_motion_profile(motion_profile_t profile);
/** Return last raw OEM diagnostic byte (source decode helper). */
uint8_t can_rx_get_oem_diag_b3(void);
/** Return packed OEM decode diagnostics flags. */
uint8_t can_rx_get_oem_diag_flags(void);
/** Consume combined HVAC trend event (left first, then right). */
int8_t can_rx_consume_hvac_temp_trend(void);
/** Consume HVAC trend event for selected side (`right_side=0/1`). */
int8_t can_rx_consume_hvac_temp_trend_side(uint8_t right_side);
/** Consume merged HVAC trend event with conflict suppression. */
int8_t can_rx_consume_hvac_temp_trend_combined(void);
/** Consume unlock event latch. */
uint8_t can_rx_consume_unlock_event(void);
/** Consume lock event latch (with TTL filter). */
uint8_t can_rx_consume_lock_event(void);
/** Clear pending lock event latch. */
void can_rx_clear_lock_event_pending(void);
/** Return 1 if lock source frame was observed recently. */
uint8_t can_rx_is_lock_source_recent(void);
/** Consume door-open event filtered for board topology. */
uint8_t can_rx_consume_door_open_event(uint8_t board_type);
/** Return 1 if any door for this board topology is currently open. */
uint8_t can_rx_is_any_door_open_for_board(uint8_t board_type);
/** Return HVAC split bias for selected side. */
float can_rx_get_hvac_split_bias_side(uint8_t right_side);
/** Return absolute HVAC split bias combined for both sides. */
float can_rx_get_hvac_split_bias_combined(void);
/** Return smoothed HVAC fan level in range `[0..1]`. */
float can_rx_get_hvac_fan_level(void);
/** Return seat heat level for side in range `[0..1]`. */
float can_rx_get_seat_heat_level_side(uint8_t right_side);
/** Return max seat heat level across cabin sides in range `[0..1]`. */
float can_rx_get_seat_heat_level_combined(void);
/** Return bank-character speed scale for current OEM bank. */
float can_rx_get_bank_character_speed_scale(void);
/** Return ILM dim multiplier for board role/topology. */
float can_rx_get_ilm_dim_scale(uint8_t board_type);
/** Return sun compensation gain for board side/topology. */
float can_rx_get_sun_gain_scale(uint8_t board_type);
/** Return parking warning state filtered for board topology. */
can_parking_warn_state_t can_rx_get_parking_warn_state(uint8_t board_type);
/** Return reverse active state used by scene overlay policy. */
uint8_t can_rx_is_reverse_active(void);
/** Get last parsed modern HU request tuple (color/brightness/effect). */
uint8_t can_rx_get_modern_request(uint8_t *color_rq,
                                  uint8_t *brightness_rq,
                                  uint8_t *effect_rq,
                                  uint32_t *timestamp_ms);
/** Get last parsed modern bus status tuple (color/brightness/effect). */
uint8_t can_rx_get_modern_bus_status(uint8_t *color_stat,
                                     uint8_t *brightness_stat,
                                     uint8_t *effect_stat,
                                     uint32_t *timestamp_ms);
/** Export CAN ingress validation diagnostics counters. */
void can_rx_get_ingress_diag(can_ingress_diag_t *out);
/** Reset CAN ingress validation diagnostics counters. */
void can_rx_reset_ingress_diag(void);

#ifdef __cplusplus
}
#endif
