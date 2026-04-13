#pragma once

#include <stdint.h>

#include "base_scene.h"
#include "director.h"
#include "led_runtime.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RUNTIME_BOOT = 0,
    RUNTIME_WAIT_OEM,
    RUNTIME_ACTIVE,
    RUNTIME_SLEEP_PREP,
    RUNTIME_STOP,
    RUNTIME_WAKE_RECOVER
} runtime_mode_t;

typedef struct {
    runtime_mode_t *runtime_state;
    uint8_t *can_protocol_started;
    uint32_t *can_protocol_start_ms;
    uint32_t *wait_oem_enter_ms;
    uint8_t *wait_oem_after_wake;
    uint32_t *last_tick_ms;
#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
    uint8_t *sleep_fade_active;
    uint32_t *sleep_fade_start_ms;
    float *sleep_fade_start_brightness;
#endif
    base_scene_t *base_scene;
    director_t *director;
    theme_id_t *current_theme;
    oem_color_id_t *oem_color;
    IWDG_HandleTypeDef *watchdog;
    void (*demo_mode_update)(uint32_t now_ms);
    void (*system_clock_restore)(void);
#if AMB_ENABLE_SLEEP_MODE && !DEMO_MODE
    void (*prepare_wakeup_io)(void);
    void (*restore_after_wake)(void);
    void (*rtc_wakeup_start)(void);
    void (*rtc_wakeup_stop)(void);
#endif
#if AMB_DEBUG_BSM_RX_PULSE
    volatile uint32_t *dbg_bsm_rx_until_ms;
    volatile uint32_t *dbg_non_oem_rx_until_ms;
    volatile uint32_t *dbg_non_oem_rx_id;
#endif
} runtime_flow_ctx_t;

void runtime_flow_set_state(runtime_flow_ctx_t *ctx, runtime_mode_t s);
uint8_t runtime_flow_dispatch_state(runtime_flow_ctx_t *ctx,
                                    led_runtime_strip_t *main_strip,
                                    uint32_t dt,
                                    uint8_t oem_received);
uint8_t runtime_flow_handle_active_state(runtime_flow_ctx_t *ctx,
                                         led_runtime_strip_t *main_strip,
                                         uint32_t now,
                                         uint32_t dt,
                                         uint8_t oem_received);

#ifdef __cplusplus
}
#endif
