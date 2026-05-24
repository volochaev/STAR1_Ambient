#pragma once

#include "runtime_state.h"
#include "event_layer.h"
#include "led_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    director_runtime_t runtime;
    uint8_t                 welcome_scene_armed;
    base_scene_stage_t          last_stage;
} director_t;

/** Initialize director runtime and transient orchestration flags. */
void director_init(director_t *director);
/** Update director state and apply stage-level orchestration decisions. */
void director_update(director_t *director,
                         led_runtime_strip_t *ws,
                         base_scene_t *pl,
                         const runtime_inputs_t *inputs);

#ifdef __cplusplus
}
#endif
