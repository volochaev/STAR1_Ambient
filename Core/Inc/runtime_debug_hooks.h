#pragma once

#include <stdint.h>
#include "runtime_event_queue.h"
#include "zone_roles.h"
#include "scene_preset.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize debug hooks state.
 */
void runtime_debug_hooks_init(void);
/**
 * @brief Feed debug hooks with each observed CAN RX frame.
 */
void runtime_debug_hooks_note_can_rx(uint32_t can_id, uint32_t now_ms);
/**
 * @brief Optional strip overlay (debug pulse/marker), returns 1 when active.
 */
uint8_t runtime_debug_hooks_get_strip_overlay(uint32_t now_ms,
                                              uint8_t *r,
                                              uint8_t *g,
                                              uint8_t *b);

typedef struct {
    uint32_t submitted;
    uint32_t rejected_by_role;
    uint32_t rejected_by_group;
    uint32_t alpha_clamped;
    uint32_t safety_fallbacks;
    uint32_t safety_floor_applied;
    uint8_t active_role_mask[ZONE_MAX];
    uint8_t effective_role_order[ZONE_MAX][ZONE_ROLE_MAX];
} runtime_zone_roles_diag_t;

typedef struct {
    runtime_event_queue_diag_t queue;
    runtime_zone_roles_diag_t zone_roles;
    scene_preset_t preset;
    uint8_t preset_valid;
    uint8_t world_mode;
    uint8_t world_id;
    uint8_t world_generated;
    uint8_t world_dominant_r;
    uint8_t world_dominant_g;
    uint8_t world_dominant_b;
    float world_selection_distance;
    uint8_t stop_last_wake_source;
    uint32_t stop_last_wake_ms;
    uint32_t stop_rtc_wakeup_count;
} runtime_diag_snapshot_t;

typedef enum {
    RUNTIME_DIAG_SUBMIT_OK = 0u,
    RUNTIME_DIAG_SUBMIT_REJECT_ROLE = 1u,
    RUNTIME_DIAG_SUBMIT_REJECT_GROUP = 2u
} runtime_diag_zone_submit_result_t;

void runtime_debug_hooks_runtime_diag_reset(void);
void runtime_debug_hooks_runtime_diag_get(runtime_diag_snapshot_t *out);
void runtime_debug_hooks_diag_note_zone_submit(zone_id_t zone,
                                               zone_role_id_t role,
                                               runtime_diag_zone_submit_result_t result,
                                               uint8_t alpha_clamped,
                                               uint8_t safety_fallback,
                                               uint8_t safety_floor_applied);
void runtime_debug_hooks_diag_set_zone_frame(zone_id_t zone,
                                             uint8_t active_role_mask,
                                             const uint8_t *effective_role_order,
                                             uint8_t order_len);
void runtime_debug_hooks_diag_set_preset(const scene_preset_t *preset);
void runtime_debug_hooks_diag_set_world(uint8_t active,
                                        uint8_t world_id,
                                        uint8_t generated,
                                        uint8_t dominant_r,
                                        uint8_t dominant_g,
                                        uint8_t dominant_b,
                                        float selection_distance);
void runtime_debug_hooks_set_self_check_ok(uint8_t ok);
void runtime_debug_hooks_note_stop_wake(uint8_t source, uint32_t now_ms);
void runtime_debug_hooks_note_stop_rtc_cycle(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
