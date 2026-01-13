#include "zones.h"
#include "features.h"
#include "palette.h"
#include "effects.h"
#include "stm32g4xx_hal.h"
#include <math.h>
#include <string.h>

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint32_t led_index3(uint16_t i) { return (uint32_t)i * 3u; }

/* Живой FX для зоны, с учётом темы и текущего состояния плеера */
static void apply_zone_fx(const ws_theme_desc_t *T,
                          ws_zone_id_t z,
                          const scene_player_t *pl)
{
    const zone_map_t *zm = &g_zone_map[z];
    if (!zm || !zm->ws || zm->count == 0) return;

    const ws_zone_profile_t *zp = &T->zone[z];
    if (!zp) return;

    const ws_palette_t *pal =
        ws_palette_get(zp->pal_id ? zp->pal_id : T->pal_main);
    if (!pal) return;

    /* Базовая яркость темы + относительная яркость зоны */
    float base = clamp01f(pl->calc_brightness);
    float rel  = (zp->rel_brightness > 0.0f) ? zp->rel_brightness : 1.0f;
    float br   = base * rel;

    /* Night mode – дополнительное притемнение */
    if (g_amb_night_mode) {
        br *= AMB_NIGHT_BRIGHTNESS_SCALE;
    }

    br = clamp01f(br);

    uint32_t base_idx = led_index3(zm->first);

    if (br <= 0.0f) {
        /* Если зона выключена – просто гасим её сегмент */
        for (uint16_t i = 0; i < zm->count; ++i) {
            uint32_t idx = base_idx + (uint32_t)i * 3u;
            zm->ws->rgb[idx+0] = 0;
            zm->ws->rgb[idx+1] = 0;
            zm->ws->rgb[idx+2] = 0;
        }
        return;
    }

    fx_id_t fx = zp->fx ? zp->fx : T->fx_main;

    /* Базовая позиция на палитре */
    float base_u = (zp->accent_u > 0.0f) ? clamp01f(zp->accent_u) : 0.5f;

#if !AMB_ENABLE_ZONE_FX
    /* Если продвинутая анимация зон выключена – просто мягкий solid */
    fx = FX_MB_SOFT_SOLID;
#endif

    uint32_t now_ms = HAL_GetTick();
    float    t_sec  = now_ms * 0.001f;

    switch (fx) {

    /* --------------------------------------------------------------
     * 1) Мягкий статичный цвет (soft solid)
     * -------------------------------------------------------------- */
    case FX_MB_SOFT_SOLID:
    default:
    {
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, base_u, &r, &g, &b);

        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);

        for (uint16_t i = 0; i < zm->count; ++i) {
            uint16_t led_idx = zm->first + i;
            ws_set_pixel_rgb(zm->ws, led_idx, r, g, b);
        }
    }
    break;

    /* --------------------------------------------------------------
     * 2) Лёгкое дыхание яркости (soft breathe)
     * -------------------------------------------------------------- */
    case FX_MB_SOFT_BREATHE:
    {
        /* Очень медленное дыхание:
         * ~0.05 Гц => 20 секунд на цикл.
         * Каждой зоне даём фазовый сдвиг, чтобы не синхронно моргали.
         */
        const float freq_hz = 0.05f;
        const float phase   = (float)z * 1.3f;

        float s = 0.5f + 0.5f * sinf(2.0f * (float)M_PI * freq_hz * t_sec + phase);
        float br_dyn = br * (0.4f + 0.6f * s);  // 0.4..1.0 от br

        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, base_u, &r, &g, &b);

        r = (uint8_t)(r * br_dyn);
        g = (uint8_t)(g * br_dyn);
        b = (uint8_t)(b * br_dyn);

        for (uint16_t i = 0; i < zm->count; ++i) {
            uint16_t led_idx = zm->first + i;
            ws_set_pixel_rgb(zm->ws, led_idx, r, g, b);
        }
    }
    break;

    /* --------------------------------------------------------------
     * 3) Мягкий flow по палитре (цвет «течёт» по оттенкам зоны)
     * -------------------------------------------------------------- */
    case FX_MB_FLOW_SOFT:
    case FX_MB_OCEAN_FLOW:
    {
        const float speed_u  = 0.02f;   // единиц палитры в секунду
        const float spread_u = 0.10f;   // растяжение по длине зоны

        for (uint16_t i = 0; i < zm->count; ++i) {
            float pos = (zm->count > 1) ? (float)i / (float)(zm->count - 1) : 0.0f;
            float u   = base_u + speed_u * t_sec + spread_u * (pos - 0.5f);

            u = u - floorf(u);
            if (u < 0.0f) u += 1.0f;

            uint8_t r, g, b;
            ws_palette_sample_rgb8(pal, u, &r, &g, &b);

            r = (uint8_t)(r * br);
            g = (uint8_t)(g * br);
            b = (uint8_t)(b * br);

            uint16_t led_idx = zm->first + i;
            ws_set_pixel_rgb(zm->ws, led_idx, r, g, b);
        }
    }
    break;
    }
}

void zones_apply_scene(const scene_player_t *pl)
{
    if (!pl) return;
    const ws_theme_desc_t *T = ws_theme_get(pl->theme);
    if (!T) return;

    for (int z = 0; z < (int)WS_ZONE_MAX; ++z) {
        if (z == WS_ZONE_STRIP)
            continue; // главную ленту рисует scene_player

        apply_zone_fx(T, (ws_zone_id_t)z, pl);
    }
}

void zones_apply_intro(const scene_player_t *pl, float t_norm)
{
    if (!pl) return;

    const ws_theme_desc_t *T = ws_theme_get(pl->theme);
    if (!T) return;

    float t = t_norm;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    for (int z = 0; z < (int)WS_ZONE_MAX; ++z) {

        if (z == WS_ZONE_STRIP)
            continue;   // strip в это время рисует oneshot welcome

        ws_zone_id_t zone_id = (ws_zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->ws || zm->count == 0) continue;

        const ws_zone_profile_t *zp = &T->zone[zone_id];
        if (!zp) continue;

        const ws_palette_t *pal =
            ws_palette_get(zp->pal_id ? zp->pal_id : T->pal_main);
        if (!pal) continue;

        /* Базовая яркость темы */
        float base = clamp01f(pl->calc_brightness);
        float rel  = (zp->rel_brightness > 0.0f) ? zp->rel_brightness : 1.0f;
        float br   = base * rel;

        /* Night mode */
        if (g_amb_night_mode) {
            br *= AMB_NIGHT_BRIGHTNESS_SCALE;
        }

        /* --- Фаза включения по зонам ---
         *  Handle:   старт с небольшой задержкой
         *  Storage:  позже
         *  Footwell: последним
         */

        float local = 0.0f;

        switch (zone_id) {
        case WS_ZONE_HANDLE:
            if (t > 0.15f) {
                local = (t - 0.15f) / 0.60f;  // растягиваем 0.15..0.75
            }
            break;

        case WS_ZONE_STORAGE:
            if (t > 0.30f) {
                local = (t - 0.30f) / 0.55f;  // 0.30..0.85
            }
            break;

        case WS_ZONE_FOOTWELL:
            if (t > 0.40f) {
                local = (t - 0.40f) / 0.50f;  // 0.40..0.90
            }
            break;

        default:
            local = t;  // на всякий случай
            break;
        }

        if (local <= 0.0f) {
            /* Зона ещё не включается – гасим её */
            for (uint16_t i = 0; i < zm->count; ++i) {
                uint16_t led = zm->first + i;
                ws_set_pixel_rgb(zm->ws, led, 0, 0, 0);
            }
            continue;
        }

        if (local > 1.0f) local = 1.0f;

        /* Слегка сглаживаем включение */
        float k = local;
        k = k * k * (3.0f - 2.0f * k);  // smoothstep

        float br_zone = br * k;

        /* Цвет зоны – по палитре и accent_u */
        float u = (zp->accent_u > 0.0f) ? clamp01f(zp->accent_u) : 0.5f;
        uint8_t r, g, b;
        ws_palette_sample_rgb8(pal, u, &r, &g, &b);

        r = (uint8_t)(r * br_zone);
        g = (uint8_t)(g * br_zone);
        b = (uint8_t)(b * br_zone);

        for (uint16_t i = 0; i < zm->count; ++i) {
            uint16_t led = zm->first + i;
            ws_set_pixel_rgb(zm->ws, led, r, g, b);
        }
    }
}

void zones_apply_outro(const scene_player_t *pl, float t_norm)
{
    if (!pl) return;

    float t = t_norm;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* k_base – общая затухалка */
    float k_base = 1.0f - t;

    const ws_theme_desc_t *T = ws_theme_get(pl->theme);
    if (!T) return;

    for (int z = 0; z < (int)WS_ZONE_MAX; ++z) {

        if (z == WS_ZONE_STRIP)
            continue;   // strip делает свой goodbye-oneshot

        ws_zone_id_t zone_id = (ws_zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->ws || zm->count == 0) continue;

        float k_zone = k_base;

        /* Немного разных таймингов для зон */
        switch (zone_id) {
        case WS_ZONE_HANDLE:
            /* handle гаснет чуть быстрее */
            k_zone = 1.0f - t * 1.2f;
            break;
        case WS_ZONE_STORAGE:
            /* storage – примерно вместе с базой */
            k_zone = k_base;
            break;
        case WS_ZONE_FOOTWELL:
            /* footwell гаснет медленнее (снизу остаётся "эхо") */
            k_zone = 1.0f - t * 0.8f;
            break;
        default:
            k_zone = k_base;
            break;
        }

        if (k_zone < 0.0f) k_zone = 0.0f;
        if (k_zone > 1.0f) k_zone = 1.0f;

        /* Просто домножаем текущий кадр зоны на k_zone
         * (здесь предполагается, что zones_apply_scene уже отрисовал сцену до вызова outro).
         * Используем frame_get_pixel для правильного чтения RGB формата.
         */
        for (uint16_t i = 0; i < zm->count; ++i) {
            uint16_t led_idx = zm->first + i;
            uint8_t r, g, b;
            
            /* Читаем текущий цвет пикселя (формат RGB в буфере) */
            uint32_t idx = (uint32_t)led_idx * 3u;
            r = zm->ws->rgb[idx + 0];
            g = zm->ws->rgb[idx + 1];
            b = zm->ws->rgb[idx + 2];
            
            /* Применяем затухание */
            r = (uint8_t)(r * k_zone);
            g = (uint8_t)(g * k_zone);
            b = (uint8_t)(b * k_zone);
            
            /* Записываем обратно */
            ws_set_pixel_rgb(zm->ws, led_idx, r, g, b);
        }
    }
}
