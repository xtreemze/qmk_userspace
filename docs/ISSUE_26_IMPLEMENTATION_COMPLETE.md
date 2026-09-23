# Issue #26: M10 bootstrap hardening status

**Status:** Open

## Current production behavior

The canonical Vial profile and compiled firmware currently type:

```bash
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/master/bootstrap.sh | bash
```

M10 does **not** append Enter/Return. It only types the command; execution still requires explicit user submission. That is a useful safety boundary, but it does not make the fetched script reproducible.

## Why #26 remains open

Changing `master` to another branch name such as `main` would not solve the issue: both are mutable refs. Reproducible firmware must identify immutable bootstrap content, normally by a reviewed commit SHA or by a wrapper that verifies an immutable digest/revision before execution.

The live configuration therefore does not yet satisfy #26's hardening objective. Previous documentation that described a `main` branch URL as "stable" or "resolved" was incorrect and did not match the shipped profile.

## Integration constraint

M10 is part of the canonical dynamic Vial profile. Updating it by bumping `XTREEMZE_DEFAULTS_EE_MARKER` would trigger factory reseeding and can replace user-edited Vial state. Hardening M10 must therefore be coordinated with the persistence/migration policy rather than using a broad factory reset solely to change one macro.

A hardened change must update together:

- the canonical `xtreemzeVial.vil` M10 action;
- generated/compiled macro defaults and any compatibility alias;
- regression fixtures and canonical profile hash;
- release notes/provenance identifying the reviewed bootstrap revision;
- persistence logic that changes M10 without unnecessarily resetting unrelated Vial configuration.

## Acceptance

#26 can close when the shipped M10 source is immutable or independently verified, the command remains non-executing until manual submission, and the migration path does not erase unrelated user configuration.
