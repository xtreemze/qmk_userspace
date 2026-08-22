// Copyright 2025 Carlos Velasco
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Panel fault diagnostic build for the Halcyon TFT module.
//
// This replaces the entire production display module with a full-screen solid
// colour fill. No fonts, no glyphs, no layer patterns, no host telemetry, no
// surface blit -- just qp_rect() straight to the ST7789 followed by qp_flush().
//
// If the fixed horizontal lines still cross a uniform full-screen fill, there
// is no firmware drawing code left that could be producing them, which points
// at the panel/row-driver hardware. Cycling through several colours also shows
// whether the affected rows are stuck bright, stuck dark, or colour dependent.
//
// Enable with: -e XTREEMZE_TFT_SOLID_FILL=yes

#include "halcyon.h"
#include "hlc_tft_display.h"

painter_device_t lcd;
painter_device_t lcd_surface; // Unused here; kept so the module header stays satisfied.

#define SOLID_FILL_HOLD_MS 2000

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} solid_fill_color_t;

// Order matters: white first so the lines are immediately obvious on power-up.
static const solid_fill_color_t solid_fill_colors[] = {
    {   0,   0, 255 }, // white
    {   0,   0,   0 }, // black
    {   0, 255, 255 }, // red
    {  85, 255, 255 }, // green
    { 170, 255, 255 }, // blue
    {   0,   0, 128 }, // mid grey
};

#define SOLID_FILL_COLOR_COUNT (sizeof(solid_fill_colors) / sizeof(solid_fill_colors[0]))

static uint8_t  solid_fill_index    = 0;
static uint32_t solid_fill_last     = 0;
static bool     solid_fill_pending  = true;
static volatile bool display_wakeup_pending = false;

static void solid_fill_paint(void) {
    const solid_fill_color_t color = solid_fill_colors[solid_fill_index];

    qp_rect(lcd, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color.h, color.s, color.v, true);
    qp_flush(lcd);
}

// Called from halcyon.c
void module_suspend_power_down_kb(void) {
    backlight_suspend();
    qp_power(lcd, false);
}

// Called from halcyon.c
void module_suspend_wakeup_init_kb(void) {
    // Deferred out of the USB wake ISR, same as the production module.
    display_wakeup_pending = true;
}

// Called from halcyon.c
bool module_post_init_kb(void) {
    backlight_wakeup();

    lcd = qp_st7789_make_spi_device(LCD_WIDTH, LCD_HEIGHT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_SPI_DIVISOR, LCD_SPI_MODE);

    qp_init(lcd, LCD_ROTATION);
    qp_set_viewport_offsets(lcd, LCD_OFFSET_X, LCD_OFFSET_Y);
    qp_clear(lcd);
    qp_power(lcd, true);

    solid_fill_index   = 0;
    solid_fill_last    = timer_read32();
    solid_fill_pending = true;

    return true;
}

// Called from halcyon.c
bool display_module_housekeeping_task_kb(bool second_display) {
    if (display_wakeup_pending) {
        display_wakeup_pending = false;

        if (!qp_power(lcd, true)) {
            display_wakeup_pending = true;
            return true;
        }

        solid_fill_pending = true;
    }

    if (timer_elapsed32(solid_fill_last) >= SOLID_FILL_HOLD_MS) {
        solid_fill_last    = timer_read32();
        solid_fill_index   = (uint8_t)((solid_fill_index + 1) % SOLID_FILL_COLOR_COUNT);
        solid_fill_pending = true;
    }

    if (solid_fill_pending) {
        solid_fill_pending = false;
        solid_fill_paint();
    }

    return true;
}
