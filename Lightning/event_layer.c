#include "event_layer.h"

#include "zones.h"
#include "led_runtime.h"
#include "ambient_config.h"
#include "ambient.h"
#include "ambient_state_store.h"
#include "scene_color_model.h"
#include "overlay_palette.h"
#include <math.h>
#include <string.h>

static event_scene_t g_event_scene;
static int8_t g_climate_memory_sign = 0;
static uint32_t g_climate_memory_until_ms = 0u;
static uint32_t g_hvac_temp_guard_until_ms = 0u;
typedef struct {
    uint8_t active;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    float gain;
    uint32_t start_ms;
} hvac_wave_state_t;
static hvac_wave_state_t g_hvac_wave;
typedef struct {
    uint8_t active;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    float gain;
    uint32_t hold_ms;
    uint32_t until_ms;
} context_hold_t;
static context_hold_t g_context_hold;

static void event_overlay_palette_build(overlay_palette_t *out)
{
    ambient_state_snapshot_t amb_state;
    scene_color_entity_t entity;

    if (!out) return;
    ambient_state_store_get_snapshot(&amb_state);
    scene_color_entity_from_ambient(&amb_state,
                                    can_ambient_get_motion_profile(),
                                    can_ambient_is_night_mode(),
                                    &entity);
    overlay_palette_build(&entity, out);
}

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static uint8_t clamp_u8f(float x)
{
    if (x <= 0.0f) return 0u;
    if (x >= 255.0f) return 255u;
    return (uint8_t)(x + 0.5f);
}

static float smoothstep5f(float x)
{
    x = clamp01f(x);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

static float event_scene_level(const event_scene_t *scene, uint32_t now_ms)
{
    uint32_t fade_in_end;
    uint32_t hold_end;
    uint32_t fade_out_end;

    if (!scene || !scene->active) return 0.0f;

    fade_in_end = scene->start_ms + (uint32_t)scene->fade_in_ms;
    hold_end = fade_in_end + scene->hold_ms;
    fade_out_end = hold_end + (uint32_t)scene->fade_out_ms;

    if (scene->fade_in_ms > 0u && now_ms < fade_in_end) {
        return clamp01f((float)(now_ms - scene->start_ms) / (float)scene->fade_in_ms);
    }
    if (now_ms < hold_end) {
        return 1.0f;
    }
    if (scene->fade_out_ms > 0u && now_ms < fade_out_end) {
        return clamp01f(1.0f - ((float)(now_ms - hold_end) / (float)scene->fade_out_ms));
    }
    return 0.0f;
}

static uint8_t event_scene_is_hvac_temp_active(uint32_t now_ms)
{
    if (now_ms < g_hvac_temp_guard_until_ms) return 1u;
    if (!g_event_scene.active) return 0u;
    if (!(g_event_scene.id == EVENT_SCENE_TEMP_WARM || g_event_scene.id == EVENT_SCENE_TEMP_COOL)) return 0u;
    return (event_scene_level(&g_event_scene, now_ms) > 0.0f) ? 1u : 0u;
}

static void apply_zone_override_solid(const event_zone_override_t *ovr,
                                      zone_id_t zone_id,
                                      float level)
{
    const zone_map_t *zm;
    uint8_t r, g, b;
    float gain;

    if (!ovr || !ovr->enabled || level <= 0.0f) return;
    zm = &g_zone_map[zone_id];
    if (!zm || !zm->strip || zm->count == 0u) return;

    r = ovr->r;
    g = ovr->g;
    b = ovr->b;

    gain = clamp01f(level * ((ovr->gain > 0.0f) ? ovr->gain : 1.0f));
    r = (uint8_t)((float)r * gain + 0.5f);
    g = (uint8_t)((float)g * gain + 0.5f);
    b = (uint8_t)((float)b * gain + 0.5f);

    for (uint16_t i = 0; i < zm->count; ++i) {
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
    }
}

static void clear_zone(zone_id_t zone_id)
{
    const zone_map_t *zm = &g_zone_map[zone_id];
    uint16_t i;
    if (!zm || !zm->strip || zm->count == 0u) return;
    for (i = 0u; i < zm->count; ++i) {
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), 0u, 0u, 0u);
    }
}

/* Paint strip with center-weighted falloff and gain envelope. */
static void apply_strip_center_weighted(uint8_t r,
                                        uint8_t g,
                                        uint8_t b,
                                        float gain,
                                        float side_falloff)
{
    const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
    uint16_t i;

    if (!zm || !zm->strip || zm->count == 0u) return;
    if (gain <= 0.0f) return;
    if (side_falloff < 0.0f) side_falloff = 0.0f;
    if (side_falloff > 1.0f) side_falloff = 1.0f;

    for (i = 0u; i < zm->count; ++i) {
        float center = (float)(zm->count - 1u) * 0.5f;
        float x = ((float)i - center) / (center + 1.0f);
        float w = 1.0f - side_falloff * fabsf(x);
        float gk = gain * w;
        led_runtime_set_pixel_rgb(zm->strip,
                                  (uint16_t)(zm->first + i),
                                  clamp_u8f((float)r * gk),
                                  clamp_u8f((float)g * gk),
                                  clamp_u8f((float)b * gk));
    }
}

static float hvac_wave_mask(const zone_map_t *zm, uint16_t idx, float head_pos)
{
    float trail_len;
    float lead_len;
    float rel;

    if (!zm || zm->count == 0u) return 0.0f;
    trail_len = (float)zm->count * 0.32f;
    if (trail_len < 4.0f) trail_len = 4.0f;
    lead_len = (float)zm->count * 0.03f;
    if (lead_len < 1.0f) lead_len = 1.0f;

    rel = head_pos - (float)idx;
    if (rel < -lead_len || rel > trail_len) return 0.0f;
    if (rel <= 0.0f) return 1.0f;
    rel /= trail_len;
    return expf(-rel * rel * 1.2f);
}

static void apply_hvac_wave_layer(uint32_t now_ms)
{
    const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
    uint32_t elapsed;
    uint32_t total_ms;
    uint16_t i;

    if (!g_hvac_wave.active || !zm || !zm->strip || zm->count == 0u) return;
    total_ms = (AMB_HVAC_WAVE_TRAVEL_MS > 0u) ? AMB_HVAC_WAVE_TRAVEL_MS : 1u;
    elapsed = (now_ms >= g_hvac_wave.start_ms) ? (now_ms - g_hvac_wave.start_ms) : 0u;
    if (elapsed >= total_ms) {
        g_hvac_wave.active = 0u;
        return;
    }

    {
        float t = smoothstep5f((float)elapsed / (float)total_ms);
        float head = t * (float)(zm->count - 1u);
        float edge = clamp01f(AMB_HVAC_WAVE_EDGE_ENVELOPE);
        float env = 1.0f;
        float rise = 1.0f;
        float fall = 1.0f;
        float overlay_level;
        float bg_dim_k;
        uint32_t fade_out_ms = AMB_HVAC_WAVE_FADE_OUT_MS;
        uint32_t fade_in_ms = AMB_HVAC_WAVE_FADE_IN_MS;

        if (fade_out_ms > 0u) {
            float x = (float)elapsed / (float)fade_out_ms;
            if (x > 1.0f) x = 1.0f;
            rise = smoothstep5f(x);
        }
        if (fade_in_ms > 0u && total_ms > fade_in_ms && elapsed > (total_ms - fade_in_ms)) {
            float x = (float)(total_ms - elapsed) / (float)fade_in_ms;
            if (x < 0.0f) x = 0.0f;
            if (x > 1.0f) x = 1.0f;
            fall = smoothstep5f(x);
        }
        overlay_level = (rise < fall) ? rise : fall;
        bg_dim_k = 1.0f - (1.0f - AMB_HVAC_WAVE_BG_DIM) * overlay_level;

        if (edge > 0.01f) {
            if (t < edge) {
                env = smoothstep5f(t / edge);
            } else if (t > (1.0f - edge)) {
                env = smoothstep5f((1.0f - t) / edge);
            }
        }
        for (i = 0u; i < zm->count; ++i) {
            uint16_t led_idx = (uint16_t)(zm->first + i);
            float w = hvac_wave_mask(zm, i, head) * env * overlay_level;
            uint8_t ar, ag, ab;
            float base_r, base_g, base_b;
            float wave_r, wave_g, wave_b;
            float a;
            led_runtime_get_pixel_rgb(zm->strip, led_idx, &ar, &ag, &ab);
            base_r = (float)ar * bg_dim_k;
            base_g = (float)ag * bg_dim_k;
            base_b = (float)ab * bg_dim_k;
            wave_r = (float)g_hvac_wave.r * g_hvac_wave.gain;
            wave_g = (float)g_hvac_wave.g * g_hvac_wave.gain;
            wave_b = (float)g_hvac_wave.b * g_hvac_wave.gain;
            if (wave_r > 255.0f) wave_r = 255.0f;
            if (wave_g > 255.0f) wave_g = 255.0f;
            if (wave_b > 255.0f) wave_b = 255.0f;
            a = clamp01f(w);
            led_runtime_set_pixel_rgb(zm->strip,
                                      led_idx,
                                      clamp_u8f(base_r * (1.0f - a) + wave_r * a),
                                      clamp_u8f(base_g * (1.0f - a) + wave_g * a),
                                      clamp_u8f(base_b * (1.0f - a) + wave_b * a));
        }
    }
}

static void apply_zone_override_trail(const event_zone_override_t *ovr,
                                      zone_id_t zone_id,
                                      float level,
                                      float phase,
                                      uint8_t reverse,
                                      uint8_t cycles)
{
    const zone_map_t *zm;
    uint8_t r, g, b;
    float spread;
    float head;
    uint16_t i;

    if (!ovr || !ovr->enabled || level <= 0.0f) return;
    if (zone_id != ZONE_STRIP) {
        apply_zone_override_solid(ovr, zone_id, level);
        return;
    }
    zm = &g_zone_map[zone_id];
    if (!zm || !zm->strip || zm->count == 0u) return;

    r = ovr->r;
    g = ovr->g;
    b = ovr->b;

    if (phase < 0.0f) phase = 0.0f;
    if (phase > 1.0f) phase = 1.0f;
    if (cycles < 1u) cycles = 1u;
    phase *= (float)cycles;
    phase = phase - floorf(phase);
    spread = (float)zm->count * AMB_HVAC_TRAIL_SPREAD_NORM;
    if (spread < 1.0f) spread = 1.0f;
    head = reverse ? ((1.0f - phase) * (float)(zm->count - 1u)) : (phase * (float)(zm->count - 1u));

    for (i = 0u; i < zm->count; ++i) {
        float d = fabsf((float)i - head) / spread;
        float w = expf(-d * d * 2.6f);
        float gain = level * ((ovr->gain > 0.0f) ? ovr->gain : 1.0f) * AMB_HVAC_TRAIL_GAIN * w;
        uint8_t rr = clamp_u8f((float)r * gain);
        uint8_t gg = clamp_u8f((float)g * gain);
        uint8_t bb = clamp_u8f((float)b * gain);
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), rr, gg, bb);
    }
}

static void apply_zone_override_sweep(const event_zone_override_t *ovr,
                                      zone_id_t zone_id,
                                      float level,
                                      float phase,
                                      uint8_t reverse,
                                      uint8_t cycles)
{
    const zone_map_t *zm;
    uint8_t r, g, b;
    float trail_len;
    float lead_len;
    float head;
    uint16_t i;

    if (!ovr || !ovr->enabled || level <= 0.0f) return;
    if (zone_id != ZONE_STRIP) {
        apply_zone_override_solid(ovr, zone_id, level);
        return;
    }
    zm = &g_zone_map[zone_id];
    if (!zm || !zm->strip || zm->count == 0u) return;

    r = ovr->r;
    g = ovr->g;
    b = ovr->b;

    if (phase < 0.0f) phase = 0.0f;
    if (phase > 1.0f) phase = 1.0f;
    if (cycles < 1u) cycles = 1u;
    phase *= (float)cycles;
    phase = phase - floorf(phase);
    head = reverse ? ((1.0f - phase) * (float)(zm->count - 1u)) : (phase * (float)(zm->count - 1u));
    trail_len = (float)zm->count * 0.35f;
    if (trail_len < 4.0f) trail_len = 4.0f;
    lead_len = (float)zm->count * 0.03f;
    if (lead_len < 1.0f) lead_len = 1.0f;

    for (i = 0u; i < zm->count; ++i) {
        float rel = reverse ? ((float)i - head) : (head - (float)i);
        float w = 0.0f;
        float gain;
        if (rel >= -lead_len && rel <= trail_len) {
            if (rel <= 0.0f) {
                /* Bright leading edge (small forward core). */
                w = 1.0f;
            } else {
                float d = rel / trail_len;
                w = expf(-d * d * 1.2f);
            }
        }
        gain = ((ovr->gain > 0.0f) ? ovr->gain : 1.0f) * AMB_HVAC_TRAIL_GAIN * w;
        led_runtime_set_pixel_rgb(zm->strip,
                                  (uint16_t)(zm->first + i),
                                  clamp_u8f((float)r * gain),
                                  clamp_u8f((float)g * gain),
                                  clamp_u8f((float)b * gain));
    }
}

static void apply_unlock_signature_strip(float level, float phase)
{
    const zone_map_t *zm;
    overlay_palette_t palette;
    float spread;
    float head_cool;
    float head_warm;
    float global_gain;
    uint16_t i;

    if (level <= 0.0f) return;
    zm = &g_zone_map[ZONE_STRIP];
    if (!zm || !zm->strip || zm->count == 0u) return;
    event_overlay_palette_build(&palette);

    if (phase < 0.0f) phase = 0.0f;
    if (phase > 1.0f) phase = 1.0f;
    spread = (float)zm->count * AMB_UNLOCK_SIGNATURE_SPREAD_NORM * 0.62f;
    if (spread < 1.0f) spread = 1.0f;
    head_cool = phase * (float)(zm->count - 1u);
    head_warm = (1.0f - phase) * (float)(zm->count - 1u);
    global_gain = level * AMB_UNLOCK_SIGNATURE_GAIN;
    if (g_night_mode_state) {
        global_gain *= AMB_UNLOCK_SIGNATURE_NIGHT_GAIN;
    }

    for (i = 0u; i < zm->count; ++i) {
        float d_cool = ((float)i - head_cool) / spread;
        float d_warm = ((float)i - head_warm) / spread;
        float w_cool = expf(-d_cool * d_cool * 2.3f);
        float w_warm = expf(-d_warm * d_warm * 2.3f);
        float out_r = ((float)palette.cool.r * w_cool +
                       (float)palette.warm.r * w_warm)
                      * global_gain;
        float out_g = ((float)palette.cool.g * w_cool +
                       (float)palette.warm.g * w_warm)
                      * global_gain;
        float out_b = ((float)palette.cool.b * w_cool +
                       (float)palette.warm.b * w_warm)
                      * global_gain;

        if (out_r > 255.0f) out_r = 255.0f;
        if (out_g > 255.0f) out_g = 255.0f;
        if (out_b > 255.0f) out_b = 255.0f;
        led_runtime_set_pixel_rgb(zm->strip,
                                  (uint16_t)(zm->first + i),
                                  (uint8_t)(out_r + 0.5f),
                                  (uint8_t)(out_g + 0.5f),
                                  (uint8_t)(out_b + 0.5f));
    }
}

void event_layer_init(void)
{
    memset(&g_event_scene, 0, sizeof(g_event_scene));
    memset(&g_hvac_wave, 0, sizeof(g_hvac_wave));
    g_climate_memory_sign = 0;
    g_climate_memory_until_ms = 0u;
    g_hvac_temp_guard_until_ms = 0u;
    memset(&g_context_hold, 0, sizeof(g_context_hold));
}

void event_layer_cancel_all(void)
{
    g_event_scene.active = 0u;
    g_hvac_wave.active = 0u;
    g_hvac_temp_guard_until_ms = 0u;
}

uint8_t event_layer_is_hvac_temp_active(uint32_t now_ms)
{
    if (g_hvac_wave.active) return 1u;
    return event_scene_is_hvac_temp_active(now_ms);
}

void event_layer_start_hvac_wave(uint8_t r, uint8_t g, uint8_t b, float gain, uint32_t now_ms)
{
    g_hvac_wave.active = 1u;
    g_hvac_wave.r = r;
    g_hvac_wave.g = g;
    g_hvac_wave.b = b;
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 3.0f) gain = 3.0f;
    g_hvac_wave.gain = gain;
    g_hvac_wave.start_ms = now_ms;
    g_hvac_temp_guard_until_ms = now_ms + AMB_HVAC_WAVE_TRAVEL_MS + AMB_HVAC_WAVE_GUARD_MS;
}

void event_layer_note_context_hold(uint8_t r, uint8_t g, uint8_t b, float gain, uint32_t hold_ms, uint32_t now_ms)
{
    if (hold_ms == 0u) hold_ms = AMB_CONTEXT_HOLD_DEFAULT_MS;
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    g_context_hold.active = 1u;
    g_context_hold.r = r;
    g_context_hold.g = g;
    g_context_hold.b = b;
    g_context_hold.gain = gain;
    g_context_hold.hold_ms = hold_ms;
    g_context_hold.until_ms = now_ms + hold_ms;
}

void event_layer_note_climate_memory(uint8_t warmer, uint32_t now_ms)
{
#if AMB_ENABLE_CLIMATE_MEMORY
    overlay_palette_t palette;
    event_overlay_palette_build(&palette);
    g_climate_memory_sign = warmer ? 1 : -1;
    g_climate_memory_until_ms = now_ms + AMB_CLIMATE_MEMORY_HOLD_MS;
    event_layer_note_context_hold(warmer ? palette.warm.r : palette.cool.r,
                                  warmer ? palette.warm.g : palette.cool.g,
                                  warmer ? palette.warm.b : palette.cool.b,
                                  AMB_CONTEXT_HOLD_GAIN_CLIMATE,
                                  AMB_CONTEXT_HOLD_DEFAULT_MS,
                                  now_ms);
#else
    (void)warmer;
    (void)now_ms;
#endif
}

uint8_t event_layer_start(const event_scene_t *scene, uint32_t now_ms)
{
    if (!scene) return 0u;
    if (g_event_scene.active && g_event_scene.priority > scene->priority) {
        return 0u;
    }

    g_event_scene = *scene;
    g_event_scene.active = 1u;
    g_event_scene.start_ms = now_ms;
    if (scene->id == EVENT_SCENE_TEMP_WARM || scene->id == EVENT_SCENE_TEMP_COOL) {
        uint32_t total = (uint32_t)scene->fade_in_ms + scene->hold_ms + (uint32_t)scene->fade_out_ms;
        g_hvac_temp_guard_until_ms = now_ms + total + 260u;
    }
    return 1u;
}

void event_layer_apply(uint32_t now_ms)
{
    float level;
    float phase = 1.0f;
    float hvac_sweep_phase = 1.0f;
    uint32_t total_ms;
    uint32_t elapsed_ms;
    uint8_t z;
#if AMB_ENABLE_CLIMATE_MEMORY
    if (!event_scene_is_hvac_temp_active(now_ms) && g_climate_memory_sign != 0 && g_climate_memory_until_ms > now_ms) {
        overlay_palette_t palette;
        uint32_t rem = g_climate_memory_until_ms - now_ms;
        float k = (float)rem / (float)AMB_CLIMATE_MEMORY_HOLD_MS;
        uint8_t r;
        uint8_t g;
        uint8_t b;
        event_overlay_palette_build(&palette);
        r = (g_climate_memory_sign > 0) ? palette.warm.r : palette.cool.r;
        g = (g_climate_memory_sign > 0) ? palette.warm.g : palette.cool.g;
        b = (g_climate_memory_sign > 0) ? palette.warm.b : palette.cool.b;

        apply_strip_center_weighted(r, g, b, AMB_CLIMATE_MEMORY_GAIN * k, 0.45f);
    } else {
        g_climate_memory_sign = 0;
    }
#endif

    if (g_context_hold.active) {
        if (g_context_hold.until_ms > now_ms) {
            uint32_t rem = g_context_hold.until_ms - now_ms;
            float k = (float)rem / (float)((g_context_hold.hold_ms > 0u) ? g_context_hold.hold_ms : 1u);
            if (k > 1.0f) k = 1.0f;
            apply_strip_center_weighted(g_context_hold.r,
                                        g_context_hold.g,
                                        g_context_hold.b,
                                        g_context_hold.gain * k,
                                        0.35f);
        } else {
            g_context_hold.active = 0u;
        }
    }

    if (g_hvac_wave.active) {
        apply_hvac_wave_layer(now_ms);
        return;
    }

    if (!g_event_scene.active) return;

    level = event_scene_level(&g_event_scene, now_ms);
    if (level <= 0.0f) {
        g_event_scene.active = 0u;
        return;
    }

    total_ms = (uint32_t)g_event_scene.fade_in_ms + g_event_scene.hold_ms + (uint32_t)g_event_scene.fade_out_ms;
    elapsed_ms = (now_ms >= g_event_scene.start_ms) ? (now_ms - g_event_scene.start_ms) : 0u;
    if (total_ms > 0u) {
        if (elapsed_ms > total_ms) elapsed_ms = total_ms;
        phase = (float)elapsed_ms / (float)total_ms;
    }
    if (g_event_scene.id == EVENT_SCENE_TEMP_WARM || g_event_scene.id == EVENT_SCENE_TEMP_COOL) {
        /* Move head from start->end during fade-in+hold; keep at end during fade-out. */
        uint32_t move_ms = (uint32_t)g_event_scene.fade_in_ms + g_event_scene.hold_ms;
        if (move_ms == 0u) move_ms = 1u;
        if (elapsed_ms >= move_ms) {
            hvac_sweep_phase = 1.0f;
        } else {
            hvac_sweep_phase = (float)elapsed_ms / (float)move_ms;
        }
    }

    if (g_event_scene.id == EVENT_SCENE_WELCOME_ACCENT) {
        apply_unlock_signature_strip(level, phase);
        return;
    }

    if (g_event_scene.id == EVENT_SCENE_DOOR_OPEN) {
        uint32_t total = (uint32_t)g_event_scene.fade_in_ms + g_event_scene.hold_ms + (uint32_t)g_event_scene.fade_out_ms;
        uint32_t elapsed = (now_ms >= g_event_scene.start_ms) ? (now_ms - g_event_scene.start_ms) : 0u;
        uint8_t rear_side = g_event_scene.strip_trail_reverse ? 1u : 0u;
        if (elapsed < AMB_DOOR_PRE_GLOW_MS) {
            event_zone_override_t h = g_event_scene.zone[ZONE_HANDLE];
            event_zone_override_t f = g_event_scene.zone[ZONE_FOOTWELL];
            h.gain = AMB_DOOR_PRE_GLOW_GAIN_HANDLE;
            f.gain = AMB_DOOR_PRE_GLOW_GAIN_FOOTWELL;
            apply_zone_override_solid(&h, ZONE_HANDLE, level);
            apply_zone_override_solid(&f, ZONE_FOOTWELL, level);
            return;
        }
        if (total > 0u) {
            float p2 = (float)(elapsed - AMB_DOOR_PRE_GLOW_MS) / (float)((total > AMB_DOOR_PRE_GLOW_MS) ? (total - AMB_DOOR_PRE_GLOW_MS) : 1u);
            event_zone_override_t s = g_event_scene.zone[ZONE_STRIP];
            event_zone_override_t h = g_event_scene.zone[ZONE_HANDLE];
            event_zone_override_t f = g_event_scene.zone[ZONE_FOOTWELL];
            if (p2 < 0.0f) p2 = 0.0f;
            if (p2 > 1.0f) p2 = 1.0f;
            s.gain = AMB_DOOR_CHOREO_STRIP_GAIN;
            h.gain = 0.48f;
            f.gain = 0.32f;
            apply_zone_override_trail(&s, ZONE_STRIP, level, p2, rear_side, g_event_scene.strip_trail_cycles);
            apply_zone_override_solid(&h, ZONE_HANDLE, level);
            apply_zone_override_solid(&f, ZONE_FOOTWELL, level);
            return;
        }
    }

    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        if (g_event_scene.strip_trail && z == (uint8_t)ZONE_STRIP) {
            if (g_event_scene.id == EVENT_SCENE_TEMP_WARM || g_event_scene.id == EVENT_SCENE_TEMP_COOL) {
                /* HVAC temp event is rendered as exclusive strip animation (no ambient background). */
                clear_zone((zone_id_t)z);
            }
            if (g_event_scene.id == EVENT_SCENE_TEMP_WARM || g_event_scene.id == EVENT_SCENE_TEMP_COOL) {
                apply_zone_override_sweep(&g_event_scene.zone[z],
                                          (zone_id_t)z,
                                          level,
                                          hvac_sweep_phase,
                                          g_event_scene.strip_trail_reverse,
                                          g_event_scene.strip_trail_cycles);
            } else {
                apply_zone_override_trail(&g_event_scene.zone[z],
                                          (zone_id_t)z,
                                          level,
                                          phase,
                                          g_event_scene.strip_trail_reverse,
                                          g_event_scene.strip_trail_cycles);
            }
        } else {
            apply_zone_override_solid(&g_event_scene.zone[z], (zone_id_t)z, level);
        }
    }
}

void event_scene_build_dual_accent(event_scene_t *scene,
                                       uint8_t cool_r, uint8_t cool_g, uint8_t cool_b,
                                       uint8_t warm_r, uint8_t warm_g, uint8_t warm_b,
                                       uint32_t hold_ms)
{
    if (!scene) return;
    memset(scene, 0, sizeof(*scene));
    scene->id = EVENT_SCENE_DUAL_ACCENT;
    scene->priority = EVENT_PRIORITY_DECOR;
    scene->fade_in_ms = 180u;
    scene->fade_out_ms = 650u;
    scene->hold_ms = hold_ms;

    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u, cool_r, cool_g, cool_b, 0.82f };
    scene->zone[ZONE_HANDLE] = (event_zone_override_t){ 1u, cool_r, cool_g, cool_b, 0.72f };
    scene->zone[ZONE_STORAGE] = (event_zone_override_t){ 1u, warm_r, warm_g, warm_b, 0.52f };
    scene->zone[ZONE_FOOTWELL] = (event_zone_override_t){ 1u, warm_r, warm_g, warm_b, 0.66f };
}

void event_scene_build_directional_cue(event_scene_t *scene,
                                           uint8_t left_side,
                                           uint8_t r, uint8_t g, uint8_t b,
                                           uint32_t hold_ms)
{
    if (!scene) return;
    memset(scene, 0, sizeof(*scene));
    scene->id = left_side ? EVENT_SCENE_DIRECTION_LEFT : EVENT_SCENE_DIRECTION_RIGHT;
    scene->priority = EVENT_PRIORITY_INFO;
    scene->fade_in_ms = 90u;
    scene->fade_out_ms = 240u;
    scene->hold_ms = hold_ms;

    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u, r, g, b, 0.72f };
    scene->zone[ZONE_HANDLE] = (event_zone_override_t){ 1u, r, g, b, 1.00f };
}

void event_scene_build_door_open(event_scene_t *scene, uint8_t rear_side)
{
    if (!scene) return;
    memset(scene, 0, sizeof(*scene));
    scene->id = EVENT_SCENE_DOOR_OPEN;
    scene->priority = EVENT_PRIORITY_INFO;
    scene->fade_in_ms = 110u;
    scene->fade_out_ms = 380u;
    scene->hold_ms = AMB_DOOR_OPEN_EVENT_HOLD_MS;
    scene->strip_trail = 1u;
    scene->strip_trail_reverse = rear_side ? 1u : 0u;
    scene->strip_trail_cycles = 1u;

    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u,
        AMB_DOOR_OPEN_ACCENT_R, AMB_DOOR_OPEN_ACCENT_G, AMB_DOOR_OPEN_ACCENT_B, 0.60f };
    scene->zone[ZONE_HANDLE] = (event_zone_override_t){ 1u,
        AMB_DOOR_OPEN_ACCENT_R, AMB_DOOR_OPEN_ACCENT_G, AMB_DOOR_OPEN_ACCENT_B, 0.88f };
    scene->zone[ZONE_FOOTWELL] = (event_zone_override_t){ 1u,
        AMB_DOOR_OPEN_ACCENT_R, AMB_DOOR_OPEN_ACCENT_G, AMB_DOOR_OPEN_ACCENT_B, 0.48f };
}

void event_scene_build_temp_delta(event_scene_t *scene,
                                  uint8_t warmer,
                                  uint8_t r, uint8_t g, uint8_t b,
                                  uint32_t hold_ms)
{
    float strip_gain;

    if (!scene) return;
    memset(scene, 0, sizeof(*scene));
    scene->id = warmer ? EVENT_SCENE_TEMP_WARM : EVENT_SCENE_TEMP_COOL;
    scene->priority = EVENT_PRIORITY_INFO;
    scene->fade_in_ms = 80u;
    scene->fade_out_ms = 420u;
    scene->hold_ms = hold_ms;
    strip_gain = warmer ? AMB_HVAC_TEMP_EVENT_STRIP_GAIN_WARM : AMB_HVAC_TEMP_EVENT_STRIP_GAIN_COOL;
#if AMB_ENABLE_HVAC_DRAG_TRAIL
    scene->strip_trail = 1u;
    /* HVAC trail policy: deterministic sweep from strip start to strip end. */
    scene->strip_trail_reverse = 0u;
    scene->strip_trail_cycles = 1u;
#endif

    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u, r, g, b, strip_gain };
}

void event_scene_build_unlock_signature(event_scene_t *scene)
{
    if (!scene) return;
    memset(scene, 0, sizeof(*scene));
    scene->id = EVENT_SCENE_WELCOME_ACCENT;
    scene->priority = EVENT_PRIORITY_DECOR;
    scene->fade_in_ms = (uint16_t)AMB_UNLOCK_SIGNATURE_FADE_IN_MS;
    scene->hold_ms = AMB_UNLOCK_SIGNATURE_HOLD_MS;
    scene->fade_out_ms = (uint16_t)AMB_UNLOCK_SIGNATURE_FADE_OUT_MS;
    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u, 0u, 0u, 0u, 1.0f };
}

void event_scene_build_dual_accent_auto(event_scene_t *scene, uint32_t hold_ms)
{
    overlay_palette_t palette;
    event_overlay_palette_build(&palette);
    event_scene_build_dual_accent(scene,
                                  palette.cool.r, palette.cool.g, palette.cool.b,
                                  palette.warm.r, palette.warm.g, palette.warm.b,
                                  hold_ms);
}

void event_layer_start_hvac_wave_auto(uint8_t warmer, uint32_t now_ms)
{
    overlay_palette_t palette;
    event_overlay_palette_build(&palette);
    if (warmer) {
        event_layer_start_hvac_wave(palette.warm.r,
                                    palette.warm.g,
                                    palette.warm.b,
                                    AMB_HVAC_WAVE_OVERLAY_GAIN_WARM,
                                    now_ms);
    } else {
        event_layer_start_hvac_wave(palette.cool.r,
                                    palette.cool.g,
                                    palette.cool.b,
                                    AMB_HVAC_WAVE_OVERLAY_GAIN_COOL,
                                    now_ms);
    }
}
