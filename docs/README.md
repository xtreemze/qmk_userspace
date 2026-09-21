# Halcyon fork documentation

This index separates stable usage documentation from fork-specific project memory, audits and migration material.

## Start here

- [Contribution workflow](../CONTRIBUTING.md) — where code changes, research, decisions, risks, audit findings, hardware evidence, and project-state updates belong.
- [Project guide](PROJECT_GUIDE.md) — collaboration model, validation policy, upstream synchronization, dependency/configuration policy, risk management and documentation standards.
- [Project status and audit snapshot](PROJECT_STATUS.md) — current repository-wide findings, management blockers and follow-up backlog.
- [Related projects and dependency watch](RELATED_PROJECTS.md) — QMK, Vial and SplitKB upstream roles, high-risk surfaces and adoption rules.
- [Module documentation](MODULES.md) — Halcyon module behavior and configuration.
- [Porting guide](PORTING.md) — adding Halcyon support to compatible keyboards/keymaps.

## Ferris `xtreemze_final`

- [Interactive layer atlas](https://xtreemze.github.io/qmk_userspace/) — behavior-first web visualizer for the 34-key Ferris geometry, all 13 layers, module controls and encoders; alpha legends are hidden by default. [Source](visualizer/).
- [Keymap documentation](../keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/readme.md) — canonical profile, layer identities, TFT/backlight behavior, OS-aware shortcuts, Vial custom keys and exact module build commands.
- [Canonical Vial profile](../keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil) — source configuration for compiled dynamic defaults.
- [2026-08-26 default-profile audit](halcyon-default-profile-2026-08-26.md) — historical profile snapshot and migration context.

## Migration and integration

- [Halcyon legacy migration](halcyon-legacy-migration.md) — compatibility and migration notes for the fork-specific firmware line.
- [Host configurator protocol contract](CONFIGURATOR_PROTOCOL.md) — stable raw-HID compatibility boundary and machine-readable contract for Python, WebHID, and native/Tauri clients.
- [Fork patches](../patches/README.md) — patches intentionally applied to the pinned firmware dependency.

## Validation

The source-level regression entry point is:

```sh
bash tests/run_ci_regressions.sh
```

Firmware builds, explicit binary comparisons and hardware acceptance remain separate evidence classes. See the [project guide](PROJECT_GUIDE.md#validation-policy) before recording a result as validated.

## Keeping documentation current

Behavior-changing PRs should update the corresponding documentation in the same change. Values that can be cheaply derived from source should be protected by regression checks; `tests/xtreemze_docs_consistency.sh` currently verifies the Ferris factory-default marker and canonical Vial-profile SHA-256 documented by the keymap README.

Use [CONTRIBUTING.md](../CONTRIBUTING.md) as the short operational entry point and `PROJECT_GUIDE.md` as the complete policy. Durable decisions, risks, research, lessons learned, configuration findings, and audit results should remain discoverable through Issues rather than existing only in chat or merged PR discussion.