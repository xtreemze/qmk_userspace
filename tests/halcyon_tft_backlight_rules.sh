#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
rules_file="$repo_root/users/halcyon_modules/splitkb/rules.mk"
halcyon_file="$repo_root/users/halcyon_modules/splitkb/halcyon.c"
tft_file="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.c"

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

housekeeping_body="$(
    awk '
        /^void housekeeping_task_kb\(void\) \{/ {
            in_func = 1
        }
        in_func {
            print
        }
        in_func && /^}$/ {
            exit
        }
    ' "$halcyon_file"
)"

if grep -Eq '\bwait_(ms|us)[[:space:]]*\(' <<<"$housekeeping_body"; then
    echo "Expected housekeeping_task_kb to avoid blocking waits that can delay input scanning." >&2
    exit 1
fi

backlight_line="$(grep -n 'last_input_activity_elapsed' "$halcyon_file" | head -n1 | cut -d: -f1)"
display_line="$(grep -n 'display_module_housekeeping_task_kb(false)' "$halcyon_file" | head -n1 | cut -d: -f1)"

if [[ -z "$backlight_line" || -z "$display_line" || "$backlight_line" -gt "$display_line" ]]; then
    echo "Expected backlight wake/suspend handling before display housekeeping." >&2
    exit 1
fi

tft_wakeup_body="$(
    awk '
        /^void module_suspend_wakeup_init_kb\(void\) \{/ {
            in_func = 1
        }
        in_func {
            print
        }
        in_func && /^}$/ {
            exit
        }
    ' "$tft_file"
)"

if grep -q 'qp_power' <<<"$tft_wakeup_body"; then
    echo "Expected the interrupt-context wake callback to avoid TFT SPI operations." >&2
    exit 1
fi

if ! grep -q 'display_wakeup_pending[[:space:]]*=[[:space:]]*true' <<<"$tft_wakeup_body"; then
    echo "Expected the wake callback to defer TFT recovery to housekeeping." >&2
    exit 1
fi

if ! grep -q 'display_wakeup_pending' "$tft_file" || ! grep -q 'qp_power(lcd, true)' "$tft_file"; then
    echo "Expected housekeeping-context TFT wake recovery." >&2
    exit 1
fi
