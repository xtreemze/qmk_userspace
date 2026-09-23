# Macro hardening: Vial M10 bootstrap macro

**Issue:** #26 — avoid a mutable curl-to-shell target in M10

## Current command and safety boundary

The shipped M10 macro types this text:

```bash
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/master/bootstrap.sh | bash
```

It does not include Enter/Return. The firmware therefore does not execute the script by invoking M10 alone; the user can inspect the command and must submit it manually.

Manual submission reduces accidental execution, but the `master` ref remains mutable. The effective script can change without a firmware/profile change, so the command is not reproducible.

## Required hardened form

A branch rename is not a security boundary. `master`, `main`, or any other branch can move.

The preferred form is a reviewed immutable commit:

```bash
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/<full-reviewed-commit-sha>/bootstrap.sh | bash
```

An alternative is a local/bootstrap wrapper that downloads content and verifies an immutable digest or revision before execution.

Do not add Enter/Return when hardening the macro.

## Release and migration requirements

The chosen bootstrap revision must be reviewed and accessible through the intended distribution path. A firmware release that changes M10 should record that exact revision in its release/provenance notes.

Because M10 is stored in Vial dynamic configuration, changing a compiled factory default must not be implemented by silently resetting unrelated user keymaps, macros, tap dances, combos, overrides, or settings. Coordinate the update with the user-data/default-profile migration policy.

When M10 changes, keep these representations synchronized:

1. `xtreemzeVial.vil`;
2. generated compiled defaults in `keymap.c`;
3. any compatibility macro path;
4. factory/profile regressions and documented profile hash;
5. release/provenance documentation.

## Verification

Before manually running a command typed by M10, inspect the immutable revision and script content. For a pinned revision, fetching it without piping to the shell is sufficient for review:

```bash
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/<full-reviewed-commit-sha>/bootstrap.sh
```

Only submit the executable form after deciding to trust that exact content.

## Status

The current shipped M10 still targets mutable `master`; #26 remains open until an immutable or independently verified source and a non-destructive migration are integrated.
