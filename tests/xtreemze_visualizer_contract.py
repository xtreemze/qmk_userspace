#!/usr/bin/env python3
"""Source-level contract checks for the dependency-free Ferris layer atlas."""
from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil"
VISUALIZER = ROOT / "docs/visualizer"
INDEX = VISUALIZER / "index.html"
SCRIPT = VISUALIZER / "atlas.js"
STYLES = VISUALIZER / "atlas.css"
ENHANCEMENTS = VISUALIZER / "atlas-enhancements.js"
ENHANCEMENT_STYLES = VISUALIZER / "atlas-enhancements.css"

LAYER_NAMES = (
    "MOUSE",
    "QWERTY",
    "COLEMAK",
    "NUMSYMS",
    "NUMFLIP",
    "ONESHOT",
    "EDITING",
    "FNSYMS",
    "FNFLIP",
    "SYMBOLS",
    "RGBHUE",
    "RGBVAL",
    "BKLIGHT",
)


def key_polygon(x: float, y: float, angle: float, size: float = 0.9) -> tuple[tuple[float, float], ...]:
    """Return a rotated square keycap polygon in normalized key units."""
    cx = x + size / 2
    cy = y + size / 2
    radians = math.radians(angle)
    cos_a = math.cos(radians)
    sin_a = math.sin(radians)
    points = []
    for dx, dy in ((-size / 2, -size / 2), (size / 2, -size / 2), (size / 2, size / 2), (-size / 2, size / 2)):
        points.append((cx + dx * cos_a - dy * sin_a, cy + dx * sin_a + dy * cos_a))
    return tuple(points)


def polygons_overlap(a: tuple[tuple[float, float], ...], b: tuple[tuple[float, float], ...]) -> bool:
    """Separating-axis test: touching is allowed, area overlap is not."""
    for polygon in (a, b):
        for i, (x1, y1) in enumerate(polygon):
            x2, y2 = polygon[(i + 1) % len(polygon)]
            edge_x, edge_y = x2 - x1, y2 - y1
            axis_x, axis_y = -edge_y, edge_x
            length = math.hypot(axis_x, axis_y)
            axis_x /= length
            axis_y /= length
            projection_a = [x * axis_x + y * axis_y for x, y in a]
            projection_b = [x * axis_x + y * axis_y for x, y in b]
            if max(projection_a) <= min(projection_b) or max(projection_b) <= min(projection_a):
                return False
    return True


def main() -> None:
    profile = json.loads(PROFILE.read_text(encoding="utf-8"))
    index = INDEX.read_text(encoding="utf-8")
    script = SCRIPT.read_text(encoding="utf-8")
    styles = STYLES.read_text(encoding="utf-8")
    enhancements = ENHANCEMENTS.read_text(encoding="utf-8")
    enhancement_styles = ENHANCEMENT_STYLES.read_text(encoding="utf-8")

    assert len(profile["layout"]) == 13, "visualizer contract expects 13 matrix layers"
    assert len(profile["encoder_layout"]) == 13, "visualizer contract expects 13 encoder layers"
    assert all(len(layer) == 10 for layer in profile["layout"]), "expected 10 matrix rows per layer"
    assert all(len(row) == 5 for layer in profile["layout"] for row in layer), "expected 5 matrix columns"
    assert all(len(layer) == 2 for layer in profile["encoder_layout"]), "expected two encoders per layer"
    assert all(len(pair) == 2 for layer in profile["encoder_layout"] for pair in layer), "expected CCW/CW pair"

    # Keep the deployed page dependency-free while allowing presentation and behavior
    # to live in dedicated source files.
    assert '<link rel="stylesheet" href="atlas.css">' in index
    assert '<link rel="stylesheet" href="atlas-enhancements.css">' in index
    assert '<script src="atlas.js" defer></script>' in index
    assert '<script src="atlas-enhancements.js" defer></script>' in index

    # The UI must remain behavior-first rather than regressing to an alpha-keycap poster.
    assert 'id="hideAlpha" type="checkbox" checked' in index
    assert "transparent / inherited" in index
    assert "QK_ALT_REPEAT_KEY" in script and "QK_REPEAT_KEY" in script
    assert "raw.githubusercontent.com/xtreemze/qmk_userspace/halcyon/" in script

    for name in LAYER_NAMES:
        assert name in script, f"missing visualizer layer identity: {name}"

    # Physical contract: model one 17-key half and mirror the opposite half. The
    # matrix offsets remain left 0..3/right 5..8, while rows 4 and 9 are modules.
    assert "const PHYSICAL=[" in script
    assert "{slot:'thumb-outer',row:3,col:1}" in script
    assert "{slot:'thumb-inner',row:3,col:0}" in script
    assert "renderHalf('left',0,4,'left')" in script
    assert "renderHalf('right',5,9,'right')" in script
    assert "moduleValue(4)" in script and "moduleValue(9)" in script
    assert ".half.right{transform:scaleX(-1)}" in styles
    assert ".right .key-face,.right .module-inner{transform:scaleX(-1)}" in styles

    # Canonical Ferris Sweep physical coordinates. The Halcyon matrix is reversed
    # across each alpha row, so c4 is the outside column and c0 is the inside column.
    ferris_positions = {
        "c4-r0": (0.0, 0.93, 0.0), "c4-r1": (0.0, 1.93, 0.0), "c4-r2": (0.0, 2.93, 0.0),
        "c3-r0": (1.0, 0.31, 0.0), "c3-r1": (1.0, 1.31, 0.0), "c3-r2": (1.0, 2.31, 0.0),
        "c2-r0": (2.0, 0.00, 0.0), "c2-r1": (2.0, 1.00, 0.0), "c2-r2": (2.0, 2.00, 0.0),
        "c1-r0": (3.0, 0.28, 0.0), "c1-r1": (3.0, 1.28, 0.0), "c1-r2": (3.0, 2.28, 0.0),
        "c0-r0": (4.0, 0.42, 0.0), "c0-r1": (4.0, 1.42, 0.0), "c0-r2": (4.0, 2.42, 0.0),
        "thumb-outer": (3.5, 3.75, 15.0),
        "thumb-inner": (4.5, 4.00, 30.0),
    }
    css_rules = {
        "c4-r0": ".slot-c4-r0{left:calc(var(--u)*0);top:calc(var(--u)*.93)}",
        "c4-r1": ".slot-c4-r1{left:calc(var(--u)*0);top:calc(var(--u)*1.93)}",
        "c4-r2": ".slot-c4-r2{left:calc(var(--u)*0);top:calc(var(--u)*2.93)}",
        "c3-r0": ".slot-c3-r0{left:calc(var(--u)*1);top:calc(var(--u)*.31)}",
        "c3-r1": ".slot-c3-r1{left:calc(var(--u)*1);top:calc(var(--u)*1.31)}",
        "c3-r2": ".slot-c3-r2{left:calc(var(--u)*1);top:calc(var(--u)*2.31)}",
        "c2-r0": ".slot-c2-r0{left:calc(var(--u)*2);top:0}",
        "c2-r1": ".slot-c2-r1{left:calc(var(--u)*2);top:calc(var(--u)*1)}",
        "c2-r2": ".slot-c2-r2{left:calc(var(--u)*2);top:calc(var(--u)*2)}",
        "c1-r0": ".slot-c1-r0{left:calc(var(--u)*3);top:calc(var(--u)*.28)}",
        "c1-r1": ".slot-c1-r1{left:calc(var(--u)*3);top:calc(var(--u)*1.28)}",
        "c1-r2": ".slot-c1-r2{left:calc(var(--u)*3);top:calc(var(--u)*2.28)}",
        "c0-r0": ".slot-c0-r0{left:calc(var(--u)*4);top:calc(var(--u)*.42)}",
        "c0-r1": ".slot-c0-r1{left:calc(var(--u)*4);top:calc(var(--u)*1.42)}",
        "c0-r2": ".slot-c0-r2{left:calc(var(--u)*4);top:calc(var(--u)*2.42)}",
        "thumb-outer": ".slot-thumb-outer{left:calc(var(--u)*3.5);top:calc(var(--u)*3.75);transform:rotate(15deg)}",
        "thumb-inner": ".slot-thumb-inner{left:calc(var(--u)*4.5);top:calc(var(--u)*4);transform:rotate(30deg)}",
    }
    for name, rule in css_rules.items():
        assert rule in enhancement_styles, f"missing canonical Ferris geometry rule for {name}: {rule}"

    # A physical Ferris cannot have overlapping keycaps. Test every pair as rotated
    # 0.9u squares so future presentation edits cannot reintroduce the thumb collision.
    polygons = {name: key_polygon(*position) for name, position in ferris_positions.items()}
    names = list(polygons)
    for i, first in enumerate(names):
        for second in names[i + 1:]:
            assert not polygons_overlap(polygons[first], polygons[second]), f"Ferris keycaps overlap: {first} / {second}"
    assert "Keycaps are never allowed to overlap" in index

    # The display is an emulator only: animated imagery plus live layer identity.
    assert "data:image/gif;base64," in script
    assert "Animated TFT display activity emulator" in script
    assert "tft-layer" in script and "tft-name" in script

    # Tap dances and combos must expose their configured behavior, not opaque slot IDs.
    assert any(any(str(k).startswith("TD(") for row in layer for k in row) for layer in profile["layout"])
    assert any(entry[:4] != ["KC_NO"] * 4 for entry in profile["tap_dance"])
    assert any(entry[0] != "KC_NO" for entry in profile["combo"])
    assert "const TAP_FIELDS=['tap','hold','double tap','tap + hold']" in enhancements
    assert "profile?.tap_dance?.[index]" in enhancements
    assert "profile?.combo?.[index]" in enhancements
    assert "macroText(Number(m[1])" in enhancements
    assert "number badge = available combo(s)" in index

    # Saved RGB profiles are on-device EEPROM state. The Atlas may read them through
    # the custom raw-HID protocol but must not mutate them.
    assert "const RGB_COMMAND=0xF0" in enhancements
    assert "CAPABILITIES:0x01,GET_PROFILE:0x02,GET_COMBO_DURATION:0x06" in enhancements
    assert "navigator.hid.requestDevice" in enhancements
    assert "SCOPE={GLOBAL:0,LAYER:1,MODIFIER:2,COMBO:3}" in enhancements
    assert "SET_PROFILE" not in enhancements
    assert "rgb_profile" not in profile, ".vil export unexpectedly became the RGB EEPROM source; review Atlas source boundaries"
    assert "Saved RGB profiles live in keyboard EEPROM" in enhancements
    assert ".key.rgb-lit:not(.disabled)" in enhancement_styles

    # Narrow layouts must reflow rather than force the desktop split surface to scroll.
    assert "@media(max-width:700px)" in styles
    assert ".keyboard{--u:min(14.4vw,61px);grid-template-columns:1fr" in styles
    assert ".half.right{margin-top:2px}" in styles
    assert "@media(prefers-reduced-motion:reduce)" in styles
    assert "@media(max-width:700px)" in enhancement_styles

    print("Ferris visualizer contract passed.")


if __name__ == "__main__":
    main()
