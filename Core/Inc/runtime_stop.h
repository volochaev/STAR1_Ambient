#pragma once

#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    IWDG_HandleTypeDef *watchdog;
    uint8_t watchdog_enabled;
    void (*prepare_wakeup_io)(void);
    void (*restore_after_wake)(void);
    void (*restore_system_clock)(void);
    void (*rtc_wakeup_start)(void);
    void (*rtc_wakeup_stop)(void);
} runtime_stop_hooks_t;

void runtime_stop_reset_wakeup(void);
void runtime_stop_signal_wakeup(uint8_t source);
uint8_t runtime_stop_get_wakeup_source(void);
void runtime_stop_enter_and_wait(const runtime_stop_hooks_t *hooks);

#ifdef __cplusplus
}
#endif
