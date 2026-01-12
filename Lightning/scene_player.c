
#include "scene_player.h"
#include "frame_utils.h"
#include "features.h"

uint8_t g_amb_night_mode = 0;   // 0 = день, 1 = ночь

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

void player_init(scene_player_t *pl, ws_theme_id_t initial_theme)
{
    if (!pl) return;
    pl->stage           = PST_IDLE;
    pl->t0_ms           = 0;
    pl->theme           = initial_theme;
    pl->theme_brightness= 0.7f;
    pl->theme_dimming   = 1.0f;
    pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
    pl->intro.active    = 0;
    pl->outro.active    = 0;
    pl->st_scene.t      = 0.0f;
    pl->st_scene.speed  = 0.0f;
}

void player_start_theme(ws2812_t *ws, scene_player_t *pl, ws_theme_id_t theme)
{
    (void)ws;
    if (!pl) return;

    pl->theme = theme;
    const ws_theme_desc_t *T = ws_theme_get(theme);
    if (!T) return;

    pl->theme_brightness = T->theme_brightness;
    pl->calc_brightness  = clamp01f(pl->theme_brightness * pl->theme_dimming);
    pl->stage            = PST_SCENE;
    pl->t0_ms            = HAL_GetTick();
}

void player_start_intro(ws2812_t *ws, scene_player_t *pl)
{
    if (!ws || !pl) return;
    const ws_theme_desc_t *T = ws_theme_get(pl->theme);
    if (!T) return;

    const ws_palette_t *pal = ws_palette_get(T->pal_main);
    if (!pal) return;

    float base_br = clamp01f(pl->theme_brightness * pl->theme_dimming);
    ws_oneshot_start(ws, &pl->intro, FX_WELCOME, pal, base_br, 4000u);
    pl->stage = PST_INTRO;
    pl->t0_ms = HAL_GetTick();
}

void player_start_outro(ws2812_t *ws, scene_player_t *pl)
{
    if (!ws || !pl) return;
    const ws_theme_desc_t *T = ws_theme_get(pl->theme);
    if (!T) return;

    const ws_palette_t *pal = ws_palette_get(T->pal_main);
    if (!pal) return;

    float base_br = clamp01f(pl->theme_brightness * pl->theme_dimming);
    ws_oneshot_start(ws, &pl->outro, FX_GOODBYE, pal, base_br, 4000u);
    pl->stage = PST_OUTRO;
    pl->t0_ms = HAL_GetTick();
}

void player_tick(ws2812_t *ws, scene_player_t *pl, uint32_t delta_ms)
{
    if (!ws || !pl) return;

    /* Power control: any non-idle stage -> power ON */
    if (pl->stage != PST_IDLE) {
        ws_power_set(1);
    } else {
        ws_power_set(0);
        return;
    }

    ws_set_global_brightness(ws, pl->calc_brightness);

    switch (pl->stage) {
    case PST_IDLE:
        return;

    case PST_INTRO:
    {
        uint8_t done = ws_oneshot_tick(ws, &pl->intro);
        ws_render(ws);
        if (done) {
            pl->stage = PST_SCENE;
            pl->t0_ms = HAL_GetTick();
        }
    }
    break;

    case PST_SCENE:
    {
        const ws_theme_desc_t *T = ws_theme_get(pl->theme);
        if (!T) {
            pl->stage = PST_IDLE;
            break;
        }
        const ws_palette_t *pal = ws_palette_get(T->pal_main);
        if (!pal) {
            pl->stage = PST_IDLE;
            break;
        }

        ws_fx_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms);
        ws_render(ws);
    }
    break;

    case PST_OUTRO:
    {
        uint8_t done = ws_oneshot_tick(ws, &pl->outro);
        ws_render(ws);
        if (done) {
            frame_clear(ws);
            ws_render(ws);
            pl->stage = PST_IDLE;
            ws_power_set(0);
        }
    }
    break;

    case PST_BRIDGE:
    default:
        pl->stage = PST_SCENE;
        break;
    }
}
