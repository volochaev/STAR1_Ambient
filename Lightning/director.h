#pragma once

#include "runtime_state.h"
#include "event_layer.h"
#include "led_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    director_runtime_t runtime;
    theme_id_t           pending_theme;
    uint8_t                 has_pending_theme;
    uint8_t                 welcome_scene_armed;
    uint32_t                last_fast_bank_switch_ms;
    base_scene_stage_t          last_stage;
} director_t;

void director_init(director_t *director);
void director_update(director_t *director,
                         led_runtime_strip_t *ws,
                         base_scene_t *pl,
                         const runtime_inputs_t *inputs);

#ifdef __cplusplus
}
#endif
