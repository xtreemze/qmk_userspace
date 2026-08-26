# Halcyon legacy migration candidate

## Acceptance boundary

On 2026-08-26 the user confirmed that firmware `9b615b9e7b92b386d7cee07315a4b3f5b27d5a5d`
passed physical fast typing, including Space during rapid alternating letters,
with the existing Vial EEPROM. This is a user-reported debounce acceptance, not
a new suspend/resume certification.

The migration remains stacked on that commit. `sym_defer_pk` and `DEBOUNCE 5`
stay unchanged. The user subsequently confirmed that the legacy migration works
and authorized merging it into the default `halcyon` branch on 2026-08-26.
This closes the user-reported legacy acceptance gate; it does not certify future
default-profile or animation changes, nor a native Halcyon matrix conversion.

## Scope and provenance

| Input | Revision |
| --- | --- |
| Accepted debounce userspace | `9b615b9e7b92b386d7cee07315a4b3f5b27d5a5d` |
| Starting migration userspace | `9eb6b0c8e8af21dd7e51f3380a5f4f9287ae910b` |
| [Splitkb Halcyon userspace](https://github.com/splitkb/qmk_userspace/tree/e0900a55dbb08ff5770236dd6129fedacab620e4) | `e0900a55dbb08ff5770236dd6129fedacab620e4` |
| [Vial-QMK](https://github.com/vial-kb/vial-qmk/tree/dd43959ae5c08d8a28d38a1acf7b04e86b14a344) | `dd43959ae5c08d8a28d38a1acf7b04e86b14a344` |
| [QMK reusable workflows](https://github.com/qmk/.github/tree/01daf5113fa50804558f21cc074ab99ba84ddeaf) | `01daf5113fa50804558f21cc074ab99ba84ddeaf` |

The selected Vial revision differs from the previous QMK baseline `00fc4627`
only in an unrelated Planck keymap. No Ferris, matrix, debounce, OS-detection,
or other core source changed in that revision range.

This is the first, legacy-preserving slice, not a wholesale module upgrade:

- Import the `HALCYON_LEGACY`, 10-row matrix and layout section from Splitkb's
  Ferris `vial_hlc_legacy/config.h` into `xtreemze_final/halcyon_legacy.h`.
- Import its `halcyon_overrides.c` body unchanged, adding license/provenance
  comments required by strict lint and making legacy RGB/matrix mapping
  keymap-owned. Explicitly compile that file from the custom keymap's rules.
- Let the shared module config/RGB fallbacks yield to legacy keymap ownership.
  Other existing keymaps retain their fallback definitions.
- Preserve the custom scanner, encoder button debounce path, display, host-family
  policy, keymap, Vial JSON/profile, factory seeding and EEPROM markers. In
  particular, do not adopt upstream's direct debounced-matrix button writes or
  its broader feature defaults during this equivalence experiment.
- Carry only the existing fingerprint patch, now as an explicit build dependency.
  Do not copy the dirty diagnostic QMK checkout or overwrite `users/halcyon_modules`.
- Pin the Vial source and both reusable workflows. Successful CI candidates use
  release `halcyon-legacy-migration`, leaving `latest` alone. Nothing was published.

Pinning these inputs does not make CI byte-reproducible: the pinned reusable
workflow still references a moving QMK CLI container, action tags and unpinned
Python dependency installation. Local evidence below identifies the toolchain.

## Local setup and builds

Work was isolated in `/Users/carlos/dev/qmk_userspace-halcyon-migration` on
`migrate-latest-halcyon-vial`. The original `diagnostics/usb-resume` checkout and
its dirty nested QMK files were left untouched. An independent QMK checkout is
at `/Users/carlos/dev/vial-qmk-halcyon-migration`; the migration worktree's ignored
`qmk_firmware` symlink points there. No global QMK configuration was changed.

For a fresh checkout, from the userspace root (do not reapply to an already
patched checkout):

```sh
git clone https://github.com/vial-kb/vial-qmk.git qmk_firmware
git -C qmk_firmware checkout --detach dd43959ae5c08d8a28d38a1acf7b04e86b14a344
git -C qmk_firmware submodule update --init --recursive
ln -s "$PWD/converters/promicro_to_halcyon" qmk_firmware/platforms/chibios/converters/promicro_to_halcyon
git -C qmk_firmware apply --check ../patches/0001-os-detection-fingerprint-trace.patch
git -C qmk_firmware apply ../patches/0001-os-detection-fingerprint-trace.patch
```

Build each half from the userspace root. Use a local CLI config because QMK CLI
1.1.8's saved global config takes precedence over environment-only overrides:

```sh
qmk --config-file "$PWD/.qmk-migration.ini" config \
  user.qmk_home="$PWD/qmk_firmware" user.overlay_dir="$PWD"
make QMK_FIRMWARE_ROOT="$PWD/qmk_firmware" QMK_BIN="$PWD/scripts/qmk-local" -j4 \
  splitkb/halcyon/ferris/rev1:xtreemze_final HLC_TFT_DISPLAY=1 SKIP_GIT=yes TARGET=halcyon_legacy_display
make QMK_FIRMWARE_ROOT="$PWD/qmk_firmware" QMK_BIN="$PWD/scripts/qmk-local" -j4 \
  splitkb/halcyon/ferris/rev1:xtreemze_final HLC_ENCODER=1 SKIP_GIT=yes TARGET=halcyon_legacy_encoder
```

`scripts/qmk-local` forwards the config to every CLI subprocess. A multiword
`QMK_BIN="qmk --config-file ..."` does not survive QMK's recursive make argument
passing. `QMK_LOCAL_CONFIG` may select an alternative isolated config for the
baseline comparison; it never changes the global config.

## Verification on 2026-08-26

- Display and encoder builds passed using QMK CLI 1.1.8 and Homebrew ARM GCC
  8.5.0_2. Both compiled `quantum/debounce/sym_defer_pk.c`.
- All six existing shell guards passed, including complete factory profile parity.
- Strict `qmk --config-file CONFIG lint -kb splitkb/halcyon/ferris/rev1
  -km xtreemze_final --strict` and `userspace-doctor` passed with an isolated CLI
  config selecting the migration userspace and pinned firmware checkout.
- `make test:os_detection COMPILEFLAGS=-Wno-error=include-next-absolute-path`
  passed 39 tests. The existing macOS `include_next` warning remains visible.
- Root and patched-QMK `git diff --check` passed.
- Built detached userspace `9b615b9` for both modules against the same pinned QMK,
  fingerprint patch and toolchain. Compared 16 initialized ELF data symbols per
  module: keymaps (1,300 bytes), encoder maps (104), RGB mapping (188), Vial
  definition (840), chordal hold map, four dynamic-default tables, QMK settings,
  and six USB descriptor/report/string tables. All matched byte for byte.
- Preprocessed display and encoder configurations retained 10 x 5 matrix dimensions, debounce
  5, encoder resolution 2, 13 layers, all 32-slot capacities, 128-byte user EEPROM
  reservation, Vial UID, unlock coordinates and default layout options.
- The binary comparison rejects an intentionally altered keymap byte and an ELF
  stripped of required symbols. Whole UF2 files are not byte-identical, and this
  comparison does not establish runtime or physical equivalence.

The reusable binary check is:

```sh
python3 tests/xtreemze_legacy_binary_equivalence.py BASELINE.elf CANDIDATE.elf
```

Local comparison ELFs are under
`/Users/carlos/dev/vial-qmk-halcyon-migration/.build/`, with target names
`halcyon_debounce_baseline_display`, `halcyon_debounce_baseline_encoder`,
`halcyon_legacy_display`, and `halcyon_legacy_encoder`.

## Accepted candidate artifacts

The candidate was built from the then-uncommitted migration working tree.
The hashes below identify the candidate presented for the user-reported acceptance.
The agent did not flash it or independently perform the hardware checks.

| Artifact in the migration userspace root | SHA-256 |
| --- | --- |
| `halcyon_legacy_display.uf2` | `d27d1b93768a2dbce0f01cd4b8df6538d7a321fe4c5c71502e0876fede6f2737` |
| `halcyon_legacy_encoder.uf2` | `33336d46d09556b101d1efc0abb8905d17aa2cd8522f4994e25909965489756e` |

Before testing, export a fresh Vial backup; the repository `.vil` is a factory
profile, not necessarily the current on-device state. Flash only the matching
module image to each half, without clearing EEPROM or reseeding defaults.

Compare against the accepted debounce firmware: rapid typing/Space, both halves,
all layers and thumb keys, both encoder directions and its button, Vial remaps,
macros/combos/tap dances/overrides, RGB, display and host shortcut behavior. Check
cold boot, reconnect and sleep/wake for regressions relative to that baseline;
passing the debounce test did not itself certify wake recovery.

The user has confirmed the candidate works. The native current Halcyon
representation and any further module behavior changes remain separate work.
