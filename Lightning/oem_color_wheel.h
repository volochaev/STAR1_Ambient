#pragma once

#include <stdint.h>

#include "scene_color_model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OEM_WHEEL_RED = 0u,
    OEM_WHEEL_ORANGE,
    OEM_WHEEL_YELLOW,
    OEM_WHEEL_LIGHT_YELLOW,
    OEM_WHEEL_GREEN,
    OEM_WHEEL_LIGHT_GREEN,
    OEM_WHEEL_SKY_BLUE,
    OEM_WHEEL_BLUE,
    OEM_WHEEL_DEEP_BLUE,
    OEM_WHEEL_PURPLE,
    OEM_WHEEL_PINK,
    OEM_WHEEL_WHITE,
    OEM_WHEEL_COUNT
} oem_wheel_color_id_t;

const scene_rgb8_t *oem_color_wheel_get(void);
uint8_t oem_color_wheel_count(void);
scene_rgb8_t oem_color_wheel_at(uint8_t idx);

#ifdef __cplusplus
}
#endif
