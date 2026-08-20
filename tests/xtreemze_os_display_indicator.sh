#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
config_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/config.h"
keymap_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"
display_header="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.h"
display_file="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.c"
font_file="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/graphics/fonts/Retron2000-27.qff.c"

extract_function() {
    local file="$1"
    local signature="$2"

    awk -v signature="$signature" '
        index($0, signature) {
            in_func = 1
        }
        in_func {
            print
            depth += gsub(/\{/, "{")
            depth -= gsub(/\}/, "}")
        }
        in_func && depth == 0 {
            exit
        }
    ' "$file"
}

fail() {
    echo "$1" >&2
    exit 1
}

grep -Eq '^#define[[:space:]]+SPLIT_DETECTED_OS_ENABLE([[:space:]]|$)' "$config_file" ||
    fail "Expected QMK split detected-OS synchronization to be enabled."

for symbol in \
    HALCYON_DISPLAY_OS_UNKNOWN HALCYON_DISPLAY_OS_MACOS HALCYON_DISPLAY_OS_IOS \
    HALCYON_DISPLAY_OS_WINDOWS HALCYON_DISPLAY_OS_LINUX \
    HALCYON_HOST_SOURCE_DEFAULT HALCYON_HOST_SOURCE_STORED HALCYON_HOST_SOURCE_LIVE \
    HALCYON_HOST_EVENT_BOOT HALCYON_HOST_EVENT_RESUME HALCYON_HOST_EVENT_CHANGE; do
    grep -q "$symbol" "$display_header" || fail "Expected display telemetry symbol $symbol."
done

grep -q 'halcyon_display_host_telemetry_user' "$display_header" ||
    fail "Expected a low-coupling display telemetry hook."

os_mapping_body="$(extract_function "$keymap_file" 'display_os_from_variant(os_variant_t detected_os)')"
[[ -n "$os_mapping_body" ]] || fail "Expected exact QMK OS variants to map at the display boundary."
for mapping in \
    'OS_UNSURE:HALCYON_DISPLAY_OS_UNKNOWN' \
    'OS_MACOS:HALCYON_DISPLAY_OS_MACOS' \
    'OS_IOS:HALCYON_DISPLAY_OS_IOS' \
    'OS_WINDOWS:HALCYON_DISPLAY_OS_WINDOWS' \
    'OS_LINUX:HALCYON_DISPLAY_OS_LINUX'; do
    first="${mapping%%:*}"
    second="${mapping##*:}"
    grep -q "$first" <<<"$os_mapping_body" && grep -q "$second" <<<"$os_mapping_body" ||
        fail "Expected exact display mapping $first -> $second."
done

for expected_label in 'DETECTING' 'macOS' 'iOS' 'Windows' 'Linux'; do
    grep -q "\"$expected_label\"" "$display_file" ||
        fail "Expected readable display label $expected_label."
done

grep -q 'exact_detected_os' "$keymap_file" || fail "Expected exact OS diagnostic state."
grep -q 'effective_host_family' "$keymap_file" || fail "Expected separate shortcut-family state."
grep -q 'effective_host_source' "$keymap_file" || fail "Expected separate shortcut-source state."

user_data_body="$(awk '/^typedef struct \{/{block=$0 ORS; next} block != "" {block=block $0 ORS} /^} xtreemze_user_data_t;/{print block; exit}' "$keymap_file")"
if grep -Eq 'exact_detected_os|os_variant_t|halcyon_display_os_t' <<<"$user_data_body"; then
    fail "Expected exact OS telemetry to remain out of EEPROM user data."
fi

telemetry_body="$(extract_function "$keymap_file" 'halcyon_display_host_telemetry_user(halcyon_host_telemetry_t *telemetry)')"
[[ -n "$telemetry_body" ]] || fail "Expected the keymap to provide TFT host telemetry."
grep -q 'detected_host_os()' <<<"$telemetry_body" ||
    fail "Expected TFT telemetry to consume QMK's split-synchronized exact OS value."
grep -q 'effective_host_family' <<<"$telemetry_body" ||
    fail "Expected telemetry to expose effective shortcut policy separately."
grep -q 'effective_host_source' <<<"$telemetry_body" ||
    fail "Expected telemetry to expose LIVE/STORED/DEFAULT source separately."
grep -q 'is_keyboard_master()' <<<"$telemetry_body" ||
    fail "Expected telemetry to expose the split role."

split_observer_body="$(extract_function "$keymap_file" 'observe_split_display_os(os_variant_t detected_os)')"
grep -q 'timer_elapsed32' <<<"$split_observer_body" ||
    fail "Expected slave policy telemetry to wait for a stable synchronized OS result."
grep -q 'HOST_DISPLAY_DETECTION_SETTLE_MS' <<<"$split_observer_body" ||
    fail "Expected slave stabilization to use the documented detector settle window."

include_guard="$(awk '/#include "hlc_tft_display\/hlc_tft_display.h"/{print previous ORS $0} {previous=$0}' "$keymap_file")"
grep -q '#ifdef HLC_TFT_DISPLAY' <<<"$include_guard" ||
    fail "Expected the encoder build to avoid TFT-only header dependencies."

wakeup_body="$(extract_function "$display_file" 'module_suspend_wakeup_init_kb(void)')"
if grep -Eq '\bqp_|\bwait_(ms|us)[[:space:]]*\(' <<<"$wakeup_body"; then
    fail "Expected the interrupt-context wake callback to remain display-I/O-free and non-blocking."
fi
grep -q 'host_resume_pending[[:space:]]*=[[:space:]]*true' <<<"$wakeup_body" ||
    fail "Expected resume telemetry to be deferred to housekeeping."

os_callback_body="$(extract_function "$keymap_file" 'process_detected_host_os_user(os_variant_t detected_os)')"
if grep -Eq '\bqp_|\bwait_(ms|us)[[:space:]]*\(' <<<"$os_callback_body"; then
    fail "Expected OS callbacks to update RAM/flags without drawing or blocking."
fi

overlay_body="$(extract_function "$display_file" 'update_host_overlay(void)')"
[[ -n "$overlay_body" ]] || fail "Expected a housekeeping-driven host overlay state machine."
grep -q 'timer_elapsed32' <<<"$overlay_body" || fail "Expected timer-driven overlay timing."
grep -q 'HOST_OVERLAY_DURATION_MS' <<<"$overlay_body" || fail "Expected one obvious bounded overlay duration."
grep -q 'draw_host_overlay' <<<"$overlay_body" || fail "Expected the state machine to render overlay frames."
grep -q 'reset_normal_display_cache' <<<"$overlay_body" ||
    fail "Expected normal layer UI to be restored after the overlay."

draw_overlay_body="$(extract_function "$display_file" 'draw_host_overlay(uint32_t elapsed)')"
if grep -Eq 'draw_host_row|"(OS|SHORT|SOURCE|EVENT|SIDE)"' <<<"$draw_overlay_body"; then
    fail "Expected the host overlay to avoid verbose labeled telemetry rows."
fi

text_row_count="$(grep -c 'draw_centered_host_text' <<<"$draw_overlay_body" || true)"
if (( text_row_count > 3 )); then
    fail "Expected no more than three significant host telemetry rows."
fi

duration_ms="$(awk '/^#define HOST_OVERLAY_DURATION_MS /{print $3}' "$display_file")"
stable_ms="$(awk '/^#define HOST_PANEL_STABLE_MS /{print $3}' "$display_file")"
pulse_ms="$(awk '/^#define HOST_PANEL_PULSE_MS /{print $3}' "$display_file")"
if [[ -z "$duration_ms" || "$duration_ms" -gt 2000 ]]; then
    fail "Expected automatic host telemetry to have a hard maximum lifetime of 2000 ms."
fi
if [[ -z "$stable_ms" || -z "$pulse_ms" || $((pulse_ms - stable_ms)) -le $((duration_ms / 2)) ]]; then
    fail "Expected fully revealed telemetry to remain stable for most of the animation."
fi

fit_body="$(extract_function "$display_file" 'fit_host_line(const char *full, const char *compact)')"
grep -q 'qp_textwidth' <<<"$fit_body" ||
    fail "Expected host overlay strings to be measured against the active font."
grep -q 'LCD_WIDTH' <<<"$fit_body" ||
    fail "Expected host overlay strings to be constrained to the display width."

node - "$font_file" <<'NODE'
const fs = require('fs');
const source = fs.readFileSync(process.argv[2], 'utf8');
const arraySource = source.split('font_Retron2000_27[4330] = {', 2)[1];
const bytes = [...arraySource.matchAll(/0x([0-9A-Fa-f]{2})/g)].map(match => Number.parseInt(match[1], 16));
const widths = new Map();
for (let codePoint = 0x20; codePoint <= 0x7e; codePoint++) {
    const offset = 30 + ((codePoint - 0x20) * 3);
    const value = bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16);
    widths.set(String.fromCharCode(codePoint), value & 0x3f);
}
const compactLines = [
    'DETECT', 'macOS', 'iOS', 'WIN', 'Linux',
    'CMD QMK', 'CMD STOR', 'CMD DEF', 'CTL QMK', 'CTL STOR', 'CTL DEF',
    'BOOT MST', 'WAKE MST', 'CHG MST', 'BOOT SLV', 'WAKE SLV', 'CHG SLV',
    'TRACE', '48/48', '!48/48', 'W FFFF', 'N48 F48', 'T48 Q48', 'C WIN', 'R WIN', 'NO TRACE',
];
const measured = compactLines.map(text => ({
    text,
    width: [...text].reduce((total, char) => total + widths.get(char), 0),
}));
const longest = measured.reduce((current, candidate) => candidate.width > current.width ? candidate : current);
if (longest.width > 135) {
    throw new Error(`Compact host line '${longest.text}' is ${longest.width}px wide on a 135px display.`);
}
NODE

if grep -Eq '\bwait_(ms|us)[[:space:]]*\(' "$display_file"; then
    fail "Expected all TFT animation to remain non-blocking."
fi

housekeeping_body="$(extract_function "$display_file" 'display_module_housekeeping_task_kb(bool second_display)')"
grep -q 'update_host_overlay()' <<<"$housekeeping_body" ||
    fail "Expected normal display housekeeping to own host-panel drawing."
grep -q 'host_resume_pending' <<<"$housekeeping_body" ||
    fail "Expected housekeeping to consume the deferred resume event."

update_display_body="$(extract_function "$display_file" 'update_display(void)')"
grep -q 'draw_host_marker' <<<"$update_display_body" ||
    fail "Expected the normal header to retain a compact host marker."
