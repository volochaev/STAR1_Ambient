#pragma once

/**
 * @brief Compile-time board profile selector.
 * @details Exactly one board profile include must stay enabled.
 */
/*
 * Select exactly one board profile for the whole firmware build.
 * BOARD_TYPE must resolve identically in all translation units.
 *
 * Enable one include below, keep others commented.
 */

//#include "board_door_fl.h"
#include "board_door_fr.h"
/* #include "board_door_rl.h" */
/* #include "board_door_rr.h" */
/* #include "board_dashboard.h" */
//#include "board_rear.h"
