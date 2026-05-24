/**
 * @file board_led_backend.c
 * @brief Shared backend helpers for board LED line init/render/DMA TC.
 */
#include "board_led_backend.h"

#include "led_runtime.h"

/* Initialize all configured LED lines for selected board profile. */
void board_led_backend_init_lines(TIM_HandleTypeDef *htim,
                                  const board_led_line_t *lines,
                                  uint8_t line_count,
                                  uint8_t power_off_after_init)
{
    uint8_t i;

    if (!htim || !lines || line_count == 0u) return;

    for (i = 0u; i < line_count; ++i) {
        const board_led_line_t *line = &lines[i];
        if (!line->ws) continue;
        ws_init(line->ws,
                htim,
                line->tim_channel,
                line->framebuffer,
                line->dma_buf,
                line->led_count);
        ws_set_color_order(line->ws, line->color_order);
    }

    if (power_off_after_init) {
        led_runtime_power_set(0u);
    }
}

/* Render all board lines that are marked as render-enabled. */
void board_led_backend_render_lines(const board_led_line_t *lines, uint8_t line_count)
{
    uint8_t i;

    if (!lines || line_count == 0u) return;

    for (i = 0u; i < line_count; ++i) {
        const board_led_line_t *line = &lines[i];
        if (!line->ws || !line->render_enabled) continue;
        ws_render(line->ws);
    }
}

/* Forward DMA transfer-complete callback to all board LED lines. */
void board_led_backend_dma_tc_lines(const board_led_line_t *lines,
                                    uint8_t line_count,
                                    TIM_HandleTypeDef *htim)
{
    uint8_t i;

    if (!lines || !htim || line_count == 0u) return;

    for (i = 0u; i < line_count; ++i) {
        const board_led_line_t *line = &lines[i];
        if (!line->ws) continue;
        led_runtime_dma_tc(line->ws, htim);
    }
}
