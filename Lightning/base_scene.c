
#include "base_scene.h"
#include "frame_utils.h"
#include "features.h"
#include "zones.h"
#include "led_runtime.h"
#include <math.h>

uint8_t g_night_mode_state = 0;   // 0 = день, 1 = ночь
extern motion_profile_t can_ambient_get_motion_profile(void);
extern float can_ambient_get_bank_character_speed_scale(void);

/* Длительность плавного перехода intro -> scene (мс) */
#define BRIDGE_DURATION_MS      AMB_BRIDGE_DURATION_MS

/* Theme crossfade configuration (from features.h) */
#if (AMB_ENABLE_AUTO_ROTATE || AMB_ENABLE_FAST_BANK_SWITCH)
#define AMB_ENABLE_THEME_CROSSFADE 1u
#define CROSSFADE_DURATION_MS   AMB_CROSSFADE_DURATION_MS
#else
#define AMB_ENABLE_THEME_CROSSFADE 0u
#endif

#if AMB_ENABLE_AUTO_ROTATE
#define AUTO_ROTATE_INTERVAL_MS (AMB_AUTO_ROTATE_INTERVAL_SEC * 1000u)

/* External runtime OEM color state used for auto-rotate bank selection */
extern oem_color_id_t g_oem_color;
#endif

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

/* Snapshot буфера финального кадра intro для мягкого бриджа */
static uint8_t g_bridge_intro_rgb[AMB_MAX_CROSSFADE_LEDS * 3];
static uint16_t g_bridge_intro_count = 0;
static uint16_t g_bridge_intro_first = 0;
static uint8_t g_bridge_intro_valid = 0;

static float theme_base_brightness(const theme_definition_t *T)
{
    if (!T) return AMB_THEME_MIN_BRIGHTNESS;
    float tb = T->theme_brightness;
    if (tb < AMB_THEME_MIN_BRIGHTNESS) tb = AMB_THEME_MIN_BRIGHTNESS;
    if (tb > 1.0f) tb = 1.0f;
    return tb;
}

static float theme_fx_speed(theme_id_t theme, fx_id_t fx)
{
    float speed = fx_base_speed(fx);
    float motion_scale = theme_motion_scale(theme);
    float profile_scale = AMB_MOTION_PROFILE_SCALE_LUXURY;
    float personality_scale = 1.0f;
    float bank_memory_scale = 1.0f;
    motion_profile_t profile = can_ambient_get_motion_profile();
    if (profile == MOTION_PROFILE_CALM) {
        profile_scale = AMB_MOTION_PROFILE_SCALE_CALM;
    } else if (profile == MOTION_PROFILE_SPORT) {
        profile_scale = AMB_MOTION_PROFILE_SCALE_SPORT;
    }
    speed *= motion_scale;
    speed *= profile_scale;
#if AMB_ENABLE_THEME_PERSONALITY
    personality_scale = theme_personality_speed(theme);
#endif
#if AMB_ENABLE_BANK_CHARACTER_MEMORY
    bank_memory_scale = can_ambient_get_bank_character_speed_scale();
#endif
    speed *= personality_scale;
    speed *= bank_memory_scale;
#if AMB_ENABLE_NIGHT_CALM
    if (g_night_mode_state) {
        speed *= AMB_NIGHT_MOTION_SPEED_SCALE;
    }
#endif
    if (speed < 0.0f) speed = 0.0f;
    return speed;
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

static void base_scene_apply_scene_profile(base_scene_t *pl, theme_id_t theme)
{
    const theme_definition_t *T;
    if (!pl) return;
    T = theme_get(theme);
    if (!T) return;
    pl->st_scene.speed = theme_fx_speed(theme, T->fx_main);
}

/* Quintic smoothstep (smoother than cubic, better for visual transitions) */
static float smoothstep5(float t)
{
    t = clamp01f(t);
    /* 6t^5 - 15t^4 + 10t^3 */
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float smooth_transition_blend(float current, float target)
{
    float a = AMB_TRANSITION_SMOOTH_ALPHA;
    a *= motion_profile_smooth_alpha_mul();
    if (a < 0.0f) a = 0.0f;
    if (a > 0.98f) a = 0.98f;
    current = current * a + target * (1.0f - a);
    return clamp01f(current);
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

/* Clear LEDs before zone.first (unused LEDs that DMA still sends) */
static void clear_unused_leds(led_runtime_strip_t *ws)
{
    if (!ws) return;
    const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
    if (!zm || zm->first == 0) return;
    
    for (uint16_t i = 0; i < zm->first; ++i) {
        led_runtime_set_pixel_rgb(ws, i, 0, 0, 0);
    }
}

void base_scene_refresh_brightness(base_scene_t *pl)
{
    if (!pl) return;
    pl->calc_brightness = clamp01f(pl->theme_brightness * pl->theme_dimming);
}

void base_scene_reset_fx_state(base_scene_t *pl)
{
    if (!pl) return;
    pl->st_scene.t = 0.0f;
    pl->st_scene.speed = 0.0f;
    pl->st_scene.phase = 0.0f;
    pl->st_scene.a = 0.0f;
    pl->st_scene.b = 0.0f;
    pl->st_scene_next.t = 0.0f;
    pl->st_scene_next.speed = 0.0f;
    pl->st_scene_next.phase = 0.0f;
    pl->st_scene_next.a = 0.0f;
    pl->st_scene_next.b = 0.0f;
}

void base_scene_start_theme_crossfade(base_scene_t *pl, theme_id_t target_theme)
{
#if AMB_ENABLE_THEME_CROSSFADE
    const theme_definition_t *T_next;

    if (!pl) return;
    if (pl->stage != BASE_SCENE_ACTIVE) return;
    if (pl->crossfade_active) return;
    if (pl->theme == target_theme) return;

    T_next = theme_get(target_theme);
    if (!T_next) return;

    pl->theme_next = target_theme;
    pl->crossfade_active = 1u;
    pl->crossfade_start_ms = HAL_GetTick();
    pl->crossfade_blend_smooth = 0.0f;
    pl->bridge_blend_smooth = 0.0f;

    pl->st_scene_next.t = pl->st_scene.t;
    pl->st_scene_next.speed = theme_fx_speed(target_theme, T_next->fx_main);
    pl->st_scene_next.phase = pl->st_scene.phase + (theme_personality_phase(target_theme) - theme_personality_phase(pl->theme));
    pl->st_scene_next.a = pl->st_scene.a;
    pl->st_scene_next.b = pl->st_scene.b;
#else
    (void)pl;
    (void)target_theme;
#endif
}

uint8_t base_scene_apply_theme_base(base_scene_t *pl, theme_id_t theme)
{
    const theme_definition_t *T;
    if (!pl) return 0u;
    T = theme_get(theme);
    if (!T) return 0u;

    pl->theme = theme;
    pl->theme_brightness = theme_base_brightness(T);
    base_scene_refresh_brightness(pl);
    return 1u;
}

uint8_t base_scene_apply_theme_with_intro(base_scene_t *pl, led_runtime_strip_t *ws, theme_id_t theme)
{
    if (!ws) return 0u;
    if (!base_scene_apply_theme_base(pl, theme)) return 0u;
    base_scene_reset_fx_state(pl);
    base_scene_start_intro(ws, pl);
    return 1u;
}

uint8_t base_scene_apply_theme_to_scene(base_scene_t *pl, theme_id_t theme)
{
    if (!base_scene_apply_theme_base(pl, theme)) return 0u;
    base_scene_reset_fx_state(pl);
    pl->stage = BASE_SCENE_ACTIVE;
    pl->t0_ms = HAL_GetTick();
    return 1u;
}

void base_scene_init(base_scene_t *pl, theme_id_t initial_theme)
{
    if (!pl) return;
    pl->stage           = BASE_SCENE_IDLE;
    pl->t0_ms           = 0;
    pl->theme           = initial_theme;
    pl->theme_brightness= 0.7f;
    pl->theme_dimming   = 1.0f;
    pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
    pl->intro.active    = 0;
    pl->outro.active    = 0;
    pl->st_scene.t      = 0.0f;
    pl->st_scene.speed  = 0.0f;
    pl->st_scene.phase  = theme_personality_phase(initial_theme);
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
    pl->crossfade_blend_smooth = 0.0f;
    pl->bridge_blend_smooth = 0.0f;
    pl->global_brightness_smooth = clamp01f(pl->calc_brightness);
    base_scene_apply_scene_profile(pl, initial_theme);
}

void base_scene_start_theme(led_runtime_strip_t *ws, base_scene_t *pl, theme_id_t theme)
{
    (void)ws;
    if (!pl) return;

    pl->theme = theme;
    const theme_definition_t *T = theme_get(theme);
    if (!T) return;

    pl->theme_brightness = theme_base_brightness(T);
    pl->calc_brightness  = clamp01f(pl->theme_brightness * pl->theme_dimming);
    pl->stage            = BASE_SCENE_ACTIVE;
    pl->t0_ms            = HAL_GetTick();
    pl->scene_start_ms   = HAL_GetTick();  /* Reset auto-rotate timer */
    pl->crossfade_active = 0;              /* Cancel any ongoing crossfade */
    pl->crossfade_blend_smooth = 0.0f;
    pl->bridge_blend_smooth = 0.0f;
    pl->global_brightness_smooth = clamp01f(pl->calc_brightness);
    
    // Инициализируем состояние сцены для корректной работы анимаций
    pl->st_scene.t      = 0.0f;
    pl->st_scene.speed  = theme_fx_speed(theme, T->fx_main);
    pl->st_scene.phase  = theme_personality_phase(theme);
    pl->st_scene.a      = 0.0f;
    pl->st_scene.b      = 0.0f;
}

void base_scene_start_intro(led_runtime_strip_t *ws, base_scene_t *pl)
{
    if (!ws || !pl) return;
    const theme_definition_t *T = theme_get(pl->theme);
    if (!T) return;

    const palette_t *pal = palette_get(T->pal_main);
    if (!pal) return;

    /* Get zone parameters for STRIP */
    const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
    uint16_t first = zm->first;
    uint16_t count = zm->count;

    /* Guard against start-frame flash: intro always starts from dark strip frame. */
    for (uint16_t i = 0; i < count; ++i) {
        led_runtime_set_pixel_rgb(ws, (uint16_t)(first + i), 0u, 0u, 0u);
    }
    led_runtime_set_global_brightness(ws, 0.0f);
    pl->global_brightness_smooth = 0.0f;

    float base_br = clamp01f(pl->theme_brightness * pl->theme_dimming);
    pl->st_scene.speed = theme_fx_speed(pl->theme, T->fx_main);
    effect_oneshot_start(ws, &pl->intro, FX_WELCOME, pal, base_br, AMB_INTRO_DURATION_MS, first, count);
    pl->stage = BASE_SCENE_INTRO;
    pl->t0_ms = HAL_GetTick();
    pl->bridge_blend_smooth = 0.0f;
}

void base_scene_start_outro(led_runtime_strip_t *ws, base_scene_t *pl)
{
    if (!ws || !pl) return;
    const theme_definition_t *T = theme_get(pl->theme);
    if (!T) return;

    const palette_t *pal = palette_get(T->pal_main);
    if (!pal) return;

    /* Get zone parameters for STRIP */
    const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
    uint16_t first = zm->first;
    uint16_t count = zm->count;

    float base_br = clamp01f(pl->theme_brightness * pl->theme_dimming);
    effect_oneshot_start(ws, &pl->outro, FX_GOODBYE, pal, base_br, AMB_OUTRO_DURATION_MS, first, count);
    pl->stage = BASE_SCENE_OUTRO;
    pl->t0_ms = HAL_GetTick();
}

void base_scene_tick(led_runtime_strip_t *ws, base_scene_t *pl, uint32_t delta_ms)
{
    if (!ws || !pl) return;

    /* Power control: any non-idle stage -> power ON */
    if (pl->stage != BASE_SCENE_IDLE) {
        led_runtime_power_set(1);
    } else {
        led_runtime_power_set(0);
        return;
    }

    switch (pl->stage) {
    case BASE_SCENE_IDLE:
        return;

    case BASE_SCENE_INTRO:
    {
        uint8_t done = effect_oneshot_tick(ws, &pl->intro);
        // Финальный render шага выполняется в общем board dispatch pass,
        // чтобы синхронизировать главную ленту с другими зонами.
        if (done) {
            /* Переходим в BRIDGE для плавного перехода к сцене */
            pl->stage = BASE_SCENE_BRIDGE;
            pl->t0_ms = HAL_GetTick();
            pl->bridge_blend_smooth = 0.0f;
            // Инициализируем состояние сцены при переходе
            pl->st_scene.t      = 0.0f;
            base_scene_apply_scene_profile(pl, pl->theme);
            pl->st_scene.phase  = theme_personality_phase(pl->theme);
            pl->st_scene.a      = 0.0f;
            pl->st_scene.b      = 0.0f;

            /* Сохраняем финальный кадр intro для бриджа */
            const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
            uint16_t first = zm->first;
            uint16_t count = zm->count;
            uint16_t copy_count = (count > AMB_MAX_CROSSFADE_LEDS) ? AMB_MAX_CROSSFADE_LEDS : count;
            g_bridge_intro_count = copy_count;
            g_bridge_intro_first = first;
            g_bridge_intro_valid = 1;
            led_runtime_copy_rgb_from_strip(ws, first, copy_count, g_bridge_intro_rgb);
        }
    }
    break;

    case BASE_SCENE_ACTIVE:
    {
        const theme_definition_t *T = theme_get(pl->theme);
        if (!T) {
            pl->stage = BASE_SCENE_IDLE;
            break;
        }
        const palette_t *pal = palette_get(T->pal_main);
        if (!pal) {
            pl->stage = BASE_SCENE_IDLE;
            break;
        }

        const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
        uint16_t first = zm->first;
        uint16_t count = zm->count;

#if AMB_ENABLE_THEME_CROSSFADE
        /* --- Auto-rotate check --- */
#if AMB_ENABLE_AUTO_ROTATE
        if (pl->auto_rotate_enabled && !pl->crossfade_active) {
            uint32_t elapsed = HAL_GetTick() - pl->scene_start_ms;
            if (elapsed >= AUTO_ROTATE_INTERVAL_MS) {
                /* Trigger crossfade to next theme (uses global g_oem_color) */
                const theme_bank_t *bank = theme_get_bank(g_oem_color);
                if (bank && bank->count > 1) {
                    base_scene_start_theme_crossfade(pl, theme_bank_next(bank, pl->theme));
                }
            }
        }
#endif

        /* --- Crossfade rendering --- */
        if (pl->crossfade_active) {
            const theme_definition_t *T_next = theme_get(pl->theme_next);
            const palette_t *pal_next = T_next ? palette_get(T_next->pal_main) : NULL;
            
            if (!T_next || !pal_next) {
                /* Invalid next theme, cancel crossfade */
                pl->crossfade_active = 0;
            } else {
                uint32_t cf_elapsed = HAL_GetTick() - pl->crossfade_start_ms;
                float cf_progress = (float)cf_elapsed / (float)CROSSFADE_DURATION_MS;
                float target_blend = profile_blend_curve(smoothstep5(cf_progress));
                float blend = target_blend;
                if (cf_progress >= 1.0f) {
                    pl->crossfade_blend_smooth = 1.0f;
                    blend = 1.0f;
                } else {
                    pl->crossfade_blend_smooth = smooth_transition_blend(pl->crossfade_blend_smooth, target_blend);
                    blend = pl->crossfade_blend_smooth;
                }
                float inv_blend = 1.0f - blend;
                float tb_curr = theme_base_brightness(T);
                float tb_next = theme_base_brightness(T_next);
                
                /* Blend brightness between themes during crossfade */
                float blended_brightness = tb_curr * inv_blend + tb_next * blend;
                pl->calc_brightness = clamp01f(blended_brightness * pl->theme_dimming);
                
                if (cf_progress >= 1.0f) {
                    /* Crossfade complete - switch to next theme */
                    pl->theme = pl->theme_next;
                    pl->theme_brightness = tb_next;
                    pl->calc_brightness = clamp01f(pl->theme_brightness * pl->theme_dimming);
                    pl->st_scene = pl->st_scene_next;
                    pl->crossfade_active = 0;
                    pl->crossfade_blend_smooth = 0.0f;
                    pl->scene_start_ms = HAL_GetTick();
                    
                    /* Render final state of new theme */
                    effect_apply(ws, T_next->fx_main, pal_next, &pl->st_scene, delta_ms, first, count);
                } else {
                    /* Render current theme to temp buffer (we'll read from rgb) */
                    effect_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);
                    
                    /* Save current theme colors to static buffer */
                    static uint8_t temp_rgb[AMB_MAX_CROSSFADE_LEDS * 3];
                    uint16_t copy_count = (count > AMB_MAX_CROSSFADE_LEDS) ? AMB_MAX_CROSSFADE_LEDS : count;
                    led_runtime_copy_rgb_from_strip(ws, first, copy_count, temp_rgb);
                    
                    /* Render next theme */
                    effect_apply(ws, T_next->fx_main, pal_next, &pl->st_scene_next, delta_ms, first, count);
                    
                    /* Screen blend to prevent darkening during transition
                     * screen(A,B) = 1 - (1-A)*(1-B) = A + B - A*B
                     * This preserves brightness even when blending opposite wave phases */
                    for (uint16_t i = 0; i < copy_count; ++i) {
                        uint32_t idx = i * 3u;
                        uint8_t curr_r = temp_rgb[idx + 0];
                        uint8_t curr_g = temp_rgb[idx + 1];
                        uint8_t curr_b = temp_rgb[idx + 2];
                        
                        uint8_t next_r, next_g, next_b;
                        led_runtime_get_pixel_rgb(ws, (uint16_t)(first + i), &next_r, &next_g, &next_b);
                        
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
                        float screen_mix = AMB_CROSSFADE_SCREEN_MIX * mid_factor;
                        float lin_r = c_r * inv_blend + n_r * blend;
                        float lin_g = c_g * inv_blend + n_g * blend;
                        float lin_b = c_b * inv_blend + n_b * blend;
                        float out_r = lin_r + (screen_r - lin_r) * screen_mix;
                        float out_g = lin_g + (screen_g - lin_g) * screen_mix;
                        float out_b = lin_b + (screen_b - lin_b) * screen_mix;
                        
                        /* Clamp and convert back to 8-bit */
                        if (out_r > 1.0f) out_r = 1.0f;
                        if (out_g > 1.0f) out_g = 1.0f;
                        if (out_b > 1.0f) out_b = 1.0f;
                        
                        led_runtime_set_pixel_rgb(ws, first + i, 
                            (uint8_t)(out_r * 255.0f + 0.5f),
                            (uint8_t)(out_g * 255.0f + 0.5f),
                            (uint8_t)(out_b * 255.0f + 0.5f));
                    }
                }
            }
        } else {
            /* Normal scene rendering (no crossfade) */
            effect_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);
        }
#else
        /* Auto-rotate disabled - just render scene */
        effect_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);
#endif /* AMB_ENABLE_AUTO_ROTATE */
    }
    break;

    case BASE_SCENE_OUTRO:
    {
        uint8_t done = effect_oneshot_tick(ws, &pl->outro);
        // Финальный render шага выполняется в общем board dispatch pass,
        // чтобы синхронизировать главную ленту с другими зонами.
        if (done) {
            frame_clear(ws);
            // Финальный рендер черного экрана будет выполнен в общем board dispatch pass.
            pl->stage = BASE_SCENE_IDLE;
            led_runtime_power_set(0);
        }
    }
    break;

    case BASE_SCENE_BRIDGE:
    {
        /* Плавный переход от финального состояния intro к сцене */
        const theme_definition_t *T = theme_get(pl->theme);
        if (!T) {
            pl->stage = BASE_SCENE_ACTIVE;
            break;
        }
        const palette_t *pal = palette_get(T->pal_main);
        if (!pal) {
            pl->stage = BASE_SCENE_ACTIVE;
            break;
        }

        uint32_t elapsed = HAL_GetTick() - pl->t0_ms;
        float progress = (float)elapsed / (float)BRIDGE_DURATION_MS;
        
        if (progress >= 1.0f) {
            /* Переход завершён - переходим к сцене */
            pl->stage = BASE_SCENE_ACTIVE;
            pl->t0_ms = HAL_GetTick();
            pl->scene_start_ms = HAL_GetTick();  /* Start auto-rotate timer */
            pl->bridge_blend_smooth = 0.0f;
            break;
        }

        /* Плавное нарастание эффекта сцены с помощью quintic smoothstep */
        float target_blend = profile_blend_curve(smoothstep5(progress));
        pl->bridge_blend_smooth = smooth_transition_blend(pl->bridge_blend_smooth, target_blend);
        float blend = pl->bridge_blend_smooth;
        float inv_blend = 1.0f - blend;

        const zone_map_t *zm = &g_zone_map[ZONE_STRIP];
        uint16_t first = zm->first;
        uint16_t count = zm->count;

        /* Финальное состояние intro - снимаем из сохранённого буфера, fallback: центр палитры */
        const uint8_t *intro_buf = NULL;
        uint16_t intro_count = 0;
        if (g_bridge_intro_valid && g_bridge_intro_first == first) {
            intro_buf = g_bridge_intro_rgb;
            intro_count = g_bridge_intro_count;
        } else {
            g_bridge_intro_valid = 0;
        }

        uint8_t intro_r_fallback, intro_g_fallback, intro_b_fallback;
        palette_sample_rgb8(pal, 0.5f, &intro_r_fallback, &intro_g_fallback, &intro_b_fallback);

        /* Применяем эффект сцены (записывает цвета в буфер) */
        effect_apply(ws, T->fx_main, pal, &pl->st_scene, delta_ms, first, count);

        for (uint16_t i = 0; i < count; ++i) {
            /* Вытаскиваем сохранённый цвет интро, либо fallback */
            float i_r, i_g, i_b;
            if (intro_buf && i < intro_count) {
                i_r = intro_buf[i * 3u + 0] / 255.0f;
                i_g = intro_buf[i * 3u + 1] / 255.0f;
                i_b = intro_buf[i * 3u + 2] / 255.0f;
            } else {
                i_r = intro_r_fallback / 255.0f;
                i_g = intro_g_fallback / 255.0f;
                i_b = intro_b_fallback / 255.0f;
            }

            uint8_t scene_r, scene_g, scene_b;
            led_runtime_get_pixel_rgb(ws, (uint16_t)(first + i), &scene_r, &scene_g, &scene_b);
            
            float s_r = scene_r / 255.0f;
            float s_g = scene_g / 255.0f;
            float s_b = scene_b / 255.0f;
            
            /* Screen blend: prevents darkening */
            float scr_r = i_r + s_r - i_r * s_r;
            float scr_g = i_g + s_g - i_g * s_g;
            float scr_b = i_b + s_b - i_b * s_b;
            
            /* Linear blend */
            float lin_r = i_r * inv_blend + s_r * blend;
            float lin_g = i_g * inv_blend + s_g * blend;
            float lin_b = i_b * inv_blend + s_b * blend;
            
            /* Добавляем screen поверх линейного, чтобы не было затемнения */
            float mid_factor = 4.0f * blend * inv_blend;
            float screen_mix = AMB_BRIDGE_SCREEN_MIX * mid_factor;
            float out_r = lin_r + (scr_r - lin_r) * screen_mix;
            float out_g = lin_g + (scr_g - lin_g) * screen_mix;
            float out_b = lin_b + (scr_b - lin_b) * screen_mix;
            
            if (out_r > 1.0f) out_r = 1.0f;
            if (out_g > 1.0f) out_g = 1.0f;
            if (out_b > 1.0f) out_b = 1.0f;
            
            led_runtime_set_pixel_rgb(ws, first + i, 
                (uint8_t)(out_r * 255.0f + 0.5f),
                (uint8_t)(out_g * 255.0f + 0.5f),
                (uint8_t)(out_b * 255.0f + 0.5f));
        }
    }
    break;

    default:
        pl->stage = BASE_SCENE_ACTIVE;
        break;
    }

    base_scene_apply_global_brightness(ws, pl);

    /* Clear LEDs before zone.first (they are still sent via DMA) */
    clear_unused_leds(ws);
}

#if AMB_ENABLE_AUTO_ROTATE
/* -----------------------------------------------------------------------
 * Auto-rotate control functions
 * ----------------------------------------------------------------------- */

void base_scene_set_auto_rotate(base_scene_t *pl, uint8_t enable)
{
    if (!pl) return;
    
    pl->auto_rotate_enabled = enable;
    
    /* Reset timer when enabling */
    if (enable) {
        pl->scene_start_ms = HAL_GetTick();
    }
}

void base_scene_trigger_next_theme(base_scene_t *pl)
{
    if (!pl) return;
    if (pl->stage != BASE_SCENE_ACTIVE) return;  /* Only trigger in scene mode */
    if (pl->crossfade_active) return;    /* Already crossfading */
    
    /* Uses global g_oem_color for bank selection */
    const theme_bank_t *bank = theme_get_bank(g_oem_color);
    if (!bank || bank->count <= 1) return;
    
    base_scene_start_theme_crossfade(pl, theme_bank_next(bank, pl->theme));
}
#endif /* AMB_ENABLE_AUTO_ROTATE */
