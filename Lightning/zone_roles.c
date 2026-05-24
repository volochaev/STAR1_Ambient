#include "zone_roles.h"

#include "ambient_config.h"
#include "runtime_debug_hooks.h"
#include "zones.h"

#define ZONE_SAFETY_ALPHA_FLOOR 0.32f

_Static_assert(ZONE_ROLE_MAX <= 8, "runtime diag role mask stores up to 8 roles");
_Static_assert(ZONE_SAFETY_ALPHA_FLOOR > 0.0f && ZONE_SAFETY_ALPHA_FLOOR <= 1.0f, "invalid safety floor");

typedef struct {
    uint8_t active;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    float alpha;
    zone_blend_mode_t blend_mode;
} zone_role_layer_t;

typedef struct {
    uint32_t allowed_role_mask;
    zone_blend_mode_t preferred_blend;
    float alpha_cap;
    int8_t priority_offset;
} zone_group_policy_t;

static const zone_descriptor_t g_zone_descriptors[ZONE_MAX] = {
    [ZONE_STRIP] = {
        .role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                     ZONE_ROLE_BIT(ZONE_ROLE_GUIDANCE_LINE) |
                     ZONE_ROLE_BIT(ZONE_ROLE_COMFORT_POOL) |
                     ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT) |
                     ZONE_ROLE_BIT(ZONE_ROLE_SAFETY_ALERT),
        .group_mask = ZONE_GROUP_BIT(ZONE_GROUP_MAIN_LINE),
    },
    [ZONE_HANDLE] = {
        .role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                     ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT) |
                     ZONE_ROLE_BIT(ZONE_ROLE_SAFETY_ALERT),
        .group_mask = ZONE_GROUP_BIT(ZONE_GROUP_INTERACTION),
    },
    [ZONE_STORAGE] = {
        .role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                     ZONE_ROLE_BIT(ZONE_ROLE_COMFORT_POOL) |
                     ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT),
        .group_mask = ZONE_GROUP_BIT(ZONE_GROUP_COMFORT) |
                      ZONE_GROUP_BIT(ZONE_GROUP_SERVICE),
    },
    [ZONE_FOOTWELL] = {
        .role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                     ZONE_ROLE_BIT(ZONE_ROLE_COMFORT_POOL) |
                     ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT),
        .group_mask = ZONE_GROUP_BIT(ZONE_GROUP_COMFORT),
    },
};

static const zone_role_policy_t g_role_policy[ZONE_ROLE_MAX] = {
    [ZONE_ROLE_AMBIENT_BASE] = {
        .priority = 10u,
        .preferred_blend = ZONE_BLEND_ADD,
        .alpha_cap = 1.0f,
    },
    [ZONE_ROLE_COMFORT_POOL] = {
        .priority = 20u,
        .preferred_blend = ZONE_BLEND_MAX,
        .alpha_cap = 0.85f,
    },
    [ZONE_ROLE_GUIDANCE_LINE] = {
        .priority = 30u,
        .preferred_blend = ZONE_BLEND_MAX,
        .alpha_cap = 0.90f,
    },
    [ZONE_ROLE_ATTENTION_POINT] = {
        .priority = 40u,
        .preferred_blend = ZONE_BLEND_MAX,
        .alpha_cap = 1.0f,
    },
    [ZONE_ROLE_SAFETY_ALERT] = {
        .priority = 60u,
        .preferred_blend = ZONE_BLEND_OVERRIDE,
        .alpha_cap = 1.0f,
    },
};

static const zone_group_policy_t g_group_policy[ZONE_GROUP_MAX] = {
    [ZONE_GROUP_MAIN_LINE] = {
        .allowed_role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                             ZONE_ROLE_BIT(ZONE_ROLE_GUIDANCE_LINE) |
                             ZONE_ROLE_BIT(ZONE_ROLE_COMFORT_POOL) |
                             ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT) |
                             ZONE_ROLE_BIT(ZONE_ROLE_SAFETY_ALERT),
        .preferred_blend = ZONE_BLEND_MAX,
        .alpha_cap = 1.0f,
        .priority_offset = 0,
    },
    [ZONE_GROUP_INTERACTION] = {
        .allowed_role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                             ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT) |
                             ZONE_ROLE_BIT(ZONE_ROLE_SAFETY_ALERT),
        .preferred_blend = ZONE_BLEND_MAX,
        .alpha_cap = 1.0f,
        .priority_offset = 4,
    },
    [ZONE_GROUP_COMFORT] = {
        .allowed_role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                             ZONE_ROLE_BIT(ZONE_ROLE_COMFORT_POOL) |
                             ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT),
        .preferred_blend = ZONE_BLEND_MAX,
        .alpha_cap = 0.90f,
        .priority_offset = -2,
    },
    [ZONE_GROUP_SERVICE] = {
        .allowed_role_mask = ZONE_ROLE_BIT(ZONE_ROLE_AMBIENT_BASE) |
                             ZONE_ROLE_BIT(ZONE_ROLE_GUIDANCE_LINE) |
                             ZONE_ROLE_BIT(ZONE_ROLE_ATTENTION_POINT) |
                             ZONE_ROLE_BIT(ZONE_ROLE_SAFETY_ALERT),
        .preferred_blend = ZONE_BLEND_OVERRIDE,
        .alpha_cap = 0.82f,
        .priority_offset = 2,
    },
};
_Static_assert(ZONE_GROUP_MAX == 4, "update group policy table");

static zone_role_layer_t g_layers[ZONE_MAX][ZONE_ROLE_MAX];
static uint8_t g_base_rgb[ZONE_MAX][AMB_MAX_CROSSFADE_LEDS * 3u];
static uint16_t g_base_count[ZONE_MAX];
static uint8_t g_base_valid[ZONE_MAX];

static uint32_t zone_roles_board_role_mask(zone_id_t zone, uint32_t default_mask)
{
#if (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    if (zone == ZONE_STORAGE) {
        /* Dashboard "storage" is used as AC vents plane; keep guidance + alerts enabled. */
        return default_mask | ZONE_ROLE_BIT(ZONE_ROLE_GUIDANCE_LINE) | ZONE_ROLE_BIT(ZONE_ROLE_SAFETY_ALERT);
    }
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    if (zone == ZONE_FOOTWELL) {
        /* Rear footwell should not carry safety overlays in current product design. */
        return default_mask & (~ZONE_ROLE_BIT(ZONE_ROLE_SAFETY_ALERT));
    }
#endif
    return default_mask;
}

static uint32_t zone_roles_board_group_mask(zone_id_t zone, uint32_t default_mask)
{
#if (BOARD_TYPE == BOARD_TYPE_DASHBOARD)
    if (zone == ZONE_STORAGE) {
        /* Dashboard storage zone acts as vent-light service lane. */
        return default_mask | ZONE_GROUP_BIT(ZONE_GROUP_SERVICE);
    }
#elif (BOARD_TYPE == BOARD_TYPE_REAR)
    if (zone == ZONE_HANDLE && g_zone_map[zone].count == 0u) {
        /* Rear builds without handle LEDs should not expose interaction group. */
        return default_mask & (~ZONE_GROUP_BIT(ZONE_GROUP_INTERACTION));
    }
#endif
    return default_mask;
}

static float zone_clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static uint8_t zone_mul_u8(uint8_t value, float alpha)
{
    float v = (float)value * zone_clamp01(alpha);
    if (v <= 0.0f) return 0u;
    if (v >= 255.0f) return 255u;
    return (uint8_t)(v + 0.5f);
}

static zone_group_id_t zone_roles_primary_group(zone_id_t zone)
{
    const zone_descriptor_t *desc = zone_roles_get_descriptor(zone);
    uint32_t gm;
    if (!desc) return ZONE_GROUP_MAIN_LINE;
    gm = desc->group_mask;
    if (gm & ZONE_GROUP_BIT(ZONE_GROUP_MAIN_LINE)) return ZONE_GROUP_MAIN_LINE;
    if (gm & ZONE_GROUP_BIT(ZONE_GROUP_INTERACTION)) return ZONE_GROUP_INTERACTION;
    if (gm & ZONE_GROUP_BIT(ZONE_GROUP_SERVICE)) return ZONE_GROUP_SERVICE;
    if (gm & ZONE_GROUP_BIT(ZONE_GROUP_COMFORT)) return ZONE_GROUP_COMFORT;
    return ZONE_GROUP_MAIN_LINE;
}

static const zone_group_policy_t *zone_roles_group_policy(zone_id_t zone)
{
    zone_group_id_t gid = zone_roles_primary_group(zone);
    if (gid >= ZONE_GROUP_MAX) return NULL;
    return &g_group_policy[gid];
}

const zone_capabilities_t *zone_roles_get_capabilities(zone_id_t zone)
{
    static zone_capabilities_t resolved;
    if (zone >= ZONE_MAX) return NULL;
    resolved.role_mask = g_zone_descriptors[zone].role_mask;
    resolved.role_mask = zone_roles_board_role_mask(zone, resolved.role_mask);
    return &resolved;
}

const zone_descriptor_t *zone_roles_get_descriptor(zone_id_t zone)
{
    static zone_descriptor_t resolved;
    if (zone >= ZONE_MAX) return NULL;
    resolved = g_zone_descriptors[zone];
    resolved.role_mask = zone_roles_board_role_mask(zone, resolved.role_mask);
    resolved.group_mask = zone_roles_board_group_mask(zone, resolved.group_mask);
    return &resolved;
}

const zone_role_policy_t *zone_roles_get_policy(zone_role_id_t role)
{
    if (role >= ZONE_ROLE_MAX) return NULL;
    return &g_role_policy[role];
}

uint8_t zone_roles_zone_supports(zone_id_t zone, zone_role_id_t role)
{
    const zone_capabilities_t *caps;
    if (role >= ZONE_ROLE_MAX) return 0u;
    caps = zone_roles_get_capabilities(zone);
    if (!caps) return 0u;
    return (uint8_t)(((caps->role_mask & ZONE_ROLE_BIT(role)) != 0u) ? 1u : 0u);
}

void zone_roles_frame_begin(void)
{
    uint8_t z;
    uint8_t r;

    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        for (r = 0u; r < (uint8_t)ZONE_ROLE_MAX; ++r) {
            g_layers[z][r].active = 0u;
            g_layers[z][r].r = 0u;
            g_layers[z][r].g = 0u;
            g_layers[z][r].b = 0u;
            g_layers[z][r].alpha = 0.0f;
            g_layers[z][r].blend_mode = ZONE_BLEND_ADD;
        }
    }
}

void zone_roles_base_begin(void)
{
    uint8_t z;
    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        g_base_count[z] = 0u;
        g_base_valid[z] = 0u;
    }
}

void zone_roles_publish_ambient_base_zone(zone_id_t zone)
{
    const zone_map_t *zm;
    uint16_t cap;
    uint16_t i;

    if (zone >= ZONE_MAX) return;
    zm = &g_zone_map[zone];
    if (!zm || !zm->strip || zm->count == 0u) {
        g_base_count[zone] = 0u;
        g_base_valid[zone] = 0u;
        return;
    }

    cap = (zm->count > (uint16_t)AMB_MAX_CROSSFADE_LEDS) ? (uint16_t)AMB_MAX_CROSSFADE_LEDS : zm->count;
    g_base_count[zone] = cap;
    g_base_valid[zone] = 1u;
    for (i = 0u; i < cap; ++i) {
        uint8_t r, g, b;
        uint16_t led_idx = (uint16_t)(zm->first + i);
        uint16_t p = (uint16_t)(i * 3u);
        led_runtime_get_pixel_rgb(zm->strip, led_idx, &r, &g, &b);
        g_base_rgb[zone][p + 0u] = r;
        g_base_rgb[zone][p + 1u] = g;
        g_base_rgb[zone][p + 2u] = b;
    }
}

void zone_roles_capture_ambient_base_from_frame(void)
{
    uint8_t z;

    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        zone_roles_publish_ambient_base_zone((zone_id_t)z);
    }
}

void zone_roles_submit(zone_id_t zone,
                       zone_role_id_t role,
                       uint8_t r,
                       uint8_t g,
                       uint8_t b,
                       float alpha,
                       zone_blend_mode_t blend_mode)
{
    zone_role_layer_t *slot;
    const zone_role_policy_t *policy;
    const zone_group_policy_t *group_policy;
    uint8_t alpha_clamped = 0u;
    uint8_t safety_fallback = 0u;
    uint8_t safety_floor_applied = 0u;

    if (zone >= ZONE_MAX || role >= ZONE_ROLE_MAX) return;
    if (!zone_roles_zone_supports(zone, role)) {
        runtime_debug_hooks_diag_note_zone_submit(zone, role, RUNTIME_DIAG_SUBMIT_REJECT_ROLE, 0u, 0u, 0u);
        return;
    }
    group_policy = zone_roles_group_policy(zone);
    if (!group_policy) {
        runtime_debug_hooks_diag_note_zone_submit(zone, role, RUNTIME_DIAG_SUBMIT_REJECT_GROUP, 0u, 0u, 0u);
        return;
    }
    if ((group_policy->allowed_role_mask & ZONE_ROLE_BIT(role)) == 0u) {
        runtime_debug_hooks_diag_note_zone_submit(zone, role, RUNTIME_DIAG_SUBMIT_REJECT_GROUP, 0u, 0u, 0u);
        return;
    }
    policy = zone_roles_get_policy(role);
    if (!policy) return;
    alpha = zone_clamp01(alpha);
    if (alpha > policy->alpha_cap) {
        alpha = policy->alpha_cap;
        alpha_clamped = 1u;
    }
    if (alpha > group_policy->alpha_cap) {
        alpha = group_policy->alpha_cap;
        alpha_clamped = 1u;
    }
    if (role == ZONE_ROLE_SAFETY_ALERT && alpha < ZONE_SAFETY_ALPHA_FLOOR) {
        alpha = ZONE_SAFETY_ALPHA_FLOOR;
        safety_floor_applied = 1u;
    }
    if (alpha <= 0.0f) return;

    slot = &g_layers[zone][role];
    if (!slot->active || alpha >= slot->alpha) {
        zone_blend_mode_t allowed_blend = blend_mode;
        if (!(allowed_blend == ZONE_BLEND_OVERRIDE || allowed_blend == ZONE_BLEND_MAX || allowed_blend == ZONE_BLEND_ADD)) {
            allowed_blend = policy->preferred_blend;
        }
        if (group_policy->preferred_blend == ZONE_BLEND_OVERRIDE && role != ZONE_ROLE_SAFETY_ALERT) {
            allowed_blend = ZONE_BLEND_MAX;
        }
        if (role == ZONE_ROLE_SAFETY_ALERT && allowed_blend != ZONE_BLEND_OVERRIDE) {
            allowed_blend = ZONE_BLEND_OVERRIDE;
            safety_fallback = 1u;
        }
        slot->active = 1u;
        slot->r = r;
        slot->g = g;
        slot->b = b;
        slot->alpha = alpha;
        slot->blend_mode = allowed_blend;
    }
    runtime_debug_hooks_diag_note_zone_submit(zone, role, RUNTIME_DIAG_SUBMIT_OK, alpha_clamped, safety_fallback, safety_floor_applied);
}

void zone_roles_frame_apply(void)
{
    uint8_t z;
    uint8_t role;

    for (z = 0u; z < (uint8_t)ZONE_MAX; ++z) {
        const zone_map_t *zm = &g_zone_map[z];
        const zone_group_policy_t *gp = zone_roles_group_policy((zone_id_t)z);
        uint8_t role_order[ZONE_ROLE_MAX];
        uint8_t role_count = 0u;
        uint16_t i;

        if (!zm || !zm->strip || zm->count == 0u) continue;
        for (role = 0u; role < (uint8_t)ZONE_ROLE_MAX; ++role) {
            uint8_t ins = role_count;
            const zone_role_policy_t *pol = zone_roles_get_policy((zone_role_id_t)role);
            int16_t prio = pol ? (int16_t)pol->priority : 255;
            int16_t offs = gp ? (int16_t)gp->priority_offset : 0;
            int16_t ep = prio + offs;
            while (ins > 0u) {
                const zone_role_policy_t *pp = zone_roles_get_policy((zone_role_id_t)role_order[ins - 1u]);
                int16_t pprev = pp ? (int16_t)pp->priority : 255;
                int16_t eprev = pprev + offs;
                if (eprev <= ep) break;
                role_order[ins] = role_order[ins - 1u];
                --ins;
            }
            role_order[ins] = role;
            ++role_count;
        }

        for (i = 0u; i < zm->count; ++i) {
            uint16_t led_idx = (uint16_t)(zm->first + i);
            uint8_t out_r, out_g, out_b;
            uint8_t active_role_mask = 0u;
            if (g_base_valid[z] && i < g_base_count[z]) {
                uint16_t p = (uint16_t)(i * 3u);
                out_r = g_base_rgb[z][p + 0u];
                out_g = g_base_rgb[z][p + 1u];
                out_b = g_base_rgb[z][p + 2u];
            } else {
                led_runtime_get_pixel_rgb(zm->strip, led_idx, &out_r, &out_g, &out_b);
            }

            for (role = 0u; role < role_count; ++role) {
                const zone_role_layer_t *layer = &g_layers[z][role_order[role]];
                uint8_t lr, lg, lb;
                if (!layer->active) continue;
                active_role_mask |= (uint8_t)ZONE_ROLE_BIT(role_order[role]);

                lr = zone_mul_u8(layer->r, layer->alpha);
                lg = zone_mul_u8(layer->g, layer->alpha);
                lb = zone_mul_u8(layer->b, layer->alpha);

                if (layer->blend_mode == ZONE_BLEND_OVERRIDE) {
                    out_r = lr;
                    out_g = lg;
                    out_b = lb;
                } else if (layer->blend_mode == ZONE_BLEND_MAX) {
                    if (lr > out_r) out_r = lr;
                    if (lg > out_g) out_g = lg;
                    if (lb > out_b) out_b = lb;
                } else {
                    uint16_t rr = (uint16_t)out_r + (uint16_t)lr;
                    uint16_t gg = (uint16_t)out_g + (uint16_t)lg;
                    uint16_t bb = (uint16_t)out_b + (uint16_t)lb;
                    out_r = (rr > 255u) ? 255u : (uint8_t)rr;
                    out_g = (gg > 255u) ? 255u : (uint8_t)gg;
                    out_b = (bb > 255u) ? 255u : (uint8_t)bb;
                }
            }

            led_runtime_set_pixel_rgb(zm->strip, led_idx, out_r, out_g, out_b);
            if (i == 0u) {
                runtime_debug_hooks_diag_set_zone_frame((zone_id_t)z, active_role_mask, role_order, role_count);
            }
        }
    }
}

uint8_t zone_roles_debug_self_check(void)
{
#if AMB_DEBUG_BSM_RX_PULSE
    zone_role_layer_t *slot;
    zone_roles_frame_begin();
    zone_roles_submit(ZONE_STRIP,
                      ZONE_ROLE_SAFETY_ALERT,
                      255u, 0u, 0u,
                      0.01f,            /* intentionally below safety floor */
                      ZONE_BLEND_ADD);  /* intentionally weak blend to trigger fallback */
    slot = &g_layers[ZONE_STRIP][ZONE_ROLE_SAFETY_ALERT];
    if (!slot->active) return 0u;
    if (slot->alpha < ZONE_SAFETY_ALPHA_FLOOR) return 0u;
    if (slot->blend_mode != ZONE_BLEND_OVERRIDE) return 0u;
    zone_roles_frame_begin();
    return 1u;
#else
    return 1u;
#endif
}
