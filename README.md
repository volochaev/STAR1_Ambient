# STAR1 Ambient Lighting System

**Version:** 3.0
**Platform:** STM32G431CBUx
**Description:** Премиальная система подсветки салона автомобиля вдохновленная реализацией Mercedes-Benz. Поддержка CAN, нескольких каналов и анимаций.

---

## Быстрые Документы

- Архитектура: [docs/architecture.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/architecture.md)
- Лог рефактора: [docs/refactor-log.md](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/docs/refactor-log.md)

---

## Содержание

1. [Структура проекта](#1-структура-проекта)
2. [Архитектура системы освещения](#2-архитектура-системы-освещения)
3. [Драйвер WS2812B](#3-драйвер-ws2812b)
4. [CAN-система](#4-can-система)
5. [Поток данных](#5-поток-данных)
6. [Работа системы](#6-работа-системы)
7. [Структура файлов](#7-структура-файлов)
8. [Настройка плат](#8-настройка-плат)
9. [Инструкции по прошивке](#9-инструкции-по-прошивке)
10. [Статус реализации](#10-статус-реализации)
11. [История изменений](#11-история-изменений)

---

## 1. Структура проекта

```
STAR1_Ambient/
├── Core/                    # STM32 HAL и системные файлы
│   ├── Inc/
│   └── Src/
│
├── Lightning/               # Система освещения
│   ├── director.h/c         # Orchestration layer: выбор сцен и переходов
│   ├── types.h/c            # Базовые типы данных
│   ├── driver.h/c           # Драйвер WS2812B (TIM2 PWM + DMA)
│   ├── event_layer.h/c     # Временные сценарии поверх базовой темы
│   ├── palette.h/c          # Цветовые палитры
│   ├── effects.h/c          # Визуальные эффекты
│   ├── runtime_state.h/c    # Нормализация runtime-состояния для director/render
│   ├── zones.h/c            # Управление зонами
│   ├── base_scene.h/c     # Машина состояний сцен
│   ├── themes.h/c          # Темы освещения
│   ├── frame_utils.h/c      # Утилиты работы с кадрами
│   └── features.h           # Флаги функций и конфигурация
│                             # (watchdog, sleep mode, эффекты, CAN timing)
│
├── Boards/                  # Конфигурации плат
│   ├── board_common.h       # Общие макросы и утилиты (DMA alignment)
│   ├── board_led_backend.h/c# Общий backend для init/render/dma всех board-линий
│   ├── board_dispatch.h/c   # Единый dispatch API по активному BOARD_TYPE
│   ├── board_selected.h     # Единая точка выбора активной платы для всего build
│   ├── board_zone_map.c     # Strong g_zone_map binding для выбранной платы
│   ├── board_door_fl.h/c    # Front-Left дверь
│   ├── board_door_fr.h/c    # Front-Right дверь
│   ├── board_door_rl.h/c    # Rear-Left дверь
│   ├── board_door_rr.h/c    # Rear-Right дверь
│   ├── board_dashboard.h/c  # Панель приборов
│   └── board_rear.h/c       # Задняя подсветка
│
├── CAN/                     # CAN-коммуникация
│   ├── ambient_state.h      # Общие runtime-типы CAN/ambient
│   ├── ambient.h/c          # CAN-протокол, parsing и runtime snapshot
│   ├── ambient_internal.h   # Внутренний state (только для CAN/*.c)
│   ├── ambient_power.h/c    # Sleep/awake policy и power diagnostics
│   ├── ambient_rx.h/c       # RX parsing OEM/master/sync/BSM и derived CAN state
│   ├── ambient_role.h/c     # Discovery, heartbeat и master/slave role policy
│   ├── ambient_theme.h/c    # OEM color -> theme bank policy и deferred flash save
│   ├── ambient_tx.h/c       # Сборка и отправка master/sync/discovery/test пакетов
│   └── flash_storage.h/c    # Хранение настроек в Flash
│
├── docs/                    # Краткие документы архитектуры/рефактора
│   ├── architecture.md
│   └── refactor-log.md
│
└── main.c                   # Точка входа
```

Дополнительно в `Core` вынесены runtime-модули:
- `Core/Inc/led_runtime.h` + `Core/Src/led_runtime.c` — нейтральный LED façade для orchestration-слоя (`Core/CAN/Boards`) поверх WS2812 driver API
- `Core/Inc/runtime_can.h` + `Core/Src/runtime_can.c` — CAN TX scheduling (master/sync/discovery cadence)
- `Core/Inc/runtime_flow.h` + `Core/Src/runtime_flow.c` — runtime state machine (WAIT_OEM/ACTIVE/SLEEP/STOP), active loop pipeline и sleep/wake orchestration
- `Core/Inc/runtime_render.h` + `Core/Src/runtime_render.c` — black-frame reset и frame postprocess
- `Core/Inc/runtime_stop.h` + `Core/Src/runtime_stop.c` — STOP/wakeup loop и ISR wakeup bridge

---

## 2. Архитектура системы освещения

### 2.0. Правило именования

- Без префикса идут проектные доменные сущности: `theme_id_t`, `zone_id_t`, `director_t`, `event_scene_t`, `BASE_SCENE_*`
- `ws_*` остаётся в низкоуровневом WS2812/render слое: драйвер, framebuffer, palette sampling, вывод пикселей
- orchestration-слой (`Core/CAN/Boards dispatch` + `Lightning/base_scene|zones|event_layer`) работает через `led_runtime_*` façade и `led_runtime_strip_t`, без прямых вызовов `ws_*`
- theme/orchestration в `Lightning` использует нейтральные alias (`palette_*`, `effect_*`), while raw `ws_*` APIs остаются в low-level `effects/palette/driver`
- `can_*` используется для runtime-типов CAN (`can_state_t`, `can_bsm_state_t`, `can_power_diag_t`), а `AMB_*` остаётся для compile-time конфигов в `features.h`

### 2.1. Зоны

Каждый блок содержит до 4 зон:

| Zone ID | Описание |
|---------|----------|
| `ZONE_STRIP` | Основная длинная линия |
| `ZONE_HANDLE` | Подсветка ручки |
| `ZONE_STORAGE` | Ниша/отделение |
| `ZONE_FOOTWELL` | Подсветка ног |

Каждая зона имеет:
- Свою длину LED
- Отдельный цвет/палитру
- Своё поведение в intro/outro
- Свой brightness scale

### 2.2. Система палитр

Палитры состоят из цветовых стопов (color stops):

```c
typedef struct {
    float pos;      // 0.0 .. 1.0
    uint8_t r, g, b;
} palette_stop_t;
```

Палитры используются в темах и эффектах для создания градиентов.

**Доступные палитры (12, W223/EQS):**
- Champagne Arc, Amber Sunset, Copper Rose, Burgundy Velvet
- Emerald Veil, Aurora Glacier, Sapphire Ice, Night Opal
- Glacier Mist, Silver Silk, Platinum Cloud, Pearl Blush

### 2.3. Эффекты (FX Engine)

**Однократные эффекты (oneshot):**
- `FX_WELCOME` / `FX_WELCOME_LUXE` - приветственная анимация
- `FX_GOODBYE` / `FX_GOODBYE_LUXE` - анимация выключения

**Базовые эффекты (scene effects):**
- `FX_GRADIENT_FLOW` - плавный градиент
- `FX_TWIN_WAVE` / `FX_TWO_TONE_WAVE` - двухтонная волна
- `FX_OCEAN_FLOW` - океанский поток (плавное сглаживание)
- `FX_VELVET_FLOW` - бархатный поток (премиальная плавность)
- `FX_ENERGIZE_PULSE` - энергетический пульс
- `FX_GENTLE_PULSE` - мягкий пульс (элегантная пульсация)
- `FX_DUAL_ZONE` - двойная зона
- `FX_SOLID_GRADIENT` - мягкий статичный цвет
- `FX_SOFT_BREATHE` - мягкое дыхание

**W223/EQS Премиум эффекты:**
- `FX_AURORA` - северное сияние (несколько синусоид с разными фазами)
- `FX_CASCADE` - каскад (волна яркости, падающая по ленте)
- `FX_SPARKLE` - мерцание (редкие вспышки отдельных LED)
- `FX_BREATHE_WAVE` - волна дыхания (движущаяся волна яркости)
- `FX_COLOR_MORPH` - морфинг цвета (плавное перетекание между зонами палитры)

**Особенности W223/EQS эффектов:**
- Улучшенные easing функции (smoothstep, ease-in-out) для премиальной плавности
- Очень низкие скорости по умолчанию для элегантности
- Уникальные характеры: магический, медитативный, праздничный

### 2.4. Base Scene (машина состояний)

**Состояния:**

```
BASE_SCENE_IDLE → BASE_SCENE_INTRO → BASE_SCENE_BRIDGE → BASE_SCENE_ACTIVE → BASE_SCENE_OUTRO → BASE_SCENE_IDLE
                                       ↑_____↓
                                    (crossfade)
```

- **BASE_SCENE_IDLE** - бездействие
- **BASE_SCENE_INTRO** - вступительная анимация (oneshot)
- **BASE_SCENE_BRIDGE** - плавный переход intro→scene (400мс, quintic smoothstep)
- **BASE_SCENE_ACTIVE** - активная сцена (непрерывная) + опциональный кроссфейд тем
- **BASE_SCENE_OUTRO** - завершающая анимация (oneshot → idle)

**Функции Player:**
- Управление яркостью (`calc_brightness`)
- Night mode
- Плавные переходы всех зон
- Смена тем
- **Авто-ротация тем** с плавным кроссфейдом (без intro/outro)

### 2.5. Слои orchestration

Начиная с текущей архитектуры, логика системы разделена на 4 слоя:

1. **CAN/Input layer**
   - `CAN/ambient.c`
   - Разбирает OEM/master/sync/BSM пакеты
   - Хранит синхронизированное состояние и отдает snapshot вверх

1.1. **Role/Discovery policy**
   - `CAN/ambient_role.c`
   - Держит discovery-состояние плат
   - Обрабатывает heartbeat/failover
   - Решает master/slave роль независимо от packet parsing

1.15. **Power/Sleep policy**
   - `CAN/ambient_power.c`
   - Держит sleep/awake state
   - Управляет idle timeout и power diagnostics
   - Изолирует low-power policy для transceiver и LED rail

1.16. **RX parsing**
   - `CAN/ambient_rx.c`
   - Разбирает OEM/master/sync/BSM пакеты
   - Поддерживает derived CAN state: `NSI`, `BSM`, `motion_profile`, `oem_received`
   - Оставляет `ambient.c` только как coordinator и public facade

1.2. **Theme/Persistence policy**
   - `CAN/ambient_theme.c`
   - Разрешает `OEM color -> theme`
   - Ведёт циклические индексы банков
   - Выполняет deferred flash save с защитой от лишнего износа

1.3. **TX builders**
   - `CAN/ambient_tx.c`
   - Собирает master/sync/discovery пакеты
   - Содержит общий HAL TX helper
   - Используется и для debug/test send path

2. **Runtime state**
   - `Lightning/runtime_state.c`
   - Нормализует raw CAN-данные в состояние для рендера:
   - выбранная тема банка
   - dimming-кривая и сглаживание яркости
   - признаки `same bank`, `theme change requires outro`

3. **Director**
   - `Lightning/director.c`
   - Решает, когда запускать intro/outro, когда можно переключить тему внутри банка напрямую, и что отдавать в `base_scene`
   - Это точка расширения для будущих сценариев event-layer и overlays

4. **Transient Event Layer**
   - `Lightning/event_layer.c`
   - Временные сценарии поверх базовой темы, но ниже warning overlays
   - Предназначен для `dual accent`, `turn cue`, `welcome accent`, `charge pulse`

5. **Renderer**
   - `base_scene + zones + effects + driver`
   - Рисует базовую тему, переходы и overlays
   - `Core/runtime_render.c` добавляет runtime postprocess: tint, boost, frame slew и deterministic black-frame reset

6. **STOP/Wakeup Runtime**
   - `Core/runtime_stop.c`
   - Держит wakeup flags вне `main.c`
   - Инкапсулирует STOP enter/wait loop
   - Получает wakeup сигнал из ISR через `runtime_stop_signal_wakeup()`

7. **Flow Runtime**
   - `Core/runtime_flow.c`
   - Держит runtime state machine (`RUNTIME_WAIT_OEM`, `RUNTIME_ACTIVE`, `RUNTIME_SLEEP_PREP`, `RUNTIME_STOP`, `RUNTIME_WAKE_RECOVER`)
   - Инкапсулирует переходы boot/wake/sleep и активный pipeline (director/base_scene/render/CAN TX)
   - `main.c` теперь в основном HAL entrypoint + wiring runtime контекста

8. **CAN State Boundary**
   - `g_can_state` больше не экспортируется через публичный `ambient.h`
   - Верхние слои (`Core/*`, `Lightning/*`) работают только через `can_ambient_*` API
   - Внутренний глобальный state доступен только внутри CAN через `ambient_internal.h`

9. **LED Runtime Boundary**
   - `Core/led_runtime.c` предоставляет нейтральный runtime API (`led_runtime_*`) для power/brightness/DMA ISR bridge и pixel/buffer операций (`set/get/copy RGB`)
   - `Core/runtime_flow.c`, `Core/runtime_render.c`, `CAN/ambient_power.c`, `Boards/board_dispatch.c`, `Lightning/base_scene.c`, `Lightning/zones.c`, `Lightning/event_layer.c` используют façade вместо прямых `ws_*`
   - Низкоуровневые `ws_*` остаются в `Core/led_runtime.c`, `Boards/board_led_backend.c`, `Lightning/driver.*`, `Lightning/effects.*`, `Lightning/palette.*`
   - `zone_map_t` теперь хранит `led_runtime_strip_t *strip` (вместо `ws2812_t *ws`), чтобы слой зон не зависел напрямую от driver-типа

10. **Board Runtime Boundary**
   - `Boards/board_dispatch.c` теперь compiled API (не header-only static inline)
   - `Boards/board_led_backend.c` убирает дублирование init/render/DMA циклов между board-файлами
   - `Boards/board_zone_map.c` держит strong `g_zone_map`, чтобы `main.c` не тянул `ws2812_t`/board internals

Такое разделение нужно для будущих фич уровня W222/W223/EQS:
- временные сценарии event-layer
- приоритетные overlays
- реакции на поворот/двери/заряд/температуру

### 2.6. Переключение тем

Система поддерживает два механизма переключения тем внутри банка:

#### 2.6.1. Циклическое переключение (при смене OEM цвета)

**Работает ВСЕГДА**, независимо от флага авто-ротации.

При каждой смене OEM цвета через CAN выбирается **следующая** тема в банке:

```
AMBER 1st → WHITE 1st → AMBER 2nd → WHITE 2nd → BLUE 1st → WHITE 3rd → ...
```

**Особенности:**
- Каждый банк (AMBER, BLUE, WHITE) помнит свой индекс
- Индексы сохраняются в Flash и восстанавливаются после перезагрузки
- При достижении конца банка цикл начинается сначала
- Переход внутри банка происходит напрямую (без outro/intro)
- Переход между разными банками:
  - при `AMB_ENABLE_FAST_BANK_SWITCH=1` - прямой screen-crossfade в `BASE_SCENE_ACTIVE`
  - при `AMB_ENABLE_FAST_BANK_SWITCH=0` - legacy путь через outro/intro

#### 2.6.2. Авто-ротация по таймеру (опционально)

При включённом флаге `AMB_ENABLE_AUTO_ROTATE` система **дополнительно** автоматически переключает темы по таймеру с плавным кроссфейдом.

**Настройка в `features.h`:**
```c
#define AMB_ENABLE_AUTO_ROTATE         0     // 1=вкл, 0=выкл (по умолчанию: выкл)
#define AMB_AUTO_ROTATE_INTERVAL_SEC   10u   // интервал смены (секунды)
#define AMB_CROSSFADE_DURATION_MS      1600u // длительность кроссфейда (мс)
#define AMB_ENABLE_FAST_BANK_SWITCH    1u    // быстрый межбанковый switch в ACTIVE
#define AMB_FAST_BANK_SWITCH_DEBOUNCE_MS 280u// анти-дребезг для частых bank change
```

**Как работает кроссфейд:**
1. По истечении интервала выбирается следующая тема из банка
2. Параллельно рендерятся текущая и следующая темы
3. Результаты смешиваются со screen blend, чтобы не терять яркость при разных фазах волн
4. После завершения кроссфейда текущая тема заменяется на следующую

**Особенности:**
- Авто-ротация активируется автоматически при включённом флаге
- Использует глобальную переменную `g_oem_color` для выбора банка
- Работает только в состоянии `BASE_SCENE_ACTIVE`
- Опциональные функции для ручного управления:
  - `base_scene_set_auto_rotate(pl, enable)` - вкл/выкл в рантайме
  - `base_scene_trigger_next_theme(pl)` - принудительный кроссфейд

**Разница между механизмами:**

| Механизм | Триггер | Анимация | Флаг |
|----------|---------|----------|------|
| Циклическое | Смена OEM цвета | Direct (same-bank) / screen-crossfade (cross-bank, fast-path) | Всегда вкл |
| Авто-ротация | По таймеру | Screen blend кроссфейд | `AMB_ENABLE_AUTO_ROTATE` |

### 2.6.3. Dynamic Profiles (W223/EQS-like motion)

Для премиального ощущения каждая тема имеет собственный темп анимации:
- Базовая скорость задаётся типом `fx_main`
- Применяется индивидуальный масштаб конкретной темы
- Профили скорости хранятся централизованно в `Lightning/motion_profiles.h`

Итог: темы отличаются не только палитрой, но и характером движения.

| Theme | Motion scale |
|------|--------------|
| AMBER_CHAMPAGNE_ARC | 0.92 |
| AMBER_SUNSET | 0.98 |
| AMBER_COPPER_ROSE | 1.04 |
| AMBER_BURGUNDY_VELVET | 0.88 |
| BLUE_AURORA_GLACIER | 0.86 |
| BLUE_SAPPHIRE_ICE | 1.06 |
| BLUE_NIGHT_OPAL | 0.90 |
| BLUE_EMERALD_STREAM | 1.08 |
| WHITE_PLATINUM_CLOUD | 0.84 |
| WHITE_SILVER_SILK | 1.00 |
| WHITE_PEARL_BLUSH | 0.88 |
| WHITE_GLACIER_MIST | 0.94 |

### 2.6.4. Theme Personality + Cabin Depth + Bank Memory

Добавлены три связанных слоя для более «живого» премиального поведения:

1. **Theme Personality**
   - У каждой темы есть не только палитра, но и собственный motion-signature:
     - `speed` (динамика),
     - `phase` (фазовый seed),
     - `depth` (глубина пространственного параллакса).
   - Профили хранятся в `Lightning/motion_profiles.h` (`WS_THEME_PERSONALITY_*_INIT`).

2. **Cabin Depth Model (pseudo-3D)**
   - Для зон учитывается положение платы в салоне: `front/rear` и `left/right`.
   - На этой основе добавляется небольшой palette/phase/brightness bias.
   - Для `BOARD_TYPE_REAR` используется **комбинированная rear-позиция** (`RL+RR`, центр по L/R и rear по F/R).

3. **Bank Character Memory**
   - Для каждого OEM-банка (AMBER/BLUE/WHITE) запоминается «характер» движения (скоростной scale).
   - При возврате в банк восстанавливается его предыдущая динамика.
   - Memory обновляется плавно в runtime (учитывает motion profile + fan/night context).

Основные compile-time тюнинги:
```c
#define AMB_ENABLE_THEME_PERSONALITY      1u
#define AMB_ENABLE_CABIN_DEPTH_MODEL      1u
#define AMB_CABIN_DEPTH_LR_GAIN           0.85f
#define AMB_CABIN_DEPTH_FR_GAIN           1.00f
#define AMB_CABIN_DEPTH_U_SPREAD          0.10f
#define AMB_CABIN_DEPTH_PHASE_SHIFT       1.35f
#define AMB_CABIN_DEPTH_BRIGHTNESS_DELTA  0.08f

#define AMB_ENABLE_BANK_CHARACTER_MEMORY  1u
#define AMB_BANK_CHARACTER_LERP_ALPHA     0.78f
#define AMB_BANK_CHARACTER_SPEED_MIN      0.84f
#define AMB_BANK_CHARACTER_SPEED_MAX      1.22f
```

---

## 3. Драйвер WS2812B

### Технические характеристики

- **Метод передачи:** TIM2 PWM (4 канала) + DMA
- **Частота:** 800 kHz
- **Формат данных:** RGB (Red, Green, Blue)
- **Количество зон:** до 4 независимых каналов

### Использование

1. Инициализация: `ws_init()` с указанием буферов
2. Установка пикселей: `ws_set_pixel_rgb()`
3. Отправка кадра: `ws_render()`
4. Обработка DMA: `ws_dma_tc_isr()` в `HAL_TIM_PWM_PulseFinishedCallback()`

**Примечание:** DMA-буферы должны иметь размер: `(led_count * 3 * 8) + WS_RESET_SLOTS` слов

### Управление питанием

- `LED_PWR_EN` включает общий buck для ленты
- Переключатели каналов `CH1_EN..CH4_EN` подают питание на каждый из четырёх выходов (например, PA6–PA9)
- `ws_power_set()` поднимает LED_PWR_EN, затем активные CHx_EN и в конце снимает `LED_DATA_OE`; выключение идёт в обратном порядке
- Активные каналы собираются автоматически в `ws_init()` (учитываются только линии с `led_count > 0`); при необходимости маску можно задать вручную через `ws_power_set_channel_mask(mask)`

**Цветопередача и яркость:**
- Включены гамма 2.2 и темпоральный дезеринг (`AMB_ENABLE_GAMMA`, `AMB_ENABLE_DITHERING`) для плавных градиентов
- Глобальная яркость берётся из base_scene (slew-контур + принудительное затемнение для sleep/outro)

---

## 4. CAN-система

### 4.1. Общая архитектура

Система состоит из нескольких блоков (FL, FR, RL, RR, Dashboard, Rear), подключенных к одной CAN-шине автомобиля.

**Роли:**
- **MASTER** - один блок (автоматически определяется), читает CAN от машины, управляет системой
- **SLAVE** - остальные блоки, принимают команды от master и синхронизируют состояние

### 4.2. Определение мастера

✅ **Реализовано:** Все блоки используют одну прошивку. Master определяется автоматически.

**Алгоритм:**

0. **Старт протокола:**
   - При включении все платы ждут первый OEM пакет `0x325`
   - Пока OEM пакет не получен, платы **не отправляют** свои пакеты в шину
   - Если в течение `AMB_CAN_ACTIVE_TIMEOUT_MS` нет входящих CAN пакетов, TX приостанавливается

1. **Discovery механизм:**
   - После получения OEM запускается discovery-окно
   - В течение `AMB_CAN_STARTUP_DISCOVERY_MS` отправляются только discovery-пакеты
   - Затем discovery продолжается фоново каждые 1500 мс
   - Пакет содержит: BOARD_TYPE, unique_id, текущая роль

2. **Приоритеты плат:**
   ```
   FL > FR > DASHBOARD > RL > RR > REAR
   ```
   Master выбирается автоматически по приоритету.

3. **Heartbeat механизм:**
   - Master начинает отправку sync после discovery-окна
   - Sync-пакет каждые 500 мс с текущим bank/theme_index
   - Если master не отвечает > 1.8 сек → автоматический failover

4. **Отказоустойчивость:**
   - При отказе master, slave с наивысшим приоритетом становится новым master
   - При восстановлении, роль автоматически возвращается к платформе с более высоким приоритетом
   - Система продолжает работать даже при отказе одной из плат

### 4.3. CAN ID

Используется 6 ключевых CAN ID:

| ID | Описание | Отправитель |
|----|----------|-------------|
| `0x025` | `LGTSENS_A1` (day/night от датчика света) | Машина → Все |
| `0x20B` | `HVAC_A2` (`HVAC_Temp_FL_Stat/FR_Stat`, передний климат) | Машина → Все |
| `0x0BC` | `HVAC_A4` (`HVAC_Temp_RL_Stat/RR_Stat`, задний климат) | Машина → Все |
| `0x339` | `HVAC_A5` (`HVAC_Fan_Curr`, интенсивность вентилятора) | Машина → Все |
| `0x369` / `0x350` | Seat heating status (`SeatHt_*_Stat`) | Машина → Все |
| `0x325` | OEM ambient от IC (яркость, цвет) | Машина → Все |
| `0x353` | Unified Master Protocol | Master/Slaves |

**Типы пакетов 0x353 (различаются старшими 4 битами Byte0):**

| Тип пакета | Byte0 (старшие 4 бита) | Описание |
|------------|------------------------|----------|
| `PKT_TYPE_DISCOVERY` | `0x00` | Discovery пакет (от всех плат) |
| `PKT_TYPE_MASTER` | `0x10` | Master пакет (полное состояние) |
| `PKT_TYPE_SYNC` | `0x20` | Sync/Heartbeat пакет |

### 4.4. Форматы пакетов

#### Discovery Packet (PKT_TYPE_DISCOVERY)

**ID:** `0x353`, **DLC:** 8

| Byte | Описание |
|------|----------|
| 0 | `0x00 \| BOARD_TYPE` (0-5: FL,FR,RL,RR,DASHBOARD,REAR) |
| 1-4 | `unique_id` (уникальный идентификатор платы) |
| 5 | Текущая роль (0=slave, 1=master) |
| 6-7 | Reserved |

**Периодичность:** Все платы каждые 1500 мс (только после OEM пакета)

---

#### Master Packet (PKT_TYPE_MASTER)

**ID:** `0x353`, **DLC:** 8

| Byte | Описание |
|------|----------|
| 0 | `0x10 \| flags` (`bit0 = night_mode`) |
| 1 | `bank_id` (0=amber, 1=blue, 2=white) |
| 2 | `theme_index` (индекс темы в банке) |
| 3 | `oem_color` (0=Amber, 1=Blue, 2=White) |
| 4 | `brightness_raw` (0..5) |
| 5 | `motion_profile` (0=Luxury, 1=Calm, 2=Sport) |
| 6-7 | Reserved |

**Периодичность:** Master каждые 200 мс после discovery-окна

**Flags (`byte0`, low nibble):**
- `MASTER_FLAG_NIGHT` (`0x01`) - ночной режим активен

---

#### Sync Packet (PKT_TYPE_SYNC)

**ID:** `0x353`, **DLC:** 8

| Byte | Описание |
|------|----------|
| 0 | `0x20 \| bank_id` |
| 1 | `theme_index` |
| 2-7 | Reserved |

**Периодичность:** Master каждые 500 мс после discovery-окна

**Назначение:** Heartbeat для отслеживания работоспособности master

---

#### OEM Packet

**ID:** `0x325`, **DLC:** 6

| Byte | Bits | Описание |
|------|------|----------|
| 0 | 0-7 | `AmbBrt_Rq` - яркость (0-5, фактически приходит целым байтом) |
| 1-2 | - | Reserved |
| 3 | 2-3 | `Amblgt_Col_Rg` - цвет (0=Amber/Solar, 1=White/Neutral, 2=Blue/Polar, 3=Reserved) |
| 3 | 0-1, 4-7 | Reserved |
| 4-7 | - | Reserved |

**Отправитель:** Машина (IC) → Все платы
**Обработка:** Только master платой

**Extended режим:** удалён. Пакет 0x325 используется только для OEM цвета и яркости (циклический выбор тем по банку).

---

#### Light Sensor Packet

**ID:** `0x025`, **DLC:** 8

| Byte | Bits | Описание |
|------|------|----------|
| 0 | bit1 | `LgtSens_Night` |
| 1 | bit5 | `LgtSens_Night2` |

**Отправитель:** Машина (SAM_F/LGTSENS) → Все платы  
**Обработка:** Master использует как основной источник `night_mode`; если `0x025` отсутствует/устарел, применяется fallback от NSI признаков в `0x325`.

---

#### HVAC Temperature Packets

**ID:** `0x20B` (`HVAC_A2`), **DLC:** 8  
**ID:** `0x0BC` (`HVAC_A4`), **DLC:** 8

| ID | Byte | Сигнал |
|----|------|--------|
| `0x20B` | 0 | `HVAC_Temp_FL_Stat` |
| `0x20B` | 1 | `HVAC_Temp_FR_Stat` |
| `0x0BC` | 0 | `HVAC_Temp_RL_Stat` |
| `0x0BC` | 1 | `HVAC_Temp_RR_Stat` |

**Обработка:** при изменении setpoint вверх/вниз запускается краткий event-layer pulse (W223-style):  
- `temp up` -> тёплый акцент (amber/orange)  
- `temp down` -> холодный акцент (cool blue)  
Срабатывание ограничено cooldown, чтобы не спамить анимацией.

Сайд-маршрутизация по платам:
- Левые платы (`BOARD_TYPE_FL`, `BOARD_TYPE_RL`) реагируют только на левую сторону (`FL/RL`).
- Правые платы (`BOARD_TYPE_FR`, `BOARD_TYPE_RR`) реагируют только на правую сторону (`FR/RR`).
- Эффект применяется только на `ZONE_STRIP` (main strip), без окраски остальных зон.
- Для `temp up/down` включён `drag trail` (движущийся warm/cool след по strip, W223-style).
  - Door-aware localization: на `FL/FR` направление trails “от переда к центру”, на `RL/RR` инвертировано по геометрии двери.
  - Progressive burst: если температура меняется серией, trail автоматически идёт несколькими короткими проходами.

HVAC runtime дополняется:
- `dual-zone split`: при расхождении левой/правой температуры на strip добавляется очень мягкий persistent warm/cool bias.
- `fan intensity modulation`: по `HVAC_Fan_Curr` изменяется «живость» динамики (через postprocess frame slew).
- `comfort auto-dim`: ночью при активном HVAC дополнительно приглушаются `footwell/storage` для комфорта глаз.
- `energy-aware trim`: postprocess подстраивает saturation/white point по motion profile и night mode.
- `cabin gradient memory`: после `temp up/down` остаётся мягкий тёплый/холодный след с плавным затуханием.
- `seat-heater coupling`: при `SeatHt_*_Stat` добавляется локальный warm overlay (strip+footwell+storage) только на соответствующей стороне/плате.

### 4.6. Motion Profiles (Luxury/Calm/Sport)

- Runtime-профиль динамики сцены:
  - `0` = `Luxury` (базовая скорость, плавно)
  - `1` = `Calm` (замедленный характер)
  - `2` = `Sport` (более активная динамика)
- Master синхронизирует профиль на все slave через `0x353`, `byte[5]`.
- Дополнительно в рантайме применяется постобработка кадра:
  - лёгкий температурный сдвиг палитры по профилю (`AMB_MOTION_PROFILE_TINT_*`)
  - короткий акцент яркости при смене профиля/drive mode (`AMB_DRIVE_MODE_BOOST_*`)
  - ограничение шага RGB между кадрами для снижения «ступенек» (`AMB_FRAME_SLEW_*`)
  - Night Calm: в night mode скорость motion FX дополнительно снижается (`AMB_NIGHT_MOTION_SPEED_SCALE`)

Автопереключение от CAN режима движения:
- Включается макросом `AMB_ENABLE_DRIVE_MODE_AUTOPROFILE`.
- Источник задаётся в `features.h`:
  - `AMB_DRIVE_MODE_CAN_ID`
  - `AMB_DRIVE_MODE_BYTE_INDEX`
  - `AMB_DRIVE_MODE_BIT_SHIFT`
  - `AMB_DRIVE_MODE_BIT_MASK`
  - `AMB_DRIVE_MODE_VALUE_COMFORT / _CALM / _SPORT`
- По умолчанию уже выставлен пресет для 212: `AMB_DRIVE_MODE_CAN_ID=0x38E` (`BO_910`).
 - В текущем пресете по DBC 212 используется:
   - `BO_910 SPP_STAT_R1`, `SG SPP_DrvProg_Rq (10|3)`
   - маппинг: `COMFORT/INDIVIDUAL -> Luxury`, `ECO/SLEEK -> Calm`, `SPORT/SPORTPLUS -> Sport`.

### 4.5. Сохранение настроек в Flash

**Расположение:** Последняя страница Flash (адрес `0x0801F800`, 2KB)

**Сохраняемые настройки:**
- `bank_id` - выбранный банк тем (0..2)
- `theme_index` - индекс темы в банке
- `last_oem_color` - последний OEM цвет (для циклической ротации)
- `oem_theme_indices[3]` - циклические индексы тем для каждого OEM банка

**Механизм:**
1. Загрузка при инициализации (`can_ambient_init()`)
2. Сохранение через 10 секунд после последнего изменения (и только если данные реально изменились)
3. Сохранение только на master плате
4. Защита: Magic number (`0x53544152` = "STAR") + CRC32

**Примечания:**
- Flash имеет ограниченное количество циклов записи (~10,000)
- При повреждении данных используются значения по умолчанию
- Индексы тем сохраняются для продолжения ротации после перезагрузки

---

## 5. Поток данных

### Инициализация

1. Все платы стартуют и отправляют discovery-пакеты
2. Плата с наивысшим приоритетом (FL) становится master
3. Master начинает отправлять sync-пакеты каждые 500 мс

### Нормальная работа

**Master:**
- Читает CAN от машины (0x325)
- Вычисляет bank, theme_index, brightness
- Отправляет master/sync/discovery пакеты (0x353)
- Сохраняет настройки в Flash

**Slave:**
- Читает master/sync пакеты от master
- Применяет bank/theme/brightness из пакетов
- Отправляет discovery-пакеты каждые 1500 мс

**Lighting engine:**
- `CAN/ambient` отдает snapshot входов
- `runtime_state` нормализует яркость/тему/флаги переходов
- `director` применяет policy переключений
- `event_layer` может временно переопределять отдельные зоны
- по умолчанию уже активирован `welcome dual-accent` после полного intro-перехода в scene
- `base_scene + zones` выполняют intro/scene/outro на всех платах синхронно

### Failover (отказоустойчивость)

1. Master перестает отвечать (нет sync пакетов > 1.8 сек)
2. Slave с наивысшим приоритетом обнаруживает timeout
3. Slave автоматически переходит в master режим
4. Новый master начинает отправлять пакеты
5. При восстановлении старого master, роль возвращается автоматически

---

## 6. Работа системы

### OEM режим (машина управляет)

1. Автомобиль отправляет цвет/яркость (0x325)
2. Master читает 0x325 и выбирает тему для банка (с циклической ротацией)
3. Master отправляет 0x353 (PKT_TYPE_MASTER) каждые 200 мс
4. Master отправляет 0x353 (PKT_TYPE_SYNC) каждые 500 мс
5. Все блоки синхронизируются и применяют одинаковую тему

**Яркость OEM (0..5):**
- raw OEM уровень переводится в `0.0..1.0`
- далее используется power-кривая (`AMB_BRIGHTNESS_EXP`) с floor/ceil из `features.h`
- поверх применяется двухступенчатое сглаживание и удержание уровня во время `outro`

**Циклическое переключение тем:**

При каждой смене OEM цвета выбирается **следующая** тема в банке (не первая):

```
AMBER 1st → WHITE 1st → AMBER 2nd → WHITE 2nd → BLUE 1st → WHITE 3rd → ...
```

- Каждый банк помнит свой индекс темы
- Индексы сохраняются в Flash и восстанавливаются после перезагрузки
- При достижении конца банка начинается с начала (циклически)

### Демо режим (DEMO_MODE == 1)

**Примечание:** Демо режим работает только на master плате и предназначен для тестирования.

**Особенности:**
- Автоматическое переключение тем каждые 20 секунд
- Использует темы из банка, соответствующего текущему OEM цвету
- Синхронизируется с яркостью и OEM цветом из CAN (0x325)
- Тема в демо режиме не перезаписывается CAN системой
- При переходе платы в master статус таймер переключения сбрасывается
- При потере master статуса демо режим автоматически останавливается

**Порядок работы:**
1. Плата становится master (автоматически или вручную)
2. Таймер переключения темы инициализируется (сбрасывается)
3. Каждые 20 секунд автоматически переключается следующая тема из банка
4. При смене OEM цвета (Amber/Blue/White) через CAN банк тем обновляется
5. Яркость и ночной режим синхронизируются с CAN состоянием

**Настройка:**
- Демо режим включается/выключается через `#define DEMO_MODE` в `main.h`
- По умолчанию: `DEMO_MODE = 0` (выключен)
- Для стенда установите: `DEMO_MODE = 1` (автопереключение каждые 20 секунд)

---

## 7. Структура файлов

### CAN/

- **ambient_state.h** - общие типы состояния, которыми пользуются CAN, director и renderer

- **ambient.h/c** - CAN-протокол и логика master/slave
  - Инициализация CAN с фильтрами
  - Обработка всех типов пакетов
  - Автоматическое определение master/slave
  - Отказоустойчивость и heartbeat
  - Формирование `runtime_inputs` snapshot для director

- **flash_storage.h/c** - Хранение настроек в Flash
  - Загрузка/сохранение настроек
  - Проверка целостности (CRC32)

### Lightning/

- **director.h/c** - orchestration слой поверх base-scene
  - Обрабатывает смены темы между банками
  - Управляет pending-theme и intro/outro policy
  - Является точкой расширения для будущих сценариев event-layer

- **runtime_state.h/c** - нормализация runtime состояния
  - Сборка производного состояния из CAN snapshot
  - Dimming curve + smoothing
  - Флаги переходов для director

- **event_layer.h/c** - transient event layer
  - Отдельный слой между базовой сценой и warning overlays
  - Поддерживает timed overrides по зонам
  - Уже содержит builder-ы для `dual accent` и `directional cue`
  - Первый активный триггер: `welcome dual-accent` через director после `intro -> bridge -> scene`

- **base_scene.h/c** - базовая state machine сцен
  - Intro / bridge / scene / outro
  - Crossfade внутри scene
  - Базовая яркость темы и FX state

- **zones.h/c** - поканальное применение темы и overlays
  - Рендер зон кроме главной strip
  - Interrupt overlays (например BSM)

### Boards/

- **board_common.h** - Общие макросы и утилиты для всех board файлов
  - Макросы для объявления DMA буферов с правильным выравниванием (`__ALIGNED(4)`)
  - `BOARD_DMA_BUF_LEN()` - расчёт размера DMA буфера
  - Уменьшает дублирование кода между board файлами

Каждый board-профиль (`board_door_fl.h/c`, `board_dashboard.h/c`, и т.д.) содержит:
- Конфигурацию LED лент и зон
- Инициализацию/рендер/DMA линии через `board_led_backend`

Централизовано:
- `board_selected.h` выбирает активный профиль платы и задаёт `BOARD_TYPE` для всего проекта
- `board_zone_map.c` держит единственный strong `g_zone_map`

---

## 8. Настройка плат

### Определение BOARD_TYPE

Выберите нужный профиль в [board_selected.h](/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Boards/board_selected.h):

```c
/* #include "board_door_fl.h" */
/* #include "board_door_fr.h" */
/* #include "board_door_rl.h" */
/* #include "board_door_rr.h" */
/* #include "board_dashboard.h" */
#include "board_rear.h"
```

`board_selected.h` подключается через `ambient.h`, поэтому `BOARD_TYPE` единый для всех `.c` файлов проекта.

### Принудительная установка роли (опционально)

Определите в `features.h` или другом заголовочном файле:

```c
#define AMB_ROLE_MASTER  1  // Принудительный master
// или
#define AMB_ROLE_MASTER  0  // Принудительный slave
// Не определять = автоматическое определение (по умолчанию)
```

### Настройка демо режима

Демо режим настраивается в `main.h`:

```c
#define DEMO_MODE  0  // Выключен (по умолчанию, продакшн режим)
// или
#define DEMO_MODE  1  // Включен (демо стенд)
```

**Примечание:** Демо режим работает только на master плате. При переключении платы в slave статус демо режим автоматически останавливается.

### Настройка переключения тем

#### Циклическое переключение (всегда включено)

Циклическое переключение тем при смене OEM цвета работает **всегда** и не требует настройки.
Индексы тем автоматически сохраняются в Flash.

#### Авто-ротация по таймеру (опционально)

Настраивается в `features.h`:

```c
#define AMB_ENABLE_AUTO_ROTATE         0     // 1=вкл, 0=выкл (по умолчанию: выкл)
#define AMB_AUTO_ROTATE_INTERVAL_SEC   10u   // интервал смены темы (секунды)
#define AMB_CROSSFADE_DURATION_MS      1600u // длительность кроссфейда (мс)
```

**Особенности:**
- При включённом флаге авто-ротация активируется автоматически
- Темы переключаются внутри текущего банка (Amber/Blue/White)
- Переход между темами происходит с плавным кроссфейдом (screen blend)
- Банк определяется по текущему OEM цвету (`g_oem_color`)

**Примечание:** Циклическое переключение и авто-ротация работают независимо друг от друга. При выключенной авто-ротации циклическое переключение при смене OEM цвета продолжает работать.

### Настройка Sleep Mode

Sleep mode для минимизации энергопотребления при подключении к бортовой сети автомобиля:

```c
// В features.h:
#define AMB_ENABLE_SLEEP_MODE       1     // 1=вкл, 0=выкл
#define AMB_SLEEP_TIMEOUT_SEC       4u    // таймаут до засыпания (секунды, агрессивно для стенда)
#define AMB_SLEEP_FADE_OUT_MS    2000u    // длительность затухания перед сном (мс)
```

**Что происходит при засыпании:**
1. Плавное затухание яркости (2 сек)
2. Outro анимация
3. Отключение питания ленты (общий `LED_PWR_EN` + индивидуальные `CHx_EN` ключи)
4. CAN трансивер в standby (`FDCAN1_STBY_Pin`)
5. STM32 в STOP mode (минимальное потребление)

**Пробуждение:** Автоматически по любой CAN активности (EXTI на линии RX)

**Важно про watchdog + STOP:** если `AMB_ENABLE_WATCHDOG = 1` и Option Byte `IWDG Stop` оставлен в режиме Run, сторож продолжает тикать в STOP и перезагрузит MCU через таймаут (PB6 опустится). Для удержания STBY высокого уровня:
- Настройте `IWDG Stop = Freeze` (CubeProgrammer: OB → User Configuration → IWDG Stop → Frozen; в CubeMX — “IWDG in Stop mode” = Freeze, прошить OB).
- Для страхования «вечного сна» можно добавить периодический будильник (RTC/LPTIM) на выход из STOP.

**Примечание:** Sleep mode отключен в DEMO режиме (`DEMO_MODE = 1`)

### Настройка Watchdog (IWDG)

Independent Watchdog для защиты от зависаний системы:

```c
// В features.h:
#define AMB_ENABLE_WATCHDOG        1       // 1=вкл, 0=выкл
#define AMB_WATCHDOG_TIMEOUT_MS    2000u    // Таймаут watchdog (мс)
```

**Как работает:**
- Watchdog инициализируется при старте системы с таймаутом 2 секунды
- В конце каждой итерации главного цикла вызывается `HAL_IWDG_Refresh()`
- Если система зависнет и refresh не произойдёт в течение 2 секунд, MCU автоматически перезагрузится
- Защищает от бесконечных циклов, зависаний в обработчиках и deadlock'ов

---

## 9. Инструкции по прошивке

### Шаг 1: Выбор типа платы

Убедитесь, что в `board_selected.h` выбран правильный board-профиль (см. раздел 8).

### Шаг 2: Настройка демо режима (опционально)

По умолчанию демо режим выключен (`DEMO_MODE = 0`). Для стендового автопереключения тем задайте в `main.h`:

```c
#define DEMO_MODE  1  // в main.h
```

**Демо режим:**
- Работает только на master плате
- Автоматически переключает темы каждые 20 секунд
- Использует темы из банка, соответствующего текущему OEM цвету (Amber/Blue/White)
- Синхронизируется с яркостью и OEM цветом из CAN
- Тема в демо режиме не перезаписывается CAN системой
- При переходе платы в master статус таймер переключения сбрасывается
- Предназначен для тестирования и демонстрации возможностей системы

### Шаг 2.1: Выбор профиля BENCH/PRODUCTION

Профиль выбирается compile-time символом:
- `AMB_PROFILE=1` → `BENCH`
- `AMB_PROFILE=2` → `PRODUCTION`

Если не задан:
- `Debug` по умолчанию использует `BENCH`
- `Release` по умолчанию использует `PRODUCTION`

Как переключить через UI STM32CubeIDE:
1. `Project` -> `Properties`
2. `C/C++ Build` -> `Settings`
3. `MCU GCC Compiler` -> `Preprocessor`
4. В `Defined symbols` добавить:
   - `AMB_PROFILE=1` (bench) или
   - `AMB_PROFILE=2` (production)

Профильно-зависимые defaults:
- `AMB_CAN_WAKE_POLICY`: bench=`AMB_CAN_WAKE_BENCH`, production=`AMB_CAN_WAKE_PRODUCTION`
- `AMB_WATCHDOG_TIMEOUT_MS`: по умолчанию `8000` мс
- `AMB_DIMMING_ATTACK_MS/AMB_DIMMING_RELEASE_MS`: bench быстрее, production мягче
- `AMB_DIMMING_POST_SMOOTH`: bench ниже, production выше (плавнее)
- `AMB_TRANSITION_SMOOTH_ALPHA`: bench ниже (резвее), production выше (премиальнее)
- `AMB_HANDLE_PWM_ATTACK_MS/AMB_HANDLE_PWM_RELEASE_MS`: bench быстрее, production плавнее
- `AMB_GLOBAL_BRIGHTNESS_SMOOTH_ALPHA`: bench быстрее отклик, production мягче яркостные переходы
- `AMB_GLOBAL_BRIGHTNESS_TRANSITION_SMOOTH_ALPHA`: отдельное сглаживание для `BASE_SCENE_INTRO/BASE_SCENE_BRIDGE`
- `AMB_GLOBAL_BRIGHTNESS_OUTRO_SMOOTH_ALPHA`: отдельное сглаживание хвоста `BASE_SCENE_OUTRO`
- `AMB_IDLE_MICRO_MOTION_AMPLITUDE/AMB_IDLE_MICRO_MOTION_HZ`: очень слабое “живое” дыхание в `BASE_SCENE_ACTIVE`
- `AMB_BSM_ENV_ATTACK_ALPHA/AMB_BSM_ENV_RELEASE_ALPHA`: быстрый фронт и мягкий спад BSM overlay
- `AMB_MOTION_PROFILE_SCALE_LUXURY/CALM/SPORT`: множители динамики анимаций
- `AMB_ENABLE_DRIVE_MODE_AUTOPROFILE` + `AMB_DRIVE_MODE_*`: автопереключение профиля от CAN сигнала режима движения

### Шаг 3: Компиляция и прошивка

1. Откройте проект в STM32CubeIDE
2. Проверьте выбранный board-профиль в `board_selected.h`
3. Скомпилируйте проект (Build → Build Project)
4. Подключите плату через ST-Link
5. Прошейте плату (Run → Debug или Run → Run)

### Шаг 4: Проверка после прошивки

После прошивки плата должна:
1. Автоматически определить свою роль (master/slave)
2. Начать отправлять discovery-пакеты каждые 1500 мс
3. Если master: читать CAN (0x325), отправлять master/sync/discovery пакеты
4. Если slave: слушать пакеты от master и синхронизировать состояние

---

## 10. Статус реализации

### ✅ Реализовано

- ✅ Lighting system (base_scene, zones, effects, palette)
- ✅ Multi-zone controller через TIM2 PWM + DMA
- ✅ Theme definitions (банки тем)
- ✅ CAN-система с master/slave режимом
- ✅ Оптимизированный CAN-протокол (2 ID вместо 4)
- ✅ Unified Master Protocol (0x353) с типами пакетов
- ✅ Автоматическое определение master через discovery
- ✅ Отказоустойчивость (failover при отказе master)
- ✅ Heartbeat механизм (sync пакеты)
- ✅ Приоритеты плат (FL > FR > DASHBOARD > RL > RR > REAR)
- ✅ Синхронизация состояния между платами
- ✅ Универсальный main.c с поддержкой всех board типов
- ✅ Демо режим с флагом DEMO_MODE (автоматическое переключение тем каждые 20 сек)
- ✅ Сохранение настроек в Flash памяти
- ✅ Корректная работа демо режима на master плате без конфликтов с CAN
- ✅ **W223/EQS Premium эффекты** (Aurora, Cascade, Sparkle, Breathe Wave, Color Morph)
- ✅ **12 премиальных палитр W223/EQS** (Champagne Arc, Amber Sunset, Copper Rose, Burgundy Velvet, Emerald Veil, Aurora Glacier, Sapphire Ice, Night Opal, Glacier Mist, Silver Silk, Platinum Cloud, Pearl Blush)
- ✅ **Gamma 2.2 + temporal dithering** для плавных градиентов и корректной яркости
- ✅ **Easing функции** для премиальной плавности анимаций
- ✅ **Плавный переход intro→scene** через BASE_SCENE_BRIDGE (quintic smoothstep, 400мс)
- ✅ **Опциональная авто-ротация тем** с плавным кроссфейдом (без intro/outro)
- ✅ **Sleep Mode** - автоматический переход в STOP mode при отсутствии CAN активности
- ✅ **Independent Watchdog (IWDG)** - защита от зависаний системы
- ✅ **DMA Buffer Alignment** - оптимизация для производительности DMA
- ✅ **Flash Wear-out Protection** - запись раз в 10с и только при изменении (минимум циклов)
- ✅ **Thread-safe операции** - исправлены race conditions
- ✅ **Циклическое переключение тем** при смене OEM цвета (с сохранением в Flash)
- ✅ **Sleep Mode** - автоматический переход в режим сна при отсутствии CAN активности
- ✅ **Ожидание CAN при старте** - лента не включается до получения первого OEM пакета

### Особенности

- Все платы используют одинаковую прошивку
- Роль master/slave определяется автоматически
- Система продолжает работать при отказе одной из плат
- Автоматическое восстановление при восстановлении master
- Не требует ручной настройки или DIP-переключателей
- Минимальное использование CAN ID (только 2 ID: 0x325 и 0x353)
- Эффективное использование CAN-шины
- **W223/EQS-inspired** премиальные анимации с уникальными характерами
- **Плавность уровня OEM** благодаря easing функциям и временному сглаживанию
- **Бесшовные переходы** intro→scene и между темами (screen blend кроссфейд)
- **Авто-ротация** тем каждые N минут с плавным кроссфейдом
- **Циклическая ротация** тем при смене OEM цвета через CAN
- **Sleep Mode** для минимизации энергопотребления (STOP mode STM32)
- **Интеллектуальный старт** - ожидание CAN данных перед запуском ленты

---

## 11. История изменений

### v3.0 - Новый цветовой pipeline и обновлённые дефолты

- TIM2 PWM + DMA для всех зон вместо TIM1 (4 канала, CubeMX настройка без TIM1)
- Цветопередача: включены гамма 2.2 + темпоральный дезеринг; OEM яркость проходит LUT [0..0.86] и сглаживание (~200 мс) с удержанием уровня на outro
- Premium FX tuning: `FX_SPARKLE` переведён на детерминированный smooth twinkle (без frame-random pop), `FX_CASCADE` получил мягкий first-frame/temporal path, `FX_TWO_TONE_WAVE` получил межкадровое сглаживание
- Добавлены `Theme Personality + Cabin Depth + Bank Character Memory`: per-theme speed/phase/depth signature, pseudo-3D cabin parallax и восстановление динамики при возврате в OEM-банк
- Добавлен климатический event pulse (W223-style): по изменению `HVAC_Temp_*_Stat` вверх/вниз запускается короткий warm/cool transient в `event_layer`
- HVAC premium behavior: `drag trail` на strip (door-aware + burst), board-side routing (left/right), dual-zone split bias, fan-driven motion modulation, comfort auto-dim, cabin gradient memory, seat-heater coupling и energy-aware saturation/whitepoint trim
- Авто-ротация: по умолчанию выключена, дефолты 10с / 1600мс; screen-blend кроссфейд сохраняет фазу FX и работает только при включённом флаге
- Sleep Mode: таймаут 4с (стенд), плавный fade + outro с отменой при новой CAN активности; пробуждение по EXTI на FDCAN RX и повторное ожидание первого OEM пакета
- Flash storage: задержка сохранения 10с и запись только при реальном изменении bank/theme/индексов (минимальный износ)
- CAN протокол: Extended режим удалён; основной поток — `0x325` + `0x353`, добавлен `0x025` (day/night от датчика света) для комфортного ночного димминга
- Переход intro→scene ускорен до 400мс (quintic smoothstep)

### v2.9 - Улучшения безопасности и производительности

**Критические исправления:**

- **Race Conditions** - исправлены атомарные операции с `can_state_t`
  - Заменено прямое присваивание структуры на `memcpy()` для thread-safety
  - Защита от частичного чтения структуры при прерываниях

- **Crossfade Buffer Overflow** - увеличен размер буфера для кроссфейда
  - `MAX_CROSSFADE_LEDS` увеличен до 256 (из `features.h`)
  - Предотвращает переполнение при больших зонах

- **DMA Buffer Alignment** - добавлено выравнивание для оптимальной работы DMA
  - Все DMA буферы теперь используют `__ALIGNED(4)`
  - Создан `board_common.h` с общими макросами для всех board файлов

- **Flash Wear-out Protection** - оптимизация записи в Flash
  - Увеличена задержка сохранения с 2 до 10 секунд
  - Добавлена проверка изменения данных перед записью
  - Минимизирует износ Flash памяти

**Новые функции:**

- **Independent Watchdog (IWDG)** - защита от зависаний
  - Автоматический сброс при зависании системы (таймаут 2 сек)
  - Настраивается через `features.h` (`AMB_ENABLE_WATCHDOG`, `AMB_WATCHDOG_TIMEOUT_MS`)

**Улучшения кода:**

- **Magic Numbers → features.h** - все параметры эффектов вынесены в конфигурацию
  - `FX_WELCOME_*`, `FX_GOODBYE_*` параметры теперь в `features.h`
  - Параметры CAN протокола (`AMB_CAN_*`) централизованы
  - Легко настраивать без изменения кода эффектов

- **Удалён неиспользуемый код** - очистка проекта
  - Удалена неиспользуемая функция `ease_crossfade()`
  - Удалён весь RTC код (не используется, HAL_RTC_MODULE_ENABLED отключён)

- **CAN Baudrate Comment** - добавлен подробный комментарий с расчётом
  - Формула: `160 MHz / 40 / 16 = 250 kbit/s`

- **Рефакторинг board файлов** - уменьшение дублирования
  - Создан `board_common.h` с общими макросами
  - Единообразное объявление DMA буферов с правильным выравниванием

**Технические детали:**

- Все DMA буферы используют `__ALIGNED(4)` для оптимальной работы DMA
- Flash сохранение происходит только при реальном изменении данных
- Watchdog refresh происходит в конце главного цикла (каждую итерацию)
- Код watchdog полностью защищён от перезаписи CubeMX

**Настройки в `features.h`:**
```c
// Watchdog
#define AMB_ENABLE_WATCHDOG        1       // 1 = enable IWDG watchdog
#define AMB_WATCHDOG_TIMEOUT_MS    8000u   // Watchdog timeout in ms

// Flash Storage
#define AMB_FLASH_SAVE_DELAY_MS    10000u  // Delay before saving to Flash (10 sec)

// Crossfade
#define AMB_MAX_CROSSFADE_LEDS     256u    // Max LEDs for crossfade temp buffer

// Brightness smoothing
#define AMB_DIMMING_ATTACK_MS      450u    // OEM brightness ramp-up
#define AMB_DIMMING_RELEASE_MS     900u    // OEM brightness ramp-down
#define AMB_DIMMING_POST_SMOOTH    0.82f   // Extra post-smoothing for premium dimming

// Scene transitions
#define AMB_TRANSITION_SMOOTH_ALPHA 0.76f  // Smoothing of bridge/crossfade blend

// CAN Protocol
#define AMB_CAN_MASTER_TX_INTERVAL_MS   200u    // Master packet interval
#define AMB_CAN_SYNC_INTERVAL_MS        500u    // Sync/heartbeat interval
#define AMB_CAN_DISCOVERY_INTERVAL_MS   1500u   // Discovery packet interval
#define AMB_CAN_STARTUP_DISCOVERY_MS    1500u   // Discovery-only window after first OEM packet
#define AMB_CAN_ACTIVE_TIMEOUT_MS       2000u   // Stop TX if no CAN RX within this window
```

### v2.8 - Sleep Mode и интеллектуальный старт

**Новые функции:**

- **Sleep Mode** - автоматический переход в режим энергосбережения
  - MCU переходит в STOP mode при отсутствии CAN активности
  - Отключение питания LED ленты (`ws_power_set(0)`)
  - CAN трансивер переводится в standby (`FDCAN1_STBY_Pin`)
  - Пробуждение по EXTI от CAN RX (PB8)
  - Плавное затухание яркости перед сном (2 сек)
  - Воспроизведение outro анимации перед засыпанием

- **Интеллектуальный старт** (не в DEMO режиме)
  - Лента НЕ включается при подаче питания
  - Ожидание первого CAN пакета с OEM данными (цвет, яркость)
  - Выбор темы на основе OEM цвета и сохранённого индекса из Flash
  - Только после получения CAN данных запускается intro анимация
  - CAN пакеты от платы начинают отправляться только после OEM пакета

**Настройки в `features.h`:**
```c
#define AMB_ENABLE_SLEEP_MODE       1     // вкл/выкл sleep mode
#define AMB_SLEEP_TIMEOUT_SEC      60u    // таймаут до засыпания (секунды)
#define AMB_SLEEP_FADE_OUT_MS    2000u    // длительность затухания перед сном (мс)
#define AMB_SLEEP_CANCEL_IDLE_DIV   2u    // порог отмены sleep/outro: timeout/div

// PT4211 (door handle), defaults зависят от AMB_PROFILE:
// BENCH:      ATTACK=900ms,  RELEASE=1500ms
// PRODUCTION: ATTACK=1400ms, RELEASE=2200ms
```

**Порядок засыпания:**
1. Нет CAN активности в течение `AMB_SLEEP_TIMEOUT_SEC`
2. Плавное затухание яркости → 0
3. Outro анимация (goodbye)
4. Отключение питания ленты
5. CAN трансивер в standby
6. STM32 в STOP mode

**Порядок пробуждения:**
1. CAN пакет на шине → EXTI прерывание
2. Выход из STOP mode
3. Восстановление тактирования
4. Включение CAN и питания ленты
5. Ожидание OEM пакета
6. Intro анимация

**API функции:**
- `can_ambient_check_sleep_timeout()` - проверка таймаута
- `can_ambient_enter_sleep()` / `can_ambient_exit_sleep()` - вход/выход из сна
- `can_ambient_is_sleeping()` - проверка статуса
- `can_ambient_oem_received()` / `can_ambient_reset_oem_received()` - флаг OEM пакета
- `can_ambient_note_stop_wakeup()` - фиксация источника wakeup после выхода из STOP
- `can_ambient_get_power_diag()` / `can_ambient_reset_power_diag()` - чтение/сброс runtime-диагностики sleep/wake

**Sleep/Wake диагностика:**
- Счётчики: запросов на sleep, реальных входов в sleep, пробуждений
- Причина sleep: `IDLE_TIMEOUT` / `API_ENTER`
- Причина wake: `CAN_RX` / `STOP_PIN` / `EXIT_SLEEP` / `MARK_AWAKE`
- Источник EXTI wake pin:
  - `1` = PA11 (`CAN_RX`)
  - `2` = PB7 (`FDCAN1_WAKEUP`)

**Примечание:** Sleep mode отключен в DEMO режиме (`DEMO_MODE = 1`)

---

### v2.7 - Циклическое переключение тем OEM

**Новые функции:**
- **Циклическая ротация тем при смене OEM цвета**
  - При переключении цвета (AMBER→WHITE→AMBER) выбирается следующая тема в банке
  - Каждый банк (AMBER, BLUE, WHITE) помнит свой текущий индекс
  - Индексы сохраняются в Flash и восстанавливаются после перезагрузки
  - **Работает независимо от флага авто-ротации** (`AMB_ENABLE_AUTO_ROTATE`)

**Пример работы:**
```
Старт AMBER    → AMBER 1st theme
  → WHITE      → WHITE 1st theme
  → AMBER      → AMBER 2nd theme (не 1st!)
  → WHITE      → WHITE 2nd theme
  → BLUE       → BLUE 1st theme
  → WHITE      → WHITE 3rd theme
  → ... и так далее циклически
```

**Изменения в Flash Storage:**
- Добавлены поля `last_oem_color` и `oem_theme_indices[3]`
- Индексы автоматически сохраняются через 2 секунды после изменения

**Два независимых механизма переключения тем:**

| Механизм | Триггер | Анимация | Настройка |
|----------|---------|----------|-----------|
| Циклическое переключение | Смена OEM цвета через CAN | Напрямую | Всегда включено |
| Авто-ротация по таймеру | По истечении интервала | Screen blend кроссфейд | `AMB_ENABLE_AUTO_ROTATE` |

**Улучшения:**
- Screen blend для кроссфейда предотвращает затемнение при переходе
- Функция `theme_same_bank()` для определения принадлежности тем к банку

---

## Offline CAN Replay (без машины)

Для регрессии CAN-сценариев добавлен инструмент:
- `tools/can_replay_check.py`

Поддерживаемые форматы:
- `candump` (`(ts) can0 325#...` и `ts can0 325 [6] ...`)
- ASC-подобные строки (`ts ch id Rx d dlc ...`)

Примеры запуска:

```bash
python3 tools/can_replay_check.py trace.log --require-oem
python3 tools/can_replay_check.py trace.asc --require-oem --expect-color blue
python3 tools/can_replay_check.py trace.log --sleep-timeout-sec 4 --min-idle-gaps 1
python3 tools/can_replay_check.py trace.log --require-bsm
python3 tools/can_replay_check.py trace.log --require-no-master-before-oem --require-master-after-oem 1
python3 tools/can_replay_check.py trace.log --profile production
python3 tools/run_replay_cases.py
```

Отчёт содержит:
- число OEM/Master/BSM кадров
- первый OEM timestamp
- последний декодированный OEM цвет и brightness raw
- число NSI/BSM active кадров
- максимальный idle gap и количество idle gaps выше порога sleep timeout

---

### v2.6 - Плавные переходы и авто-ротация

**Новые функции:**
- **BASE_SCENE_BRIDGE** - плавный переход между intro и scene (600мс)
  - Использует quintic smoothstep для максимально плавного перехода
  - Устраняет видимый "скачок" между intro и основной сценой
  - Интро заканчивается с фазой стабилизации яркости

- **Авто-ротация тем** - автоматическая смена тем внутри банка
  - Плавный кроссфейд между темами (без intro/outro)
  - Настраиваемый интервал (по умолчанию 2 минуты)
  - Настраиваемая длительность кроссфейда (по умолчанию 3 секунды)
  - Управление через флаг `AMB_ENABLE_AUTO_ROTATE` в `features.h`

**Новые настройки в `features.h`:**
```c
#define AMB_ENABLE_AUTO_ROTATE         1     // вкл/выкл авто-ротации
#define AMB_AUTO_ROTATE_INTERVAL_SEC   120u  // интервал смены темы
#define AMB_CROSSFADE_DURATION_MS      3000u // длительность кроссфейда
```

**Улучшения:**
- Оптимизирован intro эффект (FX_WELCOME):
  - Добавлена фаза стабилизации (settle) в конце анимации
  - Уменьшено затемнение краёв для более равномерного финиша
  - Тайминги волн и пульса оптимизированы для завершения до конца анимации
- Quintic smoothstep заменил кубический для более плавных переходов

---

### v2.5 - W223/EQS Premium Redesign

**Новые эффекты (5):**
- `FX_AURORA` - северное сияние с несколькими синусоидами разных фаз и скоростей
- `FX_CASCADE` - каскадный эффект с падающей волной яркости
- `FX_SPARKLE` - мерцающие искры поверх базового градиента
- `FX_BREATHE_WAVE` - движущаяся волна яркости (медитативный эффект)
- `FX_COLOR_MORPH` - гипнотическое перетекание между зонами палитры

**Новые палитры (5):**
- `WSPAL_AURORA_BOREALIS` - северное сияние (зелёный/голубой/фиолетовый)
- `WSPAL_CHAMPAGNE_GOLD` - тёплый перламутровый шампань
- `WSPAL_DEEP_BURGUNDY` - глубокий бордо с винным оттенком
- `WSPAL_EMERALD_FOREST` - изумрудный лес с тёплыми акцентами
- `WSPAL_ICE_SAPPHIRE` - ледяной сапфировый синий

**Улучшения:**
- Добавлены easing функции (smoothstep, ease_in_out_cubic, ease_out_quad) для премиальной плавности
- Обновлены все банки тем с использованием новых эффектов
- Каждая тема имеет уникальный характер: магический, медитативный, праздничный
- Переработаны скорости эффектов для более элегантного восприятия

**Обновлённые темы:**
- Amber Bank: Aurora, Cascade (Champagne), Sparkle (Gold)
- Blue Bank: Aurora (Borealis), Breathe Wave (Ocean), Sparkle (Sapphire), Color Morph
- White Bank: Cascade (Diamond), Aurora (Glacier), Sparkle (Platinum)
- Premium Bank: Breathe Wave (Diamond/Burgundy), Aurora (Magenta), Color Morph (Emerald)

---

### v2.4 - Премиальная оптимизация тем и эффектов

**Добавлено:**
- 5 палитр (Diamond White, Silver Mist, Platinum, Amber Gold, Magenta Royal)
- 2 эффекта (Velvet Flow, Gentle Pulse)

**Оптимизация:**
- Все палитры оптимизированы для более премиального вида (мягче цвета, плавнее переходы)
- Замена резких эффектов на более плавные (ENERGIZE_PULSE → GENTLE_PULSE, FLOW_SOFT → VELVET_FLOW)
- Корректировка яркости тем для более премиального эффекта
- Добавление BREATHE эффектов в зоны для более живого освещения
- Улучшение цветовых акцентов для более мягкого восприятия

**Исправлено:**
- Устранены дубликаты тем (THEME_BLUE_SKY_NIGHT vs THEME_INDIGO_SKY, THEME_AMBER_SOFT_PULSE vs THEME_RED_MOON_PREMIUM)
- Каждая тема имеет уникальную комбинацию палитры и эффекта

### v2.3 - Исправление демо режима

**Исправлено:**
- Исправлена работа демо режима в main цикле
- Демо режим больше не конфликтует с CAN системой
- Добавлено отслеживание перехода платы в master статус
- Таймер переключения тем сбрасывается при переходе в master
- Демо режим синхронизируется с OEM цветом из CAN
- `DEMO_MODE` определен в `main.h` для доступности в CAN модуле

**Улучшения:**
- Правильный порядок вызовов функций в main цикле
- Демо режим корректно работает только на master плате
- Автоматическое переключение тем каждые 20 секунд
- Плавные переходы между темами через intro/outro

### v2.2 - Flash Storage

**Добавлено:**
- Автоматическое сохранение настроек Extended режима в Flash
- Защита данных (Magic number + CRC32)
- Оптимизация записи (задержка 2 секунды)

### v2.1 - Исправления

**Исправлено:**
1. Индексация пикселей в zones.c
2. Формат данных RGB в эффектах
3. Защита от деления на ноль
4. Улучшение плавности анимаций

**Результат:** Все анимации работают плавно без артефактов, улучшена стабильность

### v2.0 - Optimized CAN Protocol

**Основные изменения:**
- Оптимизированная CAN-архитектура с минимальным количеством ID
- Unified Master Protocol (0x353)
- Автоматическое определение master/slave
- Отказоустойчивость
├── Core/                    # Runtime orchestration around HAL entrypoint
│   ├── Inc/led_runtime.h    # Neutral LED façade over WS2812 driver calls
│   ├── Src/led_runtime.c    # Power/brightness/DMA bridge used by runtime modules
│   ├── Inc/runtime_flow.h   # Runtime state machine + active/sleep flow API
│   ├── Src/runtime_flow.c   # WAIT_OEM/ACTIVE/SLEEP/STOP orchestration
│   ├── Inc/runtime_render.h # Black-frame reset и frame postprocess
│   └── Src/runtime_render.c # Motion tint / boost / frame slew
│
