#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
rules_file="$repo_root/users/halcyon_modules/splitkb/rules.mk"

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

if ! grep -Eq '^[[:space:]]*BACKLIGHT_ENABLE[[:space:]]*=[[:space:]]*yes([[:space:]]|$)' <<<"$tft_block"; then
    echo "Expected the HLC_TFT_DISPLAY rules block to enable QMK backlight support." >&2
    exit 1
fi

if ! grep -Eq '^[[:space:]]*BACKLIGHT_DRIVER[[:space:]]*=[[:space:]]*pwm([[:space:]]|$)' <<<"$tft_block"; then
    echo "Expected the HLC_TFT_DISPLAY rules block to select the PWM backlight driver." >&2
    exit 1
fi
