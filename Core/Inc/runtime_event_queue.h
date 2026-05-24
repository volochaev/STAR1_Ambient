#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
} runtime_can_event_t;

/** Queue diagnostics snapshot for RX event buffering. */
typedef struct {
    uint16_t size;
    uint16_t high_watermark;
    uint32_t dropped;
} runtime_event_queue_diag_t;

/** Reset queue storage and counters. */
void runtime_event_queue_init(void);
/** Push CAN event from ISR context; returns 1 on success, 0 if dropped/full. */
uint8_t runtime_event_queue_push_isr(uint32_t id, const uint8_t *data, uint8_t len);
/** Pop one pending CAN event from task context; returns 1 if event returned. */
uint8_t runtime_event_queue_pop(runtime_can_event_t *out);
/** Return current queue size. */
uint16_t runtime_event_queue_size(void);
/** Return total dropped event counter. */
uint32_t runtime_event_queue_dropped_count(void);
/** Return queue diagnostics snapshot. */
void runtime_event_queue_get_diag(runtime_event_queue_diag_t *out);

#ifdef __cplusplus
}
#endif
