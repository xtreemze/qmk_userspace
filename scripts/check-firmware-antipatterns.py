#!/usr/bin/env python3
"""Reject high-confidence embedded-C anti-patterns in repository-owned firmware."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOTS = ("keyboards", "users", "converters")
SOURCE_SUFFIXES = {".c", ".h", ".inc"}

RULES = (
    (
        "heap-allocation",
        re.compile(r"\b(?:malloc|calloc|realloc|free)\s*\("),
        "Heap allocation is forbidden in repository-owned firmware; use bounded static storage.",
    ),
    (
        "unbounded-string-api",
        re.compile(r"\b(?:strcpy|strcat|sprintf|vsprintf|gets)\s*\("),
        "Unbounded legacy string APIs are forbidden; use bounded alternatives and explicit sizes.",
    ),
)


def iter_sources() -> list[Path]:
    sources: list[Path] = []
    for root_name in SOURCE_ROOTS:
        root = ROOT / root_name
        if not root.exists():
            continue
        sources.extend(
            path
            for path in root.rglob("*")
            if path.is_file() and path.suffix in SOURCE_SUFFIXES
        )
    return sorted(sources)


def main() -> int:
    violations: list[str] = []

    for path in iter_sources():
        relative_path = path.relative_to(ROOT)
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            for rule_name, pattern, message in RULES:
                if pattern.search(line):
                    violations.append(
                        f"{relative_path}:{line_number}: {rule_name}: {message}"
                    )

    if violations:
        print("Firmware anti-pattern policy violations:", file=sys.stderr)
        for violation in violations:
            print(f"  {violation}", file=sys.stderr)
        return 1

    print("Firmware anti-pattern policy: clean")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
