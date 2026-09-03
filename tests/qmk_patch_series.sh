#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
qmk_dir="$repo_root/qmk_firmware"
os_patch="$repo_root/patches/0001-os-detection-fingerprint-trace.patch"
repeat_patch="$repo_root/patches/0002-repeat-last-record-accessor.patch"
override_patch="$repo_root/patches/0003-repeat-key-override-weak-mods.patch"
repeat_source="$qmk_dir/quantum/repeat_key.c"
override_source="$qmk_dir/quantum/process_keycode/process_key_override.c"
workflow="$repo_root/.github/workflows/build_binaries.yaml"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

[[ -d "$qmk_dir/.git" ]] || fail "pinned QMK checkout is required"
[[ -f "$os_patch" ]] || fail "OS fingerprint patch is missing"
[[ -f "$repeat_patch" ]] || fail "Repeat last-record accessor patch is missing"
[[ -f "$override_patch" ]] || fail "Repeat/Key Override weak-mod patch is missing"

if grep -q 'quantum/repeat_key.c' "$os_patch"; then
    fail "OS fingerprint patch must not contain Repeat-engine changes"
fi

grep -q '721affff7b2ca2aafcef3092a707b0ff1196dfb1' "$repeat_patch" || fail "Repeat accessor patch must record its upstream retirement commit"
grep -q 'keyrecord_t\* get_last_record(void)' "$repeat_source" || fail "patched Repeat engine lacks get_last_record implementation"
grep -q 'return &last_record;' "$repeat_source" || fail "get_last_record implementation does not expose native Repeat state"

grep -q 'd7ad3bf8aa05ead807984845480542affb3a054e' "$override_patch" || fail "Repeat/Key Override patch must record its upstream retirement commit"
grep -q 'elif defined(REPEAT_KEY_ENABLE)' "$override_source" || fail "patched Key Override engine lacks Repeat-specific weak-mod handling"
grep -q 'if (get_repeat_key_count())' "$override_source" || fail "patched Key Override engine does not gate weak mods on active Repeat"
grep -q 'effective_mods |= get_weak_mods();' "$override_source" || fail "patched Key Override engine does not include restored weak mods"

helper_uses=$(grep -c 'bash scripts/apply-qmk-patches.sh qmk_firmware' "$workflow" || true)
[[ "$helper_uses" -eq 2 ]] || fail "regression and reusable-build jobs must share the patch-series helper"

mapfile -t patches < <(find "$repo_root/patches" -maxdepth 1 -type f -name '[0-9][0-9][0-9][0-9]-*.patch' -printf '%f\n' | sort)
expected='0001-os-detection-fingerprint-trace.patch 0002-repeat-last-record-accessor.patch 0003-repeat-key-override-weak-mods.patch'
[[ "${patches[*]}" == "$expected" ]] || fail "numbered QMK patch series is unexpected: ${patches[*]}"

git -C "$qmk_dir" diff --check
printf 'QMK patch responsibilities and upstream backport checks passed.\n'
