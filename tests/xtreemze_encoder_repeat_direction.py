#!/usr/bin/env python3
"""Exercise the extracted encoder Repeat policy with a real C harness."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"
BEGIN = "/* BEGIN XTREEMZE_ENCODER_REPEAT_POLICY */"
END = "/* END XTREEMZE_ENCODER_REPEAT_POLICY */"


def extracted_policy(source: str) -> str:
    assert BEGIN in source and END in source, "encoder Repeat pre-process policy is absent"
    return source[source.index(BEGIN):source.index(END) + len(END)]


policy = extracted_policy(SOURCE.read_text())
assert "set_last_" not in policy, "encoder policy must not rewrite Repeat source state"

harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NUM_ENCODERS 2
#define VIAL_ALT_REPEAT_KEY_ENTRIES 32
#define QK_REPEAT_KEY 0x5F00
#define QK_ALT_REPEAT_KEY 0x5F01
#define QK_MODS 0x0100
#define QK_MODS_MAX 0x1FFF
#define QK_MOD_TAP 0x2000
#define QK_MOD_TAP_MAX 0x2FFF
#define QK_LAYER_TAP 0x4000
#define QK_LAYER_TAP_MAX 0x4FFF
#define QK_MODS_GET_MODS(kc) (((kc) >> 8) & 0x1F)
#define QK_MODS_GET_BASIC_KEYCODE(kc) ((kc) & 0xFF)
#define QK_MOD_TAP_GET_TAP_KEYCODE(kc) ((kc) & 0xFF)
#define QK_LAYER_TAP_GET_TAP_KEYCODE(kc) ((kc) & 0xFF)
#define MODS5(mods, kc) ((uint16_t)(((mods) << 8) | (kc)))
#define LCTL(kc) MODS5(0x01, (kc))
#define LSFT(kc) MODS5(0x02, (kc))
#define LALT(kc) MODS5(0x04, (kc))
#define LGUI(kc) MODS5(0x08, (kc))
#define SGUI(kc) MODS5(0x0A, (kc))
#define TD(n) ((uint16_t)(0x5000 + (n)))
#define KC_NO 0
#define KC_1 30
#define KC_2 31
#define KC_A 4
#define KC_B 5
#define KC_D 7
#define KC_H 11
#define KC_J 13
#define KC_K 14
#define KC_L 15
#define KC_N 17
#define KC_U 20
#define KC_W 22
#define KC_UP 40
#define KC_DOWN 41
#define KC_LEFT 42
#define KC_RGHT 43
#define KC_BSPC 44
#define KC_DEL 45
#define KC_TAB 46
#define KC_LBRC 47
#define KC_RBRC 48
#define KC_COMM 49
#define KC_DOT 50
#define MOD_LCTL 0x01
#define MOD_LSFT 0x02
#define MOD_LALT 0x04
#define MOD_LGUI 0x08
#define MOD_RCTL 0x10
#define MOD_RSFT 0x20

enum { KEY_EVENT = 1, ENCODER_CW_EVENT = 2, ENCODER_CCW_EVENT = 3 };
typedef struct { uint8_t row, col; } keypos_t;
typedef struct { keypos_t key; bool pressed; uint16_t time; uint8_t type; } keyevent_t;
typedef struct { keyevent_t event; } keyrecord_t;
typedef struct { uint16_t keycode, alt_keycode; uint8_t allowed_mods, options; } vial_alt_repeat_key_entry_t;
enum {
    vial_arep_option_default_to_this_alt_key = (1 << 0),
    vial_arep_option_bidirectional = (1 << 1),
    vial_arep_option_ignore_mod_handedness = (1 << 2),
    vial_arep_enabled = (1 << 3),
};

static vial_alt_repeat_key_entry_t live[VIAL_ALT_REPEAT_KEY_ENTRIES];
static int live_error[VIAL_ALT_REPEAT_KEY_ENTRIES];
static uint16_t remembered_key;
static uint8_t remembered_mods;
static unsigned native_repeat_calls, native_alt_calls;
static bool native_repeat_pressed[64], native_alt_pressed[64];
static unsigned repeat_events, alt_events;
static uint16_t repeat_latched, alt_latched, observed_endpoint;

static uint8_t bitpop(uint8_t value) {
    uint8_t count = 0;
    while (value) { count += value & 1; value >>= 1; }
    return count;
}
static bool IS_ENCODEREVENT(keyevent_t event) {
    return event.type == ENCODER_CW_EVENT || event.type == ENCODER_CCW_EVENT;
}
static int dynamic_keymap_get_alt_repeat_key(uint8_t index, vial_alt_repeat_key_entry_t *entry) {
    if (live_error[index]) return live_error[index];
    *entry = live[index];
    return 0;
}
static uint16_t get_last_keycode(void) { return remembered_key; }
static uint8_t get_last_mods(void) { return remembered_mods; }

static uint8_t unpack_mods5_for_stock(uint8_t mods5) { return (mods5 & 0x10) ? (uint8_t)(mods5 << 4) : mods5; }
static uint16_t normalize_for_stock(uint16_t keycode, uint8_t *mods) {
    if (keycode >= QK_MODS && keycode <= QK_MODS_MAX) {
        *mods |= unpack_mods5_for_stock(QK_MODS_GET_MODS(keycode));
        return QK_MODS_GET_BASIC_KEYCODE(keycode);
    }
    if (keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX) return QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
    if (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) return QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    return keycode;
}
static bool stock_mods_match(uint8_t mods, uint8_t required, uint8_t allowed, uint8_t options) {
    allowed |= required;
    if (options & vial_arep_option_ignore_mod_handedness) {
        mods = (mods & 0x0F) | (mods >> 4);
        required = (required & 0x0F) | (required >> 4);
        allowed = (allowed & 0x0F) | (allowed >> 4);
    }
    return (mods & required) == required && (mods & ~allowed) == 0;
}
static uint16_t stock_alt_endpoint(void) {
    int best = -1;
    uint16_t result = KC_NO;
    uint8_t mods = remembered_mods;
    uint16_t keycode = normalize_for_stock(remembered_key, &mods);
    for (uint8_t i = 0; i < VIAL_ALT_REPEAT_KEY_ENTRIES; ++i) {
        vial_alt_repeat_key_entry_t entry = live[i];
        uint8_t primary_mods = 0, alternate_mods = 0;
        uint16_t primary, alternate;
        if (live_error[i] || !(entry.options & vial_arep_enabled)) continue;
        primary = normalize_for_stock(entry.keycode, &primary_mods);
        alternate = normalize_for_stock(entry.alt_keycode, &alternate_mods);
        if (primary == keycode && stock_mods_match(mods, primary_mods, entry.allowed_mods, entry.options) && bitpop(primary_mods) > best) {
            result = ((uint16_t)alternate_mods << 8) | alternate;
            best = bitpop(primary_mods);
        }
        if ((entry.options & vial_arep_option_bidirectional) && alternate == keycode && stock_mods_match(mods, alternate_mods, entry.allowed_mods, entry.options) && bitpop(alternate_mods) > best) {
            result = ((uint16_t)primary_mods << 8) | primary;
            best = bitpop(alternate_mods);
        }
    }
    return result;
}
static void repeat_key_invoke(const keyevent_t *event) {
    native_repeat_pressed[repeat_events++] = event->pressed;
    native_repeat_calls++;
    if (event->pressed) repeat_latched = ((uint16_t)remembered_mods << 8) | remembered_key;
    observed_endpoint = repeat_latched;
}
static void alt_repeat_key_invoke(const keyevent_t *event) {
    native_alt_pressed[alt_events++] = event->pressed;
    native_alt_calls++;
    if (event->pressed) alt_latched = stock_alt_endpoint();
    observed_endpoint = alt_latched;
}
'''
harness += policy
harness += r'''
static keyrecord_t event_for(uint8_t encoder, bool pressed, bool matrix) {
    return (keyrecord_t){.event = {.key = {.col = encoder}, .pressed = pressed, .type = matrix ? KEY_EVENT : ENCODER_CW_EVENT}};
}
static void reset_live(void) {
    memset(live, 0, sizeof(live)); memset(live_error, 0, sizeof(live_error));
    live[0] = (vial_alt_repeat_key_entry_t){KC_N, LSFT(KC_N), 0, 12};
    live[1] = (vial_alt_repeat_key_entry_t){LCTL(KC_D), LCTL(KC_U), 0, 12};
    live[2] = (vial_alt_repeat_key_entry_t){KC_W, KC_B, 0, 12};
    live[3] = (vial_alt_repeat_key_entry_t){KC_J, KC_K, 68, 12};
    live[4] = (vial_alt_repeat_key_entry_t){KC_L, KC_H, 68, 12};
    live[7] = (vial_alt_repeat_key_entry_t){KC_U, LSFT(KC_U), 0, 14};
    live[9] = (vial_alt_repeat_key_entry_t){KC_RBRC, KC_LBRC, 119, 14};
    live[11] = (vial_alt_repeat_key_entry_t){KC_BSPC, KC_DEL, 103, 12};
    live[12] = (vial_alt_repeat_key_entry_t){KC_RGHT, KC_LEFT, 0, 14};
    live[13] = (vial_alt_repeat_key_entry_t){KC_UP, KC_DOWN, 0, 14};
    live[14] = (vial_alt_repeat_key_entry_t){TD(12), KC_UP, 0, 14};
    live[15] = (vial_alt_repeat_key_entry_t){KC_1, KC_2, 0, 8};
    live[16] = (vial_alt_repeat_key_entry_t){TD(9), KC_DOWN, 0, 14};
}
static void expect_native(uint16_t seed, uint8_t mods, uint16_t request, bool alternate, uint16_t endpoint) {
    keyrecord_t record = event_for(0, true, false);
    unsigned repeats = native_repeat_calls, alts = native_alt_calls;
    remembered_key = seed; remembered_mods = mods;
    assert(!pre_process_record_user(request, &record));
    assert(native_repeat_calls == repeats + (!alternate));
    assert(native_alt_calls == alts + alternate);
    assert(observed_endpoint == endpoint);
    record.event.pressed = false;
    assert(!pre_process_record_user(request, &record));
    assert(native_repeat_calls == repeats + 2 * (!alternate));
    assert(native_alt_calls == alts + 2 * alternate);
}
static void expect_passthrough(uint16_t seed, uint8_t mods, uint16_t request) {
    keyrecord_t record = event_for(0, true, false);
    unsigned repeats = native_repeat_calls, alts = native_alt_calls;
    remembered_key = seed; remembered_mods = mods;
    assert(pre_process_record_user(request, &record));
    record.event.pressed = false;
    assert(pre_process_record_user(request, &record));
    assert(native_repeat_calls == repeats && native_alt_calls == alts);
}
int main(void) {
    reset_live();
    /* Pair orientation is stable from either remembered side. */
    expect_native(KC_UP, 0, QK_REPEAT_KEY, false, KC_UP);
    expect_native(KC_DOWN, 0, QK_REPEAT_KEY, true, KC_UP);
    expect_native(KC_UP, 0, QK_ALT_REPEAT_KEY, true, KC_DOWN);
    expect_native(KC_DOWN, 0, QK_ALT_REPEAT_KEY, false, KC_DOWN);
    expect_native(KC_RGHT, 0, QK_REPEAT_KEY, false, KC_RGHT);
    expect_native(KC_LEFT, 0, QK_REPEAT_KEY, true, KC_RGHT);
    expect_native(KC_RGHT, 0, QK_ALT_REPEAT_KEY, true, KC_LEFT);
    expect_native(KC_LEFT, 0, QK_ALT_REPEAT_KEY, false, KC_LEFT);
    expect_native(KC_U, 0, QK_REPEAT_KEY, false, KC_U);
    expect_native(LSFT(KC_U), 0, QK_REPEAT_KEY, true, KC_U);
    expect_native(KC_U, 0, QK_ALT_REPEAT_KEY, true, LSFT(KC_U));
    expect_native(LSFT(KC_U), 0, QK_ALT_REPEAT_KEY, false, LSFT(KC_U));
    expect_native(KC_RBRC, MOD_RCTL | MOD_LALT, QK_REPEAT_KEY, false, ((uint16_t)(MOD_RCTL | MOD_LALT) << 8) | KC_RBRC);
    expect_native(KC_LBRC, MOD_RCTL | MOD_LALT, QK_ALT_REPEAT_KEY, false, ((uint16_t)(MOD_RCTL | MOD_LALT) << 8) | KC_LBRC);

    /* Direct winner precedence: slot 13 wins arrows; TD slots remain distinct. */
    expect_native(KC_UP, 0, QK_ALT_REPEAT_KEY, true, KC_DOWN);
    expect_native(KC_DOWN, 0, QK_ALT_REPEAT_KEY, false, KC_DOWN);
    expect_native(TD(12), 0, QK_REPEAT_KEY, false, TD(12));
    expect_native(TD(12), 0, QK_ALT_REPEAT_KEY, true, KC_UP);
    expect_native(TD(9), 0, QK_REPEAT_KEY, false, TD(9));
    expect_native(TD(9), 0, QK_ALT_REPEAT_KEY, true, KC_DOWN);
    expect_native(QK_MOD_TAP | KC_DOWN, 0, QK_REPEAT_KEY, true, KC_UP);
    expect_native(QK_LAYER_TAP | KC_UP, 0, QK_ALT_REPEAT_KEY, true, KC_DOWN);

    /* One-way winners and all fallbacks retain stock processing. */
    expect_passthrough(KC_J, 0, QK_REPEAT_KEY);
    expect_passthrough(KC_L, 0, QK_ALT_REPEAT_KEY);
    expect_passthrough(LCTL(KC_D), 0, QK_REPEAT_KEY);
    expect_passthrough(KC_BSPC, 0, QK_ALT_REPEAT_KEY);
    expect_passthrough(KC_W, 0, QK_REPEAT_KEY);
    expect_passthrough(KC_1, 0, QK_ALT_REPEAT_KEY);
    expect_passthrough(KC_A, 0, QK_REPEAT_KEY);
    live[0] = (vial_alt_repeat_key_entry_t){LCTL(KC_D), LCTL(KC_U), 0, 8};
    live[1] = (vial_alt_repeat_key_entry_t){KC_D, KC_U, 0, 14};
    expect_passthrough(KC_D, MOD_LCTL, QK_ALT_REPEAT_KEY);
    reset_live();
    memset(live, 0, sizeof(live));
    live[0] = (vial_alt_repeat_key_entry_t){LCTL(KC_D), LCTL(KC_U), 0, 8};
    live[1] = (vial_alt_repeat_key_entry_t){LCTL(KC_D), LCTL(KC_A), 0, 10};
    expect_passthrough(LCTL(KC_D), 0, QK_REPEAT_KEY);
    expect_passthrough(LCTL(KC_D), 0, QK_ALT_REPEAT_KEY);
    memset(live, 0, sizeof(live));
    live[0] = (vial_alt_repeat_key_entry_t){KC_N, KC_B, 0, 9};
    expect_passthrough(KC_A, 0, QK_REPEAT_KEY);
    expect_passthrough(KC_A, 0, QK_ALT_REPEAT_KEY);
    reset_live();
    live_error[0] = 1;
    expect_passthrough(KC_UP, 0, QK_REPEAT_KEY);
    reset_live();

    /* Normal matrix events are untouched. */
    keyrecord_t matrix = event_for(0, true, true);
    assert(pre_process_record_user(QK_REPEAT_KEY, &matrix));
    assert(pre_process_record_user(QK_ALT_REPEAT_KEY, &matrix));

    /* Live Vial edits are observed without a cache or re-seeding. */
    expect_native(KC_RGHT, 0, QK_REPEAT_KEY, false, KC_RGHT);
    live[12] = (vial_alt_repeat_key_entry_t){KC_LEFT, KC_RGHT, 0, 14};
    expect_native(KC_RGHT, 0, QK_REPEAT_KEY, true, KC_LEFT);
    reset_live();

    /* Press selection and native endpoint stay latched through a live edit. */
    keyrecord_t latched = event_for(0, true, false);
    remembered_key = KC_UP; remembered_mods = 0;
    unsigned repeats = native_repeat_calls, alts = native_alt_calls;
    assert(!pre_process_record_user(QK_REPEAT_KEY, &latched));
    live[13] = (vial_alt_repeat_key_entry_t){KC_DOWN, KC_UP, 0, 14};
    remembered_key = KC_DOWN;
    latched.event.pressed = false;
    assert(!pre_process_record_user(QK_REPEAT_KEY, &latched));
    assert(native_repeat_calls == repeats + 2 && native_alt_calls == alts && observed_endpoint == KC_UP);
    reset_live();

    /* Invalid encoder indices never index the dispatch array. */
    keyrecord_t out_of_bounds = event_for(NUM_ENCODERS, true, false);
    assert(pre_process_record_user(QK_REPEAT_KEY, &out_of_bounds));
    out_of_bounds.event.pressed = false;
    assert(pre_process_record_user(QK_REPEAT_KEY, &out_of_bounds));

    /* Alternating physical encoders leave no latched policy behind. */
    for (unsigned i = 0; i < 8; ++i) {
        keyrecord_t rapid = event_for(i % NUM_ENCODERS, true, false);
        remembered_key = (i & 1) ? KC_LEFT : KC_UP;
        assert(!pre_process_record_user((i & 1) ? QK_ALT_REPEAT_KEY : QK_REPEAT_KEY, &rapid));
        rapid.event.pressed = false;
        assert(!pre_process_record_user((i & 1) ? QK_ALT_REPEAT_KEY : QK_REPEAT_KEY, &rapid));
    }
    for (unsigned i = 0; i < NUM_ENCODERS; ++i) assert(encoder_repeat_dispatch[i] == encoder_repeat_passthrough);
    for (unsigned i = 0; i < repeat_events; i += 2) assert(native_repeat_pressed[i] && !native_repeat_pressed[i + 1]);
    for (unsigned i = 0; i < alt_events; i += 2) assert(native_alt_pressed[i] && !native_alt_pressed[i + 1]);
    puts("encoder repeat direction: Vial orientation, normalization, precedence, fallback, latching, and bounds pass.");
}
'''

with tempfile.TemporaryDirectory(prefix="halcyon-encoder-repeat-") as tmp:
    c = Path(tmp) / "encoder_repeat.c"
    exe = Path(tmp) / "encoder_repeat"
    c.write_text(harness)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
        "-fsanitize=address,undefined", str(c), "-o", str(exe),
    ], check=True)
    subprocess.run([str(exe)], check=True)
