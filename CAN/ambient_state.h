#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t oem_color;
    float   oem_brightness;
    uint8_t night_mode;
    uint8_t bank_id;
    uint8_t theme_index;
    uint8_t last_oem_color;
    uint8_t oem_theme_indices[3];
} can_state_t;

typedef struct {
    uint8_t left_active;
    uint8_t right_active;
    float left_level;
    float right_level;
} can_bsm_state_t;

#ifdef __cplusplus
}
#endif
