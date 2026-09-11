// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#if defined(RGB_MATRIX_ENABLE) && defined(VIA_ENABLE)

#include "halcyon_settings_protocol.h"
#include "eeconfig.h"
#include "eeprom.h"
#include "rgb_matrix.h"
#include "timer.h"
#include "via.h"

#include <stdint.h>
#include <string.h>

/*
 * Host protocol
 * -------------
 * This deliberately uses a dedicated raw VIA command rather than custom-menu
 * channel 0x07/0x08. VialRGB owns those command IDs on this firmware and
 * interprets data[1] as a VialRGB operation, so sharing that namespace would
 * be ambiguous.
 *
 * The app sends command 0xF0 followed by one of these operations. Request
 * bytes are left intact so KeyboardAPI's command echo validation continues to
 * work; GET payloads are written immediately after the request prefix.
 */
#define XTREEMZE_RGB_PROFILE_COMMAND 0xF0
#define XTREEMZE_RGB_PROFILE_PROTOCOL_VERSION 1

#define XTREEMZE_RGB_LAYER_PROFILE_COUNT 13
#define XTREEMZE_RGB_MOD_PROFILE_COUNT 4
#define XTREEMZE_RGB_COMBO_PROFILE_COUNT 32

#define XTREEMZE_RGB_PROFILE_UNASSIGNED 0xFF
#define XTREEMZE_RGB_PROFILE_STORE_MAGIC 0x58524742UL /* "XRGB" */
#define XTREEMZE_RGB_PROFILE_STORE_VERSION 1
#define XTREEMZE_RGB_PROFILE_PREVIEW_MS 1500
#define XTREEMZE_RGB_COMBO_MS_DEFAULT 2000
#define XTREEMZE_RGB_COMBO_MS_MIN 250
#define XTREEMZE_RGB_COMBO_MS_MAX 10000

#define XTREEMZE_LEGACY_USER_DATA_MAGIC 0x58
#define XTREEMZE_LEGACY_USER_DATA_VERSION 0x02

#define XTREEMZE_RGB_SCOPE_GLOBAL 0
#define XTREEMZE_RGB_SCOPE_LAYER 1
#define XTREEMZE_RGB_SCOPE_MODIFIER 2
#define XTREEMZE_RGB_SCOPE_COMBO 3
#define XTREEMZE_RGB_SCOPE_FLAGS 0x0F

#define XTREEMZE_RGB_FIELD_MODE (1U << 0)
#define XTREEMZE_RGB_FIELD_HUE (1U << 1)
#define XTREEMZE_RGB_FIELD_SATURATION (1U << 2)
#define XTREEMZE_RGB_FIELD_BRIGHTNESS (1U << 3)
#define XTREEMZE_RGB_FIELD_SPEED (1U << 4)
#define XTREEMZE_RGB_FIELD_FLAGS (XTREEMZE_RGB_FIELD_MODE | XTREEMZE_RGB_FIELD_HUE | XTREEMZE_RGB_FIELD_SATURATION | XTREEMZE_RGB_FIELD_BRIGHTNESS | XTREEMZE_RGB_FIELD_SPEED)

enum xtreemze_rgb_profile_operation {
    XTREEMZE_RGB_GET_CAPABILITIES = 0x01,
    XTREEMZE_RGB_GET_PROFILE = 0x02,
    XTREEMZE_RGB_SET_PROFILE = 0x03,
    XTREEMZE_RGB_SAVE = 0x04,
    XTREEMZE_RGB_PREVIEW = 0x05,
    XTREEMZE_RGB_GET_COMBO_DURATION = 0x06,
    XTREEMZE_RGB_SET_COMBO_DURATION = 0x07,
    XTREEMZE_RGB_CANCEL_PREVIEW = 0x08,
};

typedef struct {
    uint8_t mode;
    uint8_t h;
    uint8_t s;
    uint8_t v;
    uint8_t speed;
} xtreemze_rgb_profile_t;

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t flags;
    uint16_t combo_duration_ms;
    xtreemze_rgb_profile_t global_profile;
    xtreemze_rgb_profile_t layer_profiles[XTREEMZE_RGB_LAYER_PROFILE_COUNT];
    xtreemze_rgb_profile_t mod_profiles[XTREEMZE_RGB_MOD_PROFILE_COUNT];
    xtreemze_rgb_profile_t combo_profiles[XTREEMZE_RGB_COMBO_PROFILE_COUNT];
    uint8_t reserved[14];
} xtreemze_rgb_profile_store_t;

typedef struct {
    uint8_t magic;
    uint8_t version;
    uint8_t defaults_marker;
    uint8_t last_host_family;
    uint16_t chord_override_ms;
    uint8_t reserved1[2];
    xtreemze_rgb_profile_t layer_profiles[XTREEMZE_RGB_LAYER_PROFILE_COUNT];
    xtreemze_rgb_profile_t mod_profiles[XTREEMZE_RGB_MOD_PROFILE_COUNT];
    xtreemze_rgb_profile_t chord_profile;
} xtreemze_legacy_user_data_t;

_Static_assert(DYNAMIC_KEYMAP_LAYER_COUNT == XTREEMZE_RGB_LAYER_PROFILE_COUNT, "RGB profile layer count must match Vial layer count");
_Static_assert(VIAL_COMBO_ENTRIES == XTREEMZE_RGB_COMBO_PROFILE_COUNT, "RGB combo profile count must match Vial combo slots");
_Static_assert(sizeof(xtreemze_rgb_profile_store_t) == XTREEMZE_RGB_PROFILE_EEPROM_SIZE, "RGB profile EEPROM reservation does not match store size");

static xtreemze_rgb_profile_store_t rgb_store;
static bool rgb_store_loaded;
static bool rgb_store_dirty;
static uint32_t rgb_profile_generation;
static bool combo_override_active;
static uint8_t active_combo_index = 0xFF;
static uint32_t combo_override_started;
static bool preview_active;
static xtreemze_rgb_profile_t preview_profile;
static uint32_t preview_started;
static bool legacy_passthrough_active;
static uint32_t legacy_passthrough_started;
static uint16_t legacy_passthrough_duration;
static bool has_last_applied_profile;
static xtreemze_rgb_profile_t last_applied_profile;
static uint32_t last_applied_generation = UINT32_MAX;
static uint8_t last_applied_layer = 0xFF;
static uint8_t last_applied_mods = 0xFF;
static uint8_t last_applied_combo = 0xFE;
static bool last_applied_preview;

static inline void *rgb_store_eeprom_address(void) {
    return (void *)(uintptr_t)(TOTAL_EEPROM_BYTE_COUNT - XTREEMZE_RGB_PROFILE_EEPROM_SIZE);
}

static inline bool rgb_profile_is_assigned(const xtreemze_rgb_profile_t *profile) {
    return profile->mode != XTREEMZE_RGB_PROFILE_UNASSIGNED;
}

static inline bool rgb_profile_equal(const xtreemze_rgb_profile_t *a, const xtreemze_rgb_profile_t *b) {
    return a->mode == b->mode && a->h == b->h && a->s == b->s && a->v == b->v && a->speed == b->speed;
}

static void rgb_profile_clear(xtreemze_rgb_profile_t *profile) {
    memset(profile, 0, sizeof(*profile));
    profile->mode = XTREEMZE_RGB_PROFILE_UNASSIGNED;
}

static xtreemze_rgb_profile_t rgb_profile_capture_current(void) {
    return (xtreemze_rgb_profile_t){
        .mode = rgb_matrix_is_enabled() ? rgb_matrix_get_mode() : 0,
        .h = rgb_matrix_get_hue(),
        .s = rgb_matrix_get_sat(),
        .v = rgb_matrix_get_val(),
        .speed = rgb_matrix_get_speed(),
    };
}

static bool rgb_profile_is_valid(const xtreemze_rgb_profile_t *profile, bool allow_unassigned) {
    if (profile->mode == XTREEMZE_RGB_PROFILE_UNASSIGNED) return allow_unassigned;
    if (profile->mode >= RGB_MATRIX_EFFECT_MAX) return false;
    if (profile->v > RGB_MATRIX_MAXIMUM_BRIGHTNESS) return false;
    return true;
}

static void rgb_profile_apply(const xtreemze_rgb_profile_t *profile) {
    if (!rgb_profile_is_assigned(profile)) return;
    if (profile->mode == 0) {
        rgb_matrix_disable_noeeprom();
        return;
    }
    rgb_matrix_enable_noeeprom();
    rgb_matrix_mode_noeeprom(profile->mode);
    rgb_matrix_sethsv_noeeprom(profile->h, profile->s, profile->v);
    rgb_matrix_set_speed_noeeprom(profile->speed);
}

static void rgb_profile_invalidate_runtime(void) {
    rgb_profile_generation++;
    has_last_applied_profile = false;
}

static bool read_legacy_user_data(xtreemze_legacy_user_data_t *legacy) {
    memset(legacy, 0, sizeof(*legacy));
    if (!eeconfig_is_user_datablock_valid()) return false;
    eeconfig_read_user_datablock(legacy, 0, sizeof(*legacy));
    return legacy->magic == XTREEMZE_LEGACY_USER_DATA_MAGIC && legacy->version == XTREEMZE_LEGACY_USER_DATA_VERSION;
}

static void rgb_store_save(void) {
    eeprom_update_block(&rgb_store, rgb_store_eeprom_address(), sizeof(rgb_store));
    rgb_store_dirty = false;
}

static void rgb_store_set_defaults(void) {
    memset(&rgb_store, 0, sizeof(rgb_store));
    rgb_store.magic = XTREEMZE_RGB_PROFILE_STORE_MAGIC;
    rgb_store.version = XTREEMZE_RGB_PROFILE_STORE_VERSION;
    rgb_store.combo_duration_ms = XTREEMZE_RGB_COMBO_MS_DEFAULT;
    rgb_store.global_profile = rgb_profile_capture_current();
    for (uint8_t i = 0; i < XTREEMZE_RGB_LAYER_PROFILE_COUNT; ++i) rgb_profile_clear(&rgb_store.layer_profiles[i]);
    for (uint8_t i = 0; i < XTREEMZE_RGB_MOD_PROFILE_COUNT; ++i) rgb_profile_clear(&rgb_store.mod_profiles[i]);
    for (uint8_t i = 0; i < XTREEMZE_RGB_COMBO_PROFILE_COUNT; ++i) rgb_profile_clear(&rgb_store.combo_profiles[i]);

    xtreemze_legacy_user_data_t legacy;
    if (read_legacy_user_data(&legacy)) {
        memcpy(rgb_store.layer_profiles, legacy.layer_profiles, sizeof(rgb_store.layer_profiles));
        memcpy(rgb_store.mod_profiles, legacy.mod_profiles, sizeof(rgb_store.mod_profiles));
        if (legacy.chord_override_ms >= XTREEMZE_RGB_COMBO_MS_MIN && legacy.chord_override_ms <= XTREEMZE_RGB_COMBO_MS_MAX) {
            rgb_store.combo_duration_ms = legacy.chord_override_ms;
        }
    }
}

static void rgb_store_ensure_loaded(void) {
    if (rgb_store_loaded) return;
    eeprom_read_block(&rgb_store, rgb_store_eeprom_address(), sizeof(rgb_store));
    if (rgb_store.magic != XTREEMZE_RGB_PROFILE_STORE_MAGIC || rgb_store.version != XTREEMZE_RGB_PROFILE_STORE_VERSION ||
        rgb_store.combo_duration_ms < XTREEMZE_RGB_COMBO_MS_MIN || rgb_store.combo_duration_ms > XTREEMZE_RGB_COMBO_MS_MAX ||
        !rgb_profile_is_valid(&rgb_store.global_profile, false)) {
        rgb_store_set_defaults();
        rgb_store_save();
    }
    rgb_store_loaded = true;
    rgb_profile_invalidate_runtime();
}

static xtreemze_rgb_profile_t *rgb_profile_for_scope(uint8_t scope, uint8_t index) {
    switch (scope) {
        case XTREEMZE_RGB_SCOPE_GLOBAL: return index == 0 ? &rgb_store.global_profile : NULL;
        case XTREEMZE_RGB_SCOPE_LAYER: return index < XTREEMZE_RGB_LAYER_PROFILE_COUNT ? &rgb_store.layer_profiles[index] : NULL;
        case XTREEMZE_RGB_SCOPE_MODIFIER: return index < XTREEMZE_RGB_MOD_PROFILE_COUNT ? &rgb_store.mod_profiles[index] : NULL;
        case XTREEMZE_RGB_SCOPE_COMBO: return index < XTREEMZE_RGB_COMBO_PROFILE_COUNT ? &rgb_store.combo_profiles[index] : NULL;
        default: return NULL;
    }
}

static const xtreemze_rgb_profile_t *rgb_profile_resolve(uint8_t layer, uint8_t mods) {
    if (combo_override_active && active_combo_index < XTREEMZE_RGB_COMBO_PROFILE_COUNT) {
        const xtreemze_rgb_profile_t *combo = &rgb_store.combo_profiles[active_combo_index];
        if (rgb_profile_is_assigned(combo)) return combo;
    }
    if ((mods & MOD_MASK_CTRL) != 0U && rgb_profile_is_assigned(&rgb_store.mod_profiles[0])) return &rgb_store.mod_profiles[0];
    if ((mods & MOD_MASK_GUI) != 0U && rgb_profile_is_assigned(&rgb_store.mod_profiles[1])) return &rgb_store.mod_profiles[1];
    if ((mods & MOD_MASK_SHIFT) != 0U && rgb_profile_is_assigned(&rgb_store.mod_profiles[2])) return &rgb_store.mod_profiles[2];
    if ((mods & MOD_MASK_ALT) != 0U && rgb_profile_is_assigned(&rgb_store.mod_profiles[3])) return &rgb_store.mod_profiles[3];
    if (layer < XTREEMZE_RGB_LAYER_PROFILE_COUNT && rgb_profile_is_assigned(&rgb_store.layer_profiles[layer])) return &rgb_store.layer_profiles[layer];
    return &rgb_store.global_profile;
}

static void rgb_profile_refresh(void) {
    rgb_store_ensure_loaded();
    if (legacy_passthrough_active) {
        if (timer_elapsed32(legacy_passthrough_started) <= legacy_passthrough_duration) return;
        legacy_passthrough_active = false;
        rgb_profile_invalidate_runtime();
    }
    if (preview_active && timer_elapsed32(preview_started) > XTREEMZE_RGB_PROFILE_PREVIEW_MS) {
        preview_active = false;
        rgb_profile_invalidate_runtime();
    }
    if (combo_override_active && timer_elapsed32(combo_override_started) > rgb_store.combo_duration_ms) {
        combo_override_active = false;
        active_combo_index = 0xFF;
        rgb_profile_invalidate_runtime();
    }

    const uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    const uint8_t mods = get_mods() | get_oneshot_mods();
    const uint8_t combo = combo_override_active ? active_combo_index : 0xFF;
    const xtreemze_rgb_profile_t *target = preview_active ? &preview_profile : rgb_profile_resolve(layer, mods);
    const bool state_changed = last_applied_generation != rgb_profile_generation || last_applied_layer != layer || last_applied_mods != mods || last_applied_combo != combo || last_applied_preview != preview_active;
    if (!state_changed && has_last_applied_profile && rgb_profile_equal(target, &last_applied_profile)) return;

    rgb_profile_apply(target);
    last_applied_profile = *target;
    has_last_applied_profile = true;
    last_applied_generation = rgb_profile_generation;
    last_applied_layer = layer;
    last_applied_mods = mods;
    last_applied_combo = combo;
    last_applied_preview = preview_active;
}

void housekeeping_task_user(void) {
    rgb_profile_refresh();
}

void combo_triggered_user(uint16_t combo_index, uint16_t keycode) {
    (void)keycode;
    rgb_store_ensure_loaded();
    if (combo_index >= XTREEMZE_RGB_COMBO_PROFILE_COUNT || !rgb_profile_is_assigned(&rgb_store.combo_profiles[combo_index])) return;
    active_combo_index = (uint8_t)combo_index;
    combo_override_started = timer_read32();
    combo_override_active = true;
    preview_active = false;
    rgb_profile_invalidate_runtime();
}

void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return;
    const uint16_t rgb_slay = QK_KB_0 + 10;
    const uint16_t rgb_smod = QK_KB_0 + 11;
    const uint16_t rgb_tchd = QK_KB_0 + 13;
    if (keycode != rgb_slay && keycode != rgb_smod && keycode != rgb_tchd) return;
    rgb_store_ensure_loaded();

    xtreemze_legacy_user_data_t legacy;
    if (!read_legacy_user_data(&legacy)) return;
    if (keycode == rgb_slay) {
        const uint8_t layer = get_highest_layer(layer_state | default_layer_state);
        if (layer < XTREEMZE_RGB_LAYER_PROFILE_COUNT) {
            rgb_store.layer_profiles[layer] = legacy.layer_profiles[layer];
            rgb_store_dirty = true;
            rgb_profile_invalidate_runtime();
            rgb_store_save();
        }
        return;
    }
    if (keycode == rgb_smod) {
        memcpy(rgb_store.mod_profiles, legacy.mod_profiles, sizeof(rgb_store.mod_profiles));
        rgb_store_dirty = true;
        rgb_profile_invalidate_runtime();
        rgb_store_save();
        return;
    }
    legacy_passthrough_duration = legacy.chord_override_ms;
    if (legacy_passthrough_duration < XTREEMZE_RGB_COMBO_MS_MIN || legacy_passthrough_duration > XTREEMZE_RGB_COMBO_MS_MAX) legacy_passthrough_duration = XTREEMZE_RGB_COMBO_MS_DEFAULT;
    legacy_passthrough_started = timer_read32();
    legacy_passthrough_active = true;
}

static void rgb_command_get_capabilities(uint8_t *data, uint8_t length) {
    if (length < 11) { data[0] = id_unhandled; return; }
    data[2] = XTREEMZE_RGB_PROFILE_PROTOCOL_VERSION;
    data[3] = XTREEMZE_RGB_SCOPE_FLAGS;
    data[4] = XTREEMZE_RGB_LAYER_PROFILE_COUNT;
    data[5] = XTREEMZE_RGB_MOD_PROFILE_COUNT;
    data[6] = XTREEMZE_RGB_COMBO_PROFILE_COUNT;
    data[7] = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
    data[8] = RGB_MATRIX_EFFECT_MAX - 1;
    data[9] = XTREEMZE_RGB_FIELD_FLAGS;
    data[10] = 1;
}

static void rgb_command_get_profile(uint8_t *data, uint8_t length) {
    if (length < 9) { data[0] = id_unhandled; return; }
    xtreemze_rgb_profile_t *profile = rgb_profile_for_scope(data[2], data[3]);
    if (profile == NULL) { data[0] = id_unhandled; return; }
    data[4] = profile->mode;
    data[5] = profile->h;
    data[6] = profile->s;
    data[7] = profile->v;
    data[8] = profile->speed;
}

static void rgb_command_set_profile(uint8_t *data, uint8_t length) {
    if (length < 9) { data[0] = id_unhandled; return; }
    xtreemze_rgb_profile_t *target = rgb_profile_for_scope(data[2], data[3]);
    const xtreemze_rgb_profile_t requested = {.mode = data[4], .h = data[5], .s = data[6], .v = data[7], .speed = data[8]};
    const bool is_global = data[2] == XTREEMZE_RGB_SCOPE_GLOBAL;
    if (target == NULL || !rgb_profile_is_valid(&requested, !is_global)) { data[0] = id_unhandled; return; }
    *target = requested;
    rgb_store_dirty = true;
    preview_active = false;
    rgb_profile_invalidate_runtime();
}

static void rgb_command_preview(uint8_t *data, uint8_t length) {
    if (length < 7) { data[0] = id_unhandled; return; }
    const xtreemze_rgb_profile_t requested = {.mode = data[2], .h = data[3], .s = data[4], .v = data[5], .speed = data[6]};
    if (!rgb_profile_is_valid(&requested, false)) { data[0] = id_unhandled; return; }
    preview_profile = requested;
    preview_started = timer_read32();
    preview_active = true;
    rgb_profile_invalidate_runtime();
}

/* One keyboard-level raw HID router owns the custom command namespace. */
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    if (xtreemze_halcyon_settings_raw_hid_receive(data, length)) {
        return;
    }
    if (length < 2 || data[0] != XTREEMZE_RGB_PROFILE_COMMAND) {
        data[0] = id_unhandled;
        return;
    }

    rgb_store_ensure_loaded();
    switch (data[1]) {
        case XTREEMZE_RGB_GET_CAPABILITIES:
            rgb_command_get_capabilities(data, length);
            break;
        case XTREEMZE_RGB_GET_PROFILE:
            rgb_command_get_profile(data, length);
            break;
        case XTREEMZE_RGB_SET_PROFILE:
            rgb_command_set_profile(data, length);
            break;
        case XTREEMZE_RGB_SAVE:
            if (rgb_store_dirty) rgb_store_save();
            break;
        case XTREEMZE_RGB_PREVIEW:
            rgb_command_preview(data, length);
            break;
        case XTREEMZE_RGB_GET_COMBO_DURATION:
            if (length < 4) { data[0] = id_unhandled; break; }
            data[2] = (uint8_t)(rgb_store.combo_duration_ms >> 8);
            data[3] = (uint8_t)(rgb_store.combo_duration_ms & 0xFF);
            break;
        case XTREEMZE_RGB_SET_COMBO_DURATION: {
            if (length < 4) { data[0] = id_unhandled; break; }
            const uint16_t duration = ((uint16_t)data[2] << 8) | data[3];
            if (duration < XTREEMZE_RGB_COMBO_MS_MIN || duration > XTREEMZE_RGB_COMBO_MS_MAX) { data[0] = id_unhandled; break; }
            rgb_store.combo_duration_ms = duration;
            rgb_store_dirty = true;
            break;
        }
        case XTREEMZE_RGB_CANCEL_PREVIEW:
            preview_active = false;
            rgb_profile_invalidate_runtime();
            break;
        default:
            data[0] = id_unhandled;
            break;
    }
}

#endif /* RGB_MATRIX_ENABLE && VIA_ENABLE */
