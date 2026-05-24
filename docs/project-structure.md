# Project Structure

## Top-level layout

```text
STAR1_Ambient/
├── Core/                    # STM32 HAL + runtime orchestration
│   ├── Inc/
│   └── Src/
├── CAN/                     # CAN parsing/state/tx/persist/power/role
├── Lightning/               # Scene render pipeline and zone logic
├── Boards/                  # Per-board mapping and backend
├── Drivers/                 # STM32 HAL/CMSIS
└── docs/                    # Project documentation
```

## Core runtime modules

- `Core/Src/app_runtime.c` - high-level runtime wiring.
- `Core/Src/runtime_flow.c` - state machine (`BOOT/WAIT_OEM/ACTIVE/SLEEP/STOP/WAKE_RECOVER`).
- `Core/Src/runtime_can.c` - CAN tx scheduling.
- `Core/Src/runtime_events.c` - event hooks/triggers.
- `Core/Src/runtime_render.c` - render helpers/postprocess.
- `Core/Src/runtime_state_machine.c` - transition guards.
- `Core/Src/runtime_stop.c` - STOP/wakeup orchestration.

## CAN modules

- `CAN/ambient.c` - orchestration entry for CAN subsystem.
- `CAN/ambient_rx.c` - parse OEM/HU/BODY + derived states.
- `CAN/ambient_tx.c` - broadcast/status packet build.
- `CAN/ambient_backend.c` - backend facade for ambient ingress/state updates.
- `CAN/ambient_source_arbiter.c` - source arbitration (`463` modern priority, `325` legacy fallback) + legacy bank cycler.
- `CAN/ambient_state_store.c` - normalized ambient state snapshot.
- `CAN/ambient_persist.c` + `CAN/flash_storage.c` - persistence.
- `CAN/ambient_role.c` - discovery/role policy.
- `CAN/ambient_power.c` - sleep/wake policy and diagnostics.

## Lighting modules

- `Lightning/base_scene.c` - base scene lifecycle.
- `Lightning/director.c` - runtime dimming + scene orchestration.
- `Lightning/zones.c` - zone render logic.
- `Lightning/event_layer.c` - overlays/events over base scene.
- `Lightning/scene_color_model.c` - color model generation.
- `Lightning/oem_color_wheel.c` - canonical OEM 12-color wheel lookup.
- `Lightning/scene_preset.c` - active preset selection from runtime context.
- `Lightning/lighting_profile_registry.c` - canonical preset coefficient registry.
- `Lightning/color_worlds.c` - world-first animated ambient palettes (`effect=1`).
- `Lightning/runtime_dimming_policy.c` - dimming policy layer.
- `Lightning/zone_roles.c` - role-based zone contributions.
- `Lightning/driver.c` + `Core/Src/led_runtime.c` - LED output backend.

Model details: [docs/lighting-model.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/lighting-model.md)

## Board abstraction

- `Boards/board_selected.h` - active board profile selection.
- `Boards/board_zone_map.c` - zone map binding for selected board.
- `Boards/board_dispatch.c` - unified board dispatch API.
- `Boards/board_led_backend.c` - common LED backend glue.
- `Boards/board_*.c` - per-board geometry/order/strip specifics.
