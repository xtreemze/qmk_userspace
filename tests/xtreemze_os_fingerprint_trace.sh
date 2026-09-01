#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
detector_header="$repo_root/qmk_firmware/quantum/os_detection.h"
detector_file="$repo_root/qmk_firmware/quantum/os_detection.c"
display_rules="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/rules.mk"
display_header="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.h"
display_file="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.c"
keymap_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"
keymap_rules="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/rules.mk"
config_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/config.h"
vial_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/vial.json"
readme_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/readme.md"

fail() {
    echo "$1" >&2
    exit 1
}

extract_function() {
    local file="$1"
    local signature="$2"

    awk -v signature="$signature" '
        index($0, signature) { in_func = 1 }
        in_func {
            print
            depth += gsub(/\{/, "{")
            depth -= gsub(/\}/, "}")
        }
        in_func && depth == 0 { exit }
    ' "$file"
}

default_trace_defs="$(make -s -f - <<MAKEFILE
USER_PATH := $repo_root/users/halcyon_modules
include $display_rules
all:
	@printf '%s' '\$(OPT_DEFS)'
MAKEFILE
)"
disabled_trace_defs="$(make -s -f - XTREEMZE_OS_FINGERPRINT_TRACE=no <<MAKEFILE
USER_PATH := $repo_root/users/halcyon_modules
include $display_rules
all:
	@printf '%s' '\$(OPT_DEFS)'
MAKEFILE
)"
grep -q -- '-DXTREEMZE_OS_FINGERPRINT_TRACE' <<<"$default_trace_defs" ||
    fail "Expected the TFT diagnostic build to enable the XTREEMZE trace flag by default."
if grep -q -- '-DXTREEMZE_OS_FINGERPRINT_TRACE' <<<"$disabled_trace_defs"; then
    fail "Expected an explicit no value to compile H1F instrumentation out."
fi

if grep -Eq 'OS_DETECTION_DEBUG_ENABLE|CONSOLE_ENABLE[[:space:]]*=[[:space:]]*yes' "$config_file" "$keymap_rules" "$display_rules"; then
    fail "Expected H1F to preserve Vial USB descriptors without stock debug or console interfaces."
fi

grep -Eq 'XTREEMZE_OS_FINGERPRINT_TRACE_CAPACITY[[:space:]]+48' "$detector_header" ||
    fail "Expected a bounded 48-entry fingerprint trace."
for symbol in \
    xtreemze_os_fingerprint_entry_t \
    xtreemze_os_fingerprint_trace_count \
    xtreemze_os_fingerprint_trace_overflowed \
    xtreemze_os_fingerprint_trace_read; do
    grep -q "$symbol" "$detector_header" || fail "Expected read-only trace API symbol $symbol."
done

trace_body="$(extract_function "$detector_file" 'trace_fingerprint_packet(uint16_t w_length, os_variant_t candidate_os)')"
[[ -n "$trace_body" ]] || fail "Expected passive packet tracing around process_wlength()."
for field in sequence_index w_length count cnt_02 cnt_04 cnt_ff candidate_os detected_os; do
    grep -q "$field" <<<"$trace_body" || fail "Expected trace entries to capture $field."
done
if grep -Eq 'eeprom|eeconfig|qp_|wait_(ms|us)|x?printf|print_stored' <<<"$trace_body"; then
    fail "Expected packet tracing to avoid persistence, display I/O, formatting, and blocking work."
fi

process_body="$(extract_function "$detector_file" 'process_wlength(const uint16_t w_length)')"
grep -q 'trace_fingerprint_packet' <<<"$process_body" ||
    fail "Expected process_wlength() to append a passive trace entry."
if grep -Eq 'eeprom|eeconfig|qp_|wait_(ms|us)|x?printf|print_stored' <<<"$process_body"; then
    fail "Expected process_wlength() to remain EEPROM-free, display-free, and non-blocking."
fi

windows_line="$(grep -n 'cnt_ff >= 2 && setups_data.cnt_04 >= 1' "$detector_file" | head -n1 | cut -d: -f1)"
macos_line="$(grep -n 'setups_data.count >= 5 && setups_data.last_wlength == 0xFF' "$detector_file" | head -n1 | cut -d: -f1)"
[[ -n "$windows_line" && -n "$macos_line" && "$windows_line" -lt "$macos_line" ]] ||
    fail "Expected H1F to preserve detector predicate ordering."

for function in draw_os_fingerprint_page update_os_fingerprint_page; do
    grep -q "$function" "$display_file" || fail "Expected TFT fingerprint function $function."
done
fingerprint_page_body="$(extract_function "$display_file" 'draw_os_fingerprint_page(uint32_t elapsed)')"
grep -Eq 'xtreemze_os_fingerprint_trace_read\(page,[[:space:]]*&entry\)' <<<"$fingerprint_page_body" ||
    fail "Expected each TFT page to render one complete per-packet trace entry."
for field in w_length count cnt_02 cnt_04 cnt_ff candidate_os detected_os; do
    grep -q "entry\.$field" <<<"$fingerprint_page_body" ||
        fail "Expected the TFT capture page to expose per-packet $field."
done
grep -q '"%s%u/%u"' <<<"$fingerprint_page_body" ||
    fail "Expected the 48-entry page header to use the width-safe compact form."

for mode in DISPLAY_MODE_NORMAL DISPLAY_MODE_HOST_OVERLAY DISPLAY_MODE_TRACE_VIEW; do
    grep -q "$mode" "$display_file" || fail "Expected explicit display state $mode."
done
if grep -Eq 'host_overlay_active|os_fingerprint_page_active' "$display_file"; then
    fail "Expected one display-mode state machine instead of overlapping booleans."
fi

duration_ms="$(awk '/^#define HOST_OVERLAY_DURATION_MS /{print $3}' "$display_file")"
if [[ -z "$duration_ms" || "$duration_ms" -gt 2000 ]]; then
    fail "Expected all automatic host telemetry to stop within 2000 ms."
fi

start_overlay_body="$(extract_function "$display_file" 'start_host_overlay(halcyon_host_event_t event, const halcyon_host_telemetry_t *telemetry)')"
grep -Eq 'display_mode[[:space:]]*=[[:space:]]*DISPLAY_MODE_HOST_OVERLAY' <<<"$start_overlay_body" ||
    fail "Expected lifecycle events to enter only HOST_OVERLAY mode."

overlay_body="$(extract_function "$display_file" 'update_host_overlay(void)')"
grep -Eq 'display_mode[[:space:]]*=[[:space:]]*DISPLAY_MODE_NORMAL' <<<"$overlay_body" ||
    fail "Expected automatic host telemetry to restore NORMAL mode."
if grep -Eq 'display_mode[[:space:]]*=[[:space:]]*DISPLAY_MODE_TRACE_VIEW|xtreemze_os_fingerprint_trace_(count|read)' <<<"$overlay_body"; then
    fail "Expected automatic host telemetry to never enter or page TRACE VIEW."
fi

toggle_body="$(extract_function "$display_file" 'halcyon_display_toggle_trace_view(void)')"
[[ -n "$toggle_body" ]] || fail "Expected an explicit TRACE VIEW toggle action."
grep -q 'DISPLAY_MODE_TRACE_VIEW' <<<"$toggle_body" || fail "Expected explicit action to enter TRACE VIEW."
grep -q 'DISPLAY_MODE_NORMAL' <<<"$toggle_body" || fail "Expected explicit action to exit TRACE VIEW."
if grep -Eq '\bqp_|\bwait_(ms|us)[[:space:]]*\(' <<<"$toggle_body"; then
    fail "Expected the explicit action to change state without rendering or blocking."
fi
grep -q 'halcyon_display_toggle_trace_view' "$display_header" ||
    fail "Expected the TFT module to expose the explicit TRACE VIEW action."

trace_update_body="$(extract_function "$display_file" 'update_os_fingerprint_page(void)')"
grep -Eq 'display_mode[[:space:]]*!=[[:space:]]*DISPLAY_MODE_TRACE_VIEW' <<<"$trace_update_body" ||
    fail "Expected trace paging to require explicit TRACE VIEW mode."

grep -q 'OS_TRACE_VIEW' "$vial_file" || fail "Expected a named Vial-assignable TRACE VIEW keycode."
grep -q 'USER20' "$readme_file" || fail "Expected USER20 TRACE VIEW assignment to be documented."

# Include the return type so this cannot accidentally match pre_process_record_user().
record_body="$(extract_function "$keymap_file" 'bool process_record_user(uint16_t keycode, keyrecord_t *record)')"
grep -q 'OS_TRACE_VIEW' <<<"$record_body" || fail "Expected USER20 to handle TRACE VIEW explicitly."
grep -q 'halcyon_display_toggle_trace_view' <<<"$record_body" || fail "Expected USER20 to toggle the display mode."
if grep -Eq '\bqp_|\bwait_(ms|us)[[:space:]]*\(' <<<"$record_body"; then
    fail "Expected TRACE VIEW input handling to remain display-I/O-free and non-blocking."
fi

keymaps_body="$(extract_function "$keymap_file" 'const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS]')"
if ! grep -q 'OS_TRACE_VIEW' <<<"$keymaps_body"; then
    fail "Expected the canonical USER20 TRACE VIEW binding in the compiled keymap."
fi

housekeeping_body="$(extract_function "$display_file" 'display_module_housekeeping_task_kb(bool second_display)')"
grep -q 'update_host_overlay()' <<<"$housekeeping_body" || fail "Expected the normal HOST LINK overlay to remain first."
grep -q 'update_os_fingerprint_page()' <<<"$housekeeping_body" || fail "Expected housekeeping to own diagnostic TFT drawing."

grep -q 'return "QMK"' "$display_file" || fail "Expected current-session detector telemetry to read QMK, not LIVE."
if grep -q 'return "LIVE"' "$display_file"; then
    fail "Expected the user-facing LIVE label to be retired."
fi

if grep -Eq '\bwait_(ms|us)[[:space:]]*\(' "$display_file"; then
    fail "Expected fingerprint paging to remain non-blocking."
fi

echo "H1F OS fingerprint trace checks passed."
