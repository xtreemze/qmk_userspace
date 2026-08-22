POST_CONFIG_H += $(USER_PATH)/splitkb/hlc_tft_display/config.h

# Panel fault diagnostic firmware: swap the whole display module for a bare
# full-screen solid colour fill. No fonts, patterns, animations or telemetry
# are compiled in, so anything still visible on screen is not being drawn by us.
XTREEMZE_TFT_SOLID_FILL ?= no

ifeq ($(strip $(XTREEMZE_TFT_SOLID_FILL)),yes)

SRC += $(USER_PATH)/splitkb/hlc_tft_display/hlc_tft_solid_fill.c
OPT_DEFS += -DXTREEMZE_TFT_SOLID_FILL

else

SRC += $(USER_PATH)/splitkb/hlc_tft_display/hlc_tft_display.c

# H1F diagnostic firmware: keep the production Vial USB personality while
# compiling passive OS fingerprint tracing into the TFT half only.
XTREEMZE_OS_FINGERPRINT_TRACE ?= yes
ifeq ($(strip $(XTREEMZE_OS_FINGERPRINT_TRACE)),yes)
OPT_DEFS += -DXTREEMZE_OS_FINGERPRINT_TRACE
endif

# Test hook: force every wake recovery attempt to fail, so the retry/backoff and
# TFT_POWER_FAILED policy can be exercised on hardware. Never ship this on.
TFT_FORCE_RECOVERY_FAILURE ?= no
ifeq ($(strip $(TFT_FORCE_RECOVERY_FAILURE)),yes)
OPT_DEFS += -DTFT_FORCE_RECOVERY_FAILURE
endif

# Fonts
SRC += $(USER_PATH)/splitkb/hlc_tft_display/graphics/fonts/Retron2000-27.qff.c

endif
