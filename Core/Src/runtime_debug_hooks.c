/**
 * @file runtime_debug_hooks.c
 * @brief Lightweight debug overlays and RX pulse markers.
 */
#include "runtime_debug_hooks.h"

#include "ambient.h"
#include <string.h>

static volatile uint32_t g_dbg_bsm_rx_until_ms = 0u;
static volatile uint32_t g_dbg_non_oem_rx_until_ms = 0u;
static volatile uint32_t g_dbg_non_oem_rx_id = 0u;
static volatile uint8_t g_dbg_self_check_ok = 1u;
static runtime_diag_snapshot_t g_runtime_diag;

/* Reset debug overlay timers and IDs. */
void runtime_debug_hooks_init(void)
{
    g_dbg_bsm_rx_until_ms = 0u;
    g_dbg_non_oem_rx_until_ms = 0u;
    g_dbg_non_oem_rx_id = 0u;
    g_dbg_self_check_ok = 1u;
    runtime_debug_hooks_runtime_diag_reset();
}

/* Record CAN RX activity for temporary overlay indication. */
void runtime_debug_hooks_note_can_rx(uint32_t can_id, uint32_t now_ms)
{
#if AMB_DEBUG_BSM_RX_PULSE
    if ((can_id == CAN_BSM_ID) || ((can_id & 0x7FFu) == CAN_BSM_ID)) {
        g_dbg_bsm_rx_until_ms = now_ms + 260u;
    } else if (can_id != CAN_OEM_ID) {
        g_dbg_non_oem_rx_id = can_id;
        g_dbg_non_oem_rx_until_ms = now_ms + 180u;
    }
#else
    (void)can_id;
    (void)now_ms;
#endif
}

/* Return active debug overlay color, if any. */
uint8_t runtime_debug_hooks_get_strip_overlay(uint32_t now_ms,
                                              uint8_t *r,
                                              uint8_t *g,
                                              uint8_t *b)
{
#if AMB_DEBUG_BSM_RX_PULSE
    can_modern_diag_t modern_diag;

    if (!r || !g || !b) return 0u;
    if (!g_dbg_self_check_ok) {
        *r = 255u; *g = 0u; *b = 255u; /* hard failure marker */
        return 1u;
    }

    if (now_ms < g_dbg_bsm_rx_until_ms) {
        *r = 255u; *g = 24u; *b = 0u;
        return 1u;
    }
    can_ambient_get_modern_diag(&modern_diag);
    if (modern_diag.conflict_active) {
        *r = 255u; *g = 180u; *b = 0u;
        return 1u;
    }
    if (now_ms < g_dbg_non_oem_rx_until_ms) {
        *r = 96u; *g = 0u; *b = 255u;
        if (g_dbg_non_oem_rx_id == CAN_BSM_ID) {
            *r = 255u; *g = 0u; *b = 0u;
        }
        return 1u;
    }
#else
    (void)now_ms; (void)r; (void)g; (void)b;
#endif
    return 0u;
}

void runtime_debug_hooks_runtime_diag_reset(void)
{
    __disable_irq();
    memset((void *)&g_runtime_diag, 0, sizeof(g_runtime_diag));
    __enable_irq();
}

void runtime_debug_hooks_runtime_diag_get(runtime_diag_snapshot_t *out)
{
    if (!out) return;
    __disable_irq();
    *out = g_runtime_diag;
    __enable_irq();
    runtime_event_queue_get_diag(&out->queue);
}

void runtime_debug_hooks_diag_note_zone_submit(zone_id_t zone,
                                               zone_role_id_t role,
                                               runtime_diag_zone_submit_result_t result,
                                               uint8_t alpha_clamped,
                                               uint8_t safety_fallback,
                                               uint8_t safety_floor_applied)
{
    (void)role;
    if (zone >= ZONE_MAX) return;
    __disable_irq();
    if (result == RUNTIME_DIAG_SUBMIT_OK) {
        g_runtime_diag.zone_roles.submitted++;
    } else if (result == RUNTIME_DIAG_SUBMIT_REJECT_ROLE) {
        g_runtime_diag.zone_roles.rejected_by_role++;
    } else if (result == RUNTIME_DIAG_SUBMIT_REJECT_GROUP) {
        g_runtime_diag.zone_roles.rejected_by_group++;
    }
    if (alpha_clamped) g_runtime_diag.zone_roles.alpha_clamped++;
    if (safety_fallback) g_runtime_diag.zone_roles.safety_fallbacks++;
    if (safety_floor_applied) g_runtime_diag.zone_roles.safety_floor_applied++;
    __enable_irq();
}

void runtime_debug_hooks_diag_set_zone_frame(zone_id_t zone,
                                             uint8_t active_role_mask,
                                             const uint8_t *effective_role_order,
                                             uint8_t order_len)
{
    uint8_t i;
    if (zone >= ZONE_MAX || !effective_role_order) return;
    if (order_len > (uint8_t)ZONE_ROLE_MAX) order_len = (uint8_t)ZONE_ROLE_MAX;
    __disable_irq();
    g_runtime_diag.zone_roles.active_role_mask[zone] = active_role_mask;
    for (i = 0u; i < (uint8_t)ZONE_ROLE_MAX; ++i) {
        g_runtime_diag.zone_roles.effective_role_order[zone][i] = (i < order_len) ? effective_role_order[i] : 0xFFu;
    }
    __enable_irq();
}

void runtime_debug_hooks_diag_set_preset(const scene_preset_t *preset)
{
    if (!preset) return;
    __disable_irq();
    g_runtime_diag.preset = *preset;
    g_runtime_diag.preset_valid = 1u;
    __enable_irq();
}

void runtime_debug_hooks_diag_set_world(uint8_t active,
                                        uint8_t world_id,
                                        uint8_t generated,
                                        uint8_t dominant_r,
                                        uint8_t dominant_g,
                                        uint8_t dominant_b,
                                        float selection_distance)
{
    __disable_irq();
    g_runtime_diag.world_mode = active ? 1u : 0u;
    g_runtime_diag.world_id = world_id;
    g_runtime_diag.world_generated = generated ? 1u : 0u;
    g_runtime_diag.world_dominant_r = dominant_r;
    g_runtime_diag.world_dominant_g = dominant_g;
    g_runtime_diag.world_dominant_b = dominant_b;
    g_runtime_diag.world_selection_distance = selection_distance;
    __enable_irq();
}

void runtime_debug_hooks_set_self_check_ok(uint8_t ok)
{
    __disable_irq();
    g_dbg_self_check_ok = ok ? 1u : 0u;
    __enable_irq();
}

void runtime_debug_hooks_note_stop_wake(uint8_t source, uint32_t now_ms)
{
    __disable_irq();
    g_runtime_diag.stop_last_wake_source = source;
    g_runtime_diag.stop_last_wake_ms = now_ms;
    __enable_irq();
}

void runtime_debug_hooks_note_stop_rtc_cycle(uint32_t now_ms)
{
    __disable_irq();
    g_runtime_diag.stop_last_wake_source = 3u; /* 1=CAN RX EXTI, 2=TRANSCEIVER EXTI, 3=RTC periodic */
    g_runtime_diag.stop_last_wake_ms = now_ms;
    g_runtime_diag.stop_rtc_wakeup_count++;
    __enable_irq();
}
