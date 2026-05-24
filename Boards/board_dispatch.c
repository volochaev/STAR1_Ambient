/**
 * @file board_dispatch.c
 * @brief Compile-time board dispatcher for board-specific entry points.
 */
#include "board_dispatch.h"

#include "board_dashboard.h"
#include "board_door_fl.h"
#include "board_door_fr.h"
#include "board_door_rl.h"
#include "board_door_rr.h"
#include "board_rear.h"

/* Return main strip object for active board profile. */
led_runtime_strip_t *board_dispatch_get_main_strip(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL)
    return &g_fl_strip;
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    return &g_fr_strip;
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    return &g_rl_strip;
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    return &g_rr_strip;
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    return &g_dashboard_strip;
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    return &g_rear_strip;
#else
    return NULL;
#endif
}

/* Dispatch board LED initialization to active board profile. */
void board_dispatch_led_init(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL)
    board_fl_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    board_fr_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    board_rl_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    board_rr_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    board_dashboard_led_init();
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    board_rear_led_init();
#endif
}

/* Dispatch per-frame render call to active board profile. */
void board_dispatch_led_render_all(void)
{
#if (BOARD_TYPE == BOARD_TYPE_FL)
    board_fl_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    board_fr_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    board_rl_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    board_rr_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    board_dashboard_led_render_all();
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    board_rear_led_render_all();
#endif
}

/* Dispatch DMA transfer-complete callback to active board profile. */
void board_dispatch_dma_tc(TIM_HandleTypeDef *htim)
{
#if (BOARD_TYPE == BOARD_TYPE_FL)
    board_fl_dma_tc(htim);
#elif (BOARD_TYPE == BOARD_TYPE_FR)
    board_fr_dma_tc(htim);
#elif (BOARD_TYPE == BOARD_TYPE_RL)
    board_rl_dma_tc(htim);
#elif (BOARD_TYPE == BOARD_TYPE_RR)
    board_rr_dma_tc(htim);
#elif (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    board_dashboard_dma_tc(htim);
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    board_rear_dma_tc(htim);
#else
    (void)htim;
#endif
}
