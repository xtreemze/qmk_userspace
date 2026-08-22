// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

// Any QMK options should go here

#pragma once

#define HLC_TFT_DISPLAY

// LCD Configuration
#define LCD_RST_PIN GP26
#define LCD_CS_PIN GP13
#define LCD_DC_PIN GP16
#define LCD_SPI_DIVISOR 0
#define LCD_SPI_MODE 3
#define LCD_WIDTH 135
#define LCD_HEIGHT 240
#define LCD_ROTATION QP_ROTATION_0
#define LCD_OFFSET_X 52
#define LCD_OFFSET_Y 40

// QP Configuration
#define QUANTUM_PAINTER_SUPPORTS_NATIVE_COLORS TRUE
#define ST7789_NO_AUTOMATIC_VIEWPORT_OFFSETS
#define ST7789_NUM_DEVICES 1

#define SURFACE_NUM_DEVICES 1

// Backlight configuration
#undef BACKLIGHT_PIN
#define BACKLIGHT_PIN GP27

// Timeout configuration
#ifdef XTREEMZE_TFT_SOLID_FILL
// Keep the panel lit indefinitely so the fill can be watched without input.
#    undef HLC_BACKLIGHT_TIMEOUT
#    define HLC_BACKLIGHT_TIMEOUT 0xFFFFFFFFUL
#endif

// Disable Quantum Painter's own display timeout task.
//
// qp_internal_display_timeout_task() (quantum/painter/qp_internal.c) runs from
// quantum/main.c, walks every registered qp_devices[] entry and calls qp_power()
// on it based on last_input_activity_elapsed(). That is a second, independent
// owner of display power with no knowledge of the suspend/resume state machine:
// it can emit SPI while we are holding the controller in hardware reset during
// wake recovery, and it defeats the "no QP/SPI activity" premise of the
// TFT_NO_WAKE_RECOVERY diagnostic entirely.
//
// Setting it to 0 compiles that task out and leaves the module as the sole owner
// of qp_power(). The visible behaviour is unchanged: halcyon.c still blanks the
// backlight at HLC_BACKLIGHT_TIMEOUT, which is what the user actually sees. The
// panel simply no longer receives a redundant DISPLAY_OFF while already dark.
#define QUANTUM_PAINTER_DISPLAY_TIMEOUT 0
