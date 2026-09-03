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

base_sha="${1:-}"
head_sha="${2:-}"
smoke_target="users/halcyon_modules/splitkb/halcyon.h"
targets=("$smoke_target")

ensure_commit() {
    local sha="$1"
    git cat-file -e "${sha}^{commit}" 2>/dev/null && return 0

    if git fetch --no-tags --depth=1 origin "$sha" >/dev/null 2>&1; then
        git cat-file -e "${sha}^{commit}" 2>/dev/null && return 0
    fi

    # Same-repository and fork PRs are both exposed through refs/pull/*/merge.
    # Fetching that ref at depth 2 makes both merge parents available without a
    # full repository history fetch.
    if [[ "${GITHUB_REF:-}" == refs/pull/*/merge ]]; then
        git fetch --no-tags --depth=2 origin "$GITHUB_REF" >/dev/null 2>&1 || true
        git cat-file -e "${sha}^{commit}" 2>/dev/null && return 0
    fi

    fail "cannot resolve PR commit $sha for changed-file formatting"
}

if [[ -n "$base_sha" && -n "$head_sha" ]]; then
    ensure_commit "$base_sha"
    ensure_commit "$head_sha"

    mapfile -d '' -t changed_files < <(
        git diff --name-only -z --diff-filter=AMR "$base_sha" "$head_sha"
    )

    for path in "${changed_files[@]}"; do
        if should_format_firmware_path "$path"; then
            targets+=("$path")
        fi
    done
else
    printf 'No pull-request base/head range supplied; running formatter smoke check only.\n'
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
