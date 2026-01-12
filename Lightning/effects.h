
#pragma once
/**
 * @file effects.h
 * @brief Visual FX for ambient lighting.
 */

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
