#include "ambient.h"
#include "palette.h"
#include "presets.h"
#include "scene_player.h"
#include "features.h"

amb_can_state_t g_amb_can = {
    .oem_color = 0,
    .oem_brightness = 1.0f,
    .extended_mode = 0,
    .night_mode = 0,
    .bank_id = 0,
    .theme_index = 0
};

static FDCAN_HandleTypeDef *g_can = NULL;

/* ========== MASTER/SLAVE STATE ========== */
static int8_t g_role = -1;  /* -1 = unknown/auto, 0 = slave, 1 = master */
static uint32_t g_last_master_heartbeat_ms = 0;
static uint32_t g_last_discovery_send_ms = 0;
static uint32_t g_last_sync_send_ms = 0;
static uint32_t g_last_ext_send_ms = 0;
static uint8_t g_discovered_boards[6] = {0, 0, 0, 0, 0, 0};  /* FL, FR, RL, RR, DASHBOARD, REAR */
static uint32_t g_board_unique_id = 0;  /* можно использовать UID чипа */

/* ========== INIT ========== */
void can_ambient_init(FDCAN_HandleTypeDef *hfdcan)
{
    g_can = hfdcan;
    g_role = -1;  /* unknown, будет определен через discovery */
    g_last_master_heartbeat_ms = 0;
    g_last_discovery_send_ms = 0;
    g_last_sync_send_ms = 0;
    g_last_ext_send_ms = 0;
    
    /* Генерируем уникальный ID для этой платы (можно использовать UID чипа) */
    g_board_unique_id = HAL_GetTick() ^ (BOARD_TYPE << 8);

    FDCAN_FilterTypeDef f;

    /* Все платы слушают discovery, sync и master пакеты */
    /* Discovery/Sync/Master packet 0x353 (объединены, различаются по типу в data[0]) */
    f.IdType = FDCAN_STANDARD_ID;
    f.FilterIndex = 0;
    f.FilterType = FDCAN_FILTER_MASK;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1 = CAN_MASTER_ID;  /* 0x353 - используется для всех трех типов пакетов */
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* OEM packet 0x351 (все слушают) */
    f.FilterIndex = 3;
    f.FilterID1 = CAN_OEM_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Extended ambient packet теперь использует тот же ID 0x353, различается по типу */
    /* Фильтр уже настроен выше для CAN_MASTER_ID (0x353) */

    HAL_FDCAN_ActivateNotification(g_can,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
        0);

    HAL_FDCAN_Start(g_can);
}

/* ========== RX PROCESSING ========== */
static void handle_oem(const uint8_t *d, uint8_t len)
{
    if (len < 3) return;

    uint8_t br_raw = d[0];   // 0..5
    uint8_t col    = d[2];   // 0=Amber,1=Blue,2=White

    g_amb_can.oem_color = col;
    g_amb_can.oem_brightness = (float)br_raw / 5.0f;

    /* OEM mode does NOT change theme_index */
}

static void handle_ext(const uint8_t *d, uint8_t len)
{
    if (len < 3) return;

    /* Формат extended пакета: data[0] = 0x30 | flags, data[1] = bank_id, data[2] = theme_index */
    uint8_t flags    = d[0] & 0x0F;  /* Извлекаем только flags (младшие 4 бита) */
    uint8_t bank_id  = d[1];
    uint8_t theme_id = d[2];

    g_amb_can.extended_mode = (flags & EXT_FLAG_EXTENDED) ? 1 : 0;
    g_amb_can.night_mode    = (flags & EXT_FLAG_NIGHT) ? 1 : 0;

    g_amb_can.bank_id       = bank_id;
    g_amb_can.theme_index   = theme_id;
}

/* ========== MASTER PACKET HANDLING ========== */
static void handle_master(const uint8_t *d, uint8_t len)
{
    if (len < 4) return;

    /* Формат master пакета:
     * [0] = 0x10 | flags (extended_mode, night_mode в младших битах)
     * [1] = bank_id
     * [2] = theme_index
     * [3] = oem_color (0=Amber,1=Blue,2=White)
     * [4] = brightness_raw (0..5)
     */
    uint8_t flags = d[0] & 0x0F;  /* Извлекаем только flags (младшие 4 бита) */
    uint8_t bank_id = d[1];
    uint8_t theme_id = d[2];
    uint8_t oem_col = d[3];
    uint8_t br_raw = (len > 4) ? d[4] : 5;

    g_amb_can.extended_mode = (flags & EXT_FLAG_EXTENDED) ? 1 : 0;
    g_amb_can.night_mode    = (flags & EXT_FLAG_NIGHT) ? 1 : 0;
    g_amb_can.bank_id       = bank_id;
    g_amb_can.theme_index   = theme_id;
    g_amb_can.oem_color     = oem_col;
    g_amb_can.oem_brightness = (float)br_raw / 5.0f;
}

/* ========== DISCOVERY HANDLING ========== */
static void handle_discovery(const uint8_t *d, uint8_t len)
{
    if (len < 2) return;
    
    uint8_t board_type = d[0] & 0x0F;  /* Извлекаем board_type (младшие 4 бита) */
    
    /* Запоминаем, что эта плата присутствует */
    if (board_type < 6) {
        g_discovered_boards[board_type] = 1;
    }
}

/* ========== SYNC HANDLING ========== */
static void handle_sync(const uint8_t *d, uint8_t len)
{
    if (len < 2) return;
    
    /* Master жив, обновляем время последнего heartbeat */
    g_last_master_heartbeat_ms = HAL_GetTick();
    
    /* Формат sync: data[0] = 0x20 | bank_id, data[1] = theme_index */
    /* Можно обновить bank_id и theme_index из sync пакета для синхронизации */
    uint8_t bank_id = d[0] & 0x0F;  /* Извлекаем bank_id (младшие 4 бита) */
    uint8_t theme_id = d[1];
    
    /* Обновляем состояние из sync пакета (опционально) */
    g_amb_can.bank_id = bank_id;
    g_amb_can.theme_index = theme_id;
    
    /* Если мы были в режиме failover и получили sync от master - остаемся slave */
    if (g_role == 1 && !can_ambient_is_master()) {
        /* Проверяем, есть ли плата с более высоким приоритетом */
        /* Приоритет: FL > FR > DASHBOARD > RL > RR > REAR */
        if (g_discovered_boards[BOARD_TYPE_FL] && BOARD_TYPE != BOARD_TYPE_FL) {
            g_role = 0;  /* FL существует, мы не master */
        } else if (g_discovered_boards[BOARD_TYPE_FR] && 
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR) {
            g_role = 0;  /* FR существует, мы не master */
        } else if (g_discovered_boards[BOARD_TYPE_DASHBOARD] && 
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR &&
                   BOARD_TYPE != BOARD_TYPE_DASHBOARD) {
            g_role = 0;  /* DASHBOARD существует, мы не master */
        }
    }
}

void can_ambient_process_rx(uint32_t id, uint8_t *data, uint8_t len)
{
    if (id == CAN_MASTER_ID) {
        /* Все пакеты с ID 0x353 различаются по типу в data[0] */
        if (len < 1) return;
        
        uint8_t pkt_type = data[0] & PKT_TYPE_MASK;
        
        if (pkt_type == PKT_TYPE_DISCOVERY) {
            /* Discovery пакет: data[0] = board_type (0-5) */
            if (len >= 2 && (data[0] & ~PKT_TYPE_MASK) < 6) {
        handle_discovery(data, len);
            }
            return;
        } else if (pkt_type == PKT_TYPE_MASTER) {
            /* Master пакет: data[0] = 0x10 | flags, data[1-4] = state */
            if (can_ambient_is_master()) {
                /* Master не обрабатывает свои собственные пакеты */
        return;
    }
            handle_master(data, len);
            /* Обновляем heartbeat при получении master пакета */
            g_last_master_heartbeat_ms = HAL_GetTick();
            return;
        } else if (pkt_type == PKT_TYPE_SYNC) {
            /* Sync пакет: data[0] = 0x20 | bank_id, data[1] = theme_index */
        handle_sync(data, len);
            return;
        } else if (pkt_type == PKT_TYPE_EXT) {
            /* Extended пакет: data[0] = 0x30 | flags, data[1] = bank_id, data[2] = theme_index */
            if (can_ambient_is_master()) {
                /* Master не обрабатывает свои собственные пакеты */
                return;
            }
            handle_ext(data, len);
            /* Обновляем heartbeat при получении ext пакета */
            g_last_master_heartbeat_ms = HAL_GetTick();
            return;
        }
        /* Неизвестный тип пакета - игнорируем */
        return;
    }
    
    if (can_ambient_is_master()) {
        /* Master обрабатывает пакеты от машины */
        if (id == CAN_OEM_ID) {
            handle_oem(data, len);
        }
        /* CAN_EXT_ID теперь отправляется от master, а не принимается */
    }
}

/* ========== THEME SELECTION LOGIC ========== */

static ws_theme_id_t pick_oem_theme(void)
{
    switch (g_amb_can.oem_color) {
    case 0:  // Amber
        return G_AMBER_THEMES[0];
    case 1:  // Blue
        return G_BLUE_THEMES[0];
    case 2:  // White
        return G_WHITE_THEMES[0];
    default:
        return G_AMBER_THEMES[0];
    }
}

static ws_theme_id_t pick_ext_theme(void)
{
    const ws_theme_id_t *bank = NULL;
    uint8_t count = 0;

    switch (g_amb_can.bank_id) {
    case 1: bank = G_AMBER_THEMES; count = G_AMBER_COUNT; break;
    case 2: bank = G_BLUE_THEMES;  count = G_BLUE_COUNT;  break;
    case 3: bank = G_WHITE_THEMES; count = G_WHITE_COUNT; break;
    default: /* auto from OEM color */
        switch (g_amb_can.oem_color) {
        case 0: bank = G_AMBER_THEMES; count = G_AMBER_COUNT; break;
        case 1: bank = G_BLUE_THEMES;  count = G_BLUE_COUNT;  break;
        case 2: bank = G_WHITE_THEMES; count = G_WHITE_COUNT; break;
        }
    }

    if (!bank || count == 0)
        return 0;

    uint8_t idx = g_amb_can.theme_index % count;
    return bank[idx];
}

/* ========== UPDATE PLAYER ========== */

void can_ambient_update(ws2812_t *ws, scene_player_t *pl)
{
    if (!pl) return;

    /* Brightness (always follows OEM) - обновляем theme_dimming */
    pl->theme_dimming = g_amb_can.oem_brightness;
    pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
    if (pl->calc_brightness > 1.0f) pl->calc_brightness = 1.0f;
    if (pl->calc_brightness < 0.0f) pl->calc_brightness = 0.0f;

    ws_theme_id_t theme_now;

    if (g_amb_can.extended_mode == 0) {
        theme_now = pick_oem_theme();
    } else {
        theme_now = pick_ext_theme();
    }

    /* Check if changed */
    if (pl->theme != theme_now) {
        if (ws) {
            player_start_theme(ws, pl, theme_now);
        } else {
            /* Если ws не передан, просто обновляем тему без визуализации */
            pl->theme = theme_now;
            const ws_theme_desc_t *T = ws_theme_get(theme_now);
            if (T) {
                pl->theme_brightness = T->theme_brightness;
                pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
                if (pl->calc_brightness > 1.0f) pl->calc_brightness = 1.0f;
                if (pl->calc_brightness < 0.0f) pl->calc_brightness = 0.0f;
                pl->stage = PST_SCENE;
                pl->t0_ms = HAL_GetTick();
            }
        }
    }

    /* Night mode -> global dimming flag */
    g_amb_night_mode = g_amb_can.night_mode;
}

/* ========== MASTER MODE FUNCTIONS ========== */

uint8_t can_ambient_is_master(void)
{
    /* Если роль определена статически через define */
    if (AMB_ROLE_MASTER >= 0) {
        return (uint8_t)AMB_ROLE_MASTER;
    }
    
    /* Иначе используем динамически определенную роль */
    return (g_role == 1) ? 1 : 0;
}

void can_ambient_update_role(uint32_t now_ms)
{
    /* Если роль задана статически, не меняем её */
    if (AMB_ROLE_MASTER >= 0) {
        g_role = AMB_ROLE_MASTER;
        return;
    }
    
    /* Автоматическое определение master по приоритету */
    /* Приоритет: FL > FR > DASHBOARD > RL > RR > REAR */
    
    uint8_t should_be_master = 0;
    
    /* Проверяем, должны ли мы быть master */
    if (BOARD_TYPE == BOARD_TYPE_FL) {
        /* FL - наивысший приоритет */
        should_be_master = 1;
    } else if (BOARD_TYPE == BOARD_TYPE_FR) {
        /* FR может быть master, если нет FL */
        should_be_master = !g_discovered_boards[BOARD_TYPE_FL];
    } else if (BOARD_TYPE == BOARD_TYPE_DASHBOARD) {
        /* DASHBOARD может быть master, если нет FL и FR */
        should_be_master = !g_discovered_boards[BOARD_TYPE_FL] && 
                          !g_discovered_boards[BOARD_TYPE_FR];
    } else if (BOARD_TYPE == BOARD_TYPE_RL) {
        /* RL может быть master, если нет FL, FR, DASHBOARD */
        should_be_master = !g_discovered_boards[BOARD_TYPE_FL] && 
                          !g_discovered_boards[BOARD_TYPE_FR] &&
                          !g_discovered_boards[BOARD_TYPE_DASHBOARD];
    } else if (BOARD_TYPE == BOARD_TYPE_RR) {
        /* RR может быть master, если нет FL, FR, DASHBOARD, RL */
        should_be_master = !g_discovered_boards[BOARD_TYPE_FL] && 
                          !g_discovered_boards[BOARD_TYPE_FR] &&
                          !g_discovered_boards[BOARD_TYPE_DASHBOARD] &&
                          !g_discovered_boards[BOARD_TYPE_RL];
    } else if (BOARD_TYPE == BOARD_TYPE_REAR) {
        /* REAR может быть master, если нет всех остальных */
        should_be_master = !g_discovered_boards[BOARD_TYPE_FL] && 
                          !g_discovered_boards[BOARD_TYPE_FR] &&
                          !g_discovered_boards[BOARD_TYPE_DASHBOARD] &&
                          !g_discovered_boards[BOARD_TYPE_RL] &&
                          !g_discovered_boards[BOARD_TYPE_RR];
    }
    
    /* Проверяем timeout master heartbeat */
    if (g_role == 0 && should_be_master) {
        /* Мы slave, но master не отвечает */
        uint32_t timeout = now_ms - g_last_master_heartbeat_ms;
        if (g_last_master_heartbeat_ms == 0 || timeout > MASTER_HEARTBEAT_TIMEOUT_MS) {
            /* Master пропал, берем на себя роль */
            g_role = 1;
            g_last_master_heartbeat_ms = now_ms;  /* сбрасываем для себя */
        }
    } else if (g_role == 1 && !should_be_master) {
        /* Мы master, но появилась плата с более высоким приоритетом */
        /* Проверяем по приоритету: FL > FR > DASHBOARD > RL > RR > REAR */
        if (g_discovered_boards[BOARD_TYPE_FL] && BOARD_TYPE != BOARD_TYPE_FL) {
            g_role = 0;  /* FL существует, мы не master */
        } else if (g_discovered_boards[BOARD_TYPE_FR] && 
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR) {
            g_role = 0;  /* FR существует, мы не master */
        } else if (g_discovered_boards[BOARD_TYPE_DASHBOARD] && 
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR &&
                   BOARD_TYPE != BOARD_TYPE_DASHBOARD) {
            g_role = 0;  /* DASHBOARD существует, мы не master */
        } else if (g_discovered_boards[BOARD_TYPE_RL] && 
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR &&
                   BOARD_TYPE != BOARD_TYPE_DASHBOARD && BOARD_TYPE != BOARD_TYPE_RL) {
            g_role = 0;  /* RL существует, мы не master */
        } else if (g_discovered_boards[BOARD_TYPE_RR] && 
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR &&
                   BOARD_TYPE != BOARD_TYPE_DASHBOARD && BOARD_TYPE != BOARD_TYPE_RL &&
                   BOARD_TYPE != BOARD_TYPE_RR) {
            g_role = 0;  /* RR существует, мы не master */
        }
    } else if (g_role == -1) {
        /* Первый запуск - определяем роль */
        g_role = should_be_master ? 1 : 0;
    }
    
    /* Отправляем discovery пакеты */
    if (now_ms - g_last_discovery_send_ms >= DISCOVERY_INTERVAL_MS) {
        can_ambient_send_discovery_packet();
        g_last_discovery_send_ms = now_ms;
    }
    
    /* Master отправляет sync пакеты */
    if (can_ambient_is_master() && (now_ms - g_last_sync_send_ms >= SYNC_INTERVAL_MS)) {
        can_ambient_send_sync_packet();
        g_last_sync_send_ms = now_ms;
    }
    
    /* Master отправляет extended пакеты для синхронизации расширенного режима */
    /* Отправляем с той же частотой, что и sync пакеты, когда включен extended режим */
    if (can_ambient_is_master() && g_amb_can.extended_mode && 
        (now_ms - g_last_ext_send_ms >= SYNC_INTERVAL_MS)) {
        can_ambient_send_ext_packet();
        g_last_ext_send_ms = now_ms;
    }
    
    /* Стареем информацию о других платах (если не получаем discovery 5 сек) */
    static uint32_t last_discovery_cleanup_ms = 0;
    if (now_ms - last_discovery_cleanup_ms >= 5000u) {
        /* Discovery пакеты обновляют g_discovered_boards автоматически */
        /* Если плата не отправляет discovery 5 сек, она считается отсутствующей */
        last_discovery_cleanup_ms = now_ms;
    }
}

uint32_t can_ambient_get_last_master_heartbeat_ms(void)
{
    return g_last_master_heartbeat_ms;
}

void can_ambient_send_master_packet(void)
{
    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    FDCAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8];

    /* Формируем пакет с текущим состоянием */
    uint8_t flags = 0;
    if (g_amb_can.extended_mode) flags |= EXT_FLAG_EXTENDED;
    if (g_amb_can.night_mode) flags |= EXT_FLAG_NIGHT;

    tx_data[0] = PKT_TYPE_MASTER | (flags & 0x0F);  /* Тип пакета + flags */
    tx_data[1] = g_amb_can.bank_id;
    tx_data[2] = g_amb_can.theme_index;
    tx_data[3] = g_amb_can.oem_color;
    tx_data[4] = (uint8_t)(g_amb_can.oem_brightness * 5.0f);
    tx_data[5] = 0;  /* резерв */
    tx_data[6] = 0;  /* резерв */
    tx_data[7] = 0;  /* резерв */

    tx_header.Identifier = CAN_MASTER_ID;
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;

    uint32_t tx_mailbox;
    HAL_FDCAN_AddMessageToTxFifoQ(g_can, &tx_header, tx_data, &tx_mailbox);
}

void can_ambient_send_sync_packet(void)
{
    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    FDCAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8];

    /* Формат sync пакета: data[0] = 0x20 | bank_id, data[1] = theme_index */
    tx_data[0] = PKT_TYPE_SYNC | (g_amb_can.bank_id & 0x0F);
    tx_data[1] = g_amb_can.theme_index;
    tx_data[2] = 0;
    tx_data[3] = 0;
    tx_data[4] = 0;
    tx_data[5] = 0;
    tx_data[6] = 0;
    tx_data[7] = 0;

    tx_header.Identifier = CAN_MASTER_ID;  /* Используем тот же ID */
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;

    uint32_t tx_mailbox;
    HAL_FDCAN_AddMessageToTxFifoQ(g_can, &tx_header, tx_data, &tx_mailbox);
}

void can_ambient_send_ext_packet(void)
{
    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    FDCAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8];

    /* Формат extended пакета: data[0] = 0x30 | flags, data[1] = bank_id, data[2] = theme_index */
    uint8_t flags = 0;
    if (g_amb_can.extended_mode) flags |= EXT_FLAG_EXTENDED;
    if (g_amb_can.night_mode) flags |= EXT_FLAG_NIGHT;

    tx_data[0] = PKT_TYPE_EXT | (flags & 0x0F);  /* Тип пакета + flags */
    tx_data[1] = g_amb_can.bank_id;
    tx_data[2] = g_amb_can.theme_index;
    tx_data[3] = 0;
    tx_data[4] = 0;
    tx_data[5] = 0;
    tx_data[6] = 0;
    tx_data[7] = 0;

    tx_header.Identifier = CAN_MASTER_ID;  /* Используем тот же ID */
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;

    uint32_t tx_mailbox;
    HAL_FDCAN_AddMessageToTxFifoQ(g_can, &tx_header, tx_data, &tx_mailbox);
}

void can_ambient_send_discovery_packet(void)
{
    if (!g_can) {
        return;
    }

    FDCAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8];

    /* Формат discovery пакета: data[0] = board_type (0-5), unique_id */
    tx_data[0] = PKT_TYPE_DISCOVERY | (BOARD_TYPE & 0x0F);
    tx_data[1] = (uint8_t)(g_board_unique_id & 0xFF);
    tx_data[2] = (uint8_t)((g_board_unique_id >> 8) & 0xFF);
    tx_data[3] = (uint8_t)((g_board_unique_id >> 16) & 0xFF);
    tx_data[4] = (uint8_t)((g_board_unique_id >> 24) & 0xFF);
    tx_data[5] = can_ambient_is_master() ? 1 : 0;  /* текущая роль */
    tx_data[6] = 0;
    tx_data[7] = 0;

    tx_header.Identifier = CAN_DISCOVERY_ID;
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;

    uint32_t tx_mailbox;
    HAL_FDCAN_AddMessageToTxFifoQ(g_can, &tx_header, tx_data, &tx_mailbox);
}
