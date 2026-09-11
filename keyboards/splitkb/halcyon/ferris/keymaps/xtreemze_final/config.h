/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "halcyon_legacy.h"

#define VIAL_KEYBOARD_UID {0x58, 0x19, 0xAE, 0x72, 0x1F, 0xA0, 0xC4, 0x36}

#define VIAL_UNLOCK_COMBO_ROWS {0, 5}
#define VIAL_UNLOCK_COMBO_COLS {0, 0}
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
#    define TAPPING_TERM_PER_KEY
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

/*
 * Keep keyboard-specific host settings in stable EEPROM tail regions without
 * moving any existing VIA/Vial dynamic-keymap addresses. New reservations are
 * allocated before the existing RGB profile store, so the RGB store address
 * remains byte-for-byte compatible with already-flashed firmware.
 */
#define XTREEMZE_RGB_PROFILE_EEPROM_SIZE 272
#define XTREEMZE_HALCYON_SETTINGS_EEPROM_SIZE 160
#define DYNAMIC_KEYMAP_EEPROM_MAX_ADDR (TOTAL_EEPROM_BYTE_COUNT - XTREEMZE_RGB_PROFILE_EEPROM_SIZE - XTREEMZE_HALCYON_SETTINGS_EEPROM_SIZE - 1)

/* Chunked replication keeps Halcyon display settings coherent on the non-USB
 * half and persists them there, so either half can become the next USB master. */
#define SPLIT_TRANSACTION_IDS_USER XTREEMZE_HALCYON_SETTINGS_SYNC

/* Preserve the effective host family through suspend/resume. Detector reports
 * are stabilized in userspace; do not turn USB reinitialization into a full
 * keyboard soft reset. */
#define SPLIT_DETECTED_OS_ENABLE

/*
 * Persistent user datablock used by the legacy RGB capture engine and host
 * family state. Keep this size stable: the extended RGB profile store and the
 * Halcyon display settings store live in separately reserved EEPROM tails.
 *
 * Note: with EECONFIG_USER_DATA_SIZE > 0, eeconfig_read_user()/update_user()
 * are replaced by eeconfig_*_user_datablock() APIs.
 */
#define EECONFIG_USER_DATA_SIZE 128
