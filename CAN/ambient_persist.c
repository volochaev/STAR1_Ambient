/**
 * @file ambient_persist.c
 * @brief Deferred flash persistence for ambient state.
 */
#include "ambient_persist.h"

#include <string.h>

#include "ambient_config.h"
#include "flash_storage.h"
#include "ambient_state_store.h"

static volatile uint32_t g_last_settings_change_ms = 0u;
static volatile uint8_t g_settings_changed = 0u;

static can_state_t g_last_saved_state = {0};
static uint8_t g_last_saved_valid = 0u;
static flash_modern_state_t g_last_saved_modern_state = {0};

/* Wrap-safe elapsed milliseconds helper. */
static uint32_t elapsed_ms32(uint32_t now, uint32_t then)
{
    return (now >= then) ? (now - then) : ((UINT32_MAX - then) + now + 1u);
}

/* Copy volatile CAN state atomically for persistence path. */
static void copy_can_state_atomic(const volatile can_state_t *src, can_state_t *out)
{
    if (!src || !out) return;
    __disable_irq();
    memcpy(out, (const void *)src, sizeof(*out));
    __enable_irq();
}

/* Detect if legacy state fields differ from last committed flash state. */
static uint8_t flash_state_relevant_changed(const can_state_t *s)
{
    if (!s) return 0u;
    if (!g_last_saved_valid) return 1u;

    if (s->bank_id != g_last_saved_state.bank_id ||
        s->oem_color != g_last_saved_state.oem_color ||
        s->night_mode != g_last_saved_state.night_mode ||
        s->oem_brightness != g_last_saved_state.oem_brightness) {
        return 1u;
    }
    return 0u;
}

/* Detect if modern unified fields differ from last committed flash state. */
static uint8_t flash_modern_state_relevant_changed(const flash_modern_state_t *s)
{
    if (!s) return 0u;
    if (!g_last_saved_valid) return 1u;

    if (s->valid != g_last_saved_modern_state.valid ||
        s->color_id != g_last_saved_modern_state.color_id ||
        s->brightness_raw != g_last_saved_modern_state.brightness_raw ||
        s->effect_id != g_last_saved_modern_state.effect_id) {
        return 1u;
    }
    return 0u;
}

/* Reset persistence scheduler and cached last-saved snapshots. */
void can_persist_state_init(void)
{
    g_last_settings_change_ms = 0u;
    g_settings_changed = 0u;
    memset(&g_last_saved_state, 0, sizeof(g_last_saved_state));
    memset(&g_last_saved_modern_state, 0, sizeof(g_last_saved_modern_state));
    g_last_saved_valid = 0u;
}

/* Seed last-saved caches after boot-time flash load. */
void can_persist_state_note_flash_loaded(const can_state_t *state)
{
    flash_modern_state_t modern_state;

    if (!state) return;
    memcpy(&g_last_saved_state, state, sizeof(g_last_saved_state));
    memset(&modern_state, 0, sizeof(modern_state));
    if (flash_storage_load_extended(NULL, &modern_state) == 0) {
        memcpy(&g_last_saved_modern_state, &modern_state, sizeof(g_last_saved_modern_state));
    } else {
        memset(&g_last_saved_modern_state, 0, sizeof(g_last_saved_modern_state));
    }
    g_last_saved_valid = 1u;
}

/* Mark persistence dirty and update debounce timestamp. */
void can_persist_state_note_state_change(uint32_t now_ms)
{
    g_settings_changed = 1u;
    g_last_settings_change_ms = now_ms;
}

/* Save to flash after debounce timeout when master role is active. */
void can_persist_state_process_deferred_save(volatile can_state_t *state,
                                           uint8_t is_master,
                                           uint32_t now_ms)
{
    can_state_t state_to_save;
    ambient_state_snapshot_t modern_snapshot;
    flash_modern_state_t modern_to_save;
    uint32_t time_since_change;
    uint8_t saved_ok = 0u;

    if (!state) return;
    if (!g_settings_changed) return;

    time_since_change = elapsed_ms32(now_ms, g_last_settings_change_ms);
    if (time_since_change < AMB_FLASH_SAVE_DELAY_MS) return;

    if (is_master) {
        copy_can_state_atomic(state, &state_to_save);
        ambient_state_store_get_snapshot(&modern_snapshot);
        modern_to_save.valid = modern_snapshot.valid;
        modern_to_save.color_id = modern_snapshot.color_id;
        modern_to_save.brightness_raw = modern_snapshot.brightness_raw;
        modern_to_save.effect_id = modern_snapshot.effect_id;
        modern_to_save.timestamp_ms = modern_snapshot.timestamp_ms;

        if (flash_state_relevant_changed(&state_to_save) ||
            flash_modern_state_relevant_changed(&modern_to_save)) {
            if (flash_storage_save_extended(&state_to_save, &modern_to_save) == 0) {
                memcpy(&g_last_saved_state, &state_to_save, sizeof(g_last_saved_state));
                memcpy(&g_last_saved_modern_state, &modern_to_save, sizeof(g_last_saved_modern_state));
                g_last_saved_valid = 1u;
                saved_ok = 1u;
            }
        } else {
            saved_ok = 1u;
        }
    }

    if (saved_ok) {
        g_settings_changed = 0u;
    }
}
