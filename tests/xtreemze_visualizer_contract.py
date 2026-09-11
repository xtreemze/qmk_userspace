#!/usr/bin/env python3
"""Source-level contract checks for the dependency-free Ferris layer atlas."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil"
VISUALIZER = ROOT / "docs/visualizer"
INDEX = VISUALIZER / "index.html"
SCRIPT = VISUALIZER / "atlas.js"
STYLES = VISUALIZER / "atlas.css"

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


def main() -> None:
    profile = json.loads(PROFILE.read_text(encoding="utf-8"))
    index = INDEX.read_text(encoding="utf-8")
    script = SCRIPT.read_text(encoding="utf-8")
    styles = STYLES.read_text(encoding="utf-8")

    assert len(profile["layout"]) == 13, "visualizer contract expects 13 matrix layers"
    assert len(profile["encoder_layout"]) == 13, "visualizer contract expects 13 encoder layers"
    assert all(len(layer) == 10 for layer in profile["layout"]), "expected 10 matrix rows per layer"
    assert all(len(row) == 5 for layer in profile["layout"] for row in layer), "expected 5 matrix columns"
    assert all(len(layer) == 2 for layer in profile["encoder_layout"]), "expected two encoders per layer"
    assert all(len(pair) == 2 for layer in profile["encoder_layout"] for pair in layer), "expected CCW/CW pair"

    # Keep the deployed page dependency-free while allowing presentation and behavior
    # to live in dedicated source files.
    assert '<link rel="stylesheet" href="atlas.css">' in index
    assert '<script src="atlas.js" defer></script>' in index

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

    # Ferris stagger and thumb angles are explicit and deterministic.
    for stagger in (
        ".slot-c4-r0{left:calc(var(--u)*0);top:calc(var(--u)*1.25)}",
        ".slot-c3-r0{left:calc(var(--u)*1);top:calc(var(--u)*.5)}",
        ".slot-c2-r0{left:calc(var(--u)*2);top:0}",
        ".slot-c1-r0{left:calc(var(--u)*3);top:calc(var(--u)*.5)}",
        ".slot-c0-r0{left:calc(var(--u)*4);top:calc(var(--u)*.75)}",
        "transform:rotate(15deg)",
        "transform:rotate(30deg)",
    ):
        assert stagger in styles, f"missing Ferris geometry rule: {stagger}"

    # The display is an emulator only: animated imagery plus live layer identity.
    assert "data:image/gif;base64," in script
    assert "Animated TFT display activity emulator" in script
    assert "tft-layer" in script and "tft-name" in script

    # Narrow layouts must reflow rather than force the desktop split surface to scroll.
    assert "@media(max-width:700px)" in styles
    assert ".keyboard{--u:min(14.4vw,61px);grid-template-columns:1fr" in styles
    assert ".half.right{margin-top:2px}" in styles
    assert "@media(prefers-reduced-motion:reduce)" in styles

    print("Ferris visualizer contract passed.")


if __name__ == "__main__":
    main()
