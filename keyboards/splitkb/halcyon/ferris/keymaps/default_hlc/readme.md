# Halcyon Ferris rev1 — Vial keymap

## Build

Example (adjust `-km` to your keymap folder name):

```sh
qmk compile -kb splitkb/halcyon/ferris/rev1 -km <your_keymap>
```

Flash the resulting firmware to the appropriate half/controller per your Halcyon setup.

## Keymap

This keymap is authored in **Vial** (dynamic keymap) and stored in the firmware’s non-volatile storage.

### Notation

- `TRNS` = transparent (falls through to lower layer)
- `NO` = unassigned
- `MO(n)` = momentary layer `n`
- `TG(n)` = toggle layer `n`
- `TO(n)` = switch to layer `n`
- `OSL(n)` = one-shot layer `n`
- `LTn(key)` / `LTn(KC_...)` = layer-tap (tap = key, hold = layer `n`)
- `OSM(MOD_...)` = one-shot modifier
- `TD(n)` = tap dance slot `n`
- `M5` = Vial macro slot 5

Layers 8–12 are currently empty (`TRNS` everywhere) and reserved.

## Layers (from `finalVial.vil`)

Each layer is shown as:

`LHS (5 keys)  |  RHS (5 keys)`

### Layer 0 — System / mouse hub

```
MO(6) MO(3) MO(5) MO(7) MO(4)  |  NO TG(2) TG(1) NO NO
LALT LSHIFT LGUI LCTRL RGB_TOG  |  MS_L MS_D MS_U MS_R NO
LGUI(KC_Z) LGUI(KC_X) LGUI(KC_C) LGUI(KC_V) SGUI(KC_Z)  |  NO BTN3 BTN4 BTN5 NO
Thumbs: LT3(KC_BSPACE) OSL(6)  |  BTN2 BTN1
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW WH_U / CW WH_D
```

### Layer 1 — Alpha (QWERTY)

```
Q W E R T  |  Y U I O P
A S D F G  |  H J K L TD(1)
Z X C V B  |  TD(4) M TD(7) TD(3) TD(0)
Thumbs: LT3(KC_BSPACE) OSL(6)  |  LT3(KC_BSPACE) SPACE
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW WH_U / CW WH_D
```

### Layer 2 — Alpha (RECURVA)

```
F R D P V  |  Q J U O Y
S N T C B  |  TD(3) H E A I
Z X K G W  |  M L TD(4) TD(7) TD(2)
Thumbs: LT3(KC_BSPACE) OSL(6)  |  LT3(KC_BSPACE) SPACE
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW WH_U / CW WH_D
```

### Layer 3 — Numbers / keypad

```
1 2 3 4 5  |  6 7 8 9 0
GRAVE BSLASH KP_SLASH KP_MINUS NO  |  KP_COMMA KP_4 KP_5 KP_6 MINUS
NO NO KP_ASTERISK KP_PLUS NO  |  KP_DOT KP_1 KP_2 KP_3 EQUAL
Thumbs: HOME LCTL(KC_A)  |  END MO(5)
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW RGB_HUD / CW RGB_HUI
```

### Layer 4 — Navigation (arrows)

```
TRNS TRNS TRNS TRNS TRNS  |  TRNS TRNS TRNS TRNS TRNS
TRNS TRNS TRNS TRNS TRNS  |  LEFT DOWN UP RIGHT TRNS
TRNS TRNS TRNS TRNS TRNS  |  TRNS TRNS TRNS TRNS TRNS
Thumbs: TRNS TRNS  |  TRNS TRNS
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW RGB_SAD / CW RGB_SAI
```

### Layer 5 — One-shot mods (right) + utilities

```
TRNS TRNS TRNS TRNS TRNS  |  TRNS TRNS TO(0) TRNS TO(1)
TRNS TRNS TRNS TRNS TRNS  |  PGDOWN OSM(MOD_RCTL) OSM(MOD_RGUI) OSM(MOD_RSFT) OSM(MOD_RALT)
TRNS TRNS TRNS TRNS TRNS  |  TRNS TRNS TRNS TRNS TRNS
Thumbs: LGUI(KC_S) LALT(KC_BSPACE)  |  TAB TRNS
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW RGB_VAD / CW RGB_VAI
```

### Layer 6 — One-shot mods (left) + utilities

```
TO(1) TRNS TO(0) TRNS TRNS  |  TRNS TRNS TRNS TRNS TRNS
OSM(MOD_LALT) OSM(MOD_LSFT) OSM(MOD_LGUI) OSM(MOD_LCTL) PGUP  |  TRNS TRNS TRNS TRNS TRNS
LGUI(KC_Z) LGUI(KC_X) LGUI(KC_C) LGUI(KC_V) SGUI(KC_Z)  |  TRNS TRNS TRNS TRNS TRNS
Thumbs: LGUI(KC_SPACE) M5  |  TAB MO(5)
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW BRID / CW BRIU
```

### Layer 7 — Function / media / arrows

```
F1 F2 F3 F4 F5  |  F8 F9 F10 F11 F12
KP_SLASH LSFT(KC_DOT) LSFT(KC_RBRACKET) LSFT(KC_0) RBRACKET  |  LBRACKET LSFT(KC_9) LSFT(KC_LBRACKET) LSFT(KC_COMMA) BSLASH
MPRV MSTP MPLY MNXT F6  |  F7 TAB TRNS TRNS TRNS
Thumbs: UP LEFT  |  DOWN RIGHT
Encoders: E0 CCW VOLD / CW VOLU ; E1 CCW RGB_RMOD / CW RGB_MOD
```
### Layers 8–12 — Reserved

All keys are `TRNS` (no assignments yet).
