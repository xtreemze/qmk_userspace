#!/usr/bin/env python3
"""Rasterize the firmware's actual procedural patterns with clipped host QP stubs."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TFT = ROOT / 'users/halcyon_modules/splitkb/hlc_tft_display'
source = (TFT / 'hlc_tft_display.c').read_text()
header = (TFT / 'hlc_tft_display.h').read_text()
parser = argparse.ArgumentParser()
parser.add_argument('--frames', type=Path, help='Optional output folder for HSV888 frames (135x240).')
args = parser.parse_args()


def block(start, end):
    return source[source.index(start):source.index(end, source.index(start))]


palette = block('static const hsv_triplet_t layer_fg_hsv', 'static inline uint16_t mod_indicator_bit')
patterns = block('static void draw_diamond(', 'static void ensure_display_font_loaded(')
base = re.search(r'^#define HSV_EF_BG\s+(.*)$', header, re.M)[1]
harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define LCD_WIDTH 135
#define LCD_HEIGHT 240
#define DISPLAY_LAYER_STYLE_COUNT 13
static const int lcd_surface = 0;
typedef struct { uint8_t h, s, v; } hsv_triplet_t;
static uint8_t pixels[LCD_HEIGHT][LCD_WIDTH][3];
static unsigned calls;
static uint8_t cycle[16][LCD_HEIGHT][LCD_WIDTH][3];
static void qp_line(int surface, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t h, uint8_t s, uint8_t v) {
    (void)surface; calls++;
    assert(x0 <= x1 && x1 < LCD_WIDTH && y0 == y1 && y1 < LCD_HEIGHT);
    for (unsigned x = x0; x <= x1; x++) {
        pixels[y0][x][0] = h; pixels[y0][x][1] = s; pixels[y0][x][2] = v;
    }
}
static void qp_rect(int surface, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t h, uint8_t s, uint8_t v, bool filled) {
    assert(filled && y0 <= y1 && y1 < LCD_HEIGHT);
    for (unsigned y = y0; y <= y1; y++) qp_line(surface, x0, y, x1, y, h, s, v);
}
'''
harness += '\n#define HSV_EF_BG ' + base + '\n' + palette + patterns
harness += r'''
int main(void) {
    for (uint8_t layer = 0; layer < DISPLAY_LAYER_STYLE_COUNT; layer++) {
        for (unsigned frame = 0; frame < 256; frame++) {
            memset(pixels, 0xFF, sizeof(pixels));
            calls = 0;
            draw_layer_background_pattern(layer, frame);
            assert(calls < 7000); // Bounded work; no per-pixel full-screen effects.
            if (frame < 16) {
                memcpy(cycle[frame], pixels, sizeof(pixels));
                assert(fwrite(pixels, sizeof(pixels), 1, stdout) == 1);
            } else {
                assert(memcmp(cycle[frame % 16], pixels, sizeof(pixels)) == 0);
            }
        }
    }
}
'''
with tempfile.TemporaryDirectory(prefix='halcyon-patterns-') as tmp:
    c, exe = Path(tmp) / 'patterns.c', Path(tmp) / 'patterns'
    c.write_text(harness)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', str(c), '-o', str(exe)], check=True)
    raw = subprocess.run([str(exe)], check=True, capture_output=True).stdout

size = 135 * 240 * 3
assert len(raw) == 13 * 16 * size
fg = [bytes(map(int, m)) for m in re.findall(r'\{ (\d+), (\d+), (\d+) \}', palette.split('static const hsv_triplet_t layer_bg_hsv')[0])]
bg = [bytes(map(int, m)) for m in re.findall(r'\{ (\d+), (\d+), (\d+) \}', palette.split('static const hsv_triplet_t layer_bg_hsv')[1])]
base_bytes = bytes(map(int, base.split(',')))
sequences = []
for layer in range(13):
    hashes = []
    for frame in range(16):
        data = raw[(layer * 16 + frame) * size:(layer * 16 + frame + 1) * size]
        colors = {data[i:i+3] for i in range(0, size, 3)}
        assert colors <= {base_bytes, fg[layer], bg[layer]}, (layer, frame, 'palette drift/unpainted pixels')
        # Each 24px tile remains symmetric around its center; animate shape, not hue flashes.
        pixel = lambda x, y: data[(y * 135 + x) * 3:(y * 135 + x + 1) * 3]
        for y in range(2, 23):
            for x in range(2, 23):
                assert pixel(x, y) == pixel(24-x, y) == pixel(x, 24-y) == pixel(y, x), (layer, frame, 'lost four-way symmetry')
        assert any(pixel(x, y) != base_bytes for x in range(120, 135) for y in range(240)), (layer, 'right edge unpainted')
        normalized = bytes(0 if data[i:i+3] == base_bytes else 1 if data[i:i+3] == fg[layer] else 2 for i in range(0, size, 3))
        hashes.append(hashlib.sha256(normalized).digest())
        if args.frames:
            args.frames.mkdir(parents=True, exist_ok=True)
            (args.frames / f'layer-{layer:02}-frame-{frame:02}.hsv').write_bytes(data)
    assert len(set(hashes)) >= 4, (layer, 'expected at least four distinct animation shapes')
    sequences.append(tuple(hashes))
assert len(set(sequences)) == 13, 'Layer geometry must be unique even without its palette.'
print('13 unique symmetric animations; all 3,328 frames bounded and sanitizer-clean.')
