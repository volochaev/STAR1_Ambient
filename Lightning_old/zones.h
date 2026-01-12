/*
 * zones.h
 *
 *  Created on: Nov 9, 2025
 *      Author: nv
 */
#pragma once

#include <stdint.h>
#include "driver.h"
#include "types.h"
#include "palette.h"
#include "theme.h"
#include "scene_player.h"

/**
 * Логические зоны внутри одного модуля.
 * Не все зоны обязаны существовать физически на каждом MCU.
 */
typedef enum {
    ZONE_STRIP = 0,    // основная линия / окантовка
    ZONE_HANDLE,       // ручка открывания / верхний акцент
    ZONE_STORAGE,      // ниша / карман внизу
    ZONE_FOOTWELL,     // подсветка ног
    ZONE_MAX
} ws_zone_id_t;

/**
 * Описание соответствия логической зоны реальному отрезку ленты.
 * На конкретной плате задаётся в board_xxx.h.
 */
typedef struct {
    ws2812_t *ws;     // какая лента
    uint16_t  first;  // первый светодиод зоны (индекс в ленте)
    uint16_t  count;  // количество диодов
} zone_map_t;

/**
 * Таблица зон для данного модуля.
 * Должна быть определена в board_xxx.c/board_xxx.h:
 *
 *   const zone_map_t g_zone_map[WS_ZONE_MAX] = { ... };
 */
extern const zone_map_t g_zone_map[ZONE_MAX];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Применить текущую тему (SCENE) ко всем зонам.
 *
 * Важно:
 * - НЕ вызывает ws_render и не трогает питание.
 * - Использует scene_player_t::theme и calc_brightness.
 * - Для главной зоны (STRIP) предполагается, что основной FX уже отрисован
 *   через ws_theme_render_scene() в player_tick().
 * - Здесь мы:
 *     - допиливаем доп. зоны по ws_theme_desc_t.zone[]
 *     - можем подправить / доокрасить STRIP при необходимости
 */
void zones_apply_scene(const scene_player_t *pl);

/**
 * Опциональная поддержка интро для зон (MB-welcome).
 * t_norm: 0..1 - прогресс интро.
 * Можно вызывать после oneshot-интро, если хочешь плавно вводить accent-зоны.
 */
void zones_apply_intro(const scene_player_t *pl, float t_norm);

/**
 * Опциональная поддержка аутро для зон (MB-goodbye).
 * t_norm: 0..1 - прогресс аутро.
 * Можно вызывать вместе с goodbye, если хочешь, чтобы accent-зоны
 * гасли синхронно и плавно.
 */
void zones_apply_outro(const scene_player_t *pl, float t_norm);

#ifdef __cplusplus
}
#endif
