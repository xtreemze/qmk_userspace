#!/usr/bin/env python3
"""Guard the custom user-data schema against accidental destructive version bumps."""

from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
KEYMAP = ROOT / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"
CONFIG = ROOT / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/config.h"
POLICY = ROOT / "docs/USER_DATA_MIGRATIONS.md"

EXPECTED_SCHEMA_VERSION = 0x02


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def function(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        fail(f"missing function {signature}")
    brace = source.find("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    fail(f"unterminated function {signature}")
    return ""


source = KEYMAP.read_text(encoding="utf-8")
config = CONFIG.read_text(encoding="utf-8")
policy = POLICY.read_text(encoding="utf-8")

match = re.search(r"^#define XTREEMZE_USER_DATA_VERSION (0x[0-9A-Fa-f]+)$", source, re.MULTILINE)
if match is None:
    fail("missing XTREEMZE_USER_DATA_VERSION")
version = int(match.group(1), 16)
if version != EXPECTED_SCHEMA_VERSION:
    fail(
        "user-data schema version changed; add an explicit migration or documented "
        "destructive decision, then update this regression and migration table"
    )

size_match = re.search(r"^#define EECONFIG_USER_DATA_SIZE (\d+)$", config, re.MULTILINE)
if size_match is None or int(size_match.group(1)) != 128:
    fail("expected the reviewed 128-byte QMK user datablock allocation")

if "_Static_assert(sizeof(xtreemze_user_data_t) <= EECONFIG_USER_DATA_SIZE" not in source:
    fail("custom user-data type must be compile-time bounded by the QMK datablock")

load = function(source, "static void load_user_data(void)")
save = function(source, "static void save_user_data(void)")
post_init = function(source, "void keyboard_post_init_user(void)")

if "eeconfig_read_user_datablock" not in load or "!= sizeof(xtreemze_user_data)" not in load:
    fail("full user-data reads must verify the exact transferred byte count")
if "xtreemze_user_data.magic != XTREEMZE_USER_DATA_MAGIC" not in load:
    fail("foreign/corrupt magic must have an explicit policy")
if "xtreemze_user_data.version != XTREEMZE_USER_DATA_VERSION" not in load:
    fail("unsupported schema versions must have an explicit policy")

version_branch = load.split("xtreemze_user_data.version != XTREEMZE_USER_DATA_VERSION", 1)[1]
version_branch = version_branch.split("if (xtreemze_user_data.chord_override_ms", 1)[0]
for required in ("set_user_data_defaults(&xtreemze_user_data);", "user_data_schema_writable = false;", "return;"):
    if required not in version_branch:
        fail(f"unsupported-version path must contain {required}")
for forbidden in ("save_user_data();", "eeconfig_update_user_datablock"):
    if forbidden in version_branch:
        fail("unsupported-version path must preserve the raw EEPROM datablock")

if "if (!user_data_schema_writable)" not in save:
    fail("all later persistence must fail closed for an unsupported schema")
if "eeconfig_update_user_datablock" not in save or "!= sizeof(xtreemze_user_data)" not in save:
    fail("full user-data writes must verify the exact transferred byte count")

if "if (user_data_schema_writable)" not in post_init:
    fail("factory-default synchronization must be gated by validated current-schema ownership")
guarded = post_init.split("if (user_data_schema_writable)", 1)[1]
if "sync_compiled_defaults_to_dynamic_keymap_once();" not in guarded:
    fail("current-schema boots must still perform the normal one-shot factory sync")

if "| `0x02` | Current; first versioned schema" not in policy:
    fail("migration policy must document the current first versioned schema")
if "There is no synthetic `0x01` migration." not in policy:
    fail("migration policy must not invent a historical schema")
if "must not trigger compiled Vial factory reseeding" not in policy:
    fail("migration policy must protect Vial dynamic configuration from schema mismatch")

print("User-data schema v0x02: bounded storage, exact I/O, non-destructive unsupported-version policy, and Vial reseed guard pass.")
