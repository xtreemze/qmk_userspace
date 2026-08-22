// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "halcyon.h"
#include "hlc_tft_display.h"
#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
#    include "os_detection.h"
#endif
#ifdef CAPS_WORD_ENABLE
#    include "caps_word.h"
#endif

#include <stdio.h>
#include <string.h>

// Fonts mono2
#include "graphics/fonts/Retron2000-27.qff.h"

static painter_font_handle_t Retron27;
static bool display_font_loaded = false;

static uint8_t lcd_surface_fb[SURFACE_REQUIRED_BUFFER_BYTE_SIZE(135, 240, 16)];

painter_device_t lcd;
painter_device_t lcd_surface;

typedef enum {
    DISPLAY_MODE_NORMAL = 0,
    DISPLAY_MODE_HOST_OVERLAY,
    DISPLAY_MODE_TRACE_VIEW,
} display_mode_t;

// USB suspend is treated as a real display shutdown boundary rather than a
// DISPLAY_OFF/DISPLAY_ON pair. There is no firmware-controlled VIK 3.3V switch
// on this board, so the closest we can get to depowering the ST7789 is holding
// its dedicated reset line low for the whole suspend, then bringing the
// controller back through a full initialisation on wake.
typedef enum {
    TFT_POWER_ACTIVE = 0,   // Initialised, safe to draw.
    TFT_POWER_SUSPENDED,    // RST held low, no SPI traffic permitted.
    TFT_POWER_WAKE_RESET,   // RST low, waiting out the reset pulse width.
    TFT_POWER_WAKE_INIT,    // RST released, waiting for controller recovery.
    TFT_POWER_WAKE_RETRY,   // Re-init failed, backing off before another cycle.
    TFT_POWER_FAILED,       // Gave up; panel stays dark, keyboard keeps working.
} tft_power_state_t;

static uint8_t last_mod_state = 0xFF;
static uint16_t last_visible_mod_mask = 0xFFFF;
static uint8_t last_display_layer = 0xFF;
static char last_arp_text[24] = "";
static uint32_t last_mod_seen[4] = { 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL };
static uint32_t last_background_redraw = 0;
static uint8_t last_background_layer = 0xFF;
static uint8_t pattern_animation_frame = 0;
static volatile bool display_wakeup_pending = false;
static tft_power_state_t tft_power_state = TFT_POWER_ACTIVE;
static uint32_t tft_power_timer = 0;
static uint8_t tft_recovery_attempts = 0;
static volatile bool host_resume_pending = false;
static bool host_event_pending = true;
static halcyon_host_event_t pending_host_event = HALCYON_HOST_EVENT_BOOT;
static display_mode_t display_mode = DISPLAY_MODE_NORMAL;
static uint32_t host_overlay_started = 0;
static uint32_t last_host_overlay_frame = 0;
static bool has_observed_host_telemetry = false;
static halcyon_host_telemetry_t observed_host_telemetry;
static halcyon_host_telemetry_t host_overlay_telemetry;
#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
static uint32_t os_fingerprint_page_started = 0;
static uint32_t last_os_fingerprint_frame = 0;
#endif

#define STATUS_X       5
#define STATUS_LAYER_Y 5
#define PATTERN_ANIMATION_FRAME_MS 200
#define HOST_PANEL_FRAME_MS 90
#define HOST_PANEL_INTRO_MS 200
#define HOST_PANEL_STABLE_MS 400
#define HOST_PANEL_PULSE_MS 1700
#define HOST_OVERLAY_DURATION_MS 2000
#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
#    define OS_FINGERPRINT_PAGE_MS 2000
#    define OS_FINGERPRINT_FRAME_MS 200
#endif
#define MOD_RECENT_MS 2200
// ST7789 reset pulse width; the datasheet minimum is ~10us, QMK's own comms
// init uses 20ms, so match that rather than inventing a shorter one.
#define TFT_RESET_LOW_MS 20
// Reset-release recovery before the controller will accept commands. The init
// sequence issues its own SWRESET + 120ms afterwards, so this is deliberately
// conservative rather than tight.
#define TFT_RESET_RECOVERY_MS 120
#define TFT_RECOVERY_RETRY_MS 1000
// Bounded so a dead panel cannot inflict a ~145ms init stall every housekeeping
// tick forever. Worst case is 5 attempts spread over ~5s, then permanently
// quiet. Note that qp_clear() can only fail at the comms layer -- this SPI
// config has no read-back path, so a controller that silently ignores commands
// is not detectable and will look like a successful recovery.
#define TFT_RECOVERY_MAX_ATTEMPTS 5
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

__attribute__((weak)) bool halcyon_display_host_telemetry_user(halcyon_host_telemetry_t *telemetry) {
    if (telemetry == NULL) {
        return false;
    }

    *telemetry = (halcyon_host_telemetry_t){
        .os              = HALCYON_DISPLAY_OS_UNKNOWN,
        .shortcut_family = HALCYON_SHORTCUT_UNKNOWN,
        .source          = HALCYON_HOST_SOURCE_DEFAULT,
        .event           = HALCYON_HOST_EVENT_BOOT,
        .is_master       = is_keyboard_master(),
    };
    return true;
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

static void ensure_display_font_loaded(void) {
    if (!display_font_loaded) {
        Retron27 = qp_load_font_mem(font_Retron2000_27);
        display_font_loaded = true;
    }
}

static const char *host_os_label(halcyon_display_os_t os) {
    switch (os) {
        case HALCYON_DISPLAY_OS_MACOS:
            return "macOS";
        case HALCYON_DISPLAY_OS_IOS:
            return "iOS";
        case HALCYON_DISPLAY_OS_WINDOWS:
            return "Windows";
        case HALCYON_DISPLAY_OS_LINUX:
            return "Linux";
        case HALCYON_DISPLAY_OS_UNKNOWN:
        default:
            return "DETECTING";
    }
}

static const char *host_os_compact_label(halcyon_display_os_t os) {
    switch (os) {
        case HALCYON_DISPLAY_OS_WINDOWS:
            return "WIN";
        case HALCYON_DISPLAY_OS_UNKNOWN:
            return "DETECT";
        default:
            return host_os_label(os);
    }
}

static const char *host_marker_label(halcyon_display_os_t os) {
    switch (os) {
        case HALCYON_DISPLAY_OS_MACOS:
            return "MAC";
        case HALCYON_DISPLAY_OS_IOS:
            return "IOS";
        case HALCYON_DISPLAY_OS_WINDOWS:
            return "WIN";
        case HALCYON_DISPLAY_OS_LINUX:
            return "LIN";
        case HALCYON_DISPLAY_OS_UNKNOWN:
        default:
            return "?";
    }
}

static const char *host_shortcut_label(halcyon_shortcut_family_t family) {
    switch (family) {
        case HALCYON_SHORTCUT_APPLE:
            return "APPLE";
        case HALCYON_SHORTCUT_CTRL:
            return "CTRL";
        case HALCYON_SHORTCUT_UNKNOWN:
        default:
            return "CTRL";
    }
}

static const char *host_shortcut_compact_label(halcyon_shortcut_family_t family) {
    return family == HALCYON_SHORTCUT_APPLE ? "CMD" : "CTL";
}

static const char *host_source_label(halcyon_host_source_t source) {
    switch (source) {
        case HALCYON_HOST_SOURCE_STORED:
            return "STORED";
        case HALCYON_HOST_SOURCE_LIVE:
            return "QMK";
        case HALCYON_HOST_SOURCE_DEFAULT:
        default:
            return "DEFAULT";
    }
}

static const char *host_source_compact_label(halcyon_host_source_t source) {
    switch (source) {
        case HALCYON_HOST_SOURCE_STORED:
            return "STOR";
        case HALCYON_HOST_SOURCE_LIVE:
            return "QMK";
        case HALCYON_HOST_SOURCE_DEFAULT:
        default:
            return "DEF";
    }
}

static const char *host_event_label(halcyon_host_event_t event) {
    switch (event) {
        case HALCYON_HOST_EVENT_RESUME:
            return "RESUME";
        case HALCYON_HOST_EVENT_CHANGE:
            return "CHANGE";
        case HALCYON_HOST_EVENT_BOOT:
        default:
            return "BOOT";
    }
}

static const char *host_event_compact_label(halcyon_host_event_t event) {
    switch (event) {
        case HALCYON_HOST_EVENT_RESUME:
            return "WAKE";
        case HALCYON_HOST_EVENT_CHANGE:
            return "CHG";
        case HALCYON_HOST_EVENT_BOOT:
        default:
            return "BOOT";
    }
}

static hsv_triplet_t host_accent(const halcyon_host_telemetry_t *telemetry) {
    hsv_triplet_t accent;

    switch (telemetry->os) {
        case HALCYON_DISPLAY_OS_MACOS:
        case HALCYON_DISPLAY_OS_IOS:
            accent = (hsv_triplet_t){ 122, 82, 187 };
            break;
        case HALCYON_DISPLAY_OS_WINDOWS:
            accent = (hsv_triplet_t){ 59, 85, 192 };
            break;
        case HALCYON_DISPLAY_OS_LINUX:
            accent = (hsv_triplet_t){ 28, 107, 219 };
            break;
        case HALCYON_DISPLAY_OS_UNKNOWN:
        default:
            accent = (hsv_triplet_t){ 254, 115, 230 };
            break;
    }

    if (telemetry->source == HALCYON_HOST_SOURCE_STORED) {
        accent.v = (uint8_t)((uint16_t)accent.v * 3U / 4U);
    } else if (telemetry->source == HALCYON_HOST_SOURCE_DEFAULT) {
        accent.h = 254;
        accent.s = 80;
        accent.v = 150;
    }

    return accent;
}

static void draw_centered_host_text(uint16_t y, const char *text, hsv_triplet_t color) {
    const int16_t width = qp_textwidth(Retron27, text);
    const uint16_t x = width < LCD_WIDTH ? (uint16_t)((LCD_WIDTH - width) / 2) : 0;
    qp_drawtext_recolor(lcd_surface, x, y, Retron27, text, color.h, color.s, color.v, HSV_EF_BG);
}

static const char *fit_host_line(const char *full, const char *compact) {
    return qp_textwidth(Retron27, full) <= LCD_WIDTH ? full : compact;
}

static void draw_host_overlay(uint32_t elapsed) {
    const hsv_triplet_t accent = host_accent(&host_overlay_telemetry);
    const hsv_triplet_t text = { 29, 50, 211 };
    const bool final_pulse = elapsed >= HOST_PANEL_PULSE_MS;
    const uint8_t pulse_v = final_pulse && (elapsed / HOST_PANEL_FRAME_MS) % 2U != 0U ? (uint8_t)(accent.v * 3U / 4U) : accent.v;
    char policy_full[32];
    char policy_compact[24];
    char lifecycle_full[32];
    char lifecycle_compact[24];

    snprintf(policy_full, sizeof(policy_full), "%s  %s", host_shortcut_label(host_overlay_telemetry.shortcut_family), host_source_label(host_overlay_telemetry.source));
    snprintf(policy_compact, sizeof(policy_compact), "%s %s", host_shortcut_compact_label(host_overlay_telemetry.shortcut_family), host_source_compact_label(host_overlay_telemetry.source));
    snprintf(lifecycle_full, sizeof(lifecycle_full), "%s  %s", host_event_label(host_overlay_telemetry.event), host_overlay_telemetry.is_master ? "MASTER" : "SLAVE");
    snprintf(lifecycle_compact, sizeof(lifecycle_compact), "%s %s", host_event_compact_label(host_overlay_telemetry.event), host_overlay_telemetry.is_master ? "MST" : "SLV");

    const char *const os_line = fit_host_line(host_os_label(host_overlay_telemetry.os), host_os_compact_label(host_overlay_telemetry.os));
    const char *const policy_line = fit_host_line(policy_full, policy_compact);
    const char *const lifecycle_line = fit_host_line(lifecycle_full, lifecycle_compact);

    qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);

    if (elapsed < HOST_PANEL_INTRO_MS) {
        uint8_t radius = (uint8_t)(4U + elapsed / 12U);
        if (radius > 19U) {
            radius = 19U;
        }
        draw_diamond(LCD_WIDTH / 2, LCD_HEIGHT / 2, radius, accent.h, accent.s, pulse_v);
        return;
    }

    draw_diamond(LCD_WIDTH / 2, 27, 6, accent.h, accent.s, pulse_v);
    draw_diamond(LCD_WIDTH / 2, LCD_HEIGHT - 28, 6, accent.h, accent.s, pulse_v);
    draw_centered_host_text(55, os_line, accent);

    if (elapsed >= HOST_PANEL_STABLE_MS) {
        draw_centered_host_text(108, policy_line, text);
        draw_centered_host_text(161, lifecycle_line, text);
    }
}

static bool host_telemetry_equal(const halcyon_host_telemetry_t *a, const halcyon_host_telemetry_t *b) {
    return a->os == b->os && a->shortcut_family == b->shortcut_family && a->source == b->source && a->is_master == b->is_master;
}

static halcyon_host_telemetry_t read_host_telemetry(halcyon_host_event_t event) {
    halcyon_host_telemetry_t telemetry = {
        .os              = HALCYON_DISPLAY_OS_UNKNOWN,
        .shortcut_family = HALCYON_SHORTCUT_UNKNOWN,
        .source          = HALCYON_HOST_SOURCE_DEFAULT,
        .event           = event,
        .is_master       = is_keyboard_master(),
    };

    halcyon_display_host_telemetry_user(&telemetry);
    telemetry.event = event;
    return telemetry;
}

static void start_host_overlay(halcyon_host_event_t event, const halcyon_host_telemetry_t *telemetry) {
    host_overlay_telemetry = *telemetry;
    host_overlay_telemetry.event = event;
    host_overlay_started = timer_read32();
    last_host_overlay_frame = host_overlay_started - HOST_PANEL_FRAME_MS;
    display_mode = DISPLAY_MODE_HOST_OVERLAY;
}

static void reset_normal_display_cache(void) {
    last_mod_state = 0xFF;
    last_visible_mod_mask = 0xFFFF;
    last_display_layer = 0xFF;
    last_background_layer = 0xFF;
    last_background_redraw = 0;
    last_arp_text[0] = '\0';
}

#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
void halcyon_display_toggle_trace_view(void) {
    if (display_mode == DISPLAY_MODE_TRACE_VIEW) {
        display_mode = DISPLAY_MODE_NORMAL;
        reset_normal_display_cache();
        return;
    }

    display_mode = DISPLAY_MODE_TRACE_VIEW;
    os_fingerprint_page_started = timer_read32();
    last_os_fingerprint_frame = os_fingerprint_page_started - OS_FINGERPRINT_FRAME_MS;
    host_event_pending = false;
}
#endif

static bool update_host_overlay(void) {
    const halcyon_host_telemetry_t current = read_host_telemetry(pending_host_event);

    if (!has_observed_host_telemetry) {
        observed_host_telemetry = current;
        has_observed_host_telemetry = true;
    } else if (!host_telemetry_equal(&observed_host_telemetry, &current)) {
        observed_host_telemetry = current;
        pending_host_event = HALCYON_HOST_EVENT_CHANGE;
        host_event_pending = true;
    }

    if (host_event_pending) {
        host_event_pending = false;
        if (display_mode != DISPLAY_MODE_TRACE_VIEW) {
            start_host_overlay(pending_host_event, &current);
        }
    }

    if (display_mode != DISPLAY_MODE_HOST_OVERLAY) {
        return false;
    }

    const uint32_t elapsed = timer_elapsed32(host_overlay_started);
    if (elapsed >= HOST_OVERLAY_DURATION_MS) {
        display_mode = DISPLAY_MODE_NORMAL;
        reset_normal_display_cache();
        return false;
    }

    if (timer_elapsed32(last_host_overlay_frame) < HOST_PANEL_FRAME_MS) {
        return false;
    }

    last_host_overlay_frame = timer_read32();
    ensure_display_font_loaded();
    draw_host_overlay(elapsed);
    return true;
}

#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
static const char *fingerprint_os_label(os_variant_t os) {
    switch (os) {
        case OS_LINUX:
            return "LIN";
        case OS_WINDOWS:
            return "WIN";
        case OS_MACOS:
            return "MAC";
        case OS_IOS:
            return "IOS";
        case OS_UNSURE:
        default:
            return "UN";
    }
}

static void draw_os_fingerprint_page(uint32_t elapsed) {
    const hsv_triplet_t accent = host_accent(&host_overlay_telemetry);
    const hsv_triplet_t text = { 29, 50, 211 };
    const uint8_t trace_count = xtreemze_os_fingerprint_trace_count();
    char line[24];

    qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);

    if (trace_count == 0) {
        draw_centered_host_text(70, "TRACE", accent);
        draw_centered_host_text(125, is_keyboard_master() ? "NO TRACE" : "SLAVE", text);
        return;
    }

    const uint8_t page = (uint8_t)((elapsed / OS_FINGERPRINT_PAGE_MS) % trace_count);
    xtreemze_os_fingerprint_entry_t entry;
    if (!xtreemze_os_fingerprint_trace_read(page, &entry)) {
        return;
    }

    draw_centered_host_text(5, "TRACE", accent);
    snprintf(line, sizeof(line), "%s%u/%u", xtreemze_os_fingerprint_trace_overflowed() ? "!" : "", (unsigned int)page + 1U, (unsigned int)trace_count);
    draw_centered_host_text(36, line, accent);

    snprintf(line, sizeof(line), "W %X", (unsigned int)entry.w_length);
    draw_centered_host_text(70, line, text);
    snprintf(line, sizeof(line), "N%u F%u", (unsigned int)entry.count, (unsigned int)entry.cnt_ff);
    draw_centered_host_text(104, line, text);
    snprintf(line, sizeof(line), "T%u Q%u", (unsigned int)entry.cnt_02, (unsigned int)entry.cnt_04);
    draw_centered_host_text(138, line, text);
    snprintf(line, sizeof(line), "C %s", fingerprint_os_label(entry.candidate_os));
    draw_centered_host_text(172, line, accent);
    snprintf(line, sizeof(line), "R %s", fingerprint_os_label(entry.detected_os));
    draw_centered_host_text(206, line, accent);
}

static bool update_os_fingerprint_page(void) {
    if (display_mode != DISPLAY_MODE_TRACE_VIEW) {
        return false;
    }

    if (timer_elapsed32(last_os_fingerprint_frame) < OS_FINGERPRINT_FRAME_MS) {
        return false;
    }

    last_os_fingerprint_frame = timer_read32();
    ensure_display_font_loaded();
    draw_os_fingerprint_page(timer_elapsed32(os_fingerprint_page_started));
    return true;
}
#endif

static void draw_host_marker(const char *layer_name, bool has_arp_text) {
    if (!has_observed_host_telemetry || has_arp_text) {
        return;
    }

    const char *const marker = host_marker_label(observed_host_telemetry.os);
    const int16_t marker_width = qp_textwidth(Retron27, marker);
    const int16_t layer_width = qp_textwidth(Retron27, layer_name);
    const int16_t marker_x = (int16_t)LCD_WIDTH - (int16_t)STATUS_X - marker_width;

    if (marker_x <= (int16_t)STATUS_X + layer_width + 3) {
        return;
    }

    const hsv_triplet_t accent = host_accent(&observed_host_telemetry);
    qp_drawtext_recolor(lcd_surface, (uint16_t)marker_x, STATUS_LAYER_Y, Retron27, marker, accent.h, accent.s, accent.v, HSV_EF_BG);
}

bool update_display(void) {
    bool display_dirty = false;

    ensure_display_font_loaded();

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

        const char *const layer_name = halcyon_display_layer_name_user(active_layer);

        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            STATUS_LAYER_Y,
            Retron27,
            layer_name,
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

        draw_host_marker(layer_name, has_arp_text);

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

static void tft_assert_reset(void) {
    gpio_set_pin_output(LCD_RST_PIN);
    gpio_write_pin_low(LCD_RST_PIN);
}

static void tft_begin_recovery(void) {
    // Normally already asserted by the suspend path; re-assert so a retry cycle
    // is self-contained. qp_clear() re-runs the init sequence, which ends in
    // DISPLAY_ON while GRAM still holds whatever survived the reset, so the
    // backlight must stay off until the blit has landed.
    halcyon_backlight_inhibit(true);

    tft_assert_reset();

    tft_power_timer = timer_read32();
    tft_power_state = TFT_POWER_WAKE_RESET;
}

// Drives the suspend/resume state machine. Returns true only when the
// controller is fully initialised and it is safe to push pixels at it.
static bool tft_power_task(void) {
    // Test-and-clear atomically. In this checkout the wake handler actually runs
    // from protocol task context (usb_event_cb only enqueues; usb_event_queue_task
    // dequeues and calls suspend_wakeup_init), so this is belt-and-braces rather
    // than strictly required -- RESTORESTATE is used because it is valid from any
    // context, unlike FORCEON, should that ever change.
    bool wake_requested = false;
    ATOMIC_BLOCK_RESTORESTATE {
        wake_requested         = display_wakeup_pending;
        display_wakeup_pending = false;
    }

    if (wake_requested) {
#ifdef TFT_NO_WAKE_RECOVERY
        // Diagnostic build: the panel is never brought back. It stays in reset
        // from the first suspend onward so the TFT/QP/SPI path is removed as a
        // variable entirely. State stays SUSPENDED, which makes every caller
        // below a no-op.
        host_resume_pending = false;
#else
        tft_recovery_attempts = 0;
        tft_begin_recovery();
#endif
    }

    switch (tft_power_state) {
        case TFT_POWER_ACTIVE:
            return true;

        case TFT_POWER_SUSPENDED:
        case TFT_POWER_FAILED:
            return false;

        case TFT_POWER_WAKE_RESET:
            if (timer_elapsed32(tft_power_timer) >= TFT_RESET_LOW_MS) {
                gpio_write_pin_high(LCD_RST_PIN);
                tft_power_timer = timer_read32();
                tft_power_state = TFT_POWER_WAKE_INIT;
            }
            return false;

        case TFT_POWER_WAKE_RETRY:
            if (timer_elapsed32(tft_power_timer) >= TFT_RECOVERY_RETRY_MS) {
                tft_begin_recovery();
            }
            return false;

        case TFT_POWER_WAKE_INIT:
            if (timer_elapsed32(tft_power_timer) < TFT_RESET_RECOVERY_MS) {
                return false;
            }

            // qp_clear() re-runs the ST7789 init sequence (SWRESET, sleep off,
            // pixel format, inversion, MADCTL, DISPLAY_ON) over SPI. Unlike
            // qp_init() it does not pulse RST again, so this is the single
            // authoritative re-initialisation after the reset cycle above.
            // qp_clear() only reports comms-layer failures (see the note in
            // TFT_RECOVERY_MAX_ATTEMPTS' commentary), so the failure policy is
            // otherwise untestable. Build with -DTFT_FORCE_RECOVERY_FAILURE to
            // exercise the retry/backoff/FAILED path on real hardware.
#ifdef TFT_FORCE_RECOVERY_FAILURE
            if (true) {
#else
            if (!qp_clear(lcd)) {
#endif
                tft_recovery_attempts++;
                tft_power_timer = timer_read32();
                tft_power_state = (tft_recovery_attempts >= TFT_RECOVERY_MAX_ATTEMPTS)
                                      ? TFT_POWER_FAILED
                                      : TFT_POWER_WAKE_RETRY;
                return false;
            }

            // ST7789_NO_AUTOMATIC_VIEWPORT_OFFSETS means init leaves these
            // alone, but re-assert them so geometry can never drift.
            qp_set_viewport_offsets(lcd, LCD_OFFSET_X, LCD_OFFSET_Y);

            // The RGB565 surface lives in RAM and survived suspend untouched,
            // so this restores the exact pre-suspend image in one blit.
            qp_surface_draw(lcd_surface, lcd, 0, 0, false);
            qp_flush(lcd);

            // Only after the framebuffer has landed: drop cached header/mod/
            // animation state so the next update_display() cannot decide it has
            // nothing to redraw.
            display_mode = DISPLAY_MODE_NORMAL;
            reset_normal_display_cache();

            if (host_resume_pending) {
                host_resume_pending = false;
                pending_host_event = HALCYON_HOST_EVENT_RESUME;
                host_event_pending = true;
            }

            tft_recovery_attempts = 0;
            tft_power_state = TFT_POWER_ACTIVE;
            halcyon_backlight_inhibit(false);
            return true;
    }

    return false;
}

// Called from halcyon.c.
//
// NOT an edge callback: protocol_pre_task() spins in
// `while (USB_DRIVER.state == USB_SUSPENDED) { suspend_power_down(); }`, so
// this runs roughly every 17ms for the whole suspend. Everything below must
// therefore happen exactly once, on the transition into SUSPENDED, or we end
// up clocking SPI at a controller we are deliberately holding in reset.
void module_suspend_power_down_kb(void) {
    if (tft_power_state == TFT_POWER_SUSPENDED) {
        return;
    }

    // Assert the inhibit here, not in the wake path: it has to survive the
    // suspend. housekeeping_task_kb() runs its backlight timeout block before
    // the display task, so on the first tick after wake it would otherwise
    // light the panel while the controller is still in reset.
    halcyon_backlight_inhibit(true);
    backlight_suspend();

    // DISPLAY_OFF is only meaningful while the controller is initialised. If a
    // suspend lands mid-recovery, RST is already low and SPI is not valid.
#ifndef TFT_NO_WAKE_RECOVERY
    if (tft_power_state == TFT_POWER_ACTIVE) {
        qp_power(lcd, false);
    }
#endif

    tft_assert_reset();

    // Discard a wake that raced the suspend, so recovery cannot start while
    // the host is still down and strand us with RST released.
    display_wakeup_pending = false;
    host_resume_pending    = false;

    tft_recovery_attempts = 0;
    tft_power_state       = TFT_POWER_SUSPENDED;
}

// Called from halcyon.c
void module_suspend_wakeup_init_kb(void) {
    // QMK invokes this callback from the USB wake ISR on ChibiOS. Defer the
    // SPI transaction until housekeeping runs in normal thread context.
    display_wakeup_pending = true;
    host_resume_pending = true;
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

    // Land in exactly the logical state a successful wake recovery reaches.
    display_wakeup_pending = false;
    host_resume_pending    = false;
    tft_recovery_attempts  = 0;
    tft_power_state        = TFT_POWER_ACTIVE;
    halcyon_backlight_inhibit(false);

    if(!module_post_init_user()) { return false; }

    return true;
}

// Called from halcyon.c
bool display_module_housekeeping_task_kb(bool second_display) {
    // Nothing below this point may touch the panel unless the controller is
    // fully initialised: no overlays, no animation frames, no trace pages.
    if (!tft_power_task()) {
        return true;
    }

    if(!display_module_housekeeping_task_user(second_display)) { return false; }

    bool display_dirty = update_host_overlay();

    if (display_mode == DISPLAY_MODE_HOST_OVERLAY) {
        if (display_dirty) {
            qp_surface_draw(lcd_surface, lcd, 0, 0, false);
        }
        return true;
    }

    if (last_input_activity_elapsed() >= HLC_BACKLIGHT_TIMEOUT) {
#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
        if (display_mode == DISPLAY_MODE_TRACE_VIEW) {
            display_mode = DISPLAY_MODE_NORMAL;
            reset_normal_display_cache();
        }
#endif
        return true;
    }

#ifdef XTREEMZE_OS_FINGERPRINT_TRACE
    display_dirty = update_os_fingerprint_page();

    if (display_mode == DISPLAY_MODE_TRACE_VIEW) {
        if (display_dirty) {
            qp_surface_draw(lcd_surface, lcd, 0, 0, false);
        }
        return true;
    }
#endif

    display_dirty = update_display();

    if (display_dirty) {
        qp_surface_draw(lcd_surface, lcd, 0, 0, false);
    }

    return true;
}
