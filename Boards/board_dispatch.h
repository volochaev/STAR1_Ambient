#pragma once

#include "ambient.h"
#include "led_runtime.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Return main strip abstraction for currently selected board profile. */
led_runtime_strip_t *board_dispatch_get_main_strip(void);
/** Initialize LED drivers for selected board profile. */
void board_dispatch_led_init(void);
/** Render current frame for selected board profile. */
void board_dispatch_led_render_all(void);
/** Handle DMA transfer-complete IRQ for selected board profile. */
void board_dispatch_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
