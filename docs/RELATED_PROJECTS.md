# Related projects and dependency watch

Snapshot date: 2026-09-03

This page records projects that materially influence the Halcyon fork. It is an orientation and watch list, not a substitute for reviewing exact commits before changing a pin or synchronizing upstream.

## `splitkb/qmk_userspace`

Role: primary Halcyon userspace upstream.

- Default branch: `halcyon`.
- This fork should treat SplitKB's current Halcyon architecture, module ownership and hardware support as the upstream baseline unless a fork-specific behavior has an explicit reason to differ.
- Synchronization work should review changes touching `users/halcyon_modules/`, supported Halcyon keyboard definitions, module buttons, display/backlight behavior, split transactions and module option parsing.
- Record the exact upstream range in each sync PR; do not use a floating "latest" description as the durable record.

Watch when: SplitKB pushes new Halcyon commits, adds/revises modules, changes split transport/module hooks, or changes supported board revisions.

## `vial-kb/vial-qmk`

Role: firmware dependency used by the Vial-enabled `halcyon` branch.

- Default branch: `vial`.
- Release-producing CI in this repository intentionally uses a pinned Vial-QMK revision rather than tracking the branch tip.
- High-risk surfaces for this fork include Vial dynamic-keymap storage/layout, repeat and alternate-repeat behavior, QMK settings, EEPROM formats, split transactions, OS detection, backlight/PWM processing and QMK hook ownership.
- A pin update should be a dedicated reviewed change with the full repository regression gate and exact Ferris module builds. Any fork patch must be re-applied with `git apply --check` before compilation.

Watch when: Vial incorporates newer QMK behavior relevant to this fork, changes dynamic configuration formats, updates repeat/tap-hold processing, or advances ChibiOS/platform support needed by Halcyon hardware.

## `qmk/qmk_firmware`

Role: upstream keyboard-firmware architecture from which Vial derives and the reference for current QMK semantics.

- Default branch: `master`.
- Use QMK upstream to understand intended hook composition, feature behavior and platform changes even when the production build remains pinned to Vial-QMK.
- Compare relevant QMK fixes with the pinned Vial tree before carrying local patches; prefer upstream semantics when they are compatible with Vial and the Halcyon integration.
- Changes in quantum processing, split keyboard transport, ChibiOS/RP2040 support, encoders, repeat keys, suspend/wakeup, USB and EEPROM are especially relevant.

Watch when: a bug being investigated may already be fixed upstream, Vial's inherited implementation differs materially, or a local workaround starts duplicating an upstream solution.

## `qmk/qmk_userspace`

Role: source project for the userspace repository model.

The SplitKB repository and this fork ultimately derive from QMK userspace. Track it mainly for reusable workflow, repository-layout and userspace tooling changes rather than Halcyon-specific behavior.

Watch when: QMK changes userspace build/publish workflows, `qmk userspace-*` commands, metadata requirements or recommended repository structure.

## Research and adoption rule

Do not copy a related project's implementation solely because it is newer. For each candidate change:

1. identify the exact problem or opportunity in this fork;
2. link the relevant external commit, issue, documentation or implementation;
3. state which local invariant it affects;
4. compare it with the pinned Vial-QMK and current SplitKB Halcyon state;
5. isolate the change in a PR with targeted regressions;
6. record unresolved trade-offs or future investigation in an issue once Issues are enabled.

Prefer deleting a fork-specific workaround when an upstream implementation fully supersedes it and the regression/hardware evidence supports that removal.
