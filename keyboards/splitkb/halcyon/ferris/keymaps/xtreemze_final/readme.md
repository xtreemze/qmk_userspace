# Halcyon Ferris rev1 - xtreemze_final

The canonical factory profile is `xtreemzeVial.vil`, imported byte-for-byte from
the user's 2026-08-26 export (SHA-256
`1f6d264bbfa1b8e9bd55a51e6456c145b6f6903a011ab7ea283cc8171c5a9bf6`).

- All 13 matrix layers and both encoder slots on every layer are compiled in.
- All 32 slots for macros, tap dance, combos, key overrides and alt-repeat are
  seeded, including cleared entries. All 26 exported QMK settings and layout
  option `1` are applied. UID and protocol fields identify the compatible device;
  the export contains no separate RGB profiles or TFT brightness state.
- **On the first boot of this profile revision, marker `0xAF` replaces the old
  Vial dynamic data with these defaults. Export any newer on-device edits before
  flashing.** No manual EEPROM reset is needed. Later edits persist across boots.
- The user data format remains `0x02`; saved custom RGB profiles and the stored
  shortcut host family are preserved. A full EEPROM reset also clears those.
- Legacy `USER0..USER9` macro buttons now dispatch the corresponding current Vial
  macro slots. Macro text (including M10's shell command) is literal payload only;
  importing/building the profile never executes it.

Regenerate compiled data from the repository root:

```sh
node scripts/generate-vial-defaults.mjs
bash tests/xtreemze_vial_factory_defaults.sh
```

The generator fails on unsupported macro actions rather than silently discarding
profile data. Review the factory marker separately for each intentional update.

## TFT layer identities

Labels contain letters only and fit the existing seven-character limit. Each
layer has unique procedural diamond geometry, including encoder-only RGB layers.

| Layer | Label | Role | Pattern |
| --- | --- | --- | --- |
| 0 | MOUSE | Mouse, shortcuts and layer access | Compass points |
| 1 | QWERTY | QWERTY typing | Woven center |
| 2 | COLEMAK | Alternate typing layout | Nested lattice |
| 3 | NUMSYMS | Numbers, keypad and symbols | Counting beads |
| 4 | NUMFLIP | Mirrored digits; saturation encoder | Counter-moving beads |
| 5 | ONESHOT | One-shot modifiers | Expanding impulse |
| 6 | EDITING | Editing, shortcuts and macro access | Cut-paper aperture |
| 7 | FNSYMS | Function keys, symbols and navigation | Stepped rosette |
| 8 | FNFLIP | Mirrored function keys; speed encoder | Rosette counterpoint |
| 9 | SYMBOLS | Shifted number symbols; mode encoder | Punctuation lattice |
| 10 | RGBHUE | RGB hue encoder | Eight-point orbit |
| 11 | RGBVAL | RGB value encoder | Nested aperture |
| 12 | BKLIGHT | Exported backlight buttons/encoders | Soft sun |

Animation retains the existing foreground/background HSV palette, dark base,
24px tiling, clipped final column and 200ms frame interval. A 16-frame/3.2-second
cycle changes radii or spacing at most one pixel at a time, with four-way symmetry
and fixed colors. Rendering remains in housekeeping; no waits, allocation,
USB changes or display work were added to wake callbacks.

## TFT backlight controls

Both module builds enable QMK backlight processing and the ChibiOS PWM driver,
so either USB master can process layer-12 buttons and encoder events. The TFT
uses `PWMD5`, channel B, GP27, ten brightness levels, with PWM5/HAL enabled and
the PWM pin mux. The encoder target keeps the upstream unconnected GP2 placeholder;
it must not take GP27 away from the encoder module.

`BL_INC` / `BL_DEC` translate to current QMK `BL_UP` / `BL_DOWN`. Increase,
decrease, step and toggle use QMK's persistent brightness state. Idle/suspend
blanking is transient, does not write EEPROM, and restores the saved brightness
and enable flag—including manual off. The first key after inactivity restores
that state before QMK processes any brightness adjustment. The slave follows
QMK's synchronized effective level. **Flash both halves together**: enabling
backlighting adds a shared split transaction.

`BL_BRTG` is retained exactly as exported but remains inactive because breathing
is not enabled. No hard-coded level override or diagnostic loop is shipped.
Physical acceptance must compare level 1 against level 10, toggle/step, both
encoders, idle wake and USB sleep/wake with the TFT half as both master and slave.

Audit the final compiled configuration after building:

```sh
python3 scripts/check-halcyon-backlight-build.py halcyon_defaults_display halcyon_defaults_encoder
```

## RGB profile keycodes (Vial custom buttons)

- `SLYR`: Save the current RGB Matrix mode/hsv/speed to the active layer profile.
- `SMOD`: Hold one or more modifiers, then press to save current RGB to those mod profiles.
- `SCHD`: Save current RGB as the chord override profile.
- `TCHD`: Trigger the chord override profile.
- `DUR+` / `DUR-`: Increase/decrease chord override duration (default `2000ms`, persistent).

## OS fingerprint diagnostics

- Vial `USER20` (`OS_TRACE_VIEW`, shown as `Trace`) toggles the manual H1F TRACE VIEW.
- The canonical profile assigns TRACE VIEW on layer 0 at matrix row 5, column 0 (the right top inner key); pressing it again exits the manual view.
- Automatic HOST LINK telemetry always returns to the normal TFT UI after at most `2000ms`.

## OS-aware shortcut policy

- Suspend/resume preserves the last effective Apple/Ctrl family; USB reinitialization does not soft-reset the keyboard.
- A QMK detector callback is only a candidate. It must remain unchanged for `1500ms` before affecting shortcuts and `10000ms` before being persisted.
- Wake adds a `5000ms` guard before post-resume detector evidence may replace the effective family.
- `OS_UNSURE` cancels pending evidence but never replaces a confirmed family. With neither stored nor settled evidence, OS-aware clipboard keys intentionally send nothing.

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
