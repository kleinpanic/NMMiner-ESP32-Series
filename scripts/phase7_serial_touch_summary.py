#!/usr/bin/env python3
"""Summarize Phase 7 touch-probe serial logs without copying raw logs into docs."""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from pathlib import Path


TOUCH_RE = re.compile(
    r"(\[nm_touch\]|directX=|directY=|directPressed=|libPressed=|irq=|miso=)"
)
PRESSED_RE = re.compile(r"\b(?:directPressed|libPressed)\s*=\s*1\b")
IRQ_LOW_RE = re.compile(r"\birq\s*=\s*0\b")
PANIC_RE = re.compile(r"(Guru Meditation|PANIC|Backtrace|brownout|abort)", re.I)
RESET_RE = re.compile(r"\brst:0x", re.I)
AP_RE = re.compile(
    r"(nmap-2\.4g|192\.168\.4\.1|StartAP|Configuration Mode|Entered Configuration|Config IP Address)",
    re.I,
)

SENSITIVE_PATTERNS = (
    (re.compile(r"bc1[ac-hj-np-z02-9]{20,}", re.I), "bc1...[redacted]"),
    (
        re.compile(
            r"(\b(?:password|pass|wifi_password|ssid|wallet|address|token|secret)\b\s*[:=]\s*)"
            r"(\"[^\"]*\"|'[^']*'|[^,\s}]+)",
            re.I,
        ),
        r"\1[redacted]",
    ),
)


def sanitize(line: str) -> str:
    sanitized = line.rstrip("\r\n")
    for pattern, replacement in SENSITIVE_PATTERNS:
        sanitized = pattern.sub(replacement, sanitized)
    if len(sanitized) > 240:
        return sanitized[:237] + "..."
    return sanitized


def matching_lines(pattern: re.Pattern[str], lines: list[str]) -> list[str]:
    return [line for line in lines if pattern.search(line)]


def summarize_log(path: Path, last_touch: int) -> int:
    data = path.read_bytes()
    text = data.decode("utf-8", errors="replace")
    lines = text.splitlines()
    touch_lines = matching_lines(TOUCH_RE, lines)

    print(f"file: {path}")
    print(f"bytes: {len(data)}")
    print(f"lines: {len(lines)}")
    print(f"sha256: {hashlib.sha256(data).hexdigest()}")
    print(f"touch_diagnostics: {len(touch_lines)}")
    print(f"pressed_observations: {len(matching_lines(PRESSED_RE, touch_lines))}")
    print(f"irq_low_observations: {len(matching_lines(IRQ_LOW_RE, touch_lines))}")
    print(f"panic_markers: {len(matching_lines(PANIC_RE, lines))}")
    print(f"reset_markers: {len(matching_lines(RESET_RE, lines))}")
    print(f"ap_config_markers: {len(matching_lines(AP_RE, lines))}")

    if last_touch > 0 and touch_lines:
        print("last_touch_lines:")
        for line in touch_lines[-last_touch:]:
            print(f"  {sanitize(line)}")

    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Print derived Phase 7 touch-probe counts from serial logs. "
            "Use the counts in planning docs; keep raw logs out of git."
        )
    )
    parser.add_argument("logs", nargs="+", type=Path, help="Serial log file(s) to summarize")
    parser.add_argument(
        "--last-touch",
        type=int,
        default=3,
        help="Print this many sanitized trailing touch diagnostic lines; use 0 for none",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    exit_status = 0

    for index, path in enumerate(args.logs):
        if index:
            print()
        try:
            summarize_log(path, args.last_touch)
        except OSError as exc:
            exit_status = 1
            print(f"error: {path}: {exc}", file=sys.stderr)

    return exit_status


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
