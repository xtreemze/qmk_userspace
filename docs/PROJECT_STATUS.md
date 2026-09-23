# Project status and audit snapshot

Snapshot date: 2026-09-23

This page is a current-state summary. Durable findings, decisions, research, risks and hardware evidence live in GitHub Issues; implementation and documentation changes live in pull requests.

## Integration baseline

- Integration branch: `halcyon`.
- Current integration tip at this snapshot: `6aa39e769128177bd108859717104e87eb1a4285` (`Pin every GitHub Action to an immutable commit (#89)`).
- SplitKB Halcyon upstream was last checked on 2026-09-04 and remained `0d2653b3ed58807a63915fa55d071f98d12a8991`; that commit was already integrated. Continue the dated decision log in #10 before the next synchronization.
- Production Vial-QMK is pinned to `dd43959ae5c08d8a28d38a1acf7b04e86b14a344`. Candidate updates and decisions remain tracked in #11.
- The local firmware build uses audited `qmk_cli` image digest `sha256:b7d7fa8fb4432b569931de5ad59098cb788f440ed61a62c5126746b71aee0f4a`; every external GitHub Action in repository workflows is commit-SHA pinned and the regression suite rejects mutable Action refs.
- The release job is repository-local, validation-first and non-destructive. Each green `halcyon` commit publishes a SHA-specific immutable `ci-<commit>` prerelease containing both UF2s and the provenance manifest; reruns verify already-published candidates without mutating them. The moving `latest` release remains a separate convenience pointer. Both surfaces explicitly state that CI success is not hardware acceptance.
- The canonical Ferris Vial profile is `keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil`, SHA-256 `281a1e2ff27dc6fff2a34b60fec276280fec2723389b4706895e657db3fd3a3a`, with factory marker `0xB0`.
- `qmk.json` defines two production Ferris targets: TFT display and encoder-module firmware.
- GitHub Issues are the durable project-memory surface; PRs are the change and review surface. `docs/PROJECT_GUIDE.md` defines the working model.
- `halcyon` remains unprotected. Required-PR/check enforcement remains #17.

## What automated validation proves

The release workflow currently provides these source/build-level guarantees:

- the standalone Halcyon regression suite passes;
- all ordered Vial-QMK compatibility patches apply to the pinned firmware dependency and satisfy their responsibility/provenance checks;
- both exact Ferris production targets compile successfully;
- regression and firmware-build jobs run with `contents: read`; only release publishing receives `contents: write`;
- firmware compilation uses the repository-controlled local build job, digest-pinned QMK CLI container and commit-pinned checkout/artifact actions, with no runtime dependency resolution;
- changed clearly hand-maintained C/C++ firmware sources are checked non-destructively with QMK-compatible `clang-format` policy;
- generated Quantum Painter assets are excluded from formatter ownership;
- the mixed generated/hand-maintained `xtreemze_final/keymap.c` remains intentionally outside whole-file formatting until #37 is resolved;
- the live factory marker and canonical Vial profile hash match documentation;
- regression/build Vial-QMK pins agree with each other and with dependency-watch documentation;
- every `qmk.json` production target has an exact documented compile command;
- retained production ELFs provide flash and RP2040 linker-region RAM headroom measurements for both production targets.

Automated success is not physical hardware acceptance. The moving `latest` artifact is a CI candidate only. Split reconnect, either-master behavior, TFT/backlight behavior, suspend/resume and other electrical/runtime observations remain tracked separately in #7; immutable hardware-accepted release history remains #25.

## Recently integrated

- PR #39 added changed-file, non-mutating formatter enforcement for clearly hand-maintained firmware sources and regression coverage for its ownership boundary.
- PR #40 split the Vial-QMK compatibility series into independently auditable patches with narrower retirement conditions.
- PR #41 expanded documentation consistency checks for the production Vial-QMK pin and exact release-target commands.
- PR #43 localized the firmware build, pinned nested Action/container identities and restricted compilation to read-only repository authority.
- PR #44 localized and pinned the release publisher.
- PR #46 made moving-`latest` publication validation-first, non-destructive and RP2040-UF2-specific.
- PR #50 added repository-local hardware-acceptance evidence snapshot tooling and documented its use. The helper captures provenance and physical-test context; it does not convert unperformed hardware checks into passes.
- PR #80 made unknown host shortcut telemetry explicitly render as waiting instead of falsely reporting Ctrl.
- PR #81 completed TFT painter-device internal-linkage cleanup after the stale public declarations were removed.
- PR #82 made automated releases explicitly non-hardware-accepted CI candidates.
- PR #83 replaced encoder Repeat endpoint translation/NVM scans with Vial's live RAM orientation and native Repeat dispatch.
- PR #84 made QMK Settings factory seeding failure-atomic before the factory marker can advance.
- PR #85 restored live Vial Alternate Repeat status on the TFT.
- PR #86 made unsupported custom user-data schemas non-destructive and prevented schema mismatch from triggering Vial factory reseeding.
- PR #87 corrected M10 hardening documentation so mutable branch-tip behavior is no longer described as resolved.
- PR #88 added immutable, digest-verified CI firmware candidate history while preserving the separate hardware-acceptance gate.
- PR #89 commit-pinned every external GitHub Action and added repository-wide enforcement.

## Active implementation

- #25 remains open for the immutable **hardware-accepted** publication/sign-off path. Immutable CI candidate history is now operational but intentionally does not substitute for physical acceptance.
- #26 remains open because shipped M10 still targets a mutable branch; documentation now requires an immutable commit/digest plus a non-destructive Vial migration.

## Open priorities

### Reliability and persistence

- #8: investigate the intermittent USB/HID no-op after host standby and power-source changes.
- #16: define and verify Vial/custom persistent-state behavior when either half becomes USB master.
- #47: decouple Vial EEPROM compatibility from per-build random `BUILD_ID` without destroying existing configured state during migration.
- #32: coalesce persistent RGB/backlight writes for bursty encoder adjustments while keeping immediate visual response.

### Input and display correctness

- Native-only deterministic encoder Repeat and TFT Alternate Repeat status are integrated; keep regression coverage aligned with live Vial pair edits and either module build.

### Hardware and release evidence

- #7: maintain the physical acceptance matrix for TFT, backlight, split reconnect, either-master and suspend/resume behavior.
- #15: identify the physical Halcyon encoder module revision and align release-target documentation with that hardware.
- #25: complete the immutable hardware-accepted release/sign-off path. SHA-specific immutable CI candidate releases and the machine-readable build manifest are already operational.
- #27: retain flash/RAM/linker deltas for both production targets and complete runtime timing/stack measurements before setting warning/failure thresholds.

### Build, security and governance

- #17: protect `halcyon` with required PR/CI rules while retaining an explicit emergency recovery path.
- #26: decide whether the canonical shell bootstrap macro should remain branch-tip mutable or be pinned/verified.
- #37: resolve formatting ownership for the large mixed `keymap.c` without creating generated-table style churn.

## Durable research and coordination records

- #10: SplitKB Halcyon upstream synchronization log.
- #11: Vial-QMK dependency review and pin decisions.
- #12: related QMK, Vial, SplitKB and RP2040 research log.
- #7: hardware acceptance evidence.
- `docs/RELATED_PROJECTS.md`: presentable repository-level summary of related implementation sources and patterns.
- `docs/PROJECT_GUIDE.md`: collaboration model, validation policy, risk management and documentation standards.

## Working boundary

Use pull requests for code, tests, workflows, configuration and documentation changes. Use Issues for long-lived decisions, research, risks, hardware observations, audits and follow-up. Keep PRs narrow enough that multiple capable executors can work concurrently without repository-wide branch ownership assumptions.

When a PR discovers a material unresolved risk or design question, leave it behind as an Issue rather than burying it in a merged conversation. When an Issue is resolved, close it with the implementing PR or evidence that changed the decision.

Do not describe source tests, mocked behavior or successful compilation as physical acceptance. Where behavior depends on actual split hardware, USB lifecycle, display electronics or persistent state across real power cycles, record that evidence separately in the relevant issue.
