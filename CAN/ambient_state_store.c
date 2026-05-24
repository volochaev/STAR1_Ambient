/**
 * @file ambient_state_store.c
 * @brief Atomic storage for unified ambient state snapshot.
 */
#include "ambient_state_store.h"
#include "main.h"
#include "ambient_persist.h"
#include "flash_storage.h"

#include <string.h>

static volatile ambient_state_snapshot_t g_ambient_state;

/* Clamp helper for bounded CAN fields. */
static uint8_t clamp_u8(uint8_t v, uint8_t max_v)
{
    return (v > max_v) ? max_v : v;
}

/* Apply HU modern request and mark persist-dirty on effective change. */
void ambient_state_store_update_from_modern_request(uint8_t color_id,
                                                    uint8_t brightness_raw,
                                                    uint8_t effect_id,
                                                    uint32_t now_ms)
{
    ambient_state_snapshot_t next_state;
    uint8_t changed = 0u;

    __disable_irq();
    next_state = g_ambient_state;
    next_state.color_id = clamp_u8(color_id, 12u);
    next_state.brightness_raw = clamp_u8(brightness_raw, 5u);
    next_state.effect_id = clamp_u8(effect_id, 3u);
    next_state.brightness_norm = (float)next_state.brightness_raw / 5.0f;
    next_state.valid = 1u;
    next_state.timestamp_ms = now_ms;
    changed = (uint8_t)((next_state.color_id != g_ambient_state.color_id) ||
                        (next_state.brightness_raw != g_ambient_state.brightness_raw) ||
                        (next_state.effect_id != g_ambient_state.effect_id) ||
                        (g_ambient_state.valid == 0u));
    next_state.seq++;
    g_ambient_state = next_state;
    __enable_irq();

    if (changed) {
        can_persist_state_note_state_change(now_ms);
    }
}

/* Initialize defaults and restore persisted modern snapshot when available. */
void ambient_state_store_init(void)
{
    ambient_state_snapshot_t init_state;

    memset(&init_state, 0, sizeof(init_state));
    init_state.color_id = 0u;
    init_state.brightness_raw = 5u;
    init_state.effect_id = 0u;
    init_state.brightness_norm = 1.0f;
    init_state.valid = 0u;
    init_state.seq = 0u;
    init_state.timestamp_ms = 0u;

    __disable_irq();
    memcpy((void *)&g_ambient_state, &init_state, sizeof(init_state));
    __enable_irq();

    {
        flash_modern_state_t persisted;
        if (flash_storage_load_extended(NULL, &persisted) == 0 && persisted.valid) {
            ambient_state_store_restore_from_persisted(persisted.color_id,
                                                       persisted.brightness_raw,
                                                       persisted.effect_id,
                                                       persisted.valid,
                                                       persisted.timestamp_ms);
        }
    }
}

/* Legacy bridge: convert legacy CAN state into unified modern snapshot. */
void ambient_state_store_update_from_legacy_can(const can_state_t *can_state, uint32_t now_ms)
{
    ambient_state_snapshot_t next_state;
    uint8_t br_raw;

    if (!can_state) return;

    br_raw = (uint8_t)(can_state->oem_brightness * 5.0f + 0.5f);
    br_raw = clamp_u8(br_raw, 5u);

    __disable_irq();
    next_state = g_ambient_state;
    next_state.color_id = clamp_u8(can_state->oem_color, 2u);
    next_state.brightness_raw = br_raw;
    next_state.effect_id = 0u;
    next_state.brightness_norm = can_state->oem_brightness;
    if (next_state.brightness_norm < 0.0f) next_state.brightness_norm = 0.0f;
    if (next_state.brightness_norm > 1.0f) next_state.brightness_norm = 1.0f;
    next_state.valid = 1u;
    next_state.timestamp_ms = now_ms;
    next_state.seq++;
    g_ambient_state = next_state;
    __enable_irq();
}

/* Restore state from flash payload without triggering extra side effects. */
void ambient_state_store_restore_from_persisted(uint8_t color_id,
                                                uint8_t brightness_raw,
                                                uint8_t effect_id,
                                                uint8_t valid,
                                                uint32_t timestamp_ms)
{
    ambient_state_snapshot_t next_state;

    __disable_irq();
    next_state = g_ambient_state;
    next_state.color_id = clamp_u8(color_id, 12u);
    next_state.brightness_raw = clamp_u8(brightness_raw, 5u);
    next_state.effect_id = clamp_u8(effect_id, 3u);
    next_state.brightness_norm = (float)next_state.brightness_raw / 5.0f;
    next_state.valid = valid ? 1u : 0u;
    next_state.timestamp_ms = timestamp_ms;
    next_state.seq++;
    g_ambient_state = next_state;
    __enable_irq();
}

/* Read consistent state snapshot for runtime/render layers. */
void ambient_state_store_get_snapshot(ambient_state_snapshot_t *out)
{
    if (!out) return;
    __disable_irq();
    *out = g_ambient_state;
    __enable_irq();
}
