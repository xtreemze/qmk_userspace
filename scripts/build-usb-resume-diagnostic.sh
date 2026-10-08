#!/usr/bin/env bash
set -euo pipefail

variant="${1:-display}"
case "$variant" in
  display)
    module_flag=HLC_TFT_DISPLAY=1
    target=splitkb_halcyon_ferris_rev1_xtreemze_final_display_usb_reset_diag
    ;;
  encoder)
    module_flag=HLC_ENCODER=1
    target=splitkb_halcyon_ferris_rev1_xtreemze_final_encoder_usb_reset_diag
    ;;
  *)
    printf 'usage: %s [display|encoder]\n' "$0" >&2
    exit 2
    ;;
esac

qmk compile \
  -kb splitkb/halcyon/ferris/rev1 \
  -km xtreemze_final \
  -e "$module_flag" \
  -e XTREEMZE_USB_RESET_DIAGNOSTIC=yes \
  -e "TARGET=$target"
