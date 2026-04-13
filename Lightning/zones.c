#include "zones.h"
#include "features.h"
#include "palette.h"
#include "effects.h"
#include "ambient.h"
#include "led_runtime.h"
#include "stm32g4xx_hal.h"
#include <math.h>
#include <string.h>

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float smoothstep5(float x)
{
    x = clamp01f(x);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

static float wrap01f(float x)
{
    x = x - floorf(x);
    if (x < 0.0f) x += 1.0f;
    return x;
}

static float motion_profile_blend_gamma(void)
{
    motion_profile_t profile = can_ambient_get_motion_profile();
    if (profile == MOTION_PROFILE_CALM) {
        return AMB_PROFILE_BLEND_GAMMA_CALM;
    }
    if (profile == MOTION_PROFILE_SPORT) {
        return AMB_PROFILE_BLEND_GAMMA_SPORT;
    }
    return AMB_PROFILE_BLEND_GAMMA_LUXURY;
}

static float comfort_hvac_zone_dim(zone_id_t z)
{
#if AMB_ENABLE_COMFORT_AUTO_DIM
    float fan;
    if (!g_night_mode_state) return 1.0f;
    fan = can_ambient_get_hvac_fan_level();
    if (fan < 0.08f) return 1.0f;
    if (z == ZONE_FOOTWELL) return AMB_COMFORT_DIM_FOOTWELL_NIGHT_HVAC;
    if (z == ZONE_STORAGE) return AMB_COMFORT_DIM_STORAGE_NIGHT_HVAC;
#else
    (void)z;
#endif
    return 1.0f;
}

static float profile_blend_curve(float x)
{
    float g;
    x = clamp01f(x);
    g = motion_profile_blend_gamma();
    if (g < 0.40f) g = 0.40f;
    if (g > 2.50f) g = 2.50f;
    if (fabsf(g - 1.0f) < 0.01f) return x;
    return clamp01f(powf(x, g));
}

static float board_cabin_pose_lr(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL) || (BOARD_TYPE == BOARD_TYPE_RL)
    return -1.0f;
#elif (BOARD_TYPE == BOARD_TYPE_FR) || (BOARD_TYPE == BOARD_TYPE_RR)
    return 1.0f;
#else
    /* DASHBOARD and REAR are center-aligned by left/right axis. */
    return 0.0f;
#endif
}

static float board_cabin_pose_fr(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL) || (BOARD_TYPE == BOARD_TYPE_FR) || (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    return 1.0f;
#elif (BOARD_TYPE == BOARD_TYPE_RL) || (BOARD_TYPE == BOARD_TYPE_RR) || (BOARD_TYPE == BOARD_TYPE_REAR)
    /* BOARD_TYPE_REAR intentionally behaves like combined RL+RR rear center. */
    return -1.0f;
#else
    return 0.0f;
#endif
}

static void apply_override_to_zone(zone_id_t zone, float level, uint8_t r, uint8_t g, uint8_t b)
{
    const zone_map_t *zm = &g_zone_map[zone];
    uint8_t rr, gg, bb;
    if (!zm || !zm->strip || zm->count == 0 || level <= 0.0f) return;
    if (level > 1.0f) level = 1.0f;

    rr = (uint8_t)((float)r * level + 0.5f);
    gg = (uint8_t)((float)g * level + 0.5f);
    bb = (uint8_t)((float)b * level + 0.5f);

    for (uint16_t i = 0; i < zm->count; ++i) {
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), rr, gg, bb);
    }
}

/* Живой FX для зоны, с учётом темы и текущего состояния плеера */
static void apply_zone_fx(const theme_definition_t *T,
                          zone_id_t z,
                          const base_scene_t *pl)
{
    const zone_map_t *zm = &g_zone_map[z];
    if (!zm || !zm->strip || zm->count == 0) return;

    const zone_profile_t *zp = &T->zone[z];
    if (!zp) return;

    const palette_t *pal =
        palette_get(zp->pal_id ? zp->pal_id : T->pal_main);
    if (!pal) return;

    /* Базовая яркость темы + относительная яркость зоны */
    float base = clamp01f(pl->calc_brightness);
    float rel  = (zp->rel_brightness > 0.0f) ? zp->rel_brightness : 1.0f;
    float br   = base * rel;
    float base_u;
    float depth_u_shift = 0.0f;
    float depth_phase_shift = 0.0f;
    float zone_depth_weight = 0.0f;

    /* Night mode – дополнительное притемнение */
    if (g_night_mode_state) {
        br *= AMB_NIGHT_BRIGHTNESS_SCALE;
    }
    br *= comfort_hvac_zone_dim(z);

#if AMB_ENABLE_CABIN_DEPTH_MODEL
    zone_depth_weight = 0.70f;
    if (z == ZONE_HANDLE) zone_depth_weight = 1.00f;
    if (z == ZONE_STORAGE) zone_depth_weight = 0.82f;
    if (z == ZONE_FOOTWELL) zone_depth_weight = 0.92f;
    {
        float theme_depth = theme_personality_depth(pl->theme);
        float pose = board_cabin_pose_lr() * AMB_CABIN_DEPTH_LR_GAIN
                   + board_cabin_pose_fr() * AMB_CABIN_DEPTH_FR_GAIN;
        float depth = theme_depth * pose * zone_depth_weight;
        depth_u_shift = depth * AMB_CABIN_DEPTH_U_SPREAD;
        depth_phase_shift = depth * AMB_CABIN_DEPTH_PHASE_SHIFT;
        br *= (1.0f + depth * AMB_CABIN_DEPTH_BRIGHTNESS_DELTA);
    }
#endif

    br = clamp01f(br);

    if (br <= 0.0f) {
        /* Если зона выключена – просто гасим её сегмент */
        for (uint16_t i = 0; i < zm->count; ++i) {
            led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), 0u, 0u, 0u);
        }
        return;
    }

    fx_id_t fx = zp->fx ? zp->fx : T->fx_main;

    /* Базовая позиция на палитре */
    base_u = (zp->accent_u > 0.0f) ? clamp01f(zp->accent_u) : 0.5f;
    base_u = wrap01f(base_u + depth_u_shift);

#if !AMB_ENABLE_ZONE_FX
    /* Если продвинутая анимация зон выключена – просто мягкий solid */
    fx = FX_SOLID_GRADIENT;
#endif

    uint32_t now_ms = HAL_GetTick();
    float    t_sec  = now_ms * 0.001f;

    switch (fx) {

    /* --------------------------------------------------------------
     * 1) Мягкий статичный цвет (soft solid)
     * -------------------------------------------------------------- */
    case FX_SOLID_GRADIENT:
    default:
    {
        uint8_t r, g, b;
        palette_sample_rgb8(pal, base_u, &r, &g, &b);

        r = (uint8_t)(r * br);
        g = (uint8_t)(g * br);
        b = (uint8_t)(b * br);

        for (uint16_t i = 0; i < zm->count; ++i) {
            uint16_t led_idx = zm->first + i;
            led_runtime_set_pixel_rgb(zm->strip, led_idx, r, g, b);
        }
    }
    break;

    /* --------------------------------------------------------------
     * 2) Лёгкое дыхание яркости (soft breathe)
     * -------------------------------------------------------------- */
    case FX_SOFT_BREATHE:
    {
        /* Очень медленное дыхание:
         * ~0.05 Гц => 20 секунд на цикл.
         * Каждой зоне даём фазовый сдвиг, чтобы не синхронно моргали.
         */
        const float freq_hz = 0.05f;
        const float phase   = (float)z * 1.3f;

        float s = 0.5f + 0.5f * sinf(2.0f * (float)M_PI * freq_hz * t_sec + phase + depth_phase_shift);
        float br_dyn = br * (0.4f + 0.6f * s);  // 0.4..1.0 от br

        uint8_t r, g, b;
        palette_sample_rgb8(pal, base_u, &r, &g, &b);

        r = (uint8_t)(r * br_dyn);
        g = (uint8_t)(g * br_dyn);
        b = (uint8_t)(b * br_dyn);

        for (uint16_t i = 0; i < zm->count; ++i) {
            uint16_t led_idx = zm->first + i;
            led_runtime_set_pixel_rgb(zm->strip, led_idx, r, g, b);
        }
    }
    break;

    /* --------------------------------------------------------------
     * 3) Мягкий flow по палитре (цвет «течёт» по оттенкам зоны)
     * -------------------------------------------------------------- */
    case FX_GRADIENT_FLOW:
    case FX_OCEAN_FLOW:
    {
        const float speed_u  = 0.02f;   // единиц палитры в секунду
        const float spread_u = 0.10f;   // растяжение по длине зоны

        for (uint16_t i = 0; i < zm->count; ++i) {
            float pos = (zm->count > 1) ? (float)i / (float)(zm->count - 1) : 0.0f;
            float u   = base_u + speed_u * t_sec + spread_u * (pos - 0.5f);
            u = wrap01f(u);

            uint8_t r, g, b;
            palette_sample_rgb8(pal, u, &r, &g, &b);

            r = (uint8_t)(r * br);
            g = (uint8_t)(g * br);
            b = (uint8_t)(b * br);

            uint16_t led_idx = zm->first + i;
            led_runtime_set_pixel_rgb(zm->strip, led_idx, r, g, b);
        }
    }
    break;
    }
}

void zones_apply_scene(const base_scene_t *pl)
{
    if (!pl) return;
    const theme_definition_t *T = theme_get(pl->theme);
    if (!T) return;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {
        if (z == ZONE_STRIP)
            continue; // главную ленту рисует base_scene

        apply_zone_fx(T, (zone_id_t)z, pl);
    }
}

void zones_apply_intro(const base_scene_t *pl, float t_norm)
{
    if (!pl) return;

    const theme_definition_t *T = theme_get(pl->theme);
    if (!T) return;

    float t = t_norm;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {

        if (z == ZONE_STRIP)
            continue;   // strip в это время рисует oneshot welcome

        zone_id_t zone_id = (zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->strip || zm->count == 0) continue;

        const zone_profile_t *zp = &T->zone[zone_id];
        if (!zp) continue;

        const palette_t *pal =
            palette_get(zp->pal_id ? zp->pal_id : T->pal_main);
        if (!pal) continue;

        /* Базовая яркость темы */
        float base = clamp01f(pl->calc_brightness);
        float rel  = (zp->rel_brightness > 0.0f) ? zp->rel_brightness : 1.0f;
        float br   = base * rel;

        /* Night mode */
        if (g_night_mode_state) {
            br *= AMB_NIGHT_BRIGHTNESS_SCALE;
        }
        br *= comfort_hvac_zone_dim(zone_id);

        /* --- Фаза включения по зонам ---
         *  Handle:   старт с небольшой задержкой
         *  Storage:  позже
         *  Footwell: последним
         */

        float local = 0.0f;

        switch (zone_id) {
        case ZONE_HANDLE:
            if (t > 0.15f) {
                local = (t - 0.15f) / 0.60f;  // растягиваем 0.15..0.75
            }
            break;

        case ZONE_STORAGE:
            if (t > 0.30f) {
                local = (t - 0.30f) / 0.55f;  // 0.30..0.85
            }
            break;

        case ZONE_FOOTWELL:
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
                led_runtime_set_pixel_rgb(zm->strip, led, 0, 0, 0);
            }
            continue;
        }

        if (local > 1.0f) local = 1.0f;

        /* Quintic easing: более премиальное разгорание без "цифровых" краёв */
        float k = profile_blend_curve(smoothstep5(local));

        float br_zone = br * k;

        /* Цвет зоны – по палитре и accent_u */
        float u = (zp->accent_u > 0.0f) ? clamp01f(zp->accent_u) : 0.5f;
        uint8_t r, g, b;
        palette_sample_rgb8(pal, u, &r, &g, &b);

        r = (uint8_t)(r * br_zone);
        g = (uint8_t)(g * br_zone);
        b = (uint8_t)(b * br_zone);

        for (uint16_t i = 0; i < zm->count; ++i) {
            uint16_t led = zm->first + i;
            led_runtime_set_pixel_rgb(zm->strip, led, r, g, b);
        }
    }
}

void zones_apply_outro(const base_scene_t *pl, float t_norm)
{
    if (!pl) return;

    float t = t_norm;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* Quintic easing for premium fade-out */
    float k_base = 1.0f - profile_blend_curve(smoothstep5(t));

    const theme_definition_t *T = theme_get(pl->theme);
    if (!T) return;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {

        if (z == ZONE_STRIP)
            continue;   // strip делает свой goodbye-oneshot

        zone_id_t zone_id = (zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->strip || zm->count == 0) continue;

        float k_zone = k_base;

        /* Немного разных таймингов для зон */
        switch (zone_id) {
        case ZONE_HANDLE:
        {
            /* handle гаснет чуть быстрее, но с мягкой S-кривой */
            float tt = t * 1.15f;
            if (tt > 1.0f) tt = 1.0f;
            k_zone = 1.0f - profile_blend_curve(smoothstep5(tt));
        }
            break;
        case ZONE_STORAGE:
            /* storage – примерно вместе с базой */
            k_zone = k_base;
            break;
        case ZONE_FOOTWELL:
        {
            /* footwell гаснет медленнее (снизу остаётся "эхо"), тоже по S-кривой */
            float tt = t * 0.85f;
            if (tt > 1.0f) tt = 1.0f;
            k_zone = 1.0f - profile_blend_curve(smoothstep5(tt));
        }
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
            led_runtime_get_pixel_rgb(zm->strip, led_idx, &r, &g, &b);
            
            /* Применяем затухание */
            r = (uint8_t)(r * k_zone);
            g = (uint8_t)(g * k_zone);
            b = (uint8_t)(b * k_zone);
            
            /* Записываем обратно */
            led_runtime_set_pixel_rgb(zm->strip, led_idx, r, g, b);
        }
    }
}

/* Линейный бридж для зон: плавный переход от финала интро (solid) к сцене */
void zones_apply_bridge(const base_scene_t *pl, float t_norm)
{
    if (!pl) return;

    const theme_definition_t *T = theme_get(pl->theme);
    if (!T) return;

    float blend = clamp01f(t_norm);
    float inv   = 1.0f - blend;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {
        if (z == ZONE_STRIP)
            continue;   // strip блендится в base_scene

        zone_id_t zone_id = (zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->strip || zm->count == 0) continue;

        const zone_profile_t *zp = &T->zone[zone_id];
        if (!zp) continue;

        const palette_t *pal =
            palette_get(zp->pal_id ? zp->pal_id : T->pal_main);
        if (!pal) continue;

        /* Сначала рисуем сцену в буфер зоны */
        apply_zone_fx(T, zone_id, pl);

        /* Intro цвет: берём центр палитры, масштабируем по яркости зоны */
        float base = clamp01f(pl->calc_brightness);
        float rel  = (zp->rel_brightness > 0.0f) ? zp->rel_brightness : 1.0f;
        float br   = base * rel;
        if (g_night_mode_state) {
            br *= AMB_NIGHT_BRIGHTNESS_SCALE;
        }
        br *= comfort_hvac_zone_dim(zone_id);
        br = clamp01f(br);

        uint8_t intro_r, intro_g, intro_b;
        float base_u = (zp->accent_u > 0.0f) ? clamp01f(zp->accent_u) : 0.5f;
        palette_sample_rgb8(pal, base_u, &intro_r, &intro_g, &intro_b);
        intro_r = (uint8_t)(intro_r * br);
        intro_g = (uint8_t)(intro_g * br);
        intro_b = (uint8_t)(intro_b * br);

        for (uint16_t i = 0; i < zm->count; ++i) {
            uint8_t s_r, s_g, s_b;
            uint16_t led_idx = (uint16_t)(zm->first + i);
            led_runtime_get_pixel_rgb(zm->strip, led_idx, &s_r, &s_g, &s_b);

            uint8_t out_r = (uint8_t)(intro_r * inv + s_r * blend + 0.5f);
            uint8_t out_g = (uint8_t)(intro_g * inv + s_g * blend + 0.5f);
            uint8_t out_b = (uint8_t)(intro_b * inv + s_b * blend + 0.5f);

            led_runtime_set_pixel_rgb(zm->strip, led_idx, out_r, out_g, out_b);
        }
    }
}

void zones_apply_interrupt_overlay(uint32_t now_ms)
{
#if AMB_ENABLE_HVAC_DUAL_SPLIT
    {
        float split = can_ambient_get_hvac_split_bias_for_board();
        float mag = fabsf(split);
        if (mag > 0.01f) {
            uint8_t r = (split > 0.0f) ? AMB_HVAC_SPLIT_WARM_R : AMB_HVAC_SPLIT_COOL_R;
            uint8_t g = (split > 0.0f) ? AMB_HVAC_SPLIT_WARM_G : AMB_HVAC_SPLIT_COOL_G;
            uint8_t b = (split > 0.0f) ? AMB_HVAC_SPLIT_WARM_B : AMB_HVAC_SPLIT_COOL_B;
            float gain = AMB_HVAC_SPLIT_GAIN * mag;
            apply_override_to_zone(ZONE_STRIP, gain, r, g, b);
        }
    }
#endif

#if AMB_ENABLE_SEAT_HEAT_COUPLING
    {
        float seat = can_ambient_get_seat_heat_level_for_board();
        if (seat > 0.02f) {
            uint8_t r = AMB_SEAT_HEAT_WARM_R;
            uint8_t g = AMB_SEAT_HEAT_WARM_G;
            uint8_t b = AMB_SEAT_HEAT_WARM_B;
            apply_override_to_zone(ZONE_FOOTWELL, AMB_SEAT_HEAT_OVERLAY_GAIN_FOOTWELL * seat, r, g, b);
            apply_override_to_zone(ZONE_STORAGE, AMB_SEAT_HEAT_OVERLAY_GAIN_STORAGE * seat, r, g, b);
            apply_override_to_zone(ZONE_STRIP, AMB_SEAT_HEAT_OVERLAY_GAIN_STRIP * seat, r, g, b);
        }
    }
#endif

#if AMB_ENABLE_BSM_OVERLAY
    can_bsm_state_t bsm = can_ambient_get_bsm_state();
    float side_level = 0.0f;
    float t;
    float pulse;
    float luma;
    static float env_luma = 0.0f;
    static uint32_t last_ms = 0u;
    float attack_alpha = AMB_BSM_ENV_ATTACK_ALPHA;
    float release_alpha = AMB_BSM_ENV_RELEASE_ALPHA;
    float target_luma;
    uint8_t dashboard_like = (uint8_t)((BOARD_TYPE == BOARD_TYPE_DASHBOARD) ? 1u : 0u);
    uint8_t wr, wg, wb;

    /* Side filtering is applied in CAN layer according to BOARD_TYPE.
     * Overlay consumes already-masked state only. */
    if (bsm.left_active && bsm.left_level > side_level) side_level = bsm.left_level;
    if (bsm.right_active && bsm.right_level > side_level) side_level = bsm.right_level;

    /* Soft premium blink envelope */
    t = ((float)now_ms * 0.001f) * AMB_BSM_BLINK_HZ;
    t = t - floorf(t);
    pulse = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * t);  /* 0..1 */
    pulse = pulse * pulse * (3.0f - 2.0f * pulse);       /* smoothstep */
    /* Clear red/black pulse for maximum readability over ambient content. */
    if (pulse < 0.0f) pulse = 0.0f;
    if (pulse > 1.0f) pulse = 1.0f;
    target_luma = side_level * pulse;
    if (target_luma < 0.0f) target_luma = 0.0f;
    if (target_luma > 1.0f) target_luma = 1.0f;

    if (attack_alpha < 0.01f) attack_alpha = 0.01f;
    if (attack_alpha > 0.98f) attack_alpha = 0.98f;
    if (release_alpha < 0.01f) release_alpha = 0.01f;
    if (release_alpha > 0.98f) release_alpha = 0.98f;
    if (last_ms == 0u) {
        last_ms = now_ms;
        env_luma = target_luma;
    } else {
        last_ms = now_ms;
        if (target_luma >= env_luma) {
            env_luma += (target_luma - env_luma) * attack_alpha;
        } else {
            env_luma += (target_luma - env_luma) * release_alpha;
        }
    }

    luma = env_luma;
    if (luma < 0.005f) return;
    if (luma > 1.0f) luma = 1.0f;

    /* More saturated red warning for clear BSM contrast over ambient palette. */
    wr = 255u;
    wg = 18u;
    wb = 0u;

    apply_override_to_zone(ZONE_HANDLE, AMB_BSM_OVERLAY_GAIN_HANDLE * luma, wr, wg, wb);
    if (dashboard_like) {
        apply_override_to_zone(ZONE_STRIP, (AMB_BSM_OVERLAY_GAIN_STRIP * 0.72f) * luma, wr, wg, wb);
    } else {
        apply_override_to_zone(ZONE_STRIP, (AMB_BSM_OVERLAY_GAIN_STRIP * 0.86f) * luma, wr, wg, wb);
    }
#else
    (void)now_ms;
#endif
}
