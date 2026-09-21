#!/usr/bin/env python3
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs" / "configurator-protocol-v1.json"

DEFINE_RE = re.compile(r"^#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)\b", re.MULTILINE)
ENUM_ENTRY_RE = re.compile(
    r"^\s*(XTREEMZE_[A-Z0-9_]+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*,?\s*$",
    re.MULTILINE,
)


def parse_int(value: str) -> int:
    return int(value, 0)


def load_macros(text: str):
    return {name: parse_int(value) for name, value in DEFINE_RE.findall(text)}


def load_enum_values(text: str):
    return {name: parse_int(value) for name, value in ENUM_ENTRY_RE.findall(text)}


def fail(message: str):
    raise SystemExit(f"FAIL: {message}")


data = json.loads(CONTRACT.read_text(encoding="utf-8"))
if data.get("schema_version") != 1:
    fail("unexpected configurator protocol contract schema version")

namespaces = data.get("namespaces")
expected_namespaces = {"0xF0", "0xF1", "0xF2"}
if not isinstance(namespaces, dict) or set(namespaces) != expected_namespaces:
    fail("active contract must contain exactly 0xF0, 0xF1, and 0xF2")

for namespace_hex, spec in namespaces.items():
    source = ROOT / spec["source"]
    source_text = source.read_text(encoding="utf-8")
    combined_text = source_text

    header_path = spec.get("header")
    if header_path:
        combined_text += "\n" + (ROOT / header_path).read_text(encoding="utf-8")

    macros = load_macros(combined_text)
    enums = load_enum_values(combined_text)

    command_macro = spec["command_macro"]
    expected_command = int(namespace_hex, 16)
    actual_command = macros.get(command_macro)
    if actual_command != expected_command:
        fail(f"{command_macro} is {actual_command!r}; contract requires {expected_command:#04x}")

    version_macro = spec["protocol_version_macro"]
    expected_version = int(spec["protocol_version"])
    actual_version = macros.get(version_macro)
    if actual_version != expected_version:
        fail(f"{version_macro} is {actual_version!r}; contract requires {expected_version}")

    expected_ops = spec.get("operations", {})
    if not expected_ops:
        fail(f"{namespace_hex} has no operations in the contract")

    for name, expected_value in expected_ops.items():
        actual_value = enums.get(name)
        if actual_value != expected_value:
            fail(
                f"{namespace_hex} {name} is {actual_value!r}; "
                f"contract requires {expected_value:#04x}"
            )

display = namespaces["0xF2"]
layouts = display.get("packet_layouts", {})
if layouts.get("capabilities_response", {}).get("minimum_length") != 14:
    fail("0xF2 capability response length must match firmware")
if layouts.get("layer", {}).get("minimum_length") != 17:
    fail("0xF2 layer packet length must match firmware")
if layouts.get("modifier_label", {}).get("minimum_length") != 8:
    fail("0xF2 modifier-label packet length must match firmware")

print(
    "Configurator protocol contract: "
    f"{sum(len(v['operations']) for v in namespaces.values())} operations "
    "match firmware source across 0xF0/0xF1/0xF2."
)
