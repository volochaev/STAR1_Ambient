/*
 * zones.c
 *
 *  Created on: Nov 9, 2025
 *      Author: nv
 */

#include "zones.h"
#include "frame_utils.h"
#include <string.h>

/* g_zone_map определяется в конкретном board_xxx.c/board_xxx.h */

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint32_t led_index3(uint16_t i)
{
    return (uint32_t)i * 3u;
}

/**
 * Применение "простого" FX к зоне:
 * - FX_MB_SOFT_SOLID -> статичный цвет из палитры по accent_u.
 * - FX_MB_FLOW_* / FX_MB_DUO_FLOW -> градиент / дуо по палитре.
 *
 * Здесь минимальная логика, всё "умное" в пресетах и палитрах.
 */
static void apply_zone_fx_simple(const ws_zone_profile_t *zp,
                                 const ws_palette_t *pal,
                                 ws2812_t *ws,
                                 uint16_t first,
                                 uint16_t count,
                                 float global_br)
{
    if (!pal || !ws || count == 0)
        return;

    float rel = (zp && zp->rel_brightness > 0.0f)
                ? zp->rel_brightness
                : 1.0f;

    float br = clamp01f(global_br * rel);
    if (br <= 0.0f) {
        // Гасим зону
        frame_fill_zone(ws, first, count, 0, 0, 0);
        return;
    }

    ws_fx_id_t fx = (zp && zp->fx) ? zp->fx : FX_MB_SOFT_SOLID;
    float accent_u = (zp && zp->accent_u > 0.0f)
                     ? clamp01f(zp->accent_u)
                     : 0.5f;

    switch (fx)
    {
    case FX_MB_SOFT_SOLID:
    default:
    {
        // Статичный цвет по accent_u (ручки, карманы, ног)
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, accent_u, &r, &g, &b);
        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);

        frame_fill_zone(ws, first, count, r, g, b);
    }
    break;

    case FX_MB_FLOW_SOFT:   // alias FX_GRADIENT_FLOW
    case FX_MB_FLOW_DEEP:   // alias FX_MB_OCEAN_FLOW
    {
        // Плавный градиент по палитре вдоль зоны
        if (count == 1) {
            uint8_t r, g, b;
            ws_palette_sample_rgb8(pal, accent_u, &r, &g, &b);
            r = (uint8_t)(r * br);
            g = (uint8_t)(g * br);
            b = (uint8_t)(b * br);
            frame_set_pixel(ws, first, r, g, b);
            break;
        }

        uint32_t base = led_index3(first);
        for (uint16_t i = 0; i < count; ++i) {
            float u = (float)i / (float)(count - 1);
            uint8_t r, g, b;
            ws_palette_sample_rgb8(pal, u, &r, &g, &b);
            r = (uint8_t)(r * br);
            g = (uint8_t)(g * br);
            b = (uint8_t)(b * br);

            uint32_t idx = base + (uint32_t)i * 3u;
            ws->grb[idx + 0] = g;
            ws->grb[idx + 1] = r;
            ws->grb[idx + 2] = b;
        }
    }
    break;

    case FX_MB_DUO_FLOW:    // alias FX_DUAL_ZONE
    {
        // Дуо-палитра: нижняя часть палитры и верхняя, как двухцветный режим
        if (count == 1) {
            uint8_t r, g, b;
            ws_palette_sample_rgb8(pal, 0.5f, &r, &g, &b);
            r = (uint8_t)(r * br);
            g = (uint8_t)(g * br);
            b = (uint8_t)(b * br);
            frame_set_pixel(ws, first, r, g, b);
            break;
        }

        uint32_t base = led_index3(first);
        for (uint16_t i = 0; i < count; ++i) {
            float v = (float)i / (float)(count - 1);
            float u = (v < 0.5f)
                      ? (v * 2.0f * 0.5f)                    // 0..0.5
                      : (0.5f + (v - 0.5f) * 2.0f * 0.5f);  // 0.5..1

            uint8_t r, g, b;
            ws_palette_sample_rgb8(pal, clamp01f(u), &r, &g, &b);
            r = (uint8_t)(r * br);
            g = (uint8_t)(g * br);
            b = (uint8_t)(b * br);

            uint32_t idx = base + (uint32_t)i * 3u;
            ws->grb[idx + 0] = g;
            ws->grb[idx + 1] = r;
            ws->grb[idx + 2] = b;
        }
    }
    break;
    }
}

/* === Публичные функции ============================= */

void zones_apply_scene(const scene_player_t *pl)
{
    if (!pl) return;

    const ws_theme_desc_t *T = ws_scene_by_theme(pl->theme);
    if (!T) return;

    float base = clamp01f(pl->calc_brightness);

    for (int z = 0; z < (int)ZONE_MAX; ++z) {
        const zone_map_t *zm = &g_zone_map[z];
        if (!zm || !zm->ws || zm->count == 0)
            continue;

        const ws_zone_profile_t *zp = &T->zone[z];

        // fallback: если профиль пустой — используем главную тему
        ws_fx_id_t fx = T->fx_main;
        ws_palette_id_t pal_id = T->pal_main;
        float rel = 1.0f;
        float accent_u = 0.5f;

        if (zp && (zp->fx || zp->pal_id || zp->rel_brightness > 0.0f)) {
            if (zp->fx) fx = zp->fx;
            if (zp->pal_id) pal_id = zp->pal_id;
            if (zp->rel_brightness > 0.0f) rel = zp->rel_brightness;
            if (zp->accent_u > 0.0f) accent_u = zp->accent_u;
        }

        const ws_palette_t *pal = ws_palette_get(pal_id);
        if (!pal) continue;

        ws_zone_profile_t eff = {
            .fx = fx,
            .pal_id = pal_id,
            .rel_brightness = rel,
            .accent_u = accent_u
        };

        apply_zone_fx_simple(&eff,
                             pal,
                             zm->ws,
                             zm->first,
                             zm->count,
                             base);
    }
}

void zones_apply_intro(const scene_player_t *pl, float t_norm)
{
    if (!pl) return;

    float t = clamp01f(t_norm);

    // Идея:
    // - при t < 0.5: accent-зоны почти выключены (главное шоу на strip)
    // - при t >= 0.5: плавно поднимаем их к целевому уровню

    const ws_theme_desc_t *T = ws_scene_by_theme(pl->theme);
    if (!T) return;

    float scene_br = clamp01f(pl->calc_brightness);

    float k = 0.0f;
    if (t > 0.5f)
        k = (t - 0.5f) / 0.5f; // 0..1

    for (int z = 0; z < (int)ZONE_MAX; ++z) {
        if (z == ZONE_STRIP) {
            // STRIP ведётся через oneshot-интро/bridge сцен-плеера
            continue;
        }

        const zone_map_t *zm = &g_zone_map[z];
        if (!zm || !zm->ws || zm->count == 0)
            continue;

        const ws_zone_profile_t *zp = &T->zone[z];
        if (!zp || zp->rel_brightness <= 0.0f)
            continue;

        const ws_palette_t *pal =
            ws_palette_get(zp->pal_id ? zp->pal_id : T->pal_main);
        if (!pal) continue;

        ws_zone_profile_t eff = *zp;
        eff.rel_brightness *= k;

        apply_zone_fx_simple(&eff,
                             pal,
                             zm->ws,
                             zm->first,
                             zm->count,
                             scene_br);
    }
}

void zones_apply_outro(const scene_player_t *pl, float t_norm)
{
    (void)pl;
    float t = clamp01f(t_norm);
    float k = 1.0f - t;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {
        const zone_map_t *zm = &g_zone_map[z];
        if (!zm || !zm->ws || zm->count == 0)
            continue;

        // Плавно гасим все пиксели зоны (пока локально, без frame_utils)
        uint32_t base = led_index3(zm->first);
        for (uint16_t i = 0; i < zm->count; ++i) {
            uint32_t idx = base + (uint32_t)i * 3u;
            zm->ws->grb[idx + 0] = (uint8_t)(zm->ws->grb[idx + 0] * k);
            zm->ws->grb[idx + 1] = (uint8_t)(zm->ws->grb[idx + 1] * k);
            zm->ws->grb[idx + 2] = (uint8_t)(zm->ws->grb[idx + 2] * k);
        }
    }
}
