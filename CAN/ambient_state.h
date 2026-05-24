#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Snapshot of ambient state mirrored from CAN domain.
 */
typedef struct {
    /** OEM color selector (legacy encoding or mapped modern ID). */
    uint8_t oem_color;
    /** Brightness in normalized 0..1 domain. */
    float   oem_brightness;
    /** Night mode flag propagated to render/runtime pipeline. */
    uint8_t night_mode;
    /** Legacy bank selector kept for compatibility and migrations. */
    uint8_t bank_id;
} can_state_t;

/**
 * @brief Blind-spot monitoring event state.
 */
typedef struct {
    uint8_t left_active;
    uint8_t right_active;
    float left_level;
    float right_level;
} can_bsm_state_t;

/**
 * @brief Parking warning event state for left/right/rear channels.
 */
typedef struct {
    uint8_t left_active;
    uint8_t right_active;
    uint8_t rear_active;
    float left_level;
    float right_level;
    float rear_level;
} can_parking_warn_state_t;

#ifdef __cplusplus
}
#endif
