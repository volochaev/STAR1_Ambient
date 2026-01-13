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
 * - 0x351: OEM packets from vehicle IC (brightness, color)
 * - 0x353: Unified Master Protocol (discovery, sync, master, extended packets)
 *
 * @section Master/Slave Architecture
 * One board acts as master (automatically determined by priority), others as slaves.
 * Master reads OEM CAN packets and broadcasts state to all slaves.
 * Automatic failover if master fails.
 *
 * @section Extended Mode
 * Extended mode allows manual theme selection via bank_id and theme_index.
 * Toggled by 5 color changes within 3 seconds.
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
#include "types.h"
#include "scene_player.h"
#include "presets.h"
#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OEM CAN ID (IC → ambient) */
#define CAN_OEM_ID       0x351U

/* Extended ambient control - объединен с CAN_MASTER_ID */
#define CAN_EXT_ID       0x353U  /* тот же ID, различается по типу пакета */

/* Master broadcast ID (master → slaves) */
/* Также используется для discovery, sync и ext пакетов (различаются по типу в data[0]) */
#define CAN_MASTER_ID    0x353U

/* Discovery packet ID (для автоматического определения master) */
#define CAN_DISCOVERY_ID 0x353U  /* тот же ID, но другой формат */

/* Sync packet ID (heartbeat от master) - объединен с CAN_MASTER_ID */
#define CAN_SYNC_ID      0x353U  /* тот же ID, различается по типу пакета */

/* Extended flags */
#define EXT_FLAG_EXTENDED   0x01
#define EXT_FLAG_NIGHT      0x02

/* Packet type markers для CAN_MASTER_ID (0x353) */
#define PKT_TYPE_DISCOVERY  0x00  /* Discovery: data[0] = board_type (0-5) */
#define PKT_TYPE_MASTER     0x10  /* Master: data[0] = 0x10 | flags, data[1-4] = state */
#define PKT_TYPE_SYNC       0x20  /* Sync: data[0] = 0x20 | bank_id, data[1] = theme_index */
#define PKT_TYPE_EXT        0x30  /* Extended: data[0] = 0x30 | flags, data[1] = bank_id, data[2] = theme_index */
#define PKT_TYPE_MASK       0xF0  /* Маска для типа пакета (старшие 4 бита) */

/* Board types для discovery */
#define BOARD_TYPE_FL        0   /* Front-Left door */
#define BOARD_TYPE_FR        1   /* Front-Right door */
#define BOARD_TYPE_RL        2   /* Rear-Left door */
#define BOARD_TYPE_RR        3   /* Rear-Right door */
#define BOARD_TYPE_DASHBOARD 4   /* Dashboard/Instrument Panel */
#define BOARD_TYPE_REAR      5   /* Rear ambient (задняя часть салона) */

/* Board type - должен быть определен в board_xxx.h, иначе по умолчанию FL */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_FL
#endif

/* Master/Slave mode configuration */
/* Можно определить через define или через GPIO */
/* Если не определено, будет использоваться автоматическое определение */
#ifndef AMB_ROLE_MASTER
#define AMB_ROLE_MASTER  -1  /* -1 = auto-detect, 0 = slave, 1 = master */
#endif

/* Timeouts для failover */
#define MASTER_HEARTBEAT_TIMEOUT_MS  1000u  /* если master не отвечает 1 сек */
#define DISCOVERY_INTERVAL_MS         1000u  /* discovery пакет каждую секунду */
#define SYNC_INTERVAL_MS              250u   /* sync пакет каждые 250мс */

/**
 * @brief CAN ambient lighting state structure
 * @details Stores current state of ambient lighting system including OEM color,
 *          brightness, extended mode, and theme selection.
 */
typedef struct {
    uint8_t oem_color;       /**< OEM color: 0=Amber, 1=Blue, 2=White */
    float   oem_brightness;  /**< OEM brightness: 0.0..1.0 */

    uint8_t extended_mode;   /**< Extended mode: 0=OEM only, 1=full theme control */
    uint8_t night_mode;      /**< Night mode: 0=off, 1=on */

    uint8_t bank_id;         /**< Bank ID: 0=auto from OEM, 1=Amber, 2=Blue, 3=White */
    uint8_t theme_index;     /**< Theme index inside bank */
    
    /* Cyclic theme rotation per OEM bank */
    uint8_t last_oem_color;        /**< Last OEM color for detecting bank change */
    uint8_t oem_theme_indices[3];  /**< Cyclic theme index for each OEM bank */
} amb_can_state_t;

/** @brief Global CAN ambient state (volatile for thread-safety) */
extern volatile amb_can_state_t g_amb_can;

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
 *          sync, extended, and discovery packets.
 */
void can_ambient_process_rx(uint32_t id, uint8_t *data, uint8_t len);

/**
 * @brief Update scene player based on CAN state
 * @param ws Pointer to WS2812 strip (can be NULL)
 * @param pl Pointer to scene player
 * @details Called from main loop after player_tick. Updates theme, brightness,
 *          and triggers intro/outro transitions.
 */
void can_ambient_update(ws2812_t *ws, scene_player_t *pl);

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
 * @brief Send extended sync packet (master only)
 * @details Syncs extended mode state. Sent every 250ms when extended mode enabled.
 */
void can_ambient_send_ext_packet(void);

/**
 * @brief Send discovery packet (all boards)
 * @details Announces board presence and role. Sent every 1000ms by all boards.
 */
void can_ambient_send_discovery_packet(void);

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
 * @brief Update master/slave role (periodic call from main loop)
 * @param now_ms Current time in milliseconds
 * @details Handles discovery, failover, and automatic role assignment.
 *          Saves settings to flash if changed.
 */
void can_ambient_update_role(uint32_t now_ms);

/**
 * @brief Get last master heartbeat time
 * @return Timestamp of last heartbeat in milliseconds
 * @details Used for failover detection by slaves.
 */
uint32_t can_ambient_get_last_master_heartbeat_ms(void);

/* ========== SLEEP MODE API ========== */
#include "features.h"

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

#endif /* AMB_ENABLE_SLEEP_MODE */

#ifdef __cplusplus
}
#endif

#endif /* CAN_AMBIENT_H */
