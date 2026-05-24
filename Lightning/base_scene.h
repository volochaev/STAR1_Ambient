#pragma once

#include "types.h"
#include "led_runtime.h"
#include "ambient_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float raw_min;
    float raw_max;
    float capped_min;
    float capped_max;
    float last_raw;
    float last_capped;
    uint32_t samples;
    uint32_t cap_low_hits;
    uint32_t cap_high_hits;
} motion_scale_diag_t;

/** Initialize base-scene runtime state. */
void base_scene_init(base_scene_t *sc);
/** Force scene state to ACTIVE stage. */
void base_scene_set_active(base_scene_t *sc);
/** Start intro animation stage. */
void base_scene_start_intro(led_runtime_strip_t *ws, base_scene_t *sc);
/** Start outro animation stage. */
void base_scene_start_outro(led_runtime_strip_t *ws, base_scene_t *sc);
/** Advance base-scene state machine by `delta_ms`. */
void base_scene_tick(led_runtime_strip_t *ws, base_scene_t *sc, uint32_t delta_ms);
/** Recompute effective brightness from scene and runtime dimming factors. */
void base_scene_refresh_brightness(base_scene_t *sc);
/** Return diagnostic snapshot for motion-scale clamp behavior. */
void base_scene_get_motion_scale_diag(motion_scale_diag_t *out);
/** Reset motion-scale diagnostics counters. */
void base_scene_reset_motion_scale_diag(void);

#ifdef __cplusplus
}
#endif
