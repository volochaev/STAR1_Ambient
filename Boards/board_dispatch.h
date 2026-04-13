#pragma once

#include "ambient.h"
#include "led_runtime.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

led_runtime_strip_t *board_dispatch_get_main_strip(void);
void board_dispatch_led_init(void);
void board_dispatch_led_render_all(void);
void board_dispatch_dma_tc(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
