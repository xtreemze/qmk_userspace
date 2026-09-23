#!/usr/bin/env python3
"""Require immutable commit-SHA refs for every external GitHub Action."""

from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"
USES = re.compile(r"(?m)^\s*uses:\s*([^\s#]+)")
FULL_SHA = re.compile(r"^[0-9a-f]{40}$")


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


checked = 0
for workflow in sorted([*WORKFLOWS.glob("*.yml"), *WORKFLOWS.glob("*.yaml")]):
    text = workflow.read_text(encoding="utf-8")
    for match in USES.finditer(text):
        action = match.group(1)
        if action.startswith("./") or action.startswith("docker://"):
            continue
        if "@" not in action:
            fail(f"{workflow.relative_to(ROOT)}: external action has no ref: {action}")
        name, ref = action.rsplit("@", 1)
        if not name or not FULL_SHA.fullmatch(ref):
            fail(
                f"{workflow.relative_to(ROOT)}: {action} must use a full 40-character "
                "commit SHA, not a mutable tag/branch"
            )
        checked += 1

if checked == 0:
    fail("no external GitHub Actions were discovered")

print(f"Workflow supply-chain policy: {checked} external action references are commit-SHA pinned.")
