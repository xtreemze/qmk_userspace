# Halcyon fork project guide

This fork is maintained as an integration and experimentation surface for the Halcyon firmware line, with `halcyon` as the integration branch. The goal is to preserve a buildable, reviewable firmware history while allowing focused experiments, hardware-specific improvements and upstream synchronization to proceed in parallel.

## Sources of truth

Use the repository itself as the primary record rather than chat history or local-only notes.

- Integration branch: `halcyon`.
- Upstream Halcyon source: `splitkb/qmk_userspace`, branch `halcyon`.
- Firmware dependency and pin: `.github/workflows/build_binaries.yaml`.
- Canonical Ferris user configuration: `keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil`.
- Compiled Ferris implementation: `keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/`.
- Regression entry point: `tests/run_ci_regressions.sh`.
- Fork patches applied to the pinned firmware tree: `patches/`.

When duplicated documentation disagrees with executable configuration, fix the disagreement and add a regression check when practical.

## Collaboration model

### Pull requests are the code-change surface

Use a narrowly scoped pull request for firmware, test, workflow, configuration and repository-documentation changes. A PR should explain:

1. the behavior or invariant being changed;
2. the evidence that motivated the change;
3. the validation performed;
4. hardware validation still outstanding, if any;
5. compatibility or upstream-sync consequences;
6. documentation, configuration or risk records that also changed.

Prefer small commits and reviewable diffs. Do not hide unrelated cleanup inside a behavioral change.

### Issues are the project-memory surface

Once repository Issues are enabled, use issues for work that benefits from discussion or durable tracking rather than an immediate code diff. Appropriate categories include:

- enhancement proposals and design alternatives;
- research and related-project comparisons;
- hardware and firmware configuration notes;
- audit findings and regression investigations;
- risks, unknowns and acceptance gaps;
- lessons learned and working agreements;
- upstream synchronization planning;
- follow-up work discovered during a PR.

Close issues with a short outcome summary and links to the PR, test evidence or external reference that resolved them. Keep unresolved questions visible rather than burying them in merged PR conversations.

### Parallel AI and human executors

Assume multiple capable executors can be active at the same time.

- Start branches from the current `halcyon` tip and use descriptive topic branches.
- Avoid force-pushing shared branches or rewriting integration history.
- Do not require exclusive worktree or branch ownership for the repository as a whole.
- Re-check the target branch before final merge when concurrent work is likely.
- Keep PR scope narrow enough that parallel work can merge independently.
- If another executor changes the same area, resolve against the newest repository state and preserve both validated intentions where compatible.
- Record material coordination decisions in GitHub rather than leaving them only in a local agent session.

A local Codex or other repository-capable agent is the preferred executor for checks that require a real checkout, QMK environment, binary comparison, flashing or hardware access. Results should be copied back to the relevant PR or issue.

## Validation policy

Match validation depth to risk.

### Source-level baseline

Run:

```sh
bash tests/run_ci_regressions.sh
```

The CI gate should remain deterministic and use the same pinned firmware dependency as builds.

### Firmware-impacting changes

For changes that can affect the Ferris `xtreemze_final` firmware, compile the exact module targets documented in the keymap README. Run focused tests for the touched subsystem in addition to the repository regression suite.

### Binary and hardware evidence

Do not describe source-level tests as physical acceptance.

- Binary-equivalence claims require explicit baseline and candidate binaries.
- USB suspend/resume, split transport, module-side behavior, TFT output, encoder feel and other physical effects require hardware evidence when they are part of the acceptance criteria.
- Record the keyboard half/module arrangement, host OS, firmware commit and relevant configuration for hardware observations.

## Upstream synchronization

Treat upstream sync as an integration task, not a blind merge.

1. Identify the exact upstream commit range.
2. Review upstream changes that touch fork-customized surfaces.
3. Preserve upstream architecture unless a fork-specific behavior has a documented reason to differ.
4. Resolve conflicts explicitly and document the policy used.
5. Run the full regression gate after integration.
6. Re-run focused build or hardware checks for any subsystem touched by both upstream and the fork.
7. Update migration, module or project documentation when the synchronization changes assumptions.

Avoid history rewrites solely to make the fork look linear; traceability is more valuable than cosmetic history.

## Dependency and configuration policy

- Pin external firmware/tooling revisions used for release-producing CI.
- Change pins intentionally through reviewable PRs.
- Keep the canonical Vial export and compiled defaults synchronized.
- Bump the factory seed marker only when an intentional defaults migration requires reseeding on-device dynamic data.
- Preserve user-persistent data unless the migration explicitly requires invalidation.
- Make generated or copied configuration auditable with tests where practical.

## Risk management

Track risks in issues once Issues are enabled. Use explicit states such as `identified`, `investigating`, `mitigated`, `accepted` or `closed` in the issue narrative when useful.

Prioritize risks that can cause:

- a non-functional keyboard or a required power cycle;
- split-half protocol incompatibility;
- persistent EEPROM/configuration loss;
- incorrect Vial dynamic defaults;
- release artifacts built against an unintended dependency;
- regressions that only appear with a particular master half or module combination;
- blocking waits or heavy work in timing-sensitive QMK hooks;
- documentation that can cause a destructive or misleading flashing/configuration action.

## Documentation standards

Documentation should distinguish current behavior, historical context and unverified hypotheses.

- Prefer exact paths, commands, commit IDs and configuration names.
- Date audit snapshots and temporary migration notes.
- Mark hardware acceptance that has not yet been performed.
- Update documentation in the same PR as a behavior change when the user-facing contract changes.
- Add consistency tests when a documented value can be derived cheaply from source.

See `docs/PROJECT_STATUS.md` for the current repository audit snapshot and active project-management blockers.
