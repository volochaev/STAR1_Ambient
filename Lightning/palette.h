/**
 ******************************************************************************
 * @file    palette.h
 * @brief   Color palette system for ambient lighting
 * @details Provides color palettes inspired by Mercedes-Benz ambient lighting.
 *          Palettes are defined as gradient stops that can be sampled at
 *          any position (0.0 to 1.0) to get RGB colors.
 *
 * @section Palette System
 * Palettes consist of color stops (ws_pal_stop_t) with position and RGB values.
 * Colors are interpolated between stops for smooth gradients.
 *
 * @section Available Palettes
 * Classic: Sunset Amber, Ocean Blue, Red Moon, Purple Silk
 * Classic: Glacier, Energize, Polar White, Night Blue
 * Classic: Hyacinth, Copper Gold, Rose
 * Premium: Diamond White, Silver Mist, Platinum, Amber Gold, Magenta Royal
 * W223/EQS: Aurora Borealis, Champagne Gold, Deep Burgundy, Emerald Forest, Ice Sapphire
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
    /* Classic Mercedes palettes */
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

    /* Premium palettes */
    WSPAL_MB_DIAMOND_WHITE,   /* бриллиантово-белый с холодными оттенками */
    WSPAL_MB_SILVER_MIST,     /* серебряный туман, нейтральный металлик */
    WSPAL_MB_PLATINUM,        /* платина, элегантный серый с теплым оттенком */
    WSPAL_MB_AMBER_GOLD,      /* янтарное золото, насыщенное */
    WSPAL_MB_MAGENTA_ROYAL,   /* королевский пурпур, глубокий и насыщенный */

    /* W223/EQS Premium palettes */
    WSPAL_AURORA_BOREALIS,    /* северное сияние - зелёный/голубой/фиолетовый */
    WSPAL_CHAMPAGNE_GOLD,     /* шампань - тёплый перламутровый золотистый */
    WSPAL_DEEP_BURGUNDY,      /* глубокий бордо с винным оттенком */
    WSPAL_EMERALD_FOREST,     /* изумрудный лес с тёплыми акцентами */
    WSPAL_ICE_SAPPHIRE,       /* ледяной сапфировый синий */

    WSPAL_MAX_
} ws_palette_id_t;

const ws_palette_t* ws_palette_get(ws_palette_id_t id);
void ws_palette_sample_rgb8(const ws_palette_t *pal, float u,
                            uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif
