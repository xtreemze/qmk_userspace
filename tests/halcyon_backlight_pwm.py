#!/usr/bin/env python3
"""Exercise real QMK backlight processing/PWM math and Halcyon timeout policy."""
from pathlib import Path
import subprocess
import tempfile
import re

ROOT = Path(__file__).resolve().parents[1]
QMK = ROOT / 'qmk_firmware'
module = (ROOT / 'users/halcyon_modules/splitkb/halcyon.c').read_text()


def function(text, signature):
    start = text.index(signature)
    return text[start:text.index('\n}', start) + 2]


def without_includes(text):
    return re.sub(r'^#include .*$', '', text, flags=re.M)


core = without_includes((QMK / 'quantum/backlight/backlight.c').read_text()).split('__attribute__((weak)) void backlight_init_ports')[0]
driver = without_includes((QMK / 'platforms/chibios/drivers/backlight_pwm.c').read_text())
processor = without_includes((QMK / 'quantum/process_keycode/process_backlight.c').read_text())
policy = '\n'.join(function(module, signature) for signature in ['static void halcyon_backlight_set', 'void backlight_wakeup', 'void backlight_suspend'])
if 'bool process_record_kb(' in module:
    policy += '\n' + function(module, 'bool process_record_kb')
else:
    policy += '\nbool process_record_kb(uint16_t key, keyrecord_t *record) { return process_record_user(key, record); }'
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#define BACKLIGHT_ENABLE
#define BACKLIGHT_LEVELS 10
#define BACKLIGHT_PIN 27
#define BACKLIGHT_PWM_DRIVER PWMD5
#define BACKLIGHT_PWM_CHANNEL 2U
#define BACKLIGHT_PAL_MODE 4
#include "backlight.h"
#include "keycodes.h"
#define dprintf(...) ((void)0)
typedef struct { struct { bool pressed; } event; } keyrecord_t;
static bool master = true;
static bool backlight_off;
static bool is_keyboard_master(void) { return master; }
static bool process_record_user(uint16_t key, keyrecord_t *record) { (void)key; (void)record; return true; }
static backlight_config_t stored;
static unsigned writes;
static void eeconfig_read_backlight(backlight_config_t *config) { *config = stored; }
static void eeconfig_update_backlight(backlight_config_t *config) { stored = *config; writes++; }
// Observe the actual driver's selected pin, PWM slice/channel and computed duty.
typedef struct { unsigned mode; } PWMChannelConfig;
typedef struct { unsigned frequency, period; PWMChannelConfig channels[2]; } PWMConfig;
typedef struct { unsigned period; } PWMDriver;
static PWMDriver PWMD5;
static unsigned width;
static unsigned pin;
#define PWM_OUTPUT_ACTIVE_HIGH 1
#define PWM_OUTPUT_ACTIVE_LOW 0
#define PAL_PORT(p) 0
#define PAL_PAD(p) (p)
#define PAL_MODE_ALTERNATE(m) (m)
#define PWM_FRACTION_TO_WIDTH(driver, denominator, numerator) ((driver)->period * (numerator) / (denominator))
static void palSetPadMode(unsigned port, unsigned pad, unsigned mode) { assert(port == 0 && mode == 4); pin = pad; }
static void pwmStart(PWMDriver *driver, const PWMConfig *config) {
    assert(driver == &PWMD5 && config->channels[1].mode == PWM_OUTPUT_ACTIVE_HIGH);
    driver->period = config->period;
}
static void pwmDisableChannel(PWMDriver *driver, unsigned channel) { assert(driver == &PWMD5 && channel == 1); width = 0; }
static void pwmEnableChannel(PWMDriver *driver, unsigned channel, unsigned duty) { assert(driver == &PWMD5 && channel == 1); width = duty; }
'''
harness += core + driver + processor + policy
harness += r'''
int main(void) {
    stored = (backlight_config_t){.valid = true, .enable = true, .level = 5};
    backlight_init(); backlight_init_ports();
    assert(pin == 27 && width > 0);
    backlight_level(1); unsigned dim = width;
    backlight_level(10); assert(width > dim * 20 && dim > 0);
    keyrecord_t press = {.event.pressed = true};
    assert(!process_backlight(BL_DOWN, &press) && get_backlight_level() == 9);
    assert(!process_backlight(BL_UP, &press) && get_backlight_level() == 10);
    assert(!process_backlight(BL_STEP, &press) && get_backlight_level() == 0 && width == 0);
    backlight_level(5);
    unsigned saved_writes = writes;
    uint8_t saved_config = stored.raw;
    backlight_suspend();
    assert(width == 0 && get_backlight_level() == 0);
    assert(writes == saved_writes && stored.raw == saved_config);
    backlight_wakeup();
    assert(get_backlight_level() == 5 && width > 0 && writes == saved_writes);
    assert(!process_backlight(BL_TOGG, &press) && width == 0);
    saved_config = stored.raw; saved_writes = writes;
    backlight_suspend(); backlight_wakeup();
    assert(width == 0 && !is_backlight_enabled() && get_backlight_level() == 5);
    assert(writes == saved_writes && stored.raw == saved_config);
    // First brightness key after inactivity applies to saved brightness, not transient zero.
    backlight_toggle();
    backlight_suspend();
    assert(process_record_kb(BL_UP, &press));
    assert(!process_backlight(BL_UP, &press));
    assert(get_backlight_level() == 6 && stored.level == 6);
    // The slave follows QMK split synchronization; lifecycle helpers must not overwrite it.
    master = false;
    backlight_level_noeeprom(3); saved_writes = writes;
    backlight_suspend(); backlight_wakeup();
    assert(get_backlight_level() == 3 && writes == saved_writes);
    assert(process_backlight(BL_BRTG, &press)); // Breathing remains intentionally disabled.
    puts("PWM GP27/slice5/B: levels 1/10 distinct; controls, timeout, manual-off and slave policy pass.");
}
'''
with tempfile.TemporaryDirectory(prefix='halcyon-pwm-') as tmp:
    c, exe = Path(tmp) / 'backlight.c', Path(tmp) / 'backlight'
    c.write_text(harness)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-fsanitize=address,undefined', '-I', str(QMK / 'quantum/backlight'), '-I', str(QMK / 'quantum'), str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
