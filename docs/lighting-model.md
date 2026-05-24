# Lighting Model Deep Dive (M4)

Этот документ - подробная спецификация текущего production lighting pipeline.
Он описывает, как система строит кадр после отказа от legacy-концепции `theme -> palette -> effect` как основной модели управления.

## 1. Scope и цели

Что покрывает документ:
- full pipeline: `CAN -> state store -> scene entity -> roles/groups -> overlays -> postprocess -> LED output`;
- математическая модель цвета и динамического смешивания;
- влияние `motion_profile` и `scene_preset`;
- policy/safety constraints;
- runtime diagnostics и practical debug flow.

Что не покрывает:
- низкоуровневый WS2812 DMA driver;
- board-level pinmux/timer electrical details;
- отдельный контур подсветки ручки (см. `docs/handle-lighting.md`).

## 2. Архитектурный срез

Основные модули:
- `CAN/ambient_rx.c` - parsing CAN-сигналов и derived runtime flags.
- `CAN/ambient_state_store.c` - normalized ambient snapshot (single source for render path).
- `Lightning/scene_color_model.c` - преобразование `color_id` в `scene_color_model_t` и `scene_color_entity_t`.
- `Lightning/scene_preset.c` - выбор активного пресета по runtime-контексту.
- `Lightning/lighting_profile_registry.c` - единый реестр коэффициентов пресетов (SSOT).
- `Lightning/zones.c` - базовый scene sampling по зонам.
- `Lightning/zone_roles.c` - role/group композиция с strict policy.
- `Lightning/event_layer.c` - контекстные scene overlays/events.
- `Core/Src/runtime_render.c` - postprocess, tint/boost/slew, финальная стабилизация.

## 3. Входная модель состояния

Непосредственно для цвета/динамики ядро использует normalized snapshot из `ambient_state_store`:
- `color_id`;
- `brightness`;
- `effect_id`;
- `valid`.

Отдельно runtime учитывает контекстные state-сигналы:
- `night_mode`;
- `motion_profile`;
- `reverse/parking/bsm/hvac/...` флаги для overlays/event layer.

Ключевой принцип:
- CAN request frames (`0x463`, legacy ingress `0x325`) не рендерятся напрямую,
- рендер всегда работает через нормализованный internal state snapshot.

## 4. Scene Color Entity

## 4.1 Структуры данных

`Lightning/scene_color_model.h`:
- `scene_color_model_t`:
  - `primary`
  - `accent_a`
  - `accent_b`
  - `neutral`
  - `energy`
  - `valid`
- `scene_color_entity_t`:
  - `base`
  - `accent_warm`
  - `accent_cool`
  - `neutral_soft`
  - `safety_alert`
  - `guidance_line`
  - `energy`
  - `valid`

## 4.2 Color wheel mapping

`scene_color_model_from_ambient(...)` использует 12-step HU wheel anchors (`k_wheel12`) и берёт:
- `idx = color_id % 12`
- `primary = k_wheel12[idx]`
- `accent_a = mix(primary, next_color, 0.42)`
- `accent_b = mix(primary, prev_color, 0.42)`
- `neutral = luma(primary) с keep_color=0.22`

Таким образом `color_id` не равен "готовому RGB", а является входом в модель, которая генерирует базу + акценты + нейтральный компонент.

## 4.3 Energy семантика

`energy` задает характер сцены:
- baseline `0.55`
- `calm -> 0.35`
- `sport -> 0.82`
- при `night_mode` дополнительно `* 0.78`

`energy` затем используется downstream для скорости и насыщенности динамики.

## 5. Scene Preset

## 5.1 Пресеты

`Lightning/scene_preset.h`:
- `SCENE_PRESET_CLASSIC`
- `SCENE_PRESET_CALM`
- `SCENE_PRESET_SPORT`
- `SCENE_PRESET_LOUNGE`

## 5.2 Deterministic mapping

`scene_preset_resolve(...)`:
- `night_mode && profile != sport` -> `LOUNGE`
- иначе `profile == calm` -> `CALM`
- иначе `profile == sport` -> `SPORT`
- иначе `CLASSIC`

Это фиксирует mapping:
- `sport -> sport`
- `calm -> calm`
- `luxury/day -> classic`
- `luxury/night -> lounge`

## 5.3 Multipliers (production values)

`scene_preset_t` multipliers:
- `accent_gain`
- `hvac_wave_gain_mul`
- `welcome_hold_mul`
- `temporal_scale`
- `contrast_gain`
- `neutral_mix_bias`
- `overlay_gain_mul`
- `motion_tint_mul`
- `energy_sat_mul`

Текущие коэффициенты:

| Preset | accent | hvac | welcome | temporal | contrast | neutral_bias | overlay | tint | sat |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| classic | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 0.00 | 1.00 | 1.00 | 1.00 |
| calm | 0.90 | 0.88 | 1.10 | 0.88 | 0.94 | 0.05 | 0.92 | 0.97 | 0.94 |
| sport | 1.10 | 1.15 | 0.92 | 1.16 | 1.10 | -0.05 | 1.10 | 1.04 | 1.08 |
| lounge | 0.86 | 0.85 | 1.18 | 0.84 | 0.92 | 0.08 | 0.88 | 0.96 | 0.92 |

## 5.4 Effect mode semantics (M4.1)

- `effect_id=0`: static OEM wheel color mode.
- `effect_id=1`: `Color Worlds` mode.

In `Color Worlds` mode:
- HU selected color is used to choose world by fixed LUT mapping.
- Rendered palette comes from fixed world stops (world-first), not from anchor-neighbor cycling.
- Preset keeps affecting speed/gain only.

## 6. Dynamic Mixing: что это значит сейчас

После hard-cut динамика больше не строится на bank-switch/palette-step.

Текущая модель динамики:
- time-dependent blend между `accent_warm` и `accent_cool`;
- zone-dependent phase offsets (`handle/storage/footwell` получают разную фазу);
- preset-scaled temporal behavior (`temporal_scale`, `contrast_gain`, `neutral_mix_bias`);
- role/group composition с разными blend-режимами и caps;
- postprocess gain/tint/slew и overlay envelope.

## 6.1 Zone sampling math (упрощенно)

В `scene_entity_sample_zone(...)`:
- `mix_wc = 0.5 + 0.5*sin(2*pi*(t*0.06) + phase + pos*1.9)`
- `wc = lerp(accent_warm, accent_cool, mix_wc)`
- `mix_neutral = 0.10 + 0.34*(1-energy) + preset.neutral_mix_bias` (clamped 0.02..0.65)
- intermediate:
  - `out = base*(0.70/contrast_gain) + wc*(0.30*contrast_gain)`
- final:
  - `out = lerp(out, neutral_soft, mix_neutral)`

Итог:
- да, `color_id` дискретный;
- нет, выход не статичен: итог RGB на пикселе изменяется во времени, по зоне и по preset.

## 7. Зональный рендер и роли

## 7.1 Зоны

Логические зоны:
- `ZONE_STRIP`
- `ZONE_HANDLE`
- `ZONE_STORAGE`
- `ZONE_FOOTWELL`

## 7.2 Роли (`zone_roles`)

`zone_role_id_t`:
- `AMBIENT_BASE`
- `GUIDANCE_LINE`
- `COMFORT_POOL`
- `ATTENTION_POINT`
- `SAFETY_ALERT`

Бленды (`zone_blend_mode_t`):
- `ADD`
- `MAX`
- `OVERRIDE`

## 7.3 Группы (`group_policy`)

`zone_group_id_t`:
- `MAIN_LINE`
- `INTERACTION`
- `COMFORT`
- `SERVICE`

Каждая группа задает:
- allowed role mask,
- preferred blend,
- alpha cap,
- priority offset.

## 7.4 Strict enforcement

`zone_roles_submit(...)` принимает вклад только если одновременно выполнено:
- роль разрешена `zone.role_mask`,
- роль разрешена `group_policy.allowed_role_mask`.

Иначе вклад отклоняется и идет в diag counters (`rejected_by_role` / `rejected_by_group`).

## 8. Safety invariants

`SAFETY_ALERT` имеет отдельные гарантии:
- floor по effective alpha (`ZONE_SAFETY_ALPHA_FLOOR`);
- запрет деградирующего blend в group override path;
- fail-safe fallback при policy конфликте;
- инкременты диагностики (`safety_fallbacks`, `safety_floor_applied`).

Это обеспечивает доминирование safety слоя над decor/info в допустимых зонах.

## 9. Event layer и overlays

`Lightning/event_layer.c` формирует контекстные сцены и hold-состояния:
- unlock signature,
- door choreography (`pre-glow -> strip trail`),
- HVAC warm/cool wave,
- context hold после климат/парковка/reverse событий.

`zones_apply_interrupt_overlay(...)` добавляет runtime overlays:
- parking visualization,
- BSM safety,
- reverse context,
- side-aware HVAC effects.

Все overlay gain проходят через `preset.overlay_gain_mul`.

## 10. Postprocess (`runtime_render_postprocess_frame`)

Финальный кадр дополнительно модифицируется:
- motion profile tint (с учетом `preset.motion_tint_mul`),
- energy/saturation shaping (`energy_sat_mul`),
- drive-mode transient boost envelope,
- channel slew limiting для уменьшения flicker/jitter,
- cabin/day-night gain normalization.

Результат postprocess - это уже кадр, уходящий в `board_dispatch_led_render_all()`.

## 11. Brightness pipeline

Яркость в кадре не равна просто CAN brightness:
- базовый коэффициент берется из `base_scene.calc_brightness`;
- далее zone-relative factors (`handle/storage/footwell` раздельно);
- comfort/hvac dim modifiers;
- overlay-specific gain;
- postprocess normalization.

Поэтому "много операций с яркостью" - это намеренная многоступенчатая декомпозиция:
- photometric consistency,
- контекстные поправки,
- безопасность и UX-приоритеты.

## 12. WAIT_OEM / ACTIVE поведение

В `WAIT_OEM` система ждет валидный startup источник и после gate:
- инициализирует director/base scene,
- инициализирует modern scene context (`entity + preset`),
- запускает welcome choreography.

В `ACTIVE` loop порядок:
1. director update,
2. runtime event hooks,
3. scene tick,
4. overlays,
5. postprocess,
6. render dispatch.

## 13. Legacy ingress в новой модели

Legacy `0x325` может приходить как входной протокол совместимости, но:
- не возвращает старый theme/palette pipeline,
- трансформируется в современное normalized ambient state,
- дальше всегда используется единый modern render path.

## 14. Диагностика и наблюдаемость

`runtime_debug_hooks` snapshot содержит:
- queue metrics: `size/high_watermark/dropped`;
- zone roles counters:
  - `submitted`,
  - `rejected_by_role`,
  - `rejected_by_group`,
  - `alpha_clamped`,
  - `safety_fallbacks`,
  - `safety_floor_applied`;
- `active_role_mask` и `effective_role_order` per zone;
- активный preset + multipliers.

Практический debug вопрос "почему зона так светит" разруливается именно через этот snapshot.

## 15. Практические tuning points

Изменения характера сцены без архитектурных изменений:
- preset multipliers (`scene_preset.c`),
- wheel anchors (`scene_color_model.c`),
- zone relative brightness factors (`ambient_color_brightness.h`),
- overlay gains/holds (`ambient_color_brightness.h`, `ambient_system_timing.h`),
- role/group policy caps/blends (`zone_roles.c`).

Рекомендованный порядок tuning:
1. preset multipliers,
2. overlay gains,
3. role/group caps,
4. color anchors (самый дорогой по regressions).

## 16. Ответ на ключевой вопрос: "цвет статичный или нет?"

- Входной `color_id` - дискретный state.
- Выходной цвет каждого пикселя - динамический сигнал, зависящий от:
  - `scene_color_entity`,
  - `scene_preset`,
  - `zone role/group policy`,
  - `event/overlay context`,
  - `postprocess`.

Именно это и заменило legacy "themes/effects" в M4.
