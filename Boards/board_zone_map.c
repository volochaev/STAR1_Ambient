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
    [ZONE_STRIP] = { .strip = &g_fl_strip, .first = FL_STRIP_FIRST_LED, .count = FL_STRIP_LEDS, .color_order = FL_STRIP_ORDER },
    [ZONE_HANDLE] = { .strip = &g_fl_handle, .first = FL_HANDLE_FIRST_LED, .count = FL_HANDLE_LEDS, .color_order = FL_HANDLE_ORDER },
    [ZONE_STORAGE] = { .strip = &g_fl_storage, .first = FL_STORAGE_FIRST_LED, .count = FL_STORAGE_LEDS, .color_order = FL_STORAGE_ORDER },
    [ZONE_FOOTWELL] = { .strip = &g_fl_footwell, .first = FL_FOOTWELL_FIRST_LED, .count = FL_FOOTWELL_LEDS, .color_order = FL_FOOTWELL_ORDER },
};
#elif (BOARD_TYPE == BOARD_TYPE_FR)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_fr_strip, .first = FR_STRIP_FIRST_LED, .count = FR_STRIP_LEDS, .color_order = FR_STRIP_ORDER },
    [ZONE_HANDLE] = { .strip = &g_fr_handle, .first = FR_HANDLE_FIRST_LED, .count = FR_HANDLE_LEDS, .color_order = FR_HANDLE_ORDER },
    [ZONE_STORAGE] = { .strip = &g_fr_storage, .first = FR_STORAGE_FIRST_LED, .count = FR_STORAGE_LEDS, .color_order = FR_STORAGE_ORDER },
    [ZONE_FOOTWELL] = { .strip = &g_fr_footwell, .first = FR_FOOTWELL_FIRST_LED, .count = FR_FOOTWELL_LEDS, .color_order = FR_FOOTWELL_ORDER },
};
#elif (BOARD_TYPE == BOARD_TYPE_RL)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_rl_strip, .first = RL_STRIP_FIRST_LED, .count = RL_STRIP_LEDS, .color_order = RL_STRIP_ORDER },
    [ZONE_HANDLE] = { .strip = &g_rl_handle, .first = RL_HANDLE_FIRST_LED, .count = RL_HANDLE_LEDS, .color_order = RL_HANDLE_ORDER },
    [ZONE_STORAGE] = { .strip = &g_rl_storage, .first = RL_STORAGE_FIRST_LED, .count = RL_STORAGE_LEDS, .color_order = RL_STORAGE_ORDER },
    [ZONE_FOOTWELL] = { .strip = &g_rl_footwell, .first = RL_FOOTWELL_FIRST_LED, .count = RL_FOOTWELL_LEDS, .color_order = RL_FOOTWELL_ORDER },
};
#elif (BOARD_TYPE == BOARD_TYPE_RR)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = { .strip = &g_rr_strip, .first = RR_STRIP_FIRST_LED, .count = RR_STRIP_LEDS, .color_order = RR_STRIP_ORDER },
    [ZONE_HANDLE] = { .strip = &g_rr_handle, .first = RR_HANDLE_FIRST_LED, .count = RR_HANDLE_LEDS, .color_order = RR_HANDLE_ORDER },
    [ZONE_STORAGE] = { .strip = &g_rr_storage, .first = RR_STORAGE_FIRST_LED, .count = RR_STORAGE_LEDS, .color_order = RR_STORAGE_ORDER },
    [ZONE_FOOTWELL] = { .strip = &g_rr_footwell, .first = RR_FOOTWELL_FIRST_LED, .count = RR_FOOTWELL_LEDS, .color_order = RR_FOOTWELL_ORDER },
};
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = {
        .strip = &g_dashboard_strip, .first = DASHBOARD_STRIP_FIRST_LED, .count = DASHBOARD_STRIP_LEDS, .color_order = DASHBOARD_STRIP_ORDER
    },
    [ZONE_HANDLE] = {
        .strip = &g_dashboard_center, .first = DASHBOARD_CENTER_FIRST_LED, .count = DASHBOARD_CENTER_LEDS, .color_order = DASHBOARD_CENTER_ORDER
    },
    [ZONE_STORAGE] = {
        .strip = &g_dashboard_ac_vents,
        .first = DASHBOARD_AC_VENTS_FIRST_LED,
        .count = DASHBOARD_AC_VENTS_LEDS,
        .color_order = DASHBOARD_AC_VENTS_ORDER
    },
    [ZONE_FOOTWELL] = {
        .strip = &g_dashboard_footwell,
        .first = DASHBOARD_FOOTWELL_FIRST_LED,
        .count = DASHBOARD_FOOTWELL_LEDS,
        .color_order = DASHBOARD_FOOTWELL_ORDER
    },
};
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
const zone_map_t g_zone_map[ZONE_MAX] = {
    [ZONE_STRIP] = {
        .strip = &g_rear_strip, .first = REAR_STRIP_FIRST_LED, .count = REAR_STRIP_LEDS, .color_order = REAR_STRIP_ORDER
    },
    [ZONE_HANDLE] = {
        .strip = (REAR_HANDLE_LEDS > 0u) ? &g_rear_handle : NULL,
        .first = REAR_HANDLE_FIRST_LED,
        .count = REAR_HANDLE_LEDS,
        .color_order = REAR_HANDLE_ORDER
    },
    [ZONE_STORAGE] = {
        .strip = (REAR_STORAGE_LEDS > 0u) ? &g_rear_storage : NULL,
        .first = REAR_STORAGE_FIRST_LED,
        .count = REAR_STORAGE_LEDS,
        .color_order = REAR_STORAGE_ORDER
    },
    [ZONE_FOOTWELL] = {
        .strip = &g_rear_footwell,
        .first = REAR_FOOTWELL_FIRST_LED,
        .count = REAR_FOOTWELL_LEDS,
        .color_order = REAR_FOOTWELL_ORDER
    },
};
#endif
