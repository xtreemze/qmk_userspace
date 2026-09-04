#!/usr/bin/env python3
"""Report deterministic static resource usage for qmk.json firmware targets."""

from __future__ import annotations

import json
import os
import pathlib
import subprocess
import sys
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
QMK_HOME = ROOT / "qmk_firmware"
QMK_JSON = ROOT / "qmk.json"
BASELINE = ROOT / "tests" / "firmware_resource_baseline.json"
OUTPUT = ROOT / "firmware-resources.json"


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def git_head(repo: pathlib.Path) -> str:
    result = subprocess.run(
        ["git", "-C", str(repo), "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def read_size(elf: pathlib.Path) -> tuple[int, int, int]:
    result = subprocess.run(
        ["arm-none-eabi-size", str(elf)],
        check=True,
        capture_output=True,
        text=True,
    )
    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if len(lines) < 2:
        fail(f"Unexpected arm-none-eabi-size output for {elf}: {result.stdout!r}")

    fields = lines[-1].split()
    if len(fields) < 6:
        fail(f"Unexpected size row for {elf}: {lines[-1]!r}")

    try:
        text_bytes, data_bytes, bss_bytes = (int(fields[index]) for index in range(3))
    except ValueError as error:
        fail(f"Non-numeric size row for {elf}: {lines[-1]!r} ({error})")

    return text_bytes, data_bytes, bss_bytes


def load_baseline() -> dict[str, Any]:
    if not BASELINE.exists():
        return {}
    data = json.loads(BASELINE.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1 or not isinstance(data.get("targets"), dict):
        fail(f"Unsupported resource baseline shape in {BASELINE}")
    return data


def format_delta(value: int | None) -> str:
    if value is None:
        return "—"
    return f"{value:+d}"


def main() -> None:
    qmk_data = json.loads(QMK_JSON.read_text(encoding="utf-8"))
    build_targets = qmk_data.get("build_targets")
    if not isinstance(build_targets, list) or not build_targets:
        fail("qmk.json must define at least one build target")

    baseline = load_baseline()
    baseline_targets = baseline.get("targets", {})
    rows: list[dict[str, Any]] = []

    for index, entry in enumerate(build_targets):
        if not isinstance(entry, list) or len(entry) != 3:
            fail(f"qmk.json build target {index} must have [keyboard, keymap, env] shape")

        keyboard, keymap, env = entry
        if not isinstance(env, dict):
            fail(f"qmk.json build target {index} environment must be an object")

        target = env.get("TARGET")
        if not isinstance(target, str) or not target:
            fail(f"qmk.json build target {index} is missing TARGET")

        module_flags = sorted(
            name for name, value in env.items() if name.startswith("HLC_") and value == "1"
        )
        if len(module_flags) != 1:
            fail(f"{target} must enable exactly one HLC_* module flag")

        elf = QMK_HOME / ".build" / f"{target}.elf"
        if not elf.is_file():
            candidates = sorted(path.name for path in (QMK_HOME / ".build").glob("*.elf"))
            fail(
                f"Expected ELF {elf.relative_to(ROOT)} was not produced. "
                f"Available ELFs: {candidates or 'none'}"
            )

        text_bytes, data_bytes, bss_bytes = read_size(elf)
        flash_bytes = text_bytes + data_bytes
        static_ram_bytes = data_bytes + bss_bytes

        baseline_row = baseline_targets.get(target)
        flash_delta = None
        static_ram_delta = None
        if isinstance(baseline_row, dict):
            try:
                flash_delta = flash_bytes - int(baseline_row["flash_bytes"])
                static_ram_delta = static_ram_bytes - int(baseline_row["static_ram_bytes"])
            except (KeyError, TypeError, ValueError) as error:
                fail(f"Invalid baseline entry for {target}: {error}")

        rows.append(
            {
                "target": target,
                "keyboard": keyboard,
                "keymap": keymap,
                "module": module_flags[0],
                "elf": str(elf.relative_to(ROOT)),
                "text_bytes": text_bytes,
                "data_bytes": data_bytes,
                "bss_bytes": bss_bytes,
                "flash_bytes": flash_bytes,
                "static_ram_bytes": static_ram_bytes,
                "baseline_flash_delta_bytes": flash_delta,
                "baseline_static_ram_delta_bytes": static_ram_delta,
            }
        )

    report = {
        "schema_version": 1,
        "userspace_sha": os.environ.get("GITHUB_SHA", ""),
        "vial_qmk_sha": git_head(QMK_HOME),
        "definitions": {
            "flash_bytes": "text + data from arm-none-eabi-size",
            "static_ram_bytes": "data + bss from arm-none-eabi-size; excludes runtime stack/heap high-water usage",
        },
        "baseline_file": str(BASELINE.relative_to(ROOT)) if BASELINE.exists() else None,
        "targets": rows,
    }
    OUTPUT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    header = (
        "| Target | Module | text | data | bss | Flash (text+data) | Δ flash | "
        "Static RAM (data+bss) | Δ RAM |\n"
        "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n"
    )
    body = "".join(
        "| {target} | {module} | {text_bytes} | {data_bytes} | {bss_bytes} | "
        "{flash_bytes} | {flash_delta} | {static_ram_bytes} | {ram_delta} |\n".format(
            **row,
            flash_delta=format_delta(row["baseline_flash_delta_bytes"]),
            ram_delta=format_delta(row["baseline_static_ram_delta_bytes"]),
        )
        for row in rows
    )
    note = (
        "\nStatic RAM is `data + bss` only; it does not include runtime stack/heap high-water usage. "
        "Resource reporting is observability, not physical/runtime headroom certification.\n"
    )

    print("Firmware resource usage:")
    print(header + body + note)

    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with pathlib.Path(summary_path).open("a", encoding="utf-8") as summary:
            summary.write("\n## Firmware resource usage\n\n")
            summary.write(header)
            summary.write(body)
            summary.write(note)


if __name__ == "__main__":
    main()
