/**
 * @file runtime_can.c
 * @brief Periodic CAN TX scheduler for master/sync/discovery traffic.
 */
#include "runtime_can.h"

#include "ambient.h"
#include "ambient_backend.h"
#include "ambient_config.h"
#include "main.h"

#define AMB_MODERN_CONFLICT_BACKOFF_MS 400u

/* Periodic CAN transmit scheduling with startup and conflict backoff rules. */
void runtime_can_tx_scheduler_tick(uint32_t now,
                                   uint8_t oem_received,
                                   uint8_t can_protocol_started,
                                   uint32_t can_protocol_start_ms,
                                   uint8_t sleep_fade_active)
{
    static uint32_t last_master_send_ms = 0u;
    static uint32_t last_sync_send_ms = 0u;
    static uint32_t last_discovery_send_ms = 0u;
    static uint32_t modern_conflict_backoff_until_ms = 0u;
    uint8_t allow_can_tx = oem_received;
    uint8_t startup_discovery_only = 0u;

#if AMB_ENABLE_SLEEP_MODE
    if (sleep_fade_active || can_ambient_should_sleep() || can_ambient_is_sleeping()) {
        allow_can_tx = 0u;
    }
    if (allow_can_tx && (can_ambient_get_idle_time_ms() >= AMB_CAN_ACTIVE_TIMEOUT_MS)) {
        allow_can_tx = 0u;
    }
#endif

    if (allow_can_tx && can_protocol_started) {
        uint32_t elapsed;
        if (now >= can_protocol_start_ms) {
            elapsed = now - can_protocol_start_ms;
        } else {
            elapsed = (UINT32_MAX - can_protocol_start_ms) + now + 1u;
        }
        startup_discovery_only = (elapsed < AMB_CAN_STARTUP_DISCOVERY_MS) ? 1u : 0u;
    }

    if (allow_can_tx) {
        if (can_ambient_is_master() && !startup_discovery_only &&
            (now - last_master_send_ms) >= AMB_CAN_MASTER_TX_INTERVAL_MS) {
            if (ambient_backend_get_kind() == AMBIENT_BACKEND_MODERN) {
                can_modern_diag_t modern_diag;
                can_ambient_get_modern_diag(&modern_diag);
                if (modern_diag.conflict_active) {
                    modern_conflict_backoff_until_ms = now + AMB_MODERN_CONFLICT_BACKOFF_MS;
                }
                if (now >= modern_conflict_backoff_until_ms) {
                    can_ambient_send_modern_status_packet();
                }
            } else {
                can_ambient_send_master_packet();
            }
            last_master_send_ms = now;
        }
        if (can_ambient_is_master() && !startup_discovery_only &&
            (now - last_sync_send_ms) >= AMB_CAN_SYNC_INTERVAL_MS) {
            can_ambient_send_sync_packet();
            last_sync_send_ms = now;
        }
        if ((now - last_discovery_send_ms) >= AMB_CAN_DISCOVERY_INTERVAL_MS) {
            can_ambient_send_discovery_packet();
            last_discovery_send_ms = now;
        }
    }
}
