# Handle Lighting Deep Dive (Door Handle Backlight)

Этот документ описывает отдельный контур подсветки ручки, который intentionally отделен от RGB strip scene pipeline.

## 1. Почему это отдельная тема

В проекте есть два разных типа "ручки":
- `ZONE_HANDLE` в RGB-зональном рендере (адресные светодиоды, если есть на плате),
- hardware handle backlight через отдельный PWM канал (`handle_pwm`, PT4211 DIM path).

Этот документ - про второй контур (PWM), потому что у него:
- свой источник правды (CAN source policy),
- свой lifecycle (может работать вне `ACTIVE` strip scene),
- своя anti-flicker и sleep интеграция.

## 2. Модульная карта

Основные файлы:
- `Core/Src/handle_pwm.c`, `Core/Inc/handle_pwm.h` - PWM engine (slew + active-low support).
- `Core/Src/runtime_flow.c` - runtime orchestration вызовов `handle_pwm_*`.
- `CAN/ambient.c` - `can_ambient_handle_request_active()` (выбор источника через policy).
- `CAN/ambient_rx.c` - парсинг CAN-сигналов для возможных handle source.
- `Core/Inc/ambient_feature_flags.h` - source policy constants.
- `Core/Inc/ambient_system_timing.h` - hold/debounce timing для handle path.

## 3. Источник запроса (source policy)

`can_ambient_handle_request_active()` выбирает источник по `AMB_HANDLE_SOURCE_POLICY`:
- `AMB_HANDLE_SOURCE_NS_ILL_ACTV` -> `can_rx_is_ns_ill_active()`
- `AMB_HANDLE_SOURCE_IL_R_ON_RQ` -> `can_rx_is_ilm_rear_on_request()`
- `AMB_HANDLE_SOURCE_PDDLLMP_ANY` -> `can_rx_is_pddllmp_any_on_request()`
  - и, если не debug override, с reverse-block guard
- fallback -> `can_rx_nsi_active()`

Текущий default (по конфигу) обычно `AMB_HANDLE_SOURCE_PDDLLMP_ANY`.

## 4. CAN-сигналы, влияющие на handle PWM

## 4.1 `0x02F` LM_A4 (puddle/entry requests)

`handle_lm_a4(...)`:
- читает `PddlLmp_FL/FR/RL/RR_On_Rq` из `byte5[7:4]`;
- агрегирует в `pddl_any`;
- ведет anti-flicker hold timestamps:
  - `g_pddllmp_last_on_ms`,
  - `g_pddllmp_off_candidate_ms`.

## 4.2 NSI (`handle_oem_nsi(...)`)

- декодирует `Surr_Ill_Rq`;
- применяет off-confirm и hold logic (`AMB_SURRILL_HANDLE_HOLD_MS`, `AMB_SURRILL_OFF_CONFIRM_MS`);
- обновляет `g_nsi_active`.

## 4.3 Reverse block guard

Когда policy основана на `PDDLLMP_ANY`, дополнительно может блокироваться в reverse через `can_rx_is_reverse_handle_block_active()` с hold-окном (`AMB_REVERSE_HANDLE_BLOCK_HOLD_MS`).

Цель: не включать handle path в нежелательных reverse-фазах и не ловить CAN phase jitter.

## 5. Runtime state machine integration

## 5.1 WAIT_OEM поведение

В `RUNTIME_WAIT_OEM` handle PWM intentionally decoupled от strip ACTIVE gate:
- если `can_ambient_handle_request_active()` и не sleep -> PWM enable,
- иначе disable,
- `handle_pwm_tick(dt)` вызывается даже при отсутствии перехода в `ACTIVE`.

Зачем:
- избежать STOP/wake циклов, когда нужен только handle light;
- позволить source-owned поведение ручки без требования полного scene startup.

## 5.2 ACTIVE/SLEEP_PREP поведение

`runtime_update_handle_pwm(...)`:
- базово включает PWM только если handle request active;
- при sleep fade-out плавно уменьшает brightness_pct до 0 по `AMB_SLEEP_FADE_OUT_MS`;
- учитывает sleep-состояние и forced sleep logic.

## 5.3 STOP/WAKE

`runtime_sleep_finish_handle_fade(...)` и lifecycle API:
- `handle_pwm_prepare_stop()` перед STOP (hard off + PWM stop),
- `handle_pwm_resume_after_stop()` после wake (restart PWM with safe off state).

## 6. PWM engine (`handle_pwm.c`)

## 6.1 Electrical polarity

`AMB_HANDLE_PWM_ACTIVE_LOW`:
- `1` - активный low (sink-to-GND topology),
- `0` - active high.

Функции `pwm_off_compare()` и `pwm_compare_from_level()` учитывают эту полярность.

## 6.2 Переходные процессы

Slew model:
- attack time: `AMB_HANDLE_PWM_ATTACK_MS`,
- release time: `AMB_HANDLE_PWM_RELEASE_MS`,
- дискретный шаг на `handle_pwm_tick(dt_ms)`.

Формула шага:
- `alpha = dt / (tau + dt)`
- `level += (target - level) * alpha`

Где `tau` зависит от направления изменения (attack/release).

## 6.3 Brightness input

`handle_pwm_set_brightness_pct(0..100)` задает target.
Runtime в обычном режиме использует 100% при активном запросе, но sleep fade может временно снижать pct.

## 7. Почему отдельный PWM контур важен

1. Разные физические каналы и драйверы (WS2812 vs PT4211 DIM).
2. Разные UX-требования (ручка как entry/safety источник, не только ambient decor).
3. Разные режимы жизни (должна работать даже когда scene-strip еще не активирован).

## 8. Взаимодействие с RGB `ZONE_HANDLE`

В системе возможны оба пути одновременно:
- RGB `ZONE_HANDLE` участвует в общей scene/overlay композиции;
- PWM-handle path живет отдельно.

Это не конфликт, если воспринимать их как:
- ambient appearance layer (RGB),
- functional entry illumination layer (PWM).

## 9. Точки настройки

## 9.1 Source selection

Файл: `Core/Inc/ambient_feature_flags.h`
- `AMB_HANDLE_SOURCE_POLICY`
- `AMB_PDDLLMP_IGNORE_REVERSE_FOR_DEBUG`

## 9.2 Timing and anti-flicker

Файл: `Core/Inc/ambient_system_timing.h`
- `AMB_SURRILL_HANDLE_HOLD_MS`
- `AMB_PDDLLMP_HANDLE_HOLD_MS`
- `AMB_REVERSE_HANDLE_BLOCK_HOLD_MS`
- `AMB_HANDLE_PWM_ATTACK_MS`
- `AMB_HANDLE_PWM_RELEASE_MS`

## 9.3 Sleep behavior

Файл: `Core/Src/runtime_flow.c`
- `runtime_update_handle_pwm(...)`
- `runtime_sleep_finish_handle_fade(...)`

## 10. Диагностика и отладка

Рекомендуемый debug sequence:
1. Проверить raw source flags в `ambient_rx` debug globals (LM_A4/NSI/reverse).
2. Проверить результат `can_ambient_handle_request_active()`.
3. Проверить runtime ветку (`WAIT_OEM`/`ACTIVE`/`SLEEP_PREP`).
4. Проверить PWM state:
   - enabled,
   - target pct,
   - current level/compare.
5. Проверить полярность `AMB_HANDLE_PWM_ACTIVE_LOW` (частый источник "инверсного" поведения).

Типовые проблемы:
- ложный OFF из-за отсутствия off-confirm hold;
- "мигание" из-за слишком коротких attack/release;
- unexpected block в reverse hold window;
- perceived lag при слишком больших tau.

## 11. Safety/UX инварианты

- Handle path не должен самопроизвольно включаться без активного source request.
- В sleep transition должен корректно затухать/отключаться.
- После wake должен возвращаться в deterministic off state до следующего valid request.
- Reverse guard (если включен политикой) имеет приоритет над `PDDLLMP_ANY` request.

## 12. Практический ответ на "ручка относится к lighting model?"

Да, но как отдельная подсистема:
- в общей lighting архитектуре она часть конечного UX освещения салона,
- в коде это независимый функциональный канал с собственными правилами,
- поэтому документационно ее правильно держать отдельно от RGB scene pipeline.
