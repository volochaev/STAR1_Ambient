/*
 * palette.h
 *
 *  Created on: Nov 9, 2025
 *      Author: nv
 */
#pragma once
#include "types.h"
#include <stdint.h>

const AMB_palette_t* palette_get(AMB_palette_id_t id);

/**
 * Сэмплирование цвета из палитры.
 * u ∈ [0,1] → линейная интерполяция между контрольными точками.
 */
void palette_sample_rgb8(const AMB_palette_t *pal, float u, uint8_t *r, uint8_t *g, uint8_t *b);
void palette_sample_blend_rgb8(const AMB_palette_t *a,
                               const AMB_palette_t *b,
                               float mix,   /* 0..1, 0=a, 1=b */
                               float u,     /* 0..1 position */
                               uint8_t *r,
                               uint8_t *g,
                               uint8_t *b_out);

