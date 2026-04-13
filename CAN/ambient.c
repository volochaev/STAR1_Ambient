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
 * Uses climate-aware CAN IDs:
 * - 0x025: Light sensor status (day/night)
 * - 0x20B/0x0BC: HVAC temperature status (front/rear)
 * - 0x339: HVAC fan current/status
 * - 0x325: OEM packets from vehicle (brightness, color)
 * - 0x353: Unified Master Protocol (discovery, sync, master, extended)
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#include "ambient.h"
#include "ambient_internal.h"
#include "palette.h"
#include "themes.h"
#include "base_scene.h"
#include "features.h"
#include "ambient_role.h"
#include "ambient_theme.h"
#include "ambient_power.h"
#include "ambient_rx.h"
#include "ambient_tx.h"
#include "flash_storage.h"
#include <string.h>

/* ========== GLOBAL STATE (volatile для thread-safety) ========== */
volatile can_state_t g_can_state = {
    .oem_color = 0,
    .oem_brightness = 1.0f,
    .night_mode = 0,
    .bank_id = 0,
    .theme_index = 0,
    .last_oem_color = 0xFF,              /* Invalid - первый вызов pick_oem_theme даст 1st theme */
    .oem_theme_indices = {0xFF, 0xFF, 0xFF}  /* 0xFF + 1 = 0 при первом визите */
};

static FDCAN_HandleTypeDef *g_can = NULL;
static uint32_t g_board_unique_id = 0;  /* можно использовать UID чипа */

static void copy_can_state_atomic(can_state_t *out)
{
    if (!out) return;
    __disable_irq();
    memcpy(out, (const void *)&g_can_state, sizeof(can_state_t));
    __enable_irq();
}

static void copy_bank_theme_atomic(uint8_t *bank_id, uint8_t *theme_index)
{
    if (!bank_id || !theme_index) return;
    __disable_irq();
    *bank_id = g_can_state.bank_id;
    *theme_index = g_can_state.theme_index;
    __enable_irq();
}

/* ========== INIT ========== */
void can_ambient_init(FDCAN_HandleTypeDef *hfdcan)
{
    g_can = hfdcan;
    can_role_init();

    /* Load saved settings from Flash */
    if (flash_storage_load((can_state_t *)&g_can_state) == 0) {
        can_theme_state_note_flash_loaded((const can_state_t *)&g_can_state);
    }

    can_theme_state_init();

    /* Генерируем уникальный ID для этой платы (можно использовать UID чипа) */
    g_board_unique_id = HAL_GetTick() ^ (BOARD_TYPE << 8);

#if AMB_ENABLE_SLEEP_MODE
    can_power_init();
#endif

    /* На старте держим CAN трансивер в active mode */
#ifdef FDCAN1_STBY_Pin
    HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_RESET);
#endif

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
    f.FilterIndex = 1;
    f.FilterID1 = CAN_OEM_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* BSM packet 0x17E (blind spot interrupt overlay) */
    f.FilterIndex = 2;
    f.FilterID1 = CAN_BSM_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Light sensor packet 0x025 (night/day source for comfort dimming) */
    f.FilterIndex = 3;
    f.FilterID1 = CAN_LGTSENS_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* HVAC temp packets 0x20B/0x0BC (temp up/down event trigger source) */
    f.FilterIndex = 4;
    f.FilterID1 = CAN_HVAC_FRONT_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 5;
    f.FilterID1 = CAN_HVAC_REAR_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 6;
    f.FilterID1 = CAN_HVAC_FAN_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Extended ambient packet теперь использует тот же ID 0x353, различается по типу */
    /* Фильтр уже настроен выше для CAN_MASTER_ID (0x353) */

    /* Bench-safe global filter: accept any unmatched standard/extended frames to FIFO0.
     * This prevents silent loss if Cube filter count/config drifts. */
    HAL_FDCAN_ConfigGlobalFilter(g_can,
        FDCAN_ACCEPT_IN_RX_FIFO0,
        FDCAN_ACCEPT_IN_RX_FIFO0,
        FDCAN_REJECT_REMOTE,
        FDCAN_REJECT_REMOTE);

    HAL_FDCAN_ActivateNotification(g_can,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
        0);

    HAL_FDCAN_Start(g_can);
}

void can_ambient_process_rx(uint32_t id, uint8_t *data, uint8_t len)
{
    can_rx_process(id, data, len);
}

void can_ambient_fill_runtime_inputs(runtime_inputs_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    copy_can_state_atomic(&out->can_state);
    out->resolved_theme = can_theme_state_resolve(&g_can_state, HAL_GetTick());
    out->motion_profile = can_rx_get_motion_profile();
    out->bsm = can_ambient_get_bsm_state();
    out->now_ms = HAL_GetTick();
}

/* ========== MASTER MODE FUNCTIONS ========== */

uint8_t can_ambient_is_master(void)
{
    return can_role_is_master();
}

uint8_t can_ambient_oem_received(void)
{
    return can_rx_oem_received();
}

void can_ambient_reset_oem_received(void)
{
    can_rx_reset_oem_received();
    can_role_reset();
}

uint8_t can_ambient_nsi_active(void)
{
    return can_rx_nsi_active();
}

can_bsm_state_t can_ambient_get_bsm_state(void)
{
    return can_rx_get_bsm_state();
}

motion_profile_t can_ambient_get_motion_profile(void)
{
    return can_rx_get_motion_profile();
}

void can_ambient_set_motion_profile(motion_profile_t profile)
{
    can_rx_set_motion_profile(profile);
}

int8_t can_ambient_consume_hvac_temp_trend(void)
{
    return can_rx_consume_hvac_temp_trend();
}

int8_t can_ambient_consume_hvac_temp_trend_for_board(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL) || (BOARD_TYPE == BOARD_TYPE_RL)
    return can_rx_consume_hvac_temp_trend_side(0u);
#elif (BOARD_TYPE == BOARD_TYPE_FR) || (BOARD_TYPE == BOARD_TYPE_RR)
    return can_rx_consume_hvac_temp_trend_side(1u);
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    return can_rx_consume_hvac_temp_trend_combined();
#else
    return can_rx_consume_hvac_temp_trend();
#endif
}

float can_ambient_get_hvac_split_bias_for_board(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL) || (BOARD_TYPE == BOARD_TYPE_RL)
    return can_rx_get_hvac_split_bias_side(0u);
#elif (BOARD_TYPE == BOARD_TYPE_FR) || (BOARD_TYPE == BOARD_TYPE_RR)
    return can_rx_get_hvac_split_bias_side(1u);
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    return can_rx_get_hvac_split_bias_combined();
#else
    return 0.0f;
#endif
}

float can_ambient_get_hvac_fan_level(void)
{
    return can_rx_get_hvac_fan_level();
}

uint8_t can_ambient_is_night_mode(void)
{
    uint8_t night;

    __disable_irq();
    night = g_can_state.night_mode;
    __enable_irq();
    return night;
}

float can_ambient_get_seat_heat_level_for_board(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL) || (BOARD_TYPE == BOARD_TYPE_RL)
    return can_rx_get_seat_heat_level_side(0u);
#elif (BOARD_TYPE == BOARD_TYPE_FR) || (BOARD_TYPE == BOARD_TYPE_RR)
    return can_rx_get_seat_heat_level_side(1u);
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    return can_rx_get_seat_heat_level_combined();
#else
    return 0.0f;
#endif
}

float can_ambient_get_bank_character_speed_scale(void)
{
    return can_rx_get_bank_character_speed_scale();
}

void can_ambient_update_role(uint32_t now_ms)
{
    can_role_update(now_ms);
    can_theme_state_process_deferred_save(&g_can_state, can_ambient_is_master(), now_ms);
}

uint32_t can_ambient_get_last_master_heartbeat_ms(void)
{
    return can_role_get_last_master_heartbeat_ms();
}

void can_ambient_send_master_packet(void)
{
    can_state_t state;

    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    copy_can_state_atomic(&state);
    can_tx_send_master(g_can, &state, can_rx_get_motion_profile());
}

void can_ambient_send_sync_packet(void)
{
    uint8_t bank_id, theme_index;

    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    copy_bank_theme_atomic(&bank_id, &theme_index);
    can_tx_send_sync(g_can, bank_id, theme_index);
}

void can_ambient_send_discovery_packet(void)
{
    if (!g_can) {
        return;
    }
    can_tx_send_discovery(g_can, g_board_unique_id, can_ambient_is_master());
}

void can_ambient_send_test_packet(uint32_t id, const uint8_t *data, uint8_t len)
{
    if (!g_can) {
        return;
    }
    can_tx_send_test(g_can, id, data, len);
}

/* ========== SLEEP MODE FUNCTIONS ========== */

#if AMB_ENABLE_SLEEP_MODE

uint8_t can_ambient_is_sleeping(void)
{
    return can_power_is_sleeping();
}

uint8_t can_ambient_should_sleep(void)
{
    return can_power_should_sleep();
}

void can_ambient_clear_sleep_request(void)
{
    can_power_clear_sleep_request();
}

void can_ambient_mark_awake(void)
{
    can_power_mark_awake();
}

uint32_t can_ambient_get_idle_time_ms(void)
{
    return can_power_get_idle_time_ms();
}

void can_ambient_check_sleep_timeout(void)
{
    can_power_check_sleep_timeout();
}

void can_ambient_enter_sleep(void)
{
    can_power_enter_sleep(g_can);
}

void can_ambient_exit_sleep(void)
{
    can_power_exit_sleep(g_can);
}

void can_ambient_note_stop_wakeup(uint8_t wake_src)
{
    can_power_note_stop_wakeup(wake_src);
}

void can_ambient_get_power_diag(can_power_diag_t *out)
{
    can_power_get_diag(out);
}

void can_ambient_reset_power_diag(void)
{
    can_power_reset_diag();
}

#endif /* AMB_ENABLE_SLEEP_MODE */
