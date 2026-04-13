# Replay Cases

Минимальные эталонные логи для офлайн регрессии `tools/can_replay_check.py`.

Кейсы:
- `case_oem_blue.log` — базовый OEM сценарий, финальный цвет `blue`
- `case_sleep_gap.log` — содержит idle gap > 4 сек
- `case_bsm.log` — содержит активные BSM кадры `0x17E`
- `case_wait_oem_gate.log` — master-пакеты идут только после первого OEM

Запуск всех кейсов:

```bash
python3 tools/run_replay_cases.py
```
