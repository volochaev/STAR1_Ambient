/**
 ******************************************************************************
 * @file    effects.h
 * @brief   Visual effects engine for ambient lighting
 * @details Provides visual effects (FX) for ambient lighting including
 *          gradient flows, waves, pulses, and one-shot animations for
 *          intro/outro sequences.
 *
 * @section Effects Types
 * - Continuous effects: Gradient flow, twin wave, ocean flow, etc.
 * - One-shot effects: Welcome animations, goodbye fade, etc.
 * - Effects use palettes for color mapping and fx_state_t for runtime state
 *
 * @section Usage
 * - One-shot effects: Use ws_oneshot_start() and ws_oneshot_tick()
 * - Continuous effects: Use ws_fx_apply() in scene player loop
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include <stdint.h>
#include "types.h"
#include "driver.h"
#include "palette.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

void ws_oneshot_start(ws2812_t        *ws,
                      oneshot_t       *os,
                      fx_id_t          fx_id,
                      const ws_palette_t *pal,
                      float            base_br,
                      uint32_t         duration_ms);

uint8_t ws_oneshot_tick(ws2812_t *ws, oneshot_t *os);

void ws_fx_apply(ws2812_t           *ws,
                 fx_id_t             fx_id,
                 const ws_palette_t *pal,
                 fx_state_t         *st,
                 uint32_t            delta_ms);

#ifdef __cplusplus
}
#endif
