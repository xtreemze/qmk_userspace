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
GENERATED_PATH_PARTS = ("/graphics/fonts/", "/graphics/numbers/")
DISPLAY_OWNER = "users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.c"
HOST_OWNER = "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"
RGB_MIGRATION_READER = "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/rgb_profile_protocol.c"

FORBIDDEN = (
    (re.compile(r"\b(?:malloc|calloc|realloc|free)\s*\("), "heap-allocation",
     "Heap allocation is forbidden in keyboard runtime code; use static/fixed-capacity ownership."),
    (re.compile(r"\b(?:strcpy|strcat|sprintf|vsprintf|gets)\s*\("), "unsafe-string-api",
     "Unbounded string APIs are forbidden; use bounded formatting/copying."),
    (re.compile(r"\bwait_(?:ms|us)\s*\("), "blocking-delay",
     "Blocking waits are forbidden in runtime userspace; use timer-driven state machines."),
    (re.compile(r"\b(?:NOLINT|cppcheck-suppress)\b|clang-format\s+off|#\s*pragma\s+GCC\s+diagnostic\s+ignored"),
     "lint-suppression", "Inline/static-analysis suppressions are forbidden in authored firmware; fix or model the invariant."),
    (re.compile(r"timer_read(?:32)?\s*\(\s*\)\s*-"), "raw-timer-subtraction",
     "Use QMK timer_elapsed/timer_elapsed32 helpers so wraparound semantics remain correct."),
)

def owned_files() -> list[Path]:
    return sorted({
        path
        for pattern in OWNED_GLOBS
        for path in ROOT.glob(pattern)
        if path.is_file()
        and not any(part in "/" + path.relative_to(ROOT).as_posix() for part in GENERATED_PATH_PARTS)
    })

def violation(violations: list[str], rel: str, line_number: int, rule: str, message: str) -> None:
    violations.append(f"{rel}:{line_number}: {rule}: {message}")

def main() -> int:
    violations: list[str] = []
    for path in owned_files():
        rel = path.relative_to(ROOT).as_posix()
        source = path.read_text(encoding="utf-8")
        for line_number, line in enumerate(source.splitlines(), start=1):
            for pattern, rule, message in FORBIDDEN:
                if pattern.search(line):
                    violation(violations, rel, line_number, rule, message)

            if re.search(r"\bqp_(?:power|flush|init|close)\s*\(", line) and rel != DISPLAY_OWNER:
                violation(
                    violations, rel, line_number, "display-power-owner",
                    "Quantum Painter lifecycle/power operations belong to the TFT display module only.",
                )

            if re.search(r"\bdetected_host_os\s*\(", line) and rel != HOST_OWNER:
                violation(
                    violations, rel, line_number, "host-detection-owner",
                    "Host OS detection belongs to the keymap host-family resolver; consumers use translated state.",
                )

            if re.search(r"\beeconfig_update_user_datablock\s*\(", line) and rel != HOST_OWNER:
                violation(
                    violations, rel, line_number, "eeprom-write-owner",
                    "Legacy userspace datablock writes belong to the canonical keymap state owner.",
                )

            if (
                re.search(r"\beeconfig_read_user_datablock\s*\(", line)
                and rel not in {HOST_OWNER, RGB_MIGRATION_READER}
            ):
                violation(
                    violations, rel, line_number, "eeprom-read-owner",
                    "Legacy userspace datablock reads are restricted to the state owner and explicit RGB migration reader.",
                )

    if violations:
        print("QMK userspace anti-pattern policy violations:", file=sys.stderr)
        for item in violations:
            print(f"  {item}", file=sys.stderr)
        return 1

    print(f"QMK userspace anti-pattern policy: clean ({len(owned_files())} authored files checked)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
