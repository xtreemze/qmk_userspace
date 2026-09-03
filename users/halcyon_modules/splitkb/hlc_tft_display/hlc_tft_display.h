// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "qp.h"
#include "qp_surface.h"

// Everforest medium-dark RGB565 constants for easy future palette swaps.
#define EF_RGB565_BG     0x29A7
#define EF_RGB565_FG     0xD635
#define EF_RGB565_GREEN  0xA610
#define EF_RGB565_BLUE   0x7DD6
#define EF_RGB565_YELLOW 0xDDEF
#define EF_RGB565_RED    0xE3F0

// Quantum Painter drawing APIs use HSV; these are approximations of the RGB565 colors above.
#define HSV_EF_BG     146, 61, 59
#define HSV_EF_FG     29, 50, 211
#define HSV_EF_GREEN  59, 85, 192
#define HSV_EF_BLUE   122, 82, 187
#define HSV_EF_YELLOW 28, 107, 219
#define HSV_EF_RED    254, 115, 230
#define HSV_EF_DIM    98, 23, 146

#define HSV_LAYER_0 HSV_EF_GREEN
#define HSV_LAYER_1 HSV_EF_BLUE
#define HSV_LAYER_2 HSV_EF_YELLOW
#define HSV_LAYER_3 HSV_EF_RED
#define HSV_LAYER_4 HSV_EF_GREEN
#define HSV_LAYER_5 HSV_EF_BLUE
#define HSV_LAYER_6 HSV_EF_YELLOW
#define HSV_LAYER_7 HSV_EF_RED
#define HSV_LAYER_8  95, 88, 194
#define HSV_LAYER_9  170, 84, 182
#define HSV_LAYER_10 12, 96, 206
#define HSV_LAYER_11 210, 80, 186
#define HSV_LAYER_12 HSV_EF_FG
#define HSV_LAYER_UNDEF HSV_EF_FG

typedef enum {
    HALCYON_DISPLAY_OS_UNKNOWN = 0,
    HALCYON_DISPLAY_OS_MACOS,
    HALCYON_DISPLAY_OS_IOS,
    HALCYON_DISPLAY_OS_WINDOWS,
    HALCYON_DISPLAY_OS_LINUX,
} halcyon_display_os_t;

typedef enum {
    HALCYON_SHORTCUT_UNKNOWN = 0,
    HALCYON_SHORTCUT_APPLE,
    HALCYON_SHORTCUT_CTRL,
} halcyon_shortcut_family_t;

typedef enum {
    HALCYON_HOST_SOURCE_DEFAULT = 0,
    HALCYON_HOST_SOURCE_STORED,
    HALCYON_HOST_SOURCE_LIVE,
} halcyon_host_source_t;

typedef enum {
    HALCYON_HOST_EVENT_BOOT = 0,
    HALCYON_HOST_EVENT_RESUME,
    HALCYON_HOST_EVENT_CHANGE,
} halcyon_host_event_t;

typedef struct {
    halcyon_display_os_t os;
    halcyon_shortcut_family_t shortcut_family;
    halcyon_host_source_t source;
    halcyon_host_event_t event;
    bool is_master;
} halcyon_host_telemetry_t;

const char *halcyon_display_layer_name_user(uint8_t layer);
const char *halcyon_display_alt_repeat_text_user(void);
bool halcyon_display_host_telemetry_user(halcyon_host_telemetry_t *telemetry);
#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
void halcyon_display_toggle_trace_view(void);
#endif

bool update_display(void);
void backlight_wakeup(void);
void backlight_suspend(void);
