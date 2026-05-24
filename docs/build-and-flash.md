# Build and Flash

## Toolchain

Project targets `STM32G431CBUx` and expects ARM GNU Embedded toolchain (`arm-none-eabi-*`).

If `arm-none-eabi-gcc` is not found in shell PATH, use CubeIDE toolchain path explicitly.

## Build (CLI)

Debug:
```bash
make -C Debug all -j4
```

Release:
```bash
make -C Release all -j4
```

Clean:
```bash
make -C Debug clean
make -C Release clean
```

## Board selection

Select active board in:
- `Boards/board_selected.h`

This controls zone map and board-specific LED topology.

## Flashing

Use STM32CubeIDE or your standard STM32 flashing flow for the generated ELF from `Debug/` or `Release/`.

## Post-flash smoke check

- Device boots without watchdog resets.
- CAN RX seen for required IDs.
- Scene enters `ACTIVE` after gating conditions.
- Color/brightness updates from modern CAN requests are applied.

## Bench validation

Use prepared scenarios:
- `docs/can-bench-examples.md`
- `docs/can-hacker-presets.md`
- `docs/canhacker.tsk`
