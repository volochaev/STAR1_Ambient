#include "runtime_render.h"

#include <math.h>
#include <string.h>

#include "ambient.h"
#include "board_dispatch.h"
#include "ambient_config.h"
#include "ambient_state_store.h"
#include "event_layer.h"
#include "led_runtime.h"
#include "scene_color_model.h"
#include "types.h"
#include "zones.h"

extern const zone_map_t g_zone_map[ZONE_MAX];
volatile float g_dbg_dim_target = 1.0f;
volatile float g_dbg_dim_applied = 1.0f;
volatile float g_dbg_sun_target = 1.0f;
volatile float g_dbg_sun_applied = 1.0f;
volatile float g_dbg_cabin_base_gain = 1.0f;
volatile float g_dbg_cabin_rev_gain = 1.0f;
volatile float g_dbg_cabin_final_gain = 1.0f;
typedef struct {
    led_runtime_strip_t *strip;
    uint8_t used;
    uint8_t initialized;
    uint16_t led_count;
    /* Previous postprocessed frame for per-channel slew limiting. */
    uint8_t prev_rgb[AMB_MAX_CROSSFADE_LEDS * 3u];
} runtime_slew_state_t;

static runtime_slew_state_t g_slew[4];

static float runtime_clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static uint8_t runtime_u8_mul(float v)
{
    if (v <= 0.0f) return 0u;
    if (v >= 255.0f) return 255u;
    return (uint8_t)(v + 0.5f);
}

#if AMB_ENABLE_DUAL_COLOR_BREATH
static void runtime_dual_breath_accent_rgb(uint8_t *r, uint8_t *g, uint8_t *b)
{
    ambient_state_snapshot_t ambient_state;
    scene_color_model_t scene_model;

    if (!r || !g || !b) return;
    ambient_state_store_get_snapshot(&ambient_state);
    scene_color_model_from_ambient(&ambient_state,
                                   can_ambient_get_motion_profile(),
                                   can_ambient_is_night_mode(),
                                   &scene_model);
    if (scene_model.valid) {
        *r = scene_model.accent_a.r;
        *g = scene_model.accent_a.g;
        *b = scene_model.accent_a.b;
    } else {
        *r = 180u;
        *g = 110u;
        *b = 255u;
    }
}

static float runtime_dual_breath_mix(zone_id_t z, float t_sec)
{
    float period = AMB_DUAL_BREATH_PERIOD_SEC;
    float base_mix = AMB_DUAL_BREATH_MIX_STRIP;
    float phase = 0.0f;
    float s;
    if (period < 2.0f) period = 2.0f;
    if (z == ZONE_HANDLE) {
        base_mix = AMB_DUAL_BREATH_MIX_HANDLE;
        phase = 0.8f;
    } else if (z == ZONE_STORAGE) {
        base_mix = AMB_DUAL_BREATH_MIX_STORAGE;
        phase = 1.6f;
    } else if (z == ZONE_FOOTWELL) {
        base_mix = AMB_DUAL_BREATH_MIX_FOOTWELL;
        phase = 2.2f;
    }
    s = 0.5f + 0.5f * sinf(2.0f * 3.14159265f * (t_sec / period) + phase);
    s = s * s * (3.0f - 2.0f * s); /* smoothstep */
    return base_mix * s;
}

static void runtime_apply_dual_breath_zone(const zone_map_t *zm,
                                           zone_id_t zone_id,
                                           uint8_t ar,
                                           uint8_t ag,
                                           uint8_t ab,
                                           float t_sec)
{
    uint16_t i;
    float mix;
    if (!zm || !zm->strip || !zm->strip->rgb || zm->count == 0u) return;
    mix = runtime_dual_breath_mix(zone_id, t_sec);
    if (mix < 0.001f) return;
    if (mix > 0.85f) mix = 0.85f;
    for (i = 0u; i < zm->count; ++i) {
        uint32_t idx = (uint32_t)(zm->first + i) * 3u;
        uint8_t br = zm->strip->rgb[idx + 0u];
        uint8_t bg = zm->strip->rgb[idx + 1u];
        uint8_t bb = zm->strip->rgb[idx + 2u];
        zm->strip->rgb[idx + 0u] = (uint8_t)((float)br * (1.0f - mix) + (float)ar * mix + 0.5f);
        zm->strip->rgb[idx + 1u] = (uint8_t)((float)bg * (1.0f - mix) + (float)ag * mix + 0.5f);
        zm->strip->rgb[idx + 2u] = (uint8_t)((float)bb * (1.0f - mix) + (float)ab * mix + 0.5f);
    }
}
#endif

static runtime_slew_state_t* runtime_get_slew_state(led_runtime_strip_t *strip)
{
    uint8_t i;

    if (!strip || strip->led_count == 0u || strip->led_count > AMB_MAX_CROSSFADE_LEDS) return NULL;

    for (i = 0u; i < (uint8_t)(sizeof(g_slew) / sizeof(g_slew[0])); ++i) {
        if (g_slew[i].used && g_slew[i].strip == strip) {
            return &g_slew[i];
        }
    }

    for (i = 0u; i < (uint8_t)(sizeof(g_slew) / sizeof(g_slew[0])); ++i) {
        if (!g_slew[i].used) {
            g_slew[i].used = 1u;
            g_slew[i].initialized = 0u;
            g_slew[i].strip = strip;
            g_slew[i].led_count = strip->led_count;
            return &g_slew[i];
        }
    }
    return NULL;
}

void runtime_render_reset_slew_for_strip(led_runtime_strip_t *strip)
{
    uint8_t i;

    if (!strip) return;
    for (i = 0u; i < (uint8_t)(sizeof(g_slew) / sizeof(g_slew[0])); ++i) {
        if (g_slew[i].used && g_slew[i].strip == strip) {
            g_slew[i].initialized = 0u;
            g_slew[i].led_count = strip->led_count;
            memset(g_slew[i].prev_rgb, 0, sizeof(g_slew[i].prev_rgb));
            return;
        }
    }
}

void runtime_render_push_black_frame(void)
{
    led_runtime_strip_t *processed[ZONE_MAX];
    uint8_t processed_count = 0u;
    uint8_t z;

    led_runtime_power_set(1u);

    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        led_runtime_strip_t *ws = g_zone_map[z].strip;
        uint8_t seen = 0u;
        uint8_t i;

        if (!ws || !ws->rgb) continue;
        for (i = 0u; i < processed_count; ++i) {
            if (processed[i] == ws) {
                seen = 1u;
                break;
            }
        }
        if (seen) continue;

        /* Several zones may share one physical strip; process each strip once. */
        processed[processed_count++] = ws;
        ws->dma_busy = 0u;
        if (ws->htim) {
            HAL_TIM_PWM_Stop_DMA(ws->htim, ws->tim_channel);
        }
        memset(ws->rgb, 0, (size_t)ws->led_count * 3u);
        led_runtime_set_global_brightness(ws, 0.0f);
        runtime_render_reset_slew_for_strip(ws);
    }

    board_dispatch_led_render_all();
    HAL_Delay(2);
    board_dispatch_led_render_all();
}

static void runtime_apply_motion_tint(led_runtime_strip_t *ws, motion_profile_t profile)
{
    float r_mul = AMB_MOTION_PROFILE_TINT_LUXURY_R;
    float g_mul = AMB_MOTION_PROFILE_TINT_LUXURY_G;
    float b_mul = AMB_MOTION_PROFILE_TINT_LUXURY_B;
    uint32_t n;
    uint32_t i;

    if (!ws || !ws->rgb) return;
    if (profile == MOTION_PROFILE_CALM) {
        r_mul = AMB_MOTION_PROFILE_TINT_CALM_R;
        g_mul = AMB_MOTION_PROFILE_TINT_CALM_G;
        b_mul = AMB_MOTION_PROFILE_TINT_CALM_B;
    } else if (profile == MOTION_PROFILE_SPORT) {
        r_mul = AMB_MOTION_PROFILE_TINT_SPORT_R;
        g_mul = AMB_MOTION_PROFILE_TINT_SPORT_G;
        b_mul = AMB_MOTION_PROFILE_TINT_SPORT_B;
    }

    n = (uint32_t)ws->led_count * 3u;
    for (i = 0u; i < n; i += 3u) {
        ws->rgb[i + 0u] = runtime_u8_mul((float)ws->rgb[i + 0u] * r_mul);
        ws->rgb[i + 1u] = runtime_u8_mul((float)ws->rgb[i + 1u] * g_mul);
        ws->rgb[i + 2u] = runtime_u8_mul((float)ws->rgb[i + 2u] * b_mul);
    }
}

static float runtime_drive_mode_boost_gain(uint32_t now, motion_profile_t profile)
{
    static motion_profile_t prev = (motion_profile_t)0xFFu;
    static uint8_t active = 0u;
    static uint32_t start_ms = 0u;
    float max_gain = AMB_DRIVE_MODE_BOOST_GAIN_LUXURY;
    uint32_t elapsed;
    float t;
    float env;

    if (profile == MOTION_PROFILE_CALM) {
        max_gain = AMB_DRIVE_MODE_BOOST_GAIN_CALM;
    } else if (profile == MOTION_PROFILE_SPORT) {
        max_gain = AMB_DRIVE_MODE_BOOST_GAIN_SPORT;
    }

    if (prev == (motion_profile_t)0xFFu) {
        prev = profile;
        return 1.0f;
    }

    if (profile != prev) {
        prev = profile;
        active = 1u;
        start_ms = now;
    }

    if (!active || AMB_DRIVE_MODE_BOOST_DURATION_MS == 0u) {
        return 1.0f;
    }

    elapsed = (now >= start_ms) ? (now - start_ms) : 0u;
    if (elapsed >= AMB_DRIVE_MODE_BOOST_DURATION_MS) {
        active = 0u;
        return 1.0f;
    }

    t = runtime_clamp01((float)elapsed / (float)AMB_DRIVE_MODE_BOOST_DURATION_MS);
    env = sinf(3.14159265f * t);
    env = env * env;
    return 1.0f + max_gain * env;
}

static void runtime_apply_boost(led_runtime_strip_t *ws, float gain)
{
    uint32_t n;
    uint32_t i;

    if (!ws || !ws->rgb || gain <= 1.0001f) return;
    n = (uint32_t)ws->led_count * 3u;
    for (i = 0u; i < n; ++i) {
        ws->rgb[i] = runtime_u8_mul((float)ws->rgb[i] * gain);
    }
}

static void runtime_apply_energy_trim(led_runtime_strip_t *ws, motion_profile_t profile, uint8_t night_mode)
{
#if AMB_ENABLE_ENERGY_AWARE_TRIM
    float sat = AMB_ENERGY_SAT_LUXURY;
    float wp = night_mode ? AMB_ENERGY_WHITEPOINT_NIGHT : 1.0f;
    uint32_t n;
    uint32_t i;

    if (!ws || !ws->rgb) return;
    if (profile == MOTION_PROFILE_CALM) {
        sat = AMB_ENERGY_SAT_CALM;
    } else if (profile == MOTION_PROFILE_SPORT) {
        sat = AMB_ENERGY_SAT_SPORT;
    }

    n = (uint32_t)ws->led_count * 3u;
    for (i = 0u; i < n; i += 3u) {
        float r = (float)ws->rgb[i + 0u];
        float g = (float)ws->rgb[i + 1u];
        float b = (float)ws->rgb[i + 2u];
        float y = 0.299f * r + 0.587f * g + 0.114f * b;
        float rr = y + (r - y) * sat;
        float gg = y + (g - y) * sat;
        float bb = y + (b - y) * sat;
        ws->rgb[i + 0u] = runtime_u8_mul(rr * wp);
        ws->rgb[i + 1u] = runtime_u8_mul(gg * wp);
        ws->rgb[i + 2u] = runtime_u8_mul(bb * wp);
    }
#else
    (void)ws;
    (void)profile;
    (void)night_mode;
#endif
}

static void runtime_apply_cabin_modifiers(led_runtime_strip_t *ws,
                                          float dim_scale,
                                          float sun_gain,
                                          float reverse_blend)
{
    uint32_t n;
    uint32_t i;
    float base_gain;
    float rev_gain;
    float rev_r;
    float rev_g;
    float rev_b;
    float sun_warm;

    if (!ws || !ws->rgb) return;
    if (dim_scale < 0.20f) dim_scale = 0.20f;
    if (dim_scale > 1.20f) dim_scale = 1.20f;
    if (sun_gain < 1.0f) sun_gain = 1.0f;
    if (sun_gain > 1.30f) sun_gain = 1.30f;
    if (reverse_blend < 0.0f) reverse_blend = 0.0f;
    if (reverse_blend > 1.0f) reverse_blend = 1.0f;

    base_gain = dim_scale * sun_gain;
    rev_gain = 1.0f + (AMB_REVERSE_SCENE_GAIN - 1.0f) * reverse_blend;
    base_gain *= rev_gain;
    rev_r = 1.0f + (AMB_REVERSE_SCENE_TINT_R - 1.0f) * reverse_blend;
    rev_g = 1.0f + (AMB_REVERSE_SCENE_TINT_G - 1.0f) * reverse_blend;
    rev_b = 1.0f + (AMB_REVERSE_SCENE_TINT_B - 1.0f) * reverse_blend;
    sun_warm = (sun_gain - 1.0f);
    g_dbg_cabin_base_gain = base_gain;
    g_dbg_cabin_rev_gain = rev_gain;
    g_dbg_cabin_final_gain = base_gain * rev_gain;

    n = (uint32_t)ws->led_count * 3u;
    for (i = 0u; i < n; i += 3u) {
        float r = (float)ws->rgb[i + 0u];
        float g = (float)ws->rgb[i + 1u];
        float b = (float)ws->rgb[i + 2u];

        r *= base_gain * rev_r * (1.0f + 0.22f * sun_warm);
        g *= base_gain * rev_g * (1.0f + 0.10f * sun_warm);
        b *= base_gain * rev_b * (1.0f - 0.08f * sun_warm);

        ws->rgb[i + 0u] = runtime_u8_mul(r);
        ws->rgb[i + 1u] = runtime_u8_mul(g);
        ws->rgb[i + 2u] = runtime_u8_mul(b);
    }
}

static void runtime_apply_low_level_chroma(led_runtime_strip_t *ws)
{
#if AMB_LOW_LEVEL_CHROMA_ENABLE
    uint32_t n;
    uint32_t i;
    const float s0 = AMB_LOW_LEVEL_CHROMA_START;
    const float s1 = AMB_LOW_LEVEL_CHROMA_END;

    if (!ws || !ws->rgb) return;
    if (s0 <= s1) return;

    n = (uint32_t)ws->led_count * 3u;
    for (i = 0u; i < n; i += 3u) {
        float r = (float)ws->rgb[i + 0u];
        float g = (float)ws->rgb[i + 1u];
        float b = (float)ws->rgb[i + 2u];
        float y;
        float luma;
        float t;
        float sat_gain;
        float floor_code;

        if ((r + g + b) <= 0.0f) continue;

        y = 0.299f * r + 0.587f * g + 0.114f * b;
        luma = y * (1.0f / 255.0f);
        if (luma >= s0) continue;

        t = (s0 - luma) / (s0 - s1);
        t = runtime_clamp01(t);
        sat_gain = 1.0f + (AMB_LOW_LEVEL_SAT_BOOST - 1.0f) * t;

        r = y + (r - y) * sat_gain;
        g = y + (g - y) * sat_gain;
        b = y + (b - y) * sat_gain;

        floor_code = 1.0f + ((float)AMB_LOW_LEVEL_CODE_FLOOR - 1.0f) * t;
        if (r > 0.0f && r < floor_code) r = floor_code;
        if (g > 0.0f && g < floor_code) g = floor_code;
        if (b > 0.0f && b < floor_code) b = floor_code;

        ws->rgb[i + 0u] = runtime_u8_mul(r);
        ws->rgb[i + 1u] = runtime_u8_mul(g);
        ws->rgb[i + 2u] = runtime_u8_mul(b);
    }
#else
    (void)ws;
#endif
}

static uint8_t runtime_profile_slew_step(motion_profile_t profile, float boost_gain, float fan_level)
{
    float step = (float)AMB_FRAME_SLEW_STEP_BASE;

    if (profile == MOTION_PROFILE_CALM) {
        step *= AMB_FRAME_SLEW_SCALE_CALM;
    } else if (profile == MOTION_PROFILE_SPORT) {
        step *= AMB_FRAME_SLEW_SCALE_SPORT;
    } else {
        step *= AMB_FRAME_SLEW_SCALE_LUXURY;
    }
    if (boost_gain > 1.0f) {
        step *= (1.0f + (boost_gain - 1.0f) * 1.6f);
    }
#if AMB_ENABLE_HVAC_FAN_MOTION_MOD
    if (fan_level < 0.0f) fan_level = 0.0f;
    if (fan_level > 1.0f) fan_level = 1.0f;
    step *= (AMB_HVAC_FAN_MOTION_MIN + (AMB_HVAC_FAN_MOTION_MAX - AMB_HVAC_FAN_MOTION_MIN) * fan_level);
#else
    (void)fan_level;
#endif
    if (step < 1.0f) step = 1.0f;
    if (step > 60.0f) step = 60.0f;
    return (uint8_t)(step + 0.5f);
}

static void runtime_apply_frame_slew(led_runtime_strip_t *ws, uint8_t max_step)
{
    runtime_slew_state_t *st;
    uint32_t n;
    uint32_t i;

    if (!ws || !ws->rgb) return;
    st = runtime_get_slew_state(ws);
    if (!st) return;

    n = (uint32_t)ws->led_count * 3u;
    if (!st->initialized || st->led_count != ws->led_count) {
        st->initialized = 1u;
        st->led_count = ws->led_count;
        memcpy(st->prev_rgb, ws->rgb, n);
        return;
    }

    for (i = 0u; i < n; ++i) {
        uint8_t prev = st->prev_rgb[i];
        uint8_t cur = ws->rgb[i];
        /* Clamp per-frame delta to suppress visible stepping on aggressive FX/profile switches. */
        if (cur > prev) {
            uint16_t lim = (uint16_t)prev + (uint16_t)max_step;
            if (cur > lim) cur = (uint8_t)lim;
        } else {
            uint16_t lim = (prev > max_step) ? (uint16_t)(prev - max_step) : 0u;
            if (cur < lim) cur = (uint8_t)lim;
        }
        ws->rgb[i] = cur;
        st->prev_rgb[i] = cur;
    }
}

void runtime_render_postprocess_frame(uint32_t now)
{
    led_runtime_strip_t *processed[ZONE_MAX];
    uint8_t processed_count = 0u;
    motion_profile_t profile = can_ambient_get_motion_profile();
    can_bsm_state_t bsm = can_ambient_get_bsm_state();
    uint8_t night_mode = can_ambient_is_night_mode();
    float boost_gain = runtime_drive_mode_boost_gain(now, profile);
    float fan_level = can_ambient_get_hvac_fan_level();
    uint8_t slew_step = runtime_profile_slew_step(profile, boost_gain, fan_level);
    float bsm_level = bsm.left_level;
    uint8_t bsm_active = 0u;
    uint8_t hvac_temp_event_active = event_layer_is_hvac_temp_active(now);
    uint8_t apply_motion_post = 1u;
    static float s_dim_scale = 1.0f;
    static float s_sun_gain = 1.0f;
    static float s_reverse_blend = 0.0f;
    uint8_t reverse_active = can_ambient_is_reverse_active() ? 1u : 0u;
    float dim_target = can_ambient_get_ilm_dim_scale_for_board();
    float sun_target = can_ambient_get_sun_gain_for_board();
    float reverse_target = reverse_active ? 1.0f : 0.0f;
    uint8_t z;
#if AMB_ENABLE_DUAL_COLOR_BREATH
    uint8_t accent_r = 0u;
    uint8_t accent_g = 0u;
    uint8_t accent_b = 0u;
#endif

    if (AMB_ILM_DIM_SMOOTH_ALPHA < 0.0f) {
        s_dim_scale = dim_target;
    } else {
        float a = AMB_ILM_DIM_SMOOTH_ALPHA;
        if (a > 0.98f) a = 0.98f;
        s_dim_scale = s_dim_scale * a + dim_target * (1.0f - a);
    }
    if (AMB_SUN_GAIN_SMOOTH_ALPHA < 0.0f) {
        s_sun_gain = sun_target;
    } else {
        float a = AMB_SUN_GAIN_SMOOTH_ALPHA;
        if (a > 0.98f) a = 0.98f;
        s_sun_gain = s_sun_gain * a + sun_target * (1.0f - a);
    }
    if (AMB_REVERSE_SCENE_SMOOTH_ALPHA < 0.0f) {
        s_reverse_blend = reverse_target;
    } else {
        float a = AMB_REVERSE_SCENE_SMOOTH_ALPHA;
        if (a > 0.98f) a = 0.98f;
        s_reverse_blend = s_reverse_blend * a + reverse_target * (1.0f - a);
    }
    g_dbg_dim_target = dim_target;
    g_dbg_sun_target = sun_target;
    g_dbg_dim_applied = s_dim_scale;
    g_dbg_sun_applied = s_sun_gain;
    /* Reverse edge should not repaint strip with context-hold overlay:
     * it looks like fade-out/fade-in on bench logs. Keep only reverse tint path. */
    (void)now;

    if (bsm.right_level > bsm_level) bsm_level = bsm.right_level;
    bsm_active = (uint8_t)((bsm.left_active || bsm.right_active || (bsm_level > 0.03f)) ? 1u : 0u);
    if (bsm_active) {
        apply_motion_post = 0u;
        if (bsm_level > 0.30f) {
            slew_step = 255u;
        } else if (slew_step < 64u) {
            slew_step = 64u;
        }
    }
    if (hvac_temp_event_active) {
        /* Keep HVAC sweep shape crisp; default per-frame slew heavily smears moving head. */
        slew_step = 255u;
    }

#if AMB_ENABLE_DUAL_COLOR_BREATH
    if (!bsm_active && !reverse_active && !hvac_temp_event_active) {
        runtime_dual_breath_accent_rgb(&accent_r, &accent_g, &accent_b);
        runtime_apply_dual_breath_zone(&g_zone_map[ZONE_STRIP], ZONE_STRIP, accent_r, accent_g, accent_b, now * 0.001f);
        runtime_apply_dual_breath_zone(&g_zone_map[ZONE_HANDLE], ZONE_HANDLE, accent_r, accent_g, accent_b, now * 0.001f);
        runtime_apply_dual_breath_zone(&g_zone_map[ZONE_STORAGE], ZONE_STORAGE, accent_r, accent_g, accent_b, now * 0.001f);
        runtime_apply_dual_breath_zone(&g_zone_map[ZONE_FOOTWELL], ZONE_FOOTWELL, accent_r, accent_g, accent_b, now * 0.001f);
    }
#endif

    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        led_runtime_strip_t *ws = g_zone_map[z].strip;
        uint8_t seen = 0u;
        uint8_t i;

        if (!ws) continue;
        for (i = 0u; i < processed_count; ++i) {
            if (processed[i] == ws) {
                seen = 1u;
                break;
            }
        }
        if (seen) continue;

        processed[processed_count++] = ws;
        runtime_apply_cabin_modifiers(ws, s_dim_scale, s_sun_gain, s_reverse_blend);
        if (apply_motion_post) {
            runtime_apply_motion_tint(ws, profile);
            runtime_apply_energy_trim(ws, profile, night_mode);
            runtime_apply_boost(ws, boost_gain);
        }
        runtime_apply_low_level_chroma(ws);
        runtime_apply_frame_slew(ws, slew_step);
    }
}
