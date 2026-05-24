#pragma once

#include <stdint.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ZONE_ROLE_AMBIENT_BASE = 0, /* Base ambient scene contribution. */
    ZONE_ROLE_GUIDANCE_LINE,    /* Directional/guidance line overlays. */
    ZONE_ROLE_COMFORT_POOL,     /* Comfort-oriented diffuse overlays. */
    ZONE_ROLE_ATTENTION_POINT,  /* Point/spot accents. */
    ZONE_ROLE_SAFETY_ALERT,     /* Safety critical warning overlays. */
    ZONE_ROLE_MAX
} zone_role_id_t;

#define ZONE_ROLE_BIT(role_id) (1u << (uint32_t)(role_id))

typedef enum {
    ZONE_BLEND_ADD = 0u,      /* Additive blend with saturation clamp. */
    ZONE_BLEND_MAX = 1u,      /* Channel-wise max blend. */
    ZONE_BLEND_OVERRIDE = 2u  /* Replace/override existing color. */
} zone_blend_mode_t;

typedef struct {
    uint32_t role_mask;
} zone_capabilities_t;

typedef enum {
    ZONE_GROUP_MAIN_LINE = 0,  /* Main ambient light line (long strip). */
    ZONE_GROUP_INTERACTION,    /* Handle/interaction local accents. */
    ZONE_GROUP_COMFORT,        /* Comfort diffuse zones (footwell/storage). */
    ZONE_GROUP_SERVICE,        /* Utility/service zones if mapped by board. */
    ZONE_GROUP_MAX
} zone_group_id_t;

#define ZONE_GROUP_BIT(group_id) (1u << (uint32_t)(group_id))

typedef struct {
    uint32_t role_mask;
    uint32_t group_mask;
} zone_descriptor_t;

typedef struct {
    uint8_t priority;                 /* Lower value is applied earlier. */
    zone_blend_mode_t preferred_blend;
    float alpha_cap;                  /* Safety cap for submitted alpha. */
} zone_role_policy_t;

/** Return capabilities for zone (supported role bitmask). */
const zone_capabilities_t *zone_roles_get_capabilities(zone_id_t zone);
/** Return 1 if zone supports specified role. */
uint8_t zone_roles_zone_supports(zone_id_t zone, zone_role_id_t role);
/** Return descriptor (role/group masks) for zone. */
const zone_descriptor_t *zone_roles_get_descriptor(zone_id_t zone);
/** Return role application policy. */
const zone_role_policy_t *zone_roles_get_policy(zone_role_id_t role);

/** Start new role-composition frame (clear accumulators). */
void zone_roles_frame_begin(void);
/** Begin ambient-base capture phase for current frame. */
void zone_roles_base_begin(void);
/** Mark zone as ambient-base producer in current frame. */
void zone_roles_publish_ambient_base_zone(zone_id_t zone);
/** Capture current rendered frame as ambient-base reference. */
void zone_roles_capture_ambient_base_from_frame(void);
/** Submit one role contribution for zone. */
void zone_roles_submit(zone_id_t zone,
                       zone_role_id_t role,
                       uint8_t r,
                       uint8_t g,
                       uint8_t b,
                       float alpha,
                       zone_blend_mode_t blend_mode);
/** Apply composed role layers to frame buffer. */
void zone_roles_frame_apply(void);
/** Debug self-check for safety role invariants (floor + blend fallback). */
uint8_t zone_roles_debug_self_check(void);

#ifdef __cplusplus
}
#endif
