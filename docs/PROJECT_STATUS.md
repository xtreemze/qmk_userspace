# Project status and audit snapshot

Snapshot date: 2026-09-03

This document is a concise audit snapshot, not a permanent backlog. Durable work items live in GitHub Issues and implementation changes live in pull requests.

## Repository state

- Default/integration branch: `halcyon`.
- Current integration lineage includes the resolved SplitKB Halcyon synchronization through upstream commit `0d2653b3ed58807a63915fa55d071f98d12a8991`.
- Upstream check on 2026-09-03 confirms `splitkb/qmk_userspace:halcyon` still points to `0d2653b3ed58807a63915fa55d071f98d12a8991`; there is no newer SplitKB Halcyon commit pending integration at this snapshot.
- Release-producing CI is gated by the repository regression suite and the QMK userspace build.
- The workflow pins the Vial-QMK revision used for both regression and firmware build inputs.
- The Ferris `xtreemze_final` profile has dedicated tests for Vial defaults, deterministic encoder repeat direction, OS-aware behavior, TFT behavior and backlight policy.
- Legacy ELF binary equivalence remains an explicit manual certification because it requires chosen baseline and candidate binaries.
- GitHub Issues are enabled and are now the durable surface for audit findings, research, risk tracking, hardware acceptance and project decisions.

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

Status: #5 with implementation in PR #6. The proposed path remains non-blocking, retries/refreshes at a bounded 500 ms cadence, invalidates state on disconnect/master-role loss and includes executable regression coverage.

### Custom EEPROM schema mismatch is destructive by default

`load_user_data()` resets the entire custom user datablock when `XTREEMZE_USER_DATA_VERSION` differs. The current schema has remained at version `0x02` through recent changes, so this is a future migration risk rather than a current data-loss event.

Risk: a future version bump could silently discard compatible saved host-family, timing and RGB-profile fields.

Status: policy/migration work tracked in #9. A future schema bump should preserve compatible fields or explicitly document intentional destruction.

### Hardware acceptance remains distinct from source validation

The Ferris keymap documentation still lists physical TFT/backlight acceptance items, including brightness range, controls, encoders, idle wake and USB sleep/wake with different master/slave arrangements.

Status: partially validated; remaining checks require physical hardware and are tracked in #7.

### Intermittent host resume no-op remains unresolved

The reported failure after host standby/power-source changes can require a USB power cycle. Existing code already defers TFT wake and host-family policy work out of the wake edge, and PR #6 addresses a separate split-state recovery gap, but neither proves the root cause of a complete USB/HID no-op.

Status: investigation tracked in #8. Evidence should distinguish USB enumeration/endpoint state, firmware liveness, split transport, display/backlight state and master-half dependence.

### Binary equivalence is not part of CI

`tests/xtreemze_legacy_binary_equivalence.py` intentionally remains outside the source-level regression runner because it needs explicit baseline and candidate ELF files.

Status: accepted manual certification step; revisit if deterministic baseline artifacts become available to CI.

## Live project tracking

- #5 — Halcyon module synchronization recovery; implementation PR #6.
- #7 — physical Ferris TFT/backlight/split/suspend-resume acceptance matrix.
- #8 — intermittent USB resume no-op investigation.
- #9 — non-destructive custom EEPROM schema migration policy.
- #10 — SplitKB Halcyon upstream synchronization watch and decision log.
- #11 — pinned Vial-QMK dependency review policy and decision log.
- #12 — related QMK/Vial/SplitKB/RP2040 research log.
- #13 — documentation/configuration consistency audit.

## Review cadence

Update this snapshot when a material audit changes repository-wide assumptions. Keep detailed investigation, evidence and decisions in the linked Issues rather than duplicating their full history here.
