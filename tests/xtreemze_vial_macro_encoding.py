#!/usr/bin/env python3
"""Run the actual factory seeder through the pinned Vial macro decoder on the host."""
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
KEYMAP = ROOT / 'keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final'
QMK = ROOT / 'qmk_firmware'
source = (KEYMAP / 'keymap.c').read_text()
profile = json.loads((KEYMAP / 'xtreemzeVial.vil').read_text())


def function(text, signature):
    start = text.index(signature)
    end = text.index('\n}', start) + 2
    return text[start:end]


seed = source[source.index('static bool macro_seed_write_byte'):source.index('\n#ifdef QMK_SETTINGS\ntypedef')]
seed = seed.rsplit('\n#endif\n#endif', 1)[0]
custom = source[source.index('enum custom_keycodes'):source.index('\n/*', source.index('enum custom_keycodes'))]
decoder_source = (QMK / 'quantum/dynamic_keymap.c').read_text()
decoder = function(decoder_source, 'static uint16_t decode_keycode') + '\n' + function(decoder_source, 'void dynamic_keymap_macro_send')
expected = []
for slot, actions in enumerate(profile['macro']):
    events = []
    for action in actions:
        if action[0] == 'text':
            events.extend(f'{{2, {byte}}}' for byte in action[1].encode())
        elif action[0] == 'tap':
            for key in action[1:]:
                assert re.fullmatch(r'[A-Z0-9_(), ]+', key), key
                key = key.replace('USER13', 'RGB_TCHD').replace('KC_BSPACE', 'KC_BSPC')
                events.append(f'{{1, {key}}}')
        else:
            raise AssertionError(f'Untested macro action {action[0]}')
    expected.append(f'''count = 0; dynamic_keymap_macro_send({slot});
    {{ const event_t expected[] = {{ {', '.join(events + ['{0, 0}'])} }};
      assert(count == sizeof(expected) / sizeof(expected[0]) - 1);
      assert(memcmp(events, expected, count * sizeof(event_t)) == 0); }}''')

harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "quantum_keycodes.h"
#include "modifiers.h"
#define DYNAMIC_KEYMAP_MACRO_COUNT 32
#define DYNAMIC_KEYMAP_MACRO_DELAY 0
#define VIAL_MACRO_EXT_TAP 5
#define VIAL_MACRO_EXT_DOWN 6
#define VIAL_MACRO_EXT_UP 7
#define SS_QMK_PREFIX 1
#define SS_TAP_CODE 1
#define SS_DOWN_CODE 2
#define SS_UP_CODE 3
#define SS_DELAY_CODE 4
static uint8_t buffer[4096];
static uint16_t capacity = sizeof(buffer);
typedef struct { uint16_t kind, value; } event_t;
static event_t events[1024];
static unsigned count;
static void emit(uint16_t kind, uint16_t value) {
    assert(count < 1024); events[count++] = (event_t){kind, value};
}
static void vial_keycode_tap(uint16_t key) { emit(1, key); }
static void vial_keycode_down(uint16_t key) { (void)key; assert(false); }
static void vial_keycode_up(uint16_t key) { (void)key; assert(false); }
static void wait_ms(int ms) { (void)ms; assert(false); }
static void send_string(const char *s) {
    assert(s[0] == SS_QMK_PREFIX && s[1] == SS_TAP_CODE);
    emit(1, (uint8_t)s[2]);
}
static void send_string_with_delay(const char *s, int delay) {
    (void)delay; while (*s) emit(2, (uint8_t)*s++);
}
static uint32_t nvm_dynamic_keymap_macro_size(void) { return capacity; }
static uint16_t dynamic_keymap_macro_get_buffer_size(void) { return capacity; }
static uint8_t dynamic_keymap_read_byte(uint32_t offset) { assert(offset < capacity); return buffer[offset]; }
static void dynamic_keymap_macro_set_buffer(uint16_t offset, uint16_t size, uint8_t *data) {
    assert(offset + size < capacity); memcpy(buffer + offset, data, size);
}
static void dynamic_keymap_macro_reset(void) { memset(buffer, 0, sizeof(buffer)); }
'''
harness += custom + '\n' + seed + '\n' + decoder
harness += r"""
#define VIAL_ENABLE
#define VIA_ENABLE
#define QMK_SETTINGS
#define XTREEMZE_DEFAULTS_EE_MARKER 0xAF
static struct { uint8_t defaults_marker, rgb_profile, host_family; } xtreemze_user_data;
static unsigned keymap_resets, dynamic_seeds, setting_seeds, layout_seeds, saves;
static void dynamic_keymap_reset(void) { keymap_resets++; }
static void seed_vial_dynamic_entry_defaults(void) { dynamic_seeds++; }
static void seed_qmk_settings_defaults(void) { setting_seeds++; }
static void seed_via_layout_options_default(void) { layout_seeds++; }
static void save_user_data(void) { saves++; }
"""
harness += function(source, 'static void sync_compiled_defaults_to_dynamic_keymap_once')

harness += r"""
int main(void) {
    // Exercise both byte-zero cases independently of the particular factory macros.
    const uint16_t keys[] = {KC_BSPC, HYPR(KC_SPACE), RGB_TCHD, 0x0100, 0x7E00};
    for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        uint16_t offset = 0;
        count = 0;
        dynamic_keymap_macro_reset();
        assert(macro_seed_write_tap(&offset, capacity, keys[i]));
        for (unsigned j = 0; j < offset; j++) assert(buffer[j] != 0);
        dynamic_keymap_macro_send(0);
        assert(count == 1 && events[0].value == keys[i]);
    }
    // Capacity failure must leave the existing macros intact.
    capacity = 1;
    memset(buffer, 0xA5, sizeof(buffer));
    assert(!seed_vial_macro_defaults());
    for (unsigned i = 0; i < sizeof(buffer); i++) assert(buffer[i] == 0xA5);
    capacity = sizeof(buffer);
    assert(seed_vial_macro_defaults());
""" + '\n'.join(expected)
harness += r"""
    xtreemze_user_data.defaults_marker = 0xAE;
    xtreemze_user_data.rgb_profile = 0x42;
    xtreemze_user_data.host_family = 1;
    capacity = 1;
    sync_compiled_defaults_to_dynamic_keymap_once();
    assert(xtreemze_user_data.defaults_marker == 0xAE && saves == 0);
    capacity = sizeof(buffer);
    sync_compiled_defaults_to_dynamic_keymap_once();
    assert(xtreemze_user_data.defaults_marker == 0xAF && saves == 1);
    assert(keymap_resets == 2 && dynamic_seeds == 2 && setting_seeds == 1 && layout_seeds == 1);
    assert(xtreemze_user_data.rgb_profile == 0x42 && xtreemze_user_data.host_family == 1);
    buffer[0] = 0x42; // A subsequent user edit survives later boots.
    sync_compiled_defaults_to_dynamic_keymap_once();
    assert(buffer[0] == 0x42 && saves == 1 && keymap_resets == 2);
    puts("All 32 macros round-trip; escaping, capacity failure and one-time seeding pass.");
}
"""

with tempfile.TemporaryDirectory(prefix='halcyon-macros-') as tmp:
    c = Path(tmp) / 'macros.c'
    exe = Path(tmp) / 'macros'
    c.write_text(harness)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-I', str(QMK / 'quantum'), '-I', str(QMK / 'quantum/keymap_extras'), '-I', str(QMK / 'quantum/sequencer'), str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
