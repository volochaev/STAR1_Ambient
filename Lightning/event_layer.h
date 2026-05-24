#pragma once

#include <stdint.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EVENT_SCENE_NONE = 0,          /* No active event scene. */
    EVENT_SCENE_DUAL_ACCENT,       /* Dual-tone welcome/accent blend. */
    EVENT_SCENE_DIRECTION_LEFT,    /* Left directional info cue. */
    EVENT_SCENE_DIRECTION_RIGHT,   /* Right directional info cue. */
    EVENT_SCENE_DOOR_OPEN,         /* Door open choreography scene. */
    EVENT_SCENE_TEMP_WARM,         /* HVAC temperature warm trend scene. */
    EVENT_SCENE_TEMP_COOL,         /* HVAC temperature cool trend scene. */
    EVENT_SCENE_WELCOME_ACCENT,    /* Unlock signature accent scene. */
    EVENT_SCENE_MAX
} event_scene_id_t;

typedef enum {
    EVENT_PRIORITY_DECOR = 0u,  /* Decorative/cosmetic overlays. */
    EVENT_PRIORITY_INFO  = 1u,  /* Informational overlays. */
    EVENT_PRIORITY_WARN  = 2u   /* Warning/safety overlays. */
} event_priority_t;

/** Solid RGB override description for one logical zone. */
typedef struct {
    uint8_t  enabled;
    uint8_t  r;
    uint8_t  g;
    uint8_t  b;
    float    gain;
} event_zone_override_t;

/** Active event scene descriptor with timing and zone overrides. */
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

/** Reset event-layer state and clear active overlays. */
void event_layer_init(void);
/** Cancel active event scene and temporary layers. */
void event_layer_cancel_all(void);
/** Start a new event scene if priority/timing policy allows it. */
uint8_t event_layer_start(const event_scene_t *scene, uint32_t now_ms);
/** Apply currently active event scene to frame buffer. */
void event_layer_apply(uint32_t now_ms);
/** Return 1 while HVAC temp scene is active or guarded. */
uint8_t event_layer_is_hvac_temp_active(uint32_t now_ms);
/** Start strip-local HVAC wave overlay. */
void event_layer_start_hvac_wave(uint8_t r, uint8_t g, uint8_t b, float gain, uint32_t now_ms);
/** Record short-lived climate-memory hint (warm/cool direction). */
void event_layer_note_climate_memory(uint8_t warmer, uint32_t now_ms);
/** Start/refresh contextual hold overlay with fixed color and gain. */
void event_layer_note_context_hold(uint8_t r, uint8_t g, uint8_t b, float gain, uint32_t hold_ms, uint32_t now_ms);
/** Build dual-accent decorative scene preset. */
void event_scene_build_dual_accent(event_scene_t *scene,
                                       uint8_t cool_r, uint8_t cool_g, uint8_t cool_b,
                                       uint8_t warm_r, uint8_t warm_g, uint8_t warm_b,
                                       uint32_t hold_ms);
/** Build directional cue scene preset. */
void event_scene_build_directional_cue(event_scene_t *scene,
                                           uint8_t left_side,
                                           uint8_t r, uint8_t g, uint8_t b,
                                           uint32_t hold_ms);
/** Build door-open choreography scene preset. */
void event_scene_build_door_open(event_scene_t *scene, uint8_t rear_side);
/** Build HVAC temperature delta scene preset. */
void event_scene_build_temp_delta(event_scene_t *scene,
                                  uint8_t warmer,
                                  uint8_t r, uint8_t g, uint8_t b,
                                  uint32_t hold_ms);
/** Build unlock signature scene preset. */
void event_scene_build_unlock_signature(event_scene_t *scene);
/** Build dual-accent scene from current runtime ambient palette. */
void event_scene_build_dual_accent_auto(event_scene_t *scene, uint32_t hold_ms);
/** Start HVAC wave using runtime ambient warm/cool accents. */
void event_layer_start_hvac_wave_auto(uint8_t warmer, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
