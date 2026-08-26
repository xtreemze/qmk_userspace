#!/usr/bin/env python3
"""Audit final target flags/post-config, not just source snippets. Run after both builds."""
import argparse
from pathlib import Path
import shlex
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
QMK = (ROOT / 'qmk_firmware').resolve()
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('display_target')
parser.add_argument('encoder_target')
args = parser.parse_args()

for target, pin in [(args.display_target, 27), (args.encoder_target, 2)]:
    obj = QMK / '.build' / f'obj_{target}'
    flags = shlex.split((obj / 'cflags.txt').read_text())
    assert '-DBACKLIGHT_ENABLE' in flags and '-DBACKLIGHT_PWM' in flags, target
    for name in ['quantum/backlight/backlight', 'quantum/process_keycode/process_backlight', 'platforms/chibios/drivers/backlight_pwm']:
        assert (obj / f'{name}.o').is_file(), (target, 'missing compiled object', name)
    check = f'''
#include QMK_KEYBOARD_H
#include <hal.h>
#include "backlight.h"
#ifndef BACKLIGHT_ENABLE
#error QMK backlight processor missing
#endif
#ifndef BACKLIGHT_PWM
#error Expected the PWM driver
#endif
#ifdef BACKLIGHT_BREATHING
#error Breathing is intentionally not enabled
#endif
_Static_assert(BACKLIGHT_PIN == {pin}, "Final backlight pin changed");
_Static_assert(BACKLIGHT_LEVELS == 10, "Expected ten levels");
_Static_assert(BACKLIGHT_PWM_CHANNEL == RP2040_PWM_CHANNEL_B, "Expected channel B");
_Static_assert(BACKLIGHT_PWM_CHANNEL == 2, "Unexpected channel B mapping");
_Static_assert(RP_PWM_USE_PWM5 == TRUE, "PWM slice 5 disabled");
_Static_assert(HAL_USE_PWM == TRUE, "ChibiOS PWM disabled");
_Static_assert(BACKLIGHT_PAL_MODE == (PAL_MODE_ALTERNATE_PWM | PAL_RP_PAD_DRIVE12 | PAL_RP_GPIO_OE), "PWM pin mux changed");
'''
    with tempfile.TemporaryDirectory(prefix='halcyon-pwm-build-') as tmp:
        src = Path(tmp) / 'check.c'
        src.write_text(check)
        subprocess.run(['arm-none-eabi-gcc', *flags, '-fsyntax-only', str(src)], cwd=QMK, check=True)
        macros = subprocess.run(['arm-none-eabi-gcc', *flags, '-E', '-dM', str(src)], cwd=QMK, check=True, capture_output=True, text=True).stdout
        assert '#define BACKLIGHT_PWM_DRIVER PWMD5\n' in macros, target
        # The GPIO fallback must not survive in the compiled module.
        preprocessed = subprocess.run(['arm-none-eabi-gcc', *flags, '-E', str(ROOT / 'users/halcyon_modules/splitkb/halcyon.c')], cwd=QMK, check=True, capture_output=True, text=True).stdout
        match = re.search(r'static void halcyon_backlight_set\([^)]*\)\s*\{(.*?)\n}', preprocessed, re.S)
        assert match, 'Missing preprocessed backlight helper'
        helper = match[1]
        assert 'palSetLineMode' not in helper and 'palWriteLine' not in helper
    print(f'{target}: BACKLIGHT_ENABLE/pwm, PWMD5/B, pin GP{pin}, 10 levels, PWM5/HAL and pin mux verified; breathing off.')
