# M3 Freeze Baseline

Date: 2026-05-24  
Repository: `STAR1_Ambient`

## Scope Frozen

This freeze locks the completed `M1/M2/M3` architecture baseline:

- `M1` runtime/state-flow refactor is active and build-stable.
- `M2` modern CAN control and persistence path is active and build-stable.
- `M3` lighting architecture cleanup (modern-first, zone-role/event layering) is active and build-stable.

## Included in Freeze

1. CAN model and ownership
- Modern request/status path: `0x463` (HU request) + `0x12B` (body-style status).
- Legacy compatibility input remains: `0x325`.
- Internal board protocol remains: `0x353`.

2. Canonical state flow
- Active runtime flow (`BOOT -> WAIT_OEM -> ACTIVE -> SLEEP_PREP -> STOP -> WAKE_RECOVER`).
- Event queue ingress path for CAN RX (`runtime_event_queue`).
- Lock/unlock/door/hvac/reverse/parking overlays integrated in runtime/event layers.

3. Persistence
- Unified modern snapshot persisted in flash.
- Deferred save policy and change-triggered persistence active.

4. Robustness baseline
- Ingress validators for `0x463/0x12B/0x325` added.
- Reject/clamp counters exported through diagnostics API.

5. Documentation baseline
- `docs/can.md` is canonical for CAN behavior and IDs.
- Refactored documentation index in `README.md` and split docs set.

## Acceptance Checklist

- [x] Debug build passes
- [x] Release build passes
- [x] Modern CAN path integrated (`0x463/0x12B`)
- [x] Legacy compatibility (`0x325`) retained
- [x] Runtime state machine integrated with STOP/wakeup flow
- [x] Persistence integrated for modern snapshot
- [x] Ingress diagnostics counters exposed
- [x] Documentation updated to current architecture

## Build Verification Snapshot

Validated on 2026-05-24:

- Debug: `text/data/bss = 88384 / 208 / 28240`
- Release: `text/data/bss = 51784 / 208 / 28224`

## Known Limits (Accepted for Freeze)

- No automated HIL regression suite in this repository.
- CAN replay tooling is external/manual process.
- Scene/premium feature expansion beyond current M3 baseline is deferred to next milestone.

## Next Milestone Entry (Post-Freeze)

`M4` starts from this baseline:

- richer scene/color entity model,
- expanded role-compositing and premium multi-zone behavior,
- stress/perf campaign with replay KPIs and queue latency targets.
