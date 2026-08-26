# Compiled defaults + Vial dynamic override support
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes
QMK_SETTINGS = yes

RGB_MATRIX_ENABLE = yes
ENCODER_ENABLE = yes
ENCODER_MAP_ENABLE = yes
COMBO_ENABLE = yes
TAP_DANCE_ENABLE = yes
KEY_OVERRIDE_ENABLE = yes
REPEAT_KEY_ENABLE = yes
LAYER_LOCK_ENABLE = yes
CAPS_WORD_ENABLE = yes
MOUSEKEY_ENABLE = yes
ONESHOT_ENABLE = yes
OS_DETECTION_ENABLE = yes

# Isolate debounce timers per switch so rapid activity on other keys cannot
# suppress a short press before it reaches QMK's key processing pipeline.
DEBOUNCE_TYPE = sym_defer_pk

# This adds module functionality to your keyboard (files found in users/halcyon_modules)
USER_NAME := halcyon_modules

LTO_ENABLE = yes

# Keep legacy RGB/matrix coordinates local to this keymap, as in vial_hlc_legacy.
SRC += halcyon_overrides.c
