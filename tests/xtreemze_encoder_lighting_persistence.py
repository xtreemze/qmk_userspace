#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
keymap = (root / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c").read_text()

start = keymap.index("/* BEGIN XTREEMZE_ENCODER_LIGHTING_PERSISTENCE */")
end = keymap.index("/* END XTREEMZE_ENCODER_LIGHTING_PERSISTENCE */")
block = keymap[start:end]

required = {
    "encoder-only guard": "if (!IS_ENCODEREVENT(record->event))",
    "quiet window": "#define ENCODER_LIGHTING_PERSIST_QUIET_MS 750",
    "RGB deferred commit": "eeconfig_force_flush_rgb_matrix();",
    "backlight deferred commit": "eeconfig_update_backlight_current();",
    "RGB saturation live": "rgb_matrix_decrease_sat_noeeprom();",
    "RGB value live": "rgb_matrix_increase_val_noeeprom();",
    "RGB speed live": "rgb_matrix_decrease_speed_noeeprom();",
    "RGB mode live": "rgb_matrix_step_noeeprom();",
    "RGB hue live": "rgb_matrix_increase_hue_noeeprom();",
    "backlight live": "backlight_level_noeeprom(",
    "press-only dirty mark": "if (record->event.pressed) {\n        mark_encoder_lighting_dirty(rgb_keycode);",
}
for label, needle in required.items():
    assert needle in block, f"missing {label}: {needle}"

for persistent_call in (
    "rgb_matrix_increase_sat();",
    "rgb_matrix_decrease_sat();",
    "rgb_matrix_increase_val();",
    "rgb_matrix_decrease_val();",
    "rgb_matrix_increase_speed();",
    "rgb_matrix_decrease_speed();",
    "rgb_matrix_step();",
    "rgb_matrix_step_reverse();",
    "rgb_matrix_increase_hue();",
    "rgb_matrix_decrease_hue();",
    "backlight_increase();",
    "backlight_decrease();",
):
    assert persistent_call not in block, f"encoder path must not call persistent API directly: {persistent_call}"

process = keymap.index("bool process_record_user")
post_init = keymap.index("void keyboard_post_init_user")
process_block = keymap[process:post_init]
assert "process_encoder_lighting_keycode(keycode, record)" in process_block
assert process_block.index("process_encoder_lighting_keycode") < process_block.index("if (!record->event.pressed)")

scan = keymap.index("void matrix_scan_user")
wake = keymap.index("void suspend_wakeup_init_user")
scan_block = keymap[scan:wake]
assert "persist_encoder_lighting_if_idle();" in scan_block

print("encoder lighting persistence contract passed")
