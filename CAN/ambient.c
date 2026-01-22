/**
 ******************************************************************************
 * @file    ambient.c
 * @brief   CAN communication implementation for ambient lighting system
 * @details Implements master/slave CAN protocol with automatic discovery,
 *          failover mechanisms, and synchronized theme control.
 *
 * @section Architecture
 * The system uses a master/slave architecture:
 * - Master board reads OEM CAN packets and broadcasts state to all slaves
 * - Slaves receive master packets and synchronize their lighting state
 * - Automatic role discovery based on board type priority
 * - Failover mechanism if master fails (1 second heartbeat timeout)
 *
 * @section CAN Protocol
 * Uses two CAN IDs:
 * - 0x325: OEM packets from vehicle (brightness, color)
 * - 0x353: Unified Master Protocol (discovery, sync, master, extended)
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"
#include "palette.h"
#include "presets.h"
#include "scene_player.h"
#include "features.h"
#include "flash_storage.h"
#include "driver.h"
#include <math.h>
#include <string.h>

/* ========== GLOBAL STATE (volatile для thread-safety) ========== */
volatile amb_can_state_t g_amb_can = {
    .oem_color = 0,
    .oem_brightness = 1.0f,
    .night_mode = 0,
    .bank_id = 0,
    .theme_index = 0,
    .last_oem_color = 0xFF,              /* Invalid - первый вызов pick_oem_theme даст 1st theme */
    .oem_theme_indices = {0xFF, 0xFF, 0xFF}  /* 0xFF + 1 = 0 при первом визите */
};

static FDCAN_HandleTypeDef *g_can = NULL;
static volatile ws_theme_id_t g_pending_theme = 0;  /* Следующая тема для плавного перехода */
static volatile uint8_t g_has_pending_theme = 0;   /* Флаг наличия ожидающей темы */
static volatile uint8_t g_oem_packet_received = 0; /* Флаг получения первого OEM пакета */

/* OEM brightness constants */
#define OEM_BRIGHTNESS_MAX  5u  /* Max OEM brightness value (0..5) */
static volatile uint32_t g_last_settings_change_ms = 0;
static volatile uint8_t g_settings_changed = 0;  /* Settings change flag */

/* Last saved state for Flash wear optimization (only save if actually changed) */
static amb_can_state_t g_last_saved_state = {0};
static uint8_t g_last_saved_valid = 0;

/* ========== EXTENDED MODE TOGGLE ========== */
/* Extended mode удалён. Цветовые смены используются только для OEM циклического выбора тем. */

/* ========== MASTER/SLAVE STATE ========== */
static volatile int8_t g_role = -1;  /* -1 = unknown/auto, 0 = slave, 1 = master */
static volatile uint32_t g_last_master_heartbeat_ms = 0;
static volatile uint8_t g_discovered_boards[6] = {0, 0, 0, 0, 0, 0};  /* FL, FR, RL, RR, DASHBOARD, REAR */
static volatile uint32_t g_discovery_last_seen_ms[6] = {0, 0, 0, 0, 0, 0};
static uint32_t g_board_unique_id = 0;  /* можно использовать UID чипа */

/* ========== SLEEP MODE STATE ========== */
#if AMB_ENABLE_SLEEP_MODE
static volatile uint32_t g_last_can_activity_ms = 0;  /* Время последней CAN активности */
static volatile uint8_t g_sleep_requested = 0;         /* Флаг запроса на засыпание */
static volatile uint8_t g_is_sleeping = 0;             /* Флаг текущего состояния сна */
#endif

static void reset_fx_state(scene_player_t *p)
{
    if (!p) return;
    p->st_scene.t = 0.0f;
    p->st_scene.speed = 0.0f;
    p->st_scene.phase = 0.0f;
    p->st_scene.a = 0.0f;
    p->st_scene.b = 0.0f;
    p->st_scene_next.t = 0.0f;
    p->st_scene_next.speed = 0.0f;
    p->st_scene_next.phase = 0.0f;
    p->st_scene_next.a = 0.0f;
    p->st_scene_next.b = 0.0f;
}

/* ========== INIT ========== */
void can_ambient_init(FDCAN_HandleTypeDef *hfdcan)
{
    g_can = hfdcan;
    g_role = -1;  /* unknown, будет определен через discovery */
    g_last_master_heartbeat_ms = 0;
    for (uint8_t i = 0; i < 6; i++) {
        g_discovered_boards[i] = 0;
        g_discovery_last_seen_ms[i] = 0;
    }

    /* Load saved settings from Flash */
    if (flash_storage_load((amb_can_state_t *)&g_amb_can) == 0) {
        /* Successfully loaded - initialize last saved state for wear optimization */
        memcpy(&g_last_saved_state, (const void *)&g_amb_can, sizeof(amb_can_state_t));
        g_last_saved_valid = 1;
    }

    /* Initialize save flags */
    g_settings_changed = 0;
    g_last_settings_change_ms = HAL_GetTick();

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

    /* OEM packet 0x325 (все слушают) */
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
    if (len < 4) return;

    /* Машина реально шлёт яркость сразу в байте 0 (0..5), хотя в DBC указаны биты 5-7 */
    uint8_t br_raw = d[0];
    /* Машина реально шлёт цвет в байте 3, биты 2-3:
     * 00 = Solar/Amber, 01 = Neutral/White, 10 = Polar/Blue
     * Внутренняя нумерация: 0=Amber, 1=Blue, 2=White
     */
    uint8_t raw_col = (d[3] >> 2) & 0x03;
    uint8_t col;

    switch (raw_col) {
        case 0: col = OEM_COLOR_AMBER; break;
        case 1: col = OEM_COLOR_WHITE; break;
        case 2: col = OEM_COLOR_BLUE;  break;
        default: return;  /* Reserved / invalid */
    }

    /* Валидация входных данных */
    if (col > 2) return;  /* oem_color должен быть 0-2 */
    if (br_raw > OEM_BRIGHTNESS_MAX) br_raw = OEM_BRIGHTNESS_MAX;  /* Ограничиваем яркость */

    /* Помечаем что первый OEM пакет получен */
    g_oem_packet_received = 1;

#if AMB_ENABLE_SLEEP_MODE
    /* Обновляем время последней CAN активности */
    g_last_can_activity_ms = HAL_GetTick();
    g_is_sleeping = 0;  /* Пробуждаемся если были в режиме сна */
#endif

    /* Критическая секция для изменения g_amb_can из ISR */
    __disable_irq();
    uint8_t old_color = g_amb_can.oem_color;
    g_amb_can.oem_color = col;
    g_amb_can.oem_brightness = (float)br_raw / (float)OEM_BRIGHTNESS_MAX;

    (void)old_color;
    __enable_irq();  /* Восстанавливаем прерывания после критической секции */
}

/* ========== MASTER PACKET HANDLING ========== */
static void handle_master(const uint8_t *d, uint8_t len)
{
    if (len < 4) return;

    /* Формат master пакета:
     * [0] = 0x10 | flags (night_mode в бит0)
     * [1] = bank_id (0..2 = OEM color)
     * [2] = theme_index
     * [3] = oem_color (0=Amber,1=Blue,2=White)
     * [4] = brightness_raw (0..5)
     */
    uint8_t flags = d[0] & 0x0F;  /* Извлекаем только flags (младшие 4 бита) */
    uint8_t bank_id = d[1];
    uint8_t theme_id = d[2];
    uint8_t oem_col = d[3];
    uint8_t br_raw = (len > 4) ? d[4] : 5;

    /* Валидация входных данных */
    if (bank_id > 2) return;  /* bank_id должен быть 0-2 (OEM color) */
    if (oem_col > 2) return;  /* oem_color должен быть 0-2 */
    if (br_raw > 5) br_raw = 5;  /* brightness_raw ограничиваем 0-5 */

    /* Помечаем что данные от master получены (для slave плат) */
    g_oem_packet_received = 1;

#if AMB_ENABLE_SLEEP_MODE
    /* Обновляем время последней CAN активности */
    g_last_can_activity_ms = HAL_GetTick();
    g_is_sleeping = 0;  /* Пробуждаемся если были в режиме сна */
#endif

    /* Критическая секция для изменения g_amb_can из ISR */
    __disable_irq();
    uint8_t old_bank = g_amb_can.bank_id;
    uint8_t old_theme = g_amb_can.theme_index;

    g_amb_can.night_mode    = (flags & MASTER_FLAG_NIGHT) ? 1 : 0;
    g_amb_can.bank_id       = bank_id;
    g_amb_can.theme_index   = theme_id;
    g_amb_can.oem_color     = oem_col;
    g_amb_can.oem_brightness = (float)br_raw / (float)OEM_BRIGHTNESS_MAX;
    /* Обновляем циклический индекс банка для корректного сохранения */
    g_amb_can.oem_theme_indices[oem_col] = theme_id % 255;

    /* Помечаем настройки как измененные, если что-то изменилось */
    if (old_bank != g_amb_can.bank_id ||
        old_theme != g_amb_can.theme_index) {
        g_settings_changed = 1;
        g_last_settings_change_ms = HAL_GetTick();
    }
    __enable_irq();  /* Восстанавливаем прерывания после критической секции */
}

/* ========== DISCOVERY HANDLING ========== */
static void handle_discovery(const uint8_t *d, uint8_t len)
{
    if (len < 2) return;

    uint8_t board_type = d[0] & 0x0F;  /* Извлекаем board_type (младшие 4 бита) */

    /* Запоминаем, что эта плата присутствует */
    if (board_type < 6) {
        /* Критическая секция для изменения g_discovered_boards из ISR */
        __disable_irq();
        g_discovered_boards[board_type] = 1;
        g_discovery_last_seen_ms[board_type] = HAL_GetTick();
        __enable_irq();
    }
}

/* ========== SYNC HANDLING ========== */
static void handle_sync(const uint8_t *d, uint8_t len)
{
    if (len < 2) return;

    /* Формат sync: data[0] = 0x20 | bank_id, data[1] = theme_index */
    /* Можно обновить bank_id и theme_index из sync пакета для синхронизации */
    uint8_t bank_id = d[0] & 0x0F;  /* Извлекаем bank_id (младшие 4 бита) */
    uint8_t theme_id = d[1];

    /* Критическая секция для изменения g_amb_can и g_last_master_heartbeat_ms из ISR */
    __disable_irq();
    /* Master жив, обновляем время последнего heartbeat */
    g_last_master_heartbeat_ms = HAL_GetTick();

    /* Обновляем состояние из sync пакета (опционально) */
    g_amb_can.bank_id = bank_id;
    g_amb_can.theme_index = theme_id;
    /* Обновляем циклический индекс для сохранения */
    g_amb_can.oem_theme_indices[bank_id] = theme_id % 255;
    __enable_irq();

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
    /* Пока роль не определена, принимаем OEM пакет для старта протокола */
    if (g_role == -1 && id == CAN_OEM_ID) {
        handle_oem(data, len);
    }
}

/* ========== THEME SELECTION LOGIC ========== */

/* Флаг что индексы изменились и нужно сохранить в flash */
static uint8_t g_oem_indices_dirty = 0;

static ws_theme_id_t pick_oem_theme(void)
{
    /* Атомарное чтение для защиты от race conditions */
    uint8_t oem_color, last_oem_color;
    uint8_t oem_theme_indices[3];
    __disable_irq();
    oem_color = g_amb_can.oem_color;
    last_oem_color = g_amb_can.last_oem_color;
    for (int i = 0; i < 3; i++) {
        oem_theme_indices[i] = g_amb_can.oem_theme_indices[i];
    }
    __enable_irq();

    if (oem_color >= OEM_COLOR_MAX) {
        return ws_theme_default_for_oem(OEM_COLOR_AMBER);
    }

    const ws_theme_bank_t *bank = ws_theme_get_bank((oem_color_id_t)oem_color);
    if (!bank || !bank->themes || bank->count == 0) {
        return ws_theme_default_for_oem((oem_color_id_t)oem_color);
    }

    /* При смене OEM цвета - переходим к следующей теме в новом банке */
    if (last_oem_color != oem_color) {
        /* Инкрементируем индекс для нового банка (циклически) */
        /* 0xFF + 1 = 0 (первый визит даст 1st theme) */
        uint8_t next_idx = oem_theme_indices[oem_color] + 1;
        if (next_idx >= bank->count) {
            next_idx = 0;
        }

        /* Обновляем состояние атомарно */
        __disable_irq();
        g_amb_can.oem_theme_indices[oem_color] = next_idx;
        g_amb_can.last_oem_color = oem_color;
        __enable_irq();

        oem_theme_indices[oem_color] = next_idx;
        g_oem_indices_dirty = 1;  /* Пометить для сохранения в flash */
    }

    /* Обновляем синхронизационные поля для мастера/слейва */
    __disable_irq();
    g_amb_can.bank_id = oem_color;                 /* 0..2 */
    g_amb_can.theme_index = g_amb_can.oem_theme_indices[oem_color];
    __enable_irq();

    /* Если индекс/банк изменились – помечаем для сохранения в flash */
    static uint8_t last_bank = 0xFF;
    static uint8_t last_theme_idx = 0xFF;
    uint8_t bank_now = oem_color;
    uint8_t theme_idx_now = g_amb_can.oem_theme_indices[oem_color];
    if (bank_now != last_bank || theme_idx_now != last_theme_idx) {
        g_settings_changed = 1;
        g_last_settings_change_ms = HAL_GetTick();
        last_bank = bank_now;
        last_theme_idx = theme_idx_now;
    }

    return bank->themes[oem_theme_indices[oem_color]];
}

/* ========== UPDATE PLAYER ========== */

/* OEM brightness LUT (steps 0..5 → perceptual dimming 0..1) */
static const float g_oem_brightness_lut[OEM_BRIGHTNESS_MAX + 1] = {
    0.00f,  /* 0 */
    0.32f,  /* 1 */
    0.46f,  /* 2 */
    0.60f,  /* 3 */
    0.72f,  /* 4 */
    0.86f   /* 5 */
};

static float map_oem_brightness(float x)
{
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;
    /* Приводим нормализованное значение обратно к шагу 0..5 и берём из LUT */
    int idx = (int)(x * (float)OEM_BRIGHTNESS_MAX + 0.5f);
    if (idx < 0) idx = 0;
    if (idx > (int)OEM_BRIGHTNESS_MAX) idx = OEM_BRIGHTNESS_MAX;
    return g_oem_brightness_lut[idx];
}

static float theme_base_brightness(const ws_theme_desc_t *T)
{
    if (!T) return AMB_THEME_MIN_BRIGHTNESS;
    float tb = T->theme_brightness;
    if (tb < AMB_THEME_MIN_BRIGHTNESS) tb = AMB_THEME_MIN_BRIGHTNESS;
    if (tb > 1.0f) tb = 1.0f;
    return tb;
}

/* Плавный переход яркости (slew) за ~200 мс на полный ход */
static float slew_dimming(float current, float target, uint32_t now_ms, uint32_t *last_ms)
{
    if (!last_ms) return target;
    if (*last_ms == 0u) {
        *last_ms = now_ms;
        return target;
    }
    uint32_t dt = (now_ms >= *last_ms) ? (now_ms - *last_ms) : 0u;
    *last_ms = now_ms;
    float max_delta = (float)dt / 200.0f;  /* полный шаг за 200 мс */
    if (max_delta > 1.0f) max_delta = 1.0f;
    float delta = target - current;
    if (delta > max_delta) delta = max_delta;
    if (delta < -max_delta) delta = -max_delta;
    return current + delta;
}

void can_ambient_update(ws2812_t *ws, scene_player_t *pl)
{
    if (!pl) return;

    static float smoothed_dimming = 1.0f;
    static uint32_t smoothed_last_ms = 0;

    /* Atomic copy of state using memcpy (struct assignment may not be atomic) */
    amb_can_state_t state;
    __disable_irq();
    memcpy(&state, (const void *)&g_amb_can, sizeof(amb_can_state_t));
    __enable_irq();

    ws_theme_id_t theme_now = pick_oem_theme();
    uint8_t same_bank_now = ws_theme_same_bank(pl->theme, theme_now);
    uint8_t theme_change_requires_outro = 0;

    /* В демо режиме для master пропускаем смену темы через CAN - тема управляется через demo_mode_update() */
    /* Проверяем, не включен ли демо режим (через проверку DEMO_MODE, если он определен) */
#if defined(DEMO_MODE) && DEMO_MODE == 1
    if (can_ambient_is_master()) {
        /* В демо режиме для master не меняем тему через CAN - она управляется через demo_mode_update() */
        /* Просто обновляем яркость и ночной режим */
        g_amb_night_mode = state.night_mode;
        return;
    }
#endif

    /* Дежурное определение, будет ли смена темы требовать outro на этом тике */
    if (pl->stage == PST_SCENE && !same_bank_now && pl->theme != theme_now) {
        theme_change_requires_outro = 1;
    }

    /* Brightness (always follows OEM), но во время OUTRO или при старте outro удерживаем прежнее значение,
       чтобы новые OEM пакеты не влияли на завершающий fade-out. Дополнительно плавно тянем шаги (~200 мс). */
    static float last_non_outro_dimming = 1.0f;
    float target_dimming = map_oem_brightness(state.oem_brightness);

    uint8_t hold_dimming = (pl->stage == PST_OUTRO) || theme_change_requires_outro;
    if (!hold_dimming) {
        last_non_outro_dimming = target_dimming;
    } else {
        target_dimming = last_non_outro_dimming;
    }

    /* Плавное изменение dimming, но не во время hold */
    uint32_t now_ms = HAL_GetTick();
    if (!hold_dimming) {
        smoothed_dimming = slew_dimming(smoothed_dimming, target_dimming, now_ms, &smoothed_last_ms);
    } else {
        smoothed_last_ms = now_ms;  /* сброс таймера, чтобы после hold не было скачка */
    }

    pl->theme_dimming = smoothed_dimming;

    pl->calc_brightness = pl->theme_brightness * smoothed_dimming;
    if (pl->calc_brightness > 1.0f) pl->calc_brightness = 1.0f;
    if (pl->calc_brightness < 0.0f) pl->calc_brightness = 0.0f;

    /* Проверяем, завершился ли outro, и нужно ли запустить intro новой темы */
    if (g_has_pending_theme && pl->stage == PST_IDLE) {
        /* outro завершен, запускаем intro новой темы */
        if (ws) {
            pl->theme = g_pending_theme;
            const ws_theme_desc_t *T = ws_theme_get(g_pending_theme);
            if (T) {
                pl->theme_brightness = theme_base_brightness(T);
                pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
                if (pl->calc_brightness > 1.0f) pl->calc_brightness = 1.0f;
                if (pl->calc_brightness < 0.0f) pl->calc_brightness = 0.0f;
                reset_fx_state(pl);
                player_start_intro(ws, pl);
            }
        } else {
            /* Если ws не передан, просто обновляем тему */
            pl->theme = g_pending_theme;
            const ws_theme_desc_t *T = ws_theme_get(g_pending_theme);
            if (T) {
                pl->theme_brightness = theme_base_brightness(T);
                pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
                if (pl->calc_brightness > 1.0f) pl->calc_brightness = 1.0f;
                if (pl->calc_brightness < 0.0f) pl->calc_brightness = 0.0f;
                reset_fx_state(pl);
                pl->stage = PST_SCENE;
                pl->t0_ms = HAL_GetTick();
            }
        }
        g_has_pending_theme = 0;
        return;
    }

    /* Проверяем, завершился ли intro, и нужно ли запустить outro для новой темы */
    /* НЕ запускаем outro если темы в одном банке (циклическое переключение) */
    if (g_has_pending_theme && pl->stage == PST_SCENE && pl->theme != g_pending_theme) {
        /* Темы в одном банке = циклическое переключение, outro не нужен */
        uint8_t same_bank = ws_theme_same_bank(pl->theme, g_pending_theme);
#if AMB_ENABLE_AUTO_ROTATE
        uint8_t skip_outro = same_bank || pl->crossfade_active;
#else
        uint8_t skip_outro = same_bank;
#endif
        if (!skip_outro) {
            /* intro завершен, но тема изменилась во время intro, запускаем outro */
            if (ws) {
                reset_fx_state(pl);
                player_start_outro(ws, pl);
            }
            return;
        }
    }

    /* Check if theme changed */
    /* Если тема в том же банке - НЕ запускаем outro (циклическое переключение) */
#if AMB_ENABLE_AUTO_ROTATE
    /* При авто-ротации также проверяем crossfade_active */
    uint8_t theme_really_changed = (pl->theme != theme_now)
                                 && !pl->crossfade_active
                                 && !same_bank_now;
#else
    uint8_t theme_really_changed = (pl->theme != theme_now) && !same_bank_now;
#endif
    if (theme_really_changed) {
        /* Тема изменилась на другой банк - используем outro/intro */
        if (ws) {
            /* Если мы в SCENE - делаем плавный переход через outro/intro */
            if (pl->stage == PST_SCENE) {
                /* Сохраняем новую тему для перехода */
                g_pending_theme = theme_now;
                g_has_pending_theme = 1;
                /* Запускаем outro текущей темы */
                reset_fx_state(pl);
                player_start_outro(ws, pl);
            } else if (pl->stage == PST_IDLE) {
                /* Если в IDLE - сразу запускаем intro новой темы */
                pl->theme = theme_now;
                const ws_theme_desc_t *T = ws_theme_get(theme_now);
                if (T) {
                    pl->theme_brightness = theme_base_brightness(T);
                    pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
                    if (pl->calc_brightness > 1.0f) pl->calc_brightness = 1.0f;
                    if (pl->calc_brightness < 0.0f) pl->calc_brightness = 0.0f;
                    reset_fx_state(pl);
                    player_start_intro(ws, pl);
                }
            } else {
                /* Если в INTRO/OUTRO - сохраняем новую тему для перехода */
                if (pl->stage == PST_OUTRO) {
                    /* outro уже идет, сохраняем новую тему для intro после outro */
                    g_pending_theme = theme_now;
                    g_has_pending_theme = 1;
                } else if (pl->stage == PST_INTRO) {
                    /* intro идет, сохраняем новую тему - после завершения intro запустим outro */
                    g_pending_theme = theme_now;
                    g_has_pending_theme = 1;
                } else {
                    /* Другие стадии - просто меняем тему напрямую */
            player_start_theme(ws, pl, theme_now);
                }
            }
        } else {
            /* Если ws не передан, просто обновляем тему без визуализации */
            pl->theme = theme_now;
            const ws_theme_desc_t *T = ws_theme_get(theme_now);
            if (T) {
                pl->theme_brightness = theme_base_brightness(T);
                pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
                if (pl->calc_brightness > 1.0f) pl->calc_brightness = 1.0f;
                if (pl->calc_brightness < 0.0f) pl->calc_brightness = 0.0f;
                reset_fx_state(pl);
                pl->stage = PST_SCENE;
                pl->t0_ms = HAL_GetTick();
            }
        }
    } else if (pl->theme != theme_now && same_bank_now) {
        /* Тема изменилась внутри того же банка (циклическое переключение) */
#if AMB_ENABLE_AUTO_ROTATE
        /* При включённой авто-ротации кроссфейд обрабатывается в scene_player.c */
        /* Просто обновляем тему - scene_player подхватит изменение */
        if (pl->stage == PST_SCENE && !pl->crossfade_active) {
            reset_fx_state(pl);
            player_start_theme(ws, pl, theme_now);
        }
#else
        /* При выключенной авто-ротации - применяем тему напрямую */
        if (pl->stage == PST_SCENE) {
            reset_fx_state(pl);
            player_start_theme(ws, pl, theme_now);
        }
#endif
    }

    /* Night mode -> global dimming flag */
    g_amb_night_mode = state.night_mode;
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

uint8_t can_ambient_oem_received(void)
{
    return g_oem_packet_received;
}

void can_ambient_reset_oem_received(void)
{
    __disable_irq();
    g_oem_packet_received = 0;
    if (AMB_ROLE_MASTER < 0) {
        g_role = -1;
    }
    g_last_master_heartbeat_ms = 0;
    memset((void *)g_discovered_boards, 0, sizeof(g_discovered_boards));
    memset((void *)g_discovery_last_seen_ms, 0, sizeof(g_discovery_last_seen_ms));
    __enable_irq();
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

    /* Атомарное чтение g_discovered_boards для защиты от race conditions */
    uint8_t discovered_boards[6];
    __disable_irq();
    for (uint8_t i = 0; i < 6; i++) {
        discovered_boards[i] = g_discovered_boards[i];
    }
    __enable_irq();

    uint8_t should_be_master = 0;

    /* Проверяем, должны ли мы быть master */
    if (BOARD_TYPE == BOARD_TYPE_FL) {
        /* FL - наивысший приоритет */
        should_be_master = 1;
    } else if (BOARD_TYPE == BOARD_TYPE_FR) {
        /* FR может быть master, если нет FL */
        should_be_master = !discovered_boards[BOARD_TYPE_FL];
    } else if (BOARD_TYPE == BOARD_TYPE_DASHBOARD) {
        /* DASHBOARD может быть master, если нет FL и FR */
        should_be_master = !discovered_boards[BOARD_TYPE_FL] &&
                          !discovered_boards[BOARD_TYPE_FR];
    } else if (BOARD_TYPE == BOARD_TYPE_RL) {
        /* RL может быть master, если нет FL, FR, DASHBOARD */
        should_be_master = !discovered_boards[BOARD_TYPE_FL] &&
                          !discovered_boards[BOARD_TYPE_FR] &&
                          !discovered_boards[BOARD_TYPE_DASHBOARD];
    } else if (BOARD_TYPE == BOARD_TYPE_RR) {
        /* RR может быть master, если нет FL, FR, DASHBOARD, RL */
        should_be_master = !discovered_boards[BOARD_TYPE_FL] &&
                          !discovered_boards[BOARD_TYPE_FR] &&
                          !discovered_boards[BOARD_TYPE_DASHBOARD] &&
                          !discovered_boards[BOARD_TYPE_RL];
    } else if (BOARD_TYPE == BOARD_TYPE_REAR) {
        /* REAR может быть master, если нет всех остальных */
        should_be_master = !discovered_boards[BOARD_TYPE_FL] &&
                          !discovered_boards[BOARD_TYPE_FR] &&
                          !discovered_boards[BOARD_TYPE_DASHBOARD] &&
                          !discovered_boards[BOARD_TYPE_RL] &&
                          !discovered_boards[BOARD_TYPE_RR];
    }

    /* Проверяем timeout master heartbeat */
    if (g_role == 0 && should_be_master) {
        /* Мы slave, но master не отвечает */
        /* Атомарное чтение для защиты от race condition */
        __disable_irq();
        uint32_t last_heartbeat = g_last_master_heartbeat_ms;
        __enable_irq();

        uint32_t timeout;
        if (last_heartbeat == 0) {
            timeout = UINT32_MAX;  /* Никогда не получали heartbeat */
        } else if (now_ms >= last_heartbeat) {
            timeout = now_ms - last_heartbeat;
        } else {
            /* Переполнение uint32_t */
            timeout = UINT32_MAX - last_heartbeat + now_ms;
        }

        if (timeout > MASTER_HEARTBEAT_TIMEOUT_MS) {
            /* Master пропал, берем на себя роль */
            __disable_irq();
            g_role = 1;
            g_last_master_heartbeat_ms = now_ms;  /* сбрасываем для себя */
            __enable_irq();
        }
    } else if (g_role == 1 && !should_be_master) {
        /* Мы master, но появилась плата с более высоким приоритетом */
        /* Проверяем по приоритету: FL > FR > DASHBOARD > RL > RR > REAR */
        if (discovered_boards[BOARD_TYPE_FL] && BOARD_TYPE != BOARD_TYPE_FL) {
            __disable_irq();
            g_role = 0;  /* FL существует, мы не master */
            __enable_irq();
        } else if (discovered_boards[BOARD_TYPE_FR] &&
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR) {
            __disable_irq();
            g_role = 0;  /* FR существует, мы не master */
            __enable_irq();
        } else if (discovered_boards[BOARD_TYPE_DASHBOARD] &&
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR &&
                   BOARD_TYPE != BOARD_TYPE_DASHBOARD) {
            __disable_irq();
            g_role = 0;  /* DASHBOARD существует, мы не master */
            __enable_irq();
        } else if (discovered_boards[BOARD_TYPE_RL] &&
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR &&
                   BOARD_TYPE != BOARD_TYPE_DASHBOARD && BOARD_TYPE != BOARD_TYPE_RL) {
            __disable_irq();
            g_role = 0;  /* RL существует, мы не master */
            __enable_irq();
        } else if (discovered_boards[BOARD_TYPE_RR] &&
                   BOARD_TYPE != BOARD_TYPE_FL && BOARD_TYPE != BOARD_TYPE_FR &&
                   BOARD_TYPE != BOARD_TYPE_DASHBOARD && BOARD_TYPE != BOARD_TYPE_RL &&
                   BOARD_TYPE != BOARD_TYPE_RR) {
            __disable_irq();
            g_role = 0;  /* RR существует, мы не master */
            __enable_irq();
        }
    } else if (g_role == -1) {
        /* Первый запуск - определяем роль */
        __disable_irq();
        g_role = should_be_master ? 1 : 0;
        __enable_irq();
    }

    /* Стареем информацию о других платах (если не получаем discovery 5 сек) */
    static uint32_t last_discovery_cleanup_ms = 0;
    if (now_ms - last_discovery_cleanup_ms >= 5000u) {
        /* Если плата не отправляет discovery 5 сек, она считается отсутствующей */
        __disable_irq();
        for (uint8_t i = 0; i < 6; i++) {
            uint32_t last_seen = g_discovery_last_seen_ms[i];
            if (last_seen == 0) {
                continue;
            }
            uint32_t age;
            if (now_ms >= last_seen) {
                age = now_ms - last_seen;
            } else {
                age = UINT32_MAX;
            }
            if (age > 5000u) {
                g_discovered_boards[i] = 0;
                g_discovery_last_seen_ms[i] = 0;
            }
        }
        __enable_irq();
        last_discovery_cleanup_ms = now_ms;
    }

    /* Сохраняем настройки в Flash, если они изменились и прошло достаточно времени */
    /* g_oem_indices_dirty также триггерит сохранение (циклические индексы тем) */
    if (g_settings_changed || g_oem_indices_dirty) {
        uint32_t time_since_change;
        if (now_ms >= g_last_settings_change_ms) {
            time_since_change = now_ms - g_last_settings_change_ms;
        } else {
            /* Переполнение uint32_t */
            time_since_change = UINT32_MAX;
        }

        /* Для oem_indices используем тот же delay чтобы не писать flash слишком часто */
        if (g_oem_indices_dirty && !g_settings_changed) {
            g_last_settings_change_ms = now_ms;
            g_settings_changed = 1;  /* Использовать стандартный механизм задержки */
            g_oem_indices_dirty = 0;
        }

        if (time_since_change >= AMB_FLASH_SAVE_DELAY_MS) {
            uint8_t saved_ok = 0;
            /* Only save on master board */
            if (can_ambient_is_master()) {
                /* Atomic copy of state for saving (memcpy for thread-safety) */
                amb_can_state_t state_to_save;
                __disable_irq();
                memcpy(&state_to_save, (const void *)&g_amb_can, sizeof(amb_can_state_t));
                __enable_irq();

                /* Flash wear optimization: only save if data actually changed */
                uint8_t data_changed = 0;
                if (!g_last_saved_valid) {
                    data_changed = 1;  /* First save */
                } else {
                    /* Compare relevant fields */
                    if (state_to_save.bank_id != g_last_saved_state.bank_id ||
                        state_to_save.theme_index != g_last_saved_state.theme_index ||
                        state_to_save.last_oem_color != g_last_saved_state.last_oem_color ||
                        state_to_save.oem_theme_indices[0] != g_last_saved_state.oem_theme_indices[0] ||
                        state_to_save.oem_theme_indices[1] != g_last_saved_state.oem_theme_indices[1] ||
                        state_to_save.oem_theme_indices[2] != g_last_saved_state.oem_theme_indices[2]) {
                        data_changed = 1;
                    }
                }

                if (data_changed) {
                    if (flash_storage_save(&state_to_save) == 0) {
                        /* Update last saved state on successful save */
                        memcpy(&g_last_saved_state, &state_to_save, sizeof(amb_can_state_t));
                        g_last_saved_valid = 1;
                        saved_ok = 1;
                    }
                } else {
                    /* Nothing changed, treat as success to clear flags */
                    saved_ok = 1;
                }
            }
            if (saved_ok) {
                g_settings_changed = 0;
                g_oem_indices_dirty = 0;
            }
        }
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

    /* Atomic copy of state using memcpy (struct assignment may not be atomic) */
    amb_can_state_t state;
    __disable_irq();
    memcpy(&state, (const void *)&g_amb_can, sizeof(amb_can_state_t));
    __enable_irq();

    /* Формируем пакет с текущим состоянием */
    uint8_t flags = 0;
    if (state.night_mode) flags |= MASTER_FLAG_NIGHT;

    tx_data[0] = PKT_TYPE_MASTER | (flags & 0x0F);  /* Тип пакета + flags */
    tx_data[1] = state.bank_id;
    tx_data[2] = state.theme_index;
    tx_data[3] = state.oem_color;
    tx_data[4] = (uint8_t)(state.oem_brightness * (float)OEM_BRIGHTNESS_MAX);
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

    HAL_StatusTypeDef status = HAL_FDCAN_AddMessageToTxFifoQ(g_can, &tx_header, tx_data);
    (void)status;  /* Игнорируем ошибку - в production можно добавить retry логику */
}

void can_ambient_send_sync_packet(void)
{
    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    FDCAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8];

    /* Атомарное чтение состояния для защиты от race conditions */
    uint8_t bank_id, theme_index;
    __disable_irq();
    bank_id = g_amb_can.bank_id;
    theme_index = g_amb_can.theme_index;
    __enable_irq();

    /* Формат sync пакета: data[0] = 0x20 | bank_id, data[1] = theme_index */
    tx_data[0] = PKT_TYPE_SYNC | (bank_id & 0x0F);
    tx_data[1] = theme_index;
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

    HAL_StatusTypeDef status = HAL_FDCAN_AddMessageToTxFifoQ(g_can, &tx_header, tx_data);
    (void)status;  /* Игнорируем ошибку - в production можно добавить retry логику */
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

    HAL_StatusTypeDef status = HAL_FDCAN_AddMessageToTxFifoQ(g_can, &tx_header, tx_data);
    (void)status;  /* Игнорируем ошибку - в production можно добавить retry логику */
}

/* ========== SLEEP MODE FUNCTIONS ========== */

#if AMB_ENABLE_SLEEP_MODE

uint8_t can_ambient_is_sleeping(void)
{
    return g_is_sleeping;
}

uint8_t can_ambient_should_sleep(void)
{
    return g_sleep_requested;
}

void can_ambient_clear_sleep_request(void)
{
    g_sleep_requested = 0;
}

void can_ambient_mark_awake(void)
{
    g_is_sleeping = 0;
    g_sleep_requested = 0;
    g_last_can_activity_ms = HAL_GetTick();
}

uint32_t can_ambient_get_idle_time_ms(void)
{
    uint32_t now = HAL_GetTick();
    if (now >= g_last_can_activity_ms) {
        return now - g_last_can_activity_ms;
    }
    /* Переполнение таймера */
    return (UINT32_MAX - g_last_can_activity_ms) + now + 1;
}

void can_ambient_check_sleep_timeout(void)
{
    if (g_is_sleeping || g_sleep_requested) {
        return;  /* Уже спим или уже запрошено */
    }

    uint32_t idle_ms = can_ambient_get_idle_time_ms();
    uint32_t timeout_ms = AMB_SLEEP_TIMEOUT_SEC * 1000u;

    if (idle_ms >= timeout_ms) {
        g_sleep_requested = 1;
    }
}

void can_ambient_enter_sleep(void)
{
    if (g_is_sleeping) {
        return;  /* Уже спим */
    }

    g_is_sleeping = 1;

    /* Отключаем питание ленты */
    ws_power_set(0);

    /* CAN трансивер в standby mode */
#ifdef FDCAN1_STBY_Pin
    HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_SET);
#endif

    /* Останавливаем FDCAN перед сном для корректного пробуждения */
    if (g_can) {
        HAL_FDCAN_Stop(g_can);
    }
}

void can_ambient_exit_sleep(void)
{
    if (!g_is_sleeping) {
        return;  /* Не спали */
    }

    /* CAN трансивер активен */
#ifdef FDCAN1_STBY_Pin
    HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_RESET);
#endif

    /* Перезапускаем FDCAN */
    if (g_can) {
        HAL_FDCAN_Start(g_can);
        HAL_FDCAN_ActivateNotification(g_can,
            FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
    }

    /* Включаем питание ленты */
    ws_power_set(1);

    g_is_sleeping = 0;
    g_sleep_requested = 0;
    g_last_can_activity_ms = HAL_GetTick();
}

#endif /* AMB_ENABLE_SLEEP_MODE */
