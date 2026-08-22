# Out-of-tree `qmk_firmware` patches

Some firmware here depends on changes to the Vial QMK checkout itself, not just
to this userspace. Those changes live outside this repository and are therefore
invisible to anyone reviewing or cloning it. This directory captures them so the
tree is reproducible and reviewable.

They are kept as patches rather than committed upstream because
`qmk_firmware`'s only remote is `vial-kb/vial-qmk` — upstream, not a fork — so
there is nowhere to push them. If a fork is created later, these become branches
there and this directory can go away.

## Applying

From the repository root, with `qmk_firmware` checked out at the revision this
userspace expects:

```
git -C qmk_firmware apply ../patches/0001-os-detection-fingerprint-trace.patch
git -C qmk_firmware apply ../patches/0002-usb-event-queue-instrumentation.patch
```

Both are additive and fully guarded by compile flags, but "guarded" does not
mean "optional for every target here":

- `0001` is required by the **normal display module**, which defaults
  `XTREEMZE_OS_FINGERPRINT_TRACE` to `yes` in
  `users/halcyon_modules/splitkb/hlc_tft_display/rules.mk`. Build with
  `-e XTREEMZE_OS_FINGERPRINT_TRACE=no` to skip it.
- `0002` is required by any target built with `XTREEMZE_USB_EVENT_TRACE=yes`.
  It is a hard dependency, not a soft one: the hook is a plain `extern`, not a
  weak symbol, so a missing patch is a link error.

Targets that enable neither feature build against an unpatched checkout.

## Contents

### `0001-os-detection-fingerprint-trace.patch`

Pre-existing. Adds the passive USB fingerprint trace behind
`XTREEMZE_OS_FINGERPRINT_TRACE`, which commits `9763e78` and `8855474` already
depend on. Captured here retroactively — it was never recorded anywhere.

### `0002-usb-event-queue-instrumentation.patch`

Adds RAM counters and a trace hook to the ChibiOS USB event queue behind
`XTREEMZE_USB_EVENT_TRACE`. See `users/halcyon_modules/splitkb/xtreemze_usb_trace.h`
for why: all four `usb_event_queue_enqueue()` call sites discard the return
value, so an event dropped because the 16-entry queue is full is invisible.
