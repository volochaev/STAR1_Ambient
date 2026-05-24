/**
 * @file ambient_power.c
 * @brief CAN-driven sleep/wake policy and diagnostics.
 */
#include "ambient_power.h"

#if AMB_ENABLE_SLEEP_MODE

#include <string.h>

#include "ambient_config.h"
#include "led_runtime.h"

static volatile uint32_t g_last_can_activity_ms = 0u;
static volatile uint8_t g_sleep_requested = 0u;
static volatile uint8_t g_is_sleeping = 0u;
static volatile can_power_diag_t g_power_diag = {0};

/* Wrap-safe elapsed milliseconds helper. */
static uint32_t elapsed_ms32(uint32_t now, uint32_t then)
{
    return (now >= then) ? (now - then) : ((UINT32_MAX - then) + now + 1u);
}

/* Initialize power manager state and diagnostics. */
void can_power_init(void)
{
    g_last_can_activity_ms = HAL_GetTick();
    g_sleep_requested = 0u;
    g_is_sleeping = 0u;
    memset((void *)&g_power_diag, 0, sizeof(g_power_diag));
}

/* Register bus activity and clear idle-timeout sleep request when needed. */
void can_power_note_can_activity(void)
{
    g_last_can_activity_ms = HAL_GetTick();
    if (g_sleep_requested && g_power_diag.last_sleep_reason == CAN_SLEEP_REASON_IDLE_TIMEOUT) {
        g_sleep_requested = 0u;
    }
    if (g_is_sleeping) {
        g_power_diag.last_wake_reason = CAN_WAKE_REASON_CAN_RX;
        g_power_diag.last_wake_ms = g_last_can_activity_ms;
        g_power_diag.wake_count++;
    }
    g_is_sleeping = 0u;
}

/* Report whether low-power mode is currently active. */
uint8_t can_power_is_sleeping(void)
{
    return g_is_sleeping;
}

/* Report whether sleep transition has been requested. */
uint8_t can_power_should_sleep(void)
{
    return g_sleep_requested;
}

/* Clear pending sleep request flag. */
void can_power_clear_sleep_request(void)
{
    g_sleep_requested = 0u;
}

/* Request sleep due to lock event policy. */
void can_power_request_sleep_lock(void)
{
    uint32_t now;
    if (g_is_sleeping || g_sleep_requested) {
        return;
    }
    now = HAL_GetTick();
    g_sleep_requested = 1u;
    g_power_diag.sleep_request_count++;
    g_power_diag.last_sleep_reason = CAN_SLEEP_REASON_LOCK_EVENT;
    g_power_diag.last_sleep_request_ms = now;
    g_power_diag.last_idle_ms_at_request = elapsed_ms32(now, g_last_can_activity_ms);
}

/* Mark module as awake from explicit runtime path. */
void can_power_mark_awake(void)
{
    g_is_sleeping = 0u;
    g_sleep_requested = 0u;
    g_last_can_activity_ms = HAL_GetTick();
    g_power_diag.last_wake_reason = CAN_WAKE_REASON_MANUAL_AWAKE;
    g_power_diag.last_wake_ms = g_last_can_activity_ms;
    g_power_diag.wake_count++;
}

/* Return current idle time since last CAN activity. */
uint32_t can_power_get_idle_time_ms(void)
{
    uint32_t now = HAL_GetTick();
    return elapsed_ms32(now, g_last_can_activity_ms);
}

/* Evaluate idle timeout and arm sleep request when threshold is reached. */
void can_power_check_sleep_timeout(void)
{
    uint32_t idle_ms;
    uint32_t timeout_ms;

    if (g_is_sleeping || g_sleep_requested) {
        return;
    }

    idle_ms = can_power_get_idle_time_ms();
    timeout_ms = AMB_SLEEP_TIMEOUT_SEC * 1000u;

    if (idle_ms >= timeout_ms) {
        g_sleep_requested = 1u;
        g_power_diag.sleep_request_count++;
        g_power_diag.last_sleep_reason = CAN_SLEEP_REASON_IDLE_TIMEOUT;
        g_power_diag.last_sleep_request_ms = HAL_GetTick();
        g_power_diag.last_idle_ms_at_request = idle_ms;
    }
}

/* Enter sleep mode: power down LEDs, set transceiver standby, stop CAN. */
void can_power_enter_sleep(FDCAN_HandleTypeDef *hfdcan)
{
    if (g_is_sleeping) {
        return;
    }

    g_is_sleeping = 1u;
    g_power_diag.sleep_enter_count++;
    if (g_power_diag.last_sleep_reason == CAN_SLEEP_REASON_NONE) {
        g_power_diag.last_sleep_reason = CAN_SLEEP_REASON_API_ENTER;
    }
    g_power_diag.last_sleep_enter_ms = HAL_GetTick();

    led_runtime_power_set(0u);

#ifdef FDCAN1_STBY_Pin
#if (AMB_CAN_WAKE_POLICY == AMB_CAN_WAKE_BENCH)
    HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_RESET);
#else
    HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_SET);
#endif
#endif

    if (hfdcan) {
        HAL_FDCAN_Stop(hfdcan);
    }
}

/* Exit sleep mode and restore CAN notifications/LED power policy. */
void can_power_exit_sleep(FDCAN_HandleTypeDef *hfdcan)
{
    if (!g_is_sleeping) {
        return;
    }

#ifdef FDCAN1_STBY_Pin
    HAL_GPIO_WritePin(FDCAN1_STBY_GPIO_Port, FDCAN1_STBY_Pin, GPIO_PIN_RESET);
#endif

    if (hfdcan) {
        HAL_FDCAN_Start(hfdcan);
        HAL_FDCAN_ActivateNotification(hfdcan,
            FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
    }

    led_runtime_power_set(0u);

    g_is_sleeping = 0u;
    g_sleep_requested = 0u;
    g_last_can_activity_ms = HAL_GetTick();
    if (g_power_diag.last_wake_reason == CAN_WAKE_REASON_NONE ||
        g_power_diag.last_wake_ms < g_power_diag.last_sleep_enter_ms) {
        g_power_diag.last_wake_reason = CAN_WAKE_REASON_EXIT_SLEEP;
        g_power_diag.last_wake_ms = g_last_can_activity_ms;
        g_power_diag.wake_count++;
    }
}

/* Record STOP wakeup source for diagnostics. */
void can_power_note_stop_wakeup(uint8_t wake_src)
{
    uint32_t now = HAL_GetTick();

    if (wake_src == 1u) {
        g_power_diag.last_wake_reason = CAN_WAKE_REASON_STOP_EXTI_CAN_RX;
    } else if (wake_src == 2u) {
        g_power_diag.last_wake_reason = CAN_WAKE_REASON_STOP_EXTI_TRANSCEIVER;
    } else {
        return;
    }
    g_power_diag.last_wake_ms = now;
    g_power_diag.wake_count++;
}

void can_power_get_diag(can_power_diag_t *out)
{
    if (!out) return;
    __disable_irq();
    *out = g_power_diag;
    __enable_irq();
}

void can_power_reset_diag(void)
{
    __disable_irq();
    memset((void *)&g_power_diag, 0, sizeof(g_power_diag));
    __enable_irq();
}

#endif
