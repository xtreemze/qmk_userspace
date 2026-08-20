# Halcyon Ferris rev1 - xtreemze_final

This keymap is generated from `finalVial.vil` into compiled QMK structures and shipped with
Vial enabled.

- Compiled firmware contains 13 default layers (`L0..L12`) plus all translated behaviors.
- Vial dynamic keymap storage can override those defaults after flashing.
- Reset EEPROM to return to compiled defaults.

## RGB profile keycodes (Vial custom buttons)

- `SLYR`: Save the current RGB Matrix mode/hsv/speed to the active layer profile.
- `SMOD`: Hold one or more modifiers, then press to save current RGB to those mod profiles.
- `SCHD`: Save current RGB as the chord override profile.
- `TCHD`: Trigger the chord override profile.
- `DUR+` / `DUR-`: Increase/decrease chord override duration (default `2000ms`, persistent).

## OS fingerprint diagnostics

- Vial `USER20` (`OS_TRACE_VIEW`, shown as `Trace`) toggles the manual H1F TRACE VIEW.
- TRACE VIEW is intentionally unassigned in the compiled physical keymap; assign it temporarily through Vial when reviewing a RAM capture.
- Automatic HOST LINK telemetry always returns to the normal TFT UI after at most `2000ms`.

## Module builds

Build left-half firmware (TFT module):

```bash
qmk compile -kb splitkb/halcyon/ferris/rev1 -km xtreemze_final -e HLC_TFT_DISPLAY=1 -e TARGET=splitkb_halcyon_ferris_rev1_xtreemze_final_display
```

Build right-half firmware (rotary encoder module):

```bash
qmk compile -kb splitkb/halcyon/ferris/rev1 -km xtreemze_final -e HLC_ENCODER=1 -e TARGET=splitkb_halcyon_ferris_rev1_xtreemze_final_encoder
```

Flash the left half with the `*_display` artifact and the right half with the `*_encoder` artifact.
