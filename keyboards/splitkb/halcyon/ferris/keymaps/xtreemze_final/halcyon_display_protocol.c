// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#if defined(VIA_ENABLE) && defined(HALCYON_ENABLE)

#    include "halcyon_display_protocol.h"
#    include "eeprom.h"
#    include "split_util.h"
#    include "timer.h"
#    include "transactions.h"
#    include "via.h"
#    ifdef HLC_TFT_DISPLAY
#        include "hlc_tft_display/hlc_tft_display.h"
#    endif

#    include <stdint.h>
#    include <string.h>

#    define XTREEMZE_HALCYON_DISPLAY_MAGIC 0x58484450UL /* "XHDP" */
#    define XTREEMZE_HALCYON_DISPLAY_STORE_VERSION 1
#    define XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT 13
#    define XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT 10
#    define XTREEMZE_HALCYON_DISPLAY_MOTIF_COUNT 13
#    define XTREEMZE_HALCYON_LAYER_LABEL_SIZE 9
#    define XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE 5

#    define XTREEMZE_HALCYON_TILE_MIN 12
#    define XTREEMZE_HALCYON_TILE_MAX 48
#    define XTREEMZE_HALCYON_MOTION_MAX 6
#    define XTREEMZE_HALCYON_PULSE_MAX 4

#    define XTREEMZE_HALCYON_DISPLAY_CAP_LAYER_LABELS (1U << 0)
#    define XTREEMZE_HALCYON_DISPLAY_CAP_MODIFIER_LABELS (1U << 1)
#    define XTREEMZE_HALCYON_DISPLAY_CAP_PATTERNS (1U << 2)
#    define XTREEMZE_HALCYON_DISPLAY_CAP_SPLIT_REPLICATION (1U << 3)
#    define XTREEMZE_HALCYON_DISPLAY_CAP_FLAGS (XTREEMZE_HALCYON_DISPLAY_CAP_LAYER_LABELS | XTREEMZE_HALCYON_DISPLAY_CAP_MODIFIER_LABELS | XTREEMZE_HALCYON_DISPLAY_CAP_PATTERNS | XTREEMZE_HALCYON_DISPLAY_CAP_SPLIT_REPLICATION)

#    define XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA 27
#    define XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_COUNT ((XTREEMZE_HALCYON_DISPLAY_EEPROM_SIZE + XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA - 1) / XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA)
#    define XTREEMZE_HALCYON_DISPLAY_SYNC_INTERVAL_MS 25
#    define XTREEMZE_HALCYON_DISPLAY_SYNC_FLAG_PERSIST (1U << 0)

enum xtreemze_halcyon_display_operation {
    XTREEMZE_HALCYON_DISPLAY_GET_CAPABILITIES   = 0x01,
    XTREEMZE_HALCYON_DISPLAY_GET_LAYER          = 0x02,
    XTREEMZE_HALCYON_DISPLAY_SET_LAYER          = 0x03,
    XTREEMZE_HALCYON_DISPLAY_GET_MODIFIER_LABEL = 0x04,
    XTREEMZE_HALCYON_DISPLAY_SET_MODIFIER_LABEL = 0x05,
    XTREEMZE_HALCYON_DISPLAY_SAVE               = 0x06,
    XTREEMZE_HALCYON_DISPLAY_RESET              = 0x07,
};

typedef struct {
    uint8_t motif;
    uint8_t tile_width;
    uint8_t tile_height;
    uint8_t motion_amplitude;
    uint8_t pulse_amplitude;
} xtreemze_halcyon_pattern_t;

typedef struct {
    uint32_t                   magic;
    uint8_t                    version;
    uint8_t                    flags;
    char                       layer_labels[XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT][XTREEMZE_HALCYON_LAYER_LABEL_SIZE];
    char                       modifier_labels[XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT][XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE];
    xtreemze_halcyon_pattern_t patterns[XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT];
    uint8_t                    reserved[18];
} xtreemze_halcyon_display_store_t;

typedef struct {
    uint8_t protocol_version;
    uint8_t generation;
    uint8_t flags;
    uint8_t offset;
    uint8_t size;
    uint8_t payload[XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA];
} xtreemze_halcyon_display_sync_packet_t;

_Static_assert(sizeof(xtreemze_halcyon_display_store_t) == XTREEMZE_HALCYON_DISPLAY_EEPROM_SIZE, "Halcyon display EEPROM reservation does not match store size");
_Static_assert(sizeof(xtreemze_halcyon_display_sync_packet_t) == 32, "Halcyon display split packet must fit default RPC buffer");
_Static_assert(XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_COUNT <= 16, "Halcyon display chunk mask must fit uint16_t");
_Static_assert(XTREEMZE_HALCYON_DISPLAY_EEPROM_SIZE <= 256, "Halcyon display sync offset is one byte");

static const char *const default_layer_labels[XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT] = {
    "MOUSE", "QWERTY", "COLEMAK", "NUMSYMS", "NUMFLIP", "ONESHOT", "EDITING", "FNSYMS", "FNFLIP", "SYMBOLS", "RGBHUE", "RGBVAL", "BKLIGHT",
};

static const char *const default_modifier_labels[XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT] = {
    "Ctl", "Gui", "Shf", "Alt", "Meh", "S+A", "S+G", "A+G", "Hypr", "Caps",
};

static xtreemze_halcyon_display_store_t display_store;
static bool                             display_store_loaded;
static bool                             display_store_dirty;

static bool     display_sync_pending;
static bool     display_sync_persist;
static uint8_t  display_sync_generation;
static uint8_t  display_sync_chunk;
static uint32_t display_last_sync_attempt;
static bool     display_last_transport_connected;
static bool     display_last_master_state;

static xtreemze_halcyon_display_store_t display_incoming_store;
static uint8_t                          display_incoming_generation;
static uint16_t                         display_incoming_mask;
static bool                             display_incoming_persist;

static inline void *display_store_eeprom_address(void) {
    return (void *)(uintptr_t)(TOTAL_EEPROM_BYTE_COUNT - XTREEMZE_RGB_PROFILE_EEPROM_SIZE - XTREEMZE_HALCYON_SETTINGS_EEPROM_SIZE - XTREEMZE_HALCYON_DISPLAY_EEPROM_SIZE);
}

static bool display_label_valid(const char *label, uint8_t size) {
    bool terminated = false;
    for (uint8_t index = 0; index < size; ++index) {
        const uint8_t value = (uint8_t)label[index];
        if (value == 0) {
            terminated = true;
            break;
        }
        if (value < 0x20 || value > 0x7E) {
            return false;
        }
    }
    return terminated;
}

static bool display_pattern_valid(const xtreemze_halcyon_pattern_t *pattern) {
    return pattern->motif < XTREEMZE_HALCYON_DISPLAY_MOTIF_COUNT && pattern->tile_width >= XTREEMZE_HALCYON_TILE_MIN && pattern->tile_width <= XTREEMZE_HALCYON_TILE_MAX && pattern->tile_height >= XTREEMZE_HALCYON_TILE_MIN && pattern->tile_height <= XTREEMZE_HALCYON_TILE_MAX && pattern->motion_amplitude <= XTREEMZE_HALCYON_MOTION_MAX && pattern->pulse_amplitude <= XTREEMZE_HALCYON_PULSE_MAX;
}

static bool display_store_valid(const xtreemze_halcyon_display_store_t *store) {
    if (store->magic != XTREEMZE_HALCYON_DISPLAY_MAGIC || store->version != XTREEMZE_HALCYON_DISPLAY_STORE_VERSION) {
        return false;
    }
    for (uint8_t index = 0; index < XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT; ++index) {
        if (!display_label_valid(store->layer_labels[index], XTREEMZE_HALCYON_LAYER_LABEL_SIZE) || !display_pattern_valid(&store->patterns[index])) {
            return false;
        }
    }
    for (uint8_t index = 0; index < XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT; ++index) {
        if (!display_label_valid(store->modifier_labels[index], XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE)) {
            return false;
        }
    }
    return true;
}

static void display_store_set_defaults(void) {
    memset(&display_store, 0, sizeof(display_store));
    display_store.magic   = XTREEMZE_HALCYON_DISPLAY_MAGIC;
    display_store.version = XTREEMZE_HALCYON_DISPLAY_STORE_VERSION;
    for (uint8_t index = 0; index < XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT; ++index) {
        strncpy(display_store.layer_labels[index], default_layer_labels[index], XTREEMZE_HALCYON_LAYER_LABEL_SIZE - 1);
        display_store.patterns[index] = (xtreemze_halcyon_pattern_t){
            .motif            = index,
            .tile_width       = 24,
            .tile_height      = 24,
            .motion_amplitude = 1,
            .pulse_amplitude  = 1,
        };
    }
    for (uint8_t index = 0; index < XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT; ++index) {
        strncpy(display_store.modifier_labels[index], default_modifier_labels[index], XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE - 1);
    }
    display_store_dirty = true;
}

static void display_store_save_local(void) {
    xtreemze_halcyon_display_store_t persisted;
    eeprom_read_block(&persisted, display_store_eeprom_address(), sizeof(persisted));
    if (memcmp(&persisted, &display_store, sizeof(display_store)) != 0) {
        eeprom_update_block(&display_store, display_store_eeprom_address(), sizeof(display_store));
    }
    display_store_dirty = false;
}

static void display_store_ensure_loaded(void) {
    if (display_store_loaded) {
        return;
    }
    eeprom_read_block(&display_store, display_store_eeprom_address(), sizeof(display_store));
    if (!display_store_valid(&display_store)) {
        display_store_set_defaults();
        display_store_save_local();
    }
    display_store_loaded = true;
}

static void display_schedule_sync(bool persist) {
    display_sync_generation++;
    display_sync_chunk   = 0;
    display_sync_pending = true;
    display_sync_persist = persist;
}

static void display_sync_slave_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)out_buflen;
    (void)out_data;

    if (in_buflen != sizeof(xtreemze_halcyon_display_sync_packet_t)) {
        return;
    }
    const xtreemze_halcyon_display_sync_packet_t *packet = (const xtreemze_halcyon_display_sync_packet_t *)in_data;
    if (packet->protocol_version != XTREEMZE_HALCYON_DISPLAY_PROTOCOL_VERSION || packet->size > XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA || (uint16_t)packet->offset + packet->size > sizeof(display_incoming_store)) {
        return;
    }

    if (packet->generation != display_incoming_generation) {
        display_incoming_generation = packet->generation;
        display_incoming_mask       = 0;
        display_incoming_persist    = false;
        memset(&display_incoming_store, 0, sizeof(display_incoming_store));
    }

    const uint8_t chunk = packet->offset / XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA;
    if (chunk >= XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_COUNT || packet->offset != chunk * XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA) {
        return;
    }

    memcpy((uint8_t *)&display_incoming_store + packet->offset, packet->payload, packet->size);
    display_incoming_mask |= (uint16_t)(1U << chunk);
    display_incoming_persist = display_incoming_persist || ((packet->flags & XTREEMZE_HALCYON_DISPLAY_SYNC_FLAG_PERSIST) != 0U);

    const uint16_t complete_mask = (uint16_t)((1U << XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_COUNT) - 1U);
    if (display_incoming_mask != complete_mask || !display_store_valid(&display_incoming_store)) {
        return;
    }

    display_store        = display_incoming_store;
    display_store_loaded = true;
    display_store_dirty  = !display_incoming_persist;
    if (display_incoming_persist) {
        display_store_save_local();
    }
}

static void display_sync_housekeeping(void) {
    const bool master    = is_keyboard_master();
    const bool connected = is_transport_connected();

    if (master != display_last_master_state || connected != display_last_transport_connected) {
        display_last_master_state        = master;
        display_last_transport_connected = connected;
        if (master && connected) {
            display_schedule_sync(true);
        }
    }

    if (!master || !connected || !display_sync_pending || timer_elapsed32(display_last_sync_attempt) < XTREEMZE_HALCYON_DISPLAY_SYNC_INTERVAL_MS) {
        return;
    }

    display_store_ensure_loaded();
    const uint16_t                         offset    = (uint16_t)display_sync_chunk * XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA;
    const uint16_t                         remaining = sizeof(display_store) - offset;
    const uint8_t                          size      = remaining > XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA ? XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_DATA : (uint8_t)remaining;
    xtreemze_halcyon_display_sync_packet_t packet    = {
           .protocol_version = XTREEMZE_HALCYON_DISPLAY_PROTOCOL_VERSION,
           .generation       = display_sync_generation,
           .flags            = display_sync_persist ? XTREEMZE_HALCYON_DISPLAY_SYNC_FLAG_PERSIST : 0,
           .offset           = (uint8_t)offset,
           .size             = size,
    };
    memcpy(packet.payload, (const uint8_t *)&display_store + offset, size);

    display_last_sync_attempt = timer_read32();
    if (!transaction_rpc_send(XTREEMZE_HALCYON_DISPLAY_SYNC, sizeof(packet), &packet)) {
        return;
    }

    display_sync_chunk++;
    if (display_sync_chunk >= XTREEMZE_HALCYON_DISPLAY_SYNC_CHUNK_COUNT) {
        display_sync_chunk   = 0;
        display_sync_pending = false;
        display_sync_persist = false;
    }
}

void xtreemze_halcyon_display_protocol_init(void) {
    display_store_ensure_loaded();
    transaction_register_rpc(XTREEMZE_HALCYON_DISPLAY_SYNC, display_sync_slave_handler);
    display_last_master_state        = is_keyboard_master();
    display_last_transport_connected = is_transport_connected();
    if (display_last_master_state && display_last_transport_connected) {
        display_schedule_sync(true);
    }
}

void xtreemze_halcyon_display_protocol_housekeeping(void) {
    display_sync_housekeeping();
}

#    ifdef HLC_TFT_DISPLAY
const char *halcyon_display_layer_label_override_user(uint8_t layer) {
    display_store_ensure_loaded();
    return layer < XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT ? display_store.layer_labels[layer] : NULL;
}

const char *halcyon_display_modifier_label_override_user(uint8_t modifier) {
    display_store_ensure_loaded();
    return modifier < XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT ? display_store.modifier_labels[modifier] : NULL;
}

bool halcyon_display_pattern_override_user(uint8_t layer, halcyon_display_pattern_t *pattern) {
    if (pattern == NULL || layer >= XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT) {
        return false;
    }
    display_store_ensure_loaded();
    const xtreemze_halcyon_pattern_t *source = &display_store.patterns[layer];
    pattern->motif                           = source->motif;
    pattern->tile_width                      = source->tile_width;
    pattern->tile_height                     = source->tile_height;
    pattern->motion_amplitude                = source->motion_amplitude;
    pattern->pulse_amplitude                 = source->pulse_amplitude;
    return true;
}
#    endif

static void display_command_get_capabilities(uint8_t *data, uint8_t length) {
    if (length < 14) {
        data[0] = id_unhandled;
        return;
    }
    data[2]  = XTREEMZE_HALCYON_DISPLAY_PROTOCOL_VERSION;
    data[3]  = XTREEMZE_HALCYON_DISPLAY_CAP_FLAGS;
    data[4]  = XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT;
    data[5]  = XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT;
    data[6]  = XTREEMZE_HALCYON_DISPLAY_MOTIF_COUNT;
    data[7]  = XTREEMZE_HALCYON_LAYER_LABEL_SIZE - 1;
    data[8]  = XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE - 1;
    data[9]  = XTREEMZE_HALCYON_TILE_MIN;
    data[10] = XTREEMZE_HALCYON_TILE_MAX;
    data[11] = XTREEMZE_HALCYON_MOTION_MAX;
    data[12] = XTREEMZE_HALCYON_PULSE_MAX;
    data[13] = XTREEMZE_HALCYON_DISPLAY_STORE_VERSION;
}

static void display_command_get_layer(uint8_t *data, uint8_t length) {
    if (length < 17 || data[2] >= XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    const uint8_t index = data[2];
    memcpy(&data[3], display_store.layer_labels[index], XTREEMZE_HALCYON_LAYER_LABEL_SIZE);
    data[12] = display_store.patterns[index].motif;
    data[13] = display_store.patterns[index].tile_width;
    data[14] = display_store.patterns[index].tile_height;
    data[15] = display_store.patterns[index].motion_amplitude;
    data[16] = display_store.patterns[index].pulse_amplitude;
}

static void display_command_set_layer(uint8_t *data, uint8_t length) {
    if (length < 17 || data[2] >= XTREEMZE_HALCYON_DISPLAY_LAYER_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    char requested_label[XTREEMZE_HALCYON_LAYER_LABEL_SIZE];
    memcpy(requested_label, &data[3], sizeof(requested_label));
    requested_label[XTREEMZE_HALCYON_LAYER_LABEL_SIZE - 1] = '\0';
    const xtreemze_halcyon_pattern_t requested_pattern     = {
            .motif            = data[12],
            .tile_width       = data[13],
            .tile_height      = data[14],
            .motion_amplitude = data[15],
            .pulse_amplitude  = data[16],
    };
    if (!display_label_valid(requested_label, sizeof(requested_label)) || !display_pattern_valid(&requested_pattern)) {
        data[0] = id_unhandled;
        return;
    }
    const uint8_t index = data[2];
    memcpy(display_store.layer_labels[index], requested_label, sizeof(requested_label));
    display_store.patterns[index] = requested_pattern;
    display_store_dirty           = true;
    display_schedule_sync(false);
}

static void display_command_get_modifier_label(uint8_t *data, uint8_t length) {
    if (length < 8 || data[2] >= XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    memcpy(&data[3], display_store.modifier_labels[data[2]], XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE);
}

static void display_command_set_modifier_label(uint8_t *data, uint8_t length) {
    if (length < 8 || data[2] >= XTREEMZE_HALCYON_DISPLAY_MODIFIER_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    char requested_label[XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE];
    memcpy(requested_label, &data[3], sizeof(requested_label));
    requested_label[XTREEMZE_HALCYON_MODIFIER_LABEL_SIZE - 1] = '\0';
    if (!display_label_valid(requested_label, sizeof(requested_label))) {
        data[0] = id_unhandled;
        return;
    }
    memcpy(display_store.modifier_labels[data[2]], requested_label, sizeof(requested_label));
    display_store_dirty = true;
    display_schedule_sync(false);
}

bool xtreemze_halcyon_display_raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 2 || data[0] != XTREEMZE_HALCYON_DISPLAY_COMMAND) {
        return false;
    }

    display_store_ensure_loaded();
    switch (data[1]) {
        case XTREEMZE_HALCYON_DISPLAY_GET_CAPABILITIES:
            display_command_get_capabilities(data, length);
            break;
        case XTREEMZE_HALCYON_DISPLAY_GET_LAYER:
            display_command_get_layer(data, length);
            break;
        case XTREEMZE_HALCYON_DISPLAY_SET_LAYER:
            display_command_set_layer(data, length);
            break;
        case XTREEMZE_HALCYON_DISPLAY_GET_MODIFIER_LABEL:
            display_command_get_modifier_label(data, length);
            break;
        case XTREEMZE_HALCYON_DISPLAY_SET_MODIFIER_LABEL:
            display_command_set_modifier_label(data, length);
            break;
        case XTREEMZE_HALCYON_DISPLAY_SAVE:
            if (display_store_dirty) {
                display_store_save_local();
            }
            display_schedule_sync(true);
            break;
        case XTREEMZE_HALCYON_DISPLAY_RESET:
            display_store_set_defaults();
            display_schedule_sync(false);
            break;
        default:
            data[0] = id_unhandled;
            break;
    }
    return true;
}

#endif /* VIA_ENABLE && HALCYON_ENABLE */
