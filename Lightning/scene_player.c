
#include "scene_player.h"
#include "frame_utils.h"
#include "features.h"
#include "zones.h"
#include <math.h>

uint8_t g_amb_night_mode = 0;   // 0 = день, 1 = ночь

/* Длительность плавного перехода intro -> scene (мс) */
#define BRIDGE_DURATION_MS      1000u

/* Auto-rotate configuration (from features.h) */
#if AMB_ENABLE_AUTO_ROTATE
#define AUTO_ROTATE_INTERVAL_MS (AMB_AUTO_ROTATE_INTERVAL_SEC * 1000u)
#define CROSSFADE_DURATION_MS   AMB_CROSSFADE_DURATION_MS
#define MAX_CROSSFADE_LEDS      160u     /* Max LEDs for crossfade temp buffer */

/* External OEM color from main.c - used for auto-rotate bank selection */
extern oem_color_id_t g_oem_color;
#endif

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

/* Quintic smoothstep (smoother than cubic, better for visual transitions) */
static float smoothstep5(float t)
{
    t = clamp01f(t);
    /* 6t^5 - 15t^4 + 10t^3 */
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/* Ultra-smooth ease for crossfade - very slow at start and end, faster in middle */
static float ease_crossfade(float t)
{
    t = clamp01f(t);
    /* Комбинация: медленный старт + плавная середина + медленный финиш */
    /* Используем синус для максимально органичного перехода */
    return (1.0f - cosf(t * 3.14159265f)) * 0.5f;
}

/* Clear LEDs before zone.first (unused LEDs that DMA still sends) */
static void clear_unused_leds(ws2812_t *ws)
{
    if (!ws) return;
    const zone_map_t *zm = &g_zone_map[WS_ZONE_STRIP];
    if (!zm || zm->first == 0) return;
    
    for (uint16_t i = 0; i < zm->first; ++i) {
        ws_set_pixel_rgb(ws, i, 0, 0, 0);
    }
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
    pl->st_scene.phase  = 0.0f;
    pl->st_scene.a      = 0.0f;
    pl->st_scene.b      = 0.0f;
    
    /* Auto-rotate / crossfade init */
#if AMB_ENABLE_AUTO_ROTATE
    pl->auto_rotate_enabled = 1;  /* Auto-enabled when flag is set */
#else
    pl->auto_rotate_enabled = 0;
#endif
    pl->oem_color           = 0;
    pl->scene_start_ms      = 0;
    pl->crossfade_active    = 0;
    pl->theme_next          = initial_theme;
    pl->st_scene_next.t     = 0.0f;
    pl->st_scene_next.speed = 0.0f;
    pl->st_scene_next.phase = 0.0f;
    pl->st_scene_next.a     = 0.0f;
    pl->st_scene_next.b     = 0.0f;
    pl->crossfade_start_ms  = 0;
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
    pl->scene_start_ms   = HAL_GetTick();  /* Reset auto-rotate timer */
    pl->crossfade_active = 0;              /* Cancel any ongoing crossfade */
    
    // Инициализируем состояние сцены для корректной работы анимаций
    pl->st_scene.t      = 0.0f;
    pl->st_scene.speed  = 0.0f;
    pl->st_scene.phase  = 0.0f;
    pl->st_scene.a      = 0.0f;
    pl->st_scene.b      = 0.0f;
}

void player_start_intro(ws2812_t *ws, scene_player_t *pl)
{
    if (!ws || !pl) return;
    const ws_theme_desc_t *T = ws_theme_get(pl->theme);
    if (!T) return;

    const ws_palette_t *pal = ws_palette_get(T->pal_main);
    if (!pal) return;

    /* Get zone parameters for STRIP */
    const zone_map_t *zm = &g_zone_map[WS_ZONE_STRIP];
    uint16_t first = zm->first;
    uint16_t count = zm->count;

    float base_br = clamp01f(pl->theme_brightness * pl->theme_dimming);
    ws_oneshot_start(ws, &pl->intro, FX_WELCOME, pal, base_br, 4000u, first, count);
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

    /* Get zone parameters for STRIP */
    const zone_map_t *zm = &g_zone_map[WS_ZONE_STRIP];
    uint16_t first = zm->first;
    uint16_t count = zm->count;

    float base_br = clamp01f(pl->theme_brightness * pl->theme_dimming);
    ws_oneshot_start(ws, &pl->outro, FX_GOODBYE, pal, base_br, 4000u, first, count);
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
        // Рендеринг выполняется в board_led_render_all() в main.c
        // чтобы синхронизировать главную ленту с другими зонами
        if (done) {
            /* Переходим в BRIDGE для плавного перехода к сцене */
            pl->stage = PST_BRIDGE;
            pl->t0_ms = HAL_GetTick();
            // Инициализируем состояние сцены при переходе
            pl->st_scene.t      = 0.0f;
            pl->st_scene.speed  = 0.0f;
            pl->st_scene.phase  = 0.0f;
            pl->st_scene.a      = 0.0f;
            pl->st_scene.b      = 0.0f;
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

        const zone_map_t *zm = &g_zone_map[WS_ZONE_STRIP];
        uint16_t first = zm->first;
        uint16_t count = zm->count;

#if AMB_ENABLE_AUTO_ROTATE
        /* --- Auto-rotate check --- */
        if (pl->auto_rotate_enabled && !pl->crossfade_active) {
            uint32_t elapsed = HAL_GetTick() - pl->scene_start_ms;
            if (elapsed >= AUTO_ROTATE_INTERVAL_MS) {
                /* Trigger crossfade to next theme (uses global g_oem_color) */
                const ws_theme_bank_t *bank = ws_theme_get_bank(g_oem_color);
                if (bank && bank->count > 1) {
                    pl->theme_next = ws_theme_bank_next(bank, pl->theme);
                    pl->crossfade_active = 1;
                    pl->crossfade_start_ms = HAL_GetTick();
                    /* Initialize next theme effect state - start from current t to avoid "dark start" */
                    pl->st_scene_next.t     = pl->st_scene.t;  /* Continue from current position */
                    pl->st_scene_next.speed = pl->st_scene.speed;
                    pl->st_scene_next.phase = pl->st_scene.phase;
                    pl->st_scene_next.a     = pl->st_scene.a;
                    pl->st_scene_next.b     = pl->st_scene.b;
                }
            }
        }

        /* --- Crossfade rendering --- */
        if (pl->crossfade_active) {
            const ws_theme_desc_t *T_next = ws_theme_get(pl->theme_next);
            const ws_palette_t *pal_next = T_next ? ws_palette_get(T_next->pal_main) : NULL;
            
            if (!T_next || !pal_next) {
                /* Invalid next theme, cancel crossfade */
                pl->crossfade_active = 0;
            } else {
                uint32_t cf_elapsed = HAL_GetTick() - pl->crossfade_start_ms;
                float cf_progress = (float)cf_elapsed / (float)CROSSFADE_DURATION_MS;
                
                /* Blend brightness between themes during crossfade */
                float blended_brightness = T->theme_brightness * (1.0f - cf_progress) 
                                         + T_next->theme_brightness * cf_progress;
                pl->calc_brightness = clamp01f(blended_brightness * pl->theme_dimming);
                ws_set_global_brightness(ws, pl->calc_brightness);
                
                if (cf_progress >= 1.0f) {
                    /* Crossfade complete - switch to next theme */
                    pl->theme = pl->theme_next;
                    pl->theme_brightness = T_next->theme_brightness;
                    pl->calc_brightness = clamp01f(pl->theme_brightness * pl->theme_dimming);
                    pl->st_scene = pl->st_scene_next;
                    pl->crossfade_active = 0;
                    pl->scene_start_ms = HAL_GetTick();
                    
                    /* Render final state of new theme */
                    ws_fx_apply(ws, T_next->fx_main, pal_next, &pl->st_scene, delta_ms, first, count);
                } else {
                    /* Blend both themes - simple linear for now to debug */
                    float blend = cf_progress;  // linear blend for testing
                    float inv_blend = 1.0f - blend;
                    
                    /* Render current theme to temp buffer (we'll read from grb) */
                    ws_fx_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);
                    
                    /* Save current theme colors to static buffer */
                    static uint8_t temp_grb[MAX_CROSSFADE_LEDS * 3];
                    uint16_t copy_count = (count > MAX_CROSSFADE_LEDS) ? MAX_CROSSFADE_LEDS : count;
                    uint8_t *curr_colors = (uint8_t*)ws->grb + first * 3;
                    for (uint16_t i = 0; i < copy_count * 3; ++i) {
                        temp_grb[i] = curr_colors[i];
                    }
                    
                    /* Render next theme */
                    ws_fx_apply(ws, T_next->fx_main, pal_next, &pl->st_scene_next, delta_ms, first, count);
                    
                    /* Screen blend to prevent darkening during transition
                     * screen(A,B) = 1 - (1-A)*(1-B) = A + B - A*B
                     * This preserves brightness even when blending opposite wave phases */
                    for (uint16_t i = 0; i < copy_count; ++i) {
                        uint32_t idx = i * 3u;
                        uint8_t curr_r = temp_grb[idx + 0];
                        uint8_t curr_g = temp_grb[idx + 1];
                        uint8_t curr_b = temp_grb[idx + 2];
                        
                        uint32_t buf_idx = (first + i) * 3u;
                        uint8_t next_r = ws->grb[buf_idx + 0];
                        uint8_t next_g = ws->grb[buf_idx + 1];
                        uint8_t next_b = ws->grb[buf_idx + 2];
                        
                        /* Normalize to 0-1 range */
                        float c_r = curr_r / 255.0f;
                        float c_g = curr_g / 255.0f;
                        float c_b = curr_b / 255.0f;
                        float n_r = next_r / 255.0f;
                        float n_g = next_g / 255.0f;
                        float n_b = next_b / 255.0f;
                        
                        /* Screen blend for both, then crossfade between results */
                        /* At blend=0: use curr, at blend=1: use next, in between: screen */
                        float screen_r = c_r + n_r - c_r * n_r;
                        float screen_g = c_g + n_g - c_g * n_g;
                        float screen_b = c_b + n_b - c_b * n_b;
                        
                        /* Interpolate: at edges use source, in middle use screen */
                        float mid_factor = 4.0f * blend * inv_blend;  /* Peaks at 1.0 when blend=0.5 */
                        float out_r = c_r * inv_blend + n_r * blend + (screen_r - (c_r * inv_blend + n_r * blend)) * mid_factor;
                        float out_g = c_g * inv_blend + n_g * blend + (screen_g - (c_g * inv_blend + n_g * blend)) * mid_factor;
                        float out_b = c_b * inv_blend + n_b * blend + (screen_b - (c_b * inv_blend + n_b * blend)) * mid_factor;
                        
                        /* Clamp and convert back to 8-bit */
                        if (out_r > 1.0f) out_r = 1.0f;
                        if (out_g > 1.0f) out_g = 1.0f;
                        if (out_b > 1.0f) out_b = 1.0f;
                        
                        ws_set_pixel_rgb(ws, first + i, 
                            (uint8_t)(out_r * 255.0f + 0.5f),
                            (uint8_t)(out_g * 255.0f + 0.5f),
                            (uint8_t)(out_b * 255.0f + 0.5f));
                    }
                }
            }
        } else {
            /* Normal scene rendering (no crossfade) */
            ws_fx_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);
        }
#else
        /* Auto-rotate disabled - just render scene */
        ws_fx_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);
#endif /* AMB_ENABLE_AUTO_ROTATE */
    }
    break;

    case PST_OUTRO:
    {
        uint8_t done = ws_oneshot_tick(ws, &pl->outro);
        // Рендеринг выполняется в board_led_render_all() в main.c
        // чтобы синхронизировать главную ленту с другими зонами
        if (done) {
            frame_clear(ws);
            // Финальный рендер черного экрана будет выполнен в board_led_render_all()
            pl->stage = PST_IDLE;
            ws_power_set(0);
        }
    }
    break;

    case PST_BRIDGE:
    {
        /* Плавный переход от финального состояния intro к сцене */
        const ws_theme_desc_t *T = ws_theme_get(pl->theme);
        if (!T) {
            pl->stage = PST_SCENE;
            break;
        }
        const ws_palette_t *pal = ws_palette_get(T->pal_main);
        if (!pal) {
            pl->stage = PST_SCENE;
            break;
        }

        uint32_t elapsed = HAL_GetTick() - pl->t0_ms;
        float progress = (float)elapsed / (float)BRIDGE_DURATION_MS;
        
        if (progress >= 1.0f) {
            /* Переход завершён - переходим к сцене */
            pl->stage = PST_SCENE;
            pl->t0_ms = HAL_GetTick();
            pl->scene_start_ms = HAL_GetTick();  /* Start auto-rotate timer */
            break;
        }

        /* Плавное нарастание эффекта сцены с помощью quintic smoothstep */
        float blend = smoothstep5(progress);
        float inv_blend = 1.0f - blend;

        const zone_map_t *zm = &g_zone_map[WS_ZONE_STRIP];
        uint16_t first = zm->first;
        uint16_t count = zm->count;

        /* Финальное состояние intro - solid color из центра палитры */
        uint8_t intro_r, intro_g, intro_b;
        ws_palette_sample_rgb8(pal, 0.5f, &intro_r, &intro_g, &intro_b);

        /* Применяем эффект сцены (записывает цвета в буфер) */
        ws_fx_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);

        /* Screen blend для плавного перехода без затемнения */
        /* При blend=0: intro, при blend=1: scene, посередине: screen blend */
        float i_r = intro_r / 255.0f;
        float i_g = intro_g / 255.0f;
        float i_b = intro_b / 255.0f;
        float mid_factor = 4.0f * blend * inv_blend;  /* Peak at blend=0.5 */
        
        for (uint16_t i = 0; i < count; ++i) {
            uint32_t idx = (uint32_t)(first + i) * 3u;
            uint8_t scene_r = ws->grb[idx + 0];
            uint8_t scene_g = ws->grb[idx + 1];
            uint8_t scene_b = ws->grb[idx + 2];
            
            float s_r = scene_r / 255.0f;
            float s_g = scene_g / 255.0f;
            float s_b = scene_b / 255.0f;
            
            /* Screen blend: prevents darkening */
            float scr_r = i_r + s_r - i_r * s_r;
            float scr_g = i_g + s_g - i_g * s_g;
            float scr_b = i_b + s_b - i_b * s_b;
            
            /* Linear blend with screen boost in middle */
            float lin_r = i_r * inv_blend + s_r * blend;
            float lin_g = i_g * inv_blend + s_g * blend;
            float lin_b = i_b * inv_blend + s_b * blend;
            
            float out_r = lin_r + (scr_r - lin_r) * mid_factor;
            float out_g = lin_g + (scr_g - lin_g) * mid_factor;
            float out_b = lin_b + (scr_b - lin_b) * mid_factor;
            
            if (out_r > 1.0f) out_r = 1.0f;
            if (out_g > 1.0f) out_g = 1.0f;
            if (out_b > 1.0f) out_b = 1.0f;
            
            ws_set_pixel_rgb(ws, first + i, 
                (uint8_t)(out_r * 255.0f + 0.5f),
                (uint8_t)(out_g * 255.0f + 0.5f),
                (uint8_t)(out_b * 255.0f + 0.5f));
        }
    }
    break;

    default:
        pl->stage = PST_SCENE;
        break;
    }

    /* Clear LEDs before zone.first (they are still sent via DMA) */
    clear_unused_leds(ws);
}

#if AMB_ENABLE_AUTO_ROTATE
/* -----------------------------------------------------------------------
 * Auto-rotate control functions
 * ----------------------------------------------------------------------- */

void player_set_auto_rotate(scene_player_t *pl, uint8_t enable)
{
    if (!pl) return;
    
    pl->auto_rotate_enabled = enable;
    
    /* Reset timer when enabling */
    if (enable) {
        pl->scene_start_ms = HAL_GetTick();
    }
}

void player_trigger_next_theme(scene_player_t *pl)
{
    if (!pl) return;
    if (pl->stage != PST_SCENE) return;  /* Only trigger in scene mode */
    if (pl->crossfade_active) return;    /* Already crossfading */
    
    /* Uses global g_oem_color for bank selection */
    const ws_theme_bank_t *bank = ws_theme_get_bank(g_oem_color);
    if (!bank || bank->count <= 1) return;
    
    pl->theme_next = ws_theme_bank_next(bank, pl->theme);
    pl->crossfade_active = 1;
    pl->crossfade_start_ms = HAL_GetTick();
    
    /* Initialize next theme effect state */
    pl->st_scene_next.t     = 0.0f;
    pl->st_scene_next.speed = 0.0f;
    pl->st_scene_next.phase = 0.0f;
    pl->st_scene_next.a     = 0.0f;
    pl->st_scene_next.b     = 0.0f;
}
#endif /* AMB_ENABLE_AUTO_ROTATE */
