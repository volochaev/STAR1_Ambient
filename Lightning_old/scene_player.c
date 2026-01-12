#include "scene_player.h"
#include "LED.h"
#include <string.h>
#include <math.h>

/*
 * Важно:
 *  - ws_oneshot_tick, ws_theme_* и ws_fx_* только пишут в ws->grb.
 *  - НИГДЕ внутри них не вызывается ws_render / DMA.
 *  - Здесь, в player_tick, мы гарантированно вызываем ws_render(ws) ровно 1 раз за кадр.
 */

#ifndef LED_NUM
#define LED_NUM 120u
#endif

/* Буферы для плавного моста INTRO -> SCENE */
static uint8_t  g_intro_last[LED_NUM * 3u];
static uint8_t  g_scene_first[LED_NUM * 3u];
static uint16_t g_bridge_leds = 0;

static inline float clamp01f(float x)
{
    return (x < 0.f) ? 0.f : (x > 1.f ? 1.f : x);
}

/* Красивый easing вместо линейного: smoothstep */
static inline float ease_smooth(float t)
{
    // t in [0,1]
    return t * t * (3.f - 2.f * t);
}


/* ───────── INTERNAL HELPERS ───────── */

/* Универсальная подготовка плавного моста:
 *  - Берём ТЕКУЩИЙ кадр в ws->grb как начало.
 *  - Считаем первый кадр новой темы в g_scene_first.
 *  - Настраиваем st_scene под новую тему.
 *  - Дальше PST_BRIDGE сам сделает lerp(from -> to) и переведёт в PST_SCENE.
 */
static void prepare_bridge_to_theme(ws2812_t *ws,
                                    scene_player_t *pl,
                                    ws_theme_id_t new_theme)
{
    if (!ws || !pl) return;

    const ws_scene_desc_t *S = ws_scene_by_theme(new_theme);
    if (!S) return;

    uint16_t n = ws->led_count;
    if (!n) return;
#if LED_NUM
    if (n > LED_NUM) n = LED_NUM;
#endif

    g_bridge_leds = n;

    /* 1) FROM: текущий кадр (последний MB_WELCOME или текущая сцена) */
    memcpy(g_intro_last, ws->grb, (size_t)n * 3u);

    /* 2) TO: первый кадр новой темы, в теневом ws */
    memset(&pl->st_scene, 0, sizeof(pl->st_scene));

    ws2812_t shadow = *ws;
    shadow.grb       = g_scene_first;
    shadow.led_count = n;

    float new_br = S->default_brightness;
    ws_theme_render_scene(&shadow,
                          new_theme,
                          &pl->st_scene,
                          &new_br);

    /* 3) Обновляем параметры темы в плеере */
    pl->theme           = new_theme;
    pl->theme_brightness= clamp01f(new_br);
    pl->calc_brightness = clamp01f(pl->theme_brightness * pl->theme_dimming);

    /* 4) Переходим в BRIDGE: PST_BRIDGE сам плавно сведёт from -> to */
    pl->stage = PST_BRIDGE;
    pl->t0_ms = HAL_GetTick();
}

void player_tick(ws2812_t *ws,
                 scene_player_t *pl)
{
    if (!ws || !pl) return;

    /* Питание:
     *  - Любая стадия, кроме PST_IDLE → питание ON.
     *  - PST_IDLE → питание OFF и выходим.
     */
    if (pl->stage != PST_IDLE) {
        ws_power_set(1);
    } else {
        ws_power_set(0);
        return;
    }

    /* Яркость:
     *  Считается в плеере (theme_brightness * theme_dimming = calc_brightness).
     *  Применяем её для INTRO / BRIDGE / SCENE.
     *  Для OUTRO яркость задаёт сам oneshot (MB_GOODBYE), чтобы не было конфликтов.
     */
    float gbr = clamp01f(pl->calc_brightness);
    if (pl->stage != PST_OUTRO) {
        ws_set_global_brightness(ws, gbr);
    }

    switch (pl->stage)
    {
    case PST_IDLE:
        /* Сюда мы фактически не попадём (return выше), оставлено для читаемости. */
        return;

    case PST_INTRO:
    {
        uint8_t done = ws_oneshot_tick(ws, &pl->intro);
        ws_render(ws);

        if (done) {
            /* Плавный переход:
             * from = последний кадр MB_WELCOME (ws->grb),
             * to   = первый кадр SCENE для текущей темы.
             */
            prepare_bridge_to_theme(ws, pl, pl->theme);
        }
    }
    break;

    case PST_BRIDGE:
    {
        if (g_bridge_leds == 0) {
            /* Если что-то пошло не так — сразу в сцену. */
            pl->stage = PST_SCENE;
            break;
        }

        uint32_t elapsed   = HAL_GetTick() - pl->t0_ms;
        const uint32_t dur = 300u;  /* длительность моста, можно тюнить */
        float t = (dur == 0u) ? 1.f : (float)elapsed / (float)dur;
        if (t >= 1.f) t = 1.f;

        float s = ease_smooth(t);
        uint32_t nbytes = (uint32_t)g_bridge_leds * 3u;

        for (uint32_t i = 0; i < nbytes; ++i) {
            float a = (float)g_intro_last[i];   /* from */
            float b = (float)g_scene_first[i];  /* to */
            ws->grb[i] = (uint8_t)(a + (b - a) * s + 0.5f);
        }

        ws_render(ws);

        if (t >= 1.f) {
            /* Мост закончен:
             * st_scene уже инициализирован под SCENE в prepare_bridge_to_theme,
             * поэтому продолжаем анимацию без ступеньки.
             */
            pl->stage = PST_SCENE;
            pl->t0_ms = HAL_GetTick();
        }
    }
    break;

    case PST_SCENE:
    {
        /* Рендерим текущую тему, она может чуть подстроить theme_brightness. */
        ws_theme_render_scene(ws,
                              pl->theme,
                              &pl->st_scene,
                              &pl->theme_brightness);

        pl->theme_brightness = clamp01f(pl->theme_brightness);
        pl->calc_brightness  =
            clamp01f(pl->theme_brightness * pl->theme_dimming);

        ws_render(ws);
    }
    break;

    case PST_OUTRO:
    {
        /* Здесь яркость контролирует сам FX_MB_GOODBYE (через os->base_br).
         * Мы не трогаем global_brightness (см. if выше),
         * чтобы не дублировать fade.
         */
        uint8_t done = ws_oneshot_tick(ws, &pl->outro);
        ws_render(ws);

        if (done) {
            /* Гарантируем чёрный кадр на ленте */
            ws_force_brightness(ws, 0.0f);
            ws_render(ws);
            ws_release_brightness(ws);

            pl->stage = PST_IDLE;
            ws_power_set(0);
        }
    }
    break;

    default:
        pl->stage = PST_IDLE;
        ws_power_set(0);
        break;
    }
}

/* ───────── PUBLIC API ───────── */

void player_init(scene_player_t *pl)
{
	if (!pl) return;
	memset(pl, 0, sizeof(*pl));

	pl->stage           = PST_IDLE;
	pl->theme           = THEME_WARM_LOUNGE;  // какой-то дефолт
	pl->theme_brightness= 0.30f;
	pl->theme_dimming   = 1.0f;
	pl->calc_brightness = pl->theme_brightness * pl->theme_dimming;
	pl->t0_ms           = HAL_GetTick();
}

void player_start_theme(ws2812_t *ws,
                        scene_player_t *pl,
                        ws_theme_id_t theme,
                        float theme_dimming)
{
    if (!ws || !pl) return;

    theme_dimming = clamp01f(theme_dimming);

    /* Если просто обновили ручку яркости для той же темы в SCENE/INTRO — не рвём. */
    if (pl->theme == theme &&
        (pl->stage == PST_SCENE || pl->stage == PST_INTRO))
    {
        pl->theme_dimming = theme_dimming;
        pl->calc_brightness =
            clamp01f(pl->theme_brightness * pl->theme_dimming);
        return;
    }

    /* Если мы в IDLE или только что потушились — классический MB_WELCOME */
    if (pl->stage == PST_IDLE || pl->stage == PST_OUTRO)
    {
        const ws_scene_desc_t *S = ws_scene_by_theme(theme);
        if (!S) return;

        pl->theme            = theme;
        pl->theme_brightness = clamp01f(S->default_brightness);
        pl->theme_dimming    = theme_dimming;
        pl->calc_brightness  =
            clamp01f(pl->theme_brightness * pl->theme_dimming);

        ws_theme_start_intro(ws,
                             &pl->intro,
                             theme,
                             pl->calc_brightness);

        pl->stage = PST_INTRO;
        pl->t0_ms = HAL_GetTick();
        g_bridge_leds = 0;
        return;
    }

    /* Если идёт стабильная сцена — делаем плавный мост SCENE(A) -> SCENE(B) */
    if (pl->stage == PST_SCENE)
    {
        pl->theme_dimming = theme_dimming;
        /* from = текущий кадр сцены в ws->grb, to = первая сцена новой темы */
        prepare_bridge_to_theme(ws, pl, theme);
        return;
    }

    /* Если нас поймали в INTRO/BRIDGE/OUTRO —
       не усложняем: рестарт MB_WELCOME для новой темы. */
    const ws_scene_desc_t *S = ws_scene_by_theme(theme);
    if (!S) return;

    pl->theme            = theme;
    pl->theme_brightness = clamp01f(S->default_brightness);
    pl->theme_dimming    = theme_dimming;
    pl->calc_brightness  =
        clamp01f(pl->theme_brightness * pl->theme_dimming);

    ws_theme_start_intro(ws,
                         &pl->intro,
                         theme,
                         pl->calc_brightness);

    pl->stage = PST_INTRO;
    pl->t0_ms = HAL_GetTick();
    g_bridge_leds = 0;
}
void player_start_outro(ws2812_t *ws,
                        scene_player_t *pl)
{
    if (!ws || !pl) return;
    if (pl->stage == PST_IDLE || pl->stage == PST_OUTRO)
        return;

    float current_br = ws_effective_brightness(ws);

    /* Чтобы не было current_br * old_global,
     * на время OUTRO работаем в нормализованном 1.0.
     */
    ws_set_global_brightness(ws, 1.0f);

    ws_theme_start_outro(ws,
                         &pl->outro,
                         pl->theme,
                         current_br);

    pl->stage = PST_OUTRO;
    pl->t0_ms = HAL_GetTick();
}

/* Реально применим в следующем tick через ws_force_brightness */
void player_set_theme_dimming(ws2812_t *ws, scene_player_t *pl, float theme_dimming) {
    (void)ws;
    if (!pl) return;
    pl->theme_dimming   = clamp01f(theme_dimming);
    pl->calc_brightness = clamp01f(pl->theme_brightness * pl->theme_dimming);
}
