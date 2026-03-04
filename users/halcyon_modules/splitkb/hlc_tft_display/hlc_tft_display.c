// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "halcyon.h"
#include "hlc_tft_display.h"

#include "hardware/structs/rosc.h"
#include <stdio.h>
#include <string.h>

// Fonts mono2
#include "graphics/fonts/Retron2000-27.qff.h"

static const char *ctrl =  "Ctl";
static const char *gui =   "Gui";
static const char *shift = "Shf";
static const char *alt =   "Alt";
static painter_font_handle_t Retron27;

static uint8_t lcd_surface_fb[SURFACE_REQUIRED_BUFFER_BYTE_SIZE(135, 240, 16)];

int color_value = 0;

painter_device_t lcd;
painter_device_t lcd_surface;

static uint8_t last_mod_state = 0xFF;
static uint8_t last_visible_mod_mask = 0xFF;
static uint8_t last_display_layer = 0xFF;
static char last_arp_text[24] = "";
static uint32_t last_mod_seen[4] = { 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL };
static bool life_animation_initialized = false;
static uint32_t life_last_draw = 0;
static uint32_t life_prev_activity_time = 0;
static bool status_overlay_active = true;
static uint32_t status_overlay_start = 0;
static uint8_t status_overlay_layer = 0xFF;

#define STATUS_X       5
#define STATUS_LAYER_Y 5
#define STATUS_ARP_X   150
#define LIFE_FRAME_MS 100
#define STATUS_OVERLAY_MS 3000
#define MOD_RECENT_MS 2200
#define MOD_TS_UNSET 0xFFFFFFFFUL

static bool mod_is_recent(uint32_t ts) {
    return ts != MOD_TS_UNSET && timer_elapsed32(ts) < MOD_RECENT_MS;
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

#define GRID_WIDTH 27
#define GRID_HEIGHT 48
#define CELL_SIZE 4  // Cell size excluding outline
#define OUTLINE_SIZE 1

// Define the probability factor for initial alive cells
#define INITIAL_ALIVE_PROBABILITY 0.2  // 20% chance of being alive

bool grid[GRID_HEIGHT][GRID_WIDTH];  // Current state
bool new_grid[GRID_HEIGHT][GRID_WIDTH];  // Next state
bool changed_grid[GRID_HEIGHT][GRID_WIDTH]; // Tracks changed cells

uint32_t get_random_32bit(void) {
    uint32_t random_value = 0;
    for (int i = 0; i < 32; i++) {
        wait_ms(1);
        random_value = (random_value << 1) | (rosc_hw->randombit & 1);
    }
    return random_value;
}

void init_grid() {
    // Initialize grid with alive cells
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = (rand() < INITIAL_ALIVE_PROBABILITY * RAND_MAX);  // Use probability factor
            changed_grid[y][x] = true;      // Mark all as changed initially
        }
    }
}

void draw_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (changed_grid[y][x]) { // Only update changed cells
                uint16_t left = x * (CELL_SIZE + OUTLINE_SIZE);
                uint16_t top = y * (CELL_SIZE + OUTLINE_SIZE);
                uint16_t right = left + CELL_SIZE + OUTLINE_SIZE;
                uint16_t bottom = top + CELL_SIZE + OUTLINE_SIZE;

                // Draw the outline
                qp_rect(lcd_surface, left, top, right, bottom, HSV_EF_BG, true);

                // Draw the filled cell inside the outline if it's alive
                if (grid[y][x]) {
                    switch (color_value) {
                    case 0:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_0, true);
                        break;
                    case 1:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_1, true);
                        break;
                    case 2:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_2, true);
                        break;
                    case 3:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_3, true);
                        break;
                    case 4:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_4, true);
                        break;
                    case 5:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_5, true);
                        break;
                    case 6:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_6, true);
                        break;
                    case 7:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_7, true);
                        break;
                    default:
                        qp_rect(lcd_surface, left + OUTLINE_SIZE, top + OUTLINE_SIZE, right - OUTLINE_SIZE, bottom - OUTLINE_SIZE, HSV_LAYER_UNDEF, true);
                    }
                }
            }
        }
    }
}

void update_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int alive_neighbors = 0;

            // Count alive neighbors
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dy == 0 && dx == 0) continue;  // Skip the current cell
                    int ny = y + dy;
                    int nx = x + dx;
                    if (ny >= 0 && ny < GRID_HEIGHT && nx >= 0 && nx < GRID_WIDTH) {
                        alive_neighbors += grid[ny][nx];
                    }
                }
            }

            // Apply the rules of the Game of Life
            if (grid[y][x]) {
                // Any live cell with two or three live neighbours survives.
                new_grid[y][x] = (alive_neighbors == 2 || alive_neighbors == 3);
            } else {
                // Any dead cell with exactly three live neighbours becomes a live cell.
                new_grid[y][x] = (alive_neighbors == 3);
            }

            // Track changed cells
            changed_grid[y][x] = (grid[y][x] != new_grid[y][x]);
        }
    }

    // Copy new grid state to current grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = new_grid[y][x];
        }
    }
}

// Function to add a cluster of cells at a random position
void add_cell_cluster() {
    int cluster_size = 3;  // Size of the cluster (3x3)
    int x = rand() % (GRID_WIDTH - cluster_size);
    int y = rand() % (GRID_HEIGHT - cluster_size);

    for (int dy = 0; dy < cluster_size; dy++) {
        for (int dx = 0; dx < cluster_size; dx++) {
            bool is_alive = rand() % 2; // Randomly choose between 0 and 1
            grid[y + dy][x + dx] = is_alive;  // Set the cell to be alive
            changed_grid[y + dy][x + dx] = true; // Mark the cell as changed
        }
    }
}

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} hsv_triplet_t;

static const hsv_triplet_t layer_fg_hsv[13] = {
    { 59, 85, 192 },  { 122, 82, 187 }, { 28, 107, 219 }, { 254, 115, 230 },
    { 170, 84, 182 }, { 12, 96, 206 },  { 95, 88, 194 },  { 210, 80, 186 },
    { 59, 85, 192 },  { 122, 82, 187 }, { 28, 107, 219 }, { 95, 88, 194 },
    { 170, 84, 182 },
};

static const hsv_triplet_t layer_bg_hsv[13] = {
    { 122, 36, 82 }, { 28, 34, 82 },  { 254, 34, 80 }, { 59, 32, 78 },
    { 95, 32, 78 },  { 170, 30, 78 }, { 12, 34, 80 },  { 210, 30, 78 },
    { 122, 36, 82 }, { 28, 34, 82 },  { 254, 34, 80 }, { 210, 30, 78 },
    { 95, 32, 78 },
};

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

static void draw_layer_background_pattern(uint8_t layer) {
    const uint8_t tile_w = 24;
    const uint8_t tile_h = 24;
    const uint8_t variant = layer % 13;
    const hsv_triplet_t fg = layer_fg_hsv[variant];
    const hsv_triplet_t bg = layer_bg_hsv[variant];

    qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);

    for (uint16_t y = 0; y + tile_h <= LCD_HEIGHT; y += tile_h) {
        for (uint16_t x = 0; x + tile_w <= LCD_WIDTH; x += tile_w) {
            const bool phase = (((x / tile_w) + (y / tile_h) + layer) & 1U) != 0U;
            const uint16_t cx = x + tile_w / 2;
            const uint16_t cy = y + tile_h / 2;
            const uint8_t ah = phase ? fg.h : bg.h;
            const uint8_t as = phase ? fg.s : bg.s;
            const uint8_t av = phase ? fg.v : bg.v;
            const uint8_t bh = phase ? bg.h : fg.h;
            const uint8_t bs = phase ? bg.s : fg.s;
            const uint8_t bv = phase ? bg.v : fg.v;

            // All variants are tiled diamond motifs and remain four-way symmetric.
            switch (variant) {
                case 0:
                    draw_diamond(cx, cy, 6, ah, as, av);
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
                    draw_diamond(cx, cy, 7, ah, as, av);
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
                    draw_diamond(cx, cy, 6, ah, as, av);
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

void update_display(void) {
    static bool fonts_loaded = false;

    if (!fonts_loaded) {
        Retron27 = qp_load_font_mem(font_Retron2000_27);
        fonts_loaded = true;
    }

    const uint8_t active_layer = get_highest_layer(layer_state | default_layer_state);
    const uint8_t active_mods = get_mods() | get_oneshot_mods();
    const uint32_t now = timer_read32();
    const char *const arp_text = halcyon_display_alt_repeat_text_user();
    const bool first_run = (last_display_layer == 0xFF);
    const bool arp_changed = strcmp(last_arp_text, arp_text) != 0;

    if ((active_mods & MOD_MASK_CTRL) != 0) {
        last_mod_seen[0] = now;
    }
    if ((active_mods & MOD_MASK_GUI) != 0) {
        last_mod_seen[1] = now;
    }
    if ((active_mods & MOD_MASK_SHIFT) != 0) {
        last_mod_seen[2] = now;
    }
    if ((active_mods & MOD_MASK_ALT) != 0) {
        last_mod_seen[3] = now;
    }

    const bool ctrl_visible = (active_mods & MOD_MASK_CTRL) != 0 || mod_is_recent(last_mod_seen[0]);
    const bool gui_visible = (active_mods & MOD_MASK_GUI) != 0 || mod_is_recent(last_mod_seen[1]);
    const bool shift_visible = (active_mods & MOD_MASK_SHIFT) != 0 || mod_is_recent(last_mod_seen[2]);
    const bool alt_visible = (active_mods & MOD_MASK_ALT) != 0 || mod_is_recent(last_mod_seen[3]);
    const uint8_t visible_mod_mask = (ctrl_visible ? 1U : 0U) | (gui_visible ? 2U : 0U) | (shift_visible ? 4U : 0U) | (alt_visible ? 8U : 0U);

    const bool need_full_redraw = first_run || active_layer != last_display_layer;
    const bool force_full_redraw = need_full_redraw || (last_visible_mod_mask != 0U && visible_mod_mask == 0U);

    if (force_full_redraw) {
        draw_layer_background_pattern(active_layer);
    }

    if (force_full_redraw || arp_changed || active_layer != last_display_layer) {
        qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, Retron27->line_height + 12, HSV_EF_BG, true);

        uint8_t layer_h = 0;
        uint8_t layer_s = 0;
        uint8_t layer_v = 0;
        switch (active_layer) {
            case 0: layer_h = 59;  layer_s = 85;  layer_v = 192; break;
            case 1: layer_h = 122; layer_s = 82;  layer_v = 187; break;
            case 2: layer_h = 28;  layer_s = 107; layer_v = 219; break;
            case 3: layer_h = 254; layer_s = 115; layer_v = 230; break;
            case 4: layer_h = 59;  layer_s = 85;  layer_v = 192; break;
            case 5: layer_h = 122; layer_s = 82;  layer_v = 187; break;
            case 6: layer_h = 28;  layer_s = 107; layer_v = 219; break;
            case 7: layer_h = 254; layer_s = 115; layer_v = 230; break;
            case 8: layer_h = 59;  layer_s = 85;  layer_v = 192; break;
            case 9: layer_h = 122; layer_s = 82;  layer_v = 187; break;
            case 10: layer_h = 28; layer_s = 107; layer_v = 219; break;
            case 11: layer_h = 254; layer_s = 115; layer_v = 230; break;
            case 12: layer_h = 59; layer_s = 85;  layer_v = 192; break;
            default: layer_h = 29; layer_s = 50; layer_v = 211; break;
        }

        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            STATUS_LAYER_Y,
            Retron27,
            halcyon_display_layer_name_user(active_layer),
            layer_h, layer_s, layer_v,
            HSV_EF_BG
        );
        qp_drawtext_recolor(lcd_surface, STATUS_ARP_X, STATUS_LAYER_Y, Retron27, arp_text, 122, 82, 187, HSV_EF_BG);

        last_display_layer = active_layer;
    }

    if (visible_mod_mask != 0U && (first_run || force_full_redraw || active_mods != last_mod_state || visible_mod_mask != last_visible_mod_mask)) {
        const uint16_t mod_top = LCD_HEIGHT - (Retron27->line_height * 4) - 8;
        qp_rect(lcd_surface, 0, mod_top - 4, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);

        uint8_t line = 0;
        if (ctrl_visible) {
            const bool ctrl_active = (active_mods & MOD_MASK_CTRL) != 0;
            qp_drawtext_recolor(lcd_surface, STATUS_X, mod_top + (Retron27->line_height * line++), Retron27, ctrl, ctrl_active ? 122 : 98, ctrl_active ? 82 : 23, ctrl_active ? 187 : 146, HSV_EF_BG);
        }
        if (gui_visible) {
            const bool gui_active = (active_mods & MOD_MASK_GUI) != 0;
            qp_drawtext_recolor(lcd_surface, STATUS_X, mod_top + (Retron27->line_height * line++), Retron27, gui, gui_active ? 254 : 98, gui_active ? 115 : 23, gui_active ? 230 : 146, HSV_EF_BG);
        }
        if (shift_visible) {
            const bool shift_active = (active_mods & MOD_MASK_SHIFT) != 0;
            qp_drawtext_recolor(lcd_surface, STATUS_X, mod_top + (Retron27->line_height * line++), Retron27, shift, shift_active ? 28 : 98, shift_active ? 107 : 23, shift_active ? 219 : 146, HSV_EF_BG);
        }
        if (alt_visible) {
            const bool alt_active = (active_mods & MOD_MASK_ALT) != 0;
            qp_drawtext_recolor(lcd_surface, STATUS_X, mod_top + (Retron27->line_height * line++), Retron27, alt, alt_active ? 59 : 98, alt_active ? 85 : 23, alt_active ? 192 : 146, HSV_EF_BG);
        }
        last_mod_state = active_mods;
    }

    last_visible_mod_mask = visible_mod_mask;

    if (arp_changed) {
        snprintf(last_arp_text, sizeof(last_arp_text), "%s", arp_text);
    }
}

// Called from halcyon.c
void module_suspend_power_down_kb(void) {
    qp_power(lcd, false);
}

// Called from halcyon.c
void module_suspend_wakeup_init_kb(void) {
    qp_power(lcd, true);
}

// Called from halcyon.c
bool module_post_init_kb(void) {
    // Turn on backlight
    backlight_enable();

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
    qp_surface_draw(lcd_surface, lcd, 0, 0, 0);
    qp_flush(lcd);

    if(!module_post_init_user()) { return false; }

    return true;
}

// Called from halcyon.c
bool display_module_housekeeping_task_kb(bool second_display) {
    if(!display_module_housekeeping_task_user(second_display)) { return false; }

    const uint8_t active_layer = get_highest_layer(layer_state | default_layer_state);

    if (!life_animation_initialized) {
        srand(get_random_32bit());
        init_grid();
        color_value = rand() % 8;
        life_prev_activity_time = last_matrix_activity_time();
        life_last_draw = timer_read32();
        status_overlay_layer = active_layer;
        status_overlay_start = timer_read32();
        status_overlay_active = true;
        life_animation_initialized = true;
    }

    if (active_layer != status_overlay_layer) {
        status_overlay_layer = active_layer;
        status_overlay_start = timer_read32();
        status_overlay_active = true;
    }

    if (status_overlay_active) {
        update_display();

        if (timer_elapsed32(status_overlay_start) >= STATUS_OVERLAY_MS) {
            status_overlay_active = false;
        }
    } else {
        if (timer_elapsed32(life_last_draw) >= LIFE_FRAME_MS) { // Throttle to 10 fps
            draw_grid();
            update_grid();

            if (life_prev_activity_time != last_matrix_activity_time()) {
                color_value = rand() % 8;
                add_cell_cluster();
                life_prev_activity_time = last_matrix_activity_time();
            }

            life_last_draw = timer_read32();
        }
    }

    // Move surface to lcd
    qp_surface_draw(lcd_surface, lcd, 0, 0, 0);
    qp_flush(lcd);

    return true;
}
