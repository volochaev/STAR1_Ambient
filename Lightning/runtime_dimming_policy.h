#pragma once

#include <stdint.h>

#include "base_scene.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Intermediate policy output for current runtime tick.
 */
typedef struct {
    /** When set, keep dimming value stable for this frame. */
    uint8_t hold_dimming;
} runtime_dimming_policy_eval_t;

/**
 * @brief Evaluate dimming policy based on scene and runtime context.
 */
void runtime_dimming_policy_eval(const base_scene_t *pl,
                                 runtime_dimming_policy_eval_t *out);

#ifdef __cplusplus
}
#endif
