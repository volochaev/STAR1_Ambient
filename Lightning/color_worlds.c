#include "color_worlds.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    color_world_desc_t desc;
    scene_rgb8_t stops[4];
    uint8_t stop_count;
    float zone_phase[ZONE_MAX];
    float luma_norm;
} color_world_entry_t;

static const color_world_entry_t k_worlds[COLOR_WORLD_MAX] = {
    [COLOR_WORLD_OCEAN_BLUE] = {
        .desc = { COLOR_WORLD_OCEAN_BLUE, COLOR_WORLD_DUAL, {36u, 96u, 250u}, 0u, 8200u, 1.40f, 1 },
        .stops = { {18u, 64u, 248u}, {0u, 188u, 244u} },
        .stop_count = 2u,
        .zone_phase = { 0.0f, 0.40f, 0.90f, 1.20f },
        .luma_norm = 0.96f,
    },
    [COLOR_WORLD_RED_MOON] = {
        .desc = { COLOR_WORLD_RED_MOON, COLOR_WORLD_SPECTRAL, {236u, 16u, 54u}, 0u, 8800u, 1.30f, 1 },
        .stops = { {78u, 0u, 108u}, {178u, 0u, 146u}, {236u, 16u, 54u} },
        .stop_count = 3u,
        .zone_phase = { 0.0f, 0.34f, 0.76f, 1.08f },
        .luma_norm = 0.94f,
    },
    [COLOR_WORLD_MIAMI_ROSE] = {
        .desc = { COLOR_WORLD_MIAMI_ROSE, COLOR_WORLD_DUAL, {192u, 24u, 170u}, 0u, 8000u, 1.22f, 1 },
        .stops = { {242u, 22u, 148u}, {32u, 70u, 226u} },
        .stop_count = 2u,
        .zone_phase = { 0.0f, (float)M_PI, (float)M_PI, (float)M_PI },
        .luma_norm = 0.90f,
    },
    [COLOR_WORLD_MALIBU_SUNSET] = {
        .desc = { COLOR_WORLD_MALIBU_SUNSET, COLOR_WORLD_SPECTRAL, {222u, 94u, 42u}, 0u, 8600u, 1.28f, -1 },
        .stops = { {182u, 28u, 86u}, {230u, 88u, 34u}, {252u, 158u, 62u} },
        .stop_count = 3u,
        .zone_phase = { 0.0f, 0.30f, 0.66f, 0.98f },
        .luma_norm = 0.95f,
    },
    [COLOR_WORLD_FIERY_WHITE] = {
        .desc = { COLOR_WORLD_FIERY_WHITE, COLOR_WORLD_DUAL, {255u, 180u, 148u}, 0u, 7600u, 1.20f, 1 },
        .stops = { {255u, 255u, 255u}, {255u, 34u, 0u} },
        .stop_count = 2u,
        .zone_phase = { 0.0f, 0.28f, 0.56f, 0.84f },
        .luma_norm = 0.88f,
    },
    [COLOR_WORLD_PURPLE_SKY] = {
        .desc = { COLOR_WORLD_PURPLE_SKY, COLOR_WORLD_SPECTRAL, {106u, 86u, 202u}, 0u, 10200u, 1.08f, -1 },
        .stops = { {92u, 36u, 176u}, {70u, 128u, 232u}, {142u, 190u, 248u} },
        .stop_count = 3u,
        .zone_phase = { 0.0f, 0.30f, 0.70f, 1.10f },
        .luma_norm = 0.93f,
    },
    [COLOR_WORLD_JUNGLE_GREEN] = {
        .desc = { COLOR_WORLD_JUNGLE_GREEN, COLOR_WORLD_SPECTRAL, {40u, 224u, 110u}, 0u, 9000u, 1.24f, 1 },
        .stops = { {30u, 206u, 66u}, {28u, 224u, 110u}, {90u, 236u, 168u} },
        .stop_count = 3u,
        .zone_phase = { 0.0f, 0.28f, 0.62f, 0.96f },
        .luma_norm = 0.94f,
    },
    [COLOR_WORLD_GLACIER_BLUE] = {
        .desc = { COLOR_WORLD_GLACIER_BLUE, COLOR_WORLD_DUAL, {102u, 170u, 232u}, 0u, 11000u, 1.05f, 1 },
        .stops = { {208u, 240u, 255u}, {0u, 0u, 128u} },
        .stop_count = 2u,
        .zone_phase = { 0.0f, 0.34f, 0.68f, 1.02f },
        .luma_norm = 0.91f,
    },
    [COLOR_WORLD_SUN_YELLOW] = {
        .desc = { COLOR_WORLD_SUN_YELLOW, COLOR_WORLD_DUAL, {248u, 178u, 28u}, 0u, 8000u, 1.24f, -1 },
        .stops = { {238u, 238u, 0u}, {255u, 119u, 0u} },
        .stop_count = 2u,
        .zone_phase = { 0.0f, 0.26f, 0.54f, 0.82f },
        .luma_norm = 0.92f,
    },
    [COLOR_WORLD_SILVER_LIGHT] = {
        .desc = { COLOR_WORLD_SILVER_LIGHT, COLOR_WORLD_LUMA, {220u, 220u, 220u}, 0u, 12200u, 0.92f, 1 },
        .stops = { {255u, 255u, 255u}, {76u, 76u, 76u} },
        .stop_count = 2u,
        .zone_phase = { 0.0f, 0.35f, 0.75f, 1.15f },
        .luma_norm = 0.90f,
    },
    [COLOR_WORLD_CITRUS_DAWN] = {
        .desc = { COLOR_WORLD_CITRUS_DAWN, COLOR_WORLD_SPECTRAL, {206u, 228u, 72u}, 1u, 8600u, 1.20f, 1 },
        .stops = { {170u, 204u, 62u}, {222u, 224u, 88u}, {246u, 174u, 62u} },
        .stop_count = 3u,
        .zone_phase = { 0.0f, 0.24f, 0.58f, 0.92f },
        .luma_norm = 0.94f,
    },
    [COLOR_WORLD_MINT_BREEZE] = {
        .desc = { COLOR_WORLD_MINT_BREEZE, COLOR_WORLD_DUAL, {110u, 232u, 188u}, 1u, 9800u, 1.10f, -1 },
        .stops = { {82u, 228u, 168u}, {118u, 206u, 242u} },
        .stop_count = 2u,
        .zone_phase = { 0.0f, 0.22f, 0.48f, 0.78f },
        .luma_norm = 0.95f,
    },
    [COLOR_WORLD_MIDNIGHT_TIDE] = {
        .desc = { COLOR_WORLD_MIDNIGHT_TIDE, COLOR_WORLD_SPECTRAL, {34u, 66u, 174u}, 1u, 10400u, 1.16f, 1 },
        .stops = { {10u, 34u, 128u}, {28u, 66u, 170u}, {56u, 108u, 214u} },
        .stop_count = 3u,
        .zone_phase = { 0.0f, 0.20f, 0.44f, 0.72f },
        .luma_norm = 0.93f,
    },
};

/* Canonical fixed mapping: HU OEM color_id (0..11) -> world_id.
 * This table is the only runtime selection policy for effect=1. */
static const color_world_id_t k_color_to_world_lut[12] = {
    COLOR_WORLD_RED_MOON,       /* Red: strongest red-forward world. */
    COLOR_WORLD_MALIBU_SUNSET,  /* Orange: warm sunset gradient. */
    COLOR_WORLD_SUN_YELLOW,     /* Yellow: amber-yellow dual wave. */
    COLOR_WORLD_CITRUS_DAWN,    /* Light Yellow: generated yellow-green bridge. */
    COLOR_WORLD_JUNGLE_GREEN,   /* Green: pure green spectral world. */
    COLOR_WORLD_MINT_BREEZE,    /* Light Green: generated mint/cyan soft world. */
    COLOR_WORLD_GLACIER_BLUE,   /* Sky Blue: ice-blue style world. */
    COLOR_WORLD_OCEAN_BLUE,     /* Blue: canonical blue/cyan world. */
    COLOR_WORLD_MIDNIGHT_TIDE,  /* Deep Blue: dedicated generated deep-blue world. */
    COLOR_WORLD_PURPLE_SKY,     /* Purple: violet/sky spectral world. */
    COLOR_WORLD_MIAMI_ROSE,     /* Pink: neon pink/blue contrast world. */
    COLOR_WORLD_SILVER_LIGHT,   /* White: luma-only white wave. */
};

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float wrap01f(float x)
{
    x = x - floorf(x);
    if (x < 0.0f) x += 1.0f;
    return x;
}

static scene_rgb8_t rgb_lerp(scene_rgb8_t a, scene_rgb8_t b, float t)
{
    scene_rgb8_t out;
    t = clamp01f(t);
    out.r = (uint8_t)((float)a.r * (1.0f - t) + (float)b.r * t + 0.5f);
    out.g = (uint8_t)((float)a.g * (1.0f - t) + (float)b.g * t + 0.5f);
    out.b = (uint8_t)((float)a.b * (1.0f - t) + (float)b.b * t + 0.5f);
    return out;
}

static scene_rgb8_t sample_spectral(const color_world_entry_t *w, float phase)
{
    float u;
    float x;
    uint8_t idx;
    if (!w || w->stop_count < 2u) return (scene_rgb8_t){0u, 0u, 0u};

    u = wrap01f(phase) * (float)(w->stop_count - 1u);
    idx = (uint8_t)u;
    if (idx >= (uint8_t)(w->stop_count - 1u)) idx = (uint8_t)(w->stop_count - 2u);
    x = u - (float)idx;
    return rgb_lerp(w->stops[idx], w->stops[idx + 1u], x);
}

static scene_rgb8_t sample_dual(const color_world_entry_t *w, float phase)
{
    float u = wrap01f(phase);
    float tri = (u < 0.5f) ? (u * 2.0f) : ((1.0f - u) * 2.0f);
    return rgb_lerp(w->stops[0u], w->stops[1u], tri);
}

static scene_rgb8_t sample_luma(const color_world_entry_t *w, float phase)
{
    float u = wrap01f(phase);
    float s = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * u);
    return rgb_lerp(w->stops[1u], w->stops[0u], s);
}

static scene_rgb8_t apply_world_norm(scene_rgb8_t c, float norm)
{
    scene_rgb8_t out;
    if (norm < 0.2f) norm = 0.2f;
    if (norm > 1.4f) norm = 1.4f;
    out.r = (uint8_t)fminf(255.0f, (float)c.r * norm + 0.5f);
    out.g = (uint8_t)fminf(255.0f, (float)c.g * norm + 0.5f);
    out.b = (uint8_t)fminf(255.0f, (float)c.b * norm + 0.5f);
    return out;
}

static scene_rgb8_t apply_sat_scale(scene_rgb8_t c, float sat_scale)
{
    float y;
    scene_rgb8_t gray;
    if (sat_scale < 0.0f) sat_scale = 0.0f;
    if (sat_scale > 1.3f) sat_scale = 1.3f;
    y = 0.299f * (float)c.r + 0.587f * (float)c.g + 0.114f * (float)c.b;
    gray.r = (uint8_t)(y + 0.5f);
    gray.g = (uint8_t)(y + 0.5f);
    gray.b = (uint8_t)(y + 0.5f);
    return rgb_lerp(gray, c, sat_scale);
}

const color_world_desc_t *color_world_get_desc(color_world_id_t id)
{
    if (id >= COLOR_WORLD_MAX) return NULL;
    return &k_worlds[id].desc;
}

void color_world_select(uint8_t effect_id,
                        uint8_t color_id,
                        uint32_t now_ms,
                        color_world_selection_t *out)
{
    static uint8_t s_prev_valid = 0u;
    static color_world_id_t s_prev_id = COLOR_WORLD_OCEAN_BLUE;
    static color_world_id_t s_curr_id = COLOR_WORLD_OCEAN_BLUE;
    static uint32_t s_switch_ms = 0u;
    const uint32_t k_blend_ms = 420u;
    color_world_id_t id;
    float k = 1.0f;

    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (effect_id == 0u) {
        s_prev_valid = 0u;
        return;
    }

    id = k_color_to_world_lut[color_id % 12u];
    if (!s_prev_valid) {
        s_prev_valid = 1u;
        s_prev_id = id;
        s_curr_id = id;
        s_switch_ms = now_ms;
    } else if (id != s_curr_id) {
        s_prev_id = s_curr_id;
        s_curr_id = id;
        s_switch_ms = now_ms;
    }
    if (k_blend_ms > 0u) {
        uint32_t elapsed = now_ms - s_switch_ms;
        if (elapsed < k_blend_ms) {
            k = (float)elapsed / (float)k_blend_ms;
            k = clamp01f(k);
            k = k * k * (3.0f - 2.0f * k);
        }
    }

    out->active = 1u;
    out->id = s_curr_id;
    out->prev_id = s_prev_id;
    out->dominant = k_worlds[s_curr_id].desc.dominant;
    out->transition_k = k;
    out->selection_distance = 0.0f;
    out->generated = k_worlds[s_curr_id].desc.generated;
}

void color_world_sample(const color_world_selection_t *sel,
                        zone_id_t zone,
                        float pos01,
                        uint32_t now_ms,
                        const scene_preset_t *preset,
                        uint8_t *r,
                        uint8_t *g,
                        uint8_t *b)
{
    const color_world_entry_t *w;
    const color_world_entry_t *w_prev;
    float period_s;
    float temporal = 1.0f;
    float phase;
    float phase_prev;
    float t;
    float sat_scale = 1.0f;
    float norm = 1.0f;
    scene_rgb8_t c;
    scene_rgb8_t c_prev;
    scene_rgb8_t out_rgb;

    if (!sel || !preset || !r || !g || !b || sel->id >= COLOR_WORLD_MAX || !sel->active) return;
    if (zone >= ZONE_MAX) zone = ZONE_STRIP;

    w = &k_worlds[sel->id];
    w_prev = &k_worlds[(sel->prev_id < COLOR_WORLD_MAX) ? sel->prev_id : sel->id];
    period_s = (float)w->desc.period_ms * 0.001f;
    if (period_s < 0.5f) period_s = 0.5f;
    if (preset->temporal_scale > 0.05f) temporal = preset->temporal_scale;

    t = ((float)now_ms * 0.001f) / period_s;
    phase = (float)w->desc.direction * t * temporal;
    phase += pos01 * w->desc.spatial_freq;
    phase += w->zone_phase[zone];
    phase_prev = (float)w_prev->desc.direction * t * temporal;
    phase_prev += pos01 * w_prev->desc.spatial_freq;
    phase_prev += w_prev->zone_phase[zone];

    if (w->desc.type == COLOR_WORLD_DUAL) {
        c = sample_dual(w, phase);
    } else if (w->desc.type == COLOR_WORLD_SPECTRAL) {
        c = sample_spectral(w, phase);
    } else {
        c = sample_luma(w, phase);
    }
    if (w_prev->desc.type == COLOR_WORLD_DUAL) {
        c_prev = sample_dual(w_prev, phase_prev);
    } else if (w_prev->desc.type == COLOR_WORLD_SPECTRAL) {
        c_prev = sample_spectral(w_prev, phase_prev);
    } else {
        c_prev = sample_luma(w_prev, phase_prev);
    }

    norm = w->luma_norm;
    c = apply_world_norm(c, norm);
    c_prev = apply_world_norm(c_prev, w_prev->luma_norm);

    if (preset->id == SCENE_PRESET_LOUNGE) {
        if (sel->id == COLOR_WORLD_MIAMI_ROSE || sel->id == COLOR_WORLD_SUN_YELLOW) {
            sat_scale = 0.88f;
        } else {
            sat_scale = 0.92f;
        }
    }
    c = apply_sat_scale(c, sat_scale);
    c_prev = apply_sat_scale(c_prev, sat_scale);

    out_rgb = rgb_lerp(c_prev, c, sel->transition_k);

    *r = out_rgb.r;
    *g = out_rgb.g;
    *b = out_rgb.b;
}
