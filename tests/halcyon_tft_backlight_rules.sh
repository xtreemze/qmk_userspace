#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
rules_file="$repo_root/users/halcyon_modules/splitkb/rules.mk"
halcyon_file="$repo_root/users/halcyon_modules/splitkb/halcyon.c"

tft_block="$(
    awk '
        /^ifneq \(\$\(HLC_TFT_DISPLAY_ENABLED\),\)/ {
            in_block = 1
        }
        in_block {
            print
        }
        in_block && /^endif$/ {
            exit
        }
    ' "$rules_file"
)"

if grep -Eq '^[[:space:]]*BACKLIGHT_ENABLE[[:space:]]*=' <<<"$tft_block"; then
    echo "Expected the HLC_TFT_DISPLAY rules block to avoid QMK BACKLIGHT_ENABLE." >&2
    exit 1
fi

if grep -Eq '^[[:space:]]*BACKLIGHT_DRIVER[[:space:]]*=' <<<"$tft_block"; then
    echo "Expected the HLC_TFT_DISPLAY rules block to avoid QMK backlight drivers." >&2
    exit 1
fi

if ! grep -q 'halcyon_backlight_set' "$halcyon_file"; then
    echo "Expected direct Halcyon TFT backlight control in halcyon.c." >&2
    exit 1
fi

if ! grep -q 'gpio_write_pin' "$halcyon_file"; then
    echo "Expected Halcyon TFT backlight control to drive a GPIO pin." >&2
    exit 1
fi
