# Changelog

## v3.1 (M3 Freeze)

- Freeze baseline created for completed `M1/M2/M3` architecture.
- Added `docs/m3-freeze.md` with:
  - frozen scope and acceptance checklist,
  - build verification snapshot,
  - accepted limitations and post-freeze M4 entry point.
- CAN robustness baseline included in freeze:
  - ingress validation for `0x463/0x12B/0x325`,
  - reject/clamp counters exported via diagnostics APIs.
- Documentation index updated to include M3 freeze baseline.

## v3.0

- Новый цветовой pipeline: `gamma 2.2` + temporal dithering + сглаживание яркости.
- Premium-tuning эффектов (`FX_SPARKLE`, `FX_CASCADE`, `FX_TWO_TONE_WAVE`) и переходов сцен.
- Добавлены `Theme Personality`, `Cabin Depth`, `Bank Character Memory`.
- Климатические event-эффекты: temp trend, drag trail, split bias, fan modulation, comfort dim.
- Sleep/wake flow усилен: gate по двери, lock-trigger goodbye + ускоренный forced sleep.
- Добавлены CAN-события `EIS_A2` (unlock/lock) и `DrRLtch_*` (door-open).
- Расширена DBC-интеграция: `CLkS_TurnLmp_*_Rq`, `ILM_RQ` dim, `TGW_A8` day/night fallback, `PTS_A1` parking overlay, reverse (`RVC_A2/CHASSIS_R2`), `HVAC_A1` sun compensation.
- Добавлены `door choreography` (pre-glow -> strip trail) и общий `context hold` (climate/door/parking/reverse).
- Протокол в production: `0x325` + `0x353`, с `0x025` (day/night) как приоритетным источником night mode.
- OEM color decode в `0x325` сверён с реальными дампами: default источник `data[3].bits[3:2]` (`orange=0`, `neutral=1`, `polar=2`); DBC `28|2` для этого набора логов не совпадает с фактической сменой цвета.
- Для slave-плат свежий локальный `0x325` теперь имеет приоритет над `0x353 MASTER` по OEM color/brightness, чтобы master broadcast не перетирал фактический банк цвета на платах, которые сами видят штатную шину.
- Theme resolver теперь клампит сохраненный/синхронизированный `theme_index` по размеру текущего банка, чтобы некорректное значение из Flash или master sync не ломало выбор темы.
- Director теперь применяет смену банка немедленно через crossfade, без pending/debounce окна; если crossfade недоступен, тема применяется напрямую.
- В `0x353 MASTER` добавлены diagnostic bytes `b6/b7` для проверки raw `0x325.data[3]`, decoded raw color, `oem_color` и `bank_id` на машине.
- `WAIT_OEM` door-gate теперь обходится при ignition ON (`0x006 SAM_F_A1`, `PwrSup15_On`/`PwrSup15R_On`), чтобы ambient стартовал без ожидания открытия двери при включенном зажигании.
- `door handle` / PT4211 DIM тестово переключен на `0x02F LM_A4.PddlLmp_*_On_Rq` (OR по FL/FR/RL/RR); `0x069 NS_Ill_Actv` и `0x325 SurrIll_Rq` оставлены как request-кандидаты, `IL_R_On_Rq` и `DrHdl_Brt` не используются как trigger.
- Исправлены условия, которые могли блокировать handle PWM: общий `ambient_config.h` теперь подтягивает `ambient_visual_motion.h` с правильной `AMB_HANDLE_PWM_ACTIVE_LOW`, а новая CAN activity сбрасывает pending idle sleep-request.
- Door-open gate полностью убран из `door handle` / PT4211: состояние ручки теперь определяется только выбранным handle source, stale-guard и sleep.

## v2.9

- Исправлены race conditions при доступе к CAN-state.
- Усилена защита DMA/буферов (`__ALIGNED(4)`, корректные лимиты crossfade).
- Оптимизирована запись Flash (deferred save, запись только при реальном изменении).
- Добавлен IWDG watchdog.

## v2.8

- Введён Sleep Mode (fade/outro -> STOP -> wake по CAN RX/EXTI).
- Интеллектуальный старт: ожидание OEM CAN перед активацией подсветки.

## v2.7

- Циклическая ротация тем по смене OEM-банка (`AMBER/WHITE/BLUE`) с памятью индексов в Flash.

## v2.6

- Добавлен `BASE_SCENE_BRIDGE` для плавного intro->scene перехода.
- Добавлена авто-ротация тем с кроссфейдом внутри банка.

## v2.5

- W223/EQS premium redesign: новые FX (`AURORA/CASCADE/SPARKLE/BREATHE_WAVE/COLOR_MORPH`) и палитры.

## v2.4

- Премиальная ретюнировка палитр/эффектов, удаление дублей тем, более мягкая динамика.

## v2.3

- Удалена legacy-ветка runtime, оставлен единый production flow.

## v2.2

- Добавлено хранение настроек в Flash (magic + CRC + delayed write).

## v2.1

- Исправлены ошибки индексации зон, RGB-format и edge cases плавности.

## v2.0

- Оптимизирован CAN-протокол и master/slave архитектура с failover.
