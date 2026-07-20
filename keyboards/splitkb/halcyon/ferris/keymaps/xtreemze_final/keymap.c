// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "dynamic_keymap.h"
#ifdef VIA_ENABLE
#include "via.h"
#endif
#ifdef VIAL_ENABLE
#include "vial.h"
#endif
#ifdef QMK_SETTINGS
#include "qmk_settings.h"
#endif
#include <stdio.h>
#include <string.h>
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
    XM_0 = QK_KB_0,
    XM_1,
    XM_2,
    XM_3,
    XM_4,
    XM_5,
    XM_6,
    XM_7,
    XM_8,
    XM_9,
    RGB_SLAY,
    RGB_SMOD,
    RGB_SCHD,
    RGB_TCHD,
    OS_REDO,
    OS_PSTE,
    OS_COPY,
    OS_CUT,
    OS_UNDO,
    OS_SALL
};

/*
 * When this marker changes, firmware will re-seed Vial's dynamic keymap from the
 * compiled keymap once on boot. This guarantees shipped defaults are applied while
 * still allowing later Vial edits to persist.
 */
#define XTREEMZE_DEFAULTS_EE_MARKER 0xAE
#define XTREEMZE_USER_DATA_MAGIC 0x58
#define XTREEMZE_USER_DATA_VERSION 0x02
#define XTREEMZE_CHORD_MS_DEFAULT 2000
#define XTREEMZE_CHORD_MS_MIN 250
#define XTREEMZE_CHORD_MS_MAX 10000

static char alt_repeat_display_text[24] = "";

#ifdef OS_DETECTION_ENABLE
/*
 * OS detection is based on USB setup traffic, which can change after macOS
 * sleep/resume or delayed descriptor requests. Keep the first confident
 * result for this keyboard boot; changing hosts normally power-cycles it.
 */
static os_variant_t cached_host_os = OS_UNSURE;

bool process_detected_host_os_user(os_variant_t detected_os) {
    if (cached_host_os == OS_UNSURE && detected_os != OS_UNSURE) {
        cached_host_os = detected_os;
    }
    return true;
}
#endif

#ifdef RGB_MATRIX_ENABLE
typedef struct {
    uint8_t mode;
    uint8_t h;
    uint8_t s;
    uint8_t v;
    uint8_t speed;
} rgb_profile_t;

#define RGB_PROFILE_UNASSIGNED 0xFF
#endif

typedef struct {
    uint8_t magic;
    uint8_t version;
    uint8_t defaults_marker;
    uint8_t reserved;
    uint16_t chord_override_ms;
    uint8_t reserved1[2];
#ifdef RGB_MATRIX_ENABLE
    rgb_profile_t layer_profiles[13];
    rgb_profile_t mod_profiles[4]; // Ctrl, Gui, Shift, Alt
    rgb_profile_t chord_profile;
#endif
} xtreemze_user_data_t;

static xtreemze_user_data_t xtreemze_user_data;

static void set_user_data_defaults(xtreemze_user_data_t *data) {
    memset(data, 0, sizeof(*data));
    data->magic = XTREEMZE_USER_DATA_MAGIC;
    data->version = XTREEMZE_USER_DATA_VERSION;
    data->chord_override_ms = XTREEMZE_CHORD_MS_DEFAULT;

#ifdef RGB_MATRIX_ENABLE
    for (uint8_t i = 0; i < ARRAY_SIZE(data->layer_profiles); ++i) {
        data->layer_profiles[i].mode = RGB_PROFILE_UNASSIGNED;
    }
    for (uint8_t i = 0; i < ARRAY_SIZE(data->mod_profiles); ++i) {
        data->mod_profiles[i].mode = RGB_PROFILE_UNASSIGNED;
    }
    data->chord_profile.mode = RGB_PROFILE_UNASSIGNED;
#endif
}

void eeconfig_init_user_datablock(void) {
    set_user_data_defaults(&xtreemze_user_data);
    eeconfig_update_user_datablock(&xtreemze_user_data, 0, sizeof(xtreemze_user_data));
}

static void save_user_data(void) {
    eeconfig_update_user_datablock(&xtreemze_user_data, 0, sizeof(xtreemze_user_data));
}

static void load_user_data(void) {
    if (!eeconfig_is_user_datablock_valid()) {
        eeconfig_init_user_datablock();
        return;
    }

    eeconfig_read_user_datablock(&xtreemze_user_data, 0, sizeof(xtreemze_user_data));
    if (xtreemze_user_data.magic != XTREEMZE_USER_DATA_MAGIC || xtreemze_user_data.version != XTREEMZE_USER_DATA_VERSION) {
        set_user_data_defaults(&xtreemze_user_data);
        save_user_data();
    }

    if (xtreemze_user_data.chord_override_ms < XTREEMZE_CHORD_MS_MIN || xtreemze_user_data.chord_override_ms > XTREEMZE_CHORD_MS_MAX) {
        xtreemze_user_data.chord_override_ms = XTREEMZE_CHORD_MS_DEFAULT;
        save_user_data();
    }
}

#ifdef VIAL_ENABLE
static void seed_vial_dynamic_entry_defaults(void);
#if (DYNAMIC_KEYMAP_MACRO_COUNT > 0)
static void seed_vial_macro_defaults(void);
#endif
#endif
#ifdef QMK_SETTINGS
static void seed_qmk_settings_defaults(void);
#endif
#ifdef VIA_ENABLE
static void seed_via_layout_options_default(void);
#endif

static void sync_compiled_defaults_to_dynamic_keymap_once(void) {
    if (xtreemze_user_data.defaults_marker == XTREEMZE_DEFAULTS_EE_MARKER) {
        return;
    }

    dynamic_keymap_reset();
#ifdef VIAL_ENABLE
    seed_vial_dynamic_entry_defaults();
#if (DYNAMIC_KEYMAP_MACRO_COUNT > 0)
    seed_vial_macro_defaults();
#endif
#endif
#ifdef QMK_SETTINGS
    seed_qmk_settings_defaults();
#endif
#ifdef VIA_ENABLE
    seed_via_layout_options_default();
#endif
    xtreemze_user_data.defaults_marker = XTREEMZE_DEFAULTS_EE_MARKER;
    save_user_data();
}

#ifdef VIAL_ENABLE
#if defined(VIAL_TAP_DANCE_ENTRIES) && (VIAL_TAP_DANCE_ENTRIES > 0)
static const vial_tap_dance_entry_t xtreemze_default_tap_dances[] = {
    { KC_SLASH, LSFT(KC_SLASH), QK_MACRO_9, KC_PSLS, 140 },
    { LSFT(KC_SCLN), KC_QUOT, KC_MINS, KC_QUOT, 160 },
    { KC_COMM, LSFT(KC_SLASH), KC_SLASH, LSFT(KC_SLASH), 175 },
    { KC_DOT, KC_DOT, QK_MACRO_3, KC_DOT, 175 },
    { KC_N, QK_MACRO_6, QK_MACRO_7, QK_MACRO_6, 175 },
    { OSM(MOD_LSFT), MO(6), QK_MACRO_5, MO(6), 45 },
    { KC_Q, LSFT(KC_1), KC_GRV, KC_Q, 180 },
    { KC_COMM, KC_SCLN, KC_COMM, KC_SCLN, 175 },
    { KC_O, KC_MINS, KC_O, KC_O, 200 },
    { KC_UP, KC_UP, KC_PGUP, KC_UP, 90 },
    { KC_LEFT, KC_LEFT, LALT(KC_LEFT), LALT(KC_LEFT), 175 },
    { KC_RGHT, KC_RGHT, RALT(KC_RGHT), RALT(KC_RGHT), 175 },
    { KC_DOWN, KC_DOWN, KC_PGDN, KC_DOWN, 90 },
    { KC_ESC, KC_GRV, QK_MACRO_0, KC_NO, 220 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
    { KC_NO, KC_NO, KC_NO, KC_NO, 200 },
};
#endif

#if defined(VIAL_COMBO_ENTRIES) && (VIAL_COMBO_ENTRIES > 0)
static const vial_combo_entry_t xtreemze_default_combos[] = {
    { { KC_F, KC_D, KC_NO, KC_NO }, MO(7) },
    { { KC_J, KC_K, KC_NO, KC_NO }, MO(7) },
    { { KC_S, KC_D, KC_NO, KC_NO }, KC_ESC },
    { { KC_K, KC_L, KC_NO, KC_NO }, KC_ENT },
    { { KC_J, KC_L, KC_NO, KC_NO }, QK_MACRO_2 },
    { { KC_S, KC_F, KC_NO, KC_NO }, QK_MACRO_0 },
    { { KC_DOWN, KC_RGHT, KC_NO, KC_NO }, QK_MACRO_2 },
    { { MS_RGHT, MS_UP, KC_NO, KC_NO }, KC_ENT },
    { { MS_RGHT, MS_DOWN, KC_NO, KC_NO }, QK_MACRO_2 },
    { { KC_Y, KC_N, KC_NO, KC_NO }, QK_REBOOT },
    { { KC_T, KC_B, KC_NO, KC_NO }, QK_REBOOT },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_DOT, KC_SPACE, KC_NO, KC_NO }, QK_MACRO_1 },
    { { KC_LALT, KC_LGUI, KC_NO, KC_NO }, TD(13) },
    { { KC_W, KC_R, KC_NO, KC_NO }, RCTL(KC_B) },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
    { { KC_NO, KC_NO, KC_NO, KC_NO }, KC_NO },
};
#endif

#if defined(VIAL_KEY_OVERRIDE_ENTRIES) && (VIAL_KEY_OVERRIDE_ENTRIES > 0)
static const vial_key_override_entry_t xtreemze_default_key_overrides[] = {
    { .trigger = MS_WHLU, .replacement = LSFT(KC_TAB), .layers = 3, .trigger_mods = 68, .negative_mod_mask = 0, .suppressed_mods = 68, .options = 7 },
    { .trigger = MS_WHLD, .replacement = KC_TAB, .layers = 7, .trigger_mods = 68, .negative_mod_mask = 0, .suppressed_mods = 68, .options = 7 },
    { .trigger = MS_WHLD, .replacement = LCTL(KC_TAB), .layers = 7, .trigger_mods = 17, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = MS_WHLU, .replacement = LCTL(LSFT(KC_TAB)), .layers = 7, .trigger_mods = 17, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = MS_WHLU, .replacement = SGUI(KC_TAB), .layers = 7, .trigger_mods = 136, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = MS_WHLD, .replacement = LGUI(KC_TAB), .layers = 7, .trigger_mods = 136, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = MS_WHLD, .replacement = KC_VOLU, .layers = 7, .trigger_mods = 34, .negative_mod_mask = 0, .suppressed_mods = 34, .options = 7 },
    { .trigger = MS_WHLU, .replacement = KC_VOLD, .layers = 7, .trigger_mods = 34, .negative_mod_mask = 0, .suppressed_mods = 34, .options = 7 },
    { .trigger = KC_BRID, .replacement = LSFT(KC_TAB), .layers = 64, .trigger_mods = 136, .negative_mod_mask = 119, .suppressed_mods = 119, .options = 135 },
    { .trigger = KC_BRIU, .replacement = KC_TAB, .layers = 64, .trigger_mods = 136, .negative_mod_mask = 119, .suppressed_mods = 119, .options = 135 },
    { .trigger = KC_BRID, .replacement = KC_VOLD, .layers = 64, .trigger_mods = 34, .negative_mod_mask = 221, .suppressed_mods = 255, .options = 135 },
    { .trigger = KC_BRIU, .replacement = KC_VOLU, .layers = 64, .trigger_mods = 34, .negative_mod_mask = 221, .suppressed_mods = 255, .options = 135 },
    { .trigger = KC_BRIU, .replacement = LCTL(KC_N), .layers = 64, .trigger_mods = 68, .negative_mod_mask = 187, .suppressed_mods = 238, .options = 135 },
    { .trigger = KC_BRID, .replacement = LCTL(KC_P), .layers = 64, .trigger_mods = 68, .negative_mod_mask = 187, .suppressed_mods = 238, .options = 135 },
    { .trigger = KC_BRIU, .replacement = KC_TAB, .layers = 64, .trigger_mods = 17, .negative_mod_mask = 238, .suppressed_mods = 255, .options = 135 },
    { .trigger = KC_BRID, .replacement = LSFT(KC_TAB), .layers = 64, .trigger_mods = 17, .negative_mod_mask = 238, .suppressed_mods = 255, .options = 135 },
    { .trigger = KC_KB_VOLUME_UP, .replacement = MS_WHLD, .layers = 16, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_BRIU, .replacement = LCTL(KC_TAB), .layers = 64, .trigger_mods = 102, .negative_mod_mask = 153, .suppressed_mods = 255, .options = 135 },
    { .trigger = KC_BRID, .replacement = LCTL(LSFT(KC_TAB)), .layers = 64, .trigger_mods = 102, .negative_mod_mask = 153, .suppressed_mods = 255, .options = 135 },
    { .trigger = KC_KB_VOLUME_DOWN, .replacement = LCTL(LSFT(KC_TAB)), .layers = 0, .trigger_mods = 17, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 4 },
    { .trigger = KC_KB_VOLUME_UP, .replacement = KC_DOWN, .layers = 0, .trigger_mods = 34, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_KB_VOLUME_DOWN, .replacement = KC_UP, .layers = 0, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
    { .trigger = KC_NO, .replacement = KC_NO, .layers = 65535, .trigger_mods = 0, .negative_mod_mask = 0, .suppressed_mods = 0, .options = 7 },
};
#endif

#if defined(VIAL_ALT_REPEAT_KEY_ENTRIES) && (VIAL_ALT_REPEAT_KEY_ENTRIES > 0)
static const vial_alt_repeat_key_entry_t xtreemze_default_alt_repeat_keys[] = {
    { .keycode = KC_N, .alt_keycode = LSFT(KC_N), .allowed_mods = 0, .options = 12 },
    { .keycode = LCTL(KC_D), .alt_keycode = LCTL(KC_U), .allowed_mods = 0, .options = 12 },
    { .keycode = KC_W, .alt_keycode = KC_B, .allowed_mods = 0, .options = 12 },
    { .keycode = KC_J, .alt_keycode = KC_K, .allowed_mods = 68, .options = 12 },
    { .keycode = KC_L, .alt_keycode = KC_H, .allowed_mods = 68, .options = 12 },
    { .keycode = KC_TAB, .alt_keycode = LSFT(KC_TAB), .allowed_mods = 17, .options = 13 },
    { .keycode = LGUI(KC_G), .alt_keycode = SGUI(KC_G), .allowed_mods = 0, .options = 12 },
    { .keycode = KC_U, .alt_keycode = LSFT(KC_U), .allowed_mods = 0, .options = 14 },
    { .keycode = LSFT(KC_DOT), .alt_keycode = LSFT(KC_COMM), .allowed_mods = 0, .options = 12 },
    { .keycode = KC_RBRC, .alt_keycode = KC_LBRC, .allowed_mods = 119, .options = 14 },
    { .keycode = LCTL(KC_A), .alt_keycode = LCTL(KC_X), .allowed_mods = 17, .options = 12 },
    { .keycode = KC_BSPC, .alt_keycode = KC_DEL, .allowed_mods = 103, .options = 12 },
    { .keycode = KC_RGHT, .alt_keycode = KC_LEFT, .allowed_mods = 0, .options = 14 },
    { .keycode = KC_UP, .alt_keycode = KC_DOWN, .allowed_mods = 0, .options = 14 },
    { .keycode = TD(12), .alt_keycode = KC_UP, .allowed_mods = 0, .options = 14 },
    { .keycode = KC_1, .alt_keycode = KC_2, .allowed_mods = 0, .options = 8 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
    { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 },
};
#endif

static void seed_vial_dynamic_entry_defaults(void) {
#if defined(VIAL_TAP_DANCE_ENTRIES) && (VIAL_TAP_DANCE_ENTRIES > 0)
    const vial_tap_dance_entry_t blank_td = { KC_NO, KC_NO, KC_NO, KC_NO, TAPPING_TERM };
    for (uint8_t i = 0; i < VIAL_TAP_DANCE_ENTRIES; ++i) {
        const vial_tap_dance_entry_t *entry = (i < ARRAY_SIZE(xtreemze_default_tap_dances)) ? &xtreemze_default_tap_dances[i] : &blank_td;
        dynamic_keymap_set_tap_dance(i, entry);
    }
#endif

#if defined(VIAL_COMBO_ENTRIES) && (VIAL_COMBO_ENTRIES > 0)
    const vial_combo_entry_t blank_combo = { .input = { KC_NO, KC_NO, KC_NO, KC_NO }, .output = KC_NO };
    for (uint8_t i = 0; i < VIAL_COMBO_ENTRIES; ++i) {
        const vial_combo_entry_t *entry = (i < ARRAY_SIZE(xtreemze_default_combos)) ? &xtreemze_default_combos[i] : &blank_combo;
        dynamic_keymap_set_combo(i, entry);
    }
#endif

#if defined(VIAL_KEY_OVERRIDE_ENTRIES) && (VIAL_KEY_OVERRIDE_ENTRIES > 0)
    const vial_key_override_entry_t blank_ko = {
        .trigger = KC_NO,
        .replacement = KC_NO,
        .layers = 65535,
        .trigger_mods = 0,
        .negative_mod_mask = 0,
        .suppressed_mods = 0,
        .options = 7,
    };
    for (uint8_t i = 0; i < VIAL_KEY_OVERRIDE_ENTRIES; ++i) {
        const vial_key_override_entry_t *entry = (i < ARRAY_SIZE(xtreemze_default_key_overrides)) ? &xtreemze_default_key_overrides[i] : &blank_ko;
        dynamic_keymap_set_key_override(i, entry);
    }
#endif

#if defined(VIAL_ALT_REPEAT_KEY_ENTRIES) && (VIAL_ALT_REPEAT_KEY_ENTRIES > 0)
    const vial_alt_repeat_key_entry_t blank_arep = { .keycode = KC_NO, .alt_keycode = KC_NO, .allowed_mods = 0, .options = 0 };
    for (uint8_t i = 0; i < VIAL_ALT_REPEAT_KEY_ENTRIES; ++i) {
        const vial_alt_repeat_key_entry_t *entry = (i < ARRAY_SIZE(xtreemze_default_alt_repeat_keys)) ? &xtreemze_default_alt_repeat_keys[i] : &blank_arep;
        dynamic_keymap_set_alt_repeat_key(i, entry);
    }
#endif
}

#if (DYNAMIC_KEYMAP_MACRO_COUNT > 0)
#ifndef VIAL_MACRO_EXT_TAP
#define VIAL_MACRO_EXT_TAP 5
#endif

static bool macro_seed_write_byte(uint16_t *offset, uint16_t max_size, uint8_t value) {
    if (*offset >= max_size) {
        return false;
    }
    dynamic_keymap_macro_set_buffer(*offset, 1, &value);
    (*offset)++;
    return true;
}

static bool macro_seed_write_tap(uint16_t *offset, uint16_t max_size, uint16_t keycode) {
    return macro_seed_write_byte(offset, max_size, 1) && macro_seed_write_byte(offset, max_size, VIAL_MACRO_EXT_TAP) && macro_seed_write_byte(offset, max_size, keycode & 0xFF) && macro_seed_write_byte(offset, max_size, keycode >> 8);
}

static bool macro_seed_write_text(uint16_t *offset, uint16_t max_size, const char *text) {
    while (*text) {
        if (!macro_seed_write_byte(offset, max_size, (uint8_t)*text)) {
            return false;
        }
        ++text;
    }
    return true;
}

static bool macro_seed_end_slot(uint16_t *offset, uint16_t max_size) {
    return macro_seed_write_byte(offset, max_size, 0);
}

static void seed_vial_macro_defaults(void) {
    dynamic_keymap_macro_reset();

    uint16_t offset = 0;
    const uint16_t macro_buffer_size = dynamic_keymap_macro_get_buffer_size();
    bool ok = true;

    // M0: tap USER13, HYPR(KC_SPACE)
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, RGB_TCHD);
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, HYPR(KC_SPACE));
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M1: tap KC_DOT, KC_SPACE, OSM(MOD_RSFT)
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_DOT);
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_SPACE);
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, OSM(MOD_RSFT));
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M2: tap LCTL(KC_QUOT)
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, LCTL(KC_QUOT));
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M3: text "..."
    ok &= macro_seed_write_text(&offset, macro_buffer_size, "...");
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M4: text ".  " then tap OSM(MOD_RSFT)
    ok &= macro_seed_write_text(&offset, macro_buffer_size, ".  ");
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, OSM(MOD_RSFT));
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M5: tap KC_BSPC, KC_BSPC, KC_BSPC, LALT(KC_BSPC)
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_BSPC);
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_BSPC);
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_BSPC);
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, LALT(KC_BSPC));
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M6: tap LALT(KC_N), KC_N
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, LALT(KC_N));
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_N);
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M7: text "nn"
    ok &= macro_seed_write_text(&offset, macro_buffer_size, "nn");
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M8: tap TG(1), TG(4)
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, TG(1));
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, TG(4));
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M9: tap KC_SLASH, KC_SLASH
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_SLASH);
    ok &= macro_seed_write_tap(&offset, macro_buffer_size, KC_SLASH);
    ok &= macro_seed_end_slot(&offset, macro_buffer_size);

    // M10..M31 empty by default.
    for (uint8_t i = 10; i < DYNAMIC_KEYMAP_MACRO_COUNT && ok; ++i) {
        ok &= macro_seed_end_slot(&offset, macro_buffer_size);
    }

    (void)ok;
}
#endif
#endif

#ifdef QMK_SETTINGS
typedef struct {
    uint16_t id;
    uint32_t value;
} qmk_setting_seed_t;

static const qmk_setting_seed_t xtreemze_qmk_settings_defaults[] = {
    { 1, 0 },
    { 2, 25 },
    { 3, 33 },
    { 4, 170 },
    { 5, 2 },
    { 6, 1100 },
    { 7, 160 },
    { 9, 14 },
    { 10, 28 },
    { 11, 8 },
    { 12, 10 },
    { 13, 30 },
    { 14, 20 },
    { 15, 48 },
    { 16, 24 },
    { 17, 64 },
    { 18, 0 },
    { 19, 80 },
    { 20, 5 },
    { 21, 0 },
    { 22, 1 },
    { 23, 0 },
    { 24, 0 },
    { 25, 120 },
    { 26, 0 },
    { 27, 0 },
};

static void seed_qmk_settings_defaults(void) {
    for (uint8_t i = 0; i < ARRAY_SIZE(xtreemze_qmk_settings_defaults); ++i) {
        const qmk_setting_seed_t setting = xtreemze_qmk_settings_defaults[i];
        const uint32_t value = setting.value;
        qmk_settings_set(setting.id, &value, sizeof(value));
    }
}
#endif

#ifdef VIA_ENABLE
static void seed_via_layout_options_default(void) {
    // final Vial profile sets layout_options=1 (right module = encoder, left module = none/TFT profile path).
    via_set_layout_options(1U);
}
#endif

#ifdef RGB_MATRIX_ENABLE
static uint32_t chord_override_start = 0;
static bool chord_override_active = false;
static rgb_profile_t last_applied_profile;
static bool has_last_applied_profile = false;
static uint32_t rgb_profile_generation = 0;

static inline bool is_profile_assigned(const rgb_profile_t *profile) {
    return profile->mode != RGB_PROFILE_UNASSIGNED;
}

static inline bool rgb_profile_equal(const rgb_profile_t *a, const rgb_profile_t *b) {
    return a->mode == b->mode && a->h == b->h && a->s == b->s && a->v == b->v && a->speed == b->speed;
}

static rgb_profile_t capture_current_rgb_profile(void) {
    return (rgb_profile_t){
        .mode = rgb_matrix_get_mode(),
        .h = rgb_matrix_get_hue(),
        .s = rgb_matrix_get_sat(),
        .v = rgb_matrix_get_val(),
        .speed = rgb_matrix_get_speed(),
    };
}

static void apply_rgb_profile(const rgb_profile_t *profile) {
    if (!is_profile_assigned(profile)) {
        return;
    }

    rgb_matrix_mode_noeeprom(profile->mode);
    rgb_matrix_sethsv_noeeprom(profile->h, profile->s, profile->v);
    rgb_matrix_set_speed_noeeprom(profile->speed);
}

static void invalidate_rgb_profile_cache(void) {
    rgb_profile_generation++;
    has_last_applied_profile = false;
}

static void set_layer_profile_from_current_rgb(void) {
    const uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer >= ARRAY_SIZE(xtreemze_user_data.layer_profiles)) {
        return;
    }

    xtreemze_user_data.layer_profiles[layer] = capture_current_rgb_profile();
    save_user_data();
    invalidate_rgb_profile_cache();
}

static void set_mod_profiles_from_current_rgb(void) {
    const uint8_t mods = get_mods() | get_oneshot_mods();
    if (mods == 0U) {
        return;
    }

    const rgb_profile_t profile = capture_current_rgb_profile();

    if ((mods & MOD_MASK_CTRL) != 0U) {
        xtreemze_user_data.mod_profiles[0] = profile;
    }
    if ((mods & MOD_MASK_GUI) != 0U) {
        xtreemze_user_data.mod_profiles[1] = profile;
    }
    if ((mods & MOD_MASK_SHIFT) != 0U) {
        xtreemze_user_data.mod_profiles[2] = profile;
    }
    if ((mods & MOD_MASK_ALT) != 0U) {
        xtreemze_user_data.mod_profiles[3] = profile;
    }

    save_user_data();
    invalidate_rgb_profile_cache();
}

static void set_chord_profile_from_current_rgb(void) {
    xtreemze_user_data.chord_profile = capture_current_rgb_profile();
    save_user_data();
    invalidate_rgb_profile_cache();
}

static void trigger_chord_profile(void) {
    if (!is_profile_assigned(&xtreemze_user_data.chord_profile)) {
        return;
    }

    chord_override_active = true;
    chord_override_start = timer_read32();
}

static void refresh_rgb_profile_state(void) {
    static uint32_t last_rgb_refresh_generation = 0xFFFFFFFFUL;
    static uint8_t last_rgb_refresh_mods = 0xFF;
    static uint8_t last_rgb_refresh_layer = 0xFF;
    static bool last_rgb_refresh_chord_active = false;
    static bool last_rgb_refresh_chord_assigned = false;

    const uint8_t mods = get_mods() | get_oneshot_mods();
    const uint8_t layer = get_highest_layer(layer_state | default_layer_state);

    if (chord_override_active && timer_elapsed32(chord_override_start) > xtreemze_user_data.chord_override_ms) {
        chord_override_active = false;
    }

    const bool chord_assigned = is_profile_assigned(&xtreemze_user_data.chord_profile);
    if (!chord_override_active &&
        last_rgb_refresh_generation == rgb_profile_generation &&
        last_rgb_refresh_mods == mods &&
        last_rgb_refresh_layer == layer &&
        !last_rgb_refresh_chord_active &&
        last_rgb_refresh_chord_assigned == chord_assigned) {
        return;
    }

    const rgb_profile_t *target = NULL;
    if (chord_override_active && chord_assigned) {
        target = &xtreemze_user_data.chord_profile;
    } else {
        if ((mods & MOD_MASK_CTRL) != 0U && is_profile_assigned(&xtreemze_user_data.mod_profiles[0])) {
            target = &xtreemze_user_data.mod_profiles[0];
        } else if ((mods & MOD_MASK_GUI) != 0U && is_profile_assigned(&xtreemze_user_data.mod_profiles[1])) {
            target = &xtreemze_user_data.mod_profiles[1];
        } else if ((mods & MOD_MASK_SHIFT) != 0U && is_profile_assigned(&xtreemze_user_data.mod_profiles[2])) {
            target = &xtreemze_user_data.mod_profiles[2];
        } else if ((mods & MOD_MASK_ALT) != 0U && is_profile_assigned(&xtreemze_user_data.mod_profiles[3])) {
            target = &xtreemze_user_data.mod_profiles[3];
        } else {
            if (layer < ARRAY_SIZE(xtreemze_user_data.layer_profiles) && is_profile_assigned(&xtreemze_user_data.layer_profiles[layer])) {
                target = &xtreemze_user_data.layer_profiles[layer];
            }
        }
    }

    last_rgb_refresh_generation = rgb_profile_generation;
    last_rgb_refresh_mods = mods;
    last_rgb_refresh_layer = layer;
    last_rgb_refresh_chord_active = chord_override_active;
    last_rgb_refresh_chord_assigned = chord_assigned;

    if (target == NULL) {
        has_last_applied_profile = false;
        return;
    }

    if (!has_last_applied_profile || !rgb_profile_equal(target, &last_applied_profile)) {
        apply_rgb_profile(target);
        last_applied_profile = *target;
        has_last_applied_profile = true;
    }
}
#endif

#if defined(REPEAT_KEY_ENABLE) && !defined(VIAL_ALT_REPEAT_KEY_ENTRIES)
uint16_t get_alt_repeat_key_keycode_user(uint16_t keycode, uint8_t mods);
#endif

static void __attribute__((unused)) format_basic_keycode_name(uint8_t keycode, char *out, size_t out_size) {
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
        alt_repeat_display_text[0] = '\0';
        return;
    }

    const uint8_t basic = (uint8_t)(alt_keycode & 0xFF);
    char key_name[8] = {0};

    format_basic_keycode_name(basic, key_name, sizeof(key_name));
    snprintf(alt_repeat_display_text, sizeof(alt_repeat_display_text), "%s", key_name);
#else
    (void)keycode;
    alt_repeat_display_text[0] = '\0';
#endif
}

static inline bool is_macro_keycode(uint16_t keycode) {
    return keycode >= XM_0 && keycode <= XM_9;
}

static bool host_is_apple(void) {
#ifdef OS_DETECTION_ENABLE
    return cached_host_os == OS_MACOS || cached_host_os == OS_IOS;
#else
    return false;
#endif
}

static void tap_os_clipboard(uint16_t mac_keycode, uint16_t other_keycode) {
    tap_code16(host_is_apple() ? mac_keycode : other_keycode);
}

static void run_macro_slot(uint8_t slot);

static void tap_action_keycode(uint16_t keycode) {
    if (keycode == KC_NO || keycode == KC_TRNS) {
        return;
    }

    if (is_macro_keycode(keycode)) {
        run_macro_slot((uint8_t)(keycode - XM_0));
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

static void __attribute__((unused)) press_action_keycode(uint16_t keycode) {
    if (keycode == KC_NO || keycode == KC_TRNS) {
        return;
    }

    if (is_macro_keycode(keycode)) {
        run_macro_slot((uint8_t)(keycode - XM_0));
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

static void __attribute__((unused)) release_action_keycode(uint16_t keycode) {
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
        {               MO(10),                MO(9),               MO(11),                MO(4),                MO(8) },
        {              RM_TOGG,              KC_LCTL,              KC_LGUI,              KC_LSFT,              KC_LALT },
        {              OS_REDO,              OS_PSTE,              OS_COPY,               OS_CUT,              OS_UNDO },
        {     LT(3, KC_BSPC),                MO(6),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                KC_NO,             RGB_SLAY,             RGB_SMOD,             RGB_SCHD,                KC_NO },
        {              MS_LEFT,              MS_DOWN,                MS_UP,              MS_RGHT,                KC_NO },
        {                KC_NO,              KC_BTN3,              KC_BTN4,              KC_BTN5,                KC_NO },
        {              KC_BTN2,              KC_BTN1,                KC_NO,                KC_NO,                KC_NO },
        {                TO(1),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L1] = {
        {                 KC_T,                 KC_R,                 KC_E,                 KC_W,                 KC_Q },
        {                 KC_G,                 KC_F,                 KC_D,                 KC_S,                 KC_A },
        {                 KC_B,                 KC_V,                 KC_C,                 KC_X,                 KC_Z },
        {     LT(3, KC_BSPC),                MO(6),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                 KC_Y,                 KC_U,                 KC_I,                 KC_O,                 KC_P },
        {                 KC_H,                 KC_J,                 KC_K,                 KC_L,                TD(1) },
        {                 KC_N,                 KC_M,                TD(7),               KC_DOT,                TD(0) },
        {     LT(3, KC_BSPC),             KC_SPACE,                KC_NO,                KC_NO,                KC_NO },
        {                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L2] = {
        {                 KC_V,                 KC_P,                 KC_D,                 KC_R,                 KC_F },
        {                 KC_B,                 KC_C,                 KC_T,                 KC_N,                 KC_S },
        {                 KC_W,                 KC_G,                 KC_K,                 KC_X,                 KC_Z },
        {     LT(3, KC_BSPC),                MO(6),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                 KC_Q,                 KC_J,                 KC_U,                 KC_O,                 KC_Y },
        {                TD(3),                 KC_H,                 KC_E,                 KC_A,                 KC_I },
        {                 KC_M,                 KC_L,                TD(4),                TD(7),                TD(2) },
        {     LT(3, KC_BSPC),             KC_SPACE,                KC_NO,                KC_NO,                KC_NO },
        {                TO(0),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L3] = {
        {                 KC_5,                 KC_4,                 KC_3,                 KC_2,                 KC_1 },
        {              KC_PPLS,      LCTL_T(KC_SPC),      LGUI_T(KC_TAB),     LSFT_T(KC_BSLS),     LALT_T(KC_PAST) },
        {              KC_PMNS,                TG(4),        LSFT(KC_TAB),               KC_ESC,        QK_LAYER_LOCK },
        {              KC_BTN1,              KC_BTN2,                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                 KC_6,                 KC_7,                 KC_8,                 KC_9,                 KC_0 },
        {              KC_PCMM,                 KC_4,                 KC_5,                 KC_6,              KC_MINS },
        {              KC_PDOT,                 KC_1,                 KC_2,                 KC_3,               KC_EQL },
        {               KC_END,        LT(4, KC_PGDN),                KC_NO,                KC_NO,                KC_NO },
        {        QK_LAYER_LOCK,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L4] = {
        {                 KC_6,                 KC_7,                 KC_8,                 KC_9,                 KC_0 },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                 KC_5,                 KC_4,                 KC_3,                 KC_2,                 KC_1 },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {                KC_NO,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L5] = {
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {           LGUI(KC_S),      LALT(KC_BSPC),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {              _______,         OSM(MOD_MEH), OSM(MOD_LSFT|MOD_LALT),  QK_CAPS_WORD_TOGGLE, OSM(MOD_LSFT|MOD_LGUI) },
        { OSM(MOD_LCTL|MOD_LSFT),        OSM(MOD_RCTL),        OSM(MOD_RGUI),        OSM(MOD_RSFT),        OSM(MOD_RALT) },
        {              _______,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L6] = {
        {              OS_SALL,         OSM(MOD_MEH), OSM(MOD_LSFT|MOD_LALT),  QK_CAPS_WORD_TOGGLE, OSM(MOD_LSFT|MOD_LGUI) },
        { OSM(MOD_LCTL|MOD_LSFT),        OSM(MOD_LCTL),        OSM(MOD_LGUI),        OSM(MOD_LSFT),        OSM(MOD_LALT) },
        {              OS_REDO,              OS_PSTE,              OS_COPY,               OS_CUT,              OS_UNDO },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              AS_TOGG,              _______ },
        {              _______,              _______,              _______,        OSM(MOD_RSFT),              _______ },
        {              _______,              _______,              _______,              _______,              _______ },
        {               KC_TAB,                MO(5),                KC_NO,                KC_NO,                KC_NO },
        {              _______,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L7] = {
        {                KC_F5,                KC_F4,                KC_F3,                KC_F2,                KC_F1 },
        {              SC_LSPO,              KC_LBRC,         LSFT(KC_COMM),    LSFT(KC_LBRC),              KC_PSLS },
        {                KC_F6,              KC_MNXT,              KC_MPLY,        LT(8, KC_SPC),               KC_GRV },
        {                TD(9),               TD(10),                KC_NO,                KC_NO,                KC_NO },
        {              KC_MUTE,                KC_NO,                KC_NO,                KC_NO,                KC_NO },
        {                KC_F8,                KC_F9,               KC_F10,               KC_F11,               KC_F12 },
        {              SC_RSPC,              KC_RBRC,         LSFT(KC_DOT),    LSFT(KC_RBRC),              KC_BSLS },
        {                KC_F7,              KC_MNXT,              KC_MPLY,              KC_MSTP,              KC_MPRV },
        {               TD(12),               TD(11),                KC_NO,                KC_NO,                KC_NO },
        {           LGUI(KC_0),                KC_NO,                KC_NO,                KC_NO,                KC_NO },
    },
    [L8] = {
        {                KC_F8,                KC_F9,               KC_F10,               KC_F11,               KC_F12 },
        {              _______,              _______,              _______,              _______,              _______ },
        {                KC_F7,              _______,              _______,              _______,              _______ },
        {              _______,              _______,                KC_NO,                KC_NO,                KC_NO },
        {              _______,              _______,              _______,              _______,              _______ },
        {                KC_F5,                KC_F4,                KC_F3,                KC_F2,                KC_F1 },
        {              _______,              _______,              _______,              _______,              _______ },
        {                KC_F6,              _______,              _______,              _______,              _______ },
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
// Preserve both split Halcyon module encoder slots from xtreemzeVial.vil.
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [L0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(MS_WHLU, MS_WHLD) },
    [L1] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(MS_WHLU, MS_WHLD) },
    [L2] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(MS_WHLU, MS_WHLD) },
    [L3] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(QK_ALT_REPEAT_KEY, QK_REPEAT_KEY) },
    [L4] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(RM_SATD, RM_SATU) },
    [L5] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(RM_VALD, RM_VALU) },
    [L6] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_BRID, KC_BRIU) },
    [L7] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(LGUI(KC_PMNS), LGUI(KC_PPLS)) },
    [L8] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(RM_SPDD, RM_SPDU) },
    [L9] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(RM_PREV, RM_NEXT) },
    [L10] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(RM_HUED, RM_HUEU) },
    [L11] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(RM_VALD, RM_VALU) },
    [L12] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
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
static const uint16_t PROGMEM combo_15[] = { KC_DOT, KC_SPACE, COMBO_END };
static const uint16_t PROGMEM combo_16[] = { KC_LALT, KC_LGUI, COMBO_END };
static const uint16_t PROGMEM combo_17[] = { KC_W, KC_R, COMBO_END };

combo_t key_combos[] = {
    [CM_0] = COMBO(combo_0, MO(7)),
    [CM_1] = COMBO(combo_1, MO(7)),
    [CM_2] = COMBO(combo_2, KC_ESC),
    [CM_3] = COMBO(combo_3, KC_ENT),
    [CM_4] = COMBO(combo_4, XM_2),
    [CM_5] = COMBO(combo_5, XM_0),
    [CM_6] = COMBO(combo_6, XM_2),
    [CM_7] = COMBO(combo_7, KC_ENT),
    [CM_8] = COMBO(combo_8, XM_2),
    [CM_9] = COMBO(combo_9, QK_REBOOT),
    [CM_10] = COMBO(combo_10, QK_REBOOT),
    [CM_11] = COMBO(combo_11, QK_BOOT),
    [CM_12] = COMBO(combo_12, QK_BOOT),
    [CM_13] = COMBO(combo_13, QK_CLEAR_EEPROM),
    [CM_14] = COMBO(combo_14, QK_CLEAR_EEPROM),
    [CM_15] = COMBO(combo_15, XM_1),
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
    [0] = { KC_PSLS, LSFT(KC_SLASH), XM_9, KC_PSLS, 150 },
    [1] = { LSFT(KC_SCLN), KC_QUOT, KC_MINS, KC_QUOT, 150 },
    [2] = { KC_COMM, LSFT(KC_SLASH), KC_SLASH, LSFT(KC_SLASH), 175 },
    [3] = { KC_DOT, KC_DOT, XM_3, KC_DOT, 175 },
    [4] = { KC_N, XM_6, XM_7, XM_6, 175 },
    [5] = { OSM(MOD_LSFT), MO(6), XM_5, MO(6), 45 },
    [6] = { KC_Q, LSFT(KC_1), KC_GRV, KC_Q, 180 },
    [7] = { KC_COMM, KC_SCLN, KC_COMM, KC_SCLN, 175 },
    [8] = { KC_O, KC_MINS, KC_O, KC_O, 200 },
    [13] = { KC_ESC, KC_GRV, XM_0, KC_NO, 220 },
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
#define KO_NEGATIVE_MOD_UP_OPTIONS (ko_option_activation_negative_mod_up)

static const key_override_t ko_0 = { .trigger = MS_WHLU, .trigger_mods = 68, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 68, .replacement = LSFT(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_1 = { .trigger = MS_WHLD, .trigger_mods = 68, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 68, .replacement = KC_TAB, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_2 = { .trigger = MS_WHLD, .trigger_mods = 17, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = LCTL(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_3 = { .trigger = MS_WHLU, .trigger_mods = 17, .layers = (layer_state_t)7, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = LCTL(LSFT(KC_TAB)), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
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
static const key_override_t ko_16 = { .trigger = KC_KB_VOLUME_UP, .trigger_mods = 0, .layers = (layer_state_t)16, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = MS_WHLD, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_17 = { .trigger = KC_BRIU, .trigger_mods = 102, .layers = (layer_state_t)64, .negative_mod_mask = 153, .suppressed_mods = 255, .replacement = LCTL(KC_TAB), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_18 = { .trigger = KC_BRID, .trigger_mods = 102, .layers = (layer_state_t)64, .negative_mod_mask = 153, .suppressed_mods = 255, .replacement = LCTL(LSFT(KC_TAB)), .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_19 = { .trigger = KC_KB_VOLUME_DOWN, .trigger_mods = 17, .layers = (layer_state_t)0, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = LCTL(LSFT(KC_TAB)), .options = KO_NEGATIVE_MOD_UP_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_20 = { .trigger = KC_KB_VOLUME_UP, .trigger_mods = 34, .layers = (layer_state_t)0, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = KC_DOWN, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };
static const key_override_t ko_21 = { .trigger = KC_KB_VOLUME_DOWN, .trigger_mods = 0, .layers = (layer_state_t)0, .negative_mod_mask = 0, .suppressed_mods = 0, .replacement = KC_UP, .options = KO_COMMON_OPTIONS, .custom_action = NULL, .context = NULL, .enabled = NULL };

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
    &ko_16,
    &ko_17,
    &ko_18,
    &ko_19,
    &ko_20,
    &ko_21,
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
    { KC_TAB, LSFT(KC_TAB), 17, 13 },
    { LGUI(KC_G), SGUI(KC_G), 0, 12 },
    { KC_U, LSFT(KC_U), 0, 14 },
    { LSFT(KC_DOT), LSFT(KC_COMM), 0, 12 },
    { KC_RBRC, KC_LBRC, 119, 12 },
    { LCTL(KC_A), LCTL(KC_X), 17, 12 },
    { KC_BSPC, KC_DEL, 103, 12 },
    { KC_RGHT, KC_LEFT, 0, 12 },
    { KC_UP, KC_DOWN, 0, 12 },
    { KC_DOWN, KC_UP, 0, 12 },
    { KC_1, KC_2, 0, 8 },
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
            tap_action_keycode(RGB_TCHD);
            tap_action_keycode(HYPR(KC_SPACE));
            break;
        case 1:
            tap_action_keycode(KC_DOT);
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

    if (keycode == RGB_SLAY) {
#ifdef RGB_MATRIX_ENABLE
        set_layer_profile_from_current_rgb();
        refresh_rgb_profile_state();
#endif
        return false;
    }

    if (keycode == OS_REDO) {
        tap_os_clipboard(SGUI(KC_Z), LCTL(KC_Y));
        return false;
    }

    if (keycode == OS_PSTE) {
        tap_os_clipboard(LGUI(KC_V), LCTL(KC_V));
        return false;
    }

    if (keycode == OS_COPY) {
        tap_os_clipboard(LGUI(KC_C), LCTL(KC_C));
        return false;
    }

    if (keycode == OS_CUT) {
        tap_os_clipboard(LGUI(KC_X), LCTL(KC_X));
        return false;
    }

    if (keycode == OS_UNDO) {
        tap_os_clipboard(LGUI(KC_Z), LCTL(KC_Z));
        return false;
    }

    if (keycode == OS_SALL) {
        tap_os_clipboard(LGUI(KC_A), LCTL(KC_A));
        return false;
    }

    if (keycode == RGB_SMOD) {
#ifdef RGB_MATRIX_ENABLE
        set_mod_profiles_from_current_rgb();
        refresh_rgb_profile_state();
#endif
        return false;
    }

    if (keycode == RGB_SCHD) {
#ifdef RGB_MATRIX_ENABLE
        set_chord_profile_from_current_rgb();
        refresh_rgb_profile_state();
#endif
        return false;
    }

    if (keycode == RGB_TCHD) {
#ifdef RGB_MATRIX_ENABLE
        trigger_chord_profile();
        refresh_rgb_profile_state();
#endif
        return false;
    }

    if (is_macro_keycode(keycode)) {
        run_macro_slot((uint8_t)(keycode - XM_0));
        return false;
    }

    return true;
}

void keyboard_post_init_user(void) {
    load_user_data();
    sync_compiled_defaults_to_dynamic_keymap_once();
#ifdef RGB_MATRIX_ENABLE
    refresh_rgb_profile_state();
#endif
}

void matrix_scan_user(void) {
#ifdef RGB_MATRIX_ENABLE
    refresh_rgb_profile_state();
#endif
}

const char *halcyon_display_alt_repeat_text_user(void) {
    return alt_repeat_display_text;
}

const char *halcyon_display_layer_name_user(uint8_t layer) {
    static const char *const layer_names[] = {
        "MOUSE",
        "QWERTY",
        "COLEMAK",
        "NUMSYMS",
        "RGBSAT",
        "ONESHOT",
        "EDITING",
        "FNMEDIA",
        "RGBSPD",
        "RGBMODE",
        "RGBHUE",
        "RGBVAL",
        "RESERVE",
    };

    if (layer < ARRAY_SIZE(layer_names)) {
        return layer_names[layer];
    }

    return "LAYX";
}
