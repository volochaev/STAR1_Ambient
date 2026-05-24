#!/usr/bin/env python3
"""
Offline CAN replay checker for STAR1 Ambient.

Parses common log formats (candump / Vector ASC-like) and validates:
- first OEM packet (0x325)
- OEM color/brightness decoding
- NSI request activity (from 0x325)
- BSM activity (from 0x17E)
- inactivity gaps relative to sleep timeout
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple


CAN_OEM_ID = 0x325
CAN_MASTER_ID = 0x353
CAN_BSM_ID = 0x17E
CAN_HVAC_FAN_ID = 0x339
CAN_LGTSENS_ID = 0x025
CAN_DRIVE_MODE_ID = 0x38E


OEM_COLOR_AMBER = 0
OEM_COLOR_WHITE = 1
OEM_COLOR_BLUE = 2

PKT_TYPE_MASK = 0xF0
PKT_TYPE_MASTER = 0x10
PKT_TYPE_SYNC = 0x20

MOTION_PROFILE_LUXURY = 0
MOTION_PROFILE_CALM = 1
MOTION_PROFILE_SPORT = 2

AMB_MOTION_PROFILE_SCALE_LUXURY = 1.00
AMB_MOTION_PROFILE_SCALE_CALM = 0.84
AMB_MOTION_PROFILE_SCALE_SPORT = 1.20
AMB_HVAC_FAN_MOTION_MIN = 0.86
AMB_HVAC_FAN_MOTION_MAX = 1.30
AMB_BANK_CHARACTER_LERP_ALPHA = 0.78
AMB_BANK_CHARACTER_SPEED_MIN = 0.84
AMB_BANK_CHARACTER_SPEED_MAX = 1.22
AMB_THEME_DYNAMIC_SCALE_MIN = 0.78
AMB_THEME_DYNAMIC_SCALE_MAX = 1.35

THEME_MOTION_SCALE = [
    0.82, 0.88, 0.92, 0.78,
    0.96, 1.14, 1.00, 1.18,
    0.74, 0.82, 0.72, 0.78,
]
THEME_PERSONALITY_SPEED = [
    0.92, 1.00, 1.06, 0.88,
    0.98, 1.12, 0.95, 1.10,
    0.90, 0.94, 0.86, 0.92,
]
THEME_BANKS = {
    OEM_COLOR_AMBER: [0, 1, 2, 3],
    OEM_COLOR_BLUE: [4, 5, 6, 7],
    OEM_COLOR_WHITE: [8, 9, 10, 11],
}


@dataclass
class CanFrame:
    ts: float
    can_id: int
    data: bytes
    source_line: str


@dataclass
class ReplaySummary:
    total_frames: int = 0
    first_ts: float = 0.0
    last_ts: float = 0.0
    oem_frames: int = 0
    master_frames: int = 0
    bsm_frames: int = 0
    first_oem_ts: Optional[float] = None
    master_before_first_oem: int = 0
    master_after_first_oem: int = 0
    last_oem_color: Optional[int] = None
    last_oem_brightness_raw: Optional[int] = None
    nsi_active_frames: int = 0
    bsm_active_frames: int = 0
    max_idle_gap_s: float = 0.0
    idle_gap_hits: int = 0


@dataclass
class DynamicScaleSummary:
    samples: int = 0
    raw_min: float = 0.0
    raw_max: float = 0.0
    capped_min: float = 0.0
    capped_max: float = 0.0
    cap_low_hits: int = 0
    cap_high_hits: int = 0
    out_of_range_hits: int = 0


def clamp01(x: float) -> float:
    if x < 0.0:
        return 0.0
    if x > 1.0:
        return 1.0
    return x


def decode_lgtsens_night(d: bytes) -> Optional[bool]:
    if len(d) < 2:
        return None
    night_1 = (d[0] >> 1) & 0x01
    night_2 = (d[1] >> 5) & 0x01
    return (night_1 != 0) or (night_2 != 0)


def decode_drive_mode_profile(d: bytes) -> Optional[int]:
    if len(d) <= 1:
        return None
    raw = (d[1] >> 2) & 0x07
    if raw in (1, 2):  # SPORT / SPORT_PLUS
        return MOTION_PROFILE_SPORT
    if raw in (5, 4):  # CALM(ECO) / SLEEK
        return MOTION_PROFILE_CALM
    if raw in (0, 3):  # COMFORT / INDIVIDUAL
        return MOTION_PROFILE_LUXURY
    return None


def profile_scale(profile: int) -> float:
    if profile == MOTION_PROFILE_CALM:
        return AMB_MOTION_PROFILE_SCALE_CALM
    if profile == MOTION_PROFILE_SPORT:
        return AMB_MOTION_PROFILE_SCALE_SPORT
    return AMB_MOTION_PROFILE_SCALE_LUXURY


def profile_target_speed(profile: int) -> float:
    if profile == MOTION_PROFILE_CALM:
        return 0.92
    if profile == MOTION_PROFILE_SPORT:
        return 1.10
    return 1.00


def resolve_theme_id(oem_color: int, theme_index: int) -> int:
    bank = THEME_BANKS.get(oem_color, THEME_BANKS[OEM_COLOR_AMBER])
    if not bank:
        return 0
    if theme_index < 0 or theme_index >= len(bank):
        return bank[0]
    return bank[theme_index]


def decode_master_fields(d: bytes) -> Optional[Tuple[int, int, int, int]]:
    if len(d) < 4:
        return None
    pkt_type = d[0] & PKT_TYPE_MASK
    if pkt_type != PKT_TYPE_MASTER:
        return None
    flags = d[0] & 0x0F
    bank_id = d[1]
    theme_index = d[2]
    oem_color = d[3]
    profile_raw = d[5] if len(d) > 5 else MOTION_PROFILE_LUXURY
    if bank_id > 2 or oem_color > 2:
        return None
    if profile_raw > MOTION_PROFILE_SPORT:
        profile_raw = MOTION_PROFILE_LUXURY
    return flags, bank_id, theme_index, profile_raw


def decode_sync_fields(d: bytes) -> Optional[Tuple[int, int]]:
    if len(d) < 2:
        return None
    pkt_type = d[0] & PKT_TYPE_MASK
    if pkt_type != PKT_TYPE_SYNC:
        return None
    return (d[0] & 0x0F), d[1]


PROFILE_BENCH = "bench"
PROFILE_PRODUCTION = "production"

PROFILE_DEFAULTS = {
    PROFILE_BENCH: {
        "can_wake_policy": "AMB_CAN_WAKE_BENCH",
        "watchdog_timeout_ms": 8000,
        "sleep_timeout_sec": 4.0,
    },
    PROFILE_PRODUCTION: {
        "can_wake_policy": "AMB_CAN_WAKE_PRODUCTION",
        "watchdog_timeout_ms": 8000,
        "sleep_timeout_sec": 4.0,
    },
}


RE_CANDUMP_A = re.compile(
    r"^\s*\((?P<ts>\d+(?:\.\d+)?)\)\s+\w+\s+(?P<id>[0-9A-Fa-f]{1,8})#(?P<data>[0-9A-Fa-f]*)\s*$"
)
RE_CANDUMP_B = re.compile(
    r"^\s*(?P<ts>\d+(?:\.\d+)?)\s+\w+\s+(?P<id>[0-9A-Fa-f]{1,8})\s+\[(?P<dlc>\d+)\]\s+(?P<data>(?:[0-9A-Fa-f]{2}\s*)+)\s*$"
)
RE_ASC = re.compile(
    r"^\s*(?P<ts>\d+(?:\.\d+)?)\s+\d+\s+(?P<id>[0-9A-Fa-f]{1,8})\s+(?:Rx|Tx)\s+d\s+(?P<dlc>\d+)\s+(?P<data>(?:[0-9A-Fa-f]{2}\s*)+)\s*$",
    re.IGNORECASE,
)


def parse_hex_data(data_hex: str) -> bytes:
    data_hex = data_hex.strip().replace(" ", "")
    if len(data_hex) % 2 != 0:
        data_hex = data_hex[:-1]
    if not data_hex:
        return b""
    return bytes.fromhex(data_hex)


def parse_line(line: str) -> Optional[CanFrame]:
    m = RE_CANDUMP_A.match(line)
    if m:
        ts = float(m.group("ts"))
        can_id = int(m.group("id"), 16)
        data = parse_hex_data(m.group("data"))
        return CanFrame(ts=ts, can_id=can_id, data=data, source_line=line.rstrip("\n"))

    m = RE_CANDUMP_B.match(line)
    if m:
        ts = float(m.group("ts"))
        can_id = int(m.group("id"), 16)
        dlc = int(m.group("dlc"))
        data = parse_hex_data(m.group("data"))[:dlc]
        return CanFrame(ts=ts, can_id=can_id, data=data, source_line=line.rstrip("\n"))

    m = RE_ASC.match(line)
    if m:
        ts = float(m.group("ts"))
        can_id = int(m.group("id"), 16)
        dlc = int(m.group("dlc"))
        data = parse_hex_data(m.group("data"))[:dlc]
        return CanFrame(ts=ts, can_id=can_id, data=data, source_line=line.rstrip("\n"))

    return None


def decode_oem_color(d: bytes) -> Optional[int]:
    if len(d) < 4:
        return None
    raw_col = (d[3] >> 2) & 0x03
    if raw_col == 0:
        return OEM_COLOR_AMBER
    if raw_col == 1:
        return OEM_COLOR_WHITE
    if raw_col == 2:
        return OEM_COLOR_BLUE
    return None


def decode_nsi_active(d: bytes) -> bool:
    if len(d) < 4:
        return False
    surr_ill_rq = d[3] & 0x03
    ns_illdur_vld = (d[0] >> 3) & 0x01
    ns_illdur = d[2]
    return (surr_ill_rq == 1) or (ns_illdur_vld == 1 and ns_illdur > 0)


def decode_bsm_active(d: bytes) -> bool:
    if len(d) < 5:
        return False
    l_brt = d[2]
    r_brt = d[3]
    l_rq = (d[4] >> 6) & 0x03
    r_rq = (d[4] >> 4) & 0x03
    left_active = (l_brt > 8) or (l_rq != 0)
    right_active = (r_brt > 8) or (r_rq != 0)
    return left_active or right_active


def analyze(frames: List[CanFrame], sleep_timeout_s: float, verify_dynamic_scale_cap: bool) -> Tuple[ReplaySummary, Optional[DynamicScaleSummary]]:
    s = ReplaySummary()
    dsum = DynamicScaleSummary() if verify_dynamic_scale_cap else None
    if not frames:
        return s, dsum

    s.total_frames = len(frames)
    s.first_ts = frames[0].ts
    s.last_ts = frames[-1].ts

    cur_oem_color = OEM_COLOR_AMBER
    cur_theme_index = 0
    cur_theme_id = resolve_theme_id(cur_oem_color, cur_theme_index)
    cur_profile = MOTION_PROFILE_LUXURY
    cur_night = 0
    cur_fan = 0.0
    bank_speed = [1.0, 1.0, 1.0]
    bank_valid = [False, False, False]

    def sample_bank_character() -> None:
        nonlocal bank_speed, bank_valid
        b = cur_oem_color if 0 <= cur_oem_color < 3 else OEM_COLOR_AMBER
        target = profile_target_speed(cur_profile)
        target += clamp01(cur_fan) * 0.08
        if cur_night:
            target *= 0.96
        if target < AMB_BANK_CHARACTER_SPEED_MIN:
            target = AMB_BANK_CHARACTER_SPEED_MIN
        if target > AMB_BANK_CHARACTER_SPEED_MAX:
            target = AMB_BANK_CHARACTER_SPEED_MAX
        if not bank_valid[b]:
            bank_speed[b] = target
            bank_valid[b] = True
        else:
            k = AMB_BANK_CHARACTER_LERP_ALPHA
            if k < 0.0:
                k = 0.0
            if k > 0.98:
                k = 0.98
            bank_speed[b] = bank_speed[b] * k + target * (1.0 - k)

    def sample_dynamic_scale() -> None:
        if dsum is None:
            return
        theme_scale = THEME_MOTION_SCALE[cur_theme_id] if 0 <= cur_theme_id < len(THEME_MOTION_SCALE) else 1.0
        personality = (
            THEME_PERSONALITY_SPEED[cur_theme_id]
            if 0 <= cur_theme_id < len(THEME_PERSONALITY_SPEED)
            else 1.0
        )
        b = cur_oem_color if 0 <= cur_oem_color < 3 else OEM_COLOR_AMBER
        bank_mem = bank_speed[b] if bank_valid[b] else 1.0
        fan = clamp01(cur_fan)
        fan_scale = AMB_HVAC_FAN_MOTION_MIN + (AMB_HVAC_FAN_MOTION_MAX - AMB_HVAC_FAN_MOTION_MIN) * fan
        raw = theme_scale * profile_scale(cur_profile) * personality * bank_mem * fan_scale
        capped = raw
        if capped < AMB_THEME_DYNAMIC_SCALE_MIN:
            capped = AMB_THEME_DYNAMIC_SCALE_MIN
            dsum.cap_low_hits += 1
        if capped > AMB_THEME_DYNAMIC_SCALE_MAX:
            capped = AMB_THEME_DYNAMIC_SCALE_MAX
            dsum.cap_high_hits += 1
        if dsum.samples == 0:
            dsum.raw_min = raw
            dsum.raw_max = raw
            dsum.capped_min = capped
            dsum.capped_max = capped
        else:
            dsum.raw_min = min(dsum.raw_min, raw)
            dsum.raw_max = max(dsum.raw_max, raw)
            dsum.capped_min = min(dsum.capped_min, capped)
            dsum.capped_max = max(dsum.capped_max, capped)
        if capped < AMB_THEME_DYNAMIC_SCALE_MIN - 1e-6 or capped > AMB_THEME_DYNAMIC_SCALE_MAX + 1e-6:
            dsum.out_of_range_hits += 1
        dsum.samples += 1

    prev_frame_ts = frames[0].ts
    sample_dynamic_scale()

    for fr in frames:
        frame_gap = fr.ts - prev_frame_ts
        if frame_gap > s.max_idle_gap_s:
            s.max_idle_gap_s = frame_gap
        if frame_gap >= sleep_timeout_s:
            s.idle_gap_hits += 1
        prev_frame_ts = fr.ts

        if fr.can_id == CAN_OEM_ID:
            s.oem_frames += 1
            if s.first_oem_ts is None:
                s.first_oem_ts = fr.ts
            col = decode_oem_color(fr.data)
            if col is not None:
                s.last_oem_color = col
                cur_oem_color = col
                cur_theme_index = 0
                cur_theme_id = resolve_theme_id(cur_oem_color, cur_theme_index)
            if len(fr.data) >= 1:
                br = fr.data[0]
                s.last_oem_brightness_raw = 5 if br > 5 else br
            if decode_nsi_active(fr.data):
                s.nsi_active_frames += 1
            sample_bank_character()
            sample_dynamic_scale()
        elif fr.can_id == CAN_MASTER_ID:
            s.master_frames += 1
            if s.first_oem_ts is None:
                s.master_before_first_oem += 1
            else:
                s.master_after_first_oem += 1
            mf = decode_master_fields(fr.data)
            if mf is not None:
                flags, bank_id, theme_idx, profile_raw = mf
                cur_night = 1 if (flags & 0x01) else 0
                cur_oem_color = bank_id
                cur_theme_index = theme_idx
                cur_theme_id = resolve_theme_id(cur_oem_color, cur_theme_index)
                cur_profile = profile_raw
                sample_bank_character()
                sample_dynamic_scale()
            else:
                sf = decode_sync_fields(fr.data)
                if sf is not None:
                    bank_id, theme_idx = sf
                    if bank_id <= 2:
                        cur_oem_color = bank_id
                        cur_theme_index = theme_idx
                        cur_theme_id = resolve_theme_id(cur_oem_color, cur_theme_index)
                        sample_dynamic_scale()
        elif fr.can_id == CAN_BSM_ID:
            s.bsm_frames += 1
            if decode_bsm_active(fr.data):
                s.bsm_active_frames += 1
        elif fr.can_id == CAN_HVAC_FAN_ID:
            if len(fr.data) >= 4:
                raw = fr.data[3]
                if raw < 0xF0:
                    level = raw / 255.0
                    cur_fan = cur_fan * 0.82 + level * 0.18
                    sample_bank_character()
                    sample_dynamic_scale()
        elif fr.can_id == CAN_LGTSENS_ID:
            night = decode_lgtsens_night(fr.data)
            if night is not None:
                cur_night = 1 if night else 0
                sample_dynamic_scale()
        elif fr.can_id == CAN_DRIVE_MODE_ID:
            p = decode_drive_mode_profile(fr.data)
            if p is not None:
                cur_profile = p
                sample_bank_character()
                sample_dynamic_scale()

    return s, dsum


def color_to_name(c: Optional[int]) -> str:
    if c is None:
        return "unknown"
    return {
        OEM_COLOR_AMBER: "amber",
        OEM_COLOR_WHITE: "white",
        OEM_COLOR_BLUE: "blue",
    }.get(c, "unknown")


def run() -> int:
    p = argparse.ArgumentParser(description="Offline CAN replay checker for STAR1 Ambient")
    p.add_argument("logfile", type=Path, help="Path to CAN log file (candump/ASC-like)")
    p.add_argument(
        "--sleep-timeout-sec",
        type=float,
        default=None,
        help="Sleep timeout threshold in seconds (defaults from selected profile)",
    )
    p.add_argument(
        "--profile",
        choices=[PROFILE_BENCH, PROFILE_PRODUCTION],
        default=PROFILE_BENCH,
        help="Target firmware profile for report context",
    )
    p.add_argument(
        "--watchdog-timeout-ms",
        type=int,
        default=None,
        help="Watchdog timeout for report context (defaults from selected profile)",
    )
    p.add_argument("--require-oem", action="store_true", help="Fail if no OEM 0x325 packet found")
    p.add_argument("--require-bsm", action="store_true", help="Fail if no active BSM frame found")
    p.add_argument(
        "--require-no-master-before-oem",
        action="store_true",
        help="Fail if any master frames appear before first OEM packet",
    )
    p.add_argument(
        "--require-master-after-oem",
        type=int,
        default=0,
        help="Require at least N master frames after first OEM packet",
    )
    p.add_argument(
        "--expect-color",
        choices=["amber", "white", "blue"],
        default=None,
        help="Fail if final decoded OEM color differs",
    )
    p.add_argument("--min-idle-gaps", type=int, default=0, help="Require at least N inactivity gaps >= timeout")
    p.add_argument(
        "--verify-dynamic-scale-cap",
        action="store_true",
        help="Replay-check dynamic scale hard-cap (theme*profile*bank*fan) and fail on out-of-range",
    )
    args = p.parse_args()

    if not args.logfile.exists():
        print(f"[ERR] File not found: {args.logfile}")
        return 2

    profile_cfg = PROFILE_DEFAULTS[args.profile]
    sleep_timeout_sec = (
        float(args.sleep_timeout_sec)
        if args.sleep_timeout_sec is not None
        else float(profile_cfg["sleep_timeout_sec"])
    )
    watchdog_timeout_ms = (
        int(args.watchdog_timeout_ms)
        if args.watchdog_timeout_ms is not None
        else int(profile_cfg["watchdog_timeout_ms"])
    )

    frames: List[CanFrame] = []
    unparsed = 0
    for line in args.logfile.read_text(encoding="utf-8", errors="ignore").splitlines():
        fr = parse_line(line)
        if fr is None:
            unparsed += 1
            continue
        frames.append(fr)

    frames.sort(key=lambda f: f.ts)
    s, dsum = analyze(frames, sleep_timeout_sec, args.verify_dynamic_scale_cap)

    print("=== STAR1 CAN Replay Report ===")
    print(f"log: {args.logfile}")
    print(
        "profile: "
        f"{args.profile} "
        f"(wake_policy={profile_cfg['can_wake_policy']}, "
        f"watchdog={watchdog_timeout_ms}ms, sleep_timeout={sleep_timeout_sec:.3f}s)"
    )
    print(f"frames parsed: {s.total_frames} (unparsed lines: {unparsed})")
    if s.total_frames > 0:
        print(f"time window: {s.first_ts:.3f}s .. {s.last_ts:.3f}s (len {s.last_ts - s.first_ts:.3f}s)")
    print(f"OEM 0x325 frames: {s.oem_frames}")
    print(f"MASTER 0x353 frames: {s.master_frames}")
    print(f"MASTER before first OEM: {s.master_before_first_oem}")
    print(f"MASTER after first OEM: {s.master_after_first_oem}")
    print(f"BSM 0x17E frames: {s.bsm_frames}")
    print(f"first OEM ts: {s.first_oem_ts if s.first_oem_ts is not None else 'n/a'}")
    print(f"last OEM color: {color_to_name(s.last_oem_color)}")
    print(f"last OEM brightness raw: {s.last_oem_brightness_raw if s.last_oem_brightness_raw is not None else 'n/a'}")
    print(f"NSI active frames: {s.nsi_active_frames}")
    print(f"BSM active frames: {s.bsm_active_frames}")
    print(f"max inactivity gap: {s.max_idle_gap_s:.3f}s")
    print(f"idle gaps >= {sleep_timeout_sec:.3f}s: {s.idle_gap_hits}")
    if dsum is not None:
        print("dynamic scale cap check:")
        print(f"  samples: {dsum.samples}")
        print(f"  raw min/max: {dsum.raw_min:.4f} / {dsum.raw_max:.4f}")
        print(f"  capped min/max: {dsum.capped_min:.4f} / {dsum.capped_max:.4f}")
        print(f"  cap hits low/high: {dsum.cap_low_hits} / {dsum.cap_high_hits}")
        print(f"  capped out-of-range hits: {dsum.out_of_range_hits}")

    failures: List[str] = []
    if args.require_oem and s.oem_frames == 0:
        failures.append("no OEM packets found")
    if args.require_bsm and s.bsm_active_frames == 0:
        failures.append("no active BSM frames found")
    if args.require_no_master_before_oem and s.master_before_first_oem > 0:
        failures.append(f"master before first OEM: {s.master_before_first_oem}")
    if args.require_master_after_oem > 0 and s.master_after_first_oem < args.require_master_after_oem:
        failures.append(
            f"master after first OEM {s.master_after_first_oem} < required {args.require_master_after_oem}"
        )
    if args.min_idle_gaps > 0 and s.idle_gap_hits < args.min_idle_gaps:
        failures.append(f"idle gap hits {s.idle_gap_hits} < required {args.min_idle_gaps}")
    if args.expect_color is not None:
        expected = {"amber": OEM_COLOR_AMBER, "white": OEM_COLOR_WHITE, "blue": OEM_COLOR_BLUE}[args.expect_color]
        if s.last_oem_color != expected:
            failures.append(
                f"final OEM color is {color_to_name(s.last_oem_color)}, expected {args.expect_color}"
            )
    if args.verify_dynamic_scale_cap:
        if AMB_THEME_DYNAMIC_SCALE_MIN > AMB_THEME_DYNAMIC_SCALE_MAX:
            failures.append(
                "invalid dynamic scale cap config: MIN > MAX "
                f"({AMB_THEME_DYNAMIC_SCALE_MIN} > {AMB_THEME_DYNAMIC_SCALE_MAX})"
            )
        if dsum is None or dsum.samples == 0:
            failures.append("dynamic scale cap check has no samples")
        elif dsum.out_of_range_hits > 0:
            failures.append(f"dynamic scale out-of-range hits: {dsum.out_of_range_hits}")

    if failures:
        print("\n[FAIL]")
        for msg in failures:
            print(f"- {msg}")
        return 1

    print("\n[OK] replay checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(run())
