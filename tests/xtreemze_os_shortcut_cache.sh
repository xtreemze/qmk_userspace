#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
keymap_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"

host_is_apple_body="$(
    awk '
        /^static bool host_is_apple\(void\) \{/ {
            in_func = 1
        }
        in_func {
            print
        }
        in_func && /^}$/ {
            exit
        }
    ' "$keymap_file"
)"

if grep -q 'detected_host_os()' <<<"$host_is_apple_body"; then
    echo "Expected OS-aware shortcuts to avoid polling the raw OS detector." >&2
    exit 1
fi

if ! grep -q 'cached_host_os' <<<"$host_is_apple_body"; then
    echo "Expected OS-aware shortcuts to use the per-boot cached OS." >&2
    exit 1
fi

os_callback_body="$(
    awk '
        /^bool process_detected_host_os_user\(os_variant_t detected_os\) \{/ {
            in_func = 1
        }
        in_func {
            print
        }
        in_func && /^}$/ {
            exit
        }
    ' "$keymap_file"
)"

if [[ -z "$os_callback_body" ]]; then
    echo "Expected a host OS detection callback to populate the cache." >&2
    exit 1
fi

if ! grep -Eq 'cached_host_os[[:space:]]*==[[:space:]]*OS_UNSURE' <<<"$os_callback_body"; then
    echo "Expected the cached OS to be assigned only once per keyboard boot." >&2
    exit 1
fi

if ! grep -Eq 'detected_os[[:space:]]*!=[[:space:]]*OS_UNSURE' <<<"$os_callback_body"; then
    echo "Expected inconclusive OS detection results to be ignored." >&2
    exit 1
fi
