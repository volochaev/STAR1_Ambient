#include "ambient_theme.h"

#include <string.h>

#include "features.h"
#include "flash_storage.h"
#include "themes.h"

static volatile uint32_t g_last_settings_change_ms = 0u;
static volatile uint8_t g_settings_changed = 0u;
static uint8_t g_oem_indices_dirty = 0u;

static can_state_t g_last_saved_state = {0};
static uint8_t g_last_saved_valid = 0u;

static uint32_t elapsed_ms32(uint32_t now, uint32_t then)
{
    return (now >= then) ? (now - then) : ((UINT32_MAX - then) + now + 1u);
}

static void copy_can_state_atomic(const volatile can_state_t *src, can_state_t *out)
{
    if (!src || !out) return;
    __disable_irq();
    memcpy(out, (const void *)src, sizeof(*out));
    __enable_irq();
}

static uint8_t flash_state_relevant_changed(const can_state_t *s)
{
    if (!s) return 0u;
    if (!g_last_saved_valid) return 1u;

    if (s->bank_id != g_last_saved_state.bank_id ||
        s->theme_index != g_last_saved_state.theme_index ||
        s->last_oem_color != g_last_saved_state.last_oem_color ||
        s->oem_theme_indices[0] != g_last_saved_state.oem_theme_indices[0] ||
        s->oem_theme_indices[1] != g_last_saved_state.oem_theme_indices[1] ||
        s->oem_theme_indices[2] != g_last_saved_state.oem_theme_indices[2]) {
        return 1u;
    }
    return 0u;
}

void can_theme_state_init(void)
{
    g_last_settings_change_ms = 0u;
    g_settings_changed = 0u;
    g_oem_indices_dirty = 0u;
    memset(&g_last_saved_state, 0, sizeof(g_last_saved_state));
    g_last_saved_valid = 0u;
}

void can_theme_state_note_flash_loaded(const can_state_t *state)
{
    if (!state) return;
    memcpy(&g_last_saved_state, state, sizeof(g_last_saved_state));
    g_last_saved_valid = 1u;
}

void can_theme_state_note_state_change(uint32_t now_ms)
{
    g_settings_changed = 1u;
    g_last_settings_change_ms = now_ms;
}

theme_id_t can_theme_state_resolve(volatile can_state_t *state, uint32_t now_ms)
{
    uint8_t oem_color;
    uint8_t last_oem_color;
    uint8_t oem_theme_indices[3];
    const theme_bank_t *bank;
    static uint8_t last_bank = 0xFFu;
    static uint8_t last_theme_idx = 0xFFu;

    if (!state) {
        return theme_default_for_oem(OEM_COLOR_AMBER);
    }

    __disable_irq();
    oem_color = state->oem_color;
    last_oem_color = state->last_oem_color;
    memcpy(oem_theme_indices, (const void *)state->oem_theme_indices, sizeof(oem_theme_indices));
    __enable_irq();

    if (oem_color >= OEM_COLOR_MAX) {
        return theme_default_for_oem(OEM_COLOR_AMBER);
    }

    bank = theme_get_bank((oem_color_id_t)oem_color);
    if (!bank || !bank->themes || bank->count == 0u) {
        return theme_default_for_oem((oem_color_id_t)oem_color);
    }

    if (last_oem_color != oem_color) {
        uint8_t next_idx = (uint8_t)(oem_theme_indices[oem_color] + 1u);
        if (next_idx >= bank->count) {
            next_idx = 0u;
        }

        __disable_irq();
        state->oem_theme_indices[oem_color] = next_idx;
        state->last_oem_color = oem_color;
        __enable_irq();

        oem_theme_indices[oem_color] = next_idx;
        g_oem_indices_dirty = 1u;
    }

    __disable_irq();
    state->bank_id = oem_color;
    state->theme_index = state->oem_theme_indices[oem_color];
    __enable_irq();

    if (oem_color != last_bank || oem_theme_indices[oem_color] != last_theme_idx) {
        can_theme_state_note_state_change(now_ms);
        last_bank = oem_color;
        last_theme_idx = oem_theme_indices[oem_color];
    }

    return bank->themes[oem_theme_indices[oem_color]];
}

void can_theme_state_process_deferred_save(volatile can_state_t *state,
                                           uint8_t is_master,
                                           uint32_t now_ms)
{
    can_state_t state_to_save;
    uint32_t time_since_change;
    uint8_t saved_ok = 0u;

    if (!state) return;
    if (!(g_settings_changed || g_oem_indices_dirty)) return;

    time_since_change = elapsed_ms32(now_ms, g_last_settings_change_ms);
    if (g_oem_indices_dirty && !g_settings_changed) {
        g_last_settings_change_ms = now_ms;
        g_settings_changed = 1u;
        g_oem_indices_dirty = 0u;
    }
    if (time_since_change < AMB_FLASH_SAVE_DELAY_MS) return;

    if (is_master) {
        copy_can_state_atomic(state, &state_to_save);

        if (flash_state_relevant_changed(&state_to_save)) {
            if (flash_storage_save(&state_to_save) == 0) {
                memcpy(&g_last_saved_state, &state_to_save, sizeof(g_last_saved_state));
                g_last_saved_valid = 1u;
                saved_ok = 1u;
            }
        } else {
            saved_ok = 1u;
        }
    }

    if (saved_ok) {
        g_settings_changed = 0u;
        g_oem_indices_dirty = 0u;
    }
}
