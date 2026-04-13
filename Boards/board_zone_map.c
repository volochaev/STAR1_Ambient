#include "ambient.h"
#include "board_dashboard.h"
#include "board_door_fl.h"
#include "board_door_fr.h"
#include "board_door_rl.h"
#include "board_door_rr.h"
#include "board_rear.h"
#include "zones.h"

/* Strong zone map selected by current BOARD_TYPE.
 * Single source of truth for zones binding in the firmware build. */
#if (BOARD_TYPE == BOARD_TYPE_FL)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_fl_strip, .first = 0, .count = FL_STRIP_LEDS },
    [ZONE_HANDLE] = { .strip = &g_fl_handle, .first = 0, .count = FL_HANDLE_LEDS },
    [ZONE_STORAGE] = { .strip = &g_fl_storage, .first = 0, .count = FL_STORAGE_LEDS },
    [ZONE_FOOTWELL] = { .strip = &g_fl_footwell, .first = 0, .count = FL_FOOTWELL_LEDS },
};
#elif (BOARD_TYPE == BOARD_TYPE_FR)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_fr_strip, .first = 0, .count = FR_STRIP_LEDS },
    [ZONE_HANDLE] = { .strip = &g_fr_handle, .first = 0, .count = FR_HANDLE_LEDS },
    [ZONE_STORAGE] = { .strip = &g_fr_storage, .first = 0, .count = FR_STORAGE_LEDS },
    [ZONE_FOOTWELL] = { .strip = &g_fr_footwell, .first = 0, .count = FR_FOOTWELL_LEDS },
};
#elif (BOARD_TYPE == BOARD_TYPE_RL)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_rl_strip, .first = 0, .count = RL_STRIP_LEDS },
    [ZONE_HANDLE] = { .strip = &g_rl_handle, .first = 0, .count = RL_HANDLE_LEDS },
    [ZONE_STORAGE] = { .strip = &g_rl_storage, .first = 0, .count = RL_STORAGE_LEDS },
    [ZONE_FOOTWELL] = { .strip = &g_rl_footwell, .first = 0, .count = RL_FOOTWELL_LEDS },
};
#elif (BOARD_TYPE == BOARD_TYPE_RR)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_rr_strip, .first = 0, .count = RR_STRIP_LEDS },
    [ZONE_HANDLE] = { .strip = &g_rr_handle, .first = 0, .count = RR_HANDLE_LEDS },
    [ZONE_STORAGE] = { .strip = &g_rr_storage, .first = 0, .count = RR_STORAGE_LEDS },
    [ZONE_FOOTWELL] = { .strip = &g_rr_footwell, .first = 0, .count = RR_FOOTWELL_LEDS },
};
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_dashboard_strip, .first = 0, .count = DASHBOARD_STRIP_LEDS },
    [ZONE_HANDLE] = { .strip = &g_dashboard_center, .first = 0, .count = DASHBOARD_CENTER_LEDS },
    [ZONE_STORAGE] = { .strip = &g_dashboard_ac_vents, .first = 0, .count = DASHBOARD_AC_VENTS_LEDS },
    [ZONE_FOOTWELL] = { .strip = &g_dashboard_footwell, .first = 0, .count = DASHBOARD_FOOTWELL_LEDS },
};
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_rear_strip, .first = 0, .count = REAR_STRIP_LEDS },
    [ZONE_HANDLE] = {
        .strip = (REAR_HANDLE_LEDS > 0u) ? &g_rear_handle : NULL, .first = 0, .count = REAR_HANDLE_LEDS
    },
    [ZONE_STORAGE] = {
        .strip = (REAR_STORAGE_LEDS > 0u) ? &g_rear_storage : NULL, .first = 0, .count = REAR_STORAGE_LEDS
    },
    [ZONE_FOOTWELL] = { .strip = &g_rear_footwell, .first = 0, .count = REAR_FOOTWELL_LEDS },
};
#endif
