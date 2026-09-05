#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_path="${1:-}"
workflow_file="$repo_root/.github/workflows/build_binaries.yaml"
qmk_json="$repo_root/qmk.json"

fail() {
    echo "$1" >&2
    exit 1
}

command -v git >/dev/null 2>&1 || fail "git is required"
command -v python3 >/dev/null 2>&1 || fail "python3 is required"
[[ -f "$workflow_file" ]] || fail "Missing build workflow: $workflow_file"
[[ -f "$qmk_json" ]] || fail "Missing userspace target manifest: $qmk_json"

userspace_sha="$(git -C "$repo_root" rev-parse HEAD)"
userspace_branch="$(git -C "$repo_root" branch --show-current 2>/dev/null || true)"
[[ -n "$userspace_branch" ]] || userspace_branch="detached"
if git -C "$repo_root" diff --quiet --ignore-submodules -- && git -C "$repo_root" diff --cached --quiet --ignore-submodules --; then
    tree_state="clean"
else
    tree_state="dirty"
fi

vial_qmk_sha="$(awk '
    /repository:[[:space:]]*vial-kb\/vial-qmk/ { in_vial = 1; next }
    in_vial && /ref:/ {
        gsub(/^[[:space:]]*ref:[[:space:]]*/, "", $0)
        print $0
        exit
    }
' "$workflow_file")"
[[ -n "$vial_qmk_sha" ]] || vial_qmk_sha="unknown"

targets="$(python3 - "$qmk_json" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    data = json.load(handle)

for keyboard, keymap, env in data.get("build_targets", []):
    target = env.get("TARGET", "<unnamed>")
    module = next((key for key, value in env.items() if key.startswith("HLC_") and value == "1"), "<module-unknown>")
    print(f"- `{target}` — `{keyboard}:{keymap}` — `{module}`")
PY
)"

host_os="$(uname -s 2>/dev/null || echo unknown)"
host_kernel="$(uname -a 2>/dev/null || echo unknown)"
if [[ "$host_os" == "Darwin" ]] && command -v sw_vers >/dev/null 2>&1; then
    host_detail="$(sw_vers -productName) $(sw_vers -productVersion) ($(sw_vers -buildVersion))"
else
    host_detail="$host_kernel"
fi

timestamp_utc="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"

report="$(cat <<EOF
# Halcyon hardware acceptance evidence

Generated: $timestamp_utc

## Software provenance

- Userspace commit: \`$userspace_sha\`
- Userspace branch: \`$userspace_branch\`
- Working tree: \`$tree_state\`
- Pinned Vial-QMK: \`$vial_qmk_sha\`

## Release targets declared by qmk.json

$targets

## Host context

- Host OS: $host_detail

## Physical configuration

- USB master half: <left/right>
- Display-side module/revision: <record marking/revision>
- Encoder-side module/revision: <record marking/revision>
- Cable/hub/dock/power path: <record if relevant>
- Firmware artifact flashed: <filename and SHA-256 if available>

## Acceptance observations

| Check | Result | Evidence / notes | Recovery required |
| --- | --- | --- | --- |
| Cold boot and normal typing | <pass/fail/not-run> |  |  |
| TFT rendering and animation stability | <pass/fail/not-run> |  |  |
| Backlight level range and controls | <pass/fail/not-run> |  |  |
| Saved brightness after reboot | <pass/fail/not-run> |  |  |
| Idle timeout and key wake | <pass/fail/not-run> |  |  |
| Host suspend/resume | <pass/fail/not-run> |  |  |
| Split disconnect/reconnect | <pass/fail/not-run> |  |  |
| One-half reset while peer remains powered | <pass/fail/not-run> |  |  |
| Module buttons / encoder behavior | <pass/fail/not-run> |  |  |
| Vial connectivity and dynamic edits after recovery | <pass/fail/not-run> |  |  |

## Additional evidence

- Photos/video/logs: <links or attachment names>
- Relevant issue/PR: <number/link>
- Tester notes: <free-form observations>

> This snapshot records provenance and prompts for physical evidence. It does not itself certify hardware behavior.
EOF
)"

if [[ -n "$output_path" ]]; then
    printf '%s\n' "$report" > "$output_path"
    echo "Wrote hardware acceptance snapshot to $output_path"
else
    printf '%s\n' "$report"
fi
