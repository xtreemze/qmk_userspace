#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

is_c_family_source() {
    case "$1" in
        *.c|*.h|*.cpp|*.hpp) return 0 ;;
        *) return 1 ;;
    esac
}

should_format_firmware_path() {
    local path="$1"

    is_c_family_source "$path" || return 1

    # Quantum Painter assets are generated binary/font translations. Their
    # layout belongs to the asset generator, not clang-format.
    case "$path" in
        users/halcyon_modules/splitkb/*/graphics/*) return 1 ;;
    esac

    case "$path" in
        users/halcyon_modules/splitkb/*)
            return 0
            ;;
        converters/promicro_to_halcyon/_pin_defs.h)
            return 0
            ;;
        keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/config.h|\
        keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/halcyon_legacy.h|\
        keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/halcyon_overrides.c)
            return 0
            ;;
    esac

    # keymap.c intentionally remains outside this first gate. It mixes
    # handwritten firmware logic with large canonical/generated Vial tables;
    # issue #37 tracks splitting or defining a deterministic generator format
    # before whole-file clang-format enforcement is safe.
    return 1
}

if [[ "${1:-}" == "--classify" ]]; then
    shift
    for path in "$@"; do
        if should_format_firmware_path "$path"; then
            printf '%s\n' "$path"
        fi
    done
    exit 0
fi

head_sha="${1:-}"
pr_number="${2:-}"
smoke_target="users/halcyon_modules/splitkb/halcyon.h"
targets=("$smoke_target")

# The QMK build runs inside a container while Actions checkout is mounted from
# the host. Trust only this known repository root before invoking Git; do not
# use the broad safe.directory='*' escape hatch.
git config --global --add safe.directory "$repo_root"

if [[ -n "$head_sha" && -n "$pr_number" ]]; then
    pr_ref="refs/pull/${pr_number}/merge"
    git fetch --no-tags --depth=2 origin "$pr_ref"
    merge_sha="$(git rev-parse FETCH_HEAD)"
    actual_base="$(git rev-parse "${merge_sha}^1")"
    actual_head="$(git rev-parse "${merge_sha}^2")"

    # GitHub can advance the base branch and regenerate refs/pull/N/merge while
    # the pull_request event payload still carries the earlier base SHA. The
    # PR head, however, is immutable for this workflow run and must match.
    [[ "$actual_head" == "$head_sha" ]] || fail "PR merge-ref head $actual_head does not match expected $head_sha"

    printf 'Resolved PR %s merge range: %s..%s\n' "$pr_number" "$actual_base" "$actual_head"
    mapfile -d '' -t changed_files < <(
        git diff --name-only -z --diff-filter=AMR "$actual_base" "$actual_head"
    )

    for path in "${changed_files[@]}"; do
        if should_format_firmware_path "$path"; then
            targets+=("$path")
        fi
    done
elif [[ -n "$head_sha" || -n "$pr_number" ]]; then
    fail "PR formatting requires head SHA and PR number together"
else
    printf 'No pull-request range supplied; running formatter smoke check only.\n'
fi

command -v qmk >/dev/null 2>&1 || fail "qmk CLI is required for firmware formatting"

# Keep one known hand-maintained source in every invocation. This proves that
# the configured QMK/clang-format path itself works even when a PR changes no C
# sources, while changed-file enforcement stays narrow.
declare -A seen=()
unique_targets=()
for path in "${targets[@]}"; do
    [[ -f "$path" ]] || fail "format target does not exist: $path"
    if [[ -z "${seen[$path]:-}" ]]; then
        seen[$path]=1
        unique_targets+=("$path")
    fi
done

printf 'Checking clang-format for:\n'
printf '  %s\n' "${unique_targets[@]}"
qmk format-c -n "${unique_targets[@]}"
