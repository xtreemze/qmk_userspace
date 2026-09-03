# Project status and audit snapshot

Snapshot date: 2026-09-03

This document is a concise audit snapshot, not a permanent backlog. Durable work items should move to GitHub Issues once Issues are enabled for this fork.

## Repository state

- Default/integration branch: `halcyon`.
- Current integration lineage includes the resolved SplitKB Halcyon synchronization through upstream commit `0d2653b3ed58807a63915fa55d071f98d12a8991`.
- Release-producing CI is gated by the repository regression suite and the QMK userspace build.
- The workflow pins the Vial-QMK revision used for both regression and firmware build inputs.
- The Ferris `xtreemze_final` profile has dedicated tests for Vial defaults, deterministic encoder repeat direction, OS-aware behavior, TFT behavior and backlight policy.
- Legacy ELF binary equivalence remains an explicit manual certification because it requires chosen baseline and candidate binaries.

## Audit findings

### Documentation drift: live factory-profile identity

The live Ferris keymap README had two stale claims:

- firmware and regression sources use factory marker `0xB0`, while the README still documented `0xAF`;
- the README still described the current `xtreemzeVial.vil` as byte-for-byte identical to the 2026-08-26 export and listed that export's SHA-256, even though commit `0f0c6a238776e891064d8e171b4fe5fcd5e42c1d` intentionally revised the profile on 2026-08-30 to align deterministic encoder repeat pairs.

The live README now documents marker `0xB0`, the current canonical profile SHA-256, and the later profile revision while preserving `docs/halcyon-default-profile-2026-08-26.md` as the historical August 26 snapshot. A source-level consistency regression checks the live marker and canonical-profile hash so this class of drift fails CI.

Status: mitigated by the accompanying PR.

### Project-management blocker: Issues disabled

Repository metadata currently has GitHub Issues disabled. This prevents the requested issue-first workflow for proposals, research, audit findings, risks, lessons and follow-up tracking.

Status: open repository-setting action.

Required action: enable **Issues** in repository settings. The repository includes issue templates so structured tracking can begin immediately afterward.

### Hardware acceptance remains distinct from source validation

The Ferris keymap documentation still lists physical TFT/backlight acceptance items, including brightness range, controls, encoders, idle wake and USB sleep/wake with different master/slave arrangements.

Status: partially validated; remaining checks require physical hardware.

### Binary equivalence is not part of CI

`tests/xtreemze_legacy_binary_equivalence.py` intentionally remains outside the source-level regression runner because it needs explicit baseline and candidate ELF files.

Status: accepted manual certification step; revisit if deterministic baseline artifacts become available to CI.

## Issue backlog to create after Issues are enabled

1. **Hardware acceptance matrix: Ferris TFT/backlight and suspend/resume** — turn the remaining physical checks into a reproducible host/module matrix with commit and firmware-artifact evidence.
2. **Upstream synchronization watch** — record the current SplitKB Halcyon head, fork divergence, conflict-prone surfaces and sync decisions.
3. **Pinned Vial-QMK review policy** — define when and how the pinned firmware dependency advances, including regression and exact-target build evidence.
4. **USB resume/no-op investigation** — capture reproduction conditions, host power-state transitions, split transport state and recovery behavior before proposing firmware changes.
5. **Documentation/configuration consistency audit** — identify additional values that can be derived automatically from `qmk.json`, the workflow, Vial export or compiled keymap and add cheap drift checks where useful.
6. **Related projects and research index** — maintain references to relevant QMK, Vial, SplitKB Halcyon, ChibiOS and hardware-module work, with notes on concrete applicability to this fork.

## Review cadence

Update this snapshot when a material audit changes repository-wide assumptions. Do not use it as a substitute for closing or updating individual issues; once Issues are enabled, this page should summarize rather than duplicate the live backlog.
