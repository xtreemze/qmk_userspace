#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

run bash tests/qmk_patch_series.sh
run bash tests/changed_firmware_format_scope.sh
run bash tests/halcyon_tft_backlight_rules.sh
run python3 tests/halcyon_module_sync.py
run bash tests/halcyon_tft_layout_rules.sh
run python3 tests/xtreemze_encoder_repeat_direction.py
run bash tests/xtreemze_os_display_indicator.sh
run bash tests/xtreemze_os_fingerprint_trace.sh
run bash tests/xtreemze_os_shortcut_cache.sh
run bash tests/xtreemze_vial_factory_defaults.sh
run bash tests/xtreemze_docs_consistency.sh

# xtreemze_legacy_binary_equivalence.py intentionally remains outside this
# source-level gate: it requires two separately compiled, unstripped ELF inputs
# representing a chosen baseline and candidate.
printf '\nAll standalone Halcyon regressions passed.\n'
