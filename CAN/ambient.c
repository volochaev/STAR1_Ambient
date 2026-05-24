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
#include "ambient_config.h"
#include "ambient_role.h"
#include "ambient_persist.h"
#include "ambient_power.h"
#include "ambient_rx.h"
#include "ambient_tx.h"
#include "ambient_backend.h"
#include "ambient_state_store.h"
#include "flash_storage.h"
#include "scene_color_model.h"
#include <string.h>

/* Global state (volatile for ISR/main-loop concurrency). */
volatile can_state_t g_can_state = {
    .oem_color = 0,
    .oem_brightness = 1.0f,
    .night_mode = 0,
    .bank_id = 0
};

static FDCAN_HandleTypeDef *g_can = NULL;
static uint32_t g_board_unique_id = 0;  /* may be replaced by MCU UID later */
static volatile uint8_t g_modern_status_conflict_active = 0u;
static volatile uint32_t g_modern_status_conflict_count = 0u;

static void copy_can_state_atomic(can_state_t *out)
{
    if (!out) return;
    __disable_irq();
    memcpy(out, (const void *)&g_can_state, sizeof(can_state_t));
    __enable_irq();
}

static void copy_sync_state_atomic(uint8_t *oem_color, uint8_t *brightness_raw)
{
    if (!oem_color || !brightness_raw) return;
    __disable_irq();
    *oem_color = g_can_state.oem_color;
    *brightness_raw = (uint8_t)(g_can_state.oem_brightness * 5.0f + 0.5f);
    __enable_irq();
}

/* Initialization. */
void can_ambient_init(FDCAN_HandleTypeDef *hfdcan)
{
    g_can = hfdcan;
    can_role_init();
    ambient_backend_init();

    /* Load saved settings from Flash */
    if (flash_storage_load((can_state_t *)&g_can_state) == 0) {
        can_persist_state_note_flash_loaded((const can_state_t *)&g_can_state);
    }

    can_persist_state_init();

    /* Generate per-board runtime ID (can be replaced with MCU UID). */
    g_board_unique_id = HAL_GetTick() ^ (BOARD_TYPE << 8);

#if AMB_ENABLE_SLEEP_MODE
    can_power_init();
#endif

    /* Keep CAN transceiver in active mode at startup. */
#ifdef FDCAN1_STBY_Pin
    HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_RESET);
#endif

    FDCAN_FilterTypeDef f;

    /* All boards listen to discovery/sync/master packet family on 0x353.
     * Subtype is determined by data[0]. */
    f.IdType = FDCAN_STANDARD_ID;
    f.FilterIndex = 0;
    f.FilterType = FDCAN_FILTER_MASK;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1 = CAN_MASTER_ID;  /* 0x353 shared for discovery/sync/master */
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* OEM packet 0x325 (all boards listen). */
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

    /* Ignition KL15/KL15R source (must be accepted for start-gate priority) */
    f.FilterIndex = 7;
    f.FilterID1 = CAN_SAM_F_A1_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Door latch states for door-open start-gate and door-open scene */
    f.FilterIndex = 8;
    f.FilterID1 = CAN_DOOR_FL_A1_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 9;
    f.FilterID1 = CAN_DOOR_FR_A1_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 10;
    f.FilterID1 = CAN_DOOR_R_A1_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Light module packets for handle/puddle logic */
    f.FilterIndex = 11;
    f.FilterID1 = CAN_LM_A1_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 12;
    f.FilterID1 = CAN_LM_A4_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Central locking / interior light requests */
    f.FilterIndex = 13;
    f.FilterID1 = CAN_EIS_A2_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 14;
    f.FilterID1 = CAN_ILM_RQ_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Sun intensity and seat heating */
    f.FilterIndex = 15;
    f.FilterID1 = CAN_HVAC_A1_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 16;
    f.FilterID1 = CAN_SEAT_FRONT_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 17;
    f.FilterID1 = CAN_SEAT_REAR_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Day/night fallback and parking/reverse helpers */
    f.FilterIndex = 18;
    f.FilterID1 = CAN_TGW_A8_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 19;
    f.FilterID1 = CAN_PTS_A1_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 20;
    f.FilterID1 = CAN_RVC_A2_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 21;
    f.FilterID1 = CAN_CHASSIS_R2_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Modern ambient request/status */
    f.FilterIndex = 22;
    f.FilterID1 = CAN_HU_AMB_RQ_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    f.FilterIndex = 23;
    f.FilterID1 = CAN_BODY_AMB_STAT_ID;
    f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(g_can, &f);

    /* Extended ambient packet uses the same 0x353 ID (subtype in data[0]). */
    /* Filter is already configured above via CAN_MASTER_ID (0x353). */

    /* Production-safe global filter: reject unmatched frames.
     * Only explicitly configured IDs above are accepted into FIFO0. */
    HAL_FDCAN_ConfigGlobalFilter(g_can,
        FDCAN_REJECT,
        FDCAN_REJECT,
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
    uint32_t now_ms;

    if (!out) return;
    now_ms = HAL_GetTick();
    memset(out, 0, sizeof(*out));
    copy_can_state_atomic(&out->can_state);
    ambient_backend_on_can_state((const can_state_t *)&out->can_state, now_ms);
    ambient_state_store_get_snapshot(&out->ambient_state);
    out->motion_profile = can_rx_get_motion_profile();
    scene_color_model_from_ambient(&out->ambient_state,
                                   out->motion_profile,
                                   out->can_state.night_mode,
                                   &out->scene_colors);
    out->bsm = can_ambient_get_bsm_state();
    out->now_ms = now_ms;
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

uint8_t can_ambient_handle_request_active(void)
{
#if AMB_HANDLE_SOURCE_POLICY == AMB_HANDLE_SOURCE_NS_ILL_ACTV
    return can_rx_is_ns_ill_active();
#elif AMB_HANDLE_SOURCE_POLICY == AMB_HANDLE_SOURCE_IL_R_ON_RQ
    return can_rx_is_ilm_rear_on_request();
#elif AMB_HANDLE_SOURCE_POLICY == AMB_HANDLE_SOURCE_PDDLLMP_ANY
#if AMB_PDDLLMP_IGNORE_REVERSE_FOR_DEBUG
    return can_rx_is_pddllmp_any_on_request();
#else
    return (uint8_t)(can_rx_is_pddllmp_any_on_request() && !can_rx_is_reverse_handle_block_active());
#endif
#else
    return can_rx_nsi_active();
#endif
}

uint8_t can_ambient_is_ignition_on(void)
{
    return can_rx_is_ignition_on();
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

uint8_t can_ambient_consume_unlock_event(void)
{
    return can_rx_consume_unlock_event();
}

uint8_t can_ambient_consume_lock_event(void)
{
    return can_rx_consume_lock_event();
}

void can_ambient_clear_lock_event_pending(void)
{
    can_rx_clear_lock_event_pending();
}

uint8_t can_ambient_is_lock_source_recent(void)
{
    return can_rx_is_lock_source_recent();
}

uint8_t can_ambient_consume_door_open_event_for_board(void)
{
    if (can_role_is_master()) {
        return can_rx_consume_door_open_event((uint8_t)BOARD_TYPE_ANY);
    }
    return can_rx_consume_door_open_event((uint8_t)BOARD_TYPE);
}

uint8_t can_ambient_is_any_door_open_for_board(void)
{
    if (can_role_is_master()) {
        return can_rx_is_any_door_open_for_board((uint8_t)BOARD_TYPE_ANY);
    }
    return can_rx_is_any_door_open_for_board((uint8_t)BOARD_TYPE);
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

float can_ambient_get_ilm_dim_scale_for_board(void)
{
    return can_rx_get_ilm_dim_scale((uint8_t)BOARD_TYPE);
}

float can_ambient_get_sun_gain_for_board(void)
{
    return can_rx_get_sun_gain_scale((uint8_t)BOARD_TYPE);
}

can_parking_warn_state_t can_ambient_get_parking_warn_state_for_board(void)
{
    return can_rx_get_parking_warn_state((uint8_t)BOARD_TYPE);
}

uint8_t can_ambient_is_reverse_active(void)
{
    return can_rx_is_reverse_active();
}

void can_ambient_update_role(uint32_t now_ms)
{
    can_role_update(now_ms);
    can_persist_state_process_deferred_save(&g_can_state, can_ambient_is_master(), now_ms);
}

uint32_t can_ambient_get_last_master_heartbeat_ms(void)
{
    return can_role_get_last_master_heartbeat_ms();
}

void can_ambient_get_modern_diag(can_modern_diag_t *out)
{
    uint8_t bus_valid = 0u;
    uint8_t bus_color = 0u;
    uint8_t bus_brightness = 0u;
    uint8_t bus_effect = 0u;
    uint32_t bus_timestamp_ms = 0u;
    can_ingress_diag_t ingress_diag;

    if (!out) {
        return;
    }

    bus_valid = can_rx_get_modern_bus_status(&bus_color, &bus_brightness, &bus_effect, &bus_timestamp_ms);
    can_rx_get_ingress_diag(&ingress_diag);

    __disable_irq();
    out->conflict_active = g_modern_status_conflict_active;
    out->conflict_count = g_modern_status_conflict_count;
    __enable_irq();

    out->last_bus_status_valid = bus_valid;
    out->last_bus_color = bus_color;
    out->last_bus_brightness = bus_brightness;
    out->last_bus_effect = bus_effect;
    out->last_bus_timestamp_ms = bus_timestamp_ms;
    out->rq_invalid_len_count = ingress_diag.rq_invalid_len_count;
    out->rq_invalid_range_count = ingress_diag.rq_invalid_range_count;
    out->stat_invalid_len_count = ingress_diag.stat_invalid_len_count;
    out->stat_invalid_range_count = ingress_diag.stat_invalid_range_count;
    out->oem_invalid_range_count = ingress_diag.oem_invalid_range_count;
    out->oem_clamped_brightness_count = ingress_diag.oem_clamped_brightness_count;
}

void can_ambient_reset_modern_diag(void)
{
    __disable_irq();
    g_modern_status_conflict_active = 0u;
    g_modern_status_conflict_count = 0u;
    __enable_irq();
    can_rx_reset_ingress_diag();
}

void can_ambient_get_ingress_diag(can_ingress_diag_t *out)
{
    can_rx_get_ingress_diag(out);
}

void can_ambient_reset_ingress_diag(void)
{
    can_rx_reset_ingress_diag();
}

uint32_t can_ambient_get_ingress_counter(can_ingress_counter_id_t counter_id)
{
    can_ingress_diag_t d;
    can_rx_get_ingress_diag(&d);

    switch (counter_id) {
    case CAN_INGRESS_COUNTER_RQ_INVALID_LEN:
        return d.rq_invalid_len_count;
    case CAN_INGRESS_COUNTER_RQ_INVALID_RANGE:
        return d.rq_invalid_range_count;
    case CAN_INGRESS_COUNTER_STAT_INVALID_LEN:
        return d.stat_invalid_len_count;
    case CAN_INGRESS_COUNTER_STAT_INVALID_RANGE:
        return d.stat_invalid_range_count;
    case CAN_INGRESS_COUNTER_OEM_INVALID_RANGE:
        return d.oem_invalid_range_count;
    case CAN_INGRESS_COUNTER_OEM_CLAMPED_BRIGHTNESS:
        return d.oem_clamped_brightness_count;
    default:
        return 0u;
    }
}

void can_ambient_send_master_packet(void)
{
    can_state_t state;

    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    copy_can_state_atomic(&state);
    can_tx_send_master(g_can,
                       &state,
                       can_rx_get_motion_profile(),
                       can_rx_get_oem_diag_b3(),
                       can_rx_get_oem_diag_flags());
}

void can_ambient_send_sync_packet(void)
{
    uint8_t oem_color, brightness_raw;

    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    copy_sync_state_atomic(&oem_color, &brightness_raw);
    can_tx_send_sync(g_can, oem_color, brightness_raw);
}

void can_ambient_send_discovery_packet(void)
{
    if (!g_can) {
        return;
    }
    can_tx_send_discovery(g_can, g_board_unique_id, can_ambient_is_master());
}

void can_ambient_send_modern_status_packet(void)
{
    ambient_state_snapshot_t st;
    uint8_t color = 0u;
    uint8_t brightness = 0u;
    uint8_t effect = 0u;
    uint8_t bus_color = 0u, bus_brightness = 0u, bus_effect = 0u;
    uint32_t bus_ts = 0u;
    uint32_t now = HAL_GetTick();

    if (!g_can || !can_ambient_is_master()) {
        return;
    }

    ambient_state_store_get_snapshot(&st);
    if (st.valid) {
        color = st.color_id & 0x0Fu;
        brightness = st.brightness_raw & 0x0Fu;
        effect = st.effect_id & 0x03u;
    } else {
        color = (uint8_t)(g_can_state.oem_color & 0x0Fu);
        brightness = (uint8_t)(g_can_state.oem_brightness * 5.0f + 0.5f);
        if (brightness > 5u) brightness = 5u;
        effect = 0u;
    }

    /* Dual-master conflict heuristic:
     * if another 0x12B source was seen recently and disagrees with our status payload,
     * latch conflict flag for diagnostics. */
    if (can_rx_get_modern_bus_status(&bus_color, &bus_brightness, &bus_effect, &bus_ts) &&
        bus_ts != 0u && (now - bus_ts) <= 300u) {
        if (bus_color != color || bus_brightness != brightness || bus_effect != effect) {
            g_modern_status_conflict_active = 1u;
            g_modern_status_conflict_count++;
        } else {
            g_modern_status_conflict_active = 0u;
        }
    }

    can_tx_send_body_ambient_status(g_can, color, brightness, effect);
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

void can_ambient_request_sleep_lock(void)
{
    can_power_request_sleep_lock();
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

void can_ambient_note_stop_rtc_wakeup_cycle(void)
{
    can_power_note_stop_rtc_wakeup_cycle();
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
