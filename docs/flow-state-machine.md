# Runtime Flow State Machine

Документ фиксирует production-flow в `Core/Src/runtime_flow.c` и CAN-события из `CAN/ambient_rx.c`.

## Состояния

- `RUNTIME_BOOT`
- `RUNTIME_WAIT_OEM`
- `RUNTIME_ACTIVE`
- `RUNTIME_SLEEP_PREP`
- `RUNTIME_STOP`
- `RUNTIME_WAKE_RECOVER`

## Таблица переходов: event -> condition -> action

| From | Event | Condition | Action | To |
|---|---|---|---|---|
| `BOOT` | startup tick | всегда | `wait_oem_after_wake=0`, `wait_oem_enter_ms=now` | `WAIT_OEM` |
| `WAKE_RECOVER` | first tick after wake | всегда | `wait_oem_after_wake=1`, `wait_oem_enter_ms=now` | `WAIT_OEM` |
| `WAIT_OEM` | OEM/master snapshot ready | `oem_received=1` и (`ignition ON` или, если `AMB_ENABLE_START_GATE_DOOR_OPEN=1`, дверь для board открыта) | выбрать theme/bank, `director_init()`, `base_scene_init()`, запуск `FX_WELCOME` intro | `ACTIVE` |
| `WAIT_OEM` | wake timeout | `wait_oem_after_wake=1` и `wait_ms >= AMB_WAIT_OEM_RESLEEP_MS` | повторный STOP cycle | `STOP -> WAKE_RECOVER` |
| `ACTIVE` | idle sleep request | `can_ambient_should_sleep()` | старт fade-out (`sleep_fade_active=1`) | `SLEEP_PREP` |
| `SLEEP_PREP` | fade finished | `fade_elapsed >= AMB_SLEEP_FADE_OUT_MS` | outro, power off, `can_ambient_enter_sleep()`, STOP | `STOP` |
| `SLEEP_PREP` | cancel sleep | idle activity восстановилась (и не lock-forced) | `runtime_sleep_cancel()` | `ACTIVE` |
| `STOP` | CAN wake / EXTI wake | wake source от `runtime_stop` | `can_ambient_exit_sleep()`, reset protocol/oem flags | `WAKE_RECOVER` |
| `ACTIVE` | lock event | `can_ambient_consume_lock_event()` и все двери закрыты | `s_lock_sleep_forced=1`, `can_ambient_request_sleep_lock()`, старт outro | `SLEEP_PREP` |
| `SLEEP_PREP` | forced lock override cancel | при `s_lock_sleep_forced=1` получен unlock event или открыта дверь | `s_lock_sleep_forced=0`, cancel sleep | `ACTIVE` |

## Event hooks в `ACTIVE` loop

Порядок (в `runtime_flow_handle_active_state()`):

1. `director_update(...)`
2. `runtime_events_maybe_arm_unlock_from_can(now)`
3. `runtime_events_maybe_trigger_unlock_signature(ctx, now)` (legacy-path, по умолчанию выключен)
4. `runtime_events_maybe_trigger_door_open(now)`
5. `runtime_events_maybe_trigger_lock_goodbye(ctx, main_strip)`
6. `runtime_events_maybe_trigger_hvac_temp(now)`
7. sleep-handling, tick/render, CAN TX scheduler

## CAN-derived события

### Unlock/Lock (`0x12D`, `EIS_A2`)

- Unlock pending по фронту `0->1`:
  - источники: `Ext_Unlk_Rq`, `Dr_*_Unlk_Rq`, `CLkS_TurnLmp_Unlk_Rq`
  - фильтр источника: `CLkS_Src` только `1 (IR)` или `2 (KG)`
  - debounce: `AMB_UNLOCK_EIS_EVENT_DEBOUNCE_MS`
- Lock pending аналогично:
  - `Ext_Lk_Rq`, `Dr_*_Lk_Rq`, `CLkS_TurnLmp_Lk_Rq`
  - тот же `CLkS_Src` фильтр
  - debounce: `AMB_LOCK_EIS_EVENT_DEBOUNCE_MS`
  - при этом `raw lock/unlock` остаются активными (fallback)

### Door latch (`0x002`, `0x004`, `0x007`)

- Статус: `1=CLS`, `2=OPN`.
- Door-open event поднимается только на фронте `CLS -> OPN` (с cooldown).
- Для board routing:
  - `FL`: только FL
  - `FR`: только FR
  - `RL`: только RL
  - `RR`: только RR
  - `DASHBOARD`: `front = FL|FR`
  - `REAR`: `rear = RL|RR`

### Дополнительные DBC-источники

- `0x141 ILM_RQ`: front/rear/background dim requests.
- `0x2E9 TGW_A8`: fallback day/night (`HU_DayNightMd_Stat`) при отсутствии `0x025`.
- `0x2EE PTS_A1`: parking warn overlay (left/right/rear).
- `0x2C3 RVC_A2` + `0x10C CHASSIS_R2`: reverse-scene state.
- `0x231 HVAC_A1`: side-aware sun intensity compensation.
- Event-layer context hold: короткое удержание low-gain контекстного оттенка на strip после climate/door/parking/reverse событий.
- Door choreography: для `door-open` сначала `pre-glow` (`handle/footwell`), затем `strip trail`.

## Ключевые флаги поведения

- `AMB_ENABLE_START_GATE_DOOR_OPEN=1`
- `AMB_DISABLE_WELCOME_INTRO=0`
- `AMB_ENABLE_LOCK_GOODBYE_EVENT=1`
- `AMB_ENABLE_DOOR_OPEN_EVENT_SCENE=1`
- `AMB_ENABLE_UNLOCK_WELCOME_SIGNATURE=0` (legacy-path оставлен в коде, но отключён)

## Итоговое поведение

1. Питание есть, но дверь закрыта: подсветка не стартует даже при OEM пакетах.
2. OEM + дверь открылась: вход в `ACTIVE` через `FX_WELCOME`.
3. При lock и закрытых дверях: мгновенный переход в sleep-последовательность (без ожидания idle timeout).
4. Если во время forced-sleep пришел unlock или открылась дверь: sleep отменяется и система возвращается в `ACTIVE`.
