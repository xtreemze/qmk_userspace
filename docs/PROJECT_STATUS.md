# Project status and audit snapshot

Snapshot date: 2026-09-04

This page is a current-state summary. Durable findings, decisions, research, risks and hardware evidence live in GitHub Issues; implementation and documentation changes live in pull requests.

## Integration baseline

- Integration branch: `halcyon`.
- Functional firmware state in this snapshot was audited through `373825f4f4def2b92651dc5bfd02ee27deef2d33`; subsequent CI/release hardening through `3b093a1127547394cc74b09ff0289d7562eaaae1` does not change firmware sources or production targets.
- SplitKB Halcyon upstream was checked on 2026-09-04 and remains `0d2653b3ed58807a63915fa55d071f98d12a8991`; that commit is already in this fork, so no upstream synchronization is pending. See #10.
- Production Vial-QMK is pinned to `dd43959ae5c08d8a28d38a1acf7b04e86b14a344`. See #11.
- The local firmware build uses audited `qmk_cli` image digest `sha256:b7d7fa8fb4432b569931de5ad59098cb788f440ed61a62c5126746b71aee0f4a` and commit-pinned checkout/upload Actions.
- The release job is repository-local, validation-first and non-destructive. Artifact download, GitHub scripting and release creation Actions are commit-pinned; only the publish job receives `contents: write`. The former `qmk/.github` reusable build/publish workflows are no longer production execution dependencies. See completed #19 and #23.
- Default-branch run 126 certified the localized publisher after PR #46: the exact two expected RP2040 UF2s were validated before mutation, `latest` was created/moved to the exact integration SHA, and the release was created successfully.
- The canonical Ferris Vial profile is `keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil`, SHA-256 `281a1e2ff27dc6fff2a34b60fec276280fec2723389b4706895e657db3fd3a3a`, with factory marker `0xB0`.
- `qmk.json` defines two production Ferris targets: TFT display and encoder-module firmware.
- GitHub Issues and Discussions are enabled and are the durable collaboration surface for project memory.
- `halcyon` remains unprotected and the repository currently has no rulesets. Required-PR/check enforcement remains #17.

## What automated validation proves

The release workflow currently provides these source/build-level guarantees:

- the standalone Halcyon regression suite passes;
- all ordered Vial-QMK compatibility patches apply to the pinned firmware dependency and satisfy their responsibility/provenance checks;
- both exact Ferris production targets compile successfully;
- regression and firmware-build jobs run with `contents: read`; only release publishing receives `contents: write`;
- firmware compilation uses the repository-controlled local build job, digest-pinned QMK CLI container and commit-pinned checkout/artifact actions, with no runtime `pip install` dependency resolution;
- changed clearly hand-maintained C/C++ firmware sources are checked non-destructively with QMK-compatible `clang-format` policy;
- the formatter resolves the current GitHub PR merge ref and verifies the immutable PR head, so normal base-branch advancement does not create a false failure from stale pull-request event metadata;
- generated Quantum Painter assets are excluded from formatter ownership;
- the mixed generated/hand-maintained `xtreemze_final/keymap.c` remains intentionally outside whole-file formatting until #37 is resolved;
- the live factory marker and canonical Vial profile hash match documentation;
- regression/build Vial-QMK pins agree with each other and with the dependency-watch documentation;
- every `qmk.json` production target has an exact documented compile command;
- the retained production ELFs are inspected after the existing compile to record flash and RP2040 linker-region RAM headroom without a second build;
- the resource baseline covers every `qmk.json` production target and validates flash, primary `ram0`, and dedicated per-core stack-region arithmetic.

PR #43 proved the digest-contained toolchain can run `qmk userspace-doctor`, formatting/patch preparation, both exact builds and firmware-artifact upload without build-time package installation. Default-branch run 117 then proved that read-only local build artifact could be consumed successfully by the write-capable publisher, satisfying #19.

PR #44 localized and pinned publishing. Its first default-branch execution exposed a destructive delete-before-create failure when unmatched HEX/BIN globs aborted an RP2040 UF2 release. PR #46 replaced that lifecycle with exact-UF2 validation before mutation, non-destructive tag/release updates and UF2-only publishing; default-branch run 126 passed the full path and closed #23.

PR #45 establishes resource observability from the retained production ELFs. Certified run 132 measured the display target at 91,768 B linked flash span, 82,776 B statically occupied `ram0`, and 179,368 B remaining default-heap capacity; the encoder target measured 73,860 B linked flash span, 15,712 B statically occupied `ram0`, and 246,432 B remaining default-heap capacity. Both cores reserve 3,072 B of their dedicated 4 KiB stack region, leaving 1,024 B unreserved per region. These are linker reservations/capacities, not runtime stack high-water measurements.

Automated success is not physical hardware acceptance. Split reconnect, either-master behavior, TFT/backlight behavior, suspend/resume and other electrical/runtime observations remain explicitly tracked in #7. Runtime stack high-water and display/housekeeping timing also require hardware or instrumented runtime evidence. Legacy ELF binary equivalence remains a manual certification because it requires chosen baseline and candidate binaries.

## Recently integrated

- PR #40 split the Vial-QMK compatibility series into independently auditable OS-fingerprint trace, Repeat last-record accessor and Repeat/Key-Override weak-mod patches, each with a narrower retirement condition.
- PR #39 added changed-file, non-mutating formatter enforcement for clearly hand-maintained firmware sources and regression coverage for its ownership boundary.
- PR #41 expanded documentation consistency checks to cover the production Vial-QMK pin and exact `qmk.json` release-target commands.
- PR #43 localized the firmware build, pinned its nested Action/container identities, removed runtime package resolution, restricted compilation to read-only repository authority, and passed the subsequent release-path proof in run 117.
- PR #44 localized and pinned the release publisher.
- PR #46 made moving-`latest` publication validation-first, non-destructive and RP2040-UF2-specific; run 126 restored and certified the release path.

## Open priorities

### Reliability and persistence

- #8: investigate the intermittent USB/HID no-op after host standby and power-source changes.
- #16: define and verify Vial/custom persistent-state behavior when either half becomes USB master.
- #21: make factory-default certification failure-atomic so a failed seed operation cannot advance the factory marker.
- #9: preserve compatible custom EEPROM data across future schema changes instead of destructive reset by default.
- #32: coalesce persistent RGB/backlight writes for bursty encoder adjustments while keeping immediate visual response.

### Input and display correctness

- #28: remove the deterministic encoder Repeat resolver's per-detent scan of Vial Alternate Repeat entries in NVM and reuse one authoritative RAM-resident policy where possible.
- #30: render unknown shortcut policy as an explicit unknown/waiting state rather than incorrectly claiming Ctrl is active.
- #31: restore production Vial Alternate Repeat status on the TFT using Vial's coherent resolver rather than the currently blank branch.
- #35: complete TFT painter-state encapsulation by making the already-unexported `lcd` and `lcd_surface` devices file-local after exact build verification.

### Hardware and release evidence

- #7: maintain the physical acceptance matrix for TFT, backlight, split reconnect, either-master and suspend/resume behavior.
- #15: identify the physical Halcyon encoder module revision and align release-target documentation with that hardware.
- #25: retain immutable accepted firmware releases and publish machine-readable source/dependency/build provenance.
- #27: retain flash/RAM/linker deltas for both production targets and complete any runtime timing/stack measurements needed before setting warning/failure thresholds.

### Build, security and governance

- #17: protect `halcyon` with required PR/CI rules while retaining an explicit emergency recovery path.
- #37: resolve formatting ownership for the large mixed `keymap.c` without creating generated-table style churn.

## Dependency watch

Dated 2026-09-04 checks are recorded in #10, #11 and completed #23:

- SplitKB Halcyon has not advanced beyond the already-integrated upstream baseline.
- Vial's `vial` branch is still exactly the production pin.
- the repository no longer depends on `qmk/.github` reusable workflows for production build or publishing; those responsibilities are locally controlled and pinned.

No dependency or upstream synchronization action is required from this snapshot.

## Working boundary

Use pull requests for code and documentation changes. Use Issues for long-lived decisions, research, risks, hardware observations and follow-up. Keep changes narrow enough that multiple executors can work concurrently without branch ownership assumptions or integration-history rewrites.

Do not describe source tests, mocked behavior or successful compilation as physical acceptance. Where a behavior depends on actual split hardware, USB lifecycle, display electronics or persistent state across real power cycles, record that evidence separately in the relevant issue.
