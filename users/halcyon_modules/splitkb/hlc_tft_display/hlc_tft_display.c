// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "halcyon.h"
#include "hlc_tft_display.h"

#include "hardware/structs/rosc.h"
#include <stdio.h>
#include <string.h>

// Fonts mono2
#include "graphics/fonts/Retron2000-27.qff.h"
#include "graphics/fonts/Retron2000-underline-27.qff.h"

static const char *ctrl =  "Ctl";
static const char *gui =   "Gui";
static const char *shift = "Shf";
static const char *alt =   "Alt";
static painter_font_handle_t Retron27;
static painter_font_handle_t Retron27_underline;

static uint8_t lcd_surface_fb[SURFACE_REQUIRED_BUFFER_BYTE_SIZE(135, 240, 16)];

int color_value = 0;

painter_device_t lcd;
painter_device_t lcd_surface;

static uint8_t last_mod_state = 0xFF;
static uint8_t last_display_layer = 0xFF;
static char last_arp_text[24] = "";

#define STATUS_X       5
#define STATUS_LAYER_Y 5

__attribute__((weak)) const char *halcyon_display_layer_name_user(uint8_t layer) {
    static const char *const fallback_layer_names[] = {
        "L0", "L1", "L2", "L3", "L4", "L5", "L6",
        "L7", "L8", "L9", "L10", "L11", "L12"
    };

    if (layer < ARRAY_SIZE(fallback_layer_names)) {
        return fallback_layer_names[layer];
    }

    return "L?";
}

__attribute__((weak)) const char *halcyon_display_alt_repeat_text_user(void) {
    return "Arp ---";
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

void update_display(void) {
    static bool fonts_loaded = false;

    if (!fonts_loaded) {
        Retron27 = qp_load_font_mem(font_Retron2000_27);
        Retron27_underline = qp_load_font_mem(font_Retron2000_underline_27);
        fonts_loaded = true;
    }

    const uint8_t active_layer = get_highest_layer(layer_state | default_layer_state);
    const uint8_t active_mods = get_mods() | get_oneshot_mods();
    const char *const arp_text = halcyon_display_alt_repeat_text_user();
    const bool first_run = (last_display_layer == 0xFF);
    const bool arp_changed = strcmp(last_arp_text, arp_text) != 0;

    if (first_run || active_layer != last_display_layer) {
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
            Retron27_underline,
            halcyon_display_layer_name_user(active_layer),
            layer_h, layer_s, layer_v,
            HSV_EF_BG
        );

        last_display_layer = active_layer;
    }

    if (first_run || active_mods != last_mod_state || arp_changed) {
        const uint16_t mod_top = LCD_HEIGHT - (Retron27->line_height * 5) - 14;
        qp_rect(lcd_surface, 0, mod_top - 4, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_EF_BG, true);

        const bool ctrl_active = (active_mods & MOD_MASK_CTRL) != 0;
        const bool gui_active = (active_mods & MOD_MASK_GUI) != 0;
        const bool shift_active = (active_mods & MOD_MASK_SHIFT) != 0;
        const bool alt_active = (active_mods & MOD_MASK_ALT) != 0;

        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            mod_top,
            ctrl_active ? Retron27_underline : Retron27,
            ctrl,
            ctrl_active ? 122 : 98, ctrl_active ? 82 : 23, ctrl_active ? 187 : 146,
            HSV_EF_BG
        );
        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            mod_top + Retron27->line_height,
            gui_active ? Retron27_underline : Retron27,
            gui,
            gui_active ? 254 : 98, gui_active ? 115 : 23, gui_active ? 230 : 146,
            HSV_EF_BG
        );
        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            mod_top + Retron27->line_height * 2,
            shift_active ? Retron27_underline : Retron27,
            shift,
            shift_active ? 28 : 98, shift_active ? 107 : 23, shift_active ? 219 : 146,
            HSV_EF_BG
        );
        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            mod_top + Retron27->line_height * 3,
            alt_active ? Retron27_underline : Retron27,
            alt,
            alt_active ? 59 : 98, alt_active ? 85 : 23, alt_active ? 192 : 146,
            HSV_EF_BG
        );
        qp_drawtext_recolor(
            lcd_surface,
            STATUS_X,
            mod_top + Retron27->line_height * 4,
            Retron27,
            arp_text,
            122, 82, 187,
            HSV_EF_BG
        );

        last_mod_state = active_mods;
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

    if(second_display) {
        static uint32_t last_draw = 0;
        static bool second_display_set = false;
        static uint32_t previous_matrix_activity_time = 0;

        if(!second_display_set) {
            srand(get_random_32bit());
            init_grid();
            color_value = rand() % 8;
            second_display_set = true;
        }

        if (timer_elapsed32(last_draw) >= 100) { // Throttle to 10 fps
            draw_grid();
            update_grid();

            if (previous_matrix_activity_time != last_matrix_activity_time()) {
                color_value = rand() % 8;
                add_cell_cluster();
                previous_matrix_activity_time = last_matrix_activity_time();
            }

            last_draw = timer_read32();
        }
    }

    // Update display information (layers, numlock, etc.)
    if(!second_display) {
        update_display();
    }

    // Move surface to lcd
    qp_surface_draw(lcd_surface, lcd, 0, 0, 0);
    qp_flush(lcd);

    return true;
}
