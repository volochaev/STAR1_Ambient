#pragma once

/*
 * Select exactly one board profile for the whole firmware build.
 * This header is included from ambient.h, so BOARD_TYPE is consistent
 * across all translation units (Core/CAN/Lightning/Boards).
 */

/* #include "board_door_fl.h" */
/* #include "board_door_fr.h" */
/* #include "board_door_rl.h" */
/* #include "board_door_rr.h" */
/* #include "board_dashboard.h" */
#include "board_rear.h"
