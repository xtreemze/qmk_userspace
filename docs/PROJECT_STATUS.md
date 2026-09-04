# Project status and audit snapshot

Snapshot date: 2026-09-04

This page is a current-state summary. Durable findings, decisions, research, risks and hardware evidence live in GitHub Issues; implementation and documentation changes live in pull requests.

## Integration baseline

- Integration branch: `halcyon`.
- Functional firmware state in this snapshot was audited through `373825f4f4def2b92651dc5bfd02ee27deef2d33`; the CI build architecture was subsequently audited by PR #43 without changing firmware sources or production targets.
- SplitKB Halcyon upstream was checked on 2026-09-04 and remains `0d2653b3ed58807a63915fa55d071f98d12a8991`; that commit is already in this fork, so no upstream synchronization is pending. See #10.
- Production Vial-QMK is pinned to `dd43959ae5c08d8a28d38a1acf7b04e86b14a344`. See #11.
- The local firmware build uses audited `qmk_cli` image digest `sha256:b7d7fa8fb4432b569931de5ad59098cb788f440ed61a62c5126746b71aee0f4a`; the remaining QMK reusable publish workflow is pinned to `01daf5113fa50804558f21cc074ab99ba84ddeaf`. Publish-side nested mutable dependencies remain #23.
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
- generated Quantum Painter assets are excluded from formatter ownership;
- the mixed generated/hand-maintained `xtreemze_final/keymap.c` remains intentionally outside whole-file formatting until #37 is resolved;
- the live factory marker and canonical Vial profile hash match documentation;
- regression/build Vial-QMK pins agree with each other and with the dependency-watch documentation;
- every `qmk.json` production target has an exact documented compile command.

PR #43 proved the digest-contained toolchain can run `qmk userspace-doctor`, formatting/patch preparation, both exact builds and firmware-artifact upload without build-time package installation. A default-branch push run is still the required release-path proof after that PR lands.

Automated success is not physical hardware acceptance. Split reconnect, either-master behavior, TFT/backlight behavior, suspend/resume and other electrical/runtime observations remain explicitly tracked in #7. Legacy ELF binary equivalence also remains a manual certification because it requires chosen baseline and candidate binaries.

## Recently integrated

- PR #40 split the Vial-QMK compatibility series into independently auditable OS-fingerprint trace, Repeat last-record accessor and Repeat/Key-Override weak-mod patches, each with a narrower retirement condition.
- PR #39 added changed-file, non-mutating formatter enforcement for clearly hand-maintained firmware sources and regression coverage for its ownership boundary.
- PR #41 expanded documentation consistency checks to cover the production Vial-QMK pin and exact `qmk.json` release-target commands.
- PR #43 localizes the firmware build, pins its nested Action/container identities and restricts compilation to read-only repository authority; the externally reusable publish path remains a separate #23 concern.

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
- #27: record flash/RAM/linker headroom for both production targets so resource growth is observable rather than inferred from build success.

### Build, security and governance

- #17: protect `halcyon` with required PR/CI rules while retaining an explicit emergency recovery path.
- #23: localize/pin the remaining publish-side nested Actions and make the full release path traceable to controlled immutable identities.
- #37: resolve formatting ownership for the large mixed `keymap.c` without creating generated-table style churn.

## Dependency watch

Dated 2026-09-04 checks are recorded in #10, #11 and #23:

- SplitKB Halcyon has not advanced beyond the already-integrated upstream baseline.
- Vial's `vial` branch is still exactly the production pin.
- `qmk/.github:main` is still exactly the reusable-workflow revision used by the remaining publish path.

No dependency or upstream synchronization action is required from this snapshot.

## Working boundary

Use pull requests for code and documentation changes. Use Issues for long-lived decisions, research, risks, hardware observations and follow-up. Keep changes narrow enough that multiple executors can work concurrently without branch ownership assumptions or integration-history rewrites.

Do not describe source tests, mocked behavior or successful compilation as physical acceptance. Where a behavior depends on actual split hardware, USB lifecycle, display electronics or persistent state across real power cycles, record that evidence separately in the relevant issue.
