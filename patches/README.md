# Required Vial-QMK patches

The production firmware uses the pinned Vial-QMK revision
`dd43959ae5c08d8a28d38a1acf7b04e86b14a344`. Fork-required engine changes live as
small numbered patches here and are applied in lexical order by
`scripts/apply-qmk-patches.sh` in both regression and release builds.

## 0001 — OS detection fingerprint trace

`0001-os-detection-fingerprint-trace.patch` preserves the existing display's
passive OS-fingerprint viewer. It is not a new detector, USB wake experiment or
Repeat-engine patch. The patch was captured in userspace commit
`cfd78d79c0dc191397cabda0402bf314f54e6f24` and was already required by the display
code inherited by the accepted debounce baseline, `9b615b9`.

It applies to the pinned Vial-QMK revision and includes the existing detector
tests. Local setup is documented in
[the migration report](../docs/halcyon-legacy-migration.md).

Retire this patch only after the replacement Vial/QMK dependency provides the
same trace contract required by the TFT diagnostic path, or after that diagnostic
contract is deliberately removed.

## 0003 — Repeat Key + Key Override weak modifiers

`0003-repeat-key-override-weak-mods.patch` backports upstream QMK commit
`d7ad3bf8aa05ead807984845480542affb3a054e` (2026-08-12). During an active Repeat
invocation, QMK restores remembered modifiers as weak modifiers. The pinned Vial
Key Override engine otherwise ignores those weak modifiers unless
`KEY_OVERRIDE_INCLUDE_WEAK_MODS` is enabled globally, so modifier-dependent
Key Overrides can fail to reproduce through Repeat.

The backport keeps upstream's narrow behavior: weak modifiers participate in
Key Override matching only while Repeat is active. It does not globally change
ordinary Key Override semantics.

Retire this patch as soon as the pinned Vial-QMK revision contains upstream QMK
commit `d7ad3bf8aa05ead807984845480542affb3a054e` or an equivalent fix. Pin review
must explicitly check this condition.

## 0005 — Vial Alternate Repeat RAM resolver

`0005-vial-alt-repeat-ram-resolver.patch` exposes the direct-match winner from
Vial's existing coherent RAM-resident Alternate Repeat table. The public result
contains the winning slot, options and whether the remembered key matched the
primary or alternate side. Vial's stock `get_alt_repeat_key_keycode_user()`
reuses the same direct-match helper, so the encoder policy does not duplicate
normalization, modifier matching, best-fit or slot-order precedence.

The resolver deliberately excludes default-alt fallback. Encoder deterministic
orientation only canonicalizes a winning direct bidirectional pair; one-way,
default-alt and unmatched cases continue through stock Repeat/Alternate Repeat.

This local compatibility extension resolves #28 and enables #63 without
per-detent EEPROM reads or `last_record` mutation. Retire it when the pinned
Vial-QMK revision exposes an equivalent coherent direct-match API, or when the
encoder orientation policy no longer needs pair-side metadata.

## Patch policy

- Keep one behavioral responsibility per numbered patch.
- Record the exact upstream source/retirement condition when a patch is a backport.
- Apply the same ordered patch series to regression and release-producing builds.
- Require `git apply --check` before each application.
- Add a focused regression for every patch contract that can be tested cheaply.
- Delete a local patch when the pinned dependency supersedes it; do not keep inert
  historical patches in the active series.

Do not import the separate USB-event-queue or split-transport instrumentation
patches from `diagnostics/usb-resume` into production without a dedicated review.
