# Halcyon Ferris rev1 - xtreemze_final

This keymap is generated from `finalVial.vil` into compiled QMK structures and shipped with
Vial enabled.

- Compiled firmware contains 13 default layers (`L0..L12`) plus all translated behaviors.
- Vial dynamic keymap storage can override those defaults after flashing.
- Reset EEPROM to return to compiled defaults.

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
