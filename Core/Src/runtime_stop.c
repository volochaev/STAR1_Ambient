/**
 * @file runtime_stop.c
 * @brief STOP-mode entry/wakeup flow helpers.
 */
#include "runtime_stop.h"

static volatile uint8_t g_runtime_wakeup_event = 0u;
static volatile uint8_t g_runtime_wakeup_source = 0u;

/* Reset software wakeup latches before STOP entry. */
void runtime_stop_reset_wakeup(void)
{
    g_runtime_wakeup_event = 0u;
    g_runtime_wakeup_source = 0u;
}

/* ISR/helper entry point to signal wakeup source. */
void runtime_stop_signal_wakeup(uint8_t source)
{
    g_runtime_wakeup_event = 1u;
    g_runtime_wakeup_source = source;
}

/* Return last captured wakeup source code. */
uint8_t runtime_stop_get_wakeup_source(void)
{
    return g_runtime_wakeup_source;
}

/* Enter STOP mode and wait for wakeup, including watchdog-aware loop mode. */
void runtime_stop_enter_and_wait(const runtime_stop_hooks_t *hooks)
{
    if (!hooks) return;

    if (hooks->prepare_wakeup_io) {
        hooks->prepare_wakeup_io();
    }

    runtime_stop_reset_wakeup();

    if (hooks->watchdog_enabled) {
        if (hooks->rtc_wakeup_start) {
            hooks->rtc_wakeup_start();
        }
        if (hooks->watchdog) {
            HAL_IWDG_Refresh(hooks->watchdog);
        }
        while (!g_runtime_wakeup_event) {
            HAL_SuspendTick();
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            if (hooks->restore_system_clock) {
                hooks->restore_system_clock();
            }
            HAL_ResumeTick();
            if (!g_runtime_wakeup_event && hooks->watchdog) {
                HAL_IWDG_Refresh(hooks->watchdog);
            }
        }
        if (hooks->rtc_wakeup_stop) {
            hooks->rtc_wakeup_stop();
        }
    } else {
        HAL_SuspendTick();
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
        if (hooks->restore_system_clock) {
            hooks->restore_system_clock();
        }
        HAL_ResumeTick();
    }

    if (hooks->restore_after_wake) {
        hooks->restore_after_wake();
    }
}
