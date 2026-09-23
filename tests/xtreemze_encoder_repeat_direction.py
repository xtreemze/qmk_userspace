#!/usr/bin/env python3
"""Exercise the native-only deterministic encoder Repeat policy with a C harness."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/keymap.c"
BEGIN = "/* BEGIN XTREEMZE_ENCODER_REPEAT_POLICY */"
END = "/* END XTREEMZE_ENCODER_REPEAT_POLICY */"


def extracted_policy(source: str) -> str:
    assert BEGIN in source and END in source, "encoder Repeat pre-process policy is absent"
    return source[source.index(BEGIN) : source.index(END) + len(END)]


policy = extracted_policy(SOURCE.read_text())
assert "vial_alt_repeat_key_resolve_direct" in policy, "encoder policy must use Vial's RAM resolver"
assert "repeat_key_invoke" in policy, "encoder primary dispatch must stay on native Repeat"
assert "alt_repeat_key_invoke" in policy, "encoder alternate dispatch must stay on native Alternate Repeat"

for forbidden in (
    "dynamic_keymap_get_alt_repeat_key",
    "get_last_record",
    "encoder_repeat_native_translated",
    "translated_endpoint",
    "xtreemze_encoder_direction_keycode",
    "xtreemze_encoder_symbol_keycode",
    "set_last_keycode",
    "set_last_record",
    "set_last_mods",
):
    assert forbidden not in policy, f"native-only encoder policy must not contain {forbidden}"

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
#define KC_TRNS 1
#define KC_A 4
#define KC_B 5
#define KC_D 7
#define KC_G 10
#define KC_H 11
#define KC_J 13
#define KC_K 14
#define KC_L 15
#define KC_N 17
#define KC_U 20
#define KC_W 22
#define KC_X 27
#define KC_9 38
#define KC_0 39
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
#define KC_PGUP 51
#define KC_PGDN 52
#define KC_HOME 53
#define KC_END 54
#define SC_LSPO 0x6000
#define SC_RSPC 0x6001
#define MOD_LCTL 0x01
#define MOD_LSFT 0x02
#define MOD_LALT 0x04
#define MOD_LGUI 0x08
#define MOD_RCTL 0x10
#define MOD_RSFT 0x20

enum { KEY_EVENT = 1, ENCODER_CW_EVENT = 2, ENCODER_CCW_EVENT = 3 };

typedef struct {
    uint8_t row;
    uint8_t col;
} keypos_t;

typedef struct {
    keypos_t key;
    bool pressed;
    uint16_t time;
    uint8_t type;
} keyevent_t;

typedef struct {
    keyevent_t event;
} keyrecord_t;

enum {
    vial_arep_option_default_to_this_alt_key = (1 << 0),
    vial_arep_option_bidirectional = (1 << 1),
    vial_arep_option_ignore_mod_handedness = (1 << 2),
    vial_arep_enabled = (1 << 3),
};

typedef enum {
    vial_alt_repeat_match_none = 0,
    vial_alt_repeat_match_primary,
    vial_alt_repeat_match_alternate,
} vial_alt_repeat_match_side_t;

typedef struct {
    uint8_t index;
    uint8_t options;
    vial_alt_repeat_match_side_t side;
} vial_alt_repeat_key_match_t;

typedef struct {
    uint16_t keycode;
    uint16_t alt_keycode;
    uint8_t required_mods;
    uint8_t alt_required_mods;
    uint8_t allowed_mods;
    uint8_t options;
} live_alt_repeat_entry_t;

typedef struct {
    uint16_t keycode;
    uint8_t mods;
} endpoint_t;

static live_alt_repeat_entry_t live[VIAL_ALT_REPEAT_KEY_ENTRIES];
static uint16_t remembered_keycode;
static uint8_t remembered_mods;
static unsigned policy_resolver_calls;
static unsigned nvm_reads;
static unsigned native_repeat_calls;
static unsigned native_alt_calls;
static bool repeat_pressed[256];
static bool alt_pressed[256];
static unsigned repeat_events;
static unsigned alt_events;
static endpoint_t observed_endpoint;

static uint8_t bitpop(uint8_t value) {
    uint8_t count = 0;
    while (value != 0) {
        count += value & 1u;
        value >>= 1;
    }
    return count;
}

static bool IS_ENCODEREVENT(keyevent_t event) {
    return event.type == ENCODER_CW_EVENT || event.type == ENCODER_CCW_EVENT;
}

static uint8_t unpack_mods5(uint8_t mods5) {
    return (mods5 & 0x10u) != 0 ? (uint8_t)(mods5 << 4) : mods5;
}

static uint16_t normalize(uint16_t keycode, uint8_t *mods) {
    if (keycode >= QK_MODS && keycode <= QK_MODS_MAX) {
        *mods |= unpack_mods5(QK_MODS_GET_MODS(keycode));
        return QK_MODS_GET_BASIC_KEYCODE(keycode);
    }
    if (keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX) {
        return QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
    }
    if (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) {
        return QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    }
    return keycode;
}

static bool mods_match(uint8_t mods, uint8_t required, uint8_t allowed, uint8_t options) {
    allowed |= required;
    if ((options & vial_arep_option_ignore_mod_handedness) != 0) {
        mods = (mods & 0x0fu) | (mods >> 4);
        required = (required & 0x0fu) | (required >> 4);
        allowed = (allowed & 0x0fu) | (allowed >> 4);
    }
    return (mods & required) == required && (mods & (uint8_t)~allowed) == 0;
}

static void set_entry(uint8_t index, uint16_t primary, uint16_t alternate, uint8_t allowed, uint8_t options) {
    uint8_t primary_mods = 0;
    uint8_t alternate_mods = 0;
    live[index] = (live_alt_repeat_entry_t){
        .keycode = normalize(primary, &primary_mods),
        .alt_keycode = normalize(alternate, &alternate_mods),
        .required_mods = primary_mods,
        .alt_required_mods = alternate_mods,
        .allowed_mods = allowed,
        .options = options,
    };
}

static bool resolve_direct_impl(uint16_t raw_keycode, uint8_t mods, vial_alt_repeat_key_match_t *match) {
    const uint16_t keycode = normalize(raw_keycode, &mods);
    int8_t best_fit = -1;
    vial_alt_repeat_key_match_t best = {0};

    for (uint8_t i = 0; i < VIAL_ALT_REPEAT_KEY_ENTRIES; ++i) {
        const live_alt_repeat_entry_t *entry = &live[i];
        if ((entry->options & vial_arep_enabled) == 0) {
            continue;
        }

        if (entry->keycode == keycode &&
            mods_match(mods, entry->required_mods, entry->allowed_mods, entry->options)) {
            const int8_t fit = (int8_t)bitpop(entry->required_mods);
            if (fit > best_fit) {
                best = (vial_alt_repeat_key_match_t){
                    .index = i,
                    .options = entry->options,
                    .side = vial_alt_repeat_match_primary,
                };
                best_fit = fit;
            }
        }

        if ((entry->options & vial_arep_option_bidirectional) != 0 &&
            entry->alt_keycode == keycode &&
            mods_match(mods, entry->alt_required_mods, entry->allowed_mods, entry->options)) {
            const int8_t fit = (int8_t)bitpop(entry->alt_required_mods);
            if (fit > best_fit) {
                best = (vial_alt_repeat_key_match_t){
                    .index = i,
                    .options = entry->options,
                    .side = vial_alt_repeat_match_alternate,
                };
                best_fit = fit;
            }
        }
    }

    if (best_fit < 0) {
        return false;
    }
    if (match != NULL) {
        *match = best;
    }
    return true;
}

static bool vial_alt_repeat_key_resolve_direct(uint16_t keycode, uint8_t mods, vial_alt_repeat_key_match_t *match) {
    policy_resolver_calls++;
    return resolve_direct_impl(keycode, mods, match);
}

static uint16_t get_last_keycode(void) {
    return remembered_keycode;
}

static uint8_t get_last_mods(void) {
    return remembered_mods;
}

static endpoint_t repeat_endpoint(void) {
    uint8_t mods = remembered_mods;
    const uint16_t keycode = normalize(remembered_keycode, &mods);
    return (endpoint_t){.keycode = keycode, .mods = mods};
}

static endpoint_t alt_endpoint(void) {
    uint8_t mods = remembered_mods;
    const uint16_t keycode = normalize(remembered_keycode, &mods);
    vial_alt_repeat_key_match_t match = {0};

    if (!resolve_direct_impl(keycode, mods, &match)) {
        return (endpoint_t){.keycode = KC_TRNS, .mods = 0};
    }

    const live_alt_repeat_entry_t *entry = &live[match.index];
    if (match.side == vial_alt_repeat_match_alternate) {
        return (endpoint_t){.keycode = entry->keycode, .mods = entry->required_mods};
    }
    return (endpoint_t){.keycode = entry->alt_keycode, .mods = entry->alt_required_mods};
}

static void repeat_key_invoke(const keyevent_t *event) {
    repeat_pressed[repeat_events++] = event->pressed;
    native_repeat_calls++;
    if (event->pressed) {
        observed_endpoint = repeat_endpoint();
    }
}

static void alt_repeat_key_invoke(const keyevent_t *event) {
    alt_pressed[alt_events++] = event->pressed;
    native_alt_calls++;
    if (event->pressed) {
        observed_endpoint = alt_endpoint();
    }
}
'''

harness += policy

harness += r'''
static keyrecord_t encoder_event(uint8_t encoder, bool pressed) {
    return (keyrecord_t){
        .event = {
            .key = {.row = 0, .col = encoder},
            .pressed = pressed,
            .type = ENCODER_CW_EVENT,
        },
    };
}

static keyrecord_t matrix_event(bool pressed) {
    return (keyrecord_t){
        .event = {
            .key = {.row = 0, .col = 0},
            .pressed = pressed,
            .type = KEY_EVENT,
        },
    };
}

static endpoint_t normalized_endpoint(uint16_t raw, uint8_t mods) {
    return (endpoint_t){.keycode = normalize(raw, &mods), .mods = mods};
}

static void assert_endpoint(endpoint_t expected) {
    assert(observed_endpoint.keycode == expected.keycode);
    assert(observed_endpoint.mods == expected.mods);
}

static void remember(uint16_t keycode, uint8_t mods) {
    remembered_keycode = keycode;
    remembered_mods = mods;
}

static void reset_runtime(void) {
    memset(encoder_repeat_dispatch, 0, sizeof(encoder_repeat_dispatch));
    native_repeat_calls = 0;
    native_alt_calls = 0;
    repeat_events = 0;
    alt_events = 0;
    policy_resolver_calls = 0;
    nvm_reads = 0;
    observed_endpoint = (endpoint_t){0};
}

static void reset_live(void) {
    memset(live, 0, sizeof(live));

    set_entry(0, KC_UP, KC_DOWN, 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(1, KC_RGHT, KC_LEFT, 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(2, TD(12), TD(9), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(3, TD(11), TD(10), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(4, LCTL(KC_U), LCTL(KC_D), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(5, KC_TAB, LSFT(KC_TAB), MOD_LCTL | MOD_RCTL, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(6, LGUI(KC_G), SGUI(KC_G), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(7, LSFT(KC_DOT), LSFT(KC_COMM), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(8, KC_RBRC, KC_LBRC, 0x77, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(9, SC_RSPC, SC_LSPO, 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(10, KC_PGUP, KC_PGDN, 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(11, KC_END, KC_HOME, 0, vial_arep_enabled | vial_arep_option_bidirectional);

    /* Direct but one-way: deterministic encoder policy must pass through. */
    set_entry(20, KC_N, KC_B, 0, vial_arep_enabled);

    /* Default-only fallback: public direct resolver must not synthesize a match. */
    set_entry(
        21,
        KC_W,
        KC_X,
        0,
        vial_arep_enabled | vial_arep_option_default_to_this_alt_key
    );

    /* Disabled direct pair: ignored. */
    set_entry(22, KC_A, KC_H, 0, vial_arep_option_bidirectional);
}

static void expect_request(
    uint16_t remembered,
    uint8_t remembered_modifiers,
    uint16_t request,
    bool expect_alt,
    endpoint_t expected
) {
    keyrecord_t record = encoder_event(0, true);
    const unsigned before_repeat = native_repeat_calls;
    const unsigned before_alt = native_alt_calls;
    const unsigned before_resolve = policy_resolver_calls;
    const unsigned before_nvm = nvm_reads;

    remember(remembered, remembered_modifiers);
    assert(!pre_process_record_user(request, &record));
    assert(policy_resolver_calls == before_resolve + 1);
    assert(nvm_reads == before_nvm);

    if (expect_alt) {
        assert(native_alt_calls == before_alt + 1);
        assert(native_repeat_calls == before_repeat);
    } else {
        assert(native_repeat_calls == before_repeat + 1);
        assert(native_alt_calls == before_alt);
    }
    assert_endpoint(expected);

    record.event.pressed = false;
    assert(!pre_process_record_user(request, &record));
    assert(policy_resolver_calls == before_resolve + 1);
    assert(nvm_reads == before_nvm);

    if (expect_alt) {
        assert(native_alt_calls == before_alt + 2);
        assert(native_repeat_calls == before_repeat);
    } else {
        assert(native_repeat_calls == before_repeat + 2);
        assert(native_alt_calls == before_alt);
    }
    assert(encoder_repeat_dispatch[0] == encoder_repeat_passthrough);
}

static void expect_bidirectional_pair(uint16_t primary, uint16_t alternate) {
    endpoint_t primary_endpoint = normalized_endpoint(primary, 0);
    endpoint_t alternate_endpoint = normalized_endpoint(alternate, 0);

    expect_request(primary, 0, QK_REPEAT_KEY, false, primary_endpoint);
    expect_request(alternate, 0, QK_REPEAT_KEY, true, primary_endpoint);
    expect_request(primary, 0, QK_ALT_REPEAT_KEY, true, alternate_endpoint);
    expect_request(alternate, 0, QK_ALT_REPEAT_KEY, false, alternate_endpoint);
}

static void expect_passthrough(uint16_t remembered, uint8_t mods, uint16_t request) {
    keyrecord_t record = encoder_event(0, true);
    const unsigned before_repeat = native_repeat_calls;
    const unsigned before_alt = native_alt_calls;
    const unsigned before_resolve = policy_resolver_calls;
    const unsigned before_nvm = nvm_reads;

    remember(remembered, mods);
    assert(pre_process_record_user(request, &record));
    assert(policy_resolver_calls == before_resolve + 1);
    assert(native_repeat_calls == before_repeat);
    assert(native_alt_calls == before_alt);
    assert(nvm_reads == before_nvm);

    record.event.pressed = false;
    assert(pre_process_record_user(request, &record));
    assert(policy_resolver_calls == before_resolve + 1);
    assert(native_repeat_calls == before_repeat);
    assert(native_alt_calls == before_alt);
    assert(nvm_reads == before_nvm);
}

static void expect_vial_precedence(void) {
    vial_alt_repeat_key_match_t match = {0};

    /* More required modifiers beat a less-specific earlier match. */
    set_entry(24, KC_K, KC_U, MOD_LCTL, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(25, LCTL(KC_K), LCTL(KC_A), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    assert(vial_alt_repeat_key_resolve_direct(KC_K, MOD_LCTL, &match));
    assert(match.index == 25);
    assert(match.side == vial_alt_repeat_match_primary);

    /* Equal-fit ties retain the earlier slot. */
    set_entry(26, LALT(KC_D), LALT(KC_U), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    set_entry(27, LALT(KC_D), LALT(KC_X), 0, vial_arep_enabled | vial_arep_option_bidirectional);
    assert(vial_alt_repeat_key_resolve_direct(LALT(KC_D), 0, &match));
    assert(match.index == 26);

    /* Mod-tap and layer-tap inputs use their tap key for direct matching. */
    set_entry(28, KC_D, KC_U, 0, vial_arep_enabled | vial_arep_option_bidirectional);
    assert(vial_alt_repeat_key_resolve_direct(QK_MOD_TAP | KC_D, 0, &match));
    assert(match.index == 28);
    assert(vial_alt_repeat_key_resolve_direct(QK_LAYER_TAP | KC_D, 0, &match));
    assert(match.index == 28);
}

static void expect_live_edit_coherence(void) {
    keyrecord_t record = encoder_event(0, true);

    remember(KC_UP, 0);
    assert(!pre_process_record_user(QK_REPEAT_KEY, &record));
    assert(native_repeat_calls > 0);
    record.event.pressed = false;
    assert(!pre_process_record_user(QK_REPEAT_KEY, &record));

    /* Vial reloads the same RAM table immediately after a live edit. */
    set_entry(0, KC_DOWN, KC_UP, 0, vial_arep_enabled | vial_arep_option_bidirectional);

    record = encoder_event(0, true);
    remember(KC_UP, 0);
    const unsigned before_alt = native_alt_calls;
    assert(!pre_process_record_user(QK_REPEAT_KEY, &record));
    assert(native_alt_calls == before_alt + 1);
    assert_endpoint(normalized_endpoint(KC_DOWN, 0));
    record.event.pressed = false;
    assert(!pre_process_record_user(QK_REPEAT_KEY, &record));

    set_entry(0, KC_UP, KC_DOWN, 0, vial_arep_enabled | vial_arep_option_bidirectional);
}

static void expect_press_release_latching(void) {
    keyrecord_t record = encoder_event(1, true);
    remember(KC_UP, 0);

    const unsigned before_repeat = native_repeat_calls;
    const unsigned before_alt = native_alt_calls;
    const unsigned before_resolve = policy_resolver_calls;

    assert(!pre_process_record_user(QK_REPEAT_KEY, &record));
    assert(native_repeat_calls == before_repeat + 1);
    assert(native_alt_calls == before_alt);
    assert(policy_resolver_calls == before_resolve + 1);

    /* Neither a live edit nor remembered-key change can alter release dispatch. */
    set_entry(0, KC_DOWN, KC_UP, 0, vial_arep_enabled | vial_arep_option_bidirectional);
    remember(KC_DOWN, 0);

    record.event.pressed = false;
    assert(!pre_process_record_user(QK_REPEAT_KEY, &record));
    assert(native_repeat_calls == before_repeat + 2);
    assert(native_alt_calls == before_alt);
    assert(policy_resolver_calls == before_resolve + 1);
    assert(encoder_repeat_dispatch[1] == encoder_repeat_passthrough);

    set_entry(0, KC_UP, KC_DOWN, 0, vial_arep_enabled | vial_arep_option_bidirectional);
}

int main(void) {
    reset_live();
    reset_runtime();

    /* The Vial table, not semantic hard-coding, defines every orientation. */
    expect_bidirectional_pair(KC_UP, KC_DOWN);
    expect_bidirectional_pair(KC_RGHT, KC_LEFT);
    expect_bidirectional_pair(TD(12), TD(9));
    expect_bidirectional_pair(TD(11), TD(10));
    expect_bidirectional_pair(LCTL(KC_U), LCTL(KC_D));
    expect_bidirectional_pair(KC_TAB, LSFT(KC_TAB));
    expect_bidirectional_pair(LGUI(KC_G), SGUI(KC_G));
    expect_bidirectional_pair(LSFT(KC_DOT), LSFT(KC_COMM));
    expect_bidirectional_pair(KC_RBRC, KC_LBRC);
    expect_bidirectional_pair(SC_RSPC, SC_LSPO);
    expect_bidirectional_pair(KC_PGUP, KC_PGDN);
    expect_bidirectional_pair(KC_END, KC_HOME);

    /* One-way/default/disabled/unmatched behavior is delegated to stock QMK/Vial. */
    expect_passthrough(KC_N, 0, QK_REPEAT_KEY);
    expect_passthrough(KC_N, 0, QK_ALT_REPEAT_KEY);
    expect_passthrough(KC_A, 0, QK_REPEAT_KEY);
    expect_passthrough(KC_G, 0, QK_ALT_REPEAT_KEY);

    expect_vial_precedence();
    expect_live_edit_coherence();
    expect_press_release_latching();

    /* Matrix Repeat behavior is outside the encoder-only policy. */
    keyrecord_t matrix = matrix_event(true);
    const unsigned before_resolve = policy_resolver_calls;
    assert(pre_process_record_user(QK_REPEAT_KEY, &matrix));
    assert(pre_process_record_user(QK_ALT_REPEAT_KEY, &matrix));
    assert(policy_resolver_calls == before_resolve);

    /* Invalid encoder indices are never used as latch-array indices. */
    keyrecord_t invalid = encoder_event(NUM_ENCODERS, true);
    assert(pre_process_record_user(QK_REPEAT_KEY, &invalid));
    invalid.event.pressed = false;
    assert(pre_process_record_user(QK_REPEAT_KEY, &invalid));

    for (unsigned i = 0; i < repeat_events; i += 2) {
        assert(repeat_pressed[i]);
        assert(!repeat_pressed[i + 1]);
    }
    for (unsigned i = 0; i < alt_events; i += 2) {
        assert(alt_pressed[i]);
        assert(!alt_pressed[i + 1]);
    }

    assert(nvm_reads == 0);
    puts("encoder repeat direction: native Vial orientation, RAM coherence, precedence, fallback, and latching pass.");
}
'''

with tempfile.TemporaryDirectory(prefix="halcyon-encoder-repeat-") as tmp:
    c_path = Path(tmp) / "encoder_repeat.c"
    exe_path = Path(tmp) / "encoder_repeat"
    c_path.write_text(harness)
    subprocess.run(
        [
            "cc",
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            str(c_path),
            "-o",
            str(exe_path),
        ],
        check=True,
    )
    subprocess.run([str(exe_path)], check=True)
