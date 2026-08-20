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

grep -Eq '^#define[[:space:]]+OS_DETECTION_KEYBOARD_RESET([[:space:]]|$)' "$config_file" ||
    fail "Expected genuine USB re-enumeration to reset OS detection state."

if grep -Eq '^#define[[:space:]]+OS_DETECTION_SINGLE_REPORT([[:space:]]|$)' "$config_file"; then
    fail "Expected userspace session stabilization instead of OS_DETECTION_SINGLE_REPORT."
fi

grep -Eq 'HOST_FAMILY_UNKNOWN([[:space:]]*=[[:space:]]*0)?' "$keymap_file" ||
    fail "Expected an explicit unknown host family."
grep -q 'HOST_FAMILY_APPLE' "$keymap_file" || fail "Expected an Apple host family."
grep -q 'HOST_FAMILY_CTRL' "$keymap_file" || fail "Expected a Ctrl host family."
grep -q 'session_host_family' "$keymap_file" || fail "Expected per-session host-family state."
grep -q 'effective_host_family' "$keymap_file" || fail "Expected effective shortcut host-family state."

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
grep -Eq 'detected_family[[:space:]]*==[[:space:]]*HOST_FAMILY_UNKNOWN' <<<"$callback_body" ||
    fail "Expected OS_UNSURE/Unknown callbacks to be ignored."
grep -Eq 'session_host_family[[:space:]]*!=[[:space:]]*HOST_FAMILY_UNKNOWN' <<<"$callback_body" ||
    fail "Expected later valid detector changes to be ignored for the session."
grep -Eq 'session_host_family[[:space:]]*=[[:space:]]*detected_family' <<<"$callback_body" ||
    fail "Expected the first valid family to establish the session."
grep -Eq 'effective_host_family[[:space:]]*=[[:space:]]*detected_family' <<<"$callback_body" ||
    fail "Expected the confirmed family to become effective immediately."
grep -Eq 'last_host_family[[:space:]]*!=.*detected_family' <<<"$callback_body" ||
    fail "Expected EEPROM writes to be guarded by a confirmed family change."
grep -q 'save_user_data()' <<<"$callback_body" ||
    fail "Expected a newly confirmed family change to be persisted."

load_body="$(extract_function 'load_user_data(void)')"
grep -q 'last_host_family' <<<"$load_body" ||
    fail "Expected persisted host-family state to be loaded."
grep -Eq 'effective_host_family[[:space:]]*=' <<<"$load_body" ||
    fail "Expected persisted state to initialize the effective fallback."
grep -q 'HOST_FAMILY_UNKNOWN' <<<"$load_body" ||
    fail "Expected invalid persisted host-family values to fall back to Unknown."

shortcut_body="$(extract_function 'host_uses_command_shortcuts(void)')"
[[ -n "$shortcut_body" ]] || fail "Expected semantic shortcut selection by host family."
if grep -Eq 'detected_host_os\(\)|OS_(MACOS|IOS|WINDOWS|LINUX|UNSURE)' <<<"$shortcut_body"; then
    fail "Expected shortcut execution to know only the effective semantic family."
fi
grep -Eq 'effective_host_family[[:space:]]*==[[:space:]]*HOST_FAMILY_APPLE' <<<"$shortcut_body" ||
    fail "Expected only the Apple family to select Command shortcuts."

grep -Eq 'uint8_t[[:space:]]+last_host_family;' "$keymap_file" ||
    fail "Expected the existing reserved byte to store the last known family."
if grep -Eq 'uint8_t[[:space:]]+reserved;' "$keymap_file"; then
    fail "Expected the reserved byte to be renamed without growing the EEPROM struct."
fi

grep -Eq '^#define[[:space:]]+XTREEMZE_USER_DATA_VERSION[[:space:]]+0x02([[:space:]]|$)' "$keymap_file" ||
    fail "Expected the EEPROM user-data version to remain 0x02."
grep -Eq '^#define[[:space:]]+XTREEMZE_DEFAULTS_EE_MARKER[[:space:]]+0xAE([[:space:]]|$)' "$keymap_file" ||
    fail "Expected the Vial defaults marker to remain 0xAE."
