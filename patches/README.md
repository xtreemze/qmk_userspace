# Required Vial-QMK patch

`0001-os-detection-fingerprint-trace.patch` preserves the existing display's
passive OS-fingerprint viewer. It is not a new detector or a USB wake experiment.
The patch was captured in userspace commit
`cfd78d79c0dc191397cabda0402bf314f54e6f24` and was already required by the display
code inherited by the accepted debounce baseline, `9b615b9`.

The patch applies unchanged to pinned Vial-QMK revision
`dd43959ae5c08d8a28d38a1acf7b04e86b14a344`. It includes the existing detector tests.
CI applies it explicitly; local setup is documented in
[the migration report](../docs/halcyon-legacy-migration.md).

Do not import the separate USB-event-queue or split-transport instrumentation
patches from `diagnostics/usb-resume` into this experiment.
