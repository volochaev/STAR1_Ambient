/**
 * @file base_scene.c
 * @brief Base ambient scene lifecycle and brightness orchestration.
 */
#include "base_scene.h"

#include <math.h>

#include "ambient_tuning.h"
#include "zones.h"

uint8_t g_night_mode_state = 0;
extern motion_profile_t can_ambient_get_motion_profile(void);
extern float can_ambient_get_hvac_fan_level(void);

#define BRIDGE_DURATION_MS AMB_BRIDGE_DURATION_MS

static motion_scale_diag_t g_motion_scale_diag = {0};

/* Clamp float into normalized [0..1] range. */
static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

/* Track diagnostics for raw/capped scene motion scale. */
static void motion_scale_diag_note(float raw_scale, float capped_scale)
{
    if (g_motion_scale_diag.samples == 0u) {
        g_motion_scale_diag.raw_min = raw_scale;
        g_motion_scale_diag.raw_max = raw_scale;
        g_motion_scale_diag.capped_min = capped_scale;
        g_motion_scale_diag.capped_max = capped_scale;
    } else {
        if (raw_scale < g_motion_scale_diag.raw_min) g_motion_scale_diag.raw_min = raw_scale;
        if (raw_scale > g_motion_scale_diag.raw_max) g_motion_scale_diag.raw_max = raw_scale;
        if (capped_scale < g_motion_scale_diag.capped_min) g_motion_scale_diag.capped_min = capped_scale;
        if (capped_scale > g_motion_scale_diag.capped_max) g_motion_scale_diag.capped_max = capped_scale;
    }
    g_motion_scale_diag.last_raw = raw_scale;
    g_motion_scale_diag.last_capped = capped_scale;
    g_motion_scale_diag.samples++;
}

/* Export motion-scale diagnostic snapshot. */
void base_scene_get_motion_scale_diag(motion_scale_diag_t *out)
{
    if (!out) return;
    *out = g_motion_scale_diag;
}

/* Reset motion-scale diagnostic accumulators. */
void base_scene_reset_motion_scale_diag(void)
{
    g_motion_scale_diag.raw_min = 0.0f;
    g_motion_scale_diag.raw_max = 0.0f;
    g_motion_scale_diag.capped_min = 0.0f;
    g_motion_scale_diag.capped_max = 0.0f;
    g_motion_scale_diag.last_raw = 0.0f;
    g_motion_scale_diag.last_capped = 0.0f;
    g_motion_scale_diag.samples = 0u;
    g_motion_scale_diag.cap_low_hits = 0u;
    g_motion_scale_diag.cap_high_hits = 0u;
}

static float motion_profile_smooth_alpha_mul(void)
{
    motion_profile_t profile = can_ambient_get_motion_profile();
    if (profile == MOTION_PROFILE_CALM) {
        return AMB_PROFILE_SMOOTH_ALPHA_MUL_CALM;
    }
    if (profile == MOTION_PROFILE_SPORT) {
        return AMB_PROFILE_SMOOTH_ALPHA_MUL_SPORT;
    }
    return AMB_PROFILE_SMOOTH_ALPHA_MUL_LUXURY;
}

static float scene_motion_speed_scale(void)
{
    float profile_scale = AMB_MOTION_PROFILE_SCALE_LUXURY;
    float fan_scale = 1.0f;
    float fan = can_ambient_get_hvac_fan_level();
    motion_profile_t profile = can_ambient_get_motion_profile();
    float raw;
    float capped;

    if (profile == MOTION_PROFILE_CALM) {
        profile_scale = AMB_MOTION_PROFILE_SCALE_CALM;
    } else if (profile == MOTION_PROFILE_SPORT) {
        profile_scale = AMB_MOTION_PROFILE_SCALE_SPORT;
    }

    if (fan < 0.0f) fan = 0.0f;
    if (fan > 1.0f) fan = 1.0f;
#if AMB_ENABLE_HVAC_FAN_MOTION_MOD
    fan_scale = AMB_HVAC_FAN_MOTION_MIN + (AMB_HVAC_FAN_MOTION_MAX - AMB_HVAC_FAN_MOTION_MIN) * fan;
#endif

    raw = profile_scale * fan_scale;
    capped = raw;
    if (capped < AMB_THEME_DYNAMIC_SCALE_MIN) {
        capped = AMB_THEME_DYNAMIC_SCALE_MIN;
        g_motion_scale_diag.cap_low_hits++;
    }
    if (capped > AMB_THEME_DYNAMIC_SCALE_MAX) {
        capped = AMB_THEME_DYNAMIC_SCALE_MAX;
        g_motion_scale_diag.cap_high_hits++;
    }
    motion_scale_diag_note(raw, capped);

#if AMB_ENABLE_NIGHT_CALM
    if (g_night_mode_state) {
        capped *= AMB_NIGHT_MOTION_SPEED_SCALE;
    }
#endif
    return capped;
}

static void base_scene_apply_global_brightness(led_runtime_strip_t *ws, base_scene_t *pl)
{
    float a;
    float target;
    float current;

    if (!ws || !pl) return;

    target = clamp01f(pl->calc_brightness);
    if (pl->stage == BASE_SCENE_ACTIVE && !pl->crossfade_active && target > 0.02f) {
        float t = (float)HAL_GetTick() * 0.001f;
        float w = 0.5f + 0.5f * sinf(2.0f * (float)M_PI * AMB_IDLE_MICRO_MOTION_HZ * t);
        float motion = 1.0f + AMB_IDLE_MICRO_MOTION_AMPLITUDE * (w - 0.5f) * 2.0f;
        target = clamp01f(target * motion);
    }

    current = clamp01f(pl->global_brightness_smooth);
    a = AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA;
    if (pl->stage == BASE_SCENE_OUTRO) {
        a = AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA;
    } else if (pl->stage == BASE_SCENE_INTRO || pl->stage == BASE_SCENE_BRIDGE) {
        a = AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA;
    }
    a *= motion_profile_smooth_alpha_mul();
    if (a < 0.0f) a = 0.0f;
    if (a > 0.98f) a = 0.98f;

    current = current * a + target * (1.0f - a);
    if (fabsf(target - current) < 0.0005f) {
        current = target;
    }

    pl->global_brightness_smooth = clamp01f(current);
    led_runtime_set_global_brightness(ws, pl->global_brightness_smooth);
}

static void clear_unused_leds(led_runtime_strip_t *ws)
{
    uint16_t i;
    const zone_map_t *zm;

    if (!ws) return;
    zm = &g_zone_map[ZONE_STRIP];
    if (!zm || zm->first == 0u) return;

    for (i = 0u; i < zm->first; ++i) {
        led_runtime_set_pixel_rgb(ws, i, 0u, 0u, 0u);
    }
}

void base_scene_refresh_brightness(base_scene_t *pl)
{
    if (!pl) return;
    pl->calc_brightness = clamp01f(pl->scene_brightness * pl->runtime_dimming);
}

void base_scene_init(base_scene_t *pl)
{
    if (!pl) return;

    pl->stage = BASE_SCENE_IDLE;
    pl->t0_ms = 0u;
    pl->calc_brightness = 0.7f;
    pl->scene_brightness = 0.7f;
    pl->runtime_dimming = 1.0f;

    pl->intro.active = 0u;
    pl->intro.fx_id = FX_WELCOME;
    pl->intro.base_br = 0.0f;
    pl->intro.start_ms = 0u;
    pl->intro.duration_ms = AMB_INTRO_DURATION_MS;
    pl->intro.first = 0u;
    pl->intro.count = 0u;

    pl->outro.active = 0u;
    pl->outro.fx_id = FX_GOODBYE;
    pl->outro.base_br = 0.0f;
    pl->outro.start_ms = 0u;
    pl->outro.duration_ms = AMB_OUTRO_DURATION_MS;
    pl->outro.first = 0u;
    pl->outro.count = 0u;

    pl->st_scene.t = 0.0f;
    pl->st_scene.speed = scene_motion_speed_scale();
    pl->st_scene.phase = 0.0f;
    pl->st_scene.a = 0.0f;
    pl->st_scene.b = 0.0f;

    pl->auto_rotate_enabled = 0u;
    pl->oem_color = 0u;
    pl->scene_start_ms = 0u;
    pl->crossfade_active = 0u;
    pl->st_scene_next.t = 0.0f;
    pl->st_scene_next.speed = 0.0f;
    pl->st_scene_next.phase = 0.0f;
    pl->st_scene_next.a = 0.0f;
    pl->st_scene_next.b = 0.0f;
    pl->crossfade_start_ms = 0u;
    pl->crossfade_blend_smooth = 0.0f;
    pl->bridge_blend_smooth = 0.0f;
    pl->global_brightness_smooth = clamp01f(pl->calc_brightness);
}

void base_scene_set_active(base_scene_t *pl)
{
    if (!pl) return;
    pl->stage = BASE_SCENE_ACTIVE;
    pl->t0_ms = HAL_GetTick();
    pl->scene_start_ms = pl->t0_ms;
    pl->crossfade_active = 0u;
    pl->bridge_blend_smooth = 0.0f;
    pl->crossfade_blend_smooth = 0.0f;
    pl->st_scene.speed = scene_motion_speed_scale();
}

void base_scene_start_intro(led_runtime_strip_t *ws, base_scene_t *pl)
{
    const zone_map_t *zm;
    uint16_t i;

    if (!ws || !pl) return;

    zm = &g_zone_map[ZONE_STRIP];
    pl->intro.active = 1u;
    pl->intro.start_ms = HAL_GetTick();
    pl->intro.duration_ms = AMB_INTRO_DURATION_MS;
    pl->intro.first = zm ? zm->first : 0u;
    pl->intro.count = zm ? zm->count : 0u;

    for (i = 0u; i < pl->intro.count; ++i) {
        led_runtime_set_pixel_rgb(ws, (uint16_t)(pl->intro.first + i), 0u, 0u, 0u);
    }
    led_runtime_set_global_brightness(ws, 0.0f);
    pl->global_brightness_smooth = 0.0f;

    pl->stage = BASE_SCENE_INTRO;
    pl->t0_ms = pl->intro.start_ms;
    pl->bridge_blend_smooth = 0.0f;
}

void base_scene_start_outro(led_runtime_strip_t *ws, base_scene_t *pl)
{
    const zone_map_t *zm;

    if (!ws || !pl) return;

    zm = &g_zone_map[ZONE_STRIP];
    pl->outro.active = 1u;
    pl->outro.start_ms = HAL_GetTick();
    pl->outro.duration_ms = AMB_OUTRO_DURATION_MS;
    pl->outro.first = zm ? zm->first : 0u;
    pl->outro.count = zm ? zm->count : 0u;

    pl->stage = BASE_SCENE_OUTRO;
    pl->t0_ms = pl->outro.start_ms;
}

void base_scene_tick(led_runtime_strip_t *ws, base_scene_t *pl, uint32_t delta_ms)
{
    uint32_t now;

    if (!ws || !pl) return;

    if (pl->stage != BASE_SCENE_IDLE) {
        led_runtime_power_set(1u);
    } else {
        led_runtime_power_set(0u);
        return;
    }

    now = HAL_GetTick();
    pl->st_scene.speed = scene_motion_speed_scale();
    pl->st_scene.t += ((float)delta_ms * 0.001f) * pl->st_scene.speed;

    switch (pl->stage) {
    case BASE_SCENE_INTRO:
        if ((now - pl->intro.start_ms) >= pl->intro.duration_ms) {
            pl->intro.active = 0u;
            pl->stage = BASE_SCENE_BRIDGE;
            pl->t0_ms = now;
            pl->bridge_blend_smooth = 0.0f;
        }
        break;

    case BASE_SCENE_BRIDGE:
        if ((now - pl->t0_ms) >= BRIDGE_DURATION_MS) {
            pl->stage = BASE_SCENE_ACTIVE;
            pl->t0_ms = now;
            pl->scene_start_ms = now;
            pl->bridge_blend_smooth = 0.0f;
        }
        break;

    case BASE_SCENE_OUTRO:
        if ((now - pl->outro.start_ms) >= pl->outro.duration_ms) {
            pl->outro.active = 0u;
            pl->stage = BASE_SCENE_IDLE;
            led_runtime_power_set(0u);
        }
        break;

    case BASE_SCENE_ACTIVE:
    default:
        break;
    }

    base_scene_apply_global_brightness(ws, pl);
    clear_unused_leds(ws);
}
