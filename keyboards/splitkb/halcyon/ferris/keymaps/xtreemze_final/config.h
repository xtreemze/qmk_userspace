#pragma once

#define VIAL_KEYBOARD_UID {0x58, 0x19, 0xAE, 0x72, 0x1F, 0xA0, 0xC4, 0x36}

#define VIAL_UNLOCK_COMBO_ROWS { 0, 5 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 0 }

#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_KEYPRESSES

#define ENCODER_RESOLUTION 2

#define COMBO_TERM 35
#define TAPPING_TERM 180
#define TAPPING_TERM_PER_KEY

/* Vial dynamic keymap layer count must match the compiled keymap and encoder_map layer count. */
#define DYNAMIC_KEYMAP_LAYER_COUNT 13

/* Keep parity with splitkb vial_hlc defaults for dynamic macro storage. */
#define DYNAMIC_KEYMAP_MACRO_COUNT 32

/*
 * Persistent user datablock used for RGB profile engine state.
 * Note: with EECONFIG_USER_DATA_SIZE > 0, eeconfig_read_user()/update_user()
 * are replaced by eeconfig_*_user_datablock() APIs.
 */
#define EECONFIG_USER_DATA_SIZE 128
