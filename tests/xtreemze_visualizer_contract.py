#!/usr/bin/env python3
"""Source-level contract checks for the dependency-free Ferris layer atlas."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil"
VISUALIZER = ROOT / "docs/visualizer/index.html"

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
    source = VISUALIZER.read_text(encoding="utf-8")

    assert len(profile["layout"]) == 13, "visualizer contract expects 13 matrix layers"
    assert len(profile["encoder_layout"]) == 13, "visualizer contract expects 13 encoder layers"
    assert all(len(layer) == 10 for layer in profile["layout"]), "expected 10 matrix rows per layer"
    assert all(len(row) == 5 for layer in profile["layout"] for row in layer), "expected 5 matrix columns"
    assert all(len(layer) == 2 for layer in profile["encoder_layout"]), "expected two encoders per layer"
    assert all(len(pair) == 2 for layer in profile["encoder_layout"] for pair in layer), "expected CCW/CW pair"

    # The UI must remain behavior-first rather than regressing to an alpha-keycap poster.
    assert 'id="hideAlpha" type="checkbox" checked' in source
    assert "transparent/inherited" in source
    assert "QK_ALT_REPEAT_KEY" in source and "QK_REPEAT_KEY" in source
    assert "raw.githubusercontent.com/xtreemze/qmk_userspace/halcyon/" in source

    for name in LAYER_NAMES:
        assert name in source, f"missing visualizer layer identity: {name}"

    # Physical rows used by the atlas: left 0..3, right 5..8; 4 and 9 are module rows.
    assert "renderHalf('left',[0,1,2,3],'left')" in source
    assert "renderHalf('right',[5,6,7,8],'right')" in source
    assert "moduleValue(4)" in source and "moduleValue(9)" in source

    print("Ferris visualizer contract passed.")


if __name__ == "__main__":
    main()
