#!/usr/bin/env python3
"""Report deterministic firmware resource usage for qmk.json production targets."""

from __future__ import annotations

import json
import os
import pathlib
import subprocess
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
QMK_HOME = ROOT / "qmk_firmware"
QMK_JSON = ROOT / "qmk.json"
BASELINE = ROOT / "tests" / "firmware_resource_baseline.json"
OUTPUT = ROOT / "firmware-resources.json"


class ResourceError(RuntimeError):
    pass


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


def run_tool(arguments: list[str]) -> str:
    try:
        result = subprocess.run(arguments, check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError) as error:
        detail = getattr(error, "stderr", "") or getattr(error, "stdout", "") or str(error)
        raise ResourceError(f"{' '.join(arguments)} failed: {detail.strip()}") from error
    return result.stdout


def read_size(elf: pathlib.Path) -> tuple[int, int, int]:
    output = run_tool(["arm-none-eabi-size", str(elf)])
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    if len(lines) < 2:
        raise ResourceError(f"Unexpected arm-none-eabi-size output for {elf}: {output!r}")

    fields = lines[-1].split()
    if len(fields) < 6:
        raise ResourceError(f"Unexpected size row for {elf}: {lines[-1]!r}")

    try:
        values = tuple(int(fields[index]) for index in range(3))
    except ValueError as error:
        raise ResourceError(f"Non-numeric size row for {elf}: {lines[-1]!r}") from error
    return values[0], values[1], values[2]


def read_symbols(elf: pathlib.Path) -> dict[str, int]:
    output = run_tool(["arm-none-eabi-nm", "--defined-only", "--numeric-sort", str(elf)])
    symbols: dict[str, int] = {}
    for raw_line in output.splitlines():
        fields = raw_line.split()
        if len(fields) < 3:
            continue
        try:
            value = int(fields[0], 16)
        except ValueError:
            continue
        symbols[fields[-1]] = value
    return symbols


def require_symbol(symbols: dict[str, int], name: str, elf: pathlib.Path) -> int:
    try:
        return symbols[name]
    except KeyError as error:
        raise ResourceError(f"Required linker symbol {name} is missing from {elf}") from error


def span(symbols: dict[str, int], start: str, end: str, elf: pathlib.Path) -> int:
    start_value = require_symbol(symbols, start, elf)
    end_value = require_symbol(symbols, end, elf)
    if end_value < start_value:
        raise ResourceError(f"Invalid linker span {start}..{end} in {elf}")
    return end_value - start_value


def load_baseline() -> dict[str, Any]:
    if not BASELINE.exists():
        return {}
    data = json.loads(BASELINE.read_text(encoding="utf-8"))
    if data.get("schema_version") not in (1, 2) or not isinstance(data.get("targets"), dict):
        fail(f"Unsupported resource baseline shape in {BASELINE}")
    return data


def baseline_delta(row: dict[str, Any] | None, field: str, value: int) -> int | None:
    if not isinstance(row, dict) or field not in row:
        return None
    try:
        return value - int(row[field])
    except (TypeError, ValueError) as error:
        fail(f"Invalid baseline field {field}: {error}")


def format_delta(value: int | None) -> str:
    return "—" if value is None else f"{value:+d}"


def target_rows() -> list[tuple[str, str, dict[str, str]]]:
    qmk_data = json.loads(QMK_JSON.read_text(encoding="utf-8"))
    build_targets = qmk_data.get("build_targets")
    if not isinstance(build_targets, list) or not build_targets:
        fail("qmk.json must define at least one build target")

    rows: list[tuple[str, str, dict[str, str]]] = []
    for index, entry in enumerate(build_targets):
        if not isinstance(entry, list) or len(entry) != 3:
            fail(f"qmk.json build target {index} must have [keyboard, keymap, env] shape")
        keyboard, keymap, env = entry
        if not isinstance(keyboard, str) or not isinstance(keymap, str) or not isinstance(env, dict):
            fail(f"qmk.json build target {index} has invalid field types")
        if not all(isinstance(name, str) and isinstance(value, str) for name, value in env.items()):
            fail(f"qmk.json build target {index} environment must contain string pairs")
        rows.append((keyboard, keymap, env))
    return rows


def main() -> None:
    baseline = load_baseline()
    baseline_targets = baseline.get("targets", {})
    rows: list[dict[str, Any]] = []

    for index, (keyboard, keymap, env) in enumerate(target_rows()):
        target = env.get("TARGET")
        if not target:
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

        try:
            text_bytes, data_bytes, bss_bytes = read_size(elf)
            symbols = read_symbols(elf)

            size_flash_bytes = text_bytes + data_bytes
            size_static_ram_bytes = data_bytes + bss_bytes

            flash_binary_bytes = span(symbols, "__flash_binary_start", "__flash_binary_end", elf)
            flash_capacity_bytes = require_symbol(symbols, "__flash1_size__", elf)
            flash_headroom_bytes = flash_capacity_bytes - flash_binary_bytes

            ram0_base = require_symbol(symbols, "__ram0_base__", elf)
            ram0_capacity_bytes = require_symbol(symbols, "__ram0_size__", elf)
            heap_base = require_symbol(symbols, "__heap_base__", elf)
            heap_end = require_symbol(symbols, "__heap_end__", elf)
            ram0_static_bytes = heap_base - ram0_base
            ram0_heap_capacity_bytes = heap_end - heap_base

            core0_stack_reserved_bytes = span(symbols, "__main_stack_base__", "__main_stack_end__", elf) + span(
                symbols, "__process_stack_base__", "__process_stack_end__", elf
            )
            core0_stack_capacity_bytes = require_symbol(symbols, "__ram4_size__", elf)
            core0_stack_region_free_bytes = core0_stack_capacity_bytes - core0_stack_reserved_bytes

            core1_stack_reserved_bytes = span(symbols, "__c1_main_stack_base__", "__c1_main_stack_end__", elf) + span(
                symbols, "__c1_process_stack_base__", "__c1_process_stack_end__", elf
            )
            core1_stack_capacity_bytes = require_symbol(symbols, "__ram5_size__", elf)
            core1_stack_region_free_bytes = core1_stack_capacity_bytes - core1_stack_reserved_bytes
        except ResourceError as error:
            fail(str(error))

        if flash_headroom_bytes < 0:
            fail(f"{target} linker flash usage exceeds the flash1 region")
        if ram0_static_bytes < 0 or ram0_heap_capacity_bytes < 0:
            fail(f"{target} has invalid ram0 linker bounds")
        if ram0_static_bytes + ram0_heap_capacity_bytes != ram0_capacity_bytes:
            fail(f"{target} ram0 static + heap capacity does not equal the linker ram0 capacity")
        if core0_stack_region_free_bytes < 0 or core1_stack_region_free_bytes < 0:
            fail(f"{target} stack reservation exceeds its dedicated RP2040 stack region")

        baseline_row = baseline_targets.get(target)
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
                "size_flash_bytes": size_flash_bytes,
                "size_static_ram_bytes": size_static_ram_bytes,
                "flash_binary_bytes": flash_binary_bytes,
                "flash_capacity_bytes": flash_capacity_bytes,
                "flash_headroom_bytes": flash_headroom_bytes,
                "ram0_static_bytes": ram0_static_bytes,
                "ram0_capacity_bytes": ram0_capacity_bytes,
                "ram0_heap_capacity_bytes": ram0_heap_capacity_bytes,
                "core0_stack_reserved_bytes": core0_stack_reserved_bytes,
                "core0_stack_capacity_bytes": core0_stack_capacity_bytes,
                "core0_stack_region_free_bytes": core0_stack_region_free_bytes,
                "core1_stack_reserved_bytes": core1_stack_reserved_bytes,
                "core1_stack_capacity_bytes": core1_stack_capacity_bytes,
                "core1_stack_region_free_bytes": core1_stack_region_free_bytes,
                "baseline_size_flash_delta_bytes": baseline_delta(baseline_row, "flash_bytes", size_flash_bytes),
                "baseline_size_static_ram_delta_bytes": baseline_delta(
                    baseline_row, "static_ram_bytes", size_static_ram_bytes
                ),
                "baseline_flash_binary_delta_bytes": baseline_delta(
                    baseline_row, "flash_binary_bytes", flash_binary_bytes
                ),
                "baseline_ram0_static_delta_bytes": baseline_delta(
                    baseline_row, "ram0_static_bytes", ram0_static_bytes
                ),
            }
        )

    report = {
        "schema_version": 2,
        "userspace_sha": os.environ.get("GITHUB_SHA", ""),
        "vial_qmk_sha": git_head(QMK_HOME),
        "definitions": {
            "size_flash_bytes": "text + data from arm-none-eabi-size; retained for continuity",
            "size_static_ram_bytes": "data + bss from arm-none-eabi-size across linker regions; not a single RP2040 RAM pool",
            "flash_binary_bytes": "__flash_binary_end - __flash_binary_start",
            "flash_headroom_bytes": "flash1 linker capacity minus linked firmware span",
            "ram0_static_bytes": "__heap_base__ - __ram0_base__; statically occupied primary 256 KiB SRAM region",
            "ram0_heap_capacity_bytes": "__heap_end__ - __heap_base__; linker-unoccupied ram0 capacity available to the default heap",
            "core_stack_reserved_bytes": "linked main + process stack reservations in each dedicated 4 KiB RP2040 stack region",
            "core_stack_region_free_bytes": "dedicated stack-region capacity minus linked stack reservations; not runtime stack high-water headroom",
        },
        "baseline_file": str(BASELINE.relative_to(ROOT)) if BASELINE.exists() else None,
        "targets": rows,
    }
    OUTPUT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    header = (
        "| Target | Module | Flash span | Δ flash | Flash free | RAM0 static | Δ RAM0 | "
        "RAM0 heap capacity | Core0 stack reserve/free | Core1 stack reserve/free |\n"
        "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n"
    )
    body = "".join(
        "| {target} | {module} | {flash_binary_bytes} | {flash_delta} | {flash_headroom_bytes} | "
        "{ram0_static_bytes} | {ram0_delta} | {ram0_heap_capacity_bytes} | "
        "{core0_stack_reserved_bytes}/{core0_stack_region_free_bytes} | "
        "{core1_stack_reserved_bytes}/{core1_stack_region_free_bytes} |\n".format(
            **row,
            flash_delta=format_delta(row["baseline_flash_binary_delta_bytes"]),
            ram0_delta=format_delta(row["baseline_ram0_static_delta_bytes"]),
        )
        for row in rows
    )
    note = (
        "\nRP2040 linker regions are reported separately: `ram0` holds data/BSS/default heap, while `ram4` and "
        "`ram5` are dedicated stack regions. GNU `size` aggregate data/BSS remains in `firmware-resources.json` "
        "for continuity but must not be interpreted as one 256 KiB RAM pool. Stack free values are reservation "
        "capacity, not measured runtime high-water margin.\n"
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
