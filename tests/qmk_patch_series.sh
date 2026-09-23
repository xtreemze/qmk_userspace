#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
qmk_dir="$repo_root/qmk_firmware"
os_patch="$repo_root/patches/0001-os-detection-fingerprint-trace.patch"
override_patch="$repo_root/patches/0003-repeat-key-override-weak-mods.patch"
combo_patch="$repo_root/patches/0004-combo-triggered-user-hook.patch"
resolver_patch="$repo_root/patches/0005-vial-alt-repeat-ram-resolver.patch"
vial_source="$qmk_dir/quantum/vial.c"
vial_header="$qmk_dir/quantum/vial.h"
override_source="$qmk_dir/quantum/process_keycode/process_key_override.c"
combo_source="$qmk_dir/quantum/process_keycode/process_combo.c"
combo_header="$qmk_dir/quantum/process_keycode/process_combo.h"
workflow="$repo_root/.github/workflows/build_binaries.yaml"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

[[ -d "$qmk_dir/.git" ]] || fail "pinned QMK checkout is required"
[[ -f "$os_patch" ]] || fail "OS fingerprint patch is missing"
[[ -f "$override_patch" ]] || fail "Repeat/Key Override weak-mod patch is missing"
[[ -f "$combo_patch" ]] || fail "Combo activation hook patch is missing"
[[ -f "$resolver_patch" ]] || fail "Vial Alternate Repeat RAM-resolver patch is missing"

if grep -q 'quantum/repeat_key.c' "$os_patch"; then
    fail "OS fingerprint patch must not contain Repeat-engine changes"
fi

grep -q 'vial_alt_repeat_key_resolve_direct' "$resolver_patch" || fail "Vial resolver patch must expose the coherent direct-match API"
grep -q 'bool vial_alt_repeat_key_resolve_direct' "$vial_source" || fail "patched Vial engine lacks direct-match resolver"
grep -q 'vial_alt_repeat_key_match_t' "$vial_header" || fail "Vial direct-match result is not declared publicly"
stock_body="$(sed -n '/uint16_t get_alt_repeat_key_keycode_user(/,/^}/p' "$vial_source")"
grep -q 'alt_repeat_key_normalize_keycode' <<<"$stock_body" || fail "stock Alternate Repeat normalization unexpectedly changed"
grep -q 'alt_repeat_key_mods_match' <<<"$stock_body" || fail "stock Alternate Repeat modifier policy unexpectedly changed"
resolver_body="$(sed -n '/bool vial_alt_repeat_key_resolve_direct(/,/^}/p' "$vial_source")"
grep -q 'alt_repeat_key_normalize_keycode' <<<"$resolver_body" || fail "public resolver must use Vial normalization"
if grep -q 'dynamic_keymap_get_alt_repeat_key' <<<"$resolver_body"; then
    fail "public resolver must not read the dynamic-keymap EEPROM path"
fi

grep -q 'd7ad3bf8aa05ead807984845480542affb3a054e' "$override_patch" || fail "Repeat/Key Override patch must record its upstream retirement commit"
grep -q 'elif defined(REPEAT_KEY_ENABLE)' "$override_source" || fail "patched Key Override engine lacks Repeat-specific weak-mod handling"
grep -q 'if (get_repeat_key_count())' "$override_source" || fail "patched Key Override engine does not gate weak mods on active Repeat"
grep -q 'effective_mods |= get_weak_mods();' "$override_source" || fail "patched Key Override engine does not include restored weak mods"

grep -q 'combo_triggered_user(uint16_t combo_index, uint16_t keycode)' "$combo_source" || fail "patched Combo engine lacks weak activation callback"
grep -q 'combo_triggered_user(combo_index, combo->keycode);' "$combo_source" || fail "patched Combo engine does not report the exact activated combo index"
grep -q 'combo_triggered_user(uint16_t combo_index, uint16_t keycode);' "$combo_header" || fail "Combo activation callback is not declared in process_combo.h"

helper_uses=$(grep -c 'bash scripts/apply-qmk-patches.sh qmk_firmware' "$workflow" || true)
[[ "$helper_uses" -eq 2 ]] || fail "regression and reusable-build jobs must share the patch-series helper"

mapfile -t patches < <(find "$repo_root/patches" -maxdepth 1 -type f -name '[0-9][0-9][0-9][0-9]-*.patch' -printf '%f\n' | sort)
expected='0001-os-detection-fingerprint-trace.patch 0003-repeat-key-override-weak-mods.patch 0004-combo-triggered-user-hook.patch 0005-vial-alt-repeat-ram-resolver.patch'
[[ "${patches[*]}" == "$expected" ]] || fail "numbered QMK patch series is unexpected: ${patches[*]}"

git -C "$qmk_dir" diff --check
printf 'QMK patch responsibilities and Vial RAM-resolver checks passed.\n'
