#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
display_file="$repo_root/users/halcyon_modules/splitkb/hlc_tft_display/hlc_tft_display.c"
keymap_file="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"

pattern_body="$(
    awk '
        /^static void draw_layer_background_pattern\(/ {
            in_func = 1
        }
        in_func {
            print
        }
        in_func && /^}$/ {
            exit
        }
    ' "$display_file"
)"

if ! grep -Eq 'for \(uint16_t x = 0; x < LCD_WIDTH; x \+= tile_w\)' <<<"$pattern_body"; then
    echo "Expected the TFT pattern to use the clipped final display column." >&2
    exit 1
fi

layer_names="$(
    awk '
        /static const char \*const layer_names\[\]/ {
            in_names = 1
            next
        }
        in_names && /};/ {
            exit
        }
        in_names {
            while (match($0, /"[^"]+"/)) {
                print substr($0, RSTART + 1, RLENGTH - 2)
                $0 = substr($0, RSTART + RLENGTH)
            }
        }
    ' "$keymap_file"
)"

expected_names="MOUSE QWERTY COLEMAK NUMSYMS NUMFLIP ONESHOT EDITING FNSYMS FNFLIP SYMBOLS RGBHUE RGBVAL BKLIGHT"
actual_names="$(tr '\n' ' ' <<<"$layer_names" | sed 's/[[:space:]]*$//')"

if [[ "$actual_names" != "$expected_names" ]]; then
    echo "Expected concise semantic names for all thirteen layers." >&2
    exit 1
fi

while IFS= read -r name; do
    if [[ ! "$name" =~ ^[[:alpha:]]{1,7}$ ]]; then
        echo "Layer name '$name' must contain only letters and be at most seven characters." >&2
        exit 1
    fi
done <<<"$layer_names"

python3 "$repo_root/tests/halcyon_tft_patterns.py"
