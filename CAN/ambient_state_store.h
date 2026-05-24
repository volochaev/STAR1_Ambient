#pragma once

#include <stdint.h>

#include "ambient_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t color_id;
    uint8_t brightness_raw;
    uint8_t effect_id;
    float brightness_norm;
    uint8_t valid;
    uint32_t seq;
    uint32_t timestamp_ms;
} ambient_state_snapshot_t;

/** Initialize normalized ambient state and restore persisted values. */
void ambient_state_store_init(void);
/** Update normalized state from OEM-compatible CAN snapshot. */
void ambient_state_store_update_from_legacy_can(const can_state_t *can_state, uint32_t now_ms);
/** Update normalized state from modern HU request values. */
void ambient_state_store_update_from_modern_request(uint8_t color_id,
                                                    uint8_t brightness_raw,
                                                    uint8_t effect_id,
                                                    uint32_t now_ms);
/** Restore state directly from persistence payload. */
void ambient_state_store_restore_from_persisted(uint8_t color_id,
                                                uint8_t brightness_raw,
                                                uint8_t effect_id,
                                                uint8_t valid,
                                                uint32_t timestamp_ms);
/** Return atomic snapshot of current normalized ambient state. */
void ambient_state_store_get_snapshot(ambient_state_snapshot_t *out);

#ifdef __cplusplus
}
#endif
