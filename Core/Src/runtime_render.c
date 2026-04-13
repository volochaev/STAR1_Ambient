#include "runtime_render.h"

#include <math.h>
#include <string.h>

#include "ambient.h"
#include "board_dispatch.h"
#include "features.h"
#include "led_runtime.h"
#include "types.h"
#include "zones.h"

extern const zone_map_t g_zone_map[ZONE_MAX];

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
    uint8_t apply_motion_post = 1u;
    uint8_t z;

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
        if (apply_motion_post) {
        runtime_apply_motion_tint(ws, profile);
        runtime_apply_energy_trim(ws, profile, night_mode);
        runtime_apply_boost(ws, boost_gain);
        }
        runtime_apply_frame_slew(ws, slew_step);
    }
}
