#!/usr/bin/env python3
"""Enforce xtreemze QMK userspace architecture and embedded-safety policy."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OWNED_GLOBS = (
    "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/**/*.c",
    "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/**/*.h",
    "users/halcyon_modules/splitkb/hlc_tft_display/**/*.c",
    "users/halcyon_modules/splitkb/hlc_tft_display/**/*.h",
)
DISPLAY_OWNER = "users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.c"
HOST_OWNER_SUFFIX = "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"

FORBIDDEN = (
    (re.compile(r"\b(?:malloc|calloc|realloc|free)\s*\("), "heap-allocation",
     "Heap allocation is forbidden in keyboard runtime code; use static/fixed-capacity ownership."),
    (re.compile(r"\b(?:strcpy|strcat|sprintf|vsprintf|gets)\s*\("), "unsafe-string-api",
     "Unbounded string APIs are forbidden; use bounded formatting/copying."),
    (re.compile(r"\bwait_(?:ms|us)\s*\("), "blocking-delay",
     "Blocking waits are forbidden in runtime userspace; use timer-driven state machines."),
    (re.compile(r"\b(?:NOLINT|cppcheck-suppress)\b|clang-format\s+off|#\s*pragma\s+GCC\s+diagnostic\s+ignored"),
     "lint-suppression", "Inline/static-analysis suppressions are forbidden; fix or model the invariant."),
    (re.compile(r"timer_read(?:32)?\s*\(\s*\)\s*-"), "raw-timer-subtraction",
     "Use QMK timer_elapsed/timer_elapsed32 helpers so wraparound semantics remain correct."),
)

OWNER_RULES = (
    (re.compile(r"\bqp_(?:power|flush|init|close)\s*\("), DISPLAY_OWNER, "display-power-owner",
     "Quantum Painter lifecycle/power operations belong to the TFT display module only."),
    (re.compile(r"\bdetected_host_os\s*\("), HOST_OWNER_SUFFIX, "host-detection-owner",
     "Host OS detection belongs to the keymap host-family resolver; consumers use translated state."),
    (re.compile(r"\beeconfig_(?:update|read)_user_datablock\s*\("), HOST_OWNER_SUFFIX, "eeprom-owner",
     "Userspace EEPROM persistence belongs to the canonical keymap state owner."),
)

def owned_files() -> list[Path]:
    return sorted({
        path
        for pattern in OWNED_GLOBS
        for path in ROOT.glob(pattern)
        if path.is_file()
    })

def main() -> int:
    violations: list[str] = []
    for path in owned_files():
        rel = path.relative_to(ROOT).as_posix()
        source = path.read_text(encoding="utf-8")
        for line_number, line in enumerate(source.splitlines(), start=1):
            for pattern, rule, message in FORBIDDEN:
                if pattern.search(line):
                    violations.append(f"{rel}:{line_number}: {rule}: {message}")
            for pattern, owner, rule, message in OWNER_RULES:
                if pattern.search(line) and rel != owner:
                    violations.append(f"{rel}:{line_number}: {rule}: {message}")

    if violations:
        print("QMK userspace anti-pattern policy violations:", file=sys.stderr)
        for violation in violations:
            print(f"  {violation}", file=sys.stderr)
        return 1

    print(f"QMK userspace anti-pattern policy: clean ({len(owned_files())} files checked)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
