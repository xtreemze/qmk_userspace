#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
keymap_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"
config_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/config.h"

extract_function() {
    local signature="$1"

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
    ' "$keymap_file"
}

fail() {
    echo "$1" >&2
    exit 1
}

if grep -q 'cached_host_os' "$keymap_file"; then
    fail "Expected the raw cached_host_os model to be removed."
fi

if grep -Eq '^#define[[:space:]]+OS_DETECTION_KEYBOARD_RESET([[:space:]]|$)' "$config_file"; then
    fail "Expected suspend/resume to preserve userspace host-family state without a detector-driven keyboard reset."
fi

if grep -Eq '^#define[[:space:]]+OS_DETECTION_SINGLE_REPORT([[:space:]]|$)' "$config_file"; then
    fail "Expected userspace session stabilization instead of OS_DETECTION_SINGLE_REPORT."
fi

grep -Eq 'HOST_FAMILY_UNKNOWN([[:space:]]*=[[:space:]]*0)?' "$keymap_file" ||
    fail "Expected an explicit unknown host family."
grep -q 'HOST_FAMILY_APPLE' "$keymap_file" || fail "Expected an Apple host family."
grep -q 'HOST_FAMILY_CTRL' "$keymap_file" || fail "Expected a Ctrl host family."
grep -q 'session_host_family' "$keymap_file" || fail "Expected per-session host-family state."
grep -q 'effective_host_family' "$keymap_file" || fail "Expected effective shortcut host-family state."
grep -Eq '^#define[[:space:]]+HOST_FAMILY_SETTLE_MS[[:space:]]+[1-9][0-9]*([[:space:]]|$)' "$keymap_file" ||
    fail "Expected a nonzero confidence interval before changing the effective family."
grep -Eq '^#define[[:space:]]+HOST_FAMILY_PERSIST_MS[[:space:]]+[1-9][0-9]*([[:space:]]|$)' "$keymap_file" ||
    fail "Expected a nonzero stability interval before persisting a family."
grep -Eq '^#define[[:space:]]+HOST_FAMILY_RESUME_GUARD_MS[[:space:]]+[1-9][0-9]*([[:space:]]|$)' "$keymap_file" ||
    fail "Expected a resume guard that preserves the confirmed family during wake enumeration."

mapping_body="$(extract_function 'host_family_from_os(os_variant_t detected_os)')"
[[ -n "$mapping_body" ]] || fail "Expected OS variants to be mapped only at the detector boundary."
grep -Eq 'case[[:space:]]+OS_MACOS:|detected_os[[:space:]]*==[[:space:]]*OS_MACOS' <<<"$mapping_body" ||
    fail "Expected macOS to map to the Apple family."
grep -Eq 'case[[:space:]]+OS_IOS:|detected_os[[:space:]]*==[[:space:]]*OS_IOS' <<<"$mapping_body" ||
    fail "Expected iOS to map to the Apple family."
grep -Eq 'case[[:space:]]+OS_WINDOWS:|detected_os[[:space:]]*==[[:space:]]*OS_WINDOWS' <<<"$mapping_body" ||
    fail "Expected Windows to map to the Ctrl family."
grep -Eq 'case[[:space:]]+OS_LINUX:|detected_os[[:space:]]*==[[:space:]]*OS_LINUX' <<<"$mapping_body" ||
    fail "Expected Linux to map to the Ctrl family."
grep -q 'HOST_FAMILY_UNKNOWN' <<<"$mapping_body" ||
    fail "Expected inconclusive OS detection to map to Unknown."

callback_body="$(extract_function 'process_detected_host_os_user(os_variant_t detected_os)')"
[[ -n "$callback_body" ]] || fail "Expected a host OS detection callback."
grep -q 'stage_host_family_candidate' <<<"$callback_body" ||
    fail "Expected detector callbacks to stage evidence for later evaluation."
if grep -Eq '(session_host_family|effective_host_family|last_host_family)[[:space:]]*=' <<<"$callback_body" || grep -q 'save_user_data()' <<<"$callback_body"; then
    fail "Expected detector callbacks not to commit policy or EEPROM directly."
fi

stage_body="$(extract_function 'stage_host_family_candidate(os_variant_t detected_os)')"
[[ -n "$stage_body" ]] || fail "Expected a bounded detector-candidate staging function."
grep -q 'HOST_FAMILY_UNKNOWN' <<<"$stage_body" ||
    fail "Expected OS_UNSURE to invalidate only pending evidence."
if grep -Eq 'effective_host_family[[:space:]]*=' <<<"$stage_body" || grep -q 'save_user_data()' <<<"$stage_body"; then
    fail "Expected staging to leave effective and persisted families untouched."
fi

update_body="$(extract_function 'update_host_family_candidate(void)')"
[[ -n "$update_body" ]] || fail "Expected matrix-context host-family stabilization."
grep -q 'HOST_FAMILY_RESUME_GUARD_MS' <<<"$update_body" ||
    fail "Expected post-resume evidence to wait behind the resume guard."
grep -q 'HOST_FAMILY_SETTLE_MS' <<<"$update_body" ||
    fail "Expected effective policy changes only after the candidate settles."
grep -q 'HOST_FAMILY_PERSIST_MS' <<<"$update_body" ||
    fail "Expected EEPROM changes only after the longer persistence interval."
grep -Eq 'session_host_family[[:space:]]*=[[:space:]]*pending_host_family' <<<"$update_body" ||
    fail "Expected a settled later detector result to replace session evidence."
grep -Eq 'effective_host_family[[:space:]]*=[[:space:]]*pending_host_family' <<<"$update_body" ||
    fail "Expected settled detector evidence to become effective."
grep -Eq 'last_host_family[[:space:]]*!=.*pending_host_family' <<<"$update_body" ||
    fail "Expected EEPROM writes to be guarded by a stable family change."
grep -q 'save_user_data()' <<<"$update_body" ||
    fail "Expected only stable family changes to be persisted."

wakeup_body="$(extract_function 'suspend_wakeup_init_user(void)')"
[[ -n "$wakeup_body" ]] || fail "Expected a userspace wake marker."
grep -Eq 'host_family_resume_pending[[:space:]]*=[[:space:]]*true' <<<"$wakeup_body" ||
    fail "Expected wake ISR work to be limited to deferring the resume guard."
if grep -Eq '(session_host_family|effective_host_family|last_host_family)[[:space:]]*=' <<<"$wakeup_body" || grep -q 'save_user_data()' <<<"$wakeup_body"; then
    fail "Expected wake to preserve all confirmed host-family state."
fi

load_body="$(extract_function 'load_user_data(void)')"
grep -q 'last_host_family' <<<"$load_body" ||
    fail "Expected persisted host-family state to be loaded."
grep -Eq 'effective_host_family[[:space:]]*=' <<<"$load_body" ||
    fail "Expected persisted state to initialize the effective fallback."
grep -q 'HOST_FAMILY_UNKNOWN' <<<"$load_body" ||
    fail "Expected invalid persisted host-family values to fall back to Unknown."

shortcut_body="$(extract_function 'tap_os_clipboard(uint16_t mac_keycode, uint16_t other_keycode)')"
[[ -n "$shortcut_body" ]] || fail "Expected semantic shortcut selection by host family."
if grep -Eq 'detected_host_os\(\)|OS_(MACOS|IOS|WINDOWS|LINUX|UNSURE)' <<<"$shortcut_body"; then
    fail "Expected shortcut execution to know only the effective semantic family."
fi
grep -q 'HOST_FAMILY_APPLE' <<<"$shortcut_body" || fail "Expected Apple to select Command shortcuts."
grep -q 'HOST_FAMILY_CTRL' <<<"$shortcut_body" || fail "Expected Ctrl hosts to select Ctrl shortcuts."
grep -q 'HOST_FAMILY_UNKNOWN' <<<"$shortcut_body" ||
    fail "Expected Unknown to remain distinct instead of implicitly selecting Ctrl."

grep -Eq 'uint8_t[[:space:]]+last_host_family;' "$keymap_file" ||
    fail "Expected the existing reserved byte to store the last known family."
if grep -Eq 'uint8_t[[:space:]]+reserved;' "$keymap_file"; then
    fail "Expected the reserved byte to be renamed without growing the EEPROM struct."
fi

grep -Eq '^#define[[:space:]]+XTREEMZE_USER_DATA_VERSION[[:space:]]+0x02([[:space:]]|$)' "$keymap_file" ||
    fail "Expected the EEPROM user-data version to remain 0x02."
grep -Eq '^#define[[:space:]]+XTREEMZE_DEFAULTS_EE_MARKER[[:space:]]+0xAE([[:space:]]|$)' "$keymap_file" ||
    fail "Expected the Vial defaults marker to remain 0xAE."
