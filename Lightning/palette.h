
#pragma once
/**
 * @file palette.h
 * @brief Color palettes for ambient lighting (Mercedes-style inspired).
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float    pos;   /* 0..1 */
    uint8_t  r, g, b;
} ws_pal_stop_t;

typedef struct ws_palette_s {
    const ws_pal_stop_t *stops;
    uint8_t              count;
} ws_palette_t;

typedef enum {
    WSPAL_MB_SUNSET_AMBER = 0,
    WSPAL_MB_OCEAN_BLUE,
    WSPAL_MB_RED_MOON,
    WSPAL_MB_PURPLE_SILK,
    WSPAL_MB_GLACIER,
    WSPAL_MB_ENERGIZE,
    WSPAL_MB_POLAR_WHITE,
    WSPAL_MB_NIGHT_BLUE,      /* глубокий ночной синий */
    WSPAL_MB_HYACINTH,        /* фиолетово-розовый MB Hyacinth */
    WSPAL_MB_COPPER_GOLD,     /* медно-золотая */
    WSPAL_MB_ROSE,            /* мягкий розово-золотистый */
    WSPAL_MAX_
} ws_palette_id_t;

const ws_palette_t* ws_palette_get(ws_palette_id_t id);
void ws_palette_sample_rgb8(const ws_palette_t *pal, float u,
                            uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif
