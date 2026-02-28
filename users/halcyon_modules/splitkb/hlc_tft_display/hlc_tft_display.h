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
#define HSV_LAYER_UNDEF HSV_EF_FG

extern painter_device_t lcd;
extern painter_device_t lcd_surface;
const char *halcyon_display_layer_name_user(uint8_t layer);
const char *halcyon_display_alt_repeat_text_user(void);

void draw_grid(void);
void update_grid(void);
void init_grid(void);
void add_cell_cluster(void);
uint8_t get_random_color_index(void);
void update_display(void);
void backlight_wakeup(void);
void backlight_suspend(void);
