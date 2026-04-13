/**
 ******************************************************************************
 * @file    palette.h
 * @brief   Color palette system for ambient lighting
 * @details Provides color palettes inspired by Mercedes-Benz ambient lighting.
 *          Palettes are defined as gradient stops that can be sampled at
 *          any position (0.0 to 1.0) to get RGB colors.
 *
 * @section Palette System
 * Palettes consist of color stops (palette_stop_t / ws_pal_stop_t) with
 * position and RGB values.
 * Colors are interpolated between stops for smooth gradients.
 *
 * @section Available Palettes
 * W223/EQS Premium set:
 * - Champagne Arc, Amber Sunset, Copper Rose, Burgundy Velvet
 * - Emerald Veil, Aurora Glacier, Sapphire Ice, Night Opal
 * - Glacier Mist, Silver Silk, Platinum Cloud, Pearl Blush
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

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
    WSPAL_CHAMPAGNE_ARC = 0,   /* тёплое шампанское с перламутровым свечением */
    WSPAL_AMBER_SUNSET,        /* янтарно-апельсиновый закат */
    WSPAL_COPPER_ROSE,         /* розово-медный перламутр */
    WSPAL_BURGUNDY_VELVET,     /* глубокий бордо с бархатным блеском */
    WSPAL_EMERALD_VEIL,        /* изумрудный вуаль с тёплым акцентом */
    WSPAL_AURORA_GLACIER,      /* северное сияние: бирюза → лавандовый лёд */
    WSPAL_SAPPHIRE_ICE,        /* сапфировый лёд с кристальным верхом */
    WSPAL_NIGHT_OPAL,          /* ночной опал: синий с фиолетовым свечением */
    WSPAL_GLACIER_MIST,        /* туманный лёд: холодный бело-голубой */
    WSPAL_SILVER_SILK,         /* матовый серебряный шёлк */
    WSPAL_PLATINUM_CLOUD,      /* платиновое облако, чистый нейтральный белый */
    WSPAL_PEARL_BLUSH,         /* жемчужный румянец: белый с розовым свечением */
    WSPAL_MAX_
} ws_palette_id_t;

const ws_palette_t* ws_palette_get(ws_palette_id_t id);
void ws_palette_sample_rgb8(const ws_palette_t *pal, float u,
                            uint8_t *r, uint8_t *g, uint8_t *b);

/* Neutral aliases for non-driver layers */
typedef ws_pal_stop_t palette_stop_t;
typedef ws_palette_t palette_t;
typedef ws_palette_id_t palette_id_t;

static inline const palette_t *palette_get(palette_id_t id)
{
    return ws_palette_get((ws_palette_id_t)id);
}

static inline void palette_sample_rgb8(const palette_t *pal,
                                       float u,
                                       uint8_t *r,
                                       uint8_t *g,
                                       uint8_t *b)
{
    ws_palette_sample_rgb8((const ws_palette_t *)pal, u, r, g, b);
}

#ifdef __cplusplus
}
#endif
