#include "ambient_source_arbiter.h"

#include "ambient_rx.h"
#include "ambient_state_store.h"

#define AMB_MODERN_REQUEST_FRESH_MS 5000u
#define AMB_LEGACY_BANK_COUNT 3u
#define AMB_LEGACY_SLOT_COUNT 4u

typedef struct {
    uint8_t initialized;
    uint8_t active_bank;
    uint8_t slot_by_bank[AMB_LEGACY_BANK_COUNT];
} legacy_cycler_t;

/* Canonical legacy bank/slot -> modern wheel color_id mapping.
 * 0: Amber warm bank, 1: Neutral bank, 2: Polar bank. */
static const uint8_t k_legacy_bank_slot_to_color[AMB_LEGACY_BANK_COUNT][AMB_LEGACY_SLOT_COUNT] = {
    { 1u, 0u, 2u, 3u },   /* Amber: orange/red/yellow/light-yellow */
    { 11u, 10u, 6u, 5u }, /* Neutral: white/pink/sky-blue/light-green */
    { 8u, 7u, 6u, 9u },   /* Polar: deep-blue/blue/sky-blue/purple */
};

static legacy_cycler_t g_legacy_cycler = {0u, 0u, {0u, 0u, 0u}};

static uint8_t modern_request_is_fresh(uint32_t rq_ts, uint32_t now_ms)
{
    if (rq_ts == 0u || rq_ts > now_ms) return 0u;
    return ((now_ms - rq_ts) <= AMB_MODERN_REQUEST_FRESH_MS) ? 1u : 0u;
}

static uint8_t legacy_color_from_bank_cycle(uint8_t legacy_bank)
{
    uint8_t bank = (legacy_bank < AMB_LEGACY_BANK_COUNT) ? legacy_bank : 0u;
    uint8_t slot;

    if (!g_legacy_cycler.initialized) {
        g_legacy_cycler.initialized = 1u;
        g_legacy_cycler.active_bank = bank;
    } else if (g_legacy_cycler.active_bank != bank) {
        uint8_t next = (uint8_t)((g_legacy_cycler.slot_by_bank[bank] + 1u) % AMB_LEGACY_SLOT_COUNT);
        g_legacy_cycler.slot_by_bank[bank] = next;
        g_legacy_cycler.active_bank = bank;
    }

    slot = g_legacy_cycler.slot_by_bank[bank];
    return k_legacy_bank_slot_to_color[bank][slot];
}

void ambient_source_arbiter_init(void)
{
    g_legacy_cycler.initialized = 0u;
    g_legacy_cycler.active_bank = 0u;
    g_legacy_cycler.slot_by_bank[0] = 0u;
    g_legacy_cycler.slot_by_bank[1] = 0u;
    g_legacy_cycler.slot_by_bank[2] = 0u;
}

void ambient_source_arbiter_apply(const can_state_t *can_state, uint32_t now_ms)
{
    uint8_t color_rq = 0u;
    uint8_t brightness_rq = 0u;
    uint8_t effect_rq = 0u;
    uint32_t rq_ts = 0u;

    if (!can_state) return;

    /* Modern HU request (0x463) is authoritative while fresh. */
    if (can_rx_get_modern_request(&color_rq, &brightness_rq, &effect_rq, &rq_ts) &&
        modern_request_is_fresh(rq_ts, now_ms)) {
        ambient_state_store_update_from_modern_request(color_rq, brightness_rq, effect_rq, now_ms);
        return;
    }

    /* Legacy cluster ingress (0x325) fallback path via bank cycler. */
    if (can_rx_oem_received()) {
        uint8_t legacy_bank = (uint8_t)(can_state->oem_color % AMB_LEGACY_BANK_COUNT);
        uint8_t color_id = legacy_color_from_bank_cycle(legacy_bank);
        uint8_t br_raw = (uint8_t)(can_state->oem_brightness * 5.0f + 0.5f);
        if (br_raw > 5u) br_raw = 5u;
        ambient_state_store_update_from_modern_request(color_id, br_raw, 0u, now_ms);
    }
}

uint8_t ambient_source_arbiter_selftest(void)
{
    /* Returns 1 on pass, 0 on fail. */
    if (!modern_request_is_fresh(1000u, 1000u)) return 0u;
    if (!modern_request_is_fresh(1000u, 5999u)) return 0u;
    if (modern_request_is_fresh(1000u, 6001u)) return 0u;
    if (modern_request_is_fresh(0u, 6001u)) return 0u;
    if (modern_request_is_fresh(7000u, 6001u)) return 0u;

    ambient_source_arbiter_init();
    if (legacy_color_from_bank_cycle(0u) != 1u) return 0u;  /* Amber slot0 */
    if (legacy_color_from_bank_cycle(0u) != 1u) return 0u;  /* same bank, same slot */
    if (legacy_color_from_bank_cycle(1u) != 10u) return 0u; /* Neutral slot1 after switch */
    if (legacy_color_from_bank_cycle(2u) != 7u) return 0u;  /* Polar slot1 after switch */
    if (legacy_color_from_bank_cycle(0u) != 0u) return 0u;  /* Amber slot1 after return */

    return 1u;
}
