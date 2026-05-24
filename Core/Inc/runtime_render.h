#pragma once

#include <stdint.h>

#include "main.h"
#include "led_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Clears stored history for frame-slew limiter of a specific strip. */
void runtime_render_reset_slew_for_strip(led_runtime_strip_t *strip);
/** Push immediate black frame to all board strips. */
void runtime_render_push_black_frame(void);
/** Apply postprocess overlays/filters after base scene render. */
void runtime_render_postprocess_frame(uint32_t now);

#ifdef __cplusplus
}
#endif
