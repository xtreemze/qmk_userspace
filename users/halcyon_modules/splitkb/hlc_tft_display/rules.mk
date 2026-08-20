SRC += $(USER_PATH)/splitkb/hlc_tft_display/hlc_tft_display.c
POST_CONFIG_H += $(USER_PATH)/splitkb/hlc_tft_display/config.h

# H1F diagnostic firmware: keep the production Vial USB personality while
# compiling passive OS fingerprint tracing into the TFT half only.
XTREEMZE_OS_FINGERPRINT_TRACE ?= yes
ifeq ($(strip $(XTREEMZE_OS_FINGERPRINT_TRACE)),yes)
OPT_DEFS += -DXTREEMZE_OS_FINGERPRINT_TRACE
endif

# Fonts
SRC += $(USER_PATH)/splitkb/hlc_tft_display/graphics/fonts/Retron2000-27.qff.c
