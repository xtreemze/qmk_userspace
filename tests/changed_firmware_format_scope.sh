#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

checker="scripts/check-changed-firmware-format.sh"
workflow=".github/workflows/build_binaries.yaml"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

[[ -f "$checker" ]] || fail "changed-file format checker is missing"

actual="$({
    bash "$checker" --classify \
        users/halcyon_modules/splitkb/halcyon.c \
        users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.h \
        users/halcyon_modules/splitkb/hlc_tft_display/graphics/fonts/Retron2000-27.qff.c \
        converters/promicro_to_halcyon/_pin_defs.h \
        keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/halcyon_overrides.c \
        keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c \
        examples/display/display.c \
        README.md
} )"

expected="$(cat <<'EOF'
users/halcyon_modules/splitkb/halcyon.c
users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.h
converters/promicro_to_halcyon/_pin_defs.h
keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/halcyon_overrides.c
EOF
)"

[[ "$actual" == "$expected" ]] || {
    printf 'Expected formatter scope:\n%s\n' "$expected" >&2
    printf 'Actual formatter scope:\n%s\n' "$actual" >&2
    fail "changed-file formatter ownership rules drifted"
}

grep -q 'scripts/check-changed-firmware-format.sh' "$workflow" || fail "firmware build must invoke the changed-file formatter"
grep -q 'github.event.pull_request.base.sha' "$workflow" || fail "workflow must pass the exact PR base SHA"
grep -q 'github.event.pull_request.head.sha' "$workflow" || fail "workflow must pass the exact PR head SHA"
grep -q 'github.event.pull_request.number' "$workflow" || fail "workflow must pass the PR number for merge-ref resolution"
grep -q 'refs/pull/${pr_number}/merge' "$checker" || fail "checker must resolve the shallow PR range through the merge ref"
grep -q 'actual_base.*base_sha' "$checker" || fail "checker must verify merge-ref base identity"
grep -q 'actual_head.*head_sha' "$checker" || fail "checker must verify merge-ref head identity"
grep -q 'safe.directory.*repo_root' "$checker" || fail "container format check must trust only the mounted userspace root"
if grep -Eq '^[[:space:]]*git config --global --add safe\.directory[[:space:]]+["'"']?\*(["'"']|$)' "$checker"; then
    fail "formatter must not trust every Git repository globally"
fi
grep -q 'issue #37' "$checker" || fail "mixed keymap.c exclusion must keep an explicit follow-up"

printf 'Changed firmware format scope checks passed.\n'
