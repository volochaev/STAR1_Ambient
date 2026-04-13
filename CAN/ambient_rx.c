#include "ambient_rx.h"
#include "ambient_internal.h"

#include <string.h>

#include "ambient_power.h"
#include "ambient_role.h"
#include "ambient_theme.h"
#include "features.h"

static volatile uint8_t g_oem_packet_received = 0u;
static volatile uint8_t g_nsi_active = 0u;
static volatile uint32_t g_lgtsens_last_rx_ms = 0u;
static volatile uint8_t g_hvac_prev_front_valid = 0u;
static volatile uint8_t g_hvac_prev_rear_valid = 0u;
static volatile uint8_t g_hvac_prev_fl = 0u;
static volatile uint8_t g_hvac_prev_fr = 0u;
static volatile uint8_t g_hvac_prev_rl = 0u;
static volatile uint8_t g_hvac_prev_rr = 0u;
static volatile uint32_t g_hvac_last_event_left_ms = 0u;
static volatile uint32_t g_hvac_last_event_right_ms = 0u;
static volatile int8_t g_hvac_trend_left_pending = 0;
static volatile int8_t g_hvac_trend_right_pending = 0;
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

static uint32_t elapsed_ms32(uint32_t now, uint32_t then)
{
    return (now >= then) ? (now - then) : ((UINT32_MAX - then) + now + 1u);
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
    uint32_t now;

    if (delta == 0) return;
    now = HAL_GetTick();

    __disable_irq();
    if (right_side) {
        if (elapsed_ms32(now, g_hvac_last_event_right_ms) >= AMB_HVAC_TEMP_EVENT_COOLDOWN_MS) {
            g_hvac_trend_right_pending = (delta > 0) ? 1 : -1;
            g_hvac_last_event_right_ms = now;
        }
    } else {
        if (elapsed_ms32(now, g_hvac_last_event_left_ms) >= AMB_HVAC_TEMP_EVENT_COOLDOWN_MS) {
            g_hvac_trend_left_pending = (delta > 0) ? 1 : -1;
            g_hvac_last_event_left_ms = now;
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
    uint8_t ns_illdur_vld;
    uint8_t ns_illdur;
    uint8_t nsi_active;

    if (len < 4u) return;

    surr_ill_rq = d[3] & 0x03u;
    ns_illdur_vld = (d[0] >> 3) & 0x01u;
    ns_illdur = d[2];
    nsi_active = ((surr_ill_rq == 1u) || (ns_illdur_vld && (ns_illdur > 0u))) ? 1u : 0u;
    g_nsi_active = nsi_active;

    /* Fallback: if dedicated light-sensor frame is stale, derive night mode from NSI request. */
    if (g_lgtsens_last_rx_ms == 0u || elapsed_ms32(HAL_GetTick(), g_lgtsens_last_rx_ms) > AMB_CAN_ACTIVE_TIMEOUT_MS) {
        __disable_irq();
        g_can_state.night_mode = nsi_active;
        __enable_irq();
    }
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
    night = (night_1 || night_2) ? 1u : 0u;

    g_lgtsens_last_rx_ms = HAL_GetTick();
    __disable_irq();
    g_can_state.night_mode = night;
    __enable_irq();
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
    uint8_t raw;
    float level;

    if (len < 4u) return;
    raw = d[3];
    if (!hvac_temp_raw_valid(raw)) return;
    level = (float)raw / 255.0f;

    __disable_irq();
    g_hvac_fan_level = g_hvac_fan_level * 0.82f + level * 0.18f;
    bank_character_sample();
    __enable_irq();
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
    uint8_t l_brt;
    uint8_t r_brt;
    uint8_t l_rq;
    uint8_t r_rq;
    uint8_t any_rq = 0u;

    if (len < 5u) return;

    l_brt = d[2];
    r_brt = d[3];
    l_rq = (uint8_t)((d[4] >> 6) & 0x03u);
    r_rq = (uint8_t)((d[4] >> 4) & 0x03u);
    any_rq = (uint8_t)(d[5] & 0x03u);

    if (l_brt == 0u && r_brt == 0u && len >= 2u) {
        l_brt = d[0];
        r_brt = d[1];
    }

    g_bsm.left_level = (float)l_brt / 255.0f;
    g_bsm.right_level = (float)r_brt / 255.0f;
    g_bsm.left_active = ((l_brt > 8u) || (l_rq != 0u)) ? 1u : 0u;
    g_bsm.right_active = ((r_brt > 8u) || (r_rq != 0u)) ? 1u : 0u;

    if ((g_bsm.left_active == 0u) && (g_bsm.right_active == 0u) && (any_rq != 0u)) {
        g_bsm.left_active = 1u;
        g_bsm.right_active = 1u;
        if (g_bsm.left_level < 0.55f) g_bsm.left_level = 0.55f;
        if (g_bsm.right_level < 0.55f) g_bsm.right_level = 0.55f;
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
    uint8_t raw_col;
    uint8_t col;

    if (len < 4u) return;

    br_raw = d[0];
    raw_col = (d[3] >> 2) & 0x03u;

    switch (raw_col) {
    case 0: col = OEM_COLOR_AMBER; break;
    case 1: col = OEM_COLOR_WHITE; break;
    case 2: col = OEM_COLOR_BLUE; break;
    default: return;
    }

    if (br_raw > 5u) br_raw = 5u;

    g_oem_packet_received = 1u;
#if AMB_ENABLE_SLEEP_MODE
    can_power_note_can_activity();
#endif

    __disable_irq();
    g_can_state.oem_color = col;
    g_can_state.oem_brightness = (float)br_raw / 5.0f;
    bank_character_sample();
    __enable_irq();
}

static void handle_master(const uint8_t *d, uint8_t len)
{
    uint8_t flags;
    uint8_t bank_id;
    uint8_t theme_id;
    uint8_t oem_col;
    uint8_t br_raw;
    uint8_t profile_raw;
    uint8_t old_bank;
    uint8_t old_theme;

    if (len < 4u) return;

    flags = d[0] & 0x0Fu;
    bank_id = d[1];
    theme_id = d[2];
    oem_col = d[3];
    br_raw = (len > 4u) ? d[4] : 5u;
    profile_raw = (len > 5u) ? d[5] : (uint8_t)AMB_DEFAULT_MOTION_PROFILE;

    if (bank_id > 2u || oem_col > 2u) return;
    if (br_raw > 5u) br_raw = 5u;

    g_oem_packet_received = 1u;
#if AMB_ENABLE_SLEEP_MODE
    can_power_note_can_activity();
#endif

    __disable_irq();
    old_bank = g_can_state.bank_id;
    old_theme = g_can_state.theme_index;
    g_can_state.night_mode = (flags & MASTER_FLAG_NIGHT) ? 1u : 0u;
    g_can_state.bank_id = bank_id;
    g_can_state.theme_index = theme_id;
    g_can_state.oem_color = oem_col;
    g_can_state.oem_brightness = (float)br_raw / 5.0f;
    set_motion_profile((motion_profile_t)profile_raw);
    g_can_state.oem_theme_indices[oem_col] = theme_id % 255u;
    bank_character_sample();
    __enable_irq();

    if (old_bank != bank_id || old_theme != theme_id) {
        can_theme_state_note_state_change(HAL_GetTick());
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
    uint8_t bank_id;
    uint8_t theme_id;

    if (len < 2u) return;

    bank_id = d[0] & 0x0Fu;
    theme_id = d[1];

    __disable_irq();
    g_can_state.bank_id = bank_id;
    g_can_state.theme_index = theme_id;
    g_can_state.oem_theme_indices[bank_id] = theme_id % 255u;
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
    g_nsi_active = 0u;
    g_lgtsens_last_rx_ms = 0u;
    g_hvac_prev_front_valid = 0u;
    g_hvac_prev_rear_valid = 0u;
    g_hvac_prev_fl = 0u;
    g_hvac_prev_fr = 0u;
    g_hvac_prev_rl = 0u;
    g_hvac_prev_rr = 0u;
    g_hvac_last_event_left_ms = 0u;
    g_hvac_last_event_right_ms = 0u;
    g_hvac_trend_left_pending = 0;
    g_hvac_trend_right_pending = 0;
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
}

void can_rx_process(uint32_t id, uint8_t *data, uint8_t len)
{
    int8_t role_snapshot = can_role_get();
    uint8_t role_now = (role_snapshot < 0) ? 0xFFu : (uint8_t)role_snapshot;
    uint8_t master_now = can_role_is_master();

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
        if (master_now || role_now == 0xFFu) {
            handle_oem(data, len);
        }
        return;
    }
    if (id == CAN_HVAC_FRONT_ID || id == CAN_HVAC_REAR_ID) {
        handle_hvac_temp(id, data, len);
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
    }
}

uint8_t can_rx_oem_received(void)
{
    return g_oem_packet_received;
}

void can_rx_reset_oem_received(void)
{
    __disable_irq();
    g_oem_packet_received = 0u;
    g_nsi_active = 0u;
    g_lgtsens_last_rx_ms = 0u;
    g_hvac_prev_front_valid = 0u;
    g_hvac_prev_rear_valid = 0u;
    g_hvac_prev_fl = 0u;
    g_hvac_prev_fr = 0u;
    g_hvac_prev_rl = 0u;
    g_hvac_prev_rr = 0u;
    g_hvac_last_event_left_ms = 0u;
    g_hvac_last_event_right_ms = 0u;
    g_hvac_trend_left_pending = 0;
    g_hvac_trend_right_pending = 0;
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
    return g_nsi_active;
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
