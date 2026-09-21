# Halcyon Ferris Layer Atlas

This directory contains the dependency-free interactive visualizer for the production `xtreemze_final` Halcyon Ferris profile.

## Design

The atlas treats the 34-key Ferris as a layered physical control surface rather than a conventional keycap diagram:

- alphabetic legends are hidden by default;
- the physical column stagger and thumb arc follow the adjacent `vial.json` device definition;
- one half is modeled once and the opposite half is rendered as its exact horizontal mirror, matching the symmetric Ferris hardware;
- the two thumb keys retain the Ferris 15°/30° hardware angles while sitting below, rather than overlapping, the alpha matrix;
- transparent keys are visually subordinate;
- tap/hold, momentary, toggle and one-shot semantics are surfaced before raw keycodes;
- Vial tap dances are expanded into their tap, hold, double-tap, and tap+hold actions with their configured tapping term;
- Vial combos are shown as input chords leading to functional outputs, with macro outputs expanded into readable behavior;
- combo badges identify trigger keys for combos that are available on the selected layer;
- the Halcyon TFT and encoder modules are integrated into their physical half rather than shown as unrelated controls;
- the TFT includes a lightweight animated-GIF display emulator with the active layer identity overlaid from the live profile;
- both encoder directions are shown for every layer;
- all 13 layers retain their firmware layer number and TFT identity while being grouped by task;
- the presentation adapts from a side-by-side desktop view to vertically stacked mirrored halves on narrow screens without horizontal page overflow;
- `prefers-reduced-motion` replaces the TFT animation with a static display treatment.

## Source of truth

The atlas reads the canonical profile directly from:

`keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil`

on the `halcyon` branch. It treats the file as JSON data only; macro payloads are never executed.

The physical interpretation is intentionally separate from the keymap values. `vial.json` remains the device-definition source for Ferris matrix coordinates and module placement. The web renderer encodes that geometry as one reusable half and mirrors it for the other side, preventing independent left/right positioning from drifting apart.

### RGB profile source boundary

The exported `.vil` profile contains the matrix layout, encoders, macros, tap dances, combos, overrides and related Vial settings. The custom layer/modifier/combo RGB profiles are different: firmware stores them in the keyboard's extended EEPROM profile store and exposes them through the read/write raw-HID command `0xF0`.

For that reason the static Atlas does **not** fabricate RGB values from the `.vil` file. In a WebHID-capable browser, **Connect keyboard RGB** requests access to the keyboard and uses only the read operations of the firmware protocol:

- capabilities (`0x01`);
- profile read (`0x02`);
- combo-duration read (`0x06`).

The Atlas then shows the exact saved global, layer, modifier and configured-combo profile values and renders the stored HSV/brightness as a key glow. It reports the exact effect mode and speed, but does not claim to reproduce every spatial QMK RGB Matrix animation. No profile set, preview or save operation is issued by the Atlas.

This keeps the static source truthful while allowing the deployed page to show the actual device configuration when the keyboard is connected.

## Files

- `index.html` — semantic page structure and accessible controls.
- `atlas.css` — base responsive physical geometry and presentation.
- `atlas.js` — live profile loading, key semantics, mirrored-half rendering, module/encoder views, and TFT emulator.
- `atlas-enhancements.css` — corrected thumb placement, RGB visualization, combo badges and behavior-panel presentation.
- `atlas-enhancements.js` — tap-dance/combo semantic expansion and read-only live RGB-profile inspection over WebHID.

## Run locally

From the repository root:

```sh
python3 -m http.server 8000
```

Then open `/docs/visualizer/` on that local server. A web server is required because the atlas loads the canonical profile with `fetch()`.

WebHID access requires a secure context in normal browser use; the deployed GitHub Pages site is HTTPS.

## GitHub Pages

The repository workflow `.github/workflows/pages-visualizer.yml` publishes this directory as the complete Pages artifact whenever `docs/visualizer/**` changes on `halcyon`, and also supports manual dispatch.

Configure the repository Pages source as **GitHub Actions**. The production site is:

`https://xtreemze.github.io/qmk_userspace/`

The deployment uses the official Pages actions and has no frontend build step. Only `docs/visualizer/` is uploaded, so firmware sources and project documentation are not exposed as part of the site artifact.

## Planned extensions

Tracked in issue #58:

- visual layer-topology / access graph;
- host-semantic views for macOS, Android, Linux and Windows;
- optional profile import or richer device-linked inspection without weakening the canonical-profile contract.
