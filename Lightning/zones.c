#include "zones.h"
#include "ambient_config.h"
#include "ambient.h"
#include "ambient_internal.h"
#include "ambient_tuning.h"
#include "event_layer.h"
#include "led_runtime.h"
#include "overlay_palette.h"
#include "scene_preset.h"
#include "color_worlds.h"
#include "zone_roles.h"
#include "brightness_pipeline.h"
#include "scene_color_model.h"
#include "runtime_debug_hooks.h"
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

static uint8_t zone_is_tiny(const zone_map_t *zm)
{
    return (uint8_t)((zm && zm->count <= 2u) ? 1u : 0u);
}

static float comfort_hvac_zone_dim(zone_id_t z);
static void scene_entity_sample_zone(const scene_color_entity_t *e,
                                     const scene_preset_t *preset,
                                     zone_id_t z,
                                     float pos01,
                                     float t_sec,
                                     uint8_t *r,
                                     uint8_t *g,
                                     uint8_t *b);

static void apply_strip_scene_model(const scene_color_entity_t *entity,
                                    const scene_preset_t *preset,
                                    const base_scene_t *pl)
{
    const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
    uint16_t i;
    float t_sec;

    if (!entity || !entity->valid || !pl || !preset) return;
    if (!zm || !zm->strip || zm->count == 0u) return;

    t_sec = (float)pl->t0_ms * 0.001f;
    for (i = 0u; i < zm->count; ++i) {
        float pos = (zm->count > 1u) ? ((float)i / (float)(zm->count - 1u)) : 0.0f;
        uint8_t r = 0u, g = 0u, b = 0u;
        float br = brightness_pipeline_clamp01(pl->calc_brightness);
        scene_entity_sample_zone(entity, preset, ZONE_STRIP, pos, t_sec + pos * 0.8f * preset->temporal_scale, &r, &g, &b);
        r = (uint8_t)((float)r * br);
        g = (uint8_t)((float)g * br);
        b = (uint8_t)((float)b * br);
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
    }
}

static void apply_strip_world_model(const color_world_selection_t *world,
                                    const scene_preset_t *preset,
                                    const base_scene_t *pl)
{
    const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
    uint16_t i;

    if (!world || !world->active || !pl || !preset) return;
    if (!zm || !zm->strip || zm->count == 0u) return;

    for (i = 0u; i < zm->count; ++i) {
        float pos = (zm->count > 1u) ? ((float)i / (float)(zm->count - 1u)) : 0.0f;
        uint8_t r = 0u, g = 0u, b = 0u;
        float br = brightness_pipeline_clamp01(pl->calc_brightness);
        color_world_sample(world, ZONE_STRIP, pos, pl->t0_ms, preset, &r, &g, &b);
        r = (uint8_t)((float)r * br);
        g = (uint8_t)((float)g * br);
        b = (uint8_t)((float)b * br);
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
    }
}

static void scene_entity_sample_zone(const scene_color_entity_t *e,
                                     const scene_preset_t *preset,
                                     zone_id_t z,
                                     float pos01,
                                     float t_sec,
                                     uint8_t *r,
                                     uint8_t *g,
                                     uint8_t *b)
{
    float phase = 0.0f;
    float mix_wc;
    float mix_neutral;
    scene_rgb8_t wc;
    scene_rgb8_t out;

    if (!e || !e->valid || !r || !g || !b || !preset) return;
    if (pos01 < 0.0f) pos01 = 0.0f;
    if (pos01 > 1.0f) pos01 = 1.0f;

    if (z == ZONE_HANDLE) phase = 0.7f;
    else if (z == ZONE_STORAGE) phase = 1.4f;
    else if (z == ZONE_FOOTWELL) phase = 2.1f;

    mix_wc = 0.5f + 0.5f * sinf(2.0f * (float)M_PI * (t_sec * 0.06f) + phase + pos01 * 1.9f);
    wc.r = (uint8_t)((float)e->accent_warm.r * (1.0f - mix_wc) + (float)e->accent_cool.r * mix_wc + 0.5f);
    wc.g = (uint8_t)((float)e->accent_warm.g * (1.0f - mix_wc) + (float)e->accent_cool.g * mix_wc + 0.5f);
    wc.b = (uint8_t)((float)e->accent_warm.b * (1.0f - mix_wc) + (float)e->accent_cool.b * mix_wc + 0.5f);

    mix_neutral = 0.10f + 0.34f * (1.0f - e->energy) + preset->neutral_mix_bias;
    if (mix_neutral > 0.65f) mix_neutral = 0.65f;
    if (mix_neutral < 0.02f) mix_neutral = 0.02f;

    out.r = (uint8_t)((float)e->base.r * (0.70f / preset->contrast_gain) + (float)wc.r * (0.30f * preset->contrast_gain) + 0.5f);
    out.g = (uint8_t)((float)e->base.g * (0.70f / preset->contrast_gain) + (float)wc.g * (0.30f * preset->contrast_gain) + 0.5f);
    out.b = (uint8_t)((float)e->base.b * (0.70f / preset->contrast_gain) + (float)wc.b * (0.30f * preset->contrast_gain) + 0.5f);
    out.r = (uint8_t)((float)out.r * (1.0f - mix_neutral) + (float)e->neutral_soft.r * mix_neutral + 0.5f);
    out.g = (uint8_t)((float)out.g * (1.0f - mix_neutral) + (float)e->neutral_soft.g * mix_neutral + 0.5f);
    out.b = (uint8_t)((float)out.b * (1.0f - mix_neutral) + (float)e->neutral_soft.b * mix_neutral + 0.5f);

    *r = out.r;
    *g = out.g;
    *b = out.b;
}

static void apply_zone_fx_modern(zone_id_t z,
                                 const base_scene_t *pl,
                                 const scene_color_entity_t *entity,
                                 const scene_preset_t *preset,
                                 uint8_t effect_id)
{
    const zone_map_t *zm = &g_zone_map[z];
    float br;
    float t_sec;
    brightness_zone_eval_t br_eval;
    float rel = 1.0f;
    float speed_scale = 1.0f;
    float breathe_freq = 0.05f;

    if (!pl || !entity || !entity->valid || !preset) return;
    if (!zm || !zm->strip || zm->count == 0u) return;
    if (z == ZONE_STRIP) return;

    if (z == ZONE_HANDLE) rel = 1.06f;
    else if (z == ZONE_STORAGE) rel = 0.90f;
    else if (z == ZONE_FOOTWELL) rel = 0.84f;

    brightness_pipeline_eval_zone(pl->calc_brightness,
                                  z,
                                  rel,
                                  comfort_hvac_zone_dim(z),
                                  g_night_mode_state,
                                  &br_eval);
    br = br_eval.value;
    if (br <= 0.0f) {
        uint16_t i;
        for (i = 0u; i < zm->count; ++i) {
            led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), 0u, 0u, 0u);
        }
        return;
    }

    t_sec = (float)pl->t0_ms * 0.001f;
    if (entity->energy > 0.65f) speed_scale = 1.25f;
    if (entity->energy < 0.40f) speed_scale = 0.78f;
    breathe_freq *= speed_scale * preset->temporal_scale;

    if (effect_id == 0u) {
        uint8_t r, g, b;
        scene_entity_sample_zone(entity, preset, z, 0.5f, t_sec, &r, &g, &b);
        r = (uint8_t)((float)r * br);
        g = (uint8_t)((float)g * br);
        b = (uint8_t)((float)b * br);
        for (uint16_t i = 0u; i < zm->count; ++i) {
            led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
        }
        return;
    }

    if (effect_id == 1u) {
        float phase = (float)z * 1.3f;
        float s = 0.5f + 0.5f * sinf(2.0f * (float)M_PI * breathe_freq * t_sec + phase);
        float br_dyn = br * (0.42f + 0.58f * s);
        uint8_t r, g, b;
        scene_entity_sample_zone(entity, preset, z, 0.5f, t_sec, &r, &g, &b);
        r = (uint8_t)((float)r * br_dyn);
        g = (uint8_t)((float)g * br_dyn);
        b = (uint8_t)((float)b * br_dyn);
        for (uint16_t i = 0u; i < zm->count; ++i) {
            led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
        }
        return;
    }

    {
        float spread = zone_is_tiny(zm) ? 0.06f : 0.18f;
        float speed = (effect_id == 3u) ? 0.05f : 0.03f;
        speed *= speed_scale;
        for (uint16_t i = 0u; i < zm->count; ++i) {
            float pos = (zm->count > 1u) ? ((float)i / (float)(zm->count - 1u)) : 0.0f;
            uint8_t r, g, b;
            scene_entity_sample_zone(entity, preset, z, pos, t_sec + speed * t_sec * preset->temporal_scale + spread * pos, &r, &g, &b);
            r = (uint8_t)((float)r * br);
            g = (uint8_t)((float)g * br);
            b = (uint8_t)((float)b * br);
            led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
        }
    }
}

static void apply_zone_world(zone_id_t z,
                             const base_scene_t *pl,
                             const scene_preset_t *preset,
                             const color_world_selection_t *world)
{
    const zone_map_t *zm = &g_zone_map[z];
    float br;
    brightness_zone_eval_t br_eval;
    float rel = 1.0f;
    uint16_t i;

    if (!pl || !preset || !world || !world->active) return;
    if (!zm || !zm->strip || zm->count == 0u) return;
    if (z == ZONE_STRIP) return;

    if (z == ZONE_HANDLE) rel = 1.06f;
    else if (z == ZONE_STORAGE) rel = 0.90f;
    else if (z == ZONE_FOOTWELL) rel = 0.84f;

    brightness_pipeline_eval_zone(pl->calc_brightness,
                                  z,
                                  rel,
                                  comfort_hvac_zone_dim(z),
                                  g_night_mode_state,
                                  &br_eval);
    br = br_eval.value;
    if (br <= 0.0f) {
        for (i = 0u; i < zm->count; ++i) {
            led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), 0u, 0u, 0u);
        }
        return;
    }

    for (i = 0u; i < zm->count; ++i) {
        float pos = (zm->count > 1u) ? ((float)i / (float)(zm->count - 1u)) : 0.0f;
        uint8_t r = 0u, g = 0u, b = 0u;
        color_world_sample(world, z, pos, pl->t0_ms, preset, &r, &g, &b);
        r = (uint8_t)((float)r * br);
        g = (uint8_t)((float)g * br);
        b = (uint8_t)((float)b * br);
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
    }
}

static void render_zone_model_static(zone_id_t zone_id,
                                     const scene_color_entity_t *entity,
                                     const scene_preset_t *preset,
                                     float br_scale,
                                     float t_sec)
{
    const zone_map_t *zm = &g_zone_map[zone_id];
    uint16_t i;

    if (!entity || !entity->valid || !preset) return;
    if (!zm || !zm->strip || zm->count == 0u) return;
    if (br_scale <= 0.0f) {
        for (i = 0u; i < zm->count; ++i) {
            led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), 0u, 0u, 0u);
        }
        return;
    }

    if (br_scale > 1.0f) br_scale = 1.0f;
    for (i = 0u; i < zm->count; ++i) {
        float pos = (zm->count > 1u) ? ((float)i / (float)(zm->count - 1u)) : 0.0f;
        uint8_t r, g, b;
        scene_entity_sample_zone(entity, preset, zone_id, pos, t_sec + pos * 0.4f * preset->temporal_scale, &r, &g, &b);
        r = (uint8_t)((float)r * br_scale);
        g = (uint8_t)((float)g * br_scale);
        b = (uint8_t)((float)b * br_scale);
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
    }
}

/* Premium contrast preset:
 * Accent zones drift slowly between base tone and a shifted companion tone. */

typedef struct {
    volatile float oem_brightness;
    volatile float calc_brightness;
    volatile uint8_t flags; /* bit0=night bit1=crossfade bit2=bridge */
    volatile float zone_base[ZONE_MAX];
    volatile float zone_rel[ZONE_MAX];
    volatile float zone_boost[ZONE_MAX];
    volatile float zone_factor[ZONE_MAX];
    volatile float zone_comfort_dim[ZONE_MAX];
    volatile float zone_effective[ZONE_MAX];
} dbg_zone_calib_t;

volatile dbg_zone_calib_t g_dbg_zone_calib;

typedef struct {
    float value;
} overlay_env_t;

static float overlay_env_step(overlay_env_t *st, float target, float attack_alpha, float release_alpha)
{
    if (!st) return target;
    target = clamp01f(target);
    attack_alpha = clamp01f(attack_alpha);
    release_alpha = clamp01f(release_alpha);
    if (target >= st->value) {
        st->value += (target - st->value) * attack_alpha;
    } else {
        st->value += (target - st->value) * release_alpha;
    }
    st->value = clamp01f(st->value);
    return st->value;
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

static void apply_override_to_zone(zone_id_t zone, float level, uint8_t r, uint8_t g, uint8_t b)
{
    if (level <= 0.0f) return;
    if (level > 1.0f) level = 1.0f;
    zone_roles_submit(zone, ZONE_ROLE_ATTENTION_POINT, r, g, b, level, ZONE_BLEND_MAX);
}

static void apply_safety_to_zone(zone_id_t zone, float level, uint8_t r, uint8_t g, uint8_t b)
{
    if (level <= 0.0f) return;
    if (level > 1.0f) level = 1.0f;
    zone_roles_submit(zone, ZONE_ROLE_SAFETY_ALERT, r, g, b, level, ZONE_BLEND_OVERRIDE);
}

static void submit_m4_entity_roles(uint32_t now_ms)
{
    ambient_state_snapshot_t amb_state;
    scene_color_entity_t entity;
    scene_preset_t preset;
    float t = (float)now_ms * 0.001f;
    float pulse;
    float guidance_alpha;

    ambient_state_store_get_snapshot(&amb_state);
    scene_color_entity_from_ambient(&amb_state,
                                    can_ambient_get_motion_profile(),
                                    can_ambient_is_night_mode(),
                                    &entity);
    scene_preset_resolve(can_ambient_get_motion_profile(), can_ambient_is_night_mode(), &preset);
    if (!entity.valid) return;

    /* Light ambient-base reinforcement layer (does not replace base scene). */
    zone_roles_submit(ZONE_STRIP, ZONE_ROLE_AMBIENT_BASE,
                      entity.base.r, entity.base.g, entity.base.b,
                      0.08f, ZONE_BLEND_MAX);
    zone_roles_submit(ZONE_HANDLE, ZONE_ROLE_AMBIENT_BASE,
                      entity.base.r, entity.base.g, entity.base.b,
                      0.06f, ZONE_BLEND_MAX);
    zone_roles_submit(ZONE_STORAGE, ZONE_ROLE_AMBIENT_BASE,
                      entity.neutral_soft.r, entity.neutral_soft.g, entity.neutral_soft.b,
                      0.05f, ZONE_BLEND_MAX);
    zone_roles_submit(ZONE_FOOTWELL, ZONE_ROLE_AMBIENT_BASE,
                      entity.neutral_soft.r, entity.neutral_soft.g, entity.neutral_soft.b,
                      0.05f, ZONE_BLEND_MAX);

    /* Guidance shimmer tied to scene energy. */
    pulse = 0.5f + 0.5f * sinf(2.0f * (float)M_PI * 0.08f * t);
    guidance_alpha = (0.04f + 0.10f * entity.energy) * pulse * preset.overlay_gain_mul;
    zone_roles_submit(ZONE_STRIP, ZONE_ROLE_GUIDANCE_LINE,
                      entity.guidance_line.r, entity.guidance_line.g, entity.guidance_line.b,
                      guidance_alpha, ZONE_BLEND_MAX);

    /* Optional mild safety pre-tint while reverse mode is active. */
    if (can_ambient_is_reverse_active()) {
        zone_roles_submit(ZONE_HANDLE, ZONE_ROLE_SAFETY_ALERT,
                          entity.safety_alert.r, entity.safety_alert.g, entity.safety_alert.b,
                          0.12f, ZONE_BLEND_MAX);
    }
}

/* Zone live-FX path using current scene model and runtime state. */

void zones_apply_scene(const base_scene_t *pl)
{
    ambient_state_snapshot_t amb_state;
    scene_color_entity_t scene_entity;
    scene_preset_t preset;
    uint8_t effect_id = 0u;
    color_world_selection_t world_sel;

    if (!pl) return;

    g_dbg_zone_calib.oem_brightness = g_can_state.oem_brightness;
    g_dbg_zone_calib.calc_brightness = pl->calc_brightness;
    g_dbg_zone_calib.flags = 0u;
    if (g_night_mode_state) g_dbg_zone_calib.flags |= 0x01u;
    if (pl->crossfade_active) g_dbg_zone_calib.flags |= 0x02u;
    if (pl->stage == BASE_SCENE_BRIDGE) g_dbg_zone_calib.flags |= 0x04u;

    zone_roles_base_begin();
    ambient_state_store_get_snapshot(&amb_state);
    effect_id = amb_state.effect_id;
    if (effect_id > 1u) effect_id = 1u;
    scene_preset_resolve(can_ambient_get_motion_profile(), can_ambient_is_night_mode(), &preset);
    color_world_select(effect_id, amb_state.color_id, pl->t0_ms, &world_sel);
    runtime_debug_hooks_diag_set_world(world_sel.active,
                                       (uint8_t)world_sel.id,
                                       world_sel.generated,
                                       world_sel.dominant.r,
                                       world_sel.dominant.g,
                                       world_sel.dominant.b,
                                       world_sel.selection_distance);
    scene_color_entity_from_ambient(&amb_state,
                                    can_ambient_get_motion_profile(),
                                    can_ambient_is_night_mode(),
                                    &scene_entity);
    if (world_sel.active) {
        apply_strip_world_model(&world_sel, &preset, pl);
    } else if (scene_entity.valid) {
        apply_strip_scene_model(&scene_entity, &preset, pl);
    }
    zone_roles_publish_ambient_base_zone(ZONE_STRIP);

    {
        float comfort_dim = comfort_hvac_zone_dim(ZONE_STRIP);
        brightness_zone_eval_t br_eval;
        brightness_pipeline_eval_zone(pl->calc_brightness,
                                      ZONE_STRIP,
                                      1.0f,
                                      comfort_dim,
                                      g_night_mode_state,
                                      &br_eval);
        g_dbg_zone_calib.zone_base[ZONE_STRIP] = br_eval.base;
        g_dbg_zone_calib.zone_rel[ZONE_STRIP] = br_eval.rel;
        g_dbg_zone_calib.zone_boost[ZONE_STRIP] = br_eval.boost;
        g_dbg_zone_calib.zone_factor[ZONE_STRIP] = br_eval.rel * br_eval.boost;
        g_dbg_zone_calib.zone_comfort_dim[ZONE_STRIP] = br_eval.comfort_dim;
        g_dbg_zone_calib.zone_effective[ZONE_STRIP] = br_eval.value;
    }

    for (int z = 0; z < (int)ZONE_MAX; ++z) {
        if (z == ZONE_STRIP)
            continue; // main strip is rendered by base_scene

        zone_id_t zone_id = (zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->strip || zm->count == 0) continue;

        if (world_sel.active) {
            apply_zone_world(zone_id, pl, &preset, &world_sel);
        } else {
            apply_zone_fx_modern(zone_id, pl, &scene_entity, &preset, 0u);
        }

        zone_roles_publish_ambient_base_zone(zone_id);
    }
}

void zones_apply_intro(const base_scene_t *pl, float t_norm)
{
    if (!pl) return;
    ambient_state_snapshot_t amb_state;
    scene_color_entity_t scene_entity;
    scene_preset_t preset;
    float t_sec = (float)pl->t0_ms * 0.001f;

    float t = t_norm;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    ambient_state_store_get_snapshot(&amb_state);
    scene_preset_resolve(can_ambient_get_motion_profile(), can_ambient_is_night_mode(), &preset);
    scene_color_entity_from_ambient(&amb_state,
                                    can_ambient_get_motion_profile(),
                                    can_ambient_is_night_mode(),
                                    &scene_entity);
    if (!scene_entity.valid) return;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {

        if (z == ZONE_STRIP)
            continue;   // strip is rendered by oneshot welcome during intro

        zone_id_t zone_id = (zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->strip || zm->count == 0) continue;

        /* Base scene brightness for this zone. */
        brightness_zone_eval_t br_eval;
        float br;
        brightness_pipeline_eval_zone(pl->calc_brightness,
                                      zone_id,
                                      1.0f,
                                      comfort_hvac_zone_dim(zone_id),
                                      g_night_mode_state,
                                      &br_eval);
        br = br_eval.value;

        /* Zone-specific intro staggering:
         * Handle starts with short delay, storage later, footwell last.
         */

        float local = 0.0f;

        switch (zone_id) {
        case ZONE_HANDLE:
            if (t > 0.15f) {
                local = (t - 0.15f) / 0.60f;  // stretch into 0.15..0.75
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
            local = t;  // default behavior
            break;
        }

        if (local <= 0.0f) {
            /* Zone not yet enabled: keep it dark. */
            for (uint16_t i = 0; i < zm->count; ++i) {
                uint16_t led = zm->first + i;
                led_runtime_set_pixel_rgb(zm->strip, led, 0, 0, 0);
            }
            continue;
        }

        if (local > 1.0f) local = 1.0f;

        /* Quintic easing for smoother premium ramp-up. */
        float k = profile_blend_curve(smoothstep5(local));

        float br_zone = br * k;
        render_zone_model_static(zone_id, &scene_entity, &preset, br_zone, t_sec);
    }
}

void zones_apply_outro(const base_scene_t *pl, float t_norm)
{
    if (!pl) return;
    ambient_state_snapshot_t amb_state;
    scene_color_entity_t scene_entity;
    scene_preset_t preset;
    float t_sec = (float)pl->t0_ms * 0.001f;

    float t = t_norm;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* Quintic easing for premium fade-out */
    float k_base = 1.0f - profile_blend_curve(smoothstep5(t));

    ambient_state_store_get_snapshot(&amb_state);
    scene_preset_resolve(can_ambient_get_motion_profile(), can_ambient_is_night_mode(), &preset);
    scene_color_entity_from_ambient(&amb_state,
                                    can_ambient_get_motion_profile(),
                                    can_ambient_is_night_mode(),
                                    &scene_entity);
    if (!scene_entity.valid) return;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {

        if (z == ZONE_STRIP)
            continue;   // strip handles its own goodbye oneshot

        zone_id_t zone_id = (zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->strip || zm->count == 0) continue;

        float k_zone = k_base;

        /* Slightly different fade timings per zone. */
        switch (zone_id) {
        case ZONE_HANDLE:
        {
            /* Handle fades faster with smooth S-curve. */
            float tt = t * 1.15f;
            if (tt > 1.0f) tt = 1.0f;
            k_zone = 1.0f - profile_blend_curve(smoothstep5(tt));
        }
            break;
        case ZONE_STORAGE:
            /* Storage follows base timing closely. */
            k_zone = k_base;
            break;
        case ZONE_FOOTWELL:
        {
            /* Footwell fades slower to keep low-level trailing ambience. */
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

        render_zone_model_static(zone_id, &scene_entity, &preset, pl->calc_brightness * k_zone, t_sec);
    }
}

/* Linear bridge for zones: smooth transition from intro-solid to live scene. */
void zones_apply_bridge(const base_scene_t *pl, float t_norm)
{
    if (!pl) return;
    ambient_state_snapshot_t amb_state;
    scene_color_entity_t scene_entity;
    scene_preset_t preset;

    float blend = clamp01f(t_norm);
    float intro_k = 1.0f - blend;

    ambient_state_store_get_snapshot(&amb_state);
    scene_preset_resolve(can_ambient_get_motion_profile(), can_ambient_is_night_mode(), &preset);
    scene_color_entity_from_ambient(&amb_state,
                                    can_ambient_get_motion_profile(),
                                    can_ambient_is_night_mode(),
                                    &scene_entity);
    if (!scene_entity.valid) return;

    for (int z = 0; z < (int)ZONE_MAX; ++z) {
        if (z == ZONE_STRIP)
            continue;   // strip blend is handled in base_scene

        zone_id_t zone_id = (zone_id_t)z;
        const zone_map_t *zm = &g_zone_map[zone_id];
        if (!zm || !zm->strip || zm->count == 0) continue;

        /* First render modern scene content into zone buffer. */
        apply_zone_fx_modern(zone_id, pl, &scene_entity, &preset, amb_state.effect_id);

        /* Intro-like color: primary+neutral mix at reduced energy. */
        brightness_zone_eval_t br_eval;
        float br;
        brightness_pipeline_eval_zone(pl->calc_brightness,
                                      zone_id,
                                      1.0f,
                                      comfort_hvac_zone_dim(zone_id),
                                      g_night_mode_state,
                                      &br_eval);
        br = br_eval.value;
        {
            scene_rgb8_t intro_rgb;
            uint8_t intro_r, intro_g, intro_b;
            intro_rgb.r = (uint8_t)((float)scene_entity.base.r * 0.65f + (float)scene_entity.neutral_soft.r * 0.35f + 0.5f);
            intro_rgb.g = (uint8_t)((float)scene_entity.base.g * 0.65f + (float)scene_entity.neutral_soft.g * 0.35f + 0.5f);
            intro_rgb.b = (uint8_t)((float)scene_entity.base.b * 0.65f + (float)scene_entity.neutral_soft.b * 0.35f + 0.5f);
            intro_r = (uint8_t)((float)intro_rgb.r * br);
            intro_g = (uint8_t)((float)intro_rgb.g * br);
            intro_b = (uint8_t)((float)intro_rgb.b * br);

            for (uint16_t i = 0; i < zm->count; ++i) {
                uint8_t s_r, s_g, s_b;
                uint16_t led_idx = (uint16_t)(zm->first + i);
                led_runtime_get_pixel_rgb(zm->strip, led_idx, &s_r, &s_g, &s_b);

                uint8_t out_r = (uint8_t)(intro_r * intro_k + s_r * blend + 0.5f);
                uint8_t out_g = (uint8_t)(intro_g * intro_k + s_g * blend + 0.5f);
                uint8_t out_b = (uint8_t)(intro_b * intro_k + s_b * blend + 0.5f);

                led_runtime_set_pixel_rgb(zm->strip, led_idx, out_r, out_g, out_b);
            }
        }
    }
}

void zones_apply_interrupt_overlay(uint32_t now_ms)
{
    ambient_state_snapshot_t amb_state;
    scene_color_entity_t scene_entity;
    overlay_palette_t palette;
    scene_preset_t preset;

    ambient_state_store_get_snapshot(&amb_state);
    scene_color_entity_from_ambient(&amb_state,
                                    can_ambient_get_motion_profile(),
                                    can_ambient_is_night_mode(),
                                    &scene_entity);
    scene_preset_resolve(can_ambient_get_motion_profile(), can_ambient_is_night_mode(), &preset);
    overlay_palette_build(&scene_entity, &palette);

    zone_roles_frame_begin();
    submit_m4_entity_roles(now_ms);

#if AMB_ENABLE_HVAC_DUAL_SPLIT
    {
        if (!event_layer_is_hvac_temp_active(now_ms)) {
            float split = can_ambient_get_hvac_split_bias_for_board();
            float mag = fabsf(split);
            if (mag > 0.01f) {
                uint8_t r = (split > 0.0f) ? palette.warm.r : palette.cool.r;
                uint8_t g = (split > 0.0f) ? palette.warm.g : palette.cool.g;
                uint8_t b = (split > 0.0f) ? palette.warm.b : palette.cool.b;
                float gain = AMB_HVAC_SPLIT_GAIN * mag * preset.overlay_gain_mul;
                apply_override_to_zone(ZONE_STRIP, gain, r, g, b);
            }
        }
    }
#endif

#if AMB_ENABLE_SEAT_HEAT_COUPLING
    {
        float seat = can_ambient_get_seat_heat_level_for_board();
        if (seat > 0.02f) {
            uint8_t r = palette.warm.r;
            uint8_t g = palette.warm.g;
            uint8_t b = palette.warm.b;
            apply_override_to_zone(ZONE_FOOTWELL, AMB_SEAT_HEAT_OVERLAY_GAIN_FOOTWELL * seat * preset.overlay_gain_mul, r, g, b);
            apply_override_to_zone(ZONE_STORAGE, AMB_SEAT_HEAT_OVERLAY_GAIN_STORAGE * seat * preset.overlay_gain_mul, r, g, b);
            apply_override_to_zone(ZONE_STRIP, AMB_SEAT_HEAT_OVERLAY_GAIN_STRIP * seat * preset.overlay_gain_mul, r, g, b);
        }
    }
#endif

#if AMB_ENABLE_PARKING_WARN_OVERLAY
    {
        can_parking_warn_state_t park = can_ambient_get_parking_warn_state_for_board();
        static overlay_env_t s_pts_env = {0.0f};
        float side_level = 0.0f;
        static uint8_t s_prev_any = 0u;
        float t = ((float)now_ms * 0.001f) * 2.4f;
        float pulse;
        if (park.left_active && park.left_level > side_level) side_level = park.left_level;
        if (park.right_active && park.right_level > side_level) side_level = park.right_level;
        if (park.rear_active && park.rear_level > side_level) side_level = park.rear_level;
        side_level = overlay_env_step(&s_pts_env, side_level, AMB_PTS_ENV_ATTACK_ALPHA, AMB_PTS_ENV_RELEASE_ALPHA);
        if (side_level > 0.01f) {
            if (!s_prev_any) {
                event_layer_note_context_hold(palette.parking.r,
                                              palette.parking.g,
                                              palette.parking.b,
                                              AMB_CONTEXT_HOLD_GAIN_PARKING,
                                              AMB_CONTEXT_HOLD_DEFAULT_MS,
                                              now_ms);
            }
            s_prev_any = 1u;
            pulse = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * wrap01f(t));
            pulse = smoothstep5(pulse);
            apply_override_to_zone(ZONE_HANDLE, AMB_PARKING_OVERLAY_GAIN_HANDLE * side_level * pulse * preset.overlay_gain_mul,
                                   palette.parking.r, palette.parking.g, palette.parking.b);
            apply_override_to_zone(ZONE_STRIP, AMB_PARKING_OVERLAY_GAIN_STRIP * side_level * pulse * preset.overlay_gain_mul,
                                   palette.parking.r, palette.parking.g, palette.parking.b);
        } else {
            s_prev_any = 0u;
        }
    }
#endif

#if AMB_ENABLE_BSM_OVERLAY
    can_bsm_state_t bsm = can_ambient_get_bsm_state();
    float side_level = 0.0f;
    float t;
    float pulse;
    float luma;
    static overlay_env_t s_bsm_env = {0.0f};
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
    luma = overlay_env_step(&s_bsm_env, target_luma, attack_alpha, release_alpha);
    if (luma < 0.005f) {
        goto overlays_done;
    }
    if (luma > 1.0f) luma = 1.0f;

    wr = palette.safety.r;
    wg = palette.safety.g;
    wb = palette.safety.b;

    apply_safety_to_zone(ZONE_HANDLE, AMB_BSM_OVERLAY_GAIN_HANDLE * luma * preset.overlay_gain_mul, wr, wg, wb);
    if (dashboard_like) {
        apply_safety_to_zone(ZONE_STRIP, (AMB_BSM_OVERLAY_GAIN_STRIP * 0.72f) * luma * preset.overlay_gain_mul, wr, wg, wb);
    } else {
        apply_safety_to_zone(ZONE_STRIP, (AMB_BSM_OVERLAY_GAIN_STRIP * 0.86f) * luma * preset.overlay_gain_mul, wr, wg, wb);
    }
#else
    (void)now_ms;
#endif

overlays_done:
    zone_roles_frame_apply();
}
