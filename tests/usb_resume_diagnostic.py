#!/usr/bin/env python3
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
rules = (root / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/rules.mk").read_text()
qmk = json.loads((root / "qmk.json").read_text())
script = (root / "scripts/build-usb-resume-diagnostic.sh").read_text()

assert "ifeq ($(XTREEMZE_USB_RESET_DIAGNOSTIC),yes)" in rules
assert "OPT_DEFS += -DOS_DETECTION_KEYBOARD_RESET" in rules
assert "XTREEMZE_USB_RESET_DIAGNOSTIC" not in json.dumps(qmk), "production qmk.json must not enable diagnostic reset"
assert "OS_DETECTION_KEYBOARD_RESET" not in json.dumps(qmk), "production targets must stay reset-free"
assert "-e XTREEMZE_USB_RESET_DIAGNOSTIC=yes" in script
assert "display_usb_reset_diag" in script and "encoder_usb_reset_diag" in script
print("USB resume diagnostic remains opt-in and outside production targets.")
