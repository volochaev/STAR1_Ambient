#!/usr/bin/env python3
"""
Run built-in offline CAN replay regression cases.
"""

from __future__ import annotations

import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List


ROOT = Path(__file__).resolve().parent.parent
TOOL = ROOT / "tools" / "can_replay_check.py"
CASES_DIR = ROOT / "tools" / "replay_cases"


@dataclass
class ReplayCase:
    name: str
    logfile: Path
    args: List[str]


def run_case(case: ReplayCase) -> int:
    cmd = [sys.executable, str(TOOL), str(case.logfile)] + case.args
    print(f"\n=== {case.name} ===")
    print("cmd:", " ".join(cmd))
    p = subprocess.run(cmd, capture_output=True, text=True)
    if p.stdout:
        print(p.stdout.rstrip("\n"))
    if p.stderr:
        print(p.stderr.rstrip("\n"))
    return p.returncode


def main() -> int:
    if not TOOL.exists():
        print(f"[ERR] tool not found: {TOOL}")
        return 2

    cases = [
        ReplayCase(
            name="OEM blue basic",
            logfile=CASES_DIR / "case_oem_blue.log",
            args=["--profile", "bench", "--require-oem", "--expect-color", "blue"],
        ),
        ReplayCase(
            name="Sleep gap detection",
            logfile=CASES_DIR / "case_sleep_gap.log",
            args=[
                "--profile",
                "bench",
                "--require-oem",
                "--sleep-timeout-sec",
                "4",
                "--min-idle-gaps",
                "1",
            ],
        ),
        ReplayCase(
            name="BSM active detection",
            logfile=CASES_DIR / "case_bsm.log",
            args=["--profile", "bench", "--require-oem", "--require-bsm"],
        ),
        ReplayCase(
            name="WAIT_OEM gate (master only after OEM)",
            logfile=CASES_DIR / "case_wait_oem_gate.log",
            args=[
                "--profile",
                "bench",
                "--require-oem",
                "--require-no-master-before-oem",
                "--require-master-after-oem",
                "2",
            ],
        ),
    ]

    failed = 0
    for case in cases:
        if not case.logfile.exists():
            print(f"[ERR] missing case file: {case.logfile}")
            failed += 1
            continue
        rc = run_case(case)
        if rc != 0:
            print(f"[FAIL] {case.name} (rc={rc})")
            failed += 1
        else:
            print(f"[PASS] {case.name}")

    print("\n=== Replay Summary ===")
    print(f"total: {len(cases)}")
    print(f"failed: {failed}")
    print(f"passed: {len(cases) - failed}")

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
