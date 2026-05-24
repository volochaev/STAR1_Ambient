/**
 * @file app_runtime.c
 * @brief Main application runtime tick orchestration.
 */
#include "app_runtime.h"

#include "ambient.h"
#include "board_dispatch.h"
#include "led_runtime.h"
#include "runtime_event_queue.h"

#define APP_RUNTIME_CAN_EVENTS_BUDGET_PER_TICK 24u

/* Execute one runtime loop tick: drain CAN queue, update role, run state flow. */
void app_runtime_tick(runtime_flow_ctx_t *ctx,
                      uint32_t *last_tick_ms,
                      uint8_t *can_protocol_started,
                      uint32_t *can_protocol_start_ms)
{
    uint32_t now;
    uint32_t dt;
    led_runtime_strip_t *main_strip;
    uint8_t oem_received;
    runtime_can_event_t can_ev;
    uint16_t budget = APP_RUNTIME_CAN_EVENTS_BUDGET_PER_TICK;

    if (!ctx || !last_tick_ms || !can_protocol_started || !can_protocol_start_ms) return;

    now = HAL_GetTick();
    dt = now - *last_tick_ms;
    if (dt > 1000u) {
        dt = 16u;
    }
    *last_tick_ms = now;

    while (budget-- > 0u && runtime_event_queue_pop(&can_ev)) {
        can_ambient_process_rx(can_ev.id, can_ev.data, can_ev.len);
    }

    main_strip = board_dispatch_get_main_strip();
    oem_received = can_ambient_oem_received();
    if (oem_received && !*can_protocol_started) {
        *can_protocol_started = 1u;
        *can_protocol_start_ms = now;
    }
    if (oem_received) {
        can_ambient_update_role(now);
    }

    if (runtime_flow_dispatch_state(ctx, main_strip, dt, oem_received)) {
        return;
    }
    (void)runtime_flow_handle_active_state(ctx, main_strip, now, dt, oem_received);
}

/* Hint for main loop: safe to enter WFI when CAN queue is empty. */
uint8_t app_runtime_should_idle_wfi(void)
{
    return (uint8_t)(runtime_event_queue_size() == 0u);
}
