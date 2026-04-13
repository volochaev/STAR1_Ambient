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
- Theme/state persistence
- Sleep/power policy

3. `Lightning/*`
- Theme banks, effects and palettes
- Base scene lifecycle (intro/active/bridge/outro)
- Zone mapping application and overlay composition

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
- update director
- tick base scene
- apply zones/overlays/postprocess
- render all lines
- periodic CAN TX scheduler
