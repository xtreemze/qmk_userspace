# Factory profile and TFT update — 2026-08-26

This follows the user-accepted legacy migration commit `1939c3a`. The repository's
main/default branch is `halcyon`; the dirty `diagnostics/usb-resume` checkout and
its nested QMK changes are outside this update.

## Scope

The new canonical `xtreemzeVial.vil` replaces all exported factory data: 13 matrix
layers, 26 encoder pairs, 32 slots in each of five dynamic tables, 26 QMK settings
and layout options. Eight matrix layers, one encoder layer, nine macro slots,
six tap dances, three combos, one alt-repeat entry and ten settings differ from
the previous export. Key overrides are retained exactly.

Factory marker `0xAE` becomes `0xAF` to apply the profile once on boot. Later Vial
edits persist; the user datablock version, custom RGB profiles and saved host
family remain unchanged. Macro seeding now uses Vial's null-safe encoding:
three-byte basic taps and escaped extended keycodes. Legacy USER macro aliases
use Vial's current buffer. All 32 slot boundaries are explicit, and insufficient
macro capacity cannot mark a partial seed complete.

Layers are labeled `MOUSE`, `QWERTY`, `COLEMAK`, `NUMSYMS`, `NUMFLIP`, `ONESHOT`,
`EDITING`, `FNSYMS`, `FNFLIP`, `SYMBOLS`, `RGBHUE`, `RGBVAL`, `BKLIGHT`. Thirteen
new symmetric procedural diamond patterns preserve the palette and 200ms timing,
with a smooth 16-frame shape cycle instead of temporal foreground/background
color swapping. Host overlays, modifier indicators and wake recovery are unchanged.

The user's follow-up explicitly includes restoring real PWM brightness. Both
module builds now enable QMK backlight/PWM so their split packet layouts match.
TFT output uses PWMD5/channel B/GP27, ten levels, PWM5/HAL enabled and the RP2040
PWM pin mux. The encoder target retains the unconnected GP2 placeholder.
Idle/suspend handling uses transient zero and restores the persisted level/enable
flag; it never saves idle as a user preference or overrides manual off. The first
brightness key after idle adjusts the restored level. The slave follows QMK's
master state. Breathing remains off. No hard-coded brightness override is shipped.

This matches Splitkb's documented [display brightness controls](https://docs.splitkb.com/product-guides/halcyon-series/modules/display)
and [GP27/VIK_GPIO1 mapping](https://docs.splitkb.com/product-guides/halcyon-series/schematics/controller-specification).
The profile has no separately exported RGB state. M10 is stored as literal macro
text, never run during import or validation.

## Verification

- Source parity checks compare every matrix position, encoder pair, ordered dynamic
  slot, setting, macro action and empty slot against the exact export. Generator
  check mode confirms the compiled tables are current.
- Host C tests compile the actual seeder and pinned Vial decoder. All 32 macros
  round-trip, including literal text, basic and extended keys; byte-zero escaping,
  insufficient capacity and one-time reseeding/persistence pass under sanitizers.
- Host raster tests execute the real firmware drawing functions for all 13 layers
  and all 256 frame counter values (3,328 frames). Bounds, bounded draw-call count,
  16-frame periodicity, fixed palettes, four-way tile symmetry, distinct geometry
  and the clipped final column pass under address/undefined-behavior sanitizers.
- Host C tests execute the pinned QMK backlight state, key processor and PWM
  driver with observed HAL calls. Levels 1 and 10 yield distinct duty cycles;
  up/down/step/toggle, idle wake, manual-off persistence and slave ownership pass.
  Actual build flags/post-config also confirm both targets compile the processor
  and PWM driver and that the TFT's final pin is GP27, slice 5/channel B, ten levels,
  PWM5/HAL and PWM pin mux enabled, with breathing absent.
- All six repository shell guards, strict keymap lint and 39 OS-detection tests pass.
- Display and encoder builds pass on pinned Vial `dd43959ae5c08d8a28d38a1acf7b04e86b14a344`
  using QMK CLI 1.1.8 / ARM GCC 8.5.0_2. Both retain `sym_defer_pk` / 5ms.
- These are software/build checks. The agent has not flashed or physically tested
  the new defaults/animations/PWM. The user's earlier acceptance applies to the
  preceding legacy candidate, not to this new profile revision.

## Local artifacts

| File | SHA-256 |
| --- | --- |
| `halcyon_defaults_display.uf2` | `d918884cc75af7d00c4890acf23c2fbef98d591804bfb760f8e5b70768f9d0ac` |
| `halcyon_defaults_encoder.uf2` | `e34a78710a5d90e34765f853e423914477bf5be0612cf569e2b680fc371dbd7d` |
| Canonical `xtreemzeVial.vil` | `1f6d264bbfa1b8e9bd55a51e6456c145b6f6903a011ab7ea283cc8171c5a9bf6` |

These local images were built before the commit; the exact bytes are identified
above. CI builds may differ with their toolchain and embedded version metadata.
Only a successful default-branch build updates release `latest`; branch builds
continue to use `halcyon-legacy-migration`.

Before flashing, export any newer Vial changes. Flash both halves together: use the display image on the TFT
half and encoder image on the rotary half. Backlighting adds a split transaction,
so mixing old and new images is not supported. First boot applies the attached profile;
verify macros, tap dances, mirrored layers, encoders and every TFT label/pattern.
Then make a temporary Vial edit, reboot to confirm persistence and restore it.
Check typing, reconnect and suspend/resume against the accepted firmware. A later
full EEPROM reset restores defaults but also clears saved RGB/host state.

For PWM acceptance, use layer 12 to set level 1 and level 10 and compare brightness;
verify toggle and step, both encoder mappings, manual off across idle wake, and
saved brightness across reboot and USB sleep/wake. Repeat with USB on each half.
The software audit and mocked HAL duty-cycle test do not prove physical PWM output.
