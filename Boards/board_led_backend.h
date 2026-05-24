#pragma once

#include <stdint.h>

#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ws2812_t *ws;
    uint32_t tim_channel;
    uint8_t *framebuffer;
    uint32_t *dma_buf;
    uint16_t led_count;
    uint8_t color_order;
    uint8_t render_enabled;
} board_led_line_t;

/** Initialize an array of board LED lines with common backend policy. */
void board_led_backend_init_lines(TIM_HandleTypeDef *htim,
                                  const board_led_line_t *lines,
                                  uint8_t line_count,
                                  uint8_t power_off_after_init);
/** Render all enabled LED lines from provided line table. */
void board_led_backend_render_lines(const board_led_line_t *lines, uint8_t line_count);
/** Dispatch DMA transfer-complete callback to matching active line(s). */
void board_led_backend_dma_tc_lines(const board_led_line_t *lines,
                                    uint8_t line_count,
                                    TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
