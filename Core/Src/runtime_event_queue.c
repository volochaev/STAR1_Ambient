#include "runtime_event_queue.h"
#include "main.h"

#include <string.h>

#define RUNTIME_EVENT_QUEUE_CAPACITY 32u

static volatile runtime_can_event_t g_queue[RUNTIME_EVENT_QUEUE_CAPACITY];
static volatile uint16_t g_head = 0u;
static volatile uint16_t g_tail = 0u;
static volatile uint16_t g_hwm = 0u;
static volatile uint32_t g_dropped = 0u;

/* Ring-buffer next index helper. */
static uint16_t q_next(uint16_t i)
{
    return (uint16_t)((i + 1u) % RUNTIME_EVENT_QUEUE_CAPACITY);
}

/* Reset queue and diagnostics counters. */
void runtime_event_queue_init(void)
{
    __disable_irq();
    g_head = 0u;
    g_tail = 0u;
    g_hwm = 0u;
    g_dropped = 0u;
    __enable_irq();
}

/* Push CAN frame from ISR context; drops when queue is full. */
uint8_t runtime_event_queue_push_isr(uint32_t id, const uint8_t *data, uint8_t len)
{
    uint16_t next;
    runtime_can_event_t ev;

    if (len > 8u) return 0u;

    ev.id = id;
    ev.len = len;
    if (len > 0u && data) {
        memcpy(ev.data, data, len);
    }
    if (len < 8u) {
        memset(&ev.data[len], 0, (size_t)(8u - len));
    }

    next = q_next(g_head);
    if (next == g_tail) {
        g_dropped++;
        return 0u;
    }

    g_queue[g_head] = ev;
    g_head = next;
    {
        uint16_t size;
        if (g_head >= g_tail) {
            size = (uint16_t)(g_head - g_tail);
        } else {
            size = (uint16_t)(RUNTIME_EVENT_QUEUE_CAPACITY - g_tail + g_head);
        }
        if (size > g_hwm) g_hwm = size;
    }
    return 1u;
}

/* Pop one event from main loop context. */
uint8_t runtime_event_queue_pop(runtime_can_event_t *out)
{
    uint8_t ok = 0u;

    if (!out) return 0u;

    __disable_irq();
    if (g_tail != g_head) {
        *out = g_queue[g_tail];
        g_tail = q_next(g_tail);
        ok = 1u;
    }
    __enable_irq();
    return ok;
}

/* Snapshot current queue fill level. */
uint16_t runtime_event_queue_size(void)
{
    uint16_t size;
    __disable_irq();
    if (g_head >= g_tail) {
        size = (uint16_t)(g_head - g_tail);
    } else {
        size = (uint16_t)(RUNTIME_EVENT_QUEUE_CAPACITY - g_tail + g_head);
    }
    __enable_irq();
    return size;
}

/* Total number of dropped frames due to queue overflow. */
uint32_t runtime_event_queue_dropped_count(void)
{
    uint32_t c;
    __disable_irq();
    c = g_dropped;
    __enable_irq();
    return c;
}

/* Export queue diagnostics atomically. */
void runtime_event_queue_get_diag(runtime_event_queue_diag_t *out)
{
    if (!out) return;
    __disable_irq();
    if (g_head >= g_tail) {
        out->size = (uint16_t)(g_head - g_tail);
    } else {
        out->size = (uint16_t)(RUNTIME_EVENT_QUEUE_CAPACITY - g_tail + g_head);
    }
    out->high_watermark = g_hwm;
    out->dropped = g_dropped;
    __enable_irq();
}
