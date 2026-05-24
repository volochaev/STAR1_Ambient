# STAR1 Ambient Lighting System

Version: 3.x  
Platform: STM32G431CBUx

## What this repository contains

STAR1 Ambient is an in-cabin ambient lighting firmware with:
- multi-board topology (doors/dashboard/rear)
- modern CAN control path (HU request + body-style status)
- scene-based render pipeline with zone roles and event overlays
- persistence of ambient state in flash

## Documentation Index

### Start here

- Architecture: [docs/architecture.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/architecture.md)
- Project structure: [docs/project-structure.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/project-structure.md)
- Build and flash: [docs/build-and-flash.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/build-and-flash.md)

### CAN

- CAN overview (active modern model): [docs/can.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/can.md)
- CAN Hacker task file: [docs/canhacker.tsk](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/canhacker.tsk)

### Runtime behavior

- Lighting model (entity/preset/roles): [docs/lighting-model.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/lighting-model.md)
- Color Worlds spec (M4.1): [docs/color-worlds.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/color-worlds.md)
- Handle lighting (separate PWM subsystem): [docs/handle-lighting.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/handle-lighting.md)
- Flow state machine: [docs/flow-state-machine.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/flow-state-machine.md)
- M4 incremental plan: [docs/m4-plan.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/m4-plan.md)

### History

- Changelog: [docs/changelog.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/changelog.md)
- M3 freeze baseline: [docs/m3-freeze.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/m3-freeze.md)
- Refactor log: [docs/refactor-log.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/refactor-log.md)

## Current architecture direction

- Legacy theme/palette stack is removed from active code path.
- Runtime is modern-first with centralized ambient state store.
- Event overlays are RGB-first and zone-role aware.
- CAN backend is modern-only in runtime flow.

## Quick build

```bash
make -C Debug all -j4
make -C Release all -j4
```

If toolchain is not in PATH in your shell session, set CubeIDE toolchain path explicitly before build.
