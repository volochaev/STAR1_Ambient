/**
 * @file runtime_debug_hooks.c
 * @brief Lightweight debug overlays and RX pulse markers.
 */
#include "runtime_debug_hooks.h"

#include "ambient.h"

static volatile uint32_t g_dbg_bsm_rx_until_ms = 0u;
static volatile uint32_t g_dbg_non_oem_rx_until_ms = 0u;
static volatile uint32_t g_dbg_non_oem_rx_id = 0u;

/* Reset debug overlay timers and IDs. */
void runtime_debug_hooks_init(void)
{
    g_dbg_bsm_rx_until_ms = 0u;
    g_dbg_non_oem_rx_until_ms = 0u;
    g_dbg_non_oem_rx_id = 0u;
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
