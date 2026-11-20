// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* Included by the RGB protocol owner: no parallel RGB store or resolver. */
#define XTREEMZE_PALETTE_KEYCODE (QK_KB_0 + 21)
static bool palette_active, palette_wait_release, palette_exiting;
static bool palette_entry_held, palette_old_dirty, palette_old_combos;
static bool palette_transport;
static uint8_t palette_scope = 1, palette_index, palette_parameter;
static uint32_t palette_last_action, palette_hold_started;
static layer_state_t palette_old_layers;
static xtreemze_rgb_profile_store_t palette_backup;

static uint8_t palette_count(void) {
    const uint8_t counts[] = {1, 13, 4, 32};
    return counts[palette_scope];
}

static void palette_preview(void) {
    xtreemze_rgb_profile_t *profile = rgb_profile_for_scope(palette_scope, palette_index);
    preview_profile = *profile;
    if (preview_profile.mode == XTREEMZE_RGB_PROFILE_UNASSIGNED || preview_profile.mode == 0) preview_profile.mode = 1;
    preview_profile.v = preview_profile.v > RGB_MATRIX_MAXIMUM_BRIGHTNESS ? RGB_MATRIX_MAXIMUM_BRIGHTNESS : preview_profile.v;
    xtreemze_palette_settings_preview(palette_scope, palette_index, palette_parameter, preview_profile.h, preview_profile.s);
    preview_started = timer_read32();
    preview_active = true;
    rgb_profile_invalidate_runtime();
}

static void palette_finish(bool save) {
    /* Never persist after authority or transport changes. RGB split replication
     * is not advertised by 0xF0 v1: saving here is explicitly master-local. */
    save = save && is_keyboard_master() && palette_transport == is_transport_connected() &&
           usb_device_state_get_configure_state() == USB_DEVICE_STATE_CONFIGURED;
    if (save) {
        if (rgb_store_dirty) rgb_store_save();
    } else {
        rgb_store = palette_backup;
        rgb_store_dirty = palette_old_dirty;
    }
    xtreemze_palette_settings_end(save);
    preview_active = false;
    palette_exiting = true;
    palette_wait_release = true;
    rgb_profile_invalidate_runtime();
    clear_keyboard();
}

static void xtreemze_palette_enter(void) {
    rgb_store_ensure_loaded();
    palette_backup = rgb_store;
    palette_old_dirty = rgb_store_dirty;
    palette_old_layers = layer_state;
    palette_old_combos = is_combo_enabled();
    palette_transport = is_transport_connected();
    palette_scope = 1;
    palette_index = get_highest_layer(layer_state | default_layer_state);
    if (palette_index >= 13) palette_index = 0;
    palette_parameter = 0;
    palette_last_action = timer_read32();
    palette_active = true;
    palette_exiting = false;
    palette_wait_release = true;
    combo_override_active = false;
    legacy_passthrough_active = false;
    clear_keyboard();
    combo_disable();
    layer_clear();
    xtreemze_palette_settings_begin();
    palette_preview();
}

static void palette_adjust(bool up) {
    xtreemze_rgb_profile_t *profile = rgb_profile_for_scope(palette_scope, palette_index);
    if (profile->mode == XTREEMZE_RGB_PROFILE_UNASSIGNED) *profile = (xtreemze_rgb_profile_t){1, 0, 210, RGB_MATRIX_MAXIMUM_BRIGHTNESS, 128};
    uint8_t *value = palette_parameter == 0 ? &profile->h : palette_parameter == 1 ? &profile->s : &profile->v;
    const uint8_t limit = palette_parameter == 2 ? RGB_MATRIX_MAXIMUM_BRIGHTNESS : 255;
    if (palette_parameter == 0) *value += up ? 4 : -4;
    else if (up) *value = *value > limit - 4 ? limit : *value + 4;
    else *value = *value < 4 ? 0 : *value - 4;
    rgb_store_dirty = true;
    xtreemze_palette_settings_color(palette_scope, palette_index, profile->h, profile->s, profile->v);
    palette_preview();
}

bool xtreemze_palette_process(uint16_t keycode, keyrecord_t *record) {
    if (!palette_active) {
        if (keycode != XTREEMZE_PALETTE_KEYCODE) return true;
        palette_entry_held = record->event.pressed;
        if (palette_entry_held) palette_hold_started = timer_read32();
        return false;
    }
    if (palette_wait_release || !record->event.pressed) return false;
    palette_last_action = timer_read32();
    if (IS_ENCODEREVENT(record->event)) {
        palette_adjust(record->event.type == ENCODER_CW_EVENT);
        return false;
    }
    const uint8_t row = record->event.key.row;
    const uint8_t col = record->event.key.col;
    if (row == 0 && col == 0) palette_finish(false);
    else if (row == 0 && col == 1) palette_finish(true);
    else if (row == 0 && col == 2) { palette_scope = (palette_scope + 1) % 4; palette_index = 0; }
    else if (row == 0 && col == 3) palette_index = (palette_index + 1) % palette_count();
    else if (row == 0 && col == 4) palette_parameter = (palette_parameter + 1) % 3;
    else if (row == 1 && col == 0) palette_adjust(false);
    else if (row == 1 && col == 1) palette_adjust(true);
    else if (row == 1 && col == 2) palette_index = (palette_index + palette_count() - 1) % palette_count();
    if (!palette_exiting) palette_preview();
    return false;
}

void xtreemze_palette_housekeeping(void) {
    if (!palette_active) {
        if (palette_entry_held && timer_elapsed32(palette_hold_started) >= 700 && is_keyboard_master() &&
            usb_device_state_get_configure_state() == USB_DEVICE_STATE_CONFIGURED) {
            palette_entry_held = false;
            xtreemze_palette_enter();
        }
        return;
    }
    if (!palette_exiting && (timer_elapsed32(palette_last_action) >= 60000 || !is_keyboard_master() ||
        palette_transport != is_transport_connected() || usb_device_state_get_configure_state() != USB_DEVICE_STATE_CONFIGURED)) palette_finish(false);
    if (palette_wait_release) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) if (matrix_get_row(row)) return;
        palette_wait_release = false;
        if (palette_exiting) {
            palette_active = false;
            layer_state_set(palette_old_layers);
            if (palette_old_combos) combo_enable();
            clear_keyboard();
        }
    }
    if (!palette_exiting) palette_preview();
}

bool xtreemze_palette_is_active(void) { return palette_active; }
