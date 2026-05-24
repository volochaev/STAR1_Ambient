#include "handle_pwm.h"
#include "ambient_config.h"

/* PT4211 DIM may be wired as sink-to-GND (active-low) on some boards. */
#ifndef AMB_HANDLE_PWM_ACTIVE_LOW
#define AMB_HANDLE_PWM_ACTIVE_LOW 1u
#endif
#ifndef AMB_HANDLE_PWM_ATTACK_MS
#define AMB_HANDLE_PWM_ATTACK_MS 1400u
#endif
#ifndef AMB_HANDLE_PWM_RELEASE_MS
#define AMB_HANDLE_PWM_RELEASE_MS 2200u
#endif

static TIM_HandleTypeDef *s_htim = NULL;
static uint32_t s_channel = TIM_CHANNEL_1;
static uint8_t s_enabled = 0u;
static uint8_t s_brightness_pct = 100u;
static float s_level = 0.0f;
static float s_target_level = 0.0f;
static uint16_t s_cur = 0u;

static float clamp01f(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float smoothstep3(float t)
{
    t = clamp01f(t);
    return t * t * (3.0f - 2.0f * t);
}

static float slew_level(float current, float target, uint32_t dt_ms)
{
    uint32_t tau_ms;
    float alpha;

    tau_ms = (target >= current) ? AMB_HANDLE_PWM_ATTACK_MS : AMB_HANDLE_PWM_RELEASE_MS;
    if (tau_ms == 0u) return target;

    if (dt_ms > 100u) dt_ms = 100u;
    alpha = (float)dt_ms / ((float)tau_ms + (float)dt_ms);
    /* Use alpha directly: applying smoothstep(alpha) at small dt makes
     * the effective response many times slower than configured tau. */
    current += (target - current) * alpha;
    if ((target - current < 0.001f) && (target - current > -0.001f)) {
        current = target;
    }
    return clamp01f(current);
}

static uint16_t pwm_off_compare(void)
{
    if (!s_htim) return 0u;
#if AMB_HANDLE_PWM_ACTIVE_LOW
    return (uint16_t)s_htim->Init.Period;
#else
    return 0u;
#endif
}

static uint16_t pwm_compare_from_level(float level)
{
    uint32_t max_cmp;
    uint32_t cmp_u32;
    uint16_t cmp;
    if (!s_htim) return 0u;

    level = clamp01f(level);
    level = smoothstep3(level);
    max_cmp = s_htim->Init.Period;
    cmp_u32 = (uint32_t)(level * (float)max_cmp + 0.5f);
    if (cmp_u32 > max_cmp) cmp_u32 = max_cmp;
    cmp = (uint16_t)cmp_u32;
#if AMB_HANDLE_PWM_ACTIVE_LOW
    cmp = (uint16_t)(max_cmp - cmp);
#endif
    return cmp;
}

void handle_pwm_init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    s_htim = htim;
    s_channel = channel;
    s_enabled = 0u;
    s_brightness_pct = 100u;
    s_level = 0.0f;
    s_target_level = 0.0f;

    if (!s_htim) return;
    s_cur = pwm_off_compare();
    (void)HAL_TIM_PWM_Start(s_htim, s_channel);
    __HAL_TIM_SET_COMPARE(s_htim, s_channel, s_cur);
}

void handle_pwm_set_enabled(uint8_t enabled)
{
    s_enabled = enabled ? 1u : 0u;
}

void handle_pwm_set_brightness_pct(uint8_t pct)
{
    if (pct > 100u) pct = 100u;
    s_brightness_pct = pct;
}

void handle_pwm_tick(uint32_t dt_ms)
{
    if (!s_htim) return;

    if (s_enabled) {
        s_target_level = (float)s_brightness_pct / 100.0f;
    } else {
        s_target_level = 0.0f;
    }

    s_level = slew_level(s_level, s_target_level, dt_ms);
    s_cur = pwm_compare_from_level(s_level);

    __HAL_TIM_SET_COMPARE(s_htim, s_channel, s_cur);
}

uint8_t handle_pwm_is_off(void)
{
    return (uint8_t)(s_level <= 0.001f);
}

void handle_pwm_prepare_stop(void)
{
    if (!s_htim) return;
    s_enabled = 0u;
    s_level = 0.0f;
    s_target_level = 0.0f;
    s_cur = pwm_off_compare();
    __HAL_TIM_SET_COMPARE(s_htim, s_channel, s_cur);
    (void)HAL_TIM_PWM_Stop(s_htim, s_channel);
}

void handle_pwm_resume_after_stop(void)
{
    if (!s_htim) return;
    (void)HAL_TIM_PWM_Start(s_htim, s_channel);
    s_level = 0.0f;
    s_target_level = 0.0f;
    s_cur = pwm_off_compare();
    __HAL_TIM_SET_COMPARE(s_htim, s_channel, s_cur);
}
