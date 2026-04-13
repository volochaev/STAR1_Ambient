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
 * - One-shot effects: Use effect_oneshot_start() / effect_oneshot_tick() at orchestration layer
 * - Continuous effects: Use effect_apply() (or ws_fx_apply() in low-level modules)
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
                      uint32_t         duration_ms,
                      uint16_t         first,
                      uint16_t         count);

uint8_t ws_oneshot_tick(ws2812_t *ws, oneshot_t *os);

void ws_fx_apply(ws2812_t           *ws,
                 fx_id_t             fx_id,
                 const ws_palette_t *pal,
                 fx_state_t         *st,
                 uint32_t            delta_ms,
                 uint16_t            first,
                 uint16_t            count);

/* Neutral aliases for orchestration/theme layers */
static inline void effect_oneshot_start(ws2812_t *ws,
                                        oneshot_t *os,
                                        fx_id_t fx_id,
                                        const palette_t *pal,
                                        float base_br,
                                        uint32_t duration_ms,
                                        uint16_t first,
                                        uint16_t count)
{
    ws_oneshot_start(ws,
                     os,
                     fx_id,
                     (const ws_palette_t *)pal,
                     base_br,
                     duration_ms,
                     first,
                     count);
}

static inline uint8_t effect_oneshot_tick(ws2812_t *ws, oneshot_t *os)
{
    return ws_oneshot_tick(ws, os);
}

static inline void effect_apply(ws2812_t *ws,
                                fx_id_t fx_id,
                                const palette_t *pal,
                                fx_state_t *st,
                                uint32_t delta_ms,
                                uint16_t first,
                                uint16_t count)
{
    ws_fx_apply(ws, fx_id, (const ws_palette_t *)pal, st, delta_ms, first, count);
}

#ifdef __cplusplus
}
#endif
