#!/usr/bin/env python3
"""Validate the checked-in firmware resource baseline shape and provenance."""

from __future__ import annotations

import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
QMK_JSON = ROOT / "qmk.json"
BASELINE = ROOT / "tests" / "firmware_resource_baseline.json"
WORKFLOW = ROOT / ".github" / "workflows" / "build_binaries.yaml"

EXPECTED_FLASH_CAPACITY = 2 * 1024 * 1024
EXPECTED_RAM0_CAPACITY = 256 * 1024
EXPECTED_STACK_REGION_CAPACITY = 4 * 1024


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def require_non_negative_int(row: dict[str, object], target: str, field: str) -> int:
    value = row.get(field)
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        fail(f"baseline {target}.{field} must be a non-negative integer")
    return value


def main() -> None:
    qmk = json.loads(QMK_JSON.read_text(encoding="utf-8"))
    baseline = json.loads(BASELINE.read_text(encoding="utf-8"))

    if baseline.get("schema_version") != 2:
        fail("firmware resource baseline schema_version must be 2")

    integration_commit = baseline.get("integration_commit")
    if not isinstance(integration_commit, str) or not re.fullmatch(r"[0-9a-f]{40}", integration_commit):
        fail("firmware resource baseline must record a full integration commit SHA")

    vial_qmk_sha = baseline.get("vial_qmk_sha")
    if not isinstance(vial_qmk_sha, str) or not re.fullmatch(r"[0-9a-f]{40}", vial_qmk_sha):
        fail("firmware resource baseline must record a full Vial-QMK SHA")

    build_targets = qmk.get("build_targets")
    if not isinstance(build_targets, list) or not build_targets:
        fail("qmk.json must define production build targets")

    expected_targets: set[str] = set()
    for index, entry in enumerate(build_targets):
        if not isinstance(entry, list) or len(entry) != 3 or not isinstance(entry[2], dict):
            fail(f"qmk.json build target {index} must have [keyboard, keymap, env] shape")
        target = entry[2].get("TARGET")
        if not isinstance(target, str) or not target:
            fail(f"qmk.json build target {index} is missing TARGET")
        if target in expected_targets:
            fail(f"qmk.json contains duplicate TARGET {target}")
        expected_targets.add(target)

    baseline_targets = baseline.get("targets")
    if not isinstance(baseline_targets, dict):
        fail("firmware resource baseline targets must be an object")
    if set(baseline_targets) != expected_targets:
        missing = sorted(expected_targets - set(baseline_targets))
        extra = sorted(set(baseline_targets) - expected_targets)
        fail(f"resource baseline target set differs from qmk.json; missing={missing}, extra={extra}")

    required_fields = (
        "text_bytes",
        "data_bytes",
        "bss_bytes",
        "flash_bytes",
        "static_ram_bytes",
        "flash_binary_bytes",
        "flash_capacity_bytes",
        "flash_headroom_bytes",
        "ram0_static_bytes",
        "ram0_capacity_bytes",
        "ram0_heap_capacity_bytes",
        "core0_stack_reserved_bytes",
        "core0_stack_capacity_bytes",
        "core0_stack_region_free_bytes",
        "core1_stack_reserved_bytes",
        "core1_stack_capacity_bytes",
        "core1_stack_region_free_bytes",
    )

    for target, row_value in baseline_targets.items():
        if not isinstance(target, str) or not isinstance(row_value, dict):
            fail(f"baseline row for {target} must be an object")
        row = row_value
        values = {field: require_non_negative_int(row, target, field) for field in required_fields}

        if values["flash_bytes"] != values["text_bytes"] + values["data_bytes"]:
            fail(f"baseline {target} flash_bytes must equal text_bytes + data_bytes")
        if values["static_ram_bytes"] != values["data_bytes"] + values["bss_bytes"]:
            fail(f"baseline {target} static_ram_bytes must equal data_bytes + bss_bytes")

        if values["flash_capacity_bytes"] != EXPECTED_FLASH_CAPACITY:
            fail(f"baseline {target} flash capacity must match the pinned RP2040 2 MiB linker region")
        if values["flash_binary_bytes"] + values["flash_headroom_bytes"] != values["flash_capacity_bytes"]:
            fail(f"baseline {target} flash binary + headroom must equal flash capacity")

        if values["ram0_capacity_bytes"] != EXPECTED_RAM0_CAPACITY:
            fail(f"baseline {target} ram0 capacity must match the pinned RP2040 256 KiB region")
        if values["ram0_static_bytes"] + values["ram0_heap_capacity_bytes"] != values["ram0_capacity_bytes"]:
            fail(f"baseline {target} ram0 static + heap capacity must equal ram0 capacity")

        for core in (0, 1):
            reserved = values[f"core{core}_stack_reserved_bytes"]
            capacity = values[f"core{core}_stack_capacity_bytes"]
            free = values[f"core{core}_stack_region_free_bytes"]
            if capacity != EXPECTED_STACK_REGION_CAPACITY:
                fail(f"baseline {target} core{core} stack capacity must match the pinned 4 KiB region")
            if reserved + free != capacity:
                fail(f"baseline {target} core{core} stack reserved + free must equal capacity")

    workflow_text = WORKFLOW.read_text(encoding="utf-8")
    vial_refs = set(re.findall(r"(?m)^\s+ref:\s+([0-9a-f]{40})\s*$", workflow_text))
    if vial_refs != {vial_qmk_sha}:
        fail(
            "resource baseline Vial-QMK SHA must match the exact workflow checkout pin; "
            f"baseline={vial_qmk_sha}, workflow={sorted(vial_refs)}"
        )

    print(
        f"Firmware resource baseline v2: {len(expected_targets)} targets, integration {integration_commit[:12]}, "
        f"Vial-QMK {vial_qmk_sha[:12]}."
    )


if __name__ == "__main__":
    main()
