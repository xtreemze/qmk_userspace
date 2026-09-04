#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
keymap_dir="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final"
keymap_file="$keymap_dir/keymap.c"
profile_file="$keymap_dir/xtreemzeVial.vil"
readme_file="$keymap_dir/readme.md"
workflow_file="$repo_root/.github/workflows/build_binaries.yaml"
qmk_json="$repo_root/qmk.json"
related_projects_file="$repo_root/docs/RELATED_PROJECTS.md"

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

mapfile -t vial_pins < <(
    sed -nE 's/^[[:space:]]+ref:[[:space:]]+([0-9a-f]{40})[[:space:]]*$/\1/p' "$workflow_file" | sort -u
)
[[ "${#vial_pins[@]}" == '1' ]] ||
    fail 'Regression and firmware-build checkouts must resolve one exact Vial-QMK revision.'
vial_pin="${vial_pins[0]}"

vial_repo_count="$(grep -Ec '^[[:space:]]+repository:[[:space:]]+vial-kb/vial-qmk[[:space:]]*$' "$workflow_file")"
vial_ref_count="$(grep -Ec "^[[:space:]]+ref:[[:space:]]+$vial_pin[[:space:]]*$" "$workflow_file")"
[[ "$vial_repo_count" == '2' ]] ||
    fail "Expected exactly two Vial-QMK checkout repository declarations; found $vial_repo_count."
[[ "$vial_ref_count" == '2' ]] ||
    fail "Regression and local firmware build must both use Vial-QMK pin $vial_pin."
grep -Fq "Pinned production revision: \`$vial_pin\`" "$related_projects_file" ||
    fail "Related-project documentation must record Vial-QMK pin $vial_pin."

python3 - "$qmk_json" "$readme_file" <<'PY'
import json
import pathlib
import sys

qmk_json = pathlib.Path(sys.argv[1])
readme = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8")
data = json.loads(qmk_json.read_text(encoding="utf-8"))
targets = data.get("build_targets")

if not isinstance(targets, list) or not targets:
    raise SystemExit("FAIL: qmk.json must define at least one build target.")

for index, entry in enumerate(targets):
    if not isinstance(entry, list) or len(entry) != 3:
        raise SystemExit(f"FAIL: build target {index} does not have [keyboard, keymap, env] shape.")

    keyboard, keymap, env = entry
    if not isinstance(env, dict):
        raise SystemExit(f"FAIL: build target {index} environment must be an object.")

    target_name = env.get("TARGET")
    module_flags = [name for name, value in env.items() if name.startswith("HLC_") and value == "1"]
    if not isinstance(target_name, str) or not target_name:
        raise SystemExit(f"FAIL: build target {index} is missing TARGET.")
    if len(module_flags) != 1:
        raise SystemExit(f"FAIL: build target {target_name} must enable exactly one HLC_* module flag.")

    expected_command = (
        f"qmk compile -kb {keyboard} -km {keymap} "
        f"-e {module_flags[0]}=1 -e TARGET={target_name}"
    )
    if expected_command not in readme:
        raise SystemExit(
            "FAIL: keymap README must document the exact qmk.json build command for "
            f"{target_name}."
        )

print(f"Documentation consistency: {len(targets)} qmk.json build targets are documented exactly.")
PY

printf 'Documentation consistency: factory marker %s, canonical profile SHA-256, and Vial-QMK pin %s are current.\n' "$marker" "$vial_pin"
