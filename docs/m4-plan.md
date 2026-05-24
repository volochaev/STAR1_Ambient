# M4 Plan (Incremental Start)

Date: 2026-05-24

## Goal

Build a premium ambient composition layer on top of M3 baseline:
- semantic color entities,
- multi-role zone composition,
- richer multi-zone behavior profile (W222/W223-inspired),
- no regression for existing modern CAN path.

## Principles

1. Keep M3 runtime stable at each step (buildable after every change).
2. Introduce new APIs in parallel first, then migrate call sites.
3. Preserve `0x325` compatibility via transform into modern internal model.

## Phase Outline

## P0 (done in this commit)

- Added semantic color entity type:
  - `scene_color_entity_t`
  - `scene_color_entity_from_model(...)`
- No render behavior change yet.

Files:
- `Lightning/scene_color_model.h`
- `Lightning/scene_color_model.c`

## P1 (hard-cut applied)

- Extend zone role composition to consume semantic entities:
  - `AMBIENT_BASE` from `entity.base`
  - `GUIDANCE_LINE` from `entity.guidance_line`
  - `SAFETY_ALERT` from `entity.safety_alert`
- Add optional per-zone tint weights.

Status:
- entity-role layer is integrated as active pipeline path (always-on).
- rollout flag removed intentionally (no gradual mode).
- non-strip zone rendering path now consumes `scene_color_entity_t`
  (legacy non-strip sampling path removed from active route).
- strip rendering path also consumes `scene_color_entity_t`;
  legacy `scene_model_sample_zone` path removed.
- `zones.c` runtime path now uses `scene_color_entity_from_ambient(...)`
  directly (no `scene_color_model_t` usage in zone pipeline).
- overlay colors in `zones_apply_interrupt_overlay()` now come from
  entity-driven overlay palette (warm/cool/parking/safety/guidance)
  instead of fixed RGB constants in the zone module.

## P2 (in progress)

- Add zone-group and role-policy descriptors:
  - per-board group capabilities,
  - role priorities,
  - blend policy per role.

Status:
- `zone_roles` now exposes zone descriptors (`role_mask` + `group_mask`)
  and role policies (`priority`, `preferred_blend`, `alpha_cap`).
- composition apply pass now executes roles by policy priority order
  (base-first to safety-last), not by enum declaration order.
- role submissions are now policy-clamped by `alpha_cap`.
- board-specific group/role mask adjustments are centralized in
  `zone_roles.c` board policy helpers.

## P3 (started)

- Introduce premium scene presets:
  - calm / classic / sport / lounge-like variants
  - dynamic accents and zone choreography hooks.

Status:
- event layer now has palette-driven auto builders:
  - `event_scene_build_dual_accent_auto(...)`
  - `event_layer_start_hvac_wave_auto(...)`
- director/runtime events migrated to auto builders, removing direct
  hardcoded warm/cool RGB usage from those call sites.

## P4

- Validation/stress campaign:
  - CAN replay with high frame rate,
  - queue/load diagnostics,
  - visual regression checklist.
