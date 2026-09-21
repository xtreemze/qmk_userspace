# Contributing to the Halcyon Ferris firmware

This repository is maintained as a production-oriented Vial/QMK userspace with `halcyon` as the integration branch. Contributions should preserve a reviewable firmware history and leave durable project context behind for the next executor.

## Use pull requests for changes

Firmware, tests, workflows, configuration, generated-source tooling, and repository documentation should change through focused pull requests rather than direct updates to `halcyon`.

Keep each PR narrow enough to review independently. State:

1. the behavior or invariant being changed;
2. the evidence or issue motivating it;
3. the validation performed;
4. hardware validation still outstanding, if any;
5. compatibility, persistence, split-protocol, dependency, or upstream consequences;
6. documentation or configuration that changed with the implementation.

Do not hide unrelated cleanup inside a behavioral change. Avoid force-pushing shared work or rewriting integration history when multiple executors may be active.

## Use Issues as project memory

Use GitHub Issues for work that benefits from discussion, research, or durable tracking rather than an immediate code diff. This includes:

- enhancement ideas and design alternatives;
- research and comparisons with QMK, Vial, SplitKB, or related projects;
- configuration notes and intended behavior;
- audit findings and regression investigations;
- risks, unknowns, and acceptance gaps;
- lessons learned and working agreements;
- upstream synchronization and dependency review;
- hardware observations and physical acceptance evidence;
- follow-up work discovered during a PR.

Prefer the existing structured issue forms when they fit. Close an issue with an outcome summary and links to the PR, test evidence, or external reference that resolved it.

## Validation

Run the repository source regression entry point when practical:

```sh
bash tests/run_ci_regressions.sh
```

Firmware changes must also satisfy the strict static-policy gates used in CI:

```sh
qmk lint -kb splitkb/halcyon/ferris/rev1 -km xtreemze_final --strict
python3 scripts/check-firmware-antipatterns.py
```

The repository anti-pattern check intentionally rejects dynamic heap allocation and unbounded legacy string APIs in repository-owned firmware. Do not suppress these checks locally. If a future requirement genuinely needs an exception, change the policy explicitly in a reviewed PR with a concrete bounded-memory or safety rationale.

Firmware-impacting changes should also compile the exact affected production target(s) against the pinned Vial-QMK revision and run focused tests for the touched subsystem.

Do not describe source tests, mocked behavior, a successful compile, or CI success as physical hardware acceptance. USB lifecycle, split reconnect, either-master behavior, TFT/backlight behavior, encoder feel, persistent state across real power cycles, and similar hardware-dependent behavior require separate physical evidence.

## Sources of truth

Before changing behavior, consult:

- `README.md` for the public project overview;
- `docs/PROJECT_GUIDE.md` for the complete collaboration, validation, dependency, risk, and documentation policy;
- `docs/PROJECT_STATUS.md` for the dated current-state snapshot and active priorities;
- `docs/RELATED_PROJECTS.md` for upstream and adjacent projects;
- `keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil` for the canonical Ferris Vial configuration;
- `patches/` for intentional compatibility changes applied to the pinned firmware dependency.

When documentation disagrees with executable configuration, resolve the discrepancy and add a regression check when the value can be derived cheaply.

## Documentation and project-state upkeep

Behavior-changing PRs should update user-facing documentation in the same change. Keep historical observations, hypotheses, and current behavior clearly separated.

When material work lands, update `docs/PROJECT_STATUS.md` so the snapshot does not materially lag the integration branch. Long-lived decisions, risks, audit findings, and research belong in Issues rather than being preserved only in chat history or merged PR discussion.

## Parallel execution

Assume multiple capable human or AI executors can work concurrently:

- start topic branches from the current `halcyon` tip;
- avoid repository-wide branch or worktree ownership assumptions;
- re-check the integration branch before final merge when overlapping work is possible;
- resolve conflicts against the newest validated repository state;
- record material coordination decisions in GitHub;
- use a real local checkout for tests, QMK builds, binary comparisons, flashing, and hardware checks that the GitHub interface cannot perform.

See `docs/PROJECT_GUIDE.md` for the complete working agreement.
