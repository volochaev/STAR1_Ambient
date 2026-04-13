# Refactor Log

## 2026-04-13

- Split CAN monolith into role/theme/rx/tx/power modules with `ambient.c` as façade.
- Added runtime modules in `Core`:
- `runtime_stop` (STOP/wakeup loop and ISR bridge)
- `runtime_can` (CAN TX cadence)
- `runtime_flow` (runtime state machine and active/sleep orchestration)
- `runtime_render` (postprocess + black frame reset)
- Added `led_runtime` façade to reduce direct driver coupling in orchestration.
- Moved `board_dispatch` from header-only static inline to compiled C module.
- Added `board_led_backend` to reduce duplicated board init/render/DMA code paths.
- Added `board_selected.h` as a single board profile selection point for all modules.
- Moved strong `g_zone_map` binding from `main.c` to `Boards/board_zone_map.c`.
- Closed CAN state leak from public API:
- removed `g_can_state` export from `ambient.h`
- introduced internal `ambient_internal.h` for CAN-only translation units.
- Introduced neutral naming aliases:
- `palette_t`/`palette_id_t` in `palette.h`
- `effect_*` wrappers in `effects.h`
- migrated `base_scene.c` to neutral palette/effect calls.
- migrated `zones.c` and `event_layer.c` palette access to `palette_*` aliases.
- Extended `led_runtime` with pixel and RGB buffer helpers (`set/get/copy`).
- Migrated `base_scene.c`, `zones.c`, and `event_layer.c` off direct `ws_*` calls and direct `rgb` buffer access.
- Updated `base_scene.h` public API to `led_runtime_strip_t*` for orchestration-level type neutrality.
- Updated `zone_map_t` to `led_runtime_strip_t *strip` and removed direct `driver.h` dependency from `zones.h`.
- Tuned premium FX behavior in `effects.c`:
- `FX_SPARKLE`: deterministic per-LED twinkle envelope (reduced random popping)
- `FX_CASCADE`: smoother first-frame and temporal blending path
- `FX_TWO_TONE_WAVE`: added temporal smoothing for silkier transitions
