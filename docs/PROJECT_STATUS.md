# Project status and audit snapshot

Snapshot date: 2026-09-03

This document is a concise audit snapshot, not a permanent backlog. Durable work items live in GitHub Issues and implementation changes live in pull requests.

## Repository state

- Default/integration branch: `halcyon`.
- Current integration lineage includes the resolved SplitKB Halcyon synchronization through upstream commit `0d2653b3ed58807a63915fa55d071f98d12a8991`.
- Upstream check on 2026-09-03 confirms `splitkb/qmk_userspace:halcyon` still points to `0d2653b3ed58807a63915fa55d071f98d12a8991`; there is no newer SplitKB Halcyon commit pending integration at this snapshot.
- Release-producing CI is gated by the repository regression suite and the QMK userspace build.
- The workflow pins the Vial-QMK revision and QMK reusable-workflow revisions used for regression/build/release inputs, but the pinned reusable workflow still resolves mutable nested Actions and `qmk_cli:latest`; see #23.
- The Ferris `xtreemze_final` profile has dedicated tests for Vial defaults, deterministic encoder repeat direction, OS-aware behavior, TFT behavior and backlight policy.
- Legacy ELF binary equivalence remains an explicit manual certification because it requires chosen baseline and candidate binaries.
- GitHub Issues are enabled and are now the durable surface for audit findings, research, risk tracking, hardware acceptance and project decisions.
- The `halcyon` branch is currently unprotected; CI exists but repository settings do not yet enforce PR/check completion before integration.

## Audit findings

### Documentation drift: live factory-profile identity

The live Ferris keymap README had two stale claims:

- firmware and regression sources use factory marker `0xB0`, while the README still documented `0xAF`;
- the README still described the current `xtreemzeVial.vil` as byte-for-byte identical to the 2026-08-26 export and listed that export's SHA-256, even though commit `0f0c6a238776e891064d8e171b4fe5fcd5e42c1d` intentionally revised the profile on 2026-08-30 to align deterministic encoder repeat pairs.

The live README now documents marker `0xB0`, the current canonical profile SHA-256, and the later profile revision while preserving `docs/halcyon-default-profile-2026-08-26.md` as the historical August 26 snapshot. A source-level consistency regression checks the live marker and canonical-profile hash so this class of drift fails CI.

Status: mitigated by PR #4. Broader derivable-value drift is tracked in #13.

### Split module synchronization was one-shot and did not honor RPC failure

The current Halcyon module exchange sets its historical one-shot synchronization state after an attempted `MODULE_SYNC` send and does not recover that state after transport loss. The pinned Vial-QMK API reports RPC success/failure, and existing QMK/Vial synchronization patterns advance state only on success and commonly refresh periodically.

Risk: stale `module_master` state after a failed first transaction, transport reconnect or slave-side reset, affecting second-display/module-role behavior.

Status: #5 with implementation in PR #6. The proposed path remains non-blocking, retries/refreshes at a bounded 500 ms cadence, invalidates state on disconnect/master-role loss and includes executable regression coverage. Both regression and exact firmware-build CI are green; physical recovery and degraded-link timing evidence remain in #7.

### Persistent configuration can diverge between USB-master halves

Pinned Vial-QMK writes VIA/Vial keymap, encoder, dynamic-entry and QMK-setting changes through local NVM APIs on the USB-connected half; the inspected command paths do not mirror those writes to the peer. The fork's custom `xtreemze_user_data_t` is also local EEPROM and includes the factory marker, persisted host family, chord duration and saved RGB profiles. Its host-family persistence explicitly treats the USB master as the sole EEPROM writer, and RGB profile save actions write the local datablock.

Risk: moving USB to the other half can select older/different Vial configuration or custom persistent state even when both halves run the same firmware SHA. This is especially significant because the release is designed to support either half as master.

Status: architecture and physical reproduction tracked in #16. Do not solve this with raw EEPROM cloning; define ownership, schema-aware synchronization/conflict semantics, interruption handling and the exact state regions that should be shared.

### Factory-default marker can certify a failed QMK-settings seed

The macro seeder already refuses to certify a partial factory seed, but `seed_qmk_settings_defaults()` ignores `qmk_settings_set()` return values. Pinned Vial-QMK returns an error when a settings ID is unavailable or a setter rejects its payload.

Risk: a future Vial-QMK pin can compile successfully while one canonical setting fails to seed; the firmware can then save the new factory marker and stop retrying, permanently accepting a partial default profile.

Status: failure-atomicity tracked in #21. The factory marker should only advance when every required seed phase reports success.

### Custom EEPROM schema mismatch can cascade into full Vial reseeding

`load_user_data()` resets the entire custom user datablock when `XTREEMZE_USER_DATA_VERSION` differs. Pinned Vial-QMK also truncates user-datablock transfers to `EECONFIG_USER_DATA_SIZE`; current code ignores the returned byte count. The present schema remains safely within the configured 128-byte region and at version `0x02`, so this is a forward migration risk rather than a current corruption event.

The blast radius is larger than the custom block: resetting it also zeroes `defaults_marker`. Later in the same boot, `sync_compiled_defaults_to_dynamic_keymap_once()` interprets that as an old factory revision and calls `dynamic_keymap_reset()` before reseeding compiled defaults. A future user-data version bump can therefore erase the user's Vial dynamic keymap, macros, tap dances, combos, overrides and settings even if the intended schema change concerned only host/RGB metadata.

Status: migration, storage-capacity and marker-preservation policy are consolidated in #9. Before another schema bump, preserve compatible fields and the valid factory marker, add a compile-time capacity assertion, and regress that migration does not trigger Vial factory reseeding unless destructive reset is explicitly intended.

### Release target assumes an encoder module revision without recording it

The shared Halcyon code supports both `HLC_ENCODER` and `HLC_ENCODER_REV2`. SplitKB added rev2 support and a Ferris-specific correction in March 2026; rev2 changes module encoder resolution. The current `qmk.json` release matrix and flashing documentation build only `HLC_ENCODER`.

Risk: firmware can build and pass logical encoder tests while using the wrong resolution for a rev2 physical module, or regress an original module if changed without hardware identification.

Status: confirm the physical module revision and align release/build documentation in #15.

### CI inputs are not yet fully immutable or least-privilege

Vial-QMK and the QMK reusable workflow files are pinned to exact SHAs. PR #14 pins this repository's direct checkout steps to the exact official `actions/checkout` v7.0.1 commit. However, the pinned QMK reusable build workflow itself invokes mutable `actions/checkout@v6`, `actions/upload-artifact@v7` and `ghcr.io/qmk/qmk_cli:latest`; the publish workflow invokes mutable `actions/checkout@v6`, `actions/download-artifact@v8` and `softprops/action-gh-release@v3`. The reusable workflow also requires a write-capable token ceiling.

Risk: identical repository, Vial and reusable-workflow SHAs can execute different Action/container code over time, and mutable nested dependencies run with more repository authority than ordinary compilation intrinsically needs.

Status: direct checkout pinning is in PR #14; write-token separation is #19; nested Action/container immutability and end-to-end reproducibility are #23.

### Integration branch does not enforce the CI gates

The `halcyon` branch reports `protected: false`. A direct push or administrative merge can therefore bypass the PR/check workflow even though pushes are release-producing.

Status: repository ruleset/branch-protection action tracked in #17. A duplicate audit issue was closed rather than maintaining two governance threads.

### Fork patch combines independent upstream divergences

`patches/0001-os-detection-fingerprint-trace.patch` contains both OS-fingerprint tracing and the unrelated missing `get_last_record()` implementation used by encoder translation. Current QMK has now implemented that accessor upstream, while current Vial still lacks the implementation and differs in mutability/signature.

Risk: future Vial pin updates are harder to audit and may retain/delete unrelated local divergence as one conflict unit.

Status: dependency compatibility note recorded in #11; split/retirement work tracked in #20 after workflow changes settle.

### Hardware acceptance remains distinct from source validation

The Ferris keymap documentation still lists physical TFT/backlight acceptance items, including brightness range, controls, encoders, idle wake and USB sleep/wake with different master/slave arrangements.

Status: partially validated; remaining checks require physical hardware and are tracked in #7.

### Intermittent host resume no-op remains unresolved

The reported failure after host standby/power-source changes can require a USB power cycle. Existing code already defers TFT wake and host-family policy work out of the wake edge, and PR #6 addresses a separate split-state recovery gap, but neither proves the root cause of a complete USB/HID no-op.

Pinned Vial-QMK has an optional `OS_DETECTION_KEYBOARD_RESET` path that performs an MCU soft reset if USB falls back to INIT after previously reaching CONFIGURED and remains there beyond the detector debounce. This fork deliberately removed that behavior in commit `8855474134b244541d9336515c7593d62f249419` to preserve host-family state; the host-family design has since gained persistent storage and explicit resume stabilization.

Status: investigation tracked in #8. Use the QMK reset path as an isolated A/B diagnostic candidate, not a default change, and capture USB state/liveness evidence before deciding whether reset-based recovery is appropriate.

### Binary equivalence is not part of CI

`tests/xtreemze_legacy_binary_equivalence.py` intentionally remains outside the source-level regression runner because it needs explicit baseline and candidate ELF files.

Status: accepted manual certification step; revisit if deterministic baseline artifacts become available to CI.

## Live project tracking

- #5 — Halcyon module synchronization recovery; implementation PR #6.
- #7 — physical Ferris TFT/backlight/split/suspend-resume acceptance matrix.
- #8 — intermittent USB resume no-op investigation and reset-path A/B diagnostic.
- #9 — non-destructive custom EEPROM schema/capacity/marker migration policy.
- #10 — SplitKB Halcyon upstream synchronization watch and decision log.
- #11 — pinned Vial-QMK dependency review policy and decision log.
- #12 — related QMK/Vial/SplitKB/RP2040 research log.
- #13 — documentation/configuration consistency audit.
- PR #14 — immutable/supported direct Action pins; follow-up permission risk in #19.
- #15 — physical Halcyon encoder module revision vs release target.
- #16 — master-half persistence consistency for Vial and custom EEPROM state.
- #17 — enforce PR and CI checks on `halcyon` with repository rules.
- #19 — isolate repository write permission to the release/publish path.
- #20 — split local QMK patches by responsibility and retirement condition.
- #21 — make factory-default marker certification failure-atomic across QMK Settings and other checked seed APIs.
- #23 — make nested CI Actions and the QMK CLI build environment immutable/reproducible.

## Review cadence

Update this snapshot when a material audit changes repository-wide assumptions. Keep detailed investigation, evidence and decisions in the linked Issues rather than duplicating their full history here.
