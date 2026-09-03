# Halcyon fork documentation

This index separates stable usage documentation from fork-specific project memory, audits and migration material.

## Start here

- [Project guide](PROJECT_GUIDE.md) — collaboration model, validation policy, upstream synchronization, dependency/configuration policy, risk management and documentation standards.
- [Project status and audit snapshot](PROJECT_STATUS.md) — current repository-wide findings, management blockers and follow-up backlog.
- [Related projects and dependency watch](RELATED_PROJECTS.md) — QMK, Vial and SplitKB upstream roles, high-risk surfaces and adoption rules.
- [Module documentation](MODULES.md) — Halcyon module behavior and configuration.
- [Porting guide](PORTING.md) — adding Halcyon support to compatible keyboards/keymaps.

## Ferris `xtreemze_final`

- [Keymap documentation](../keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/readme.md) — canonical profile, layer identities, TFT/backlight behavior, OS-aware shortcuts, Vial custom keys and exact module build commands.
- [Canonical Vial profile](../keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil) — source configuration for compiled dynamic defaults.
- [2026-08-26 default-profile audit](halcyon-default-profile-2026-08-26.md) — historical profile snapshot and migration context.

## Migration and integration

- [Halcyon legacy migration](halcyon-legacy-migration.md) — compatibility and migration notes for the fork-specific firmware line.
- [Fork patches](../patches/README.md) — patches intentionally applied to the pinned firmware dependency.

## Validation

The source-level regression entry point is:

```sh
bash tests/run_ci_regressions.sh
```

Firmware builds, explicit binary comparisons and hardware acceptance remain separate evidence classes. See the [project guide](PROJECT_GUIDE.md#validation-policy) before recording a result as validated.

## Keeping documentation current

Behavior-changing PRs should update the corresponding documentation in the same change. Values that can be cheaply derived from source should be protected by regression checks; `tests/xtreemze_docs_consistency.sh` currently verifies the Ferris factory-default marker and canonical Vial-profile SHA-256 documented by the keymap README.
