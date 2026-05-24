#pragma once

#include <stdint.h>

#include "base_scene.h"
#include "director.h"
#include "led_runtime.h"
#include "main.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RUNTIME_BOOT = 0,      /* Initial startup state before runtime loop settles. */
    RUNTIME_WAIT_OEM,      /* Wait for valid CAN/OEM inputs and start gates. */
    RUNTIME_ACTIVE,        /* Main active render/event processing state. */
    RUNTIME_SLEEP_PREP,    /* Transitional fade/pre-stop preparation state. */
    RUNTIME_STOP,          /* MCU STOP low-power state. */
    RUNTIME_WAKE_RECOVER   /* Post-STOP recovery and re-arm state. */
} runtime_mode_t;

/** Context bundle for runtime flow state machine and board hooks. */
typedef struct {
    runtime_mode_t *runtime_state;
    uint8_t *can_protocol_started;
    uint32_t *can_protocol_start_ms;
    uint32_t *wait_oem_enter_ms;
    uint8_t *wait_oem_after_wake;
    uint32_t *last_tick_ms;
#if AMB_ENABLE_SLEEP_MODE
    uint8_t *sleep_fade_active;
    uint32_t *sleep_fade_start_ms;
    float *sleep_fade_start_brightness;
#endif
    base_scene_t *base_scene;
    director_t *director;
    oem_color_id_t *oem_color;
    IWDG_HandleTypeDef *watchdog;
    void (*system_clock_restore)(void);
#if AMB_ENABLE_SLEEP_MODE
    void (*prepare_wakeup_io)(void);
    void (*restore_after_wake)(void);
    void (*rtc_wakeup_start)(void);
    void (*rtc_wakeup_stop)(void);
#endif
} runtime_flow_ctx_t;

/** Request runtime mode transition with guard bookkeeping. */
void runtime_flow_set_state(runtime_flow_ctx_t *ctx, runtime_mode_t s);
/** Dispatch one runtime tick according to current mode. */
uint8_t runtime_flow_dispatch_state(runtime_flow_ctx_t *ctx,
                                    led_runtime_strip_t *main_strip,
                                    uint32_t dt,
                                    uint8_t oem_received);
/** Execute ACTIVE state pipeline for one tick. */
uint8_t runtime_flow_handle_active_state(runtime_flow_ctx_t *ctx,
                                         led_runtime_strip_t *main_strip,
                                         uint32_t now,
                                         uint32_t dt,
                                         uint8_t oem_received);

#ifdef __cplusplus
}
#endif
