#ifndef CAN_AMBIENT_H
#define CAN_AMBIENT_H

#include "main.h"
#include "types.h"
#include "scene_player.h"
#include "presets.h"
#include "driver.h"

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

/* Board type - должен быть определен в board_xxx.h */
#ifndef BOARD_TYPE
#define BOARD_TYPE  BOARD_TYPE_FL  /* по умолчанию FL */
#endif

/* Timeouts для failover */
#define MASTER_HEARTBEAT_TIMEOUT_MS  1000u  /* если master не отвечает 1 сек */
#define DISCOVERY_INTERVAL_MS         1000u  /* discovery пакет каждую секунду */
#define SYNC_INTERVAL_MS              250u   /* sync пакет каждые 250мс */

typedef struct {
    uint8_t oem_color;       // 0=Amber,1=Blue,2=White
    float   oem_brightness;  // 0.0..1.0

    uint8_t extended_mode;   // 0 = OEM only, 1 = full theme control
    uint8_t night_mode;      // 0/1

    uint8_t bank_id;         // 0 = from OEM color, 1=Amber,2=Blue,3=White
    uint8_t theme_index;     // index inside bank
} amb_can_state_t;

extern volatile amb_can_state_t g_amb_can;

/* Initialization */
void can_ambient_init(FDCAN_HandleTypeDef *hfdcan);

/* Must be called from FDCAN RX callback */
void can_ambient_process_rx(uint32_t id, uint8_t *data, uint8_t len);

/* Called from main loop (after player_tick) */
void can_ambient_update(ws2812_t *ws, scene_player_t *pl);

/* Master mode: send current state to slaves */
void can_ambient_send_master_packet(void);

/* Send sync/heartbeat packet (master only) */
void can_ambient_send_sync_packet(void);

/* Send extended sync packet (master only, для синхронизации расширенного режима) */
void can_ambient_send_ext_packet(void);

/* Send discovery packet (all boards) */
void can_ambient_send_discovery_packet(void);

/* Check if this board is master (can be overridden via GPIO) */
uint8_t can_ambient_is_master(void);

/* Update master/slave state (call periodically from main loop) */
void can_ambient_update_role(uint32_t now_ms);

/* Get last master heartbeat time */
uint32_t can_ambient_get_last_master_heartbeat_ms(void);

#endif
