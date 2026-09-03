#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
qmk_dir="$repo_root/qmk_firmware"
source_file="$qmk_dir/quantum/process_keycode/process_key_override.c"
patch_file="$repo_root/patches/0002-repeat-key-override-weak-mods.patch"
workflow="$repo_root/.github/workflows/build_binaries.yaml"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

[[ -d "$qmk_dir/.git" ]] || fail "pinned QMK checkout is required"
[[ -f "$patch_file" ]] || fail "Repeat/Key Override backport patch is missing"

grep -q 'd7ad3bf8aa05ead807984845480542affb3a054e' "$patch_file" || fail "backport must record its upstream retirement commit"
grep -q 'elif defined(REPEAT_KEY_ENABLE)' "$source_file" || fail "patched Key Override engine lacks Repeat-specific weak-mod handling"
grep -q 'if (get_repeat_key_count())' "$source_file" || fail "patched Key Override engine does not gate weak mods on active Repeat"
grep -q 'effective_mods |= get_weak_mods();' "$source_file" || fail "patched Key Override engine does not include restored weak mods"

helper_uses=$(grep -c 'bash scripts/apply-qmk-patches.sh qmk_firmware' "$workflow" || true)
[[ "$helper_uses" -eq 2 ]] || fail "regression and reusable-build jobs must share the patch-series helper"

mapfile -t patches < <(find "$repo_root/patches" -maxdepth 1 -type f -name '[0-9][0-9][0-9][0-9]-*.patch' -printf '%f\n' | sort)
[[ "${patches[*]}" == '0001-os-detection-fingerprint-trace.patch 0002-repeat-key-override-weak-mods.patch' ]] || fail "numbered QMK patch series is unexpected: ${patches[*]}"

git -C "$qmk_dir" diff --check
printf 'QMK patch series and Repeat/Key Override backport checks passed.\n'
