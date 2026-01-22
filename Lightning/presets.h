/**
 ******************************************************************************
 * @file    presets.h
 * @brief   Theme presets and configuration for ambient lighting
 * @details Defines ambient lighting themes organized into banks (Amber, Blue, White).
 *          Each theme includes effect configuration, palettes, and zone profiles.
 *
 * @section Theme System
 * Themes are organized into banks corresponding to OEM colors:
 * - Amber Bank: Warm, sunset-inspired themes
 * - Blue Bank: Cool, ocean-inspired themes
 * - White Bank: Neutral, crystal-inspired themes
 *
 * @section Theme Structure
 * Each theme (ws_theme_desc_t) defines:
 * - Main effect and palette
 * - Theme brightness
 * - Zone-specific profiles (strip, handle, storage, footwell)
 *
 * @version 2.0
 * @date    2025
 ******************************************************************************
 */

#pragma once

#include <stdint.h>
#include "types.h"
#include "palette.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Профиль одной логической зоны для темы
 * ----------------------------------------------------------------------- */

typedef struct {
    fx_id_t         fx;             /* какой FX использовать в зоне */
    ws_palette_id_t pal_id;         /* какая палитра */
    float           rel_brightness; /* множитель яркости зоны (0..1) */
    float           accent_u;       /* 0..1, смещение выборки палитры для solid */
} ws_zone_profile_t;

/* -----------------------------------------------------------------------
 * Описание темы
 * ----------------------------------------------------------------------- */

typedef struct {
    fx_id_t          fx_main;                   /* FX для основной сцены */
    ws_palette_id_t  pal_main;                  /* базовая палитра темы */
    float            theme_brightness;          /* базовая яркость темы (0..1) */
    ws_zone_profile_t zone[WS_ZONE_MAX];        /* профили по логическим зонам */
} ws_theme_desc_t;

/* -----------------------------------------------------------------------
 * Список тем (12 штук, сгруппированы по 3 OEM-цветам)
 * ----------------------------------------------------------------------- */

typedef enum {
    /* AMBER BANK */
    THEME_AMBER_CHAMPAGNE_ARC = 0,
    THEME_AMBER_SUNSET,
    THEME_AMBER_COPPER_ROSE,
    THEME_AMBER_BURGUNDY_VELVET,

    /* BLUE BANK */
    THEME_BLUE_AURORA_GLACIER,
    THEME_BLUE_SAPPHIRE_ICE,
    THEME_BLUE_NIGHT_OPAL,
    THEME_BLUE_EMERALD_STREAM,

    /* WHITE BANK */
    THEME_WHITE_PLATINUM_CLOUD,
    THEME_WHITE_SILVER_SILK,
    THEME_WHITE_PEARL_BLUSH,
    THEME_WHITE_GLACIER_MIST,

    THEME_MAX_
} ws_theme_enum_t;

/* Внешний ID темы в системе — uint8_t (из types.h: ws_theme_id_t) */
const ws_theme_desc_t* ws_theme_get(ws_theme_id_t id);

/* -----------------------------------------------------------------------
 * OEM color → theme banks (для Extended Mode)
 * ----------------------------------------------------------------------- */

/* OEM-цвет, который приходит от штатного блока по CAN (3 варианта) */
typedef enum {
    OEM_COLOR_AMBER = 0,   /* solar / amber / warm */
    OEM_COLOR_BLUE,        /* polar / blue         */
    OEM_COLOR_WHITE,       /* neutral / white      */
    OEM_COLOR_MAX
} oem_color_id_t;

/* Банк тем для одного OEM-цвета */
typedef struct {
    const ws_theme_id_t *themes;  /* массив ID тем (ws_theme_id_t) */
    uint8_t              count;   /* сколько тем в банке */
} ws_theme_bank_t;

/* Глобальные банки (описаны в presets.c) */
extern const ws_theme_bank_t g_theme_banks[OEM_COLOR_MAX];

/* Получить банк по OEM-цвету; вернёт NULL, если id некорректен/пустой */
const ws_theme_bank_t* ws_theme_get_bank(oem_color_id_t id);

/*
 * Выбрать "следующую" тему внутри банка.
 * Если current не найден в банке — вернёт первую (themes[0]).
 */
ws_theme_id_t ws_theme_bank_next(const ws_theme_bank_t *bank,
                                 ws_theme_id_t          current);

/*
 * Стартовая тема для данного OEM-цвета
 * (обычно первая тема в соответствующем банке).
 */
ws_theme_id_t ws_theme_default_for_oem(oem_color_id_t id);

/*
 * Проверить, принадлежит ли тема тому же банку, что и эталонная тема.
 * Используется для авто-ротации: если обе темы в одном банке, 
 * изменение не требует outro/intro.
 */
uint8_t ws_theme_same_bank(ws_theme_id_t theme_a, ws_theme_id_t theme_b);

#ifdef __cplusplus
}
#endif
