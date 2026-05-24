/**
 ******************************************************************************
 * @file    ambient.h
 * @brief   CAN communication protocol for ambient lighting system
 * @details This module implements the CAN bus communication protocol for the
 *          STAR1 Ambient lighting system. It supports master/slave architecture
 *          with automatic role discovery, failover mechanisms, and synchronized
 *          theme control across multiple boards.
 *
 * @section Protocol Overview
 * The system uses two CAN IDs:
 * - 0x325: OEM packets from vehicle IC (brightness, color)
 * - 0x353: Unified Master Protocol (discovery, master, sync packets)
 *
 * @section Master/Slave Architecture
 * One board acts as master (automatically determined by priority), others as slaves.
 * Master reads OEM CAN packets and broadcasts state to all slaves.
 * Automatic failover if master fails.
 *
 * @note    All boards use the same firmware. Board type is determined via BOARD_TYPE.
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#ifndef CAN_AMBIENT_H
#define CAN_AMBIENT_H

#include "main.h"
#include "board_selected.h"
#include "ambient_state.h"
#include "types.h"
#include "base_scene.h"
#include "driver.h"
#include "runtime_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OEM CAN ID (IC → ambient) */
#define CAN_OEM_ID       0x325U
/* Light sensor state (SAM_F/LGTSENS) */
#define CAN_LGTSENS_ID   0x025U
/* HVAC temperature setpoint/status (front/rear) */
#define CAN_HVAC_FRONT_ID 0x20BU
#define CAN_HVAC_REAR_ID  0x0BCU
/* HVAC fan current/status */
#define CAN_HVAC_FAN_ID   0x339U
/* Seat heating status */
#define CAN_SEAT_FRONT_ID 0x369U
#define CAN_SEAT_REAR_ID  0x350U
/* Central locking / unlock requests */
#define CAN_EIS_A2_ID     0x12DU
/* Interior lighting request */
#define CAN_ILM_RQ_ID     0x141U
/* Front SAM power supply state (ignition KL15/KL15R) */
#define CAN_SAM_F_A1_ID   0x006U
/* Light module state (night security illumination active) */
#define CAN_LM_A1_ID      0x069U
/* Light module exterior/entry lamp requests */
#define CAN_LM_A4_ID      0x02FU
/* Door latch states */
#define CAN_DOOR_FL_A1_ID 0x002U
#define CAN_DOOR_FR_A1_ID 0x004U
#define CAN_DOOR_R_A1_ID  0x007U
/* BSM warning request from vehicle */
#define CAN_BSM_ID       0x17EU
/* Day/night fallback from HU via TGW */
#define CAN_TGW_A8_ID    0x2E9U
/* Parking warn elements */
#define CAN_PTS_A1_ID    0x2EEU
/* Reverse signals */
#define CAN_RVC_A2_ID    0x2C3U
#define CAN_CHASSIS_R2_ID 0x10CU
/* Modern ambient protocol: request from HU, unified status to body/consumers */
#define CAN_HU_AMB_RQ_ID 0x463U
#define CAN_BODY_AMB_STAT_ID 0x12BU
/* Sun intensity */
#define CAN_HVAC_A1_ID   0x231U

/* Master broadcast ID (master -> slaves).
 * Shared ID for discovery/sync/master subtypes (decoded via data[0] marker). */
#define CAN_MASTER_ID    0x353U

/* Discovery packet ID (same raw ID, different payload format). */
#define CAN_DISCOVERY_ID 0x353U

/* Sync packet ID (master heartbeat; same raw ID, different subtype marker). */
#define CAN_SYNC_ID      0x353U

/* Packet subtype markers for CAN_MASTER_ID (0x353). */
#define PKT_TYPE_DISCOVERY  0x00  /* Discovery: data[0] = board_type (0-5) */
#define PKT_TYPE_MASTER     0x10  /* Master: data[0] = 0x10 | flags, data[1-4] = state */
#define PKT_TYPE_SYNC       0x20  /* Sync: data[0] = 0x20 | color_id, data[1] = brightness_raw */
#define PKT_TYPE_MASK       0xF0  /* Packet subtype mask (high nibble). */

/* Master flags (PKT_TYPE_MASTER) */
#define MASTER_FLAG_NIGHT   0x01u

/* Board types for discovery/election. */
#define BOARD_TYPE_FL        0   /* Front-Left door */
#define BOARD_TYPE_FR        1   /* Front-Right door */
#define BOARD_TYPE_RL        2   /* Rear-Left door */
#define BOARD_TYPE_RR        3   /* Rear-Right door */
#define BOARD_TYPE_DASHBOARD 4   /* Dashboard/Instrument Panel */
#define BOARD_TYPE_REAR      5   /* Rear ambient board. */
#define BOARD_TYPE_ANY       255 /* Aggregate all door sources (master-wide gate/events) */

/* Board type comes from board_selected.h (active board profile), fallback = FL */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_FL
#endif

/* Master/Slave mode configuration */
/* Can be forced at compile time, otherwise role is auto-detected. */
#ifndef AMB_ROLE_MASTER
#define AMB_ROLE_MASTER  -1  /* -1 = auto-detect, 0 = slave, 1 = master */
#endif

/* Failover timeout. */
#define MASTER_HEARTBEAT_TIMEOUT_MS  1800u  /* failover timeout */

typedef enum {
    CAN_SLEEP_REASON_NONE = 0u,          /* No sleep request reason set. */
    CAN_SLEEP_REASON_IDLE_TIMEOUT = 1u,  /* Idle timeout policy requested sleep. */
    CAN_SLEEP_REASON_API_ENTER = 2u,     /* Explicit API call requested sleep. */
    CAN_SLEEP_REASON_LOCK_EVENT = 3u,    /* Lock event path requested sleep. */
} can_sleep_reason_t;

typedef enum {
    CAN_WAKE_REASON_NONE = 0u,                  /* No wake source recorded. */
    CAN_WAKE_REASON_CAN_RX = 1u,                /* Activity wake from normal CAN RX path. */
    CAN_WAKE_REASON_STOP_EXTI_CAN_RX = 2u,      /* STOP wake via CAN RX EXTI line. */
    CAN_WAKE_REASON_STOP_EXTI_TRANSCEIVER = 3u, /* STOP wake via transceiver wake EXTI line. */
    CAN_WAKE_REASON_MANUAL_AWAKE = 4u,          /* Manual wake request via API. */
    CAN_WAKE_REASON_EXIT_SLEEP = 5u,            /* Wake as part of sleep-exit sequence. */
    CAN_WAKE_REASON_STOP_RTC = 6u,              /* Periodic RTC wake while staying in STOP loop. */
} can_wake_reason_t;

typedef struct {
    uint32_t sleep_request_count;
    uint32_t sleep_enter_count;
    uint32_t wake_count;
    can_sleep_reason_t last_sleep_reason;
    can_wake_reason_t last_wake_reason;
    uint32_t last_sleep_request_ms;
    uint32_t last_sleep_enter_ms;
    uint32_t last_wake_ms;
    uint32_t last_idle_ms_at_request;
    uint32_t stop_rtc_wakeup_count;
    uint32_t stop_unexpected_wakeup_count;
} can_power_diag_t;

typedef struct {
    uint8_t conflict_active;
    uint32_t conflict_count;
    uint8_t last_bus_status_valid;
    uint8_t last_bus_color;
    uint8_t last_bus_brightness;
    uint8_t last_bus_effect;
    uint32_t last_bus_timestamp_ms;
    uint32_t rq_invalid_len_count;
    uint32_t rq_invalid_range_count;
    uint32_t stat_invalid_len_count;
    uint32_t stat_invalid_range_count;
    uint32_t oem_invalid_range_count;
    uint32_t oem_clamped_brightness_count;
} can_modern_diag_t;

typedef struct {
    uint32_t rq_invalid_len_count;
    uint32_t rq_invalid_range_count;
    uint32_t stat_invalid_len_count;
    uint32_t stat_invalid_range_count;
    uint32_t oem_invalid_range_count;
    uint32_t oem_clamped_brightness_count;
} can_ingress_diag_t;

typedef enum {
    CAN_INGRESS_COUNTER_RQ_INVALID_LEN = 0u,
    CAN_INGRESS_COUNTER_RQ_INVALID_RANGE = 1u,
    CAN_INGRESS_COUNTER_STAT_INVALID_LEN = 2u,
    CAN_INGRESS_COUNTER_STAT_INVALID_RANGE = 3u,
    CAN_INGRESS_COUNTER_OEM_INVALID_RANGE = 4u,
    CAN_INGRESS_COUNTER_OEM_CLAMPED_BRIGHTNESS = 5u,
} can_ingress_counter_id_t;

/**
 * @brief Initialize CAN ambient system
 * @param hfdcan Pointer to FDCAN handle (must be initialized)
 * @details Configures CAN filters, loads settings from flash, initializes state.
 *          Must be called once at startup.
 */
void can_ambient_init(FDCAN_HandleTypeDef *hfdcan);

/**
 * @brief Process received CAN message
 * @param id CAN message ID
 * @param data Pointer to message data (max 8 bytes)
 * @param len Message data length
 * @details Must be called from HAL_FDCAN_RxFifo0Callback. Processes OEM, master,
 *          sync, and discovery packets.
 */
void can_ambient_process_rx(uint32_t id, uint8_t *data, uint8_t len);

/**
 * @brief Build normalized runtime inputs for the director layer
 * @param out Output snapshot for orchestration/render pipeline
 * @details Copies the current CAN-derived state atomically and resolves
 *          current theme/bank selection for the higher-level director.
 */
void can_ambient_fill_runtime_inputs(runtime_inputs_t *out);

/**
 * @brief Send master packet (master only)
 * @details Broadcasts full state to all slaves. Sent every 100ms by master.
 */
void can_ambient_send_master_packet(void);

/**
 * @brief Send sync/heartbeat packet (master only)
 * @details Heartbeat for failover detection. Sent every 250ms by master.
 */
void can_ambient_send_sync_packet(void);

/**
 * @brief Send discovery packet (all boards)
 * @details Periodic announce packet for board presence and role detection.
 */
void can_ambient_send_discovery_packet(void);
void can_ambient_send_modern_status_packet(void);

/**
 * @brief Check if this board is master
 * @return 1 if master, 0 if slave
 * @details Can be overridden via AMB_ROLE_MASTER define or GPIO.
 */
uint8_t can_ambient_is_master(void);

/**
 * @brief Check if first OEM CAN packet was received
 * @return 1 if OEM packet received, 0 otherwise
 * @details Used to delay LED startup until CAN data is available.
 */
uint8_t can_ambient_oem_received(void);

/**
 * @brief Reset OEM packet received flag
 * @details Used after waking from sleep to wait for new CAN data
 */
void can_ambient_reset_oem_received(void);

/**
 * @brief Get Night Security Illumination request state from OEM CAN
 * @return 1 if request is active, 0 otherwise
 * @details Parsed from IC_A8 (0x325): SurrIll_Rq and NS_IllDur_Rq.
 */
uint8_t can_ambient_nsi_active(void);
uint8_t can_ambient_handle_request_active(void);
uint8_t can_ambient_is_ignition_on(void);
can_bsm_state_t can_ambient_get_bsm_state(void);
motion_profile_t can_ambient_get_motion_profile(void);
void can_ambient_set_motion_profile(motion_profile_t profile);
int8_t can_ambient_consume_hvac_temp_trend(void);
int8_t can_ambient_consume_hvac_temp_trend_for_board(void);
uint8_t can_ambient_consume_unlock_event(void);
uint8_t can_ambient_consume_lock_event(void);
void can_ambient_clear_lock_event_pending(void);
uint8_t can_ambient_is_lock_source_recent(void);
uint8_t can_ambient_consume_door_open_event_for_board(void);
uint8_t can_ambient_is_any_door_open_for_board(void);
float can_ambient_get_hvac_split_bias_for_board(void);
float can_ambient_get_hvac_fan_level(void);
uint8_t can_ambient_is_night_mode(void);
float can_ambient_get_seat_heat_level_for_board(void);
float can_ambient_get_bank_character_speed_scale(void);
float can_ambient_get_ilm_dim_scale_for_board(void);
float can_ambient_get_sun_gain_for_board(void);
can_parking_warn_state_t can_ambient_get_parking_warn_state_for_board(void);
uint8_t can_ambient_is_reverse_active(void);

/**
 * @brief Update master/slave role (periodic call from main loop)
 * @param now_ms Current time in milliseconds
 * @details Handles discovery, failover, and automatic role assignment.
 *          Saves settings to flash if changed. Call after first OEM packet
 *          to avoid waking bus traffic too early during startup.
 */
void can_ambient_update_role(uint32_t now_ms);

/**
 * @brief Get last master heartbeat time
 * @return Timestamp of last heartbeat in milliseconds
 * @details Used for failover detection by slaves.
 */
uint32_t can_ambient_get_last_master_heartbeat_ms(void);
void can_ambient_get_modern_diag(can_modern_diag_t *out);
void can_ambient_reset_modern_diag(void);
void can_ambient_get_ingress_diag(can_ingress_diag_t *out);
void can_ambient_reset_ingress_diag(void);
uint32_t can_ambient_get_ingress_counter(can_ingress_counter_id_t counter_id);

/* ========== SLEEP MODE API ========== */
#include "ambient_config.h"

#if AMB_ENABLE_SLEEP_MODE

/**
 * @brief Check if system is currently in sleep mode
 * @return 1 if sleeping, 0 otherwise
 */
uint8_t can_ambient_is_sleeping(void);

/**
 * @brief Check if sleep mode was requested (timeout reached)
 * @return 1 if sleep requested, 0 otherwise
 */
uint8_t can_ambient_should_sleep(void);

/**
 * @brief Clear sleep request flag
 */
void can_ambient_clear_sleep_request(void);
void can_ambient_request_sleep_lock(void);

/**
 * @brief Mark system as awake (reset activity timer)
 */
void can_ambient_mark_awake(void);

/**
 * @brief Get time since last CAN activity
 * @return Idle time in milliseconds
 */
uint32_t can_ambient_get_idle_time_ms(void);

/**
 * @brief Check if sleep timeout reached and set sleep request flag
 * @details Call periodically from main loop
 */
void can_ambient_check_sleep_timeout(void);

/**
 * @brief Enter low power sleep mode
 * @details Turns off LED power, puts CAN transceiver in standby
 */
void can_ambient_enter_sleep(void);

/**
 * @brief Exit sleep mode and restore normal operation
 * @details Restarts CAN, turns on LED power
 */
void can_ambient_exit_sleep(void);

/**
 * @brief Note wakeup source after STOP exit
 * @param wake_src 1 = CAN RX EXTI (PA11), 2 = transceiver WAKE EXTI (PB7)
 */
void can_ambient_note_stop_wakeup(uint8_t wake_src);
void can_ambient_note_stop_rtc_wakeup_cycle(void);

/**
 * @brief Read power diagnostics snapshot
 * @param out Destination struct (must be non-NULL)
 */
void can_ambient_get_power_diag(can_power_diag_t *out);

/**
 * @brief Reset power diagnostics counters/state
 */
void can_ambient_reset_power_diag(void);

#endif /* AMB_ENABLE_SLEEP_MODE */

/* ========== DEBUG/TESTING API ========== */

/**
 * @brief Send arbitrary CAN packet for testing (DEBUG ONLY)
 * @param id CAN message ID
 * @param data Pointer to message data (max 8 bytes)
 * @param len Message data length (1-8)
 * @details This function allows sending any CAN packet for transceiver testing.
 *          Should be disabled in production builds.
 * @note Use with caution - can interfere with normal operation!
 */
void can_ambient_send_test_packet(uint32_t id, const uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* CAN_AMBIENT_H */
