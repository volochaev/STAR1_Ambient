#include "event_layer.h"

#include "zones.h"
#include "palette.h"
#include "led_runtime.h"
#include "features.h"
#include "ambient.h"
#include <math.h>
#include <string.h>

static event_scene_t g_event_scene;
static int8_t g_climate_memory_sign = 0;
static uint32_t g_climate_memory_until_ms = 0u;

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
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

    if (ovr->use_palette) {
        const palette_t *pal = palette_get((palette_id_t)ovr->pal_id);
        if (!pal) return;
        palette_sample_rgb8(pal, clamp01f(ovr->pal_u), &r, &g, &b);
    } else {
        r = ovr->r;
        g = ovr->g;
        b = ovr->b;
    }

    gain = clamp01f(level * ((ovr->gain > 0.0f) ? ovr->gain : 1.0f));
    r = (uint8_t)((float)r * gain + 0.5f);
    g = (uint8_t)((float)g * gain + 0.5f);
    b = (uint8_t)((float)b * gain + 0.5f);

    for (uint16_t i = 0; i < zm->count; ++i) {
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), r, g, b);
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

    if (ovr->use_palette) {
        const palette_t *pal = palette_get((palette_id_t)ovr->pal_id);
        if (!pal) return;
        palette_sample_rgb8(pal, clamp01f(ovr->pal_u), &r, &g, &b);
    } else {
        r = ovr->r;
        g = ovr->g;
        b = ovr->b;
    }

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
        uint8_t rr = (uint8_t)((float)r * gain + 0.5f);
        uint8_t gg = (uint8_t)((float)g * gain + 0.5f);
        uint8_t bb = (uint8_t)((float)b * gain + 0.5f);
        led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), rr, gg, bb);
    }
}

void event_layer_init(void)
{
    memset(&g_event_scene, 0, sizeof(g_event_scene));
    g_climate_memory_sign = 0;
    g_climate_memory_until_ms = 0u;
}

void event_layer_cancel_all(void)
{
    g_event_scene.active = 0u;
}

void event_layer_note_climate_memory(uint8_t warmer, uint32_t now_ms)
{
#if AMB_ENABLE_CLIMATE_MEMORY
    g_climate_memory_sign = warmer ? 1 : -1;
    g_climate_memory_until_ms = now_ms + AMB_CLIMATE_MEMORY_HOLD_MS;
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
    return 1u;
}

void event_layer_apply(uint32_t now_ms)
{
    float level;
    float phase = 1.0f;
    uint32_t total_ms;
    uint32_t elapsed_ms;
    uint8_t z;
#if AMB_ENABLE_CLIMATE_MEMORY
    if (g_climate_memory_sign != 0 && g_climate_memory_until_ms > now_ms) {
        const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
        uint32_t rem = g_climate_memory_until_ms - now_ms;
        float k = (float)rem / (float)AMB_CLIMATE_MEMORY_HOLD_MS;
        uint8_t r = (g_climate_memory_sign > 0) ? AMB_HVAC_SPLIT_WARM_R : AMB_HVAC_SPLIT_COOL_R;
        uint8_t g = (g_climate_memory_sign > 0) ? AMB_HVAC_SPLIT_WARM_G : AMB_HVAC_SPLIT_COOL_G;
        uint8_t b = (g_climate_memory_sign > 0) ? AMB_HVAC_SPLIT_WARM_B : AMB_HVAC_SPLIT_COOL_B;

        if (zm && zm->strip && zm->count > 0u) {
            uint16_t i;
            float center = 0.5f * (float)(zm->count - 1u);
            for (i = 0u; i < zm->count; ++i) {
                float x = ((float)i - center) / (center + 1.0f);
                float w = 1.0f - 0.45f * fabsf(x);
                float gain = AMB_CLIMATE_MEMORY_GAIN * k * w;
                uint8_t rr = (uint8_t)((float)r * gain + 0.5f);
                uint8_t gg = (uint8_t)((float)g * gain + 0.5f);
                uint8_t bb = (uint8_t)((float)b * gain + 0.5f);
                led_runtime_set_pixel_rgb(zm->strip, (uint16_t)(zm->first + i), rr, gg, bb);
            }
        }
    } else {
        g_climate_memory_sign = 0;
    }
#endif

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

    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        if (g_event_scene.strip_trail && z == (uint8_t)ZONE_STRIP) {
            apply_zone_override_trail(&g_event_scene.zone[z],
                                      (zone_id_t)z,
                                      level,
                                      phase,
                                      g_event_scene.strip_trail_reverse,
                                      g_event_scene.strip_trail_cycles);
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

    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u, 0u, cool_r, cool_g, cool_b, 0u, 0.0f, 0.82f };
    scene->zone[ZONE_HANDLE] = (event_zone_override_t){ 1u, 0u, cool_r, cool_g, cool_b, 0u, 0.0f, 0.72f };
    scene->zone[ZONE_STORAGE] = (event_zone_override_t){ 1u, 0u, warm_r, warm_g, warm_b, 0u, 0.0f, 0.52f };
    scene->zone[ZONE_FOOTWELL] = (event_zone_override_t){ 1u, 0u, warm_r, warm_g, warm_b, 0u, 0.0f, 0.66f };
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

    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u, 0u, r, g, b, 0u, 0.0f, 0.72f };
    scene->zone[ZONE_HANDLE] = (event_zone_override_t){ 1u, 0u, r, g, b, 0u, 0.0f, 1.00f };
}

void event_scene_build_temp_delta(event_scene_t *scene,
                                  uint8_t warmer,
                                  uint8_t r, uint8_t g, uint8_t b,
                                  uint32_t hold_ms)
{
    uint8_t rear_board;
    uint8_t base_reverse;

    if (!scene) return;
    memset(scene, 0, sizeof(*scene));
    scene->id = warmer ? EVENT_SCENE_TEMP_WARM : EVENT_SCENE_TEMP_COOL;
    scene->priority = EVENT_PRIORITY_INFO;
    scene->fade_in_ms = 80u;
    scene->fade_out_ms = 420u;
    scene->hold_ms = hold_ms;
#if AMB_ENABLE_HVAC_DRAG_TRAIL
    scene->strip_trail = 1u;
    rear_board = (uint8_t)((BOARD_TYPE == BOARD_TYPE_RL) || (BOARD_TYPE == BOARD_TYPE_RR) || (BOARD_TYPE == BOARD_TYPE_REAR));
    base_reverse = warmer ? 0u : 1u;
    scene->strip_trail_reverse = (uint8_t)(base_reverse ^ rear_board);
    scene->strip_trail_cycles = 1u;
#endif

    scene->zone[ZONE_STRIP] = (event_zone_override_t){ 1u, 0u, r, g, b, 0u, 0.0f, 0.55f };
}
