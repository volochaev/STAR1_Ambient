# CAN Layer Reference

## Scope

This document describes the full CAN layer used by STAR1 Ambient firmware:
- modern ambient control (`0x463` + `0x12B`)
- internal board synchronization (`0x353`)
- OEM-compatible ambient path (`0x325`)
- auxiliary vehicle signals that affect scene logic, dimming, events, and safety overlays.

Primary implementation:
- `CAN/ambient.c`
- `CAN/ambient_rx.c`
- `CAN/ambient_tx.c`
- `CAN/ambient_backend*.c`
- `CAN/ambient_state_store.c`

## Architecture

1. `can_ambient_process_rx(id,data,len)` routes each frame by ID.
2. RX handlers update CAN-derived runtime state and event latches.
3. Modern backend consumes normalized request/state into `ambient_state_store`.
4. Runtime/director/zones consume this state for rendering and behavior.

## Core ambient control IDs

### `0x463` HU ambient request

Role:
- request from HU to set ambient parameters.

Decoded request fields (working map):
- `brightness_rq`: `Data[2] & 0x0F` (`AmbBrt_F_Rq`)
- `color_rq`: `Data[3] & 0x0F` (`AmbLgtColor_Rq`)
- `effect_rq`: `Data[5] & 0x03` (`AmbLgtEffectConfig_Rq`)

Runtime effect:
- stored as latest modern request (`g_modern_rq_*`)
- consumed by backend -> `ambient_state_store_update_from_modern_request(...)`
- active render semantics:
  - `effect=0`: static OEM color mode
  - `effect=1`: Color Worlds animated mode (fixed LUT world selection by HU color)
- source priority semantics:
  - if `0x463` request is fresh, modern request is authoritative;
  - if `0x463` is stale/missing, runtime falls back to legacy `0x325` source via legacy->modern transform
    (3 bank model with slot-cycling workaround, `effect` forced to `0`).
- ingress validation:
  - invalid length (`len < 6`) increments `rq_invalid_len_count`
  - out-of-range values are counted (`rq_invalid_range_count`) and clamped:
    - brightness `0..5`
    - color `0..12`
    - effect already bounded by `& 0x03`

### `0x12B` body ambient status

Role:
- unified ambient status format used as shared source-of-truth style payload.

Decoded status fields (working map):
- `brightness_stat`: `Data[4] & 0x0F` (`AmbBrt_F_Stat`)
- `color_stat`: `(Data[4] >> 4) & 0x0F` (`AmbLgtColor_Stat`)
- `effect_stat`: `Data[7] & 0x03` (`AmbLgtEffectConf_Stat`)

Runtime effect:
- tracked as bus status (`g_modern_stat_*`)
- used for conflict/diagnostic visibility in modern path.
- status semantics follow request semantics (`0=static`, `1=Color Worlds`) in M4.1.
- ingress validation:
  - invalid length (`len < 8`) increments `stat_invalid_len_count`
  - out-of-range values are counted (`stat_invalid_range_count`) and clamped:
    - brightness `0..5`
    - color `0..12`
    - effect already bounded by `& 0x03`

### `0x353` internal master protocol

Role:
- inter-board sync/discovery over a single ID with packet-type nibble in `data[0]`.

Packet types:
- `PKT_TYPE_DISCOVERY = 0x00`
- `PKT_TYPE_MASTER    = 0x10`
- `PKT_TYPE_SYNC      = 0x20`

Behavior:
- discovery: role election presence signaling
- master packet: synchronized state from active master to slaves
- sync packet: heartbeat/failover continuity

### `0x325` OEM ambient input from IC (legacy-compatible, still active)

Role:
- OEM ambient source retained for compatibility.

Decoded fields used by firmware:
- brightness raw: `Data[0]` -> clamped to `0..5`
- color raw source:
  - default candidate: `Data[3] bits[3:2]`
  - alternate candidate: `Data[3] bits[5:4]`
  - auto-source lock policy supported (`AMB_OEM_COLOR_SOURCE_POLICY`)

Color mapping:
- `0 -> OEM_COLOR_AMBER`
- `1 -> OEM_COLOR_WHITE`
- `2 -> OEM_COLOR_BLUE`

Runtime effect:
- updates `g_can_state.oem_color`, `g_can_state.oem_brightness`
- feeds compatibility path and OEM-derived behavior
- OEM packet freshness gate participates in startup/validity logic.
- ingress validation:
  - invalid color decode increments `oem_invalid_range_count` and frame is rejected
  - brightness is clamped to `0..5`, each clamp increments `oem_clamped_brightness_count`

## Normalized modern state

`ambient_state_store` fields:
- `color_id` (clamped `0..12`)
- `brightness_raw` (clamped `0..5`)
- `effect_id` (clamped `0..3`)
- `brightness_norm = brightness_raw / 5.0`
- validity/timestamp/sequence metadata

Persistence:
- `CAN/ambient_persist.*`
- `CAN/flash_storage.*`

## Auxiliary IDs and runtime impact

| ID | Name | What is read | Runtime impact |
|---|---|---|---|
| `0x025` | `CAN_LGTSENS_ID` | light sensor day/night bits | primary night-mode source |
| `0x2E9` | `CAN_TGW_A8_ID` | HU/TGW day-night status | night-mode fallback when sensor stale |
| `0x069` | `CAN_LM_A1_ID` | NS illumination active | additional night/context behavior |
| `0x141` | `CAN_ILM_RQ_ID` | front/rear/bg dim requests | ILM dim scaling per board |
| `0x006` | `CAN_SAM_F_A1_ID` | ignition state | startup gate / wake behavior |
| `0x02F` | `CAN_LM_A4_ID` | puddle/entry request bits | handle/entry related request gating |
| `0x12D` | `CAN_EIS_A2_ID` | lock/unlock request bits | lock/unlock event latches |
| `0x002` | `CAN_DOOR_FL_A1_ID` | door latch state | per-door open events / open-state map |
| `0x004` | `CAN_DOOR_FR_A1_ID` | door latch state | per-door open events / open-state map |
| `0x007` | `CAN_DOOR_R_A1_ID` | rear latch states | RL/RR open events / open-state map |
| `0x20B` | `CAN_HVAC_FRONT_ID` | temp setpoints L/R | HVAC trend events + split bias |
| `0x0BC` | `CAN_HVAC_REAR_ID` | temp setpoints L/R | HVAC trend events + split bias |
| `0x339` | `CAN_HVAC_FAN_ID` | fan level raw | smoothed fan-level modifier |
| `0x231` | `CAN_HVAC_A1_ID` | sun intensity FL/FR | sun compensation gain |
| `0x369` | `CAN_SEAT_FRONT_ID` | seat heat front L/R | seat heat level by side/board |
| `0x350` | `CAN_SEAT_REAR_ID` | seat heat rear L/R | seat heat level by side/board |
| `0x17E` | `CAN_BSM_ID` | BSM warning/acoustic or permissive activity | safety overlay (left/right active + level) |
| `0x2EE` | `CAN_PTS_A1_ID` | parking warning nibbles | parking overlay levels (left/right/rear) |
| `0x2C3` | `CAN_RVC_A2_ID` | reverse indication | reverse scene / reverse hold logic |
| `0x10C` | `CAN_CHASSIS_R2_ID` | gear raw (reverse detection) | reverse scene / reverse hold logic |

## Event and state side-effects

CAN layer maintains event/state latches consumed by runtime:
- unlock/lock pending flags and debouncing
- door-open event mask and per-board filtering
- night-mode source selection with staleness handling
- parking/reverse/BSM/HVAC derived overlay states
- ignition and request freshness gates for startup/sleep transitions

## CAN to Scene Influence (compact map)

| CAN ID | Signal class | Main scene / preset impact | Overlay impact |
|---|---|---|---|
| `0x325` | OEM legacy ambient | legacy ingress -> modern internal snapshot (`color/brightness`) | indirect via unified state |
| `0x463` | HU ambient request | modern request state (`color/brightness/effect`) | indirect via unified state |
| `0x12B` | body ambient status | bus truth/consistency diagnostics | conflict diagnostics |
| `0x025`, `0x2E9` | day/night | selects `classic` vs `lounge` for luxury profile | scales overlay behavior through preset |
| `0x38E` / drive profile source | motion profile | selects `calm/sport` preset; affects temporal/contrast signature | event/overlay gain scaling through preset |
| `0x339` | HVAC fan level | influences temporal motion modifiers | — |
| `0x20B`, `0x0BC` | HVAC temp trend/split | — | HVAC wave/split overlays |
| `0x369`, `0x350` | seat heat | — | comfort overlays (`warm` accents) |
| `0x2EE` | parking warn | — | parking pulse/context overlays |
| `0x17E` | BSM | — | `SAFETY_ALERT` overlays (dominant role) |
| `0x2C3`, `0x10C` | reverse | reverse scene modifiers | reverse-related safety/context behavior |

## Diagnostics

Exposed diagnostics include:
- modern conflict/bus-status tracking (`can_modern_diag_t`)
- ingress validator counters:
  - `rq_invalid_len_count`
  - `rq_invalid_range_count`
  - `stat_invalid_len_count`
  - `stat_invalid_range_count`
  - `oem_invalid_range_count`
  - `oem_clamped_brightness_count`
- sleep/wake counters and reasons (`can_power_diag_t`)
- OEM decode debug fields (`oem diag` bytes/flags)

Quick-access helper:
- `can_ambient_get_ingress_counter(can_ingress_counter_id_t id)` returns a single counter without unpacking the full struct.

## Notes

- Preserve unrelated bits when composing partial-byte signal writes.
- Range clamp is intentional at ingress to keep renderer deterministic.
- `0x325` support remains active by design for compatibility.
