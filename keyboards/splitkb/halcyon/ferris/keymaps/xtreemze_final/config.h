/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define VIAL_KEYBOARD_UID {0x58, 0x19, 0xAE, 0x72, 0x1F, 0xA0, 0xC4, 0x36}

#define VIAL_UNLOCK_COMBO_ROWS { 0, 5 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 0 }
#define VIA_EEPROM_LAYOUT_OPTIONS_DEFAULT 1

#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_KEYPRESSES

#define ENCODER_RESOLUTION 2

/* Use QMK's default 5 ms debounce interval explicitly so the selected
 * per-key debounce algorithm remains stable across firmware upgrades. */
#define DEBOUNCE 5

#define COMBO_TERM 30
#define TAPPING_TERM 180
#ifndef TAPPING_TERM_PER_KEY
#define TAPPING_TERM_PER_KEY
#endif

/* Vial dynamic keymap layer count must match the compiled keymap and encoder_map layer count. */
#define DYNAMIC_KEYMAP_LAYER_COUNT 13

/* Keep parity with splitkb vial_hlc defaults for dynamic macro storage. */
#define DYNAMIC_KEYMAP_MACRO_COUNT 32

/* Keep Vial dynamic feature slots deterministic and aligned with xtreemzeVial defaults. */
#define VIAL_TAP_DANCE_ENTRIES 32
#define VIAL_COMBO_ENTRIES 32
#define VIAL_KEY_OVERRIDE_ENTRIES 32
#define VIAL_ALT_REPEAT_KEY_ENTRIES 32

/* Preserve the effective host family through suspend/resume. Detector reports
 * are stabilized in userspace; do not turn USB reinitialization into a full
 * keyboard soft reset. */
#define SPLIT_DETECTED_OS_ENABLE

/*
 * Persistent user datablock used for RGB profile engine state.
 * Note: with EECONFIG_USER_DATA_SIZE > 0, eeconfig_read_user()/update_user()
 * are replaced by eeconfig_*_user_datablock() APIs.
 */
#define EECONFIG_USER_DATA_SIZE 128
