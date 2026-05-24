/**
 * @file ambient_rx.c
 * @brief CAN RX parser and vehicle-signal extraction for ambient runtime.
 */
#include "ambient_rx.h"
#include "ambient_internal.h"

#include <string.h>

#include "ambient_power.h"
#include "ambient_role.h"
#include "ambient_persist.h"
#include "ambient_config.h"
#include <math.h>

static volatile uint8_t g_oem_packet_received = 0u;
static volatile uint32_t g_oem_last_rx_ms = 0u;
static volatile uint8_t g_nsi_active = 0u;
static volatile uint32_t g_nsi_last_rx_ms = 0u;
static volatile uint32_t g_surrill_last_on_ms = 0u;
static volatile uint32_t g_surrill_off_candidate_ms = 0u;
static volatile uint8_t g_unlock_event_pending = 0u;
static volatile uint8_t g_unlock_event_prev_active = 0u;
static volatile uint32_t g_unlock_event_last_ms = 0u;
static volatile uint8_t g_lock_event_pending = 0u;
static volatile uint8_t g_lock_event_prev_active = 0u;
static volatile uint32_t g_lock_event_last_ms = 0u;
static volatile uint32_t g_eis_last_rx_ms = 0u;
static volatile uint8_t g_turnlmp_unlk_prev_active = 0u;
static volatile uint8_t g_turnlmp_lk_prev_active = 0u;
static volatile uint32_t g_turnlmp_unlk_last_ms = 0u;
static volatile uint32_t g_turnlmp_lk_last_ms = 0u;
static volatile uint8_t g_door_open_event_mask = 0u; /* bit0=FL bit1=FR bit2=RL bit3=RR */
static volatile uint8_t g_door_latch_valid_mask = 0u;
static volatile uint8_t g_door_latch_open_mask = 0u;
static volatile uint32_t g_door_open_last_ms = 0u;
static volatile uint32_t g_lgtsens_last_rx_ms = 0u;
static volatile uint8_t g_hu_daynight_valid = 0u;
static volatile uint8_t g_hu_daynight_night = 0u;
static volatile uint32_t g_hu_daynight_last_rx_ms = 0u;
volatile uint8_t g_dbg_lgtsens_b0 = 0u;
volatile uint8_t g_dbg_lgtsens_b1 = 0u;
volatile uint8_t g_dbg_lgtsens_night1 = 0u;
volatile uint8_t g_dbg_lgtsens_night2 = 0u;
volatile uint8_t g_dbg_hu_daynight_raw = 0u;
volatile uint8_t g_dbg_night_source = 0u; /* 1=LGTSENS, 2=HU/TGW, 3=NSI */
volatile uint8_t g_dbg_ilm_dim_front = 0u;
volatile uint8_t g_dbg_ilm_dim_rear = 0u;
volatile uint8_t g_dbg_ilm_dim_bg = 0u;
volatile float g_dbg_ilm_dim_scale = 1.0f;
static volatile uint8_t g_ilm_dim_front = 0u;
static volatile uint8_t g_ilm_dim_rear = 0u;
static volatile uint8_t g_ilm_dim_bg = 0u;
static volatile uint8_t g_ilm_front_on = 0u;
static volatile uint8_t g_ilm_rear_on = 0u;
static volatile uint32_t g_ilm_last_rx_ms = 0u;
static volatile uint8_t g_ignition_on = 0u;
static volatile uint32_t g_ignition_last_rx_ms = 0u;
volatile uint8_t g_dbg_samf_a1_b0 = 0u;
volatile uint8_t g_dbg_samf_a1_b1 = 0u;
static volatile uint8_t g_ns_ill_active = 0u;
static volatile uint32_t g_lm_a1_last_rx_ms = 0u;
static volatile uint8_t g_pddllmp_any_on_request = 0u;
static volatile uint32_t g_lm_a4_last_rx_ms = 0u;
static volatile uint32_t g_pddllmp_last_on_ms = 0u;
static volatile uint32_t g_pddllmp_off_candidate_ms = 0u;
volatile uint8_t g_dbg_lm_a4_b5 = 0u;
static volatile uint8_t g_parking_left_active = 0u;
static volatile uint8_t g_parking_right_active = 0u;
static volatile uint8_t g_parking_rear_active = 0u;
static volatile float g_parking_left_level = 0.0f;
static volatile float g_parking_right_level = 0.0f;
static volatile float g_parking_rear_level = 0.0f;
static volatile uint32_t g_parking_last_rx_ms = 0u;
static volatile uint8_t g_reverse_active = 0u;
static volatile uint32_t g_reverse_last_rx_ms = 0u;
static volatile uint32_t g_reverse_last_active_ms = 0u;
static volatile uint8_t g_reverse_samf_active = 0u;
static volatile uint32_t g_reverse_samf_last_ms = 0u;
static volatile uint8_t g_sun_valid = 0u;
static volatile uint8_t g_sun_fl = 0u;
static volatile uint8_t g_sun_fr = 0u;
static volatile uint32_t g_sun_last_rx_ms = 0u;
volatile uint8_t g_dbg_sun_valid = 0u;
volatile uint8_t g_dbg_sun_fl = 0u;
volatile uint8_t g_dbg_sun_fr = 0u;
volatile float g_dbg_sun_gain_scale = 1.0f;
static volatile uint8_t g_hvac_prev_front_valid = 0u;
static volatile uint8_t g_hvac_prev_rear_valid = 0u;
static volatile uint8_t g_hvac_prev_fl = 0u;
static volatile uint8_t g_hvac_prev_fr = 0u;
static volatile uint8_t g_hvac_prev_rl = 0u;
static volatile uint8_t g_hvac_prev_rr = 0u;
static volatile uint32_t g_hvac_last_event_left_ms = 0u;
static volatile uint32_t g_hvac_last_event_right_ms = 0u;
static volatile uint32_t g_hvac_last_trend_left_ms = 0u;
static volatile uint32_t g_hvac_last_trend_right_ms = 0u;
static volatile int8_t g_hvac_trend_left_pending = 0;
static volatile int8_t g_hvac_trend_right_pending = 0;
static volatile int8_t g_hvac_last_trend_left = 0;
static volatile int8_t g_hvac_last_trend_right = 0;
static volatile float g_hvac_split_bias = 0.0f; /* -1..1 (left minus right) */
static volatile float g_hvac_fan_level = 0.0f;  /* 0..1 */
static volatile uint8_t g_seat_front_valid = 0u;
static volatile uint8_t g_seat_rear_valid = 0u;
static volatile uint8_t g_seat_heat_fl = 0u;
static volatile uint8_t g_seat_heat_fr = 0u;
static volatile uint8_t g_seat_heat_rl = 0u;
static volatile uint8_t g_seat_heat_rr = 0u;
static volatile can_bsm_state_t g_bsm = {0, 0, 0.0f, 0.0f};
static volatile uint32_t g_bsm_last_rx_ms = 0u;
static volatile motion_profile_t g_motion_profile = (motion_profile_t)AMB_DEFAULT_MOTION_PROFILE;
static volatile float g_bank_character_speed[OEM_COLOR_MAX] = {1.0f, 1.0f, 1.0f};
static volatile uint8_t g_bank_character_valid[OEM_COLOR_MAX] = {0u, 0u, 0u};
static volatile uint8_t g_oem_color_src_locked = 0u; /* 0=auto, 1=bits3:2, 2=bits5:4 */
static volatile uint8_t g_oem_col_prev_32 = 0xFFu;
static volatile uint8_t g_oem_col_prev_54 = 0xFFu;
static volatile uint8_t g_oem_col_score_32 = 0u;
static volatile uint8_t g_oem_col_score_54 = 0u;
static volatile uint8_t g_oem_col_obs_count = 0u;
static volatile uint8_t g_oem_diag_b3 = 0u;
static volatile uint8_t g_oem_diag_flags = 0u;
static volatile uint8_t g_modern_rq_color = 0u;
static volatile uint8_t g_modern_rq_brightness = 0u;
static volatile uint8_t g_modern_rq_effect = 0u;
static volatile uint8_t g_modern_rq_valid = 0u;
static volatile uint32_t g_modern_rq_last_rx_ms = 0u;
static volatile uint8_t g_modern_stat_color = 0u;
static volatile uint8_t g_modern_stat_brightness = 0u;
static volatile uint8_t g_modern_stat_effect = 0u;
static volatile uint8_t g_modern_stat_valid = 0u;
static volatile uint32_t g_modern_stat_last_rx_ms = 0u;
static volatile can_ingress_diag_t g_ingress_diag = {0};

/* Wrap-safe elapsed milliseconds helper. */
static uint32_t elapsed_ms32(uint32_t now, uint32_t then)
{
    return (now >= then) ? (now - then) : ((UINT32_MAX - then) + now + 1u);
}

/* Allow only lock sources that represent explicit user unlock actions. */
static uint8_t clks_source_unlock_allowed(uint8_t src)
{
    /* DBC EIS_A2 CLkS_Src: 1=IR, 2=KG. Other sources are filtered out. */
    return (uint8_t)((src == 1u || src == 2u) ? 1u : 0u);
}

/* Decode door latch status to binary open/closed state. */
static uint8_t door_latch_is_open(uint8_t stat)
{
    /* DBC DrRLtch_*_Stat: 1=CLS, 2=OPN. */
    return (uint8_t)(stat == 2u ? 1u : 0u);
}

/* Check freshness of legacy OEM ambient frame stream. */
static uint8_t local_oem_fresh(uint32_t now)
{
    return (uint8_t)(g_oem_last_rx_ms != 0u &&
                     elapsed_ms32(now, g_oem_last_rx_ms) <= AMB_CAN_ACTIVE_TIMEOUT_MS);
}

static void update_night_mode_fallback(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t use_lgtsens;
    uint8_t use_hu;
    uint8_t night = 0u;

    use_lgtsens = (uint8_t)(g_lgtsens_last_rx_ms != 0u &&
                            elapsed_ms32(now, g_lgtsens_last_rx_ms) <= AMB_CAN_ACTIVE_TIMEOUT_MS);
#if !AMB_ENABLE_LGTSENS_NIGHT_SOURCE
    use_lgtsens = 0u;
#endif
    use_hu = (uint8_t)(g_hu_daynight_valid &&
                       g_hu_daynight_last_rx_ms != 0u &&
                       elapsed_ms32(now, g_hu_daynight_last_rx_ms) <= AMB_HU_DAYNIGHT_STALE_MS);
    if (use_lgtsens) {
        g_dbg_night_source = 1u;
        return; /* Dedicated light sensor has priority and already updates g_can_state.night_mode. */
    }
    if (use_hu) {
        night = g_hu_daynight_night;
        g_dbg_night_source = 2u;
    } else {
        night = g_nsi_active;
        g_dbg_night_source = 3u;
    }
    __disable_irq();
    g_can_state.night_mode = night;
    __enable_irq();
}

static void bsm_apply_board_mask(can_bsm_state_t *s)
{
    if (!s) return;
#if (BOARD_TYPE == BOARD_TYPE_FL) || (BOARD_TYPE == BOARD_TYPE_RL)
    s->right_active = 0u;
    s->right_level = 0.0f;
#elif (BOARD_TYPE == BOARD_TYPE_FR) || (BOARD_TYPE == BOARD_TYPE_RR)
    s->left_active = 0u;
    s->left_level = 0.0f;
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    s->left_active = 0u;
    s->right_active = 0u;
    s->left_level = 0.0f;
    s->right_level = 0.0f;
#else
#endif
}

static void set_motion_profile(motion_profile_t profile)
{
    if (profile > MOTION_PROFILE_SPORT) return;
    g_motion_profile = profile;
}

static float bank_profile_speed_target(motion_profile_t profile)
{
    if (profile == MOTION_PROFILE_CALM) return 0.92f;
    if (profile == MOTION_PROFILE_SPORT) return 1.10f;
    return 1.00f;
}

static void bank_character_sample(void)
{
#if AMB_ENABLE_BANK_CHARACTER_MEMORY
    uint8_t bank;
    float target;
    float fan;
    float k;

    bank = g_can_state.oem_color;
    if (bank >= OEM_COLOR_MAX) return;

    target = bank_profile_speed_target(g_motion_profile);
    fan = g_hvac_fan_level;
    if (fan < 0.0f) fan = 0.0f;
    if (fan > 1.0f) fan = 1.0f;
    target += fan * 0.08f;
    if (g_can_state.night_mode) {
        target *= 0.96f;
    }
    if (target < AMB_BANK_CHARACTER_SPEED_MIN) target = AMB_BANK_CHARACTER_SPEED_MIN;
    if (target > AMB_BANK_CHARACTER_SPEED_MAX) target = AMB_BANK_CHARACTER_SPEED_MAX;

    k = AMB_BANK_CHARACTER_LERP_ALPHA;
    if (k < 0.0f) k = 0.0f;
    if (k > 0.98f) k = 0.98f;

    if (!g_bank_character_valid[bank]) {
        g_bank_character_speed[bank] = target;
        g_bank_character_valid[bank] = 1u;
    } else {
        g_bank_character_speed[bank] = g_bank_character_speed[bank] * k + target * (1.0f - k);
    }
#endif
}

static uint8_t hvac_temp_raw_valid(uint8_t raw)
{
    return (raw < 0xF0u) ? 1u : 0u;
}

static void hvac_side_note_delta(int16_t delta, uint8_t right_side)
{
    int8_t trend;
    uint32_t now;

    if (delta == 0) return;
    if ((delta > 0 && delta < (int16_t)AMB_HVAC_TEMP_EVENT_MIN_STEP_RAW) ||
        (delta < 0 && (-delta) < (int16_t)AMB_HVAC_TEMP_EVENT_MIN_STEP_RAW)) {
        return;
    }
    now = HAL_GetTick();
    trend = (delta > 0) ? 1 : -1;

    __disable_irq();
    if (right_side) {
        if (g_hvac_last_trend_right != 0 &&
            g_hvac_last_trend_right != trend &&
            elapsed_ms32(now, g_hvac_last_trend_right_ms) < AMB_HVAC_TEMP_DIR_HOLD_MS) {
            __enable_irq();
            return;
        }
        if (elapsed_ms32(now, g_hvac_last_event_right_ms) >= AMB_HVAC_TEMP_EVENT_COOLDOWN_MS) {
            g_hvac_trend_right_pending = trend;
            g_hvac_last_event_right_ms = now;
            g_hvac_last_trend_right = trend;
            g_hvac_last_trend_right_ms = now;
        }
    } else {
        if (g_hvac_last_trend_left != 0 &&
            g_hvac_last_trend_left != trend &&
            elapsed_ms32(now, g_hvac_last_trend_left_ms) < AMB_HVAC_TEMP_DIR_HOLD_MS) {
            __enable_irq();
            return;
        }
        if (elapsed_ms32(now, g_hvac_last_event_left_ms) >= AMB_HVAC_TEMP_EVENT_COOLDOWN_MS) {
            g_hvac_trend_left_pending = trend;
            g_hvac_last_event_left_ms = now;
            g_hvac_last_trend_left = trend;
            g_hvac_last_trend_left_ms = now;
        }
    }
    __enable_irq();
}

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static uint8_t clamp_u8_count(uint8_t value, uint8_t max_v, volatile uint32_t *counter)
{
    if (value > max_v) {
        if (counter) {
            (*counter)++;
        }
        return max_v;
    }
    return value;
}

static void handle_modern_ambient_request(const uint8_t *d, uint8_t len)
{
    uint8_t brightness_rq;
    uint8_t color_rq;
    uint8_t effect_rq;
    uint32_t now;

    if (!d || len < 6u) {
        g_ingress_diag.rq_invalid_len_count++;
        return;
    }

    /* Request frame mapping (docs/can_ambient_ids_spec.md):
     * brightness = data[2] low nibble, color = data[3] low nibble, effect = data[5] low 2 bits. */
    brightness_rq = d[2] & 0x0Fu;
    color_rq = d[3] & 0x0Fu;
    effect_rq = d[5] & 0x03u;
    if (brightness_rq > 5u || color_rq > 12u) {
        g_ingress_diag.rq_invalid_range_count++;
    }
    brightness_rq = clamp_u8_count(brightness_rq, 5u, NULL);
    color_rq = clamp_u8_count(color_rq, 12u, NULL);
    now = HAL_GetTick();

    __disable_irq();
    g_modern_rq_brightness = brightness_rq;
    g_modern_rq_color = color_rq;
    g_modern_rq_effect = effect_rq;
    g_modern_rq_last_rx_ms = now;
    g_modern_rq_valid = 1u;
    __enable_irq();
}

static void handle_modern_ambient_status(const uint8_t *d, uint8_t len)
{
    uint8_t brightness_stat;
    uint8_t color_stat;
    uint8_t effect_stat;
    uint32_t now;

    if (!d || len < 8u) {
        g_ingress_diag.stat_invalid_len_count++;
        return;
    }

    brightness_stat = d[4] & 0x0Fu;
    color_stat = (d[4] >> 4) & 0x0Fu;
    effect_stat = d[7] & 0x03u;
    if (brightness_stat > 5u || color_stat > 12u) {
        g_ingress_diag.stat_invalid_range_count++;
    }
    brightness_stat = clamp_u8_count(brightness_stat, 5u, NULL);
    color_stat = clamp_u8_count(color_stat, 12u, NULL);
    now = HAL_GetTick();

    __disable_irq();
    g_modern_stat_brightness = brightness_stat;
    g_modern_stat_color = color_stat;
    g_modern_stat_effect = effect_stat;
    g_modern_stat_last_rx_ms = now;
    g_modern_stat_valid = 1u;
    __enable_irq();
}

static void hvac_recompute_split_bias(void)
{
    uint8_t has_left = 0u;
    uint8_t has_right = 0u;
    uint16_t left_sum = 0u;
    uint16_t right_sum = 0u;
    uint8_t left_cnt = 0u;
    uint8_t right_cnt = 0u;
    float left_avg;
    float right_avg;
    float delta;
    float mag;
    float bias = 0.0f;

    if (g_hvac_prev_front_valid) {
        left_sum += g_hvac_prev_fl;
        right_sum += g_hvac_prev_fr;
        left_cnt++;
        right_cnt++;
        has_left = 1u;
        has_right = 1u;
    }
    if (g_hvac_prev_rear_valid) {
        left_sum += g_hvac_prev_rl;
        right_sum += g_hvac_prev_rr;
        left_cnt++;
        right_cnt++;
        has_left = 1u;
        has_right = 1u;
    }
    if (!has_left || !has_right || left_cnt == 0u || right_cnt == 0u) {
        g_hvac_split_bias = 0.0f;
        return;
    }

    left_avg = (float)left_sum / (float)left_cnt;
    right_avg = (float)right_sum / (float)right_cnt;
    delta = left_avg - right_avg;
    if (delta > (float)AMB_HVAC_SPLIT_MIN_DELTA_RAW) {
        mag = (delta - (float)AMB_HVAC_SPLIT_MIN_DELTA_RAW) / (float)AMB_HVAC_SPLIT_FULL_DELTA_RAW;
        bias = clamp01f(mag);
    } else if (delta < -(float)AMB_HVAC_SPLIT_MIN_DELTA_RAW) {
        mag = ((-delta) - (float)AMB_HVAC_SPLIT_MIN_DELTA_RAW) / (float)AMB_HVAC_SPLIT_FULL_DELTA_RAW;
        bias = -clamp01f(mag);
    }
    g_hvac_split_bias = bias;
}

static void handle_oem_nsi(const uint8_t *d, uint8_t len)
{
    uint8_t surr_ill_rq;
    uint8_t nsi_active;
    uint32_t now;

    if (len < 4u) return;

    surr_ill_rq = d[3] & 0x03u;
    now = HAL_GetTick();
    g_nsi_last_rx_ms = now;
    /* Strict NSI policy: only explicit Surrounding Illumination request ON. */
    if (surr_ill_rq == 1u) {
        g_surrill_last_on_ms = now;
        g_surrill_off_candidate_ms = 0u;
        nsi_active = 1u;
    } else {
        if (g_surrill_off_candidate_ms == 0u) {
            g_surrill_off_candidate_ms = now;
        }
        if (g_surrill_last_on_ms != 0u &&
            elapsed_ms32(now, g_surrill_last_on_ms) < AMB_SURRILL_HANDLE_HOLD_MS) {
            /* Anti-flicker hold for sparse/jittery SurrIll frames. */
            nsi_active = 1u;
        } else if (elapsed_ms32(now, g_surrill_off_candidate_ms) < AMB_SURRILL_OFF_CONFIRM_MS) {
            /* OFF must be stable for a while before we actually disable handle. */
            nsi_active = 1u;
        } else {
            nsi_active = 0u;
        }
    }
    g_nsi_active = nsi_active;

    /* Fallback priority: LGTSENS -> HU day/night -> NSI request. */
    update_night_mode_fallback();
}

static void handle_lgtsens(const uint8_t *d, uint8_t len)
{
    uint8_t night_1;
    uint8_t night_2;
    uint8_t night;

    if (len < 2u) return;

    /* DBC BO_37 LGTSENS_A1:
     * - LgtSens_Night  : start bit 1  (byte0 bit1)
     * - LgtSens_Night2 : start bit 13 (byte1 bit5) */
    night_1 = (uint8_t)((d[0] >> 1) & 0x01u);
    night_2 = (uint8_t)((d[1] >> 5) & 0x01u);
    /* Vehicle-specific inversion:
     * observed day frame under sun: 07 00 (night bits logically "1"). */
    night = (uint8_t)(((night_1 || night_2) ? 0u : 1u));
    g_dbg_lgtsens_b0 = d[0];
    g_dbg_lgtsens_b1 = d[1];
    g_dbg_lgtsens_night1 = night_1;
    g_dbg_lgtsens_night2 = night_2;
    
    g_lgtsens_last_rx_ms = HAL_GetTick();
#if AMB_ENABLE_LGTSENS_NIGHT_SOURCE
    g_dbg_night_source = 1u;
    __disable_irq();
    g_can_state.night_mode = night;
    __enable_irq();
#else
    update_night_mode_fallback();
#endif
}

static void handle_eis_a2_lock_unlock(const uint8_t *d, uint8_t len)
{
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301 || AMB_ENABLE_LOCK_GOODBYE_EVENT || AMB_ENABLE_EIS_TURNLMP_TRIGGERS
    uint8_t ext_lk;
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
    uint8_t ext_unlk;
#endif
    uint8_t turn_lk;
    uint8_t turn_unlk;
#if AMB_ENABLE_EIS_DOOR_REQ_FALLBACK
    uint8_t fl_lk;
    uint8_t fl_unlk;
    uint8_t fr_lk;
    uint8_t fr_unlk;
    uint8_t rl_lk;
    uint8_t rl_unlk;
    uint8_t rr_lk;
    uint8_t rr_unlk;
#endif
    uint8_t any_lk;
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
    uint8_t any_unlk;
#endif
    uint8_t src;
    uint8_t src_allowed;
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
    uint8_t prev_unlk_active;
#endif
    uint8_t prev_lk_active;
    uint8_t prev_turn_unlk;
#if AMB_ENABLE_EIS_TURNLMP_LOCK_TRIGGER
    uint8_t prev_turn_lk;
#endif
    uint32_t now;

    if (!d || len < 4u) return;

    ext_lk = (uint8_t)((d[1] >> 1) & 0x01u);
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
    ext_unlk = (uint8_t)((d[1] >> 0) & 0x01u);
#endif
    turn_lk = (uint8_t)((d[1] >> 2) & 0x01u);
    turn_unlk = (uint8_t)((d[1] >> 3) & 0x01u);
    src = (uint8_t)((d[1] >> 4) & 0x0Fu);
#if AMB_ENABLE_EIS_DOOR_REQ_FALLBACK
    fl_lk = (uint8_t)((d[3] >> 6) & 0x01u);
    fl_unlk = (uint8_t)((d[3] >> 7) & 0x01u);
    fr_lk = (uint8_t)((d[3] >> 4) & 0x01u);
    fr_unlk = (uint8_t)((d[3] >> 5) & 0x01u);
    rl_lk = (uint8_t)((d[3] >> 2) & 0x01u);
    rl_unlk = (uint8_t)((d[3] >> 3) & 0x01u);
    rr_lk = (uint8_t)((d[3] >> 0) & 0x01u);
    rr_unlk = (uint8_t)((d[3] >> 1) & 0x01u);
#endif
#if AMB_ENABLE_EIS_DOOR_REQ_FALLBACK
    any_lk = (uint8_t)((ext_lk | fl_lk | fr_lk | rl_lk | rr_lk) ? 1u : 0u);
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
    any_unlk = (uint8_t)((ext_unlk | fl_unlk | fr_unlk | rl_unlk | rr_unlk) ? 1u : 0u);
#endif
#else
    any_lk = ext_lk;
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
    any_unlk = ext_unlk;
#endif
#endif
    src_allowed = clks_source_unlock_allowed(src);
    now = HAL_GetTick();

    __disable_irq();
#if AMB_ENABLE_UNLOCK_TRIGGER_EIS301
    prev_unlk_active = g_unlock_event_prev_active;
    g_unlock_event_prev_active = any_unlk;
    if (src_allowed && any_unlk && !prev_unlk_active &&
        (g_unlock_event_last_ms == 0u || elapsed_ms32(now, g_unlock_event_last_ms) >= AMB_UNLOCK_EIS_EVENT_DEBOUNCE_MS)) {
        g_unlock_event_pending = 1u;
        g_unlock_event_last_ms = now;
    }
#endif
#if AMB_ENABLE_LOCK_GOODBYE_EVENT
    prev_lk_active = g_lock_event_prev_active;
    g_lock_event_prev_active = any_lk;
    if (src_allowed && any_lk && !prev_lk_active &&
        (g_lock_event_last_ms == 0u || elapsed_ms32(now, g_lock_event_last_ms) >= AMB_LOCK_EIS_EVENT_DEBOUNCE_MS)) {
        g_lock_event_pending = 1u;
        g_lock_event_last_ms = now;
    }
#else
    (void)prev_lk_active;
#endif
#if AMB_ENABLE_EIS_TURNLMP_TRIGGERS
    prev_turn_unlk = g_turnlmp_unlk_prev_active;
    g_turnlmp_unlk_prev_active = turn_unlk;
#if AMB_ENABLE_EIS_TURNLMP_UNLOCK_TRIGGER
    if (src_allowed && turn_unlk && !prev_turn_unlk &&
        (g_turnlmp_unlk_last_ms == 0u ||
         elapsed_ms32(now, g_turnlmp_unlk_last_ms) >= AMB_EIS_TURNLMP_EVENT_DEBOUNCE_MS)) {
        g_unlock_event_pending = 1u;
        g_turnlmp_unlk_last_ms = now;
    }
#endif

#if AMB_ENABLE_EIS_TURNLMP_LOCK_TRIGGER
    prev_turn_lk = g_turnlmp_lk_prev_active;
#endif
    g_turnlmp_lk_prev_active = turn_lk;
#if AMB_ENABLE_EIS_TURNLMP_LOCK_TRIGGER
    if (src_allowed && turn_lk && !prev_turn_lk &&
        (g_turnlmp_lk_last_ms == 0u ||
         elapsed_ms32(now, g_turnlmp_lk_last_ms) >= AMB_EIS_TURNLMP_EVENT_DEBOUNCE_MS)) {
        g_lock_event_pending = 1u;
        g_turnlmp_lk_last_ms = now;
    }
#endif
#else
    (void)prev_turn_unlk;
    (void)prev_turn_lk;
#endif
    __enable_irq();
#else
    (void)d;
    (void)len;
#endif
}

static void handle_door_latch_state(uint8_t door_bit, uint8_t stat)
{
#if AMB_ENABLE_DOOR_OPEN_EVENT_SCENE || AMB_ENABLE_START_GATE_DOOR_OPEN || AMB_ENABLE_LOCK_GOODBYE_EVENT
    uint8_t valid_mask;
    uint8_t open_mask;
    uint8_t was_valid;
    uint8_t was_open;
    uint8_t now_open;
    uint8_t door_mask;
    uint32_t now;

    if (door_bit > 3u) return;
    if (stat >= 3u) return; /* 0=NDEF,1=CLS,2=OPN */

    door_mask = (uint8_t)(1u << door_bit);
    now_open = door_latch_is_open(stat);
    now = HAL_GetTick();

    __disable_irq();
    valid_mask = g_door_latch_valid_mask;
    open_mask = g_door_latch_open_mask;
    was_valid = (uint8_t)((valid_mask & door_mask) ? 1u : 0u);
    was_open = (uint8_t)((open_mask & door_mask) ? 1u : 0u);
    valid_mask |= door_mask;
    if (now_open) {
        open_mask |= door_mask;
    } else {
        open_mask &= (uint8_t)~door_mask;
    }
    g_door_latch_valid_mask = valid_mask;
    g_door_latch_open_mask = open_mask;

#if AMB_ENABLE_DOOR_OPEN_EVENT_SCENE
    if (((was_valid && !was_open && now_open) || (!was_valid && now_open)) &&
        (g_door_open_last_ms == 0u || elapsed_ms32(now, g_door_open_last_ms) >= AMB_DOOR_OPEN_EVENT_COOLDOWN_MS)) {
        g_door_open_event_mask |= door_mask;
        g_door_open_last_ms = now;
    }
#else
    (void)was_valid;
    (void)was_open;
    (void)now;
#endif
    __enable_irq();
#else
    (void)door_bit;
    (void)stat;
#endif
}

static void handle_door_fl_fr(const uint8_t *d, uint8_t len, uint8_t right)
{
    uint8_t stat;
    uint8_t stat_hi;
    uint8_t stat_lo;

    if (!d || len < 1u) return;
    g_dbg_samf_a1_b0 = d[0];
    g_dbg_samf_a1_b1 = (len > 1u) ? d[1] : 0u;
    /* Primary decode per DBC (DrRLtch_*_Stat : 6|2@1+).
     * Some real traces expose equivalent latch states in low bits (0|2),
     * while bits 6..7 stay 0. Keep bench compatibility by falling back only
     * when high bits are undefined (0) and low bits carry CLS/OPN semantics. */
    stat_hi = (uint8_t)((d[0] >> 6) & 0x03u);
    stat_lo = (uint8_t)(d[0] & 0x03u);
    stat = stat_hi;
    if (stat_hi == 0u && (stat_lo == 1u || stat_lo == 2u)) {
        stat = stat_lo;
    }
    handle_door_latch_state(right ? 1u : 0u, stat);
}

static void handle_door_rear(const uint8_t *d, uint8_t len)
{
    uint8_t rl_stat;
    uint8_t rr_stat;

    if (!d || len < 1u) return;
    rr_stat = (uint8_t)((d[0] >> 0) & 0x03u);
    rl_stat = (uint8_t)((d[0] >> 2) & 0x03u);
    handle_door_latch_state(2u, rl_stat);
    handle_door_latch_state(3u, rr_stat);
}

static void handle_hvac_temp(uint32_t id, const uint8_t *d, uint8_t len)
{
    uint8_t t0;
    uint8_t t1;

    if (len < 2u) return;

    t0 = d[0];
    t1 = d[1];
    if (!hvac_temp_raw_valid(t0) || !hvac_temp_raw_valid(t1)) return;

    if (id == CAN_HVAC_FRONT_ID) {
        if (g_hvac_prev_front_valid) {
            if (t0 != g_hvac_prev_fl) {
                hvac_side_note_delta((int16_t)t0 - (int16_t)g_hvac_prev_fl, 0u);
            }
            if (t1 != g_hvac_prev_fr) {
                hvac_side_note_delta((int16_t)t1 - (int16_t)g_hvac_prev_fr, 1u);
            }
        }
        __disable_irq();
        g_hvac_prev_fl = t0;
        g_hvac_prev_fr = t1;
        g_hvac_prev_front_valid = 1u;
        hvac_recompute_split_bias();
        __enable_irq();
        return;
    }

    if (id == CAN_HVAC_REAR_ID) {
        if (g_hvac_prev_rear_valid) {
            if (t0 != g_hvac_prev_rl) {
                hvac_side_note_delta((int16_t)t0 - (int16_t)g_hvac_prev_rl, 0u);
            }
            if (t1 != g_hvac_prev_rr) {
                hvac_side_note_delta((int16_t)t1 - (int16_t)g_hvac_prev_rr, 1u);
            }
        }
        __disable_irq();
        g_hvac_prev_rl = t0;
        g_hvac_prev_rr = t1;
        g_hvac_prev_rear_valid = 1u;
        hvac_recompute_split_bias();
        __enable_irq();
    }
}

static void handle_hvac_fan(const uint8_t *d, uint8_t len)
{
    float filtered;
    uint8_t raw;
    float level;

    if (len < 4u) return;
    raw = d[3];
    if (!hvac_temp_raw_valid(raw)) return;
    level = (float)raw / 255.0f;

    __disable_irq();
    filtered = g_hvac_fan_level;
    if (fabsf(level - filtered) < AMB_HVAC_FAN_JITTER_DEADBAND) {
        level = filtered;
    }
    g_hvac_fan_level = filtered * 0.82f + level * 0.18f;
    bank_character_sample();
    __enable_irq();
}

static void handle_ilm_rq(const uint8_t *d, uint8_t len)
{
#if AMB_ENABLE_ILM_DIM_REQUESTS
    uint8_t dim_bg;
    uint8_t dim_f;
    uint8_t dim_r;
    uint8_t on_f;
    uint8_t on_r;
    if (!d || len < 2u) return;

    dim_r = (uint8_t)((d[1] >> 0) & 0x01u);  /* IL_R_Dim_Rq 8|1 */
    on_r = (uint8_t)((d[1] >> 1) & 0x01u);   /* IL_R_On_Rq 9|1 */
    dim_f = (uint8_t)((d[1] >> 2) & 0x01u);  /* IL_F_Dim_Rq 10|1 */
    on_f = (uint8_t)((d[1] >> 3) & 0x01u);   /* IL_F_On_Rq 11|1 */
    dim_bg = (uint8_t)((d[1] >> 7) & 0x01u); /* BgCLgt_Dim_Rq 15|1 */

    __disable_irq();
    g_ilm_dim_front = dim_f;
    g_ilm_dim_rear = dim_r;
    g_ilm_dim_bg = dim_bg;
    g_ilm_front_on = on_f;
    g_ilm_rear_on = on_r;
    g_ilm_last_rx_ms = HAL_GetTick();
    __enable_irq();
    g_dbg_ilm_dim_front = dim_f;
    g_dbg_ilm_dim_rear = dim_r;
    g_dbg_ilm_dim_bg = dim_bg;
#else
    (void)d;
    (void)len;
#endif
}

static void handle_sam_f_a1(const uint8_t *d, uint8_t len)
{
    uint8_t pwr15;
    uint8_t pwr15_b1 = 0u;
    uint8_t pwr15_lo;
    uint8_t ign_on;
    uint8_t rev_mtx = 0u;
    if (!d || len < 1u) return;

    pwr15 = (uint8_t)((d[0] >> 7) & 0x01u);  /* Primary DBC mapping: 7|1 */
    if (len > 1u) {
        pwr15_b1 = (uint8_t)((d[1] >> 7) & 0x01u);
    }
    pwr15_lo = (uint8_t)((d[0] >> 0) & 0x01u);
    if (len > 4u) {
        /* BO_6 SAM_F_A1: RevGr_Engg_MTX : start bit 36 => byte4 bit4 */
        rev_mtx = (uint8_t)((d[4] >> 4) & 0x01u);
    }

    /* Robust decoding across SAM_F_A1 bit-layout variants:
     * 1) primary d0[7:6]
     * 2) alternate d1[7:6]
     * 3) legacy bench traces d0[1:0] */
    if (pwr15) {
        ign_on = 1u;
    } else if (pwr15_b1) {
        ign_on = 1u;
    } else {
        ign_on = pwr15_lo;
    }

    __disable_irq();
    g_ignition_on = ign_on;
    g_ignition_last_rx_ms = HAL_GetTick();
    g_reverse_samf_active = rev_mtx;
    g_reverse_samf_last_ms = HAL_GetTick();
    __enable_irq();
}

static void handle_lm_a1(const uint8_t *d, uint8_t len)
{
    if (!d || len < 1u) return;

    __disable_irq();
    g_ns_ill_active = (uint8_t)(d[0] & 0x01u); /* NS_Ill_Actv 0|1 */
    g_lm_a1_last_rx_ms = HAL_GetTick();
    __enable_irq();
}

static void handle_lm_a4(const uint8_t *d, uint8_t len)
{
    uint8_t pddl_any;
    uint32_t now;
    if (!d || len < 6u) return;

    /* PddlLmp_FL/FR/RL/RR_On_Rq are byte5 bits 7..4 in this DBC layout. */
    pddl_any = (uint8_t)(((d[5] & 0xF0u) != 0u) ? 1u : 0u);
    now = HAL_GetTick();
    g_dbg_lm_a4_b5 = d[5];

    __disable_irq();
    if (pddl_any) {
        g_pddllmp_last_on_ms = now;
        g_pddllmp_off_candidate_ms = 0u;
    } else if (g_pddllmp_off_candidate_ms == 0u) {
        g_pddllmp_off_candidate_ms = now;
    }
    g_pddllmp_any_on_request = pddl_any;
    g_lm_a4_last_rx_ms = now;
    __enable_irq();
}

static void handle_tgw_daynight(const uint8_t *d, uint8_t len)
{
#if AMB_ENABLE_HU_DAYNIGHT_FALLBACK
    uint8_t raw;
    uint8_t night;
    if (!d || len < 1u) return;

    raw = (uint8_t)((d[0] >> 4) & 0x03u); /* HU_DayNightMd_Stat 4|2 */
    night = (uint8_t)(raw != 0u ? 1u : 0u);
    g_dbg_hu_daynight_raw = raw;
    __disable_irq();
    g_hu_daynight_valid = 1u;
    g_hu_daynight_night = night;
    g_hu_daynight_last_rx_ms = HAL_GetTick();
    __enable_irq();
    update_night_mode_fallback();
#else
    (void)d;
    (void)len;
#endif
}

static float parking_nibble_to_level(uint8_t n)
{
    float x = (float)(n & 0x0Fu) / 15.0f;
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;
    return x;
}

static void handle_pts_a1(const uint8_t *d, uint8_t len)
{
#if AMB_ENABLE_PARKING_WARN_OVERLAY
    uint8_t fl, fr, rl, rr;
    uint8_t rq_l, rq_r;
    float lvl_l, lvl_r, lvl_rear;

    if (!d || len < 7u) return;
    fl = (uint8_t)((d[3] >> 4) & 0x0Fu); /* WarnElem_FL_Stat_V2 28|4 */
    fr = (uint8_t)((d[3] >> 0) & 0x0Fu); /* WarnElem_FR_Stat_V2 24|4 */
    rl = (uint8_t)((d[6] >> 4) & 0x0Fu); /* WarnElem_RL_Stat_V2 52|4 */
    rr = (uint8_t)((d[6] >> 0) & 0x0Fu); /* WarnElem_RR_Stat_V2 48|4 */
    rq_l = (uint8_t)((d[1] >> 3) & 0x03u); /* ParkSideProt_Lt_Warn_Rq 11|2 */
    rq_r = (uint8_t)((d[1] >> 1) & 0x03u); /* ParkSideProt_Rt_Warn_Rq 9|2 */

    lvl_l = parking_nibble_to_level((fl > rl) ? fl : rl);
    lvl_r = parking_nibble_to_level((fr > rr) ? fr : rr);
    lvl_rear = parking_nibble_to_level((rl > rr) ? rl : rr);
    if (rq_l) {
        float rq = (float)rq_l / 3.0f;
        if (rq > lvl_l) lvl_l = rq;
    }
    if (rq_r) {
        float rq = (float)rq_r / 3.0f;
        if (rq > lvl_r) lvl_r = rq;
    }

    __disable_irq();
    g_parking_left_level = lvl_l;
    g_parking_right_level = lvl_r;
    g_parking_rear_level = lvl_rear;
    g_parking_left_active = (uint8_t)(lvl_l > 0.02f ? 1u : 0u);
    g_parking_right_active = (uint8_t)(lvl_r > 0.02f ? 1u : 0u);
    g_parking_rear_active = (uint8_t)(lvl_rear > 0.02f ? 1u : 0u);
    g_parking_last_rx_ms = HAL_GetTick();
    __enable_irq();
#else
    (void)d;
    (void)len;
#endif
}

static void handle_rvc_reverse(const uint8_t *d, uint8_t len)
{
    uint8_t rev;
    uint32_t now;
    if (!d || len < 2u) return;
    rev = (uint8_t)((d[1] >> 3) & 0x01u); /* RVC_Rev_Enbl 11|1 */
    now = HAL_GetTick();
    __disable_irq();
    g_reverse_active = rev;
    if (rev) g_reverse_last_active_ms = now;
    g_reverse_last_rx_ms = now;
    __enable_irq();
}

static void handle_chassis_reverse(const uint8_t *d, uint8_t len)
{
#if AMB_ENABLE_REVERSE_SIGNAL_FROM_CHASSIS
    uint8_t gear_raw;
    uint8_t is_reverse;
    uint32_t now;
    if (!d || len < 1u) return;
    /* Vehicle-proven mapping for 0x10C byte0:
     * P: 0x00/0x01, N: 0x10/0x11, D: 0x20/0x21, R: 0x48/0x49.
     * bit0 = brake pedal, must be ignored for reverse detection. */
    gear_raw = (uint8_t)(d[0] & 0x78u);
    is_reverse = (uint8_t)((gear_raw == 0x48u) ? 1u : 0u);
    now = HAL_GetTick();
    __disable_irq();
    g_reverse_active = is_reverse;
    if (is_reverse) g_reverse_last_active_ms = now;
    g_reverse_last_rx_ms = now;
    __enable_irq();
#else
    (void)d;
    (void)len;
#endif
}

static void handle_hvac_a1_sun(const uint8_t *d, uint8_t len)
{
#if AMB_ENABLE_SUN_INTENSITY_COMP
    if (!d || len < 5u) return;
    __disable_irq();
    g_sun_fl = d[3]; /* SunInsty_FL 24|8 */
    g_sun_fr = d[4]; /* SunInsty_FR 32|8 */
    g_sun_valid = 1u;
    g_sun_last_rx_ms = HAL_GetTick();
    __enable_irq();
    g_dbg_sun_fl = d[3];
    g_dbg_sun_fr = d[4];
    g_dbg_sun_valid = 1u;
#else
    (void)d;
    (void)len;
#endif
}

static uint8_t seat_step_clamp(uint8_t v)
{
    return (v > 3u) ? 3u : v;
}

static void handle_seat_heat(uint32_t id, const uint8_t *d, uint8_t len)
{
    if (!d || len < 3u) return;

    __disable_irq();
    if (id == CAN_SEAT_FRONT_ID) {
        /* BO_873: FL=3|2, FR=14|2 */
        g_seat_heat_fl = seat_step_clamp((uint8_t)((d[0] >> 3) & 0x03u));
        g_seat_heat_fr = seat_step_clamp((uint8_t)((d[1] >> 6) & 0x03u));
        g_seat_front_valid = 1u;
    } else if (id == CAN_SEAT_REAR_ID) {
        /* BO_848: RL=12|2, RR=20|2 */
        g_seat_heat_rl = seat_step_clamp((uint8_t)((d[1] >> 4) & 0x03u));
        g_seat_heat_rr = seat_step_clamp((uint8_t)((d[2] >> 4) & 0x03u));
        g_seat_rear_valid = 1u;
    }
    __enable_irq();
}

static void handle_bsm(const uint8_t *d, uint8_t len)
{
    g_bsm_last_rx_ms = HAL_GetTick();
#if AMB_BSM_ACCEPT_ANY_17E
    float lvl = 0.80f;
    if (len >= 2u) {
        uint8_t m = (d[0] > d[1]) ? d[0] : d[1];
        if (m > 0u) {
            lvl = (float)m / 255.0f;
        }
    }
    if (lvl < 0.55f) lvl = 0.55f;
    g_bsm.left_level = lvl;
    g_bsm.right_level = lvl;
    g_bsm.left_active = 1u;
    g_bsm.right_active = 1u;
#else
    uint8_t l_warn_brt;
    uint8_t r_warn_brt;
    uint8_t l_acust_rq;
    uint8_t r_acust_rq;
    uint8_t any_acust_rq;
    uint8_t l_active;
    uint8_t r_active;

    if (len < 6u) return;

    /* BO_382 BSM_WARN_RQ:
     *   BSM_WarnLmp_Lt_Brt : byte2
     *   BSM_WarnLmp_Rt_Brt : byte3
     *   BSM_AcustWarn_Lt_Rq: byte4 bits[7:6]
     *   BSM_AcustWarn_Rt_Rq: byte4 bits[5:4]
     *   BSM_AcustWarn_Rq   : byte5 bits[1:0] */
    l_warn_brt = d[2];
    r_warn_brt = d[3];
    l_acust_rq = (uint8_t)((d[4] >> 6) & 0x03u);
    r_acust_rq = (uint8_t)((d[4] >> 4) & 0x03u);
    any_acust_rq = (uint8_t)(d[5] & 0x03u);

    /* Important: do not treat "ready/active lamp brightness" as warning.
     * Overlay is driven by acoustic warning requests only. */
    l_active = (uint8_t)(((l_acust_rq != 0u) || (any_acust_rq != 0u)) ? 1u : 0u);
    r_active = (uint8_t)(((r_acust_rq != 0u) || (any_acust_rq != 0u)) ? 1u : 0u);
    g_bsm.left_active = l_active;
    g_bsm.right_active = r_active;

    if (l_active) {
        g_bsm.left_level = (float)l_warn_brt / 255.0f;
        if (g_bsm.left_level < 0.55f) g_bsm.left_level = 0.55f;
    } else {
        g_bsm.left_level = 0.0f;
    }

    if (r_active) {
        g_bsm.right_level = (float)r_warn_brt / 255.0f;
        if (g_bsm.right_level < 0.55f) g_bsm.right_level = 0.55f;
    } else {
        g_bsm.right_level = 0.0f;
    }
#endif
}

static void handle_drive_mode_profile(uint32_t id, const uint8_t *d, uint8_t len)
{
#if AMB_ENABLE_DRIVE_MODE_AUTOPROFILE
    uint8_t raw;
    motion_profile_t p = g_motion_profile;

    if (AMB_DRIVE_MODE_CAN_ID == 0u || id != AMB_DRIVE_MODE_CAN_ID || !d) return;
    if (AMB_DRIVE_MODE_BYTE_INDEX >= len) return;

    raw = (uint8_t)((d[AMB_DRIVE_MODE_BYTE_INDEX] >> AMB_DRIVE_MODE_BIT_SHIFT) & AMB_DRIVE_MODE_BIT_MASK);
    if (raw == AMB_DRIVE_MODE_VALUE_SPORT || raw == AMB_DRIVE_MODE_VALUE_SPORT_PLUS) {
        p = MOTION_PROFILE_SPORT;
    } else if (raw == AMB_DRIVE_MODE_VALUE_CALM || raw == AMB_DRIVE_MODE_VALUE_SLEEK) {
        p = MOTION_PROFILE_CALM;
    } else if (raw == AMB_DRIVE_MODE_VALUE_COMFORT || raw == AMB_DRIVE_MODE_VALUE_INDIVIDUAL) {
        p = MOTION_PROFILE_LUXURY;
    } else {
        return;
    }
    set_motion_profile(p);
    __disable_irq();
    bank_character_sample();
    __enable_irq();
#else
    (void)id;
    (void)d;
    (void)len;
#endif
}

static void handle_oem(const uint8_t *d, uint8_t len)
{
    uint8_t br_raw;
    uint8_t raw_col_32;
#if (AMB_OEM_COLOR_SOURCE_POLICY == AMB_OEM_COLOR_SRC_BITS_5_4) || \
    (AMB_OEM_COLOR_SOURCE_POLICY == AMB_OEM_COLOR_SRC_AUTO)
    uint8_t raw_col_54;
#endif
    uint8_t raw_col;
    uint8_t col;

    if (len < 4u) return;

    br_raw = d[0];
    raw_col_32 = (uint8_t)((d[3] >> 2) & 0x03u);
#if (AMB_OEM_COLOR_SOURCE_POLICY == AMB_OEM_COLOR_SRC_BITS_5_4) || \
    (AMB_OEM_COLOR_SOURCE_POLICY == AMB_OEM_COLOR_SRC_AUTO)
    raw_col_54 = (uint8_t)((d[3] >> 4) & 0x03u);
#endif

#if (AMB_OEM_COLOR_SOURCE_POLICY == AMB_OEM_COLOR_SRC_BITS_5_4)
    raw_col = raw_col_54;
#elif (AMB_OEM_COLOR_SOURCE_POLICY == AMB_OEM_COLOR_SRC_AUTO)
    if (g_oem_color_src_locked == 0u && g_oem_col_obs_count < 64u) {
        if (raw_col_32 <= 2u && g_oem_col_prev_32 <= 2u && raw_col_32 != g_oem_col_prev_32) {
            if (g_oem_col_score_32 < 250u) g_oem_col_score_32++;
        }
        if (raw_col_54 <= 2u && g_oem_col_prev_54 <= 2u && raw_col_54 != g_oem_col_prev_54) {
            if (g_oem_col_score_54 < 250u) g_oem_col_score_54++;
        }
        g_oem_col_prev_32 = raw_col_32;
        g_oem_col_prev_54 = raw_col_54;
        g_oem_col_obs_count++;

        if (raw_col_32 <= 2u && raw_col_54 > 2u) {
            g_oem_color_src_locked = 1u;
        } else if (raw_col_54 <= 2u && raw_col_32 > 2u) {
            g_oem_color_src_locked = 2u;
        } else if (g_oem_col_obs_count >= 12u) {
            if (g_oem_col_score_32 >= (uint8_t)(g_oem_col_score_54 + 2u)) {
                g_oem_color_src_locked = 1u;
            } else if (g_oem_col_score_54 >= (uint8_t)(g_oem_col_score_32 + 2u)) {
                g_oem_color_src_locked = 2u;
            }
        }
    }
    if (g_oem_color_src_locked == 2u) {
        raw_col = raw_col_54;
    } else if (g_oem_color_src_locked == 1u) {
        raw_col = raw_col_32;
    } else {
        /* While unlocked prefer channel with observed variation. */
        raw_col = (g_oem_col_score_54 > g_oem_col_score_32) ? raw_col_54 : raw_col_32;
    }
#else
    raw_col = raw_col_32;
#endif

    switch (raw_col) {
    case 0: col = OEM_COLOR_AMBER; break;
    case 1: col = OEM_COLOR_WHITE; break;
    case 2: col = OEM_COLOR_BLUE; break;
    default:
        g_ingress_diag.oem_invalid_range_count++;
        return;
    }

    br_raw = clamp_u8_count(br_raw, 5u, &g_ingress_diag.oem_clamped_brightness_count);

    g_oem_packet_received = 1u;
    g_oem_last_rx_ms = HAL_GetTick();
    g_oem_diag_b3 = d[3];
    g_oem_diag_flags = (uint8_t)((raw_col & 0x03u) |
                                 ((col & 0x03u) << 2) |
                                 ((g_can_state.bank_id & 0x03u) << 4));
#if AMB_ENABLE_SLEEP_MODE
    can_power_note_can_activity();
#endif

    __disable_irq();
    g_can_state.oem_color = col;
    g_can_state.oem_brightness = (float)br_raw / 5.0f;
    g_oem_diag_flags = (uint8_t)((raw_col & 0x03u) |
                                 ((col & 0x03u) << 2) |
                                 ((g_can_state.bank_id & 0x03u) << 4));
    bank_character_sample();
    __enable_irq();
}

static void handle_oem_brightness_only(const uint8_t *d, uint8_t len)
{
    uint8_t br_raw;

    if (len < 1u) return;
    br_raw = d[0];
    if (br_raw > 5u) br_raw = 5u;

    __disable_irq();
    g_can_state.oem_brightness = (float)br_raw / 5.0f;
    __enable_irq();
}

static void handle_master(const uint8_t *d, uint8_t len)
{
    uint8_t flags;
    uint8_t bank_id;
    uint8_t color_id;
    uint8_t oem_col;
    uint8_t br_raw;
    uint8_t profile_raw;
    uint8_t old_bank;
    uint8_t old_color;
    uint8_t local_color;
    uint8_t use_local_oem;
    uint32_t now;

    if (len < 4u) return;

    flags = d[0] & 0x0Fu;
    bank_id = d[1];
    color_id = d[2] & 0x0Fu;
    oem_col = d[3];
    br_raw = (len > 4u) ? d[4] : 5u;
    profile_raw = (len > 5u) ? d[5] : (uint8_t)AMB_DEFAULT_MOTION_PROFILE;

    if (bank_id > 2u || oem_col > 2u) return;
    if (br_raw > 5u) br_raw = 5u;

    g_oem_packet_received = 1u;
    now = HAL_GetTick();
    g_oem_last_rx_ms = now;
    use_local_oem = local_oem_fresh(now);
#if AMB_ENABLE_SLEEP_MODE
    can_power_note_can_activity();
#endif

    __disable_irq();
    old_bank = g_can_state.bank_id;
    old_color = g_can_state.oem_color;
    local_color = g_can_state.oem_color;
    if (!use_local_oem) {
        local_color = oem_col;
    }
    g_can_state.night_mode = (flags & MASTER_FLAG_NIGHT) ? 1u : 0u;
    g_can_state.bank_id = local_color;
    g_can_state.oem_color = color_id;
    if (!use_local_oem) {
        g_can_state.oem_brightness = (float)br_raw / 5.0f;
    }
    set_motion_profile((motion_profile_t)profile_raw);
    bank_character_sample();
    __enable_irq();

    if (old_bank != local_color || old_color != color_id) {
        can_persist_state_note_state_change(now);
    }
}

static void handle_discovery(const uint8_t *d, uint8_t len)
{
    uint8_t board_type;

    if (len < 2u) return;
    board_type = d[0] & 0x0Fu;
    if (board_type < 6u) {
        can_role_handle_discovery(board_type, HAL_GetTick());
    }
}

static void handle_sync(const uint8_t *d, uint8_t len)
{
    uint8_t color_id;
    uint8_t br_raw;

    if (len < 2u) return;

    color_id = d[0] & 0x0Fu;
    br_raw = d[1];
    if (color_id > 12u) return;
    if (br_raw > 5u) br_raw = 5u;

    __disable_irq();
    g_can_state.oem_color = color_id;
    g_can_state.bank_id = (color_id <= 2u) ? color_id : g_can_state.bank_id;
    g_can_state.oem_brightness = (float)br_raw / 5.0f;
    __enable_irq();

    can_role_note_sync_from_master(HAL_GetTick());
}

static uint8_t is_bsm_can_id(uint32_t id)
{
    if (id == CAN_BSM_ID) return 1u;
#if AMB_BSM_ACCEPT_ANY_17E
    if ((id & 0x7FFu) == CAN_BSM_ID) return 1u;
#endif
    return 0u;
}

static uint8_t process_master_id_packet(uint8_t *data, uint8_t len)
{
    uint8_t pkt_type;

    if (len < 1u) return 1u;

    pkt_type = data[0] & PKT_TYPE_MASK;
    if (pkt_type == PKT_TYPE_DISCOVERY) {
        if (len >= 2u && (data[0] & ~PKT_TYPE_MASK) < 6u) {
            handle_discovery(data, len);
        }
        return 1u;
    }
    if (pkt_type == PKT_TYPE_MASTER) {
        if (!can_role_is_master()) {
            handle_master(data, len);
            can_role_note_master_heartbeat(HAL_GetTick());
        }
        return 1u;
    }
    if (pkt_type == PKT_TYPE_SYNC) {
        handle_sync(data, len);
        return 1u;
    }
    return 1u;
}

void can_rx_init(void)
{
    g_oem_packet_received = 0u;
    g_oem_last_rx_ms = 0u;
    g_nsi_active = 0u;
    g_nsi_last_rx_ms = 0u;
    g_surrill_last_on_ms = 0u;
    g_surrill_off_candidate_ms = 0u;
    g_unlock_event_pending = 0u;
    g_unlock_event_prev_active = 0u;
    g_unlock_event_last_ms = 0u;
    g_lock_event_pending = 0u;
    g_lock_event_prev_active = 0u;
    g_lock_event_last_ms = 0u;
    g_eis_last_rx_ms = 0u;
    g_turnlmp_unlk_prev_active = 0u;
    g_turnlmp_lk_prev_active = 0u;
    g_turnlmp_unlk_last_ms = 0u;
    g_turnlmp_lk_last_ms = 0u;
    g_door_open_event_mask = 0u;
    g_door_latch_valid_mask = 0u;
    g_door_latch_open_mask = 0u;
    g_door_open_last_ms = 0u;
    g_lgtsens_last_rx_ms = 0u;
    g_hu_daynight_valid = 0u;
    g_hu_daynight_night = 0u;
    g_hu_daynight_last_rx_ms = 0u;
    g_ilm_dim_front = 0u;
    g_ilm_dim_rear = 0u;
    g_ilm_dim_bg = 0u;
    g_ilm_front_on = 0u;
    g_ilm_rear_on = 0u;
    g_ilm_last_rx_ms = 0u;
    g_ignition_on = 0u;
    g_ignition_last_rx_ms = 0u;
    g_ns_ill_active = 0u;
    g_lm_a1_last_rx_ms = 0u;
    g_pddllmp_any_on_request = 0u;
    g_lm_a4_last_rx_ms = 0u;
    g_pddllmp_last_on_ms = 0u;
    g_pddllmp_off_candidate_ms = 0u;
    g_dbg_lm_a4_b5 = 0u;
    g_parking_left_active = 0u;
    g_parking_right_active = 0u;
    g_parking_rear_active = 0u;
    g_parking_left_level = 0.0f;
    g_parking_right_level = 0.0f;
    g_parking_rear_level = 0.0f;
    g_parking_last_rx_ms = 0u;
    g_reverse_active = 0u;
    g_reverse_last_rx_ms = 0u;
    g_reverse_last_active_ms = 0u;
    g_reverse_samf_active = 0u;
    g_reverse_samf_last_ms = 0u;
    g_sun_valid = 0u;
    g_sun_fl = 0u;
    g_sun_fr = 0u;
    g_sun_last_rx_ms = 0u;
    g_hvac_prev_front_valid = 0u;
    g_hvac_prev_rear_valid = 0u;
    g_hvac_prev_fl = 0u;
    g_hvac_prev_fr = 0u;
    g_hvac_prev_rl = 0u;
    g_hvac_prev_rr = 0u;
    g_hvac_last_event_left_ms = 0u;
    g_hvac_last_event_right_ms = 0u;
    g_hvac_last_trend_left_ms = 0u;
    g_hvac_last_trend_right_ms = 0u;
    g_hvac_trend_left_pending = 0;
    g_hvac_trend_right_pending = 0;
    g_hvac_last_trend_left = 0;
    g_hvac_last_trend_right = 0;
    g_hvac_split_bias = 0.0f;
    g_hvac_fan_level = 0.0f;
    g_seat_front_valid = 0u;
    g_seat_rear_valid = 0u;
    g_seat_heat_fl = 0u;
    g_seat_heat_fr = 0u;
    g_seat_heat_rl = 0u;
    g_seat_heat_rr = 0u;
    g_bsm.left_active = 0u;
    g_bsm.right_active = 0u;
    g_bsm.left_level = 0.0f;
    g_bsm.right_level = 0.0f;
    g_bsm_last_rx_ms = 0u;
    g_motion_profile = (motion_profile_t)AMB_DEFAULT_MOTION_PROFILE;
    g_bank_character_speed[0] = 1.0f;
    g_bank_character_speed[1] = 1.0f;
    g_bank_character_speed[2] = 1.0f;
    g_bank_character_valid[0] = 0u;
    g_bank_character_valid[1] = 0u;
    g_bank_character_valid[2] = 0u;
    g_oem_color_src_locked = 0u;
    g_oem_col_prev_32 = 0xFFu;
    g_oem_col_prev_54 = 0xFFu;
    g_oem_col_score_32 = 0u;
    g_oem_col_score_54 = 0u;
    g_oem_col_obs_count = 0u;
    g_oem_diag_b3 = 0u;
    g_oem_diag_flags = 0u;
    memset((void *)&g_ingress_diag, 0, sizeof(g_ingress_diag));
}

void can_rx_process(uint32_t id, uint8_t *data, uint8_t len)
{
    int8_t role_snapshot = can_role_get();
    uint8_t role_now = (role_snapshot < 0) ? 0xFFu : (uint8_t)role_snapshot;
    uint8_t master_now = can_role_is_master();

#if AMB_ENABLE_SLEEP_MODE
    /* Keep idle-timeout aligned with semantic ambient traffic, not only OEM/master frames.
     * This prevents unintended sleep fade while user-driven events are still active. */
    if (id == CAN_OEM_ID || id == CAN_MASTER_ID ||
        id == CAN_HU_AMB_RQ_ID || id == CAN_BODY_AMB_STAT_ID ||
        id == CAN_EIS_A2_ID || id == CAN_ILM_RQ_ID ||
        id == CAN_SAM_F_A1_ID || id == CAN_LM_A1_ID || id == CAN_LM_A4_ID ||
        id == CAN_DOOR_FL_A1_ID || id == CAN_DOOR_FR_A1_ID || id == CAN_DOOR_R_A1_ID ||
        id == CAN_HVAC_FRONT_ID || id == CAN_HVAC_REAR_ID || id == CAN_HVAC_FAN_ID || id == CAN_HVAC_A1_ID ||
        id == CAN_SEAT_FRONT_ID || id == CAN_SEAT_REAR_ID ||
        id == CAN_LGTSENS_ID || id == CAN_TGW_A8_ID ||
        id == CAN_PTS_A1_ID || id == CAN_RVC_A2_ID || id == CAN_CHASSIS_R2_ID ||
        is_bsm_can_id(id)) {
        can_power_note_can_activity();
    }
#endif

    if (master_now || role_now == 0xFFu) {
        handle_drive_mode_profile(id, data, len);
    }
    if (is_bsm_can_id(id)) {
        handle_bsm(data, len);
    }

    if (id == CAN_MASTER_ID) {
        process_master_id_packet(data, len);
        return;
    }
    if (id == CAN_OEM_ID) {
        handle_oem_nsi(data, len);
        /* Brightness fallback: always accept AmbBrt_Rq from 0x325 even when this board is
         * currently slave or frame is short (bench tools often send only byte0). */
        handle_oem_brightness_only(data, len);
        handle_oem(data, len);
        return;
    }
    if (id == CAN_HU_AMB_RQ_ID) {
        handle_modern_ambient_request(data, len);
        return;
    }
    if (id == CAN_BODY_AMB_STAT_ID) {
        handle_modern_ambient_status(data, len);
        return;
    }
    if (id == CAN_EIS_A2_ID) {
        g_eis_last_rx_ms = HAL_GetTick();
        handle_eis_a2_lock_unlock(data, len);
        return;
    }
    if (id == CAN_ILM_RQ_ID) {
        handle_ilm_rq(data, len);
        return;
    }
    if (id == CAN_SAM_F_A1_ID) {
        handle_sam_f_a1(data, len);
        return;
    }
    if (id == CAN_LM_A1_ID) {
        handle_lm_a1(data, len);
        return;
    }
    if (id == CAN_LM_A4_ID) {
        handle_lm_a4(data, len);
        return;
    }
    if (id == CAN_DOOR_FL_A1_ID) {
        handle_door_fl_fr(data, len, 0u);
        return;
    }
    if (id == CAN_DOOR_FR_A1_ID) {
        handle_door_fl_fr(data, len, 1u);
        return;
    }
    if (id == CAN_DOOR_R_A1_ID) {
        handle_door_rear(data, len);
        return;
    }
    if (id == CAN_HVAC_FRONT_ID || id == CAN_HVAC_REAR_ID) {
        handle_hvac_temp(id, data, len);
        return;
    }
    if (id == CAN_HVAC_A1_ID) {
        handle_hvac_a1_sun(data, len);
        return;
    }
    if (id == CAN_HVAC_FAN_ID) {
        handle_hvac_fan(data, len);
        return;
    }
    if (id == CAN_SEAT_FRONT_ID || id == CAN_SEAT_REAR_ID) {
        handle_seat_heat(id, data, len);
        return;
    }
    if (id == CAN_LGTSENS_ID) {
        if (master_now || role_now == 0xFFu) {
            handle_lgtsens(data, len);
        }
        return;
    }
    if (id == CAN_TGW_A8_ID) {
        if (master_now || role_now == 0xFFu) {
            handle_tgw_daynight(data, len);
        }
        return;
    }
    if (id == CAN_PTS_A1_ID) {
        handle_pts_a1(data, len);
        return;
    }
    if (id == CAN_RVC_A2_ID) {
        handle_rvc_reverse(data, len);
        return;
    }
    if (id == CAN_CHASSIS_R2_ID) {
        handle_chassis_reverse(data, len);
    }
}

uint8_t can_rx_get_modern_request(uint8_t *color_rq,
                                  uint8_t *brightness_rq,
                                  uint8_t *effect_rq,
                                  uint32_t *timestamp_ms)
{
    uint8_t valid;

    __disable_irq();
    valid = g_modern_rq_valid;
    if (color_rq) *color_rq = g_modern_rq_color;
    if (brightness_rq) *brightness_rq = g_modern_rq_brightness;
    if (effect_rq) *effect_rq = g_modern_rq_effect;
    if (timestamp_ms) *timestamp_ms = g_modern_rq_last_rx_ms;
    __enable_irq();

    return valid;
}

uint8_t can_rx_get_modern_bus_status(uint8_t *color_stat,
                                     uint8_t *brightness_stat,
                                     uint8_t *effect_stat,
                                     uint32_t *timestamp_ms)
{
    uint8_t valid;

    __disable_irq();
    valid = g_modern_stat_valid;
    if (color_stat) *color_stat = g_modern_stat_color;
    if (brightness_stat) *brightness_stat = g_modern_stat_brightness;
    if (effect_stat) *effect_stat = g_modern_stat_effect;
    if (timestamp_ms) *timestamp_ms = g_modern_stat_last_rx_ms;
    __enable_irq();

    return valid;
}

void can_rx_get_ingress_diag(can_ingress_diag_t *out)
{
    if (!out) return;
    __disable_irq();
    *out = g_ingress_diag;
    __enable_irq();
}

void can_rx_reset_ingress_diag(void)
{
    __disable_irq();
    memset((void *)&g_ingress_diag, 0, sizeof(g_ingress_diag));
    __enable_irq();
}

uint8_t can_rx_oem_received(void)
{
    uint32_t now;
    uint32_t last_ms;
    uint8_t received;

    __disable_irq();
    received = g_oem_packet_received;
    last_ms = g_oem_last_rx_ms;
    __enable_irq();

    if (!received || last_ms == 0u) {
        return 0u;
    }
    now = HAL_GetTick();
    if (elapsed_ms32(now, last_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        __disable_irq();
        g_oem_packet_received = 0u;
        __enable_irq();
        return 0u;
    }
    return 1u;
}

void can_rx_reset_oem_received(void)
{
    __disable_irq();
    g_oem_packet_received = 0u;
    g_oem_last_rx_ms = 0u;
    g_nsi_active = 0u;
    g_nsi_last_rx_ms = 0u;
    g_surrill_last_on_ms = 0u;
    g_surrill_off_candidate_ms = 0u;
    g_oem_color_src_locked = 0u;
    g_oem_col_prev_32 = 0xFFu;
    g_oem_col_prev_54 = 0xFFu;
    g_oem_col_score_32 = 0u;
    g_oem_col_score_54 = 0u;
    g_oem_col_obs_count = 0u;
    g_oem_diag_b3 = 0u;
    g_oem_diag_flags = 0u;
    g_unlock_event_pending = 0u;
    g_unlock_event_prev_active = 0u;
    g_unlock_event_last_ms = 0u;
    g_lock_event_pending = 0u;
    g_lock_event_prev_active = 0u;
    g_lock_event_last_ms = 0u;
    g_eis_last_rx_ms = 0u;
    g_turnlmp_unlk_prev_active = 0u;
    g_turnlmp_lk_prev_active = 0u;
    g_turnlmp_unlk_last_ms = 0u;
    g_turnlmp_lk_last_ms = 0u;
    g_door_open_event_mask = 0u;
    g_door_latch_valid_mask = 0u;
    g_door_latch_open_mask = 0u;
    g_door_open_last_ms = 0u;
    g_lgtsens_last_rx_ms = 0u;
    g_hu_daynight_valid = 0u;
    g_hu_daynight_night = 0u;
    g_hu_daynight_last_rx_ms = 0u;
    g_ilm_dim_front = 0u;
    g_ilm_dim_rear = 0u;
    g_ilm_dim_bg = 0u;
    g_ilm_front_on = 0u;
    g_ilm_rear_on = 0u;
    g_ilm_last_rx_ms = 0u;
    g_ignition_on = 0u;
    g_ignition_last_rx_ms = 0u;
    g_ns_ill_active = 0u;
    g_lm_a1_last_rx_ms = 0u;
    g_pddllmp_any_on_request = 0u;
    g_lm_a4_last_rx_ms = 0u;
    g_pddllmp_last_on_ms = 0u;
    g_pddllmp_off_candidate_ms = 0u;
    g_dbg_lm_a4_b5 = 0u;
    g_parking_left_active = 0u;
    g_parking_right_active = 0u;
    g_parking_rear_active = 0u;
    g_parking_left_level = 0.0f;
    g_parking_right_level = 0.0f;
    g_parking_rear_level = 0.0f;
    g_parking_last_rx_ms = 0u;
    g_reverse_active = 0u;
    g_reverse_last_rx_ms = 0u;
    g_reverse_last_active_ms = 0u;
    g_reverse_samf_active = 0u;
    g_reverse_samf_last_ms = 0u;
    g_sun_valid = 0u;
    g_sun_fl = 0u;
    g_sun_fr = 0u;
    g_sun_last_rx_ms = 0u;
    g_hvac_prev_front_valid = 0u;
    g_hvac_prev_rear_valid = 0u;
    g_hvac_prev_fl = 0u;
    g_hvac_prev_fr = 0u;
    g_hvac_prev_rl = 0u;
    g_hvac_prev_rr = 0u;
    g_hvac_last_event_left_ms = 0u;
    g_hvac_last_event_right_ms = 0u;
    g_hvac_last_trend_left_ms = 0u;
    g_hvac_last_trend_right_ms = 0u;
    g_hvac_trend_left_pending = 0;
    g_hvac_trend_right_pending = 0;
    g_hvac_last_trend_left = 0;
    g_hvac_last_trend_right = 0;
    g_hvac_split_bias = 0.0f;
    g_hvac_fan_level = 0.0f;
    g_seat_front_valid = 0u;
    g_seat_rear_valid = 0u;
    g_seat_heat_fl = 0u;
    g_seat_heat_fr = 0u;
    g_seat_heat_rl = 0u;
    g_seat_heat_rr = 0u;
    g_bsm.left_active = 0u;
    g_bsm.right_active = 0u;
    g_bsm.left_level = 0.0f;
    g_bsm.right_level = 0.0f;
    g_bsm_last_rx_ms = 0u;
    g_bank_character_speed[0] = 1.0f;
    g_bank_character_speed[1] = 1.0f;
    g_bank_character_speed[2] = 1.0f;
    g_bank_character_valid[0] = 0u;
    g_bank_character_valid[1] = 0u;
    g_bank_character_valid[2] = 0u;
    __enable_irq();
}

uint8_t can_rx_nsi_active(void)
{
    uint32_t now;
    uint32_t last_ms;
    uint8_t active;

    __disable_irq();
    active = g_nsi_active;
    last_ms = g_nsi_last_rx_ms;
    __enable_irq();

    if (!active) {
        return 0u;
    }
    if (last_ms == 0u) {
        return 0u;
    }
    now = HAL_GetTick();
    if (elapsed_ms32(now, last_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        __disable_irq();
        g_nsi_active = 0u;
        __enable_irq();
        return 0u;
    }
    return 1u;
}

uint8_t can_rx_is_ilm_rear_on_request(void)
{
    uint32_t now;
    uint32_t last_ms;
    uint8_t active;

    __disable_irq();
    active = g_ilm_rear_on;
    last_ms = g_ilm_last_rx_ms;
    __enable_irq();

    if (!active || last_ms == 0u) {
        return 0u;
    }
    now = HAL_GetTick();
    if (elapsed_ms32(now, last_ms) > AMB_IGNITION_ACTIVE_TIMEOUT_MS) {
        return 0u;
    }
    return 1u;
}

uint8_t can_rx_is_ns_ill_active(void)
{
    uint32_t now;
    uint32_t last_ms;
    uint8_t active;

    __disable_irq();
    active = g_ns_ill_active;
    last_ms = g_lm_a1_last_rx_ms;
    __enable_irq();

    if (!active || last_ms == 0u) {
        return 0u;
    }
    now = HAL_GetTick();
    if (elapsed_ms32(now, last_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        return 0u;
    }
    return 1u;
}

uint8_t can_rx_is_pddllmp_any_on_request(void)
{
    uint32_t now;
    uint32_t last_ms;
    uint32_t on_ms;
    uint32_t off_ms;
    uint8_t active;

    __disable_irq();
    active = g_pddllmp_any_on_request;
    last_ms = g_lm_a4_last_rx_ms;
    on_ms = g_pddllmp_last_on_ms;
    off_ms = g_pddllmp_off_candidate_ms;
    __enable_irq();

    if (last_ms == 0u) {
        return 0u;
    }
    now = HAL_GetTick();
    if (elapsed_ms32(now, last_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        return 0u;
    }
    if (active) return 1u;
    if (on_ms != 0u && elapsed_ms32(now, on_ms) < AMB_PDDLLMP_HANDLE_HOLD_MS) return 1u;
    if (off_ms != 0u && elapsed_ms32(now, off_ms) < AMB_PDDLLMP_OFF_CONFIRM_MS) return 1u;
    return 0u;
}

uint8_t can_rx_is_ignition_on(void)
{
    uint32_t now;
    uint32_t last_ms;
    uint8_t active;

    __disable_irq();
    active = g_ignition_on;
    last_ms = g_ignition_last_rx_ms;
    __enable_irq();

    if (!active || last_ms == 0u) {
        return 0u;
    }
    now = HAL_GetTick();
    if (elapsed_ms32(now, last_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        return 0u;
    }
    return 1u;
}

uint8_t can_rx_consume_unlock_event(void)
{
    uint8_t pending;
    __disable_irq();
    pending = g_unlock_event_pending;
    g_unlock_event_pending = 0u;
    __enable_irq();
    return pending;
}

uint8_t can_rx_consume_lock_event(void)
{
    uint8_t pending;
    uint32_t now = HAL_GetTick();
    __disable_irq();
    pending = g_lock_event_pending;
    if (pending && g_lock_event_last_ms != 0u &&
        elapsed_ms32(now, g_lock_event_last_ms) > AMB_LOCK_EVENT_PENDING_TTL_MS) {
        pending = 0u;
    }
    g_lock_event_pending = 0u;
    __enable_irq();
    return pending;
}

void can_rx_clear_lock_event_pending(void)
{
    __disable_irq();
    g_lock_event_pending = 0u;
    __enable_irq();
}

uint8_t can_rx_is_lock_source_recent(void)
{
    uint32_t now;
    uint32_t age;
    uint32_t last;

    __disable_irq();
    now = HAL_GetTick();
    last = g_eis_last_rx_ms;
    __enable_irq();

    if (last == 0u) return 0u;
    age = elapsed_ms32(now, last);
    return (uint8_t)(age <= AMB_LOCK_EVENT_SOURCE_MAX_AGE_MS ? 1u : 0u);
}

uint8_t can_rx_consume_door_open_event(uint8_t board_type)
{
    uint8_t out = 0u;
    uint8_t mask = 0u;

    __disable_irq();
    if (board_type == BOARD_TYPE_FL) {
        if (g_door_open_event_mask & (1u << 0)) {
            out = 1u;
            mask = (uint8_t)(1u << 0);
        }
    } else if (board_type == BOARD_TYPE_FR) {
        if (g_door_open_event_mask & (1u << 1)) {
            out = 1u;
            mask = (uint8_t)(1u << 1);
        }
    } else if (board_type == BOARD_TYPE_RL) {
        if (g_door_open_event_mask & (1u << 2)) {
            out = 2u;
            mask = (uint8_t)(1u << 2);
        }
    } else if (board_type == BOARD_TYPE_RR) {
        if (g_door_open_event_mask & (1u << 3)) {
            out = 2u;
            mask = (uint8_t)(1u << 3);
        }
    } else if (board_type == BOARD_TYPE_DASHBOARD) {
        if (g_door_open_event_mask & ((1u << 0) | (1u << 1))) {
            out = 1u;
            mask = (uint8_t)((1u << 0) | (1u << 1));
        }
    } else if (board_type == BOARD_TYPE_REAR) {
        if (g_door_open_event_mask & ((1u << 2) | (1u << 3))) {
            out = 2u;
            mask = (uint8_t)((1u << 2) | (1u << 3));
        }
    } else if (board_type == BOARD_TYPE_ANY) {
        mask = (uint8_t)((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3));
        if (g_door_open_event_mask & mask) {
            out = 1u;
        }
    }
    g_door_open_event_mask &= (uint8_t)~mask;
    __enable_irq();

    return out;
}

uint8_t can_rx_is_any_door_open_for_board(uint8_t board_type)
{
    uint8_t open_mask;
    uint8_t valid_mask;
    uint8_t has_open = 0u;

    __disable_irq();
    open_mask = g_door_latch_open_mask;
    valid_mask = g_door_latch_valid_mask;
    __enable_irq();

    if (board_type == BOARD_TYPE_FL) {
        has_open = (uint8_t)(((valid_mask & (1u << 0)) && (open_mask & (1u << 0))) ? 1u : 0u);
    } else if (board_type == BOARD_TYPE_FR) {
        has_open = (uint8_t)(((valid_mask & (1u << 1)) && (open_mask & (1u << 1))) ? 1u : 0u);
    } else if (board_type == BOARD_TYPE_RL) {
        has_open = (uint8_t)(((valid_mask & (1u << 2)) && (open_mask & (1u << 2))) ? 1u : 0u);
    } else if (board_type == BOARD_TYPE_RR) {
        has_open = (uint8_t)(((valid_mask & (1u << 3)) && (open_mask & (1u << 3))) ? 1u : 0u);
    } else if (board_type == BOARD_TYPE_DASHBOARD) {
        has_open = (uint8_t)(((valid_mask & ((1u << 0) | (1u << 1))) &&
                              (open_mask & ((1u << 0) | (1u << 1)))) ? 1u : 0u);
    } else if (board_type == BOARD_TYPE_REAR) {
        has_open = (uint8_t)(((valid_mask & ((1u << 2) | (1u << 3))) &&
                              (open_mask & ((1u << 2) | (1u << 3)))) ? 1u : 0u);
    } else if (board_type == BOARD_TYPE_ANY) {
        has_open = (uint8_t)(((valid_mask & ((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3))) &&
                              (open_mask & ((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3)))) ? 1u : 0u);
    }
    return has_open;
}

can_bsm_state_t can_rx_get_bsm_state(void)
{
    can_bsm_state_t out;
    uint32_t now;
    uint32_t age;

    __disable_irq();
    out = g_bsm;
    now = HAL_GetTick();
    __enable_irq();

    if (g_bsm_last_rx_ms == 0u) {
        out.left_active = 0u;
        out.right_active = 0u;
        out.left_level = 0.0f;
        out.right_level = 0.0f;
        return out;
    }

    age = elapsed_ms32(now, g_bsm_last_rx_ms);
    if (age > AMB_BSM_RX_TIMEOUT_MS) {
        out.left_active = 0u;
        out.right_active = 0u;
        out.left_level = 0.0f;
        out.right_level = 0.0f;
    }
    bsm_apply_board_mask(&out);
    return out;
}

motion_profile_t can_rx_get_motion_profile(void)
{
    return g_motion_profile;
}

void can_rx_set_motion_profile(motion_profile_t profile)
{
    set_motion_profile(profile);
}

uint8_t can_rx_get_oem_diag_b3(void)
{
    return g_oem_diag_b3;
}

uint8_t can_rx_get_oem_diag_flags(void)
{
    return g_oem_diag_flags;
}

int8_t can_rx_consume_hvac_temp_trend(void)
{
    int8_t trend_l;
    int8_t trend_r;

    __disable_irq();
    trend_l = g_hvac_trend_left_pending;
    trend_r = g_hvac_trend_right_pending;
    g_hvac_trend_left_pending = 0;
    g_hvac_trend_right_pending = 0;
    __enable_irq();

    if (trend_l != 0) return trend_l;
    return trend_r;
}

int8_t can_rx_consume_hvac_temp_trend_side(uint8_t right_side)
{
    int8_t trend;

    __disable_irq();
    if (right_side) {
        trend = g_hvac_trend_right_pending;
        g_hvac_trend_right_pending = 0;
    } else {
        trend = g_hvac_trend_left_pending;
        g_hvac_trend_left_pending = 0;
    }
    __enable_irq();
    return trend;
}

int8_t can_rx_consume_hvac_temp_trend_combined(void)
{
    int8_t left;
    int8_t right;

    __disable_irq();
    left = g_hvac_trend_left_pending;
    right = g_hvac_trend_right_pending;
    g_hvac_trend_left_pending = 0;
    g_hvac_trend_right_pending = 0;
    __enable_irq();

    if (left == 0) return right;
    if (right == 0) return left;
    if ((left > 0 && right > 0) || (left < 0 && right < 0)) return left;
    return 0;
}

float can_rx_get_hvac_split_bias_side(uint8_t right_side)
{
    float bias;

    __disable_irq();
    bias = g_hvac_split_bias;
    __enable_irq();

    return right_side ? -bias : bias;
}

float can_rx_get_hvac_split_bias_combined(void)
{
    float bias;
    __disable_irq();
    bias = g_hvac_split_bias;
    __enable_irq();
    return (bias < 0.0f) ? -bias : bias;
}

float can_rx_get_hvac_fan_level(void)
{
    float level;

    __disable_irq();
    level = g_hvac_fan_level;
    __enable_irq();
    return level;
}

float can_rx_get_seat_heat_level_side(uint8_t right_side)
{
    uint8_t front = 0u;
    uint8_t rear = 0u;
    uint8_t level;

    __disable_irq();
    if (right_side) {
        if (g_seat_front_valid) front = g_seat_heat_fr;
        if (g_seat_rear_valid) rear = g_seat_heat_rr;
    } else {
        if (g_seat_front_valid) front = g_seat_heat_fl;
        if (g_seat_rear_valid) rear = g_seat_heat_rl;
    }
    __enable_irq();

    level = (front > rear) ? front : rear;
    return (float)level / 3.0f;
}

float can_rx_get_seat_heat_level_combined(void)
{
    float left = can_rx_get_seat_heat_level_side(0u);
    float right = can_rx_get_seat_heat_level_side(1u);
    return (left > right) ? left : right;
}

float can_rx_get_bank_character_speed_scale(void)
{
#if AMB_ENABLE_BANK_CHARACTER_MEMORY
    uint8_t bank;
    float scale = 1.0f;

    __disable_irq();
    bank = g_can_state.oem_color;
    if (bank < OEM_COLOR_MAX && g_bank_character_valid[bank]) {
        scale = g_bank_character_speed[bank];
    }
    __enable_irq();

    if (scale < AMB_BANK_CHARACTER_SPEED_MIN) scale = AMB_BANK_CHARACTER_SPEED_MIN;
    if (scale > AMB_BANK_CHARACTER_SPEED_MAX) scale = AMB_BANK_CHARACTER_SPEED_MAX;
    return scale;
#else
    return 1.0f;
#endif
}

float can_rx_get_ilm_dim_scale(uint8_t board_type)
{
#if AMB_ENABLE_ILM_DIM_REQUESTS
    uint8_t dim_front;
    uint8_t dim_rear;
    uint8_t dim_bg;
    uint32_t last_ms;
    uint32_t now = HAL_GetTick();
    float side_scale = 1.0f;
    float bg_scale = 1.0f;
    float scale = 1.0f;

    __disable_irq();
    dim_front = g_ilm_dim_front;
    dim_rear = g_ilm_dim_rear;
    dim_bg = g_ilm_dim_bg;
    last_ms = g_ilm_last_rx_ms;
    __enable_irq();

    if (last_ms == 0u || elapsed_ms32(now, last_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        g_dbg_ilm_dim_scale = 1.0f;
        return 1.0f;
    }

    if (board_type == BOARD_TYPE_FL || board_type == BOARD_TYPE_FR || board_type == BOARD_TYPE_DASHBOARD) {
        if (dim_front) side_scale = AMB_ILM_DIM_SCALE_FRONT;
    } else if (board_type == BOARD_TYPE_RL || board_type == BOARD_TYPE_RR || board_type == BOARD_TYPE_REAR) {
        if (dim_rear) side_scale = AMB_ILM_DIM_SCALE_REAR;
    }
    if (dim_bg) bg_scale = AMB_ILM_DIM_SCALE_BG;

    /* Softer premium behavior: combine independent dim requests by cap, not product. */
    scale = (side_scale < bg_scale) ? side_scale : bg_scale;
    if (scale < 0.25f) scale = 0.25f;
    if (scale > 1.0f) scale = 1.0f;
    g_dbg_ilm_dim_scale = scale;
    return scale;
#else
    (void)board_type;
    g_dbg_ilm_dim_scale = 1.0f;
    return 1.0f;
#endif
}

float can_rx_get_sun_gain_scale(uint8_t board_type)
{
#if AMB_ENABLE_SUN_INTENSITY_COMP
    uint8_t fl;
    uint8_t fr;
    uint8_t valid;
    uint32_t last_ms;
    uint32_t now = HAL_GetTick();
    float raw = 0.0f;

    __disable_irq();
    fl = g_sun_fl;
    fr = g_sun_fr;
    valid = g_sun_valid;
    last_ms = g_sun_last_rx_ms;
    __enable_irq();

    if (!valid || last_ms == 0u || elapsed_ms32(now, last_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        g_dbg_sun_gain_scale = 1.0f;
        return 1.0f;
    }

    /* Vehicle-specific scale:
     * up to 63 -> normalize by 63; above that -> fallback to 255 scale. */
    if ((fl <= 63u) && (fr <= 63u)) {
        (void)board_type;
        raw = (float)((fl > fr) ? fl : fr) / 63.0f;
    } else {
        (void)board_type;
        raw = (float)((fl > fr) ? fl : fr) / 255.0f;
    }

    g_dbg_sun_gain_scale = 1.0f + AMB_SUN_GAIN_MAX * raw;
    return g_dbg_sun_gain_scale;
#else
    (void)board_type;
    g_dbg_sun_gain_scale = 1.0f;
    return 1.0f;
#endif
}

can_parking_warn_state_t can_rx_get_parking_warn_state(uint8_t board_type)
{
    can_parking_warn_state_t out = {0u, 0u, 0u, 0.0f, 0.0f, 0.0f};
#if AMB_ENABLE_PARKING_WARN_OVERLAY
    uint32_t now;
    uint32_t age;

    __disable_irq();
    out.left_active = g_parking_left_active;
    out.right_active = g_parking_right_active;
    out.rear_active = g_parking_rear_active;
    out.left_level = g_parking_left_level;
    out.right_level = g_parking_right_level;
    out.rear_level = g_parking_rear_level;
    now = HAL_GetTick();
    __enable_irq();

    if (g_parking_last_rx_ms == 0u) {
        out.left_active = 0u;
        out.right_active = 0u;
        out.rear_active = 0u;
        out.left_level = 0.0f;
        out.right_level = 0.0f;
        out.rear_level = 0.0f;
        return out;
    }
    age = elapsed_ms32(now, g_parking_last_rx_ms);
    if (age > AMB_PARKING_WARN_HOLD_MS) {
        out.left_active = 0u;
        out.right_active = 0u;
        out.rear_active = 0u;
        out.left_level = 0.0f;
        out.right_level = 0.0f;
        out.rear_level = 0.0f;
        return out;
    }

    if (board_type == BOARD_TYPE_FL || board_type == BOARD_TYPE_RL) {
        out.right_active = 0u;
        out.rear_active = 0u;
        out.right_level = 0.0f;
        out.rear_level = 0.0f;
    } else if (board_type == BOARD_TYPE_FR || board_type == BOARD_TYPE_RR) {
        out.left_active = 0u;
        out.rear_active = 0u;
        out.left_level = 0.0f;
        out.rear_level = 0.0f;
    } else if (board_type == BOARD_TYPE_DASHBOARD) {
        out.rear_active = 0u;
        out.rear_level = 0.0f;
    } else if (board_type == BOARD_TYPE_REAR) {
        out.left_active = 0u;
        out.right_active = 0u;
        out.left_level = 0.0f;
        out.right_level = 0.0f;
    }
#else
    (void)board_type;
#endif
    return out;
}

uint8_t can_rx_is_reverse_signal_active(void)
{
    uint8_t active;
    uint32_t now;
    uint32_t age;

    __disable_irq();
    active = g_reverse_active;
    now = HAL_GetTick();
    __enable_irq();

    age = (g_reverse_last_rx_ms == 0u) ? (AMB_CAN_ACTIVE_TIMEOUT_MS + 1u) : elapsed_ms32(now, g_reverse_last_rx_ms);
    if (age <= AMB_CAN_ACTIVE_TIMEOUT_MS && active) return 1u;
#if AMB_ENABLE_REVERSE_SIGNAL_FROM_SAMF
    {
        uint8_t samf_active;
        uint32_t samf_age;
        __disable_irq();
        samf_active = g_reverse_samf_active;
        __enable_irq();
        samf_age = (g_reverse_samf_last_ms == 0u) ? (AMB_CAN_ACTIVE_TIMEOUT_MS + 1u) : elapsed_ms32(now, g_reverse_samf_last_ms);
        if (samf_age <= AMB_CAN_ACTIVE_TIMEOUT_MS && samf_active) return 1u;
    }
#endif
    return 0u;
}

uint8_t can_rx_is_reverse_handle_block_active(void)
{
    uint8_t active_now;
    uint32_t now;
    uint32_t age_active;
    uint32_t hold_age;

    __disable_irq();
    active_now = g_reverse_active;
    now = HAL_GetTick();
    __enable_irq();

    age_active = (g_reverse_last_rx_ms == 0u) ? (AMB_CAN_ACTIVE_TIMEOUT_MS + 1u)
                                              : elapsed_ms32(now, g_reverse_last_rx_ms);
    if (age_active <= AMB_CAN_ACTIVE_TIMEOUT_MS && active_now) {
        return 1u;
    }

    if (g_reverse_last_active_ms == 0u) return 0u;
    hold_age = elapsed_ms32(now, g_reverse_last_active_ms);
    return (hold_age < AMB_REVERSE_HANDLE_BLOCK_HOLD_MS) ? 1u : 0u;
}

uint8_t can_rx_is_reverse_active(void)
{
#if AMB_ENABLE_REVERSE_SCENE
    uint8_t active;
    uint32_t now;
    uint32_t age;

    __disable_irq();
    active = g_reverse_active;
    now = HAL_GetTick();
    __enable_irq();

    if (g_reverse_last_rx_ms == 0u) return 0u;
    age = elapsed_ms32(now, g_reverse_last_rx_ms);
    if (age > AMB_CAN_ACTIVE_TIMEOUT_MS) return 0u;
    return active ? 1u : 0u;
#else
    return 0u;
#endif
}
