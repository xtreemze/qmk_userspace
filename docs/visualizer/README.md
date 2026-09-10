# Halcyon Ferris Layer Atlas

This directory contains the dependency-free interactive visualizer for the production `xtreemze_final` Halcyon Ferris profile.

## Design

The atlas treats the 34-key Ferris as a layered control surface rather than a conventional keycap diagram:

- alphabetic legends are hidden by default;
- the exact Ferris matrix geometry is preserved;
- transparent keys are visually subordinate;
- tap/hold, momentary, toggle and one-shot semantics are surfaced before raw keycodes;
- left/right Halcyon module controls are separated from the 34-key typing surface;
- both encoder directions are shown for every layer;
- all 13 layers retain their firmware layer number and TFT identity while being grouped by task.

## Source of truth

`index.html` reads the canonical profile directly from:

`keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil`

on the `halcyon` branch. It treats the file as JSON data only; macro payloads are not evaluated or executed. The physical matrix interpretation follows the coordinates in the adjacent `vial.json` device definition.

This deliberately avoids maintaining a second hand-authored copy of the keymap.

## Run locally

From the repository root:

```sh
python3 -m http.server 8000
```

Then open `/docs/visualizer/` on that local server. A web server is required because the atlas loads the canonical profile with `fetch()`.

## GitHub Pages

The repository workflow `.github/workflows/pages-visualizer.yml` publishes this directory as the complete Pages artifact whenever `docs/visualizer/**` changes on `halcyon`, and also supports manual dispatch.

Configure the repository Pages source as **GitHub Actions**. Once enabled, the production site is expected at:

`https://xtreemze.github.io/qmk_userspace/`

The deployment uses the official Pages actions and has no frontend build step. Only `docs/visualizer/` is uploaded, so firmware sources and project documentation are not exposed as part of the site artifact.

## Planned extensions

Tracked in issue #58:

- visual layer-topology / access graph;
- combo overlays;
- expanded tap-dance actions;
- host-semantic views for macOS, Android, Linux and Windows;
- optional profile import or device-linked inspection without weakening the canonical-profile contract.
