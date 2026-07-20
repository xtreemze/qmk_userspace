// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "halcyon.h"
#include "hlc_tft_display.h"
#ifdef CAPS_WORD_ENABLE
#    include "caps_word.h"
#endif

#include <stdio.h>
#include <string.h>

// Fonts mono2
#include "graphics/fonts/Retron2000-27.qff.h"

static painter_font_handle_t Retron27;

static uint8_t lcd_surface_fb[SURFACE_REQUIRED_BUFFER_BYTE_SIZE(135, 240, 16)];

painter_device_t lcd;
painter_device_t lcd_surface;

static uint8_t last_mod_state = 0xFF;
static uint16_t last_visible_mod_mask = 0xFFFF;
static uint8_t last_display_layer = 0xFF;
static char last_arp_text[24] = "";
static uint32_t last_mod_seen[4] = { 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL };
static uint32_t last_background_redraw = 0;
static uint8_t last_background_layer = 0xFF;
static uint8_t pattern_animation_frame = 0;
static volatile bool display_wakeup_pending = false;

#define STATUS_X       5
#define STATUS_LAYER_Y 5
#define PATTERN_ANIMATION_FRAME_MS 200
#define MOD_RECENT_MS 2200
#define MOD_TS_UNSET 0xFFFFFFFFUL
#define MOD_INDICATOR_COLUMNS 2

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} hsv_triplet_t;

#define DISPLAY_LAYER_STYLE_COUNT 13

enum mod_indicator {
    MOD_IND_CTRL,
    MOD_IND_GUI,
    MOD_IND_SHIFT,
    MOD_IND_ALT,
    MOD_IND_MEH,
    MOD_IND_SHIFT_ALT,
    MOD_IND_SHIFT_GUI,
    MOD_IND_ALT_GUI,
    MOD_IND_HYPER,
    MOD_IND_CAPS_WORD,
    MOD_INDICATOR_COUNT,
};

static const char *const mod_indicator_labels[MOD_INDICATOR_COUNT] = {
    "Ctl", "Gui", "Shf", "Alt", "Meh", "S+A", "S+G", "A+G", "Hypr", "Caps",
};

static const hsv_triplet_t layer_fg_hsv[DISPLAY_LAYER_STYLE_COUNT] = {
    { 59, 85, 192 },  { 122, 82, 187 }, { 28, 107, 219 }, { 254, 115, 230 },
    { 95, 88, 194 },  { 170, 84, 182 }, { 12, 96, 206 },  { 210, 80, 186 },
    { 76, 82, 188 },  { 140, 76, 184 }, { 20, 104, 211 }, { 224, 78, 202 },
    { 29, 50, 211 },
};

static const hsv_triplet_t layer_bg_hsv[DISPLAY_LAYER_STYLE_COUNT] = {
    { 122, 36, 82 }, { 28, 34, 82 },  { 254, 34, 80 }, { 59, 32, 78 },
    { 210, 30, 78 }, { 95, 32, 78 },  { 170, 30, 78 }, { 12, 34, 80 },
    { 132, 30, 78 }, { 20, 34, 80 },  { 224, 30, 78 }, { 76, 32, 78 },
    { 146, 24, 66 },
};

static inline hsv_triplet_t layer_fg(uint8_t layer) {
    return layer_fg_hsv[layer % DISPLAY_LAYER_STYLE_COUNT];
}

static inline hsv_triplet_t layer_bg(uint8_t layer) {
    return layer_bg_hsv[layer % DISPLAY_LAYER_STYLE_COUNT];
}

static inline uint16_t mod_indicator_bit(uint8_t indicator) {
    return 1U << indicator;
}

static hsv_triplet_t mod_indicator_color(uint8_t indicator, bool active) {
    if (!active) {
        return (hsv_triplet_t){ 98, 23, 146 };
    }

    switch (indicator) {
        case MOD_IND_CTRL:
            return (hsv_triplet_t){ 122, 82, 187 };
        case MOD_IND_GUI:
            return (hsv_triplet_t){ 254, 115, 230 };
        case MOD_IND_SHIFT:
            return (hsv_triplet_t){ 28, 107, 219 };
        case MOD_IND_ALT:
            return (hsv_triplet_t){ 59, 85, 192 };
        case MOD_IND_MEH:
            return (hsv_triplet_t){ 170, 84, 182 };
        case MOD_IND_SHIFT_ALT:
            return (hsv_triplet_t){ 20, 104, 211 };
        case MOD_IND_SHIFT_GUI:
            return (hsv_triplet_t){ 224, 78, 202 };
        case MOD_IND_ALT_GUI:
            return (hsv_triplet_t){ 76, 82, 188 };
        case MOD_IND_HYPER:
            return (hsv_triplet_t){ 12, 96, 206 };
        case MOD_IND_CAPS_WORD:
            return (hsv_triplet_t){ 95, 88, 194 };
        default:
            return (hsv_triplet_t){ 29, 50, 211 };
    }
}

static bool mod_is_recent(uint32_t ts) {
    return ts != MOD_TS_UNSET && timer_elapsed32(ts) < MOD_RECENT_MS;
}

static uint16_t build_mod_indicator_masks(uint8_t active_mods, uint16_t *active_indicator_mask) {
    uint16_t active_mask = 0;
    uint16_t visible_mask = 0;

    const bool ctrl_active = (active_mods & MOD_MASK_CTRL) != 0;
    const bool gui_active = (active_mods & MOD_MASK_GUI) != 0;
    const bool shift_active = (active_mods & MOD_MASK_SHIFT) != 0;
    const bool alt_active = (active_mods & MOD_MASK_ALT) != 0;

    if (ctrl_active) {
        active_mask |= mod_indicator_bit(MOD_IND_CTRL);
        last_mod_seen[0] = timer_read32();
    }
    if (gui_active) {
        active_mask |= mod_indicator_bit(MOD_IND_GUI);
        last_mod_seen[1] = timer_read32();
    }
    if (shift_active) {
        active_mask |= mod_indicator_bit(MOD_IND_SHIFT);
        last_mod_seen[2] = timer_read32();
    }
    if (alt_active) {
        active_mask |= mod_indicator_bit(MOD_IND_ALT);
        last_mod_seen[3] = timer_read32();
    }

    if (ctrl_active || mod_is_recent(last_mod_seen[0])) {
        visible_mask |= mod_indicator_bit(MOD_IND_CTRL);
    }
    if (gui_active || mod_is_recent(last_mod_seen[1])) {
        visible_mask |= mod_indicator_bit(MOD_IND_GUI);
    }
    if (shift_active || mod_is_recent(last_mod_seen[2])) {
        visible_mask |= mod_indicator_bit(MOD_IND_SHIFT);
    }
    if (alt_active || mod_is_recent(last_mod_seen[3])) {
        visible_mask |= mod_indicator_bit(MOD_IND_ALT);
    }

    if (ctrl_active && gui_active && shift_active && alt_active) {
        active_mask |= mod_indicator_bit(MOD_IND_HYPER);
        visible_mask |= mod_indicator_bit(MOD_IND_HYPER);
    } else if (ctrl_active && shift_active && alt_active && !gui_active) {
        active_mask |= mod_indicator_bit(MOD_IND_MEH);
        visible_mask |= mod_indicator_bit(MOD_IND_MEH);
    } else if (!ctrl_active && !gui_active && shift_active && alt_active) {
        active_mask |= mod_indicator_bit(MOD_IND_SHIFT_ALT);
        visible_mask |= mod_indicator_bit(MOD_IND_SHIFT_ALT);
    } else if (!ctrl_active && !alt_active && shift_active && gui_active) {
        active_mask |= mod_indicator_bit(MOD_IND_SHIFT_GUI);
        visible_mask |= mod_indicator_bit(MOD_IND_SHIFT_GUI);
    } else if (!ctrl_active && !shift_active && alt_active && gui_active) {
        active_mask |= mod_indicator_bit(MOD_IND_ALT_GUI);
        visible_mask |= mod_indicator_bit(MOD_IND_ALT_GUI);
    }

#ifdef CAPS_WORD_ENABLE
    if (is_caps_word_on()) {
        active_mask |= mod_indicator_bit(MOD_IND_CAPS_WORD);
        visible_mask |= mod_indicator_bit(MOD_IND_CAPS_WORD);
    }
#endif

    *active_indicator_mask = active_mask;
    return visible_mask;
}

static void draw_mod_indicators(uint16_t visible_mod_mask, uint16_t active_mod_mask) {
    const uint8_t rows = (MOD_INDICATOR_COUNT + MOD_INDICATOR_COLUMNS - 1) / MOD_INDICATOR_COLUMNS;
    const uint16_t col_width = LCD_WIDTH / MOD_INDICATOR_COLUMNS;
    const uint16_t mod_top = LCD_HEIGHT - (Retron27->line_height * rows) - 8;

    qp_rect(lcd_surface, 0, mod_top - 4, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);

    uint8_t drawn = 0;
    for (uint8_t indicator = 0; indicator < MOD_INDICATOR_COUNT; ++indicator) {
        const uint16_t bit = mod_indicator_bit(indicator);
        if ((visible_mod_mask & bit) == 0) {
            continue;
        }

        const uint8_t row = drawn / MOD_INDICATOR_COLUMNS;
        const uint8_t col = drawn % MOD_INDICATOR_COLUMNS;
        const bool active = (active_mod_mask & bit) != 0;
        const hsv_triplet_t color = mod_indicator_color(indicator, active);

        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X + (col * col_width),
            mod_top + (Retron27->line_height * row),
            Retron27,
            mod_indicator_labels[indicator],
            color.h, color.s, color.v,
            HSV_EF_BG
        );
        drawn++;
    }
}

__attribute__((weak)) const char *halcyon_display_layer_name_user(uint8_t layer) {
    static const char *const fallback_layer_names[] = {
        "LAYA", "LAYB", "LAYC", "LAYD", "LAYE", "LAYF", "LAYG",
        "LAYH", "LAYI", "LAYJ", "LAYK", "LAYL", "LAYM"
    };

    if (layer < ARRAY_SIZE(fallback_layer_names)) {
        return fallback_layer_names[layer];
    }

    return "LAYX";
}

__attribute__((weak)) const char *halcyon_display_alt_repeat_text_user(void) {
    return "---";
}

static void draw_diamond(uint16_t cx, uint16_t cy, uint8_t radius, uint8_t h, uint8_t s, uint8_t v) {
    for (int8_t dy = -(int8_t)radius; dy <= (int8_t)radius; ++dy) {
        int8_t span = (int8_t)radius - (dy < 0 ? -dy : dy);
        int16_t x0 = (int16_t)cx - span;
        int16_t x1 = (int16_t)cx + span;
        int16_t y = (int16_t)cy + dy;

        if (y < 0 || y >= LCD_HEIGHT) {
            continue;
        }
        if (x0 < 0) {
            x0 = 0;
        }
        if (x1 >= LCD_WIDTH) {
            x1 = LCD_WIDTH - 1;
        }
        if (x0 <= x1) {
            qp_line(lcd_surface, (uint16_t)x0, (uint16_t)y, (uint16_t)x1, (uint16_t)y, h, s, v);
        }
    }
}

static int8_t pattern_motion_offset(uint8_t frame, uint16_t tile_index, uint8_t layer, bool vertical) {
    const uint8_t step = (uint8_t)((frame + tile_index + (vertical ? layer : layer * 2U)) & 0x03U);
    switch (step) {
        case 1:
            return 1;
        case 3:
            return -1;
        default:
            return 0;
    }
}

static void draw_layer_background_pattern(uint8_t layer, uint8_t frame) {
    const uint8_t tile_w = 24;
    const uint8_t tile_h = 24;
    const uint8_t variant = layer % DISPLAY_LAYER_STYLE_COUNT;
    const hsv_triplet_t fg = layer_fg(layer);
    const hsv_triplet_t bg = layer_bg(layer);

    qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);

    for (uint16_t y = 0; y + tile_h <= LCD_HEIGHT; y += tile_h) {
        for (uint16_t x = 0; x < LCD_WIDTH; x += tile_w) {
            const uint16_t tile_index = (x / tile_w) + (y / tile_h);
            const bool phase = ((tile_index + layer + (frame >> 1)) & 1U) != 0U;
            const int8_t dx = pattern_motion_offset(frame, tile_index, layer, false);
            const int8_t dy = pattern_motion_offset(frame, tile_index, layer, true);
            const uint16_t cx = (uint16_t)((int16_t)x + tile_w / 2 + dx);
            const uint16_t cy = (uint16_t)((int16_t)y + tile_h / 2 + dy);
            const int8_t pulse = ((frame + tile_index + layer) & 0x03U) == 0 ? 1 : 0;
            const uint8_t ah = phase ? fg.h : bg.h;
            const uint8_t as = phase ? fg.s : bg.s;
            const uint8_t av = phase ? fg.v : bg.v;
            const uint8_t bh = phase ? bg.h : fg.h;
            const uint8_t bs = phase ? bg.s : fg.s;
            const uint8_t bv = phase ? bg.v : fg.v;

            // All variants are tiled diamond motifs and remain four-way symmetric.
            switch (variant) {
                case 0:
                    draw_diamond(cx, cy, 6 + pulse, ah, as, av);
                    draw_diamond(cx, cy, 2, bh, bs, bv);
                    break;
                case 1:
                    draw_diamond(cx, cy - 6, 3, ah, as, av);
                    draw_diamond(cx - 6, cy, 3, ah, as, av);
                    draw_diamond(cx + 6, cy, 3, ah, as, av);
                    draw_diamond(cx, cy + 6, 3, ah, as, av);
                    draw_diamond(cx, cy, 2, bh, bs, bv);
                    break;
                case 2:
                    draw_diamond(cx, cy, 7 + pulse, ah, as, av);
                    draw_diamond(cx, cy, 5, HSV_EF_BG);
                    draw_diamond(cx, cy, 2, bh, bs, bv);
                    break;
                case 3:
                    draw_diamond(cx - 5, cy, 4, ah, as, av);
                    draw_diamond(cx + 5, cy, 4, ah, as, av);
                    draw_diamond(cx, cy, 2, bh, bs, bv);
                    break;
                case 4:
                    draw_diamond(cx, cy - 5, 4, ah, as, av);
                    draw_diamond(cx, cy + 5, 4, ah, as, av);
                    draw_diamond(cx, cy, 2, bh, bs, bv);
                    break;
                case 5:
                    draw_diamond(cx, cy, 3, ah, as, av);
                    draw_diamond(cx - 7, cy - 7, 2, bh, bs, bv);
                    draw_diamond(cx + 7, cy - 7, 2, bh, bs, bv);
                    draw_diamond(cx - 7, cy + 7, 2, bh, bs, bv);
                    draw_diamond(cx + 7, cy + 7, 2, bh, bs, bv);
                    break;
                case 6:
                    draw_diamond(cx - 6, cy - 6, 3, ah, as, av);
                    draw_diamond(cx + 6, cy + 6, 3, ah, as, av);
                    draw_diamond(cx + 6, cy - 6, 2, bh, bs, bv);
                    draw_diamond(cx - 6, cy + 6, 2, bh, bs, bv);
                    break;
                case 7:
                    draw_diamond(cx, cy, 5, ah, as, av);
                    draw_diamond(x + 2, cy, 2, bh, bs, bv);
                    draw_diamond(x + tile_w - 3, cy, 2, bh, bs, bv);
                    break;
                case 8:
                    draw_diamond(cx, y + 3, 2, ah, as, av);
                    draw_diamond(cx, y + tile_h - 4, 2, ah, as, av);
                    draw_diamond(cx, cy, 4, bh, bs, bv);
                    break;
                case 9:
                    draw_diamond(cx, cy, 3, ah, as, av);
                    draw_diamond(cx - 6, cy, 2, bh, bs, bv);
                    draw_diamond(cx + 6, cy, 2, bh, bs, bv);
                    draw_diamond(cx, cy - 6, 2, bh, bs, bv);
                    draw_diamond(cx, cy + 6, 2, bh, bs, bv);
                    break;
                case 10:
                    draw_diamond(cx, cy, 6 + pulse, ah, as, av);
                    draw_diamond(cx - 8, cy, 2, bh, bs, bv);
                    draw_diamond(cx + 8, cy, 2, bh, bs, bv);
                    break;
                case 11:
                    draw_diamond(cx, cy, 4, ah, as, av);
                    draw_diamond(cx - 8, cy - 8, 2, bh, bs, bv);
                    draw_diamond(cx + 8, cy - 8, 2, bh, bs, bv);
                    draw_diamond(cx - 8, cy + 8, 2, bh, bs, bv);
                    draw_diamond(cx + 8, cy + 8, 2, bh, bs, bv);
                    break;
                default:
                    draw_diamond(cx - 5, cy - 5, 3, ah, as, av);
                    draw_diamond(cx + 5, cy + 5, 3, ah, as, av);
                    draw_diamond(cx, cy, 2, bh, bs, bv);
                    break;
            }
        }
    }
}

bool update_display(void) {
    static bool fonts_loaded = false;
    bool display_dirty = false;

    if (!fonts_loaded) {
        Retron27 = qp_load_font_mem(font_Retron2000_27);
        fonts_loaded = true;
    }

    const uint8_t active_layer = get_highest_layer(layer_state | default_layer_state);
    const uint8_t active_mods = get_mods() | get_weak_mods() | get_oneshot_mods() | get_oneshot_locked_mods();
    const uint32_t now = timer_read32();
    const char *const arp_text_raw = halcyon_display_alt_repeat_text_user();
    const char *const arp_text = arp_text_raw != NULL ? arp_text_raw : "";
    const bool has_arp_text = arp_text[0] != '\0';
    const bool first_run = (last_display_layer == 0xFF);
    const bool arp_changed = strcmp(last_arp_text, arp_text) != 0;
    uint16_t active_mod_indicator_mask = 0;
    const uint16_t visible_mod_mask = build_mod_indicator_masks(active_mods, &active_mod_indicator_mask);

    const bool layer_changed = active_layer != last_display_layer;
    const bool need_full_redraw = first_run || layer_changed;
    const bool mod_overlay_expired = last_visible_mod_mask != 0U && visible_mod_mask == 0U;
    const bool animation_due = timer_elapsed32(last_background_redraw) >= PATTERN_ANIMATION_FRAME_MS;
    const bool need_background_redraw = first_run || active_layer != last_background_layer || animation_due;
    bool background_redrawn = false;

    if (need_background_redraw) {
        if (first_run || layer_changed) {
            pattern_animation_frame = active_layer;
        } else {
            pattern_animation_frame++;
        }
        draw_layer_background_pattern(active_layer, pattern_animation_frame);
        last_background_layer = active_layer;
        last_background_redraw = now;
        background_redrawn = true;
        display_dirty = true;
    }

    if (background_redrawn || arp_changed || need_full_redraw) {
        qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, Retron27->line_height + 12, HSV_EF_BG, true);

        const hsv_triplet_t layer_color = layer_fg(active_layer);

        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            STATUS_LAYER_Y,
            Retron27,
            halcyon_display_layer_name_user(active_layer),
            layer_color.h, layer_color.s, layer_color.v,
            HSV_EF_BG
        );

        if (has_arp_text) {
            const int16_t arp_width = qp_textwidth(Retron27, arp_text);
            int16_t arp_x = (int16_t)LCD_WIDTH - (int16_t)STATUS_X - arp_width;
            if (arp_x < STATUS_X) {
                arp_x = STATUS_X;
            }
            qp_drawtext_recolor(lcd_surface, (uint16_t)arp_x, STATUS_LAYER_Y, Retron27, arp_text, 122, 82, 187, HSV_EF_BG);
        }

        last_display_layer = active_layer;
        display_dirty = true;
    }

    if (mod_overlay_expired) {
        const uint8_t rows = (MOD_INDICATOR_COUNT + MOD_INDICATOR_COLUMNS - 1) / MOD_INDICATOR_COLUMNS;
        const uint16_t mod_top = LCD_HEIGHT - (Retron27->line_height * rows) - 8;
        qp_rect(lcd_surface, 0, mod_top - 4, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);
        display_dirty = true;
    }

    if (visible_mod_mask != 0U && (first_run || background_redrawn || active_mods != last_mod_state || visible_mod_mask != last_visible_mod_mask)) {
        draw_mod_indicators(visible_mod_mask, active_mod_indicator_mask);
        last_mod_state = active_mods;
        display_dirty = true;
    }

    last_visible_mod_mask = visible_mod_mask;

    if (arp_changed) {
        snprintf(last_arp_text, sizeof(last_arp_text), "%s", arp_text);
    }

    return display_dirty;
}

// Called from halcyon.c
void module_suspend_power_down_kb(void) {
    backlight_suspend();
    qp_power(lcd, false);
}

// Called from halcyon.c
void module_suspend_wakeup_init_kb(void) {
    // QMK invokes this callback from the USB wake ISR on ChibiOS. Defer the
    // SPI transaction until housekeeping runs in normal thread context.
    display_wakeup_pending = true;
}

// Called from halcyon.c
bool module_post_init_kb(void) {
    backlight_wakeup();

    // Make the devices
    lcd = qp_st7789_make_spi_device(LCD_WIDTH, LCD_HEIGHT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_SPI_DIVISOR, LCD_SPI_MODE);
    lcd_surface = qp_make_rgb565_surface(LCD_WIDTH, LCD_HEIGHT, lcd_surface_fb);

    // Initialise the LCD
    qp_init(lcd, LCD_ROTATION);
    qp_set_viewport_offsets(lcd, LCD_OFFSET_X, LCD_OFFSET_Y);
    qp_clear(lcd);
    qp_rect(lcd, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);
    qp_power(lcd, true);
    qp_flush(lcd);

    // Initialise the LCD surface
    qp_init(lcd_surface, LCD_ROTATION);
    qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);
    qp_surface_draw(lcd_surface, lcd, 0, 0, false);
    qp_flush(lcd);

    if(!module_post_init_user()) { return false; }

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

        qp_surface_draw(lcd_surface, lcd, 0, 0, false);
    }

    if(!display_module_housekeeping_task_user(second_display)) { return false; }

    bool display_dirty = false;

    if (last_input_activity_elapsed() >= HLC_BACKLIGHT_TIMEOUT) {
        return true;
    }

    display_dirty = update_display();

    if (display_dirty) {
        qp_surface_draw(lcd_surface, lcd, 0, 0, false);
    }

    return true;
}
