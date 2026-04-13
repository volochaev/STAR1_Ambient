#include "ambient_role.h"

#include <stdint.h>

#include "ambient.h"

#define BOARD_COUNT 6u
#define DISCOVERY_STALE_MS 5000u

static volatile int8_t g_role = -1;
static volatile uint32_t g_last_master_heartbeat_ms = 0u;
static volatile uint8_t g_discovered_boards[BOARD_COUNT] = {0, 0, 0, 0, 0, 0};
static volatile uint32_t g_discovery_last_seen_ms[BOARD_COUNT] = {0, 0, 0, 0, 0, 0};

static uint8_t board_priority(uint8_t board_type)
{
    switch (board_type) {
    case BOARD_TYPE_FL:        return 0u;
    case BOARD_TYPE_FR:        return 1u;
    case BOARD_TYPE_DASHBOARD: return 2u;
    case BOARD_TYPE_RL:        return 3u;
    case BOARD_TYPE_RR:        return 4u;
    case BOARD_TYPE_REAR:      return 5u;
    default:                   return 0xFFu;
    }
}

static uint32_t elapsed_ms32(uint32_t now, uint32_t then)
{
    return (now >= then) ? (now - then) : ((UINT32_MAX - then) + now + 1u);
}

static int8_t role_get_atomic(void)
{
    int8_t role;
    __disable_irq();
    role = g_role;
    __enable_irq();
    return role;
}

static void role_set_atomic(int8_t role)
{
    __disable_irq();
    g_role = role;
    __enable_irq();
}

static uint32_t master_heartbeat_get_atomic(void)
{
    uint32_t hb;
    __disable_irq();
    hb = g_last_master_heartbeat_ms;
    __enable_irq();
    return hb;
}

static void master_heartbeat_set_atomic(uint32_t ts_ms)
{
    __disable_irq();
    g_last_master_heartbeat_ms = ts_ms;
    __enable_irq();
}

static void copy_discovered_boards_atomic(uint8_t out[BOARD_COUNT])
{
    uint8_t i;

    __disable_irq();
    for (i = 0u; i < BOARD_COUNT; ++i) {
        out[i] = g_discovered_boards[i];
    }
    __enable_irq();
}

static uint8_t has_higher_priority_board(const uint8_t discovered[BOARD_COUNT])
{
    uint8_t this_p = board_priority((uint8_t)BOARD_TYPE);
    uint8_t i;

    if (this_p == 0xFFu || !discovered) return 0u;

    for (i = 0u; i < BOARD_COUNT; ++i) {
        if (!discovered[i]) continue;
        if (board_priority(i) < this_p) {
            return 1u;
        }
    }
    return 0u;
}

static uint8_t should_be_master_from_discovery(const uint8_t discovered[BOARD_COUNT])
{
    return (uint8_t)(has_higher_priority_board(discovered) ? 0u : 1u);
}

static void cleanup_discovery_stale(uint32_t now_ms)
{
    uint8_t i;

    __disable_irq();
    for (i = 0u; i < BOARD_COUNT; ++i) {
        uint32_t last_seen = g_discovery_last_seen_ms[i];
        if (last_seen == 0u) {
            continue;
        }
        if (elapsed_ms32(now_ms, last_seen) > DISCOVERY_STALE_MS) {
            g_discovered_boards[i] = 0u;
            g_discovery_last_seen_ms[i] = 0u;
        }
    }
    __enable_irq();
}

void can_role_init(void)
{
    uint8_t i;

    role_set_atomic(-1);
    master_heartbeat_set_atomic(0u);

    __disable_irq();
    for (i = 0u; i < BOARD_COUNT; ++i) {
        g_discovered_boards[i] = 0u;
        g_discovery_last_seen_ms[i] = 0u;
    }
    __enable_irq();
}

void can_role_reset(void)
{
    can_role_init();
}

int8_t can_role_get(void)
{
    return role_get_atomic();
}

uint8_t can_role_is_master(void)
{
    if (AMB_ROLE_MASTER >= 0) {
        return (uint8_t)AMB_ROLE_MASTER;
    }
    return (role_get_atomic() == 1) ? 1u : 0u;
}

void can_role_handle_discovery(uint8_t board_type, uint32_t now_ms)
{
    if (board_type >= BOARD_COUNT) return;

    __disable_irq();
    g_discovered_boards[board_type] = 1u;
    g_discovery_last_seen_ms[board_type] = now_ms;
    __enable_irq();
}

void can_role_note_master_heartbeat(uint32_t now_ms)
{
    master_heartbeat_set_atomic(now_ms);
}

void can_role_note_sync_from_master(uint32_t now_ms)
{
    uint8_t discovered[BOARD_COUNT];

    master_heartbeat_set_atomic(now_ms);
    if (role_get_atomic() != 1 || AMB_ROLE_MASTER >= 0) {
        return;
    }

    copy_discovered_boards_atomic(discovered);
    if (has_higher_priority_board(discovered)) {
        role_set_atomic(0);
    }
}

void can_role_update(uint32_t now_ms)
{
    uint8_t discovered_boards[BOARD_COUNT];
    uint8_t should_be_master;
    int8_t role_now;
    static uint32_t last_discovery_cleanup_ms = 0u;

    if (AMB_ROLE_MASTER >= 0) {
        role_set_atomic((int8_t)AMB_ROLE_MASTER);
        return;
    }

    copy_discovered_boards_atomic(discovered_boards);
    should_be_master = should_be_master_from_discovery(discovered_boards);
    role_now = role_get_atomic();

    if (role_now == 0 && should_be_master) {
        uint32_t last_heartbeat = master_heartbeat_get_atomic();
        uint32_t timeout = (last_heartbeat == 0u) ? UINT32_MAX : elapsed_ms32(now_ms, last_heartbeat);

        if (timeout > MASTER_HEARTBEAT_TIMEOUT_MS) {
            role_set_atomic(1);
            master_heartbeat_set_atomic(now_ms);
        }
    } else if (role_now == 1 && !should_be_master) {
        role_set_atomic(0);
    } else if (role_now == -1) {
        role_set_atomic((int8_t)(should_be_master ? 1 : 0));
    }

    if (elapsed_ms32(now_ms, last_discovery_cleanup_ms) >= DISCOVERY_STALE_MS) {
        cleanup_discovery_stale(now_ms);
        last_discovery_cleanup_ms = now_ms;
    }
}

uint32_t can_role_get_last_master_heartbeat_ms(void)
{
    return master_heartbeat_get_atomic();
}
