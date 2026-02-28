// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "dynamic_keymap.h"
#include <stdio.h>
#ifdef RGB_MATRIX_ENABLE
#include "rgb_matrix.h"
#endif

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
    MC_9,
    RGBL0, RGBL1, RGBL2, RGBL3, RGBL4, RGBL5, RGBL6, RGBL7,
    RGBM0, RGBM1, RGBM2, RGBM3, RGBM4, RGBM5, RGBM6, RGBM7,
    RGBC0, RGBC1, RGBC2, RGBC3, RGBC4, RGBC5, RGBC6, RGBC7,
    RGBDU, RGBDD
};

/*
 * When this marker changes, firmware will re-seed Vial's dynamic keymap from the
 * compiled keymap once on boot. This guarantees shipped defaults are applied while
 * still allowing later Vial edits to persist.
 */
#define XTREEMZE_DEFAULTS_EE_MARKER 0x58544603UL

static void sync_compiled_defaults_to_dynamic_keymap_once(void) {
    if (eeconfig_read_user() == XTREEMZE_DEFAULTS_EE_MARKER) {
        return;
    }

    dynamic_keymap_reset();
    eeconfig_update_user(XTREEMZE_DEFAULTS_EE_MARKER);
}

static char alt_repeat_display_text[24] = "---";

#ifdef RGB_MATRIX_ENABLE
enum {
    RGB_PRESET_NONE = 0xFF,
    RGB_PRESET_0 = 0,
    RGB_PRESET_1,
    RGB_PRESET_2,
    RGB_PRESET_3,
    RGB_PRESET_4,
    RGB_PRESET_5,
    RGB_PRESET_6,
    RGB_PRESET_7
};

typedef struct {
    uint8_t mode;
    uint8_t h;
    uint8_t s;
    uint8_t v;
    uint8_t speed;
} rgb_preset_t;

static const rgb_preset_t rgb_presets[8] = {
    [RGB_PRESET_0] = { RGB_MATRIX_SOLID_COLOR, 59, 85, 192, 80 },
    [RGB_PRESET_1] = { RGB_MATRIX_SOLID_COLOR, 122, 82, 187, 80 },
    [RGB_PRESET_2] = { RGB_MATRIX_SOLID_COLOR, 28, 107, 219, 80 },
    [RGB_PRESET_3] = { RGB_MATRIX_SOLID_COLOR, 254, 115, 230, 80 },
    [RGB_PRESET_4] = { RGB_MATRIX_BREATHING, 59, 85, 192, 100 },
    [RGB_PRESET_5] = { RGB_MATRIX_CYCLE_ALL, 122, 82, 187, 110 },
    [RGB_PRESET_6] = { RGB_MATRIX_CYCLE_LEFT_RIGHT, 28, 107, 219, 105 },
    [RGB_PRESET_7] = { RGB_MATRIX_RAINBOW_MOVING_CHEVRON, 254, 115, 230, 120 },
};

static uint8_t layer_rgb_presets[13] = {
    RGB_PRESET_0, RGB_PRESET_1, RGB_PRESET_2, RGB_PRESET_3, RGB_PRESET_0, RGB_PRESET_1, RGB_PRESET_2,
    RGB_PRESET_3, RGB_PRESET_4, RGB_PRESET_5, RGB_PRESET_6, RGB_PRESET_7, RGB_PRESET_0,
};

static uint8_t mod_rgb_presets[4] = {
    RGB_PRESET_NONE, RGB_PRESET_NONE, RGB_PRESET_NONE, RGB_PRESET_NONE
}; // Ctrl, Gui, Shift, Alt

static uint8_t chord_override_preset = RGB_PRESET_NONE;
static uint32_t chord_override_start = 0;
static uint16_t chord_override_ms = 1500;
static uint8_t last_applied_rgb_preset = RGB_PRESET_NONE;

static void apply_rgb_preset(uint8_t preset) {
    if (preset >= ARRAY_SIZE(rgb_presets)) {
        return;
    }

    const rgb_preset_t *p = &rgb_presets[preset];
    rgb_matrix_mode_noeeprom(p->mode);
    rgb_matrix_sethsv_noeeprom(p->h, p->s, p->v);
    rgb_matrix_set_speed_noeeprom(p->speed);
}

static void refresh_rgb_preset_state(void) {
    if (chord_override_preset != RGB_PRESET_NONE && timer_elapsed32(chord_override_start) > chord_override_ms) {
        chord_override_preset = RGB_PRESET_NONE;
    }

    uint8_t target = RGB_PRESET_NONE;
    if (chord_override_preset != RGB_PRESET_NONE) {
        target = chord_override_preset;
    } else {
        const uint8_t mods = get_mods() | get_oneshot_mods();
        if ((mods & MOD_MASK_CTRL) != 0U && mod_rgb_presets[0] != RGB_PRESET_NONE) {
            target = mod_rgb_presets[0];
        } else if ((mods & MOD_MASK_GUI) != 0U && mod_rgb_presets[1] != RGB_PRESET_NONE) {
            target = mod_rgb_presets[1];
        } else if ((mods & MOD_MASK_SHIFT) != 0U && mod_rgb_presets[2] != RGB_PRESET_NONE) {
            target = mod_rgb_presets[2];
        } else if ((mods & MOD_MASK_ALT) != 0U && mod_rgb_presets[3] != RGB_PRESET_NONE) {
            target = mod_rgb_presets[3];
        } else {
            const uint8_t layer = get_highest_layer(layer_state | default_layer_state);
            if (layer < ARRAY_SIZE(layer_rgb_presets)) {
                target = layer_rgb_presets[layer];
            }
        }
    }

    if (target != RGB_PRESET_NONE && target != last_applied_rgb_preset) {
        apply_rgb_preset(target);
        last_applied_rgb_preset = target;
    }
}

static void set_layer_preset_from_keycode(uint16_t keycode) {
    const uint8_t preset = (uint8_t)(keycode - RGBL0);
    const uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer < ARRAY_SIZE(layer_rgb_presets) && preset < 8) {
        layer_rgb_presets[layer] = preset;
    }
}

static void set_mod_preset_from_keycode(uint16_t keycode) {
    const uint8_t preset = (uint8_t)(keycode - RGBM0);
    const uint8_t mods = get_mods() | get_oneshot_mods();
    if (preset >= 8) {
        return;
    }

    if ((mods & MOD_MASK_CTRL) != 0U) {
        mod_rgb_presets[0] = preset;
    }
    if ((mods & MOD_MASK_GUI) != 0U) {
        mod_rgb_presets[1] = preset;
    }
    if ((mods & MOD_MASK_SHIFT) != 0U) {
        mod_rgb_presets[2] = preset;
    }
    if ((mods & MOD_MASK_ALT) != 0U) {
        mod_rgb_presets[3] = preset;
    }
}

static void trigger_chord_preset_from_keycode(uint16_t keycode) {
    const uint8_t preset = (uint8_t)(keycode - RGBC0);
    if (preset < 8) {
        chord_override_preset = preset;
        chord_override_start = timer_read32();
    }
}
#endif

#if defined(REPEAT_KEY_ENABLE) && !defined(VIAL_ALT_REPEAT_KEY_ENTRIES)
uint16_t get_alt_repeat_key_keycode_user(uint16_t keycode, uint8_t mods);
#endif

static void format_basic_keycode_name(uint8_t keycode, char *out, size_t out_size) {
    if (keycode >= KC_A && keycode <= KC_Z) {
        out[0] = 'A' + (char)(keycode - KC_A);
        out[1] = '\0';
        return;
    }

    if (keycode >= KC_1 && keycode <= KC_9) {
        out[0] = '1' + (char)(keycode - KC_1);
        out[1] = '\0';
        return;
    }

    if (keycode == KC_0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    switch (keycode) {
        case KC_TAB:  snprintf(out, out_size, "Tab"); break;
        case KC_ESC:  snprintf(out, out_size, "Esc"); break;
        case KC_ENT:  snprintf(out, out_size, "Ent"); break;
        case KC_SPC:  snprintf(out, out_size, "Spc"); break;
        case KC_BSPC: snprintf(out, out_size, "Bsp"); break;
        case KC_DEL:  snprintf(out, out_size, "Del"); break;
        case KC_UP:   snprintf(out, out_size, "Up"); break;
        case KC_DOWN: snprintf(out, out_size, "Dn"); break;
        case KC_LEFT: snprintf(out, out_size, "Lt"); break;
        case KC_RGHT: snprintf(out, out_size, "Rt"); break;
        case KC_COMM: snprintf(out, out_size, ","); break;
        case KC_DOT:  snprintf(out, out_size, "."); break;
        case KC_SLSH: snprintf(out, out_size, "/"); break;
        default:      snprintf(out, out_size, "%02X", keycode); break;
    }
}

static void update_alt_repeat_display_text(uint16_t keycode) {
#if defined(REPEAT_KEY_ENABLE) && !defined(VIAL_ALT_REPEAT_KEY_ENTRIES)
    const uint8_t mods = get_mods() | get_oneshot_mods();
    const uint16_t alt_keycode = get_alt_repeat_key_keycode_user(keycode, mods);

    if (alt_keycode == KC_TRNS || alt_keycode == KC_NO) {
        snprintf(alt_repeat_display_text, sizeof(alt_repeat_display_text), "---");
        return;
    }

    const uint8_t alt_mods = (uint8_t)(alt_keycode >> 8);
    const uint8_t basic = (uint8_t)(alt_keycode & 0xFF);
    char key_name[8] = {0};
    char mod_prefix[6] = {0};
    uint8_t idx = 0;

    format_basic_keycode_name(basic, key_name, sizeof(key_name));

    if ((alt_mods & MOD_MASK_CTRL) != 0U) {
        mod_prefix[idx++] = 'C';
    }
    if ((alt_mods & MOD_MASK_GUI) != 0U) {
        mod_prefix[idx++] = 'G';
    }
    if ((alt_mods & MOD_MASK_SHIFT) != 0U) {
        mod_prefix[idx++] = 'S';
    }
    if ((alt_mods & MOD_MASK_ALT) != 0U) {
        mod_prefix[idx++] = 'A';
    }
    mod_prefix[idx] = '\0';

    if (idx > 0) {
        snprintf(alt_repeat_display_text, sizeof(alt_repeat_display_text), "%s-%s", mod_prefix, key_name);
    } else {
        snprintf(alt_repeat_display_text, sizeof(alt_repeat_display_text), "%s", key_name);
    }
#else
    (void)keycode;
    snprintf(alt_repeat_display_text, sizeof(alt_repeat_display_text), "---");
#endif
}

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
    [L0] = {
        {               MO(10),                MO(9),                MO(5),                MO(4),                MO(8) },
        {              RM_TOGG,              KC_LCTL,              KC_LGUI,              KC_LSFT,              KC_LALT },
        {           SGUI(KC_Z),           LGUI(KC_V),           LGUI(KC_C),           LGUI(KC_X),           LGUI(KC_Z) },
        {     LT(3, KC_BSPACE),                MO(6),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                KC_NO,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {              MS_LEFT,              MS_DOWN,                MS_UP,              MS_RGHT,                KC_NO },
        {                KC_NO,              KC_BTN3,              KC_BTN4,              KC_BTN5,                KC_NO },
        {              KC_BTN2,              KC_BTN1,                KC_NO,                KC_NO,                KC_NO },
        {                TO(1),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L1] = {
        {                 KC_T,                 KC_R,                 KC_E,                 KC_W,                 KC_Q },
        {                 KC_G,                 KC_F,                 KC_D,                 KC_S,                 KC_A },
        {                 KC_B,                 KC_V,                 KC_C,                 KC_X,                 KC_Z },
        {     LT(3, KC_BSPACE),                MO(6),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                 KC_Y,                 KC_U,                 KC_I,                 KC_O,                 KC_P },
        {                 KC_H,                 KC_J,                 KC_K,                 KC_L,                TD(1) },
        {                 KC_N,                 KC_M,                TD(7),                TD(3),                TD(0) },
        {     LT(3, KC_BSPACE),             KC_SPACE,                KC_NO,                KC_NO,                KC_NO },
        {                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L2] = {
        {                 KC_V,                 KC_P,                 KC_D,                 KC_R,                 KC_F },
        {                 KC_B,                 KC_C,                 KC_T,                 KC_N,                 KC_S },
        {                 KC_W,                 KC_G,                 KC_K,                 KC_X,                 KC_Z },
        {     LT(3, KC_BSPACE),                MO(6),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                 KC_Q,                 KC_J,                 KC_U,                 KC_O,                 KC_Y },
        {                TD(3),                 KC_H,                 KC_E,                 KC_A,                 KC_I },
        {                 KC_M,                 KC_L,                TD(4),                TD(7),                TD(2) },
        {     LT(3, KC_BSPACE),             KC_SPACE,                KC_NO,                KC_NO,                KC_NO },
        {                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L3] = {
        {                 KC_5,                 KC_4,                 KC_3,                 KC_2,                 KC_1 },
        {                KC_NO,              KC_PPLS,              KC_PAST,              KC_BSLS,               KC_GRV },
        {                KC_NO,              KC_PMNS,              KC_PSLS,                KC_NO,                KC_NO },
        {              KC_HOME,              KC_PGUP,                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                 KC_6,                 KC_7,                 KC_8,                 KC_9,                 KC_0 },
        {              KC_PCMM,                 KC_4,                 KC_5,                 KC_6,              KC_MINS },
        {              KC_PDOT,                 KC_1,                 KC_2,                 KC_3,               KC_EQL },
        {               KC_END,              KC_PGDN,                KC_NO,                KC_NO,                KC_NO },
        {        QK_LAYER_LOCK,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L4] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L5] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {           LGUI(KC_S),      LALT(KC_BSPACE),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {              _______,         OSM(MOD_MEH), OSM(MOD_LSFT|MOD_LALT),  QK_CAPS_WORD_TOGGLE, OSM(MOD_LSFT|MOD_LGUI) },
        {              _______,        OSM(MOD_RCTL),        OSM(MOD_RGUI),        OSM(MOD_RSFT),        OSM(MOD_RALT) },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L6] = {
        {           LGUI(KC_A),         OSM(MOD_MEH), OSM(MOD_LSFT|MOD_LALT),  QK_CAPS_WORD_TOGGLE, OSM(MOD_LSFT|MOD_LGUI) },
        {              _______,        OSM(MOD_LCTL),        OSM(MOD_LGUI),        OSM(MOD_LSFT),        OSM(MOD_LALT) },
        {           SGUI(KC_Z),           LGUI(KC_V),           LGUI(KC_C),           LGUI(KC_X),           LGUI(KC_Z) },
        {       LGUI(KC_SPACE),              _______,                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {               KC_TAB,                MO(5),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L7] = {
        {                KC_F5,                KC_F4,                KC_F3,                KC_F2,                KC_F1 },
        {           LSFT(KC_0),              KC_RBRC,         LSFT(KC_DOT),    LSFT(KC_RBRACKET),              KC_PSLS },
        {                KC_F6,              KC_MNXT,              KC_MPLY,              KC_MSTP,              KC_MPRV },
        {                KC_UP,              KC_LEFT,                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                KC_F8,                KC_F9,               KC_F10,               KC_F11,               KC_F12 },
        {           LSFT(KC_9),              KC_LBRC,       LSFT(KC_COMMA),    LSFT(KC_LBRACKET),              KC_BSLS },
        {                KC_F7,              KC_MNXT,              KC_MPLY,              KC_MSTP,              KC_MPRV },
        {              KC_DOWN,             KC_RIGHT,                KC_NO,                KC_NO,                KC_NO },
        {           LGUI(KC_0),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L8] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
    },
    [L9] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
    },
    [L10] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
    },
    [L11] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
    },
    [L12] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
    },
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

#if defined(REPEAT_KEY_ENABLE)
    if (keycode != QK_REPEAT_KEY && keycode != QK_ALT_REPEAT_KEY) {
        update_alt_repeat_display_text(keycode);
    }
#else
    update_alt_repeat_display_text(keycode);
#endif

    if (keycode >= RGBL0 && keycode <= RGBL7) {
#ifdef RGB_MATRIX_ENABLE
        set_layer_preset_from_keycode(keycode);
        refresh_rgb_preset_state();
#endif
        return false;
    }

    if (keycode >= RGBM0 && keycode <= RGBM7) {
#ifdef RGB_MATRIX_ENABLE
        set_mod_preset_from_keycode(keycode);
        refresh_rgb_preset_state();
#endif
        return false;
    }

    if (keycode >= RGBC0 && keycode <= RGBC7) {
#ifdef RGB_MATRIX_ENABLE
        trigger_chord_preset_from_keycode(keycode);
        refresh_rgb_preset_state();
#endif
        return false;
    }

    if (keycode == RGBDU) {
#ifdef RGB_MATRIX_ENABLE
        if (chord_override_ms < 10000) {
            chord_override_ms += 250;
        }
#endif
        return false;
    }

    if (keycode == RGBDD) {
#ifdef RGB_MATRIX_ENABLE
        if (chord_override_ms > 250) {
            chord_override_ms -= 250;
        }
#endif
        return false;
    }

    if (is_macro_keycode(keycode)) {
        run_macro_slot((uint8_t)(keycode - MC_0));
        return false;
    }

    return true;
}

void keyboard_post_init_user(void) {
    sync_compiled_defaults_to_dynamic_keymap_once();
#ifdef RGB_MATRIX_ENABLE
    refresh_rgb_preset_state();
#endif
}

void matrix_scan_user(void) {
#ifdef RGB_MATRIX_ENABLE
    refresh_rgb_preset_state();
#endif
}

const char *halcyon_display_alt_repeat_text_user(void) {
    return alt_repeat_display_text;
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
