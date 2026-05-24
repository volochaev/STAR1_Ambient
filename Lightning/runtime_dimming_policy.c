/**
 * @file runtime_dimming_policy.c
 * @brief Runtime dimming hold policy evaluation.
 */
#include "runtime_dimming_policy.h"

/* Decide whether dimming should be held on current scene stage. */
void runtime_dimming_policy_eval(const base_scene_t *pl,
                                 runtime_dimming_policy_eval_t *out)
{
    runtime_dimming_policy_eval_t eval;

    if (!out) return;

    eval.hold_dimming = 0u;

    if (!pl) {
        *out = eval;
        return;
    }

    eval.hold_dimming = (uint8_t)(pl->stage == BASE_SCENE_OUTRO);
    *out = eval;
}
