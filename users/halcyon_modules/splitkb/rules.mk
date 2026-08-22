VPATH += $(USER_PATH)/splitkb/
SRC += $(USER_PATH)/splitkb/halcyon.c
HALCONFDIR += $(USER_PATH)/splitkb/halconf.h
POST_CONFIG_H += $(USER_PATH)/splitkb/config.h

# USB resume instrumentation. RAM counters + an event ring with sequence numbers.
# Requires the matching guarded hunks in qmk_firmware's usb_main.c.
XTREEMZE_USB_EVENT_TRACE ?= no
ifeq ($(strip $(XTREEMZE_USB_EVENT_TRACE)),yes)
SRC += $(USER_PATH)/splitkb/xtreemze_usb_trace.c
OPT_DEFS += -DXTREEMZE_USB_EVENT_TRACE
endif

HLC_TRUE_VALUES := 1 yes true y
HLC_NONE_ENABLED := $(filter $(HLC_TRUE_VALUES),$(strip $(HLC_NONE)))
HLC_ENCODER_ENABLED := $(filter $(HLC_TRUE_VALUES),$(strip $(HLC_ENCODER)))
HLC_TFT_DISPLAY_ENABLED := $(filter $(HLC_TRUE_VALUES),$(strip $(HLC_TFT_DISPLAY)))
HLC_CIRQUE_TRACKPAD_ENABLED := $(filter $(HLC_TRUE_VALUES),$(strip $(HLC_CIRQUE_TRACKPAD)))

ifneq ($(HLC_ENCODER_ENABLED),)
  ENCODER_ENABLE = yes
  include $(USER_PATH)/splitkb/hlc_encoder/rules.mk
endif

ifneq ($(HLC_TFT_DISPLAY_ENABLED),)
  QUANTUM_PAINTER_ENABLE = yes
  QUANTUM_PAINTER_DRIVERS += st7789_spi surface
  include $(USER_PATH)/splitkb/hlc_tft_display/rules.mk
endif

ifneq ($(HLC_CIRQUE_TRACKPAD_ENABLED),)
  POINTING_DEVICE_ENABLE = yes
  POINTING_DEVICE_DRIVER = cirque_pinnacle_spi
  include $(USER_PATH)/splitkb/hlc_cirque_trackpad/rules.mk
endif

HLC_OPTIONS := $(HLC_NONE_ENABLED) $(HLC_CIRQUE_TRACKPAD_ENABLED) $(HLC_ENCODER_ENABLED) $(HLC_TFT_DISPLAY_ENABLED)

ifeq ($(strip $(HLC_OPTIONS)),)
$(error Halcyon_modules used but wrong or no module specified. Please specify one by adding `-e <module_name>=1` to your compile command where <module_name> can be: HLC_NONE, HLC_CIRQUE_TRACKPAD, HLC_ENCODER or HLC_TFT_DISPLAY)
endif
