#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
qmk_dir="${1:-$repo_root/qmk_firmware}"

if [[ ! -d "$qmk_dir/.git" ]]; then
    printf 'QMK checkout not found: %s\n' "$qmk_dir" >&2
    exit 1
fi

mapfile -t patches < <(find "$repo_root/patches" -maxdepth 1 -type f -name '[0-9][0-9][0-9][0-9]-*.patch' -print | sort)
if (( ${#patches[@]} == 0 )); then
    printf 'No numbered QMK patches found under %s/patches\n' "$repo_root" >&2
    exit 1
fi

for patch in "${patches[@]}"; do
    printf 'Applying %s\n' "${patch#$repo_root/}"
    git -C "$qmk_dir" apply --check "$patch"
    git -C "$qmk_dir" apply "$patch"
done
