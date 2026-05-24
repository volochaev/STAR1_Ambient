#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize debug hooks state.
 */
void runtime_debug_hooks_init(void);
/**
 * @brief Feed debug hooks with each observed CAN RX frame.
 */
void runtime_debug_hooks_note_can_rx(uint32_t can_id, uint32_t now_ms);
/**
 * @brief Optional strip overlay (debug pulse/marker), returns 1 when active.
 */
uint8_t runtime_debug_hooks_get_strip_overlay(uint32_t now_ms,
                                              uint8_t *r,
                                              uint8_t *g,
                                              uint8_t *b);

#ifdef __cplusplus
}
#endif
