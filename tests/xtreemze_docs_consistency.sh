#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
keymap_dir="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final"
keymap_file="$keymap_dir/keymap.c"
profile_file="$keymap_dir/xtreemzeVial.vil"
readme_file="$keymap_dir/readme.md"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

marker="$(sed -nE 's/^#define[[:space:]]+XTREEMZE_DEFAULTS_EE_MARKER[[:space:]]+(0x[0-9A-Fa-f]+).*/\1/p' "$keymap_file")"
[[ -n "$marker" ]] || fail 'Could not read XTREEMZE_DEFAULTS_EE_MARKER from keymap.c.'
[[ "$(printf '%s\n' "$marker" | wc -l | tr -d ' ')" == '1' ]] || fail 'Expected exactly one factory marker definition.'

grep -Fq "marker \`$marker\`" "$readme_file" ||
    fail "Keymap README must document the current factory marker $marker."

while IFS= read -r documented; do
    [[ "$documented" == "marker \`$marker\`" ]] ||
        fail "Keymap README documents stale factory marker: $documented (current: marker \`$marker\`)."
done < <(grep -Eo 'marker `0x[0-9A-Fa-f]+`' "$readme_file" || true)

if command -v sha256sum >/dev/null 2>&1; then
    profile_sha="$(sha256sum "$profile_file" | awk '{print $1}')"
elif command -v shasum >/dev/null 2>&1; then
    profile_sha="$(shasum -a 256 "$profile_file" | awk '{print $1}')"
else
    fail 'Need sha256sum or shasum to verify the canonical Vial profile hash.'
fi

grep -Fq "\`$profile_sha\`" "$readme_file" ||
    fail "Keymap README must document canonical profile SHA-256 $profile_sha."

printf 'Documentation consistency: factory marker %s and canonical profile SHA-256 are current.\n' "$marker"
