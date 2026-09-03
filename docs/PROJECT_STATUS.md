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
- Vial insecure mode is not enabled; the keymap defines an explicit Vial unlock combo, and production keymap rules do not enable console/debug features.
- The repository `.clang-format` is byte-for-byte aligned with current QMK and SplitKB Halcyon formatting policy, but CI does not yet enforce it on changed hand-maintained firmware files; see #29.

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

Status: #5 with implementation in PR #6. The proposed path remains non-blocking, retries/refreshes at a bounded 500 ms cadence, invalidates state on disconnect/master-role loss and includes executable regression coverage. Both regression and exact firmware-build CI are green; physical recovery and degraded-link timing evidence remain in #7. A convention review also requested canonical formatting of the newly touched declaration block before merge; systematic changed-file enforcement is #29.

### Split transaction identifiers are safe now but structurally sensitive to asymmetric features

`MODULE_SYNC` is declared through `SPLIT_TRANSACTION_IDS_KB` and becomes a numeric member of QMK's `serial_transaction_id` enum after many conditionally compiled built-in transactions. The TFT and encoder release targets currently compile the same transaction-affecting feature set: encoder support, split layer/LED/mod state, backlight, RGB Matrix and OS detection; neither release target enables split pointing-device transactions. TFT-specific Quantum Painter flags do not alter the transaction enum.

Risk: a future module-only feature can silently shift `MODULE_SYNC` to a different wire ID on one half while both images still compile independently.

Status: current parity is a negative finding, not a defect. A build-derived transaction-ID parity guard is recorded as a candidate in #13; avoid a brittle hand-maintained macro list if exact preprocessed/build metadata can be compared instead.

### Persistent configuration can diverge between USB-master halves

Pinned Vial-QMK writes VIA/Vial keymap, encoder, dynamic-entry and QMK-setting changes through local NVM APIs on the USB-connected half; the inspected command paths do not mirror those writes to the peer. The fork's custom `xtreemze_user_data_t` is also local EEPROM and includes the factory marker, persisted host family, chord duration and saved RGB profiles. Its host-family persistence explicitly treats the USB master as the sole EEPROM writer, and RGB profile save actions write the local datablock.

Risk: moving USB to the other half can select older/different Vial configuration or custom persistent state even when both halves run the same firmware SHA. This is especially significant because the release is designed to support either half as master.

Status: architecture and physical reproduction tracked in #16. Standard QMK split-config examples such as Charybdis/Spleeb synchronize small master-owned state into slave RAM rather than mirroring peer EEPROM; that pattern is useful for runtime authority but does not by itself solve master-swap persistence. Do not solve this with raw EEPROM cloning; define ownership, schema-aware synchronization/conflict semantics, interruption handling and the exact state regions that should be shared.

### Factory-default marker can certify a failed QMK-settings seed

The macro seeder already refuses to certify a partial factory seed, but `seed_qmk_settings_defaults()` ignores `qmk_settings_set()` return values. Pinned Vial-QMK returns an error when a settings ID is unavailable or a setter rejects its payload. Vial dynamic-entry setters also return status codes and Vial's own HID command handlers surface those statuses, while the compiled-default seeder currently discards them.

Risk: a future Vial-QMK pin can compile successfully while one canonical setting or dynamic entry fails to seed; the firmware can then save the new factory marker and stop retrying, permanently accepting a partial default profile.

Status: failure-atomicity tracked in #21. The factory marker should only advance when every required seed phase reports success.

### Custom EEPROM schema mismatch can cascade into full Vial reseeding

`load_user_data()` resets the entire custom user datablock when `XTREEMZE_USER_DATA_VERSION` differs. Pinned Vial-QMK also truncates user-datablock transfers to `EECONFIG_USER_DATA_SIZE`; current code ignores the returned byte count. The present schema remains safely within the configured 128-byte region and at version `0x02`, so this is a forward migration risk rather than a current corruption event.

The blast radius is larger than the custom block: resetting it also zeroes `defaults_marker`. Later in the same boot, `sync_compiled_defaults_to_dynamic_keymap_once()` interprets that as an old factory revision and calls `dynamic_keymap_reset()` before reseeding compiled defaults. A future user-data version bump can therefore erase the user's Vial dynamic keymap, macros, tap dances, combos, overrides and settings even if the intended schema change concerned only host/RGB metadata.

Status: migration, storage-capacity and marker-preservation policy are consolidated in #9. Before another schema bump, preserve compatible fields and the valid factory marker, add a compile-time capacity assertion, and regress that migration does not trigger Vial factory reseeding unless destructive reset is explicitly intended.

### Deterministic encoder Repeat resolution takes an avoidable NVM hot path

The deterministic encoder resolver currently scans all 32 Vial Alternate Repeat slots and calls `dynamic_keymap_get_alt_repeat_key()` for each candidate on every encoder Repeat/Alternate-Repeat press. At the pinned Vial revision that call reaches `nvm_dynamic_keymap_get_alt_repeat_key()`, which performs an `eeprom_read_block()` per slot.

Pinned Vial's normal Alternate Repeat implementation already maintains a coherent RAM `vial_alt_repeat_key[]` table and reloads it after live configuration changes; its normal resolver iterates that RAM table. The custom encoder policy therefore duplicates matching/orientation logic while taking the slower NVM path.

Risk: rapid encoder detents perform unnecessary EEPROM reads and the duplicated resolver can drift from Vial semantics during dependency updates.

Status: optimization/API design tracked in #28. Prefer exposing/reusing Vial's authoritative RAM pair resolver over adding another cache whose invalidation can go stale after live Vial edits.

### Release target assumes an encoder module revision without recording it

The shared Halcyon code supports both `HLC_ENCODER` and `HLC_ENCODER_REV2`. SplitKB added rev2 support and a Ferris-specific correction in March 2026; rev2 changes module encoder resolution. The current `qmk.json` release matrix and flashing documentation build only `HLC_ENCODER`.

Risk: firmware can build and pass logical encoder tests while using the wrong resolution for a rev2 physical module, or regress an original module if changed without hardware identification.

Status: confirm the physical module revision and align release/build documentation in #15. SplitKB documentation identifies revision 1 by its ALPS EC12 encoder and revision 2 by its Bourns EC11L encoder, providing a concrete physical discriminator rather than relying on observed step behavior alone.

### CI inputs are not yet fully immutable or least-privilege

Vial-QMK and the QMK reusable workflow files are pinned to exact SHAs. PR #14 pins this repository's direct checkout steps to the exact official `actions/checkout` v7.0.1 commit and is green for both regressions and exact firmware builds. However, the pinned QMK reusable build workflow itself invokes mutable `actions/checkout@v6`, `actions/upload-artifact@v7` and `ghcr.io/qmk/qmk_cli:latest`; the publish workflow invokes mutable `actions/checkout@v6`, `actions/download-artifact@v8` and `softprops/action-gh-release@v3`. The reusable workflow also requires a write-capable token ceiling.

A successful audited build pulled `qmk_cli:latest` as digest `sha256:b7d7fa8fb4432b569931de5ad59098cb788f440ed61a62c5126746b71aee0f4a`, demonstrating that the effective toolchain is resolved at runtime rather than fixed by the reusable-workflow SHA. `qmk/.github:main` still points to the same pinned reusable-workflow commit, so there is no newer upstream pin that resolves this today.

Risk: identical repository, Vial and reusable-workflow SHAs can execute different Action/container code over time, and mutable nested dependencies run with more repository authority than ordinary compilation intrinsically needs.

Status: direct checkout pinning is in PR #14; write-token separation is #19; nested Action/container immutability and end-to-end reproducibility are #23.

### Release history and binary provenance are not durable enough

The current `latest` release is mutable, targets the branch name `halcyon`, has an empty release body, and contains only the display/encoder UF2 files. GitHub records asset SHA-256 digests, but the distributed files do not carry a durable manifest identifying the userspace commit, Vial-QMK SHA, local patches, module hardware assumption, build-container/toolchain identity, factory-profile revision or validation status. The publishing workflow replaces `latest`, so prior known-good binaries are not retained as immutable release assets.

A green PR #6 artifact inspection found about 91,904 bytes of actual RP2040 UF2 payload for the TFT image and 73,984 bytes for the encoder image; current flash payload size is therefore not the immediate resource concern. Traceability and rollback are the stronger release risks.

Status: immutable release history and generated provenance manifest are tracked in #25, with CI-environment immutability in #23.

### Flash/RAM headroom is not observable as a trend

The exact display and encoder targets build successfully, but current reusable CI reduces the result to target `[OK]` status and final UF2 artifacts rather than retaining linker-section/resource data. The TFT target alone reserves a 135×240×16-bit Quantum Painter surface framebuffer, or 64,800 bytes of static RAM, before QMK/ChibiOS state, stacks, Vial tables and other globals.

The TFT transfer path itself is already better optimized than a naive framebuffer design: `qp_surface_draw(..., false)` uses Quantum Painter's dirty-region transfer and clears the dirty bounds afterward. Small header/modifier updates therefore do not inherently send the entire surface; full-background animation naturally dirties the full surface at its intentional 200 ms cadence.

Status: per-target flash/RAM/linker headroom and timing observability are tracked in #27. Record measurements before inventing arbitrary limits or replacing Quantum Painter's existing dirty-region mechanism.

### Code formatting policy exists but is not enforceable

The root `.clang-format` matches current QMK and SplitKB Halcyon formatting policy, including consecutive declaration/assignment alignment, four-space indentation and the existing long-line policy. `.editorconfig` also defines whitespace and Makefile conventions. Neither is currently a CI gate.

Risk: concurrent contributors can accumulate convention drift or mix style-only churn into behavioral PRs, which makes input hot paths and upstream synchronization harder to review.

Status: changed-file convention enforcement is tracked in #29. Prefer deterministic formatter checks on fork-maintained changed files while excluding generator-owned matrices/default tables and avoiding repository-wide reformat churn of upstream-synchronized code.

### TFT host telemetry can misstate an unknown shortcut policy as Ctrl

The OS-aware shortcut implementation deliberately sends no shortcut when neither a valid stored host family nor settled live detector evidence exists. Telemetry exposes that state as `HALCYON_SHORTCUT_UNKNOWN` with default source. The TFT renderer nevertheless maps the unknown family to `CTRL`/`CTL` labels.

Risk: diagnostics can claim a concrete Ctrl policy while the actual key action is intentionally disabled, undermining the display as a troubleshooting surface.

Status: semantic correction and regression coverage are tracked in #30. Preserve fail-closed input behavior and render a distinct unknown/waiting state instead of changing behavior to match the old label.

### TFT Alternate Repeat status is blank only in the Vial production build

The TFT reserves header space for `halcyon_display_alt_repeat_text_user()`, the Ferris keymap maintains an Alternate Repeat text buffer, and ordinary key processing calls its updater. However, the resolver/formatter branch is compiled only when `VIAL_ALT_REPEAT_KEY_ENTRIES` is absent. The production Vial build defines 32 entries, so its active branch unconditionally clears the text buffer.

Pinned Vial already provides a RAM-resident `get_alt_repeat_key_keycode_user()` implementation backed by the coherent Vial Alternate Repeat table, so Vial mode can obtain the value without duplicating or scanning NVM.

Status: production display fix tracked in #31. Reuse Vial's authoritative resolver, preserve encoder-only build isolation, and verify live Vial edits are reflected without reboot.

### Integration branch does not enforce the CI gates

The `halcyon` branch reports `protected: false`. The repository rulesets collection is also empty. A direct push or administrative merge can therefore bypass the PR/check workflow even though pushes are release-producing.

Status: repository ruleset/branch-protection action tracked in #17. A duplicate audit issue was closed rather than maintaining two governance threads.

### Fork patch combines independent upstream divergences

`patches/0001-os-detection-fingerprint-trace.patch` contains both OS-fingerprint tracing and the unrelated missing `get_last_record()` implementation used by encoder translation. Current QMK has now implemented that accessor upstream, while current Vial still lacks the implementation and differs in mutability/signature.

Risk: future Vial pin updates are harder to audit and may retain/delete unrelated local divergence as one conflict unit.

Status: dependency compatibility note recorded in #11; split/retirement work tracked in #20 after workflow changes settle.

### Intermittent host resume no-op remains unresolved

The reported failure after host standby/power-source changes can require a USB power cycle. Existing code already defers TFT wake and host-family policy work out of the wake edge, and PR #6 addresses a separate split-state recovery gap, but neither proves the root cause of a complete USB/HID no-op.

Pinned Vial-QMK has an optional `OS_DETECTION_KEYBOARD_RESET` path that performs an MCU soft reset if USB falls back to INIT after previously reaching CONFIGURED and remains there beyond the detector debounce. This fork deliberately removed that behavior in commit `8855474134b244541d9336515c7593d62f249419` to preserve host-family state; the host-family design has since gained persistent storage and explicit resume stabilization.

A post-pin upstream QMK RP2040 endpoint-buffer alignment fix was also reviewed. Pinned Vial lacks it, but the current keyboard/mouse/raw/shared endpoint sizes are already four-byte multiples, so it is weak evidence for this particular resume failure and should not be backported speculatively.

Status: investigation tracked in #8. Use the QMK reset path as an isolated A/B diagnostic candidate, not a default change, and capture USB state/liveness evidence before deciding whether reset-based recovery is appropriate.

### Canonical M10 is explicit but its bootstrap path is not currently reproducible

Canonical Vial macro M10 contains the literal command `curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/master/bootstrap.sh | bash`. The macro contains no Enter/Return action, so profile import, firmware boot and macro invocation do not execute it automatically; submission in a shell remains a separate explicit user action.

The referenced `.dotfiles` repository is currently private, so the unauthenticated raw GitHub URL is not a reliable fresh-machine bootstrap path. Even pinning only the outer raw script would be insufficient for reproducibility: the script subsequently clones/pulls the dotfiles repository's active branch and, on macOS without Homebrew, can fetch Homebrew's `HEAD/install.sh`.

Status: authentication/revision/verification policy is tracked in #26. Preserve the no-Enter safety boundary and solve the actual nested revision model rather than making a superficial outer-URL SHA substitution.

### Hardware acceptance remains distinct from source validation

The Ferris keymap documentation still lists physical TFT/backlight acceptance items, including brightness range, controls, encoders, idle wake and USB sleep/wake with different master/slave arrangements.

Status: partially validated; remaining checks require physical hardware and are tracked in #7.

### Binary equivalence is not part of CI

`tests/xtreemze_legacy_binary_equivalence.py` intentionally remains outside the source-level regression runner because it needs explicit baseline and candidate ELF files.

Status: accepted manual certification step; revisit if deterministic baseline artifacts become available to CI. A reproducible toolchain and immutable release history (#23/#25) would make this substantially easier.

## Live project tracking

- #5 — Halcyon module synchronization recovery; implementation PR #6.
- #7 — physical Ferris TFT/backlight/split/suspend-resume acceptance matrix.
- #8 — intermittent USB resume no-op investigation and reset-path A/B diagnostic.
- #9 — non-destructive custom EEPROM schema/capacity/marker migration policy.
- #10 — SplitKB Halcyon upstream synchronization watch and decision log.
- #11 — pinned Vial-QMK dependency review policy and decision log.
- #12 — related QMK/Vial/SplitKB/RP2040 research log.
- #13 — documentation/configuration and split-transaction consistency audit.
- PR #14 — immutable/supported direct Action pins; follow-up permission risk in #19.
- #15 — physical Halcyon encoder module revision vs release target.
- #16 — master-half persistence consistency for Vial and custom EEPROM state.
- #17 — enforce PR and CI checks on `halcyon` with repository rules.
- #19 — isolate repository write permission to the release/publish path.
- #20 — split local QMK patches by responsibility and retirement condition.
- #21 — make factory-default marker certification failure-atomic across QMK Settings and other checked seed APIs.
- #23 — make nested CI Actions and the QMK CLI build environment immutable/reproducible.
- #25 — retain immutable release history and publish a firmware provenance manifest.
- #26 — make the M10 bootstrap path authenticated/revision-aware while preserving manual submission.
- #27 — track per-target flash/RAM headroom and measured display timing in CI.
- #28 — remove per-detent Alternate Repeat EEPROM scans and reuse authoritative Vial RAM state.
- #29 — enforce upstream-compatible formatting on changed hand-maintained firmware files without mass reformat churn.
- #30 — make TFT unknown shortcut telemetry match the fail-closed host policy.
- #31 — restore Alternate Repeat status text in the production Vial/TFT build.

## Review cadence

Update this snapshot when a material audit changes repository-wide assumptions. Keep detailed investigation, evidence and decisions in the linked Issues rather than duplicating their full history here.
