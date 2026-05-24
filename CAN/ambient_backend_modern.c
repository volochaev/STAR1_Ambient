/**
 * @file ambient_backend_modern.c
 * @brief Modern backend adapter (HU request -> unified state store).
 */
#include "ambient_backend_modern.h"

#include "ambient_rx.h"
#include "ambient_state_store.h"

/* Consume latest HU request frame and update unified ambient snapshot. */
void ambient_backend_modern_on_can_state(const can_state_t *can_state, uint32_t now_ms)
{
    uint8_t color_rq = 0u;
    uint8_t brightness_rq = 0u;
    uint8_t effect_rq = 0u;
    uint32_t rq_ts = 0u;

    (void)can_state;

    if (can_rx_get_modern_request(&color_rq, &brightness_rq, &effect_rq, &rq_ts) &&
        rq_ts != 0u && rq_ts <= now_ms) {
        ambient_state_store_update_from_modern_request(color_rq, brightness_rq, effect_rq, now_ms);
        return;
    }
}
