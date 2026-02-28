// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

enum layers {
    L0 = 0,
    L1 = 1,
    L2 = 2,
    L3 = 3,
    L4 = 4,
    L5 = 5,
    L6 = 6,
    L7 = 7,
    L8 = 8,
    L9 = 9,
    L10 = 10,
    L11 = 11,
    L12 = 12,
};

enum custom_keycodes {
    MC_0 = SAFE_RANGE,
    MC_1,
    MC_2,
    MC_3,
    MC_4,
    MC_5,
    MC_6,
    MC_7,
    MC_8,
    MC_9
};

static inline bool is_macro_keycode(uint16_t keycode) {
    return keycode >= MC_0 && keycode <= MC_9;
}

static void run_macro_slot(uint8_t slot);

static void tap_action_keycode(uint16_t keycode) {
    if (keycode == KC_NO || keycode == KC_TRNS) {
        return;
    }

    if (is_macro_keycode(keycode)) {
        run_macro_slot((uint8_t)(keycode - MC_0));
        return;
    }

    if (keycode >= QK_ONE_SHOT_MOD && keycode <= QK_ONE_SHOT_MOD_MAX) {
        add_oneshot_mods(QK_ONE_SHOT_MOD_GET_MODS(keycode));
        return;
    }

    if (keycode >= QK_MOMENTARY && keycode <= QK_MOMENTARY_MAX) {
        const uint8_t layer = QK_MOMENTARY_GET_LAYER(keycode);
        layer_on(layer);
        layer_off(layer);
        return;
    }

    if (keycode >= QK_TOGGLE_LAYER && keycode <= QK_TOGGLE_LAYER_MAX) {
        layer_invert(QK_TOGGLE_LAYER_GET_LAYER(keycode));
        return;
    }

    tap_code16(keycode);
}

static void press_action_keycode(uint16_t keycode) {
    if (keycode == KC_NO || keycode == KC_TRNS) {
        return;
    }

    if (is_macro_keycode(keycode)) {
        run_macro_slot((uint8_t)(keycode - MC_0));
        return;
    }

    if (keycode >= QK_ONE_SHOT_MOD && keycode <= QK_ONE_SHOT_MOD_MAX) {
        add_oneshot_mods(QK_ONE_SHOT_MOD_GET_MODS(keycode));
        return;
    }

    if (keycode >= QK_MOMENTARY && keycode <= QK_MOMENTARY_MAX) {
        layer_on(QK_MOMENTARY_GET_LAYER(keycode));
        return;
    }

    register_code16(keycode);
}

static void release_action_keycode(uint16_t keycode) {
    if (keycode == KC_NO || keycode == KC_TRNS || is_macro_keycode(keycode)) {
        return;
    }

    if (keycode >= QK_ONE_SHOT_MOD && keycode <= QK_ONE_SHOT_MOD_MAX) {
        return;
    }

    if (keycode >= QK_MOMENTARY && keycode <= QK_MOMENTARY_MAX) {
        layer_off(QK_MOMENTARY_GET_LAYER(keycode));
        return;
    }

    unregister_code16(keycode);
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [L0] = LAYOUT_ferris_hlc(
                       MO(8),                MO(4),                MO(5),                MO(9),               MO(10),                KC_NO,                KC_NO,                KC_NO,                KC_NO,                KC_NO,              KC_LALT,
                     KC_LSFT,              KC_LGUI,              KC_LCTL,              RM_TOGG,              MS_LEFT,              MS_DOWN,              MS_UP,              MS_RGHT,                KC_NO,           LGUI(KC_Z),           LGUI(KC_X),
                  LGUI(KC_C),           LGUI(KC_V),           SGUI(KC_Z),                KC_NO,              MS_BTN3,              MS_BTN4,              MS_BTN5,                KC_NO,                MO(6),       LT(3, KC_BSPC),              MS_BTN2,
                     MS_BTN1,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,                TO(1),                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L1] = LAYOUT_ferris_hlc(
                        KC_Q,                 KC_W,                 KC_E,                 KC_R,                 KC_T,                 KC_Y,                 KC_U,                 KC_I,                 KC_O,                 KC_P,                 KC_A,
                        KC_S,                 KC_D,                 KC_F,                 KC_G,                 KC_H,                 KC_J,                 KC_K,                 KC_L,                TD(1),                 KC_Z,                 KC_X,
                        KC_C,                 KC_V,                 KC_B,                 KC_N,                 KC_M,                TD(7),                TD(3),                TD(0),                MO(6),       LT(3, KC_BSPC),       LT(3, KC_BSPC),
                    KC_SPACE,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L2] = LAYOUT_ferris_hlc(
                        KC_F,                 KC_R,                 KC_D,                 KC_P,                 KC_V,                 KC_Q,                 KC_J,                 KC_U,                 KC_O,                 KC_Y,                 KC_S,
                        KC_N,                 KC_T,                 KC_C,                 KC_B,                TD(3),                 KC_H,                 KC_E,                 KC_A,                 KC_I,                 KC_Z,                 KC_X,
                        KC_K,                 KC_G,                 KC_W,                 KC_M,                 KC_L,                TD(4),                TD(7),                TD(2),                MO(6),       LT(3, KC_BSPC),       LT(3, KC_BSPC),
                    KC_SPACE,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L3] = LAYOUT_ferris_hlc(
                        KC_1,                 KC_2,                 KC_3,                 KC_4,                 KC_5,                 KC_6,                 KC_7,                 KC_8,                 KC_9,                 KC_0,               KC_GRV,
                     KC_BSLS,              KC_PAST,              KC_PPLS,                KC_NO,              KC_PCMM,                 KC_4,                 KC_5,                 KC_6,              KC_MINS,                KC_NO,                KC_NO,
                     KC_PSLS,              KC_PMNS,                KC_NO,              KC_PDOT,                 KC_1,                 KC_2,                 KC_3,               KC_EQL,              KC_PGUP,              KC_HOME,               KC_END,
                     KC_PGDN,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,        QK_LAYER_LOCK,                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L4] = LAYOUT_ferris_hlc(
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L5] = LAYOUT_ferris_hlc(
                     _______,              _______,              _______,              _______,              _______,              _______,         OSM(MOD_MEH), OSM(MOD_LSFT|MOD_LALT),  QK_CAPS_WORD_TOGGLE, OSM(MOD_LSFT|MOD_LGUI),              _______,
                     _______,              _______,              _______,              _______,              _______,        OSM(MOD_RCTL),        OSM(MOD_RGUI),        OSM(MOD_RSFT),        OSM(MOD_RALT),              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,        LALT(KC_BSPC),           LGUI(KC_S),              _______,
                     _______,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,              _______,                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L6] = LAYOUT_ferris_hlc(
        OSM(MOD_LSFT|MOD_LGUI),  QK_CAPS_WORD_TOGGLE, OSM(MOD_LSFT|MOD_LALT),         OSM(MOD_MEH),           LGUI(KC_A),              _______,              _______,              _______,              _______,              _______,        OSM(MOD_LALT),
               OSM(MOD_LSFT),        OSM(MOD_LGUI),        OSM(MOD_LCTL),              _______,              _______,              _______,              _______,              _______,              _______,           LGUI(KC_Z),           LGUI(KC_X),
                  LGUI(KC_C),           LGUI(KC_V),           SGUI(KC_Z),              _______,              _______,              _______,              _______,              _______,              _______,       LGUI(KC_SPACE),               KC_TAB,
                       MO(5),              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L7] = LAYOUT_ferris_hlc(
                       KC_F1,                KC_F2,                KC_F3,                KC_F4,                KC_F5,                KC_F8,                KC_F9,               KC_F10,               KC_F11,               KC_F12,              KC_PSLS,
               LSFT(KC_RBRC),         LSFT(KC_DOT),              KC_RBRC,           LSFT(KC_0),           LSFT(KC_9),              KC_LBRC,        LSFT(KC_COMM),        LSFT(KC_LBRC),              KC_BSLS,              KC_MPRV,              KC_MSTP,
                     KC_MPLY,              KC_MNXT,                KC_F6,                KC_F7,              KC_MNXT,              KC_MPLY,              KC_MSTP,              KC_MPRV,              KC_LEFT,                KC_UP,              KC_DOWN,
                    KC_RIGHT,              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO,           LGUI(KC_0),                KC_NO,                KC_NO,                KC_NO,                KC_NO,
    ),
    [L8] = LAYOUT_ferris_hlc(
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
    ),
    [L9] = LAYOUT_ferris_hlc(
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
    ),
    [L10] = LAYOUT_ferris_hlc(
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
    ),
    [L11] = LAYOUT_ferris_hlc(
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
    ),
    [L12] = LAYOUT_ferris_hlc(
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
                     _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,              _______,
    ),
};

#if defined(ENCODER_MAP_ENABLE)
// finalVial.vil stores two module encoder slots (left/right). This build has one physical encoder
// on the right module, so we map only the right slot into NUM_ENCODERS=1.
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [L0] = { ENCODER_CCW_CW(MS_WHLU, MS_WHLD) },
    [L1] = { ENCODER_CCW_CW(MS_WHLU, MS_WHLD) },
    [L2] = { ENCODER_CCW_CW(MS_WHLU, MS_WHLD) },
    [L3] = { ENCODER_CCW_CW(QK_ALT_REPEAT_KEY, QK_REPEAT_KEY) },
    [L4] = { ENCODER_CCW_CW(RM_SATD, RM_SATU) },
    [L5] = { ENCODER_CCW_CW(RM_VALD, RM_VALU) },
    [L6] = { ENCODER_CCW_CW(KC_BRID, KC_BRIU) },
    [L7] = { ENCODER_CCW_CW(LGUI(KC_PMNS), LGUI(KC_PPLS)) },
    [L8] = { ENCODER_CCW_CW(RM_SPDD, RM_SPDU) },
    [L9] = { ENCODER_CCW_CW(RM_PREV, RM_NEXT) },
    [L10] = { ENCODER_CCW_CW(RM_HUED, RM_HUEU) },
    [L11] = { ENCODER_CCW_CW(_______, _______) },
    [L12] = { ENCODER_CCW_CW(_______, _______) },
};
#endif

#if defined(COMBO_ENABLE) && !defined(VIAL_COMBO_ENTRIES)
enum combo_events {
    CM_0,
    CM_1,
    CM_2,
    CM_3,
    CM_4,
    CM_5,
    CM_6,
    CM_7,
    CM_8,
    CM_9,
    CM_10,
    CM_11,
    CM_12,
    CM_13,
    CM_14,
    CM_15,
    CM_16,
    CM_17
};

static const uint16_t PROGMEM combo_0[] = { KC_F, KC_D, COMBO_END };
static const uint16_t PROGMEM combo_1[] = { KC_J, KC_K, COMBO_END };
static const uint16_t PROGMEM combo_2[] = { KC_S, KC_D, COMBO_END };
static const uint16_t PROGMEM combo_3[] = { KC_K, KC_L, COMBO_END };
static const uint16_t PROGMEM combo_4[] = { KC_J, KC_L, COMBO_END };
static const uint16_t PROGMEM combo_5[] = { KC_S, KC_F, COMBO_END };
static const uint16_t PROGMEM combo_6[] = { KC_DOWN, KC_RIGHT, COMBO_END };
static const uint16_t PROGMEM combo_7[] = { MS_RGHT, MS_UP, COMBO_END };
static const uint16_t PROGMEM combo_8[] = { MS_RGHT, MS_DOWN, COMBO_END };
static const uint16_t PROGMEM combo_9[] = { KC_Y, KC_N, COMBO_END };
static const uint16_t PROGMEM combo_10[] = { KC_T, KC_B, COMBO_END };
static const uint16_t PROGMEM combo_11[] = { KC_U, KC_M, COMBO_END };
static const uint16_t PROGMEM combo_12[] = { KC_R, KC_V, COMBO_END };
static const uint16_t PROGMEM combo_13[] = { KC_I, KC_COMM, COMBO_END };
static const uint16_t PROGMEM combo_14[] = { KC_E, KC_C, COMBO_END };
static const uint16_t PROGMEM combo_15[] = { KC_UP, KC_RIGHT, COMBO_END };
static const uint16_t PROGMEM combo_16[] = { KC_LALT, KC_LGUI, COMBO_END };
static const uint16_t PROGMEM combo_17[] = { KC_W, KC_R, COMBO_END };

combo_t key_combos[] = {
    [CM_0] = COMBO(combo_0, MO(7)),
    [CM_1] = COMBO(combo_1, MO(7)),
    [CM_2] = COMBO(combo_2, KC_ESC),
    [CM_3] = COMBO(combo_3, KC_ENT),
    [CM_4] = COMBO(combo_4, MC_2),
    [CM_5] = COMBO(combo_5, HYPR(KC_SPACE)),
    [CM_6] = COMBO(combo_6, MC_2),
    [CM_7] = COMBO(combo_7, KC_ENT),
    [CM_8] = COMBO(combo_8, MC_2),
    [CM_9] = COMBO(combo_9, QK_REBOOT),
    [CM_10] = COMBO(combo_10, QK_REBOOT),
    [CM_11] = COMBO(combo_11, QK_BOOT),
    [CM_12] = COMBO(combo_12, QK_BOOT),
    [CM_13] = COMBO(combo_13, QK_CLEAR_EEPROM),
    [CM_14] = COMBO(combo_14, QK_CLEAR_EEPROM),
    [CM_15] = COMBO(combo_15, KC_ENT),
    [CM_16] = COMBO(combo_16, TD(13)),
    [CM_17] = COMBO(combo_17, RCTL(KC_B)),
};
#endif

#if defined(TAP_DANCE_ENABLE) && !defined(VIAL_TAP_DANCE_ENTRIES)
typedef struct {
    uint16_t on_tap;
    uint16_t on_hold;
    uint16_t on_double_tap;
    uint16_t on_tap_hold;
    uint16_t custom_tapping_term;
} vial_like_tap_dance_entry_t;

#define TAP_DANCE_SLOT_COUNT 14

static const vial_like_tap_dance_entry_t tap_dance_entries[TAP_DANCE_SLOT_COUNT] = {
    [0] = { KC_PSLS, LSFT(KC_SLASH), MC_9, KC_PSLS, 160 },
    [1] = { LSFT(KC_SCLN), KC_QUOT, KC_MINS, KC_QUOT, 160 },
    [2] = { KC_COMM, LSFT(KC_SLASH), KC_SLASH, LSFT(KC_SLASH), 160 },
    [3] = { KC_DOT, KC_DOT, MC_3, KC_DOT, 180 },
    [4] = { KC_N, MC_6, MC_7, MC_6, 175 },
    [5] = { OSM(MOD_LSFT), MO(6), MC_5, MO(6), 45 },
    [6] = { KC_Q, LSFT(KC_1), KC_GRV, KC_Q, 180 },
    [7] = { KC_COMM, KC_SCLN, KC_COMM, KC_SCLN, 160 },
    [8] = { KC_O, KC_MINS, KC_O, KC_O, 200 },
    [13] = { KC_ESC, KC_GRV, MC_0, KC_NO, 220 },
};

enum {
    TD_SINGLE_TAP = 1,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP,
    TD_DOUBLE_HOLD,
    TD_DOUBLE_SINGLE_TAP,
    TD_MORE_TAPS
};

static uint8_t tap_dance_state_cache[TAP_DANCE_SLOT_COUNT];

static uint8_t td_step(tap_dance_state_t *state) {
    if (state->count == 1) {
        return (state->interrupted || !state->pressed) ? TD_SINGLE_TAP : TD_SINGLE_HOLD;
    }

    if (state->count == 2) {
        if (state->interrupted) {
            return TD_DOUBLE_SINGLE_TAP;
        }
        return state->pressed ? TD_DOUBLE_HOLD : TD_DOUBLE_TAP;
    }

    return TD_MORE_TAPS;
}

static void on_dance(tap_dance_state_t *state, void *user_data) {
    const uint8_t idx = (uint8_t)(uintptr_t)user_data;
    const vial_like_tap_dance_entry_t td = tap_dance_entries[idx];

    if (td.on_tap == KC_NO) {
        return;
    }

    if (state->count == 3) {
        tap_action_keycode(td.on_tap);
        tap_action_keycode(td.on_tap);
        tap_action_keycode(td.on_tap);
    } else if (state->count > 3) {
        tap_action_keycode(td.on_tap);
    }
}

static void on_dance_finished(tap_dance_state_t *state, void *user_data) {
    const uint8_t idx = (uint8_t)(uintptr_t)user_data;
    const vial_like_tap_dance_entry_t td = tap_dance_entries[idx];

    tap_dance_state_cache[idx] = td_step(state);

    switch (tap_dance_state_cache[idx]) {
        case TD_SINGLE_TAP:
            press_action_keycode(td.on_tap);
            break;
        case TD_SINGLE_HOLD:
            press_action_keycode(td.on_hold != KC_NO ? td.on_hold : td.on_tap);
            break;
        case TD_DOUBLE_TAP:
            if (td.on_double_tap != KC_NO) {
                press_action_keycode(td.on_double_tap);
            } else {
                tap_action_keycode(td.on_tap);
                press_action_keycode(td.on_tap);
            }
            break;
        case TD_DOUBLE_HOLD:
            if (td.on_tap_hold != KC_NO) {
                press_action_keycode(td.on_tap_hold);
            } else if (td.on_tap != KC_NO) {
                tap_action_keycode(td.on_tap);
                press_action_keycode(td.on_hold != KC_NO ? td.on_hold : td.on_tap);
            } else {
                press_action_keycode(td.on_hold);
            }
            break;
        case TD_DOUBLE_SINGLE_TAP:
            tap_action_keycode(td.on_tap);
            press_action_keycode(td.on_tap);
            break;
        default:
            break;
    }
}

static void on_dance_reset(tap_dance_state_t *state, void *user_data) {
    const uint8_t idx = (uint8_t)(uintptr_t)user_data;
    const vial_like_tap_dance_entry_t td = tap_dance_entries[idx];
    const uint8_t dance_state = tap_dance_state_cache[idx];

    state->count = 0;
    tap_dance_state_cache[idx] = 0;

    switch (dance_state) {
        case TD_SINGLE_TAP:
            release_action_keycode(td.on_tap);
            break;
        case TD_SINGLE_HOLD:
            release_action_keycode(td.on_hold != KC_NO ? td.on_hold : td.on_tap);
            break;
        case TD_DOUBLE_TAP:
            release_action_keycode(td.on_double_tap != KC_NO ? td.on_double_tap : td.on_tap);
            break;
        case TD_DOUBLE_HOLD:
            if (td.on_tap_hold != KC_NO) {
                release_action_keycode(td.on_tap_hold);
            } else if (td.on_tap != KC_NO) {
                release_action_keycode(td.on_hold != KC_NO ? td.on_hold : td.on_tap);
            } else {
                release_action_keycode(td.on_hold);
            }
            break;
        case TD_DOUBLE_SINGLE_TAP:
            release_action_keycode(td.on_tap);
            break;
        default:
            break;
    }
}

tap_dance_action_t tap_dance_actions[] = {
    [0] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)0 },
    [1] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)1 },
    [2] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)2 },
    [3] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)3 },
    [4] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)4 },
    [5] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)5 },
    [6] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)6 },
    [7] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)7 },
    [8] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)8 },
    [13] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)13 },
};

uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    if (keycode >= QK_TAP_DANCE && keycode <= QK_TAP_DANCE_MAX) {
        const uint8_t idx = QK_TAP_DANCE_GET_INDEX(keycode);
        if (idx < TAP_DANCE_SLOT_COUNT && tap_dance_entries[idx].custom_tapping_term > 0) {
            return tap_dance_entries[idx].custom_tapping_term;
        }
    }

    return TAPPING_TERM;
}
#endif

#if defined(KEY_OVERRIDE_ENABLE) && !defined(VIAL_KEY_OVERRIDE_ENTRIES)
#define KO_COMMON_OPTIONS (ko_option_activation_trigger_down | ko_option_activation_required_mod_down | ko_option_activation_negative_mod_up)

static const key_override_t ko_0 = { .trigger = MS_WHLU, .trigger_mods = 68, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 68, .replacement = LSFT(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_1 = { .trigger = MS_WHLD, .trigger_mods = 68, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 68, .replacement = KC_TAB, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_2 = { .trigger = MS_WHLD, .trigger_mods = 17, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = LCTL(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_3 = { .trigger = MS_WHLU, .trigger_mods = 17, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = C_S(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_4 = { .trigger = MS_WHLU, .trigger_mods = 136, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = SGUI(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_5 = { .trigger = MS_WHLD, .trigger_mods = 136, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = LGUI(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_6 = { .trigger = MS_WHLD, .trigger_mods = 34, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 34, .replacement = KC_VOLU, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_7 = { .trigger = MS_WHLU, .trigger_mods = 34, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 34, .replacement = KC_VOLD, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_8 = { .trigger = KC_BRID, .trigger_mods = 136, .layers = (layer_state_t)64, .negative_mod_mask = 119, .suppressed_mods = 119, .replacement = LSFT(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_9 = { .trigger = KC_BRIU, .trigger_mods = 136, .layers = (layer_state_t)64, .negative_mod_mask = 119, .suppressed_mods = 119, .replacement = KC_TAB, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_10 = { .trigger = KC_BRID, .trigger_mods = 34, .layers = (layer_state_t)64, .negative_mod_mask = 221, .suppressed_mods = 255, .replacement = KC_VOLD, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_11 = { .trigger = KC_BRIU, .trigger_mods = 34, .layers = (layer_state_t)64, .negative_mod_mask = 221, .suppressed_mods = 255, .replacement = KC_VOLU, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_12 = { .trigger = KC_BRIU, .trigger_mods = 68, .layers = (layer_state_t)64, .negative_mod_mask = 187, .suppressed_mods = 238, .replacement = LCTL(KC_N), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_13 = { .trigger = KC_BRID, .trigger_mods = 68, .layers = (layer_state_t)64, .negative_mod_mask = 187, .suppressed_mods = 238, .replacement = LCTL(KC_P), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_14 = { .trigger = KC_BRIU, .trigger_mods = 17, .layers = (layer_state_t)64, .negative_mod_mask = 238, .suppressed_mods = 255, .replacement = KC_TAB, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_15 = { .trigger = KC_BRID, .trigger_mods = 17, .layers = (layer_state_t)64, .negative_mod_mask = 238, .suppressed_mods = 255, .replacement = LSFT(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_17 = { .trigger = KC_BRIU, .trigger_mods = 102, .layers = (layer_state_t)64, .negative_mod_mask = 153, .suppressed_mods = 255, .replacement = LCTL(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_18 = { .trigger = KC_BRID, .trigger_mods = 102, .layers = (layer_state_t)64, .negative_mod_mask = 153, .suppressed_mods = 255, .replacement = C_S(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };

const key_override_t *key_overrides[] = {
    &ko_0,
    &ko_1,
    &ko_2,
    &ko_3,
    &ko_4,
    &ko_5,
    &ko_6,
    &ko_7,
    &ko_8,
    &ko_9,
    &ko_10,
    &ko_11,
    &ko_12,
    &ko_13,
    &ko_14,
    &ko_15,
    &ko_17,
    &ko_18,
};
#endif

#if defined(REPEAT_KEY_ENABLE) && !defined(VIAL_ALT_REPEAT_KEY_ENTRIES)
enum {
    AREP_OPTION_DEFAULT_TO_ALT = (1 << 0),
    AREP_OPTION_BIDIRECTIONAL = (1 << 1),
    AREP_OPTION_IGNORE_HANDEDNESS = (1 << 2),
    AREP_OPTION_ENABLED = (1 << 3),
};

typedef struct {
    uint16_t keycode;
    uint16_t alt_keycode;
    uint8_t allowed_mods;
    uint8_t options;
} alt_repeat_raw_entry_t;

static const alt_repeat_raw_entry_t alt_repeat_entries[] = {
    { KC_N, LSFT(KC_N), 0, 12 },
    { LCTL(KC_D), LCTL(KC_U), 0, 12 },
    { KC_W, KC_B, 0, 12 },
    { KC_J, KC_K, 68, 12 },
    { KC_L, KC_H, 68, 12 },
    { KC_TAB, LSFT(KC_TAB), 17, 12 },
    { LGUI(KC_G), SGUI(KC_G), 0, 12 },
    { KC_U, LSFT(KC_U), 0, 14 },
    { LSFT(KC_DOT), LSFT(KC_COMM), 0, 12 },
    { KC_RBRC, KC_LBRC, 119, 12 },
    { LCTL(KC_A), LCTL(KC_X), 17, 12 },
    { KC_BSPC, KC_DEL, 103, 12 },
};

static uint8_t unpack_mods5(uint8_t mods5) {
    return (mods5 & 0x10U) != 0 ? (mods5 << 4) : mods5;
}

static uint8_t popcount8(uint8_t value) {
    uint8_t count = 0;
    while (value != 0U) {
        count += value & 1U;
        value >>= 1U;
    }
    return count;
}

static uint16_t normalize_alt_repeat_keycode(uint16_t keycode, uint8_t *mods) {
    switch (keycode) {
        case QK_MODS ... QK_MODS_MAX:
            *mods |= unpack_mods5(QK_MODS_GET_MODS(keycode));
            return QK_MODS_GET_BASIC_KEYCODE(keycode);
        case QK_MOD_TAP ... QK_MOD_TAP_MAX:
            return QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
        case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
            return QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
        default:
            return keycode;
    }
}

static bool alt_repeat_mods_match(uint8_t mods, uint8_t required_mods, uint8_t allowed_mods, uint8_t options) {
    allowed_mods |= required_mods;

    if ((options & AREP_OPTION_IGNORE_HANDEDNESS) != 0U) {
        mods = (mods & 0x0FU) | (mods >> 4);
        required_mods = (required_mods & 0x0FU) | (required_mods >> 4);
        allowed_mods = (allowed_mods & 0x0FU) | (allowed_mods >> 4);
    }

    return (mods & required_mods) == required_mods && (mods & (uint8_t)~allowed_mods) == 0;
}

uint16_t get_alt_repeat_key_keycode_user(uint16_t keycode, uint8_t mods) {
    uint16_t alt_keycode = KC_TRNS;
    int8_t best_fit = -1;

    keycode = normalize_alt_repeat_keycode(keycode, &mods);

    for (uint8_t i = 0; i < ARRAY_SIZE(alt_repeat_entries); ++i) {
        const alt_repeat_raw_entry_t *entry = &alt_repeat_entries[i];
        const uint8_t options = entry->options;

        if ((options & AREP_OPTION_ENABLED) == 0U) {
            continue;
        }

        uint8_t required_mods = 0;
        uint8_t alt_required_mods = 0;
        const uint16_t normalized_keycode = normalize_alt_repeat_keycode(entry->keycode, &required_mods);
        const uint16_t normalized_alt_keycode = normalize_alt_repeat_keycode(entry->alt_keycode, &alt_required_mods);

        if (normalized_keycode == keycode && alt_repeat_mods_match(mods, required_mods, entry->allowed_mods, options)) {
            const int8_t fit = (int8_t)popcount8(required_mods);
            if (fit > best_fit) {
                alt_keycode = (uint16_t)(((uint16_t)alt_required_mods << 8) | normalized_alt_keycode);
                best_fit = fit;
            }
        }

        if (normalized_alt_keycode == keycode && (options & AREP_OPTION_BIDIRECTIONAL) != 0U && alt_repeat_mods_match(mods, alt_required_mods, entry->allowed_mods, options)) {
            const int8_t fit = (int8_t)popcount8(alt_required_mods);
            if (fit > best_fit) {
                alt_keycode = (uint16_t)(((uint16_t)required_mods << 8) | normalized_keycode);
                best_fit = fit;
            }
        }

        if ((options & AREP_OPTION_DEFAULT_TO_ALT) != 0U && best_fit == -1 && alt_keycode == KC_TRNS && alt_repeat_mods_match(mods, 0, entry->allowed_mods, options)) {
            alt_keycode = (uint16_t)(((uint16_t)alt_required_mods << 8) | normalized_alt_keycode);
        }
    }

    return alt_keycode;
}
#endif

static void run_macro_slot(uint8_t slot) {
    switch (slot) {
        case 0:
            tap_action_keycode(KC_ESC);
            tap_action_keycode(TG(4));
            break;
        case 1:
            tap_action_keycode(KC_SPACE);
            tap_action_keycode(OSM(MOD_RSFT));
            break;
        case 2:
            tap_action_keycode(LCTL(KC_QUOT));
            break;
        case 3:
            SEND_STRING("...");
            break;
        case 4:
            SEND_STRING(".  ");
            tap_action_keycode(OSM(MOD_RSFT));
            break;
        case 5:
            tap_action_keycode(KC_BSPC);
            tap_action_keycode(KC_BSPC);
            tap_action_keycode(LALT(KC_BSPC));
            break;
        case 6:
            tap_action_keycode(LALT(KC_N));
            tap_action_keycode(KC_N);
            break;
        case 7:
            SEND_STRING("nn");
            break;
        case 8:
            tap_action_keycode(TG(1));
            tap_action_keycode(TG(4));
            break;
        case 9:
            tap_action_keycode(KC_SLASH);
            tap_action_keycode(KC_SLASH);
            break;
        default:
            break;
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    if (is_macro_keycode(keycode)) {
        run_macro_slot((uint8_t)(keycode - MC_0));
        return false;
    }

    return true;
}

const char *halcyon_display_layer_name_user(uint8_t layer) {
    static const char *const layer_names[] = {
        "L0",
        "L1",
        "L2",
        "L3",
        "L4",
        "L5",
        "L6",
        "L7",
        "L8",
        "L9",
        "L10",
        "L11",
        "L12",
    };

    if (layer < ARRAY_SIZE(layer_names)) {
        return layer_names[layer];
    }

    return "L?";
}
