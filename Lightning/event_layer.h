#pragma once

#include <stdint.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EVENT_SCENE_NONE = 0,
    EVENT_SCENE_DUAL_ACCENT,
    EVENT_SCENE_DIRECTION_LEFT,
    EVENT_SCENE_DIRECTION_RIGHT,
    EVENT_SCENE_TEMP_WARM,
    EVENT_SCENE_TEMP_COOL,
    EVENT_SCENE_WELCOME_ACCENT,
    EVENT_SCENE_MAX
} event_scene_id_t;

typedef enum {
    EVENT_PRIORITY_DECOR = 0u,
    EVENT_PRIORITY_INFO  = 1u,
    EVENT_PRIORITY_WARN  = 2u
} event_priority_t;

typedef struct {
    uint8_t  enabled;
    uint8_t  use_palette;
    uint8_t  r;
    uint8_t  g;
    uint8_t  b;
    uint8_t  pal_id;
    float    pal_u;
    float    gain;
} event_zone_override_t;

typedef struct {
    uint8_t                   active;
    event_scene_id_t      id;
    event_priority_t      priority;
    uint32_t                  start_ms;
    uint32_t                  hold_ms;
    uint16_t                  fade_in_ms;
    uint16_t                  fade_out_ms;
    uint8_t                   strip_trail;
    uint8_t                   strip_trail_reverse;
    uint8_t                   strip_trail_cycles;
    event_zone_override_t zone[ZONE_MAX];
} event_scene_t;

void event_layer_init(void);
void event_layer_cancel_all(void);
uint8_t event_layer_start(const event_scene_t *scene, uint32_t now_ms);
void event_layer_apply(uint32_t now_ms);
void event_layer_note_climate_memory(uint8_t warmer, uint32_t now_ms);
void event_scene_build_dual_accent(event_scene_t *scene,
                                       uint8_t cool_r, uint8_t cool_g, uint8_t cool_b,
                                       uint8_t warm_r, uint8_t warm_g, uint8_t warm_b,
                                       uint32_t hold_ms);
void event_scene_build_directional_cue(event_scene_t *scene,
                                           uint8_t left_side,
                                           uint8_t r, uint8_t g, uint8_t b,
                                           uint32_t hold_ms);
void event_scene_build_temp_delta(event_scene_t *scene,
                                  uint8_t warmer,
                                  uint8_t r, uint8_t g, uint8_t b,
                                  uint32_t hold_ms);

#ifdef __cplusplus
}
#endif
