# STAR1 Ambient Architecture

## Layering

1. `Core/*`
- Runtime orchestration and state machine (`runtime_flow`)
- STOP/wakeup policy (`runtime_stop`)
- CAN TX cadence (`runtime_can`)
- Frame postprocess (`runtime_render`)
- LED runtime façade (`led_runtime`)

2. `CAN/*`
- Transport protocol and role policy
- RX parsing and derived state
- Source arbitration (`0x463` modern fresh-window priority, `0x325` legacy fallback) in `ambient_source_arbiter`
- Theme/state persistence
- Sleep/power policy

3. `Lightning/*`
- Scene entity model (`scene_color_entity`) and preset signature (`scene_preset`)
- Canonical preset registry (`lighting_profile_registry`) for classic/calm/sport/lounge multipliers
- Color Worlds world-first animation layer for `effect=1`
- Base scene lifecycle (intro/active/bridge/outro)
- Zone mapping application and overlay composition
- Dedicated handle-PWM lighting path exists in `Core/Src/handle_pwm.c` and is runtime-managed outside WS2812 role-composition flow

4. `Boards/*`
- Physical strip/channel binding for each hardware variant
- Board-local strip initialization and frame rendering
- Single board selection point (`board_selected.h`)
- Unified dispatch entrypoints (`board_dispatch.c`)
- Shared line backend (`board_led_backend.c`)
- Strong board zone binding (`board_zone_map.c`)
- Board-local files no longer export weak `g_zone_map` fallbacks

## Boundaries

- CAN internal state:
- `g_can_state` is internal (`CAN/ambient_internal.h`), not exported from `CAN/ambient.h`.
- Upper layers consume CAN data only through `can_ambient_*` API.

- LED runtime boundary:
- `Core/CAN/Boards dispatch` and orchestration in `Lightning/base_scene|zones|event_layer` use `led_runtime_*` API.
- Direct `ws_*` calls are constrained to `Core/led_runtime`, WS driver/effects/palette, and board-local low-level backend files.
- Zone mapping contract (`zones.h`) uses `led_runtime_strip_t` (`zone_map_t.strip`) instead of direct `ws2812_t`.

- Board dispatch boundary:
- `board_dispatch` is compiled C API (`Boards/board_dispatch.c`) instead of header-only static inline router.

## Runtime Flow

- Boot -> Wait OEM -> Active
- Active -> Sleep Prep -> Stop -> Wake Recover -> Wait OEM
- Active pipeline:
- fill runtime inputs
- CAN ingress normalization chain: `ambient_rx` -> `ambient_source_arbiter` -> `ambient_state_store`
- update director
- tick base scene
- resolve preset/world context: `scene_preset` -> `lighting_profile_registry`, `color_worlds`
- apply zones/overlays/postprocess
- render all lines
- periodic CAN TX scheduler

## Verification Hooks

- `CAN/ambient_source_arbiter_selftest()`:
- lightweight logic self-test for fresh-window rule and legacy bank-cycler progression.
- non-runtime-critical; intended for bring-up/regression checks.

## Module Ownership

| Concern | Owner module | Responsibility | Notes |
|---|---|---|---|
| CAN ingress parsing | `CAN/ambient_rx.c` | Decode raw CAN payloads into canonical signals | No render decisions here |
| Source arbitration (`463/325`) | `CAN/ambient_source_arbiter.c` | Choose authoritative source (`463` fresh first, `325` fallback) | Includes legacy bank-cycler behavior |
| Canonical ambient state | `CAN/ambient_state_store.c` | Hold normalized `color_id/brightness/effect` snapshot | Single source for render input state |
| Backend orchestration | `CAN/ambient_backend.c` | Wire CAN-derived state into store via arbiter | Thin facade only |
| OEM wheel mapping | `Lightning/oem_color_wheel.c` | Canonical 12-color wheel RGB table | SSOT for static OEM wheel anchors |
| Preset policy resolve | `Lightning/scene_preset.c` | Resolve active preset from runtime context | Delegates coefficients to registry |
| Preset coefficients | `Lightning/lighting_profile_registry.c` | Store preset multipliers (`classic/calm/sport/lounge`) | SSOT for preset tuning values |
| World palette/animation | `Lightning/color_worlds.c` | Fixed world registry, world sampling, `color_id->world` LUT | World-first path for `effect=1` |
| Scene color entity | `Lightning/scene_color_model.c` | Build semantic scene colors from ambient state + motion context | Input to zones/event layering |
| Zone composition | `Lightning/zones.c` | Render base/world per zone and apply zone-local model | Before overlay/event dominance |
| Role composition policy | `Lightning/zone_roles.c` | Enforce role/group masks and blending order | Safety role dominance constraints |
| Event/overlay effects | `Lightning/event_layer.c` | Build event frames (HVAC/BSM/parking/etc.) | Preset-scaled amplitudes/temporal |
| Frame postprocess | `Core/Src/runtime_render.c` | Temporal slew, contrast/tint/post gains | Final pass before LED output |
| Hardware output | `Core/Src/led_runtime.c`, `Boards/*` | Strip/channel mapping and physical writeout | Board-specific geometry/channel binding |
