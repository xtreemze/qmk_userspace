#!/usr/bin/env python3
"""Compare the legacy keyboard's compiled data contracts, not hardware behavior.

Build each revision with the same QMK/toolchain/module flags, then pass the
baseline and candidate ELF files. No third-party Python packages are needed.
"""

import argparse
from pathlib import Path
import struct


SYMBOLS = (
    "keymaps",
    "encoder_map",
    "g_led_config",
    "keyboard_definition",
    "chordal_hold_layout",
    "xtreemze_default_alt_repeat_keys",
    "xtreemze_default_combos",
    "xtreemze_default_key_overrides",
    "xtreemze_default_tap_dances",
    "xtreemze_qmk_settings_defaults",
    "DeviceDescriptor",
    "ConfigurationDescriptor",
    "KeyboardReport",
    "RawReport",
    "SharedReport",
    "SerialNumberString",
)


def read_contracts(path):
    elf = path.read_bytes()
    if elf[:6] != b"\x7fELF\x01\x01":
        raise ValueError(f"{path}: expected little-endian ELF32 firmware")
    shoff = struct.unpack_from("<I", elf, 32)[0]
    shentsize, shnum = struct.unpack_from("<HH", elf, 46)
    sections = [struct.unpack_from("<10I", elf, shoff + i * shentsize) for i in range(shnum)]
    contracts = {}
    for section in sections:
        if section[1] != 2:  # SHT_SYMTAB
            continue
        string_section = sections[section[6]]
        strings = elf[string_section[4]:string_section[4] + string_section[5]]
        for offset in range(section[4], section[4] + section[5], section[9]):
            name_offset, address, size, _, _, index = struct.unpack_from("<IIIBBH", elf, offset)
            name = strings[name_offset:strings.index(b"\0", name_offset)].decode()
            if name not in SYMBOLS:
                continue
            storage = sections[index]
            relative = address - storage[3]
            if storage[1] == 8 or relative < 0 or relative + size > storage[5]:
                raise ValueError(f"{path}: {name} has no complete initialized data")
            start = storage[4] + relative
            contracts[name] = elf[start:start + size]
    for name in SYMBOLS:
        if name not in contracts:
            raise ValueError(f"{path}: missing symbol {name}; use an unstripped ELF")
    return contracts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    baseline = read_contracts(args.baseline)
    candidate = read_contracts(args.candidate)
    for name, size in (("keymaps", 13 * 10 * 5 * 2), ("encoder_map", 13 * 2 * 2 * 2)):
        if len(baseline[name]) != size:
            raise ValueError(f"{name}: baseline does not match the 13-layer legacy contract")
    for name in SYMBOLS:
        if baseline[name] != candidate[name]:
            raise ValueError(f"{name}: compiled data differs from the debounce baseline")
        print(f"{name}: identical ({len(candidate[name])} bytes)")


if __name__ == "__main__":
    main()
