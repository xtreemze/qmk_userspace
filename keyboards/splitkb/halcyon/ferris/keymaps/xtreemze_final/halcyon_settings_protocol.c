// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#if defined(VIA_ENABLE) && defined(HALCYON_ENABLE)

#    include "halcyon_settings_protocol.h"
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

#    define XTREEMZE_HALCYON_SETTINGS_MAGIC 0x58484C43UL /* "XHLC" */
#    define XTREEMZE_HALCYON_SETTINGS_STORE_VERSION 1
#    define XTREEMZE_HALCYON_LAYER_COUNT 13
#    define XTREEMZE_HALCYON_MOD_INDICATOR_COUNT 10

#    define XTREEMZE_HALCYON_PATTERN_MS_DEFAULT 200
#    define XTREEMZE_HALCYON_PATTERN_MS_MIN 50
#    define XTREEMZE_HALCYON_PATTERN_MS_MAX 1000
#    define XTREEMZE_HALCYON_MOD_RECENT_MS_DEFAULT 2200
#    define XTREEMZE_HALCYON_MOD_RECENT_MS_MIN 0
#    define XTREEMZE_HALCYON_MOD_RECENT_MS_MAX 10000

#    define XTREEMZE_HALCYON_CAP_LAYER_COLORS (1U << 0)
#    define XTREEMZE_HALCYON_CAP_MOD_COLORS (1U << 1)
#    define XTREEMZE_HALCYON_CAP_TIMINGS (1U << 2)
#    define XTREEMZE_HALCYON_CAP_TELEMETRY (1U << 3)
#    define XTREEMZE_HALCYON_CAP_SPLIT_REPLICATION (1U << 4)
#    define XTREEMZE_HALCYON_CAP_FLAGS (XTREEMZE_HALCYON_CAP_LAYER_COLORS | XTREEMZE_HALCYON_CAP_MOD_COLORS | XTREEMZE_HALCYON_CAP_TIMINGS | XTREEMZE_HALCYON_CAP_TELEMETRY | XTREEMZE_HALCYON_CAP_SPLIT_REPLICATION)

#    define XTREEMZE_HALCYON_SYNC_CHUNK_DATA 27
#    define XTREEMZE_HALCYON_SYNC_CHUNK_COUNT ((XTREEMZE_HALCYON_SETTINGS_EEPROM_SIZE + XTREEMZE_HALCYON_SYNC_CHUNK_DATA - 1) / XTREEMZE_HALCYON_SYNC_CHUNK_DATA)
#    define XTREEMZE_HALCYON_SYNC_INTERVAL_MS 25
#    define XTREEMZE_HALCYON_SYNC_FLAG_PERSIST (1U << 0)

enum xtreemze_halcyon_settings_operation {
    XTREEMZE_HALCYON_GET_CAPABILITIES = 0x01,
    XTREEMZE_HALCYON_GET_LAYER_STYLE  = 0x02,
    XTREEMZE_HALCYON_SET_LAYER_STYLE  = 0x03,
    XTREEMZE_HALCYON_GET_MOD_STYLE    = 0x04,
    XTREEMZE_HALCYON_SET_MOD_STYLE    = 0x05,
    XTREEMZE_HALCYON_GET_TIMINGS      = 0x06,
    XTREEMZE_HALCYON_SET_TIMINGS      = 0x07,
    XTREEMZE_HALCYON_GET_DIM_STYLE    = 0x08,
    XTREEMZE_HALCYON_SET_DIM_STYLE    = 0x09,
    XTREEMZE_HALCYON_GET_TELEMETRY    = 0x0A,
    XTREEMZE_HALCYON_SAVE             = 0x0B,
    XTREEMZE_HALCYON_RESET            = 0x0C,
};

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} xtreemze_halcyon_hsv_t;

typedef struct {
    uint32_t               magic;
    uint8_t                version;
    uint8_t                flags;
    uint16_t               pattern_frame_ms;
    uint16_t               mod_recent_ms;
    xtreemze_halcyon_hsv_t layer_fg[XTREEMZE_HALCYON_LAYER_COUNT];
    xtreemze_halcyon_hsv_t layer_bg[XTREEMZE_HALCYON_LAYER_COUNT];
    xtreemze_halcyon_hsv_t mod_active[XTREEMZE_HALCYON_MOD_INDICATOR_COUNT];
    xtreemze_halcyon_hsv_t mod_dim;
    uint8_t                reserved[39];
} xtreemze_halcyon_settings_store_t;

typedef struct {
    uint8_t protocol_version;
    uint8_t generation;
    uint8_t flags;
    uint8_t offset;
    uint8_t size;
    uint8_t payload[XTREEMZE_HALCYON_SYNC_CHUNK_DATA];
} xtreemze_halcyon_sync_packet_t;

_Static_assert(sizeof(xtreemze_halcyon_settings_store_t) == XTREEMZE_HALCYON_SETTINGS_EEPROM_SIZE, "Halcyon settings EEPROM reservation does not match store size");
_Static_assert(sizeof(xtreemze_halcyon_sync_packet_t) == 32, "Halcyon settings split packet must fit default RPC buffer");
_Static_assert(XTREEMZE_HALCYON_SYNC_CHUNK_COUNT <= 8, "Halcyon settings chunk mask must fit one byte");

static const xtreemze_halcyon_hsv_t default_layer_fg[XTREEMZE_HALCYON_LAYER_COUNT] = {
    {59, 85, 192}, {122, 82, 187}, {28, 107, 219}, {254, 115, 230}, {95, 88, 194}, {170, 84, 182}, {12, 96, 206}, {210, 80, 186}, {76, 82, 188}, {140, 76, 184}, {20, 104, 211}, {224, 78, 202}, {29, 50, 211},
};

static const xtreemze_halcyon_hsv_t default_layer_bg[XTREEMZE_HALCYON_LAYER_COUNT] = {
    {122, 36, 82}, {28, 34, 82}, {254, 34, 80}, {59, 32, 78}, {210, 30, 78}, {95, 32, 78}, {170, 30, 78}, {12, 34, 80}, {132, 30, 78}, {20, 34, 80}, {224, 30, 78}, {76, 32, 78}, {146, 24, 66},
};

static const xtreemze_halcyon_hsv_t default_mod_active[XTREEMZE_HALCYON_MOD_INDICATOR_COUNT] = {
    {122, 82, 187},  /* Ctrl */
    {254, 115, 230}, /* GUI */
    {28, 107, 219},  /* Shift */
    {59, 85, 192},   /* Alt */
    {170, 84, 182},  /* Meh */
    {20, 104, 211},  /* Shift+Alt */
    {224, 78, 202},  /* Shift+GUI */
    {76, 82, 188},   /* Alt+GUI */
    {12, 96, 206},   /* Hyper */
    {95, 88, 194},   /* Caps Word */
};

static const xtreemze_halcyon_hsv_t default_mod_dim = {98, 23, 146};

static xtreemze_halcyon_settings_store_t settings_store;
static bool                              settings_store_loaded;
static bool                              settings_store_dirty;

static bool     sync_pending;
static bool     sync_persist;
static uint8_t  sync_generation;
static uint8_t  sync_chunk;
static uint32_t last_sync_attempt;
static bool     last_transport_connected;
static bool     last_master_state;

static xtreemze_halcyon_settings_store_t incoming_store;
static uint8_t                           incoming_generation;
static uint8_t                           incoming_mask;
static bool                              incoming_persist;

static inline void *settings_store_eeprom_address(void) {
    return (void *)(uintptr_t)(TOTAL_EEPROM_BYTE_COUNT - XTREEMZE_RGB_PROFILE_EEPROM_SIZE - XTREEMZE_HALCYON_SETTINGS_EEPROM_SIZE);
}

static bool timings_valid(uint16_t pattern_frame_ms, uint16_t mod_recent_ms) {
    return pattern_frame_ms >= XTREEMZE_HALCYON_PATTERN_MS_MIN && pattern_frame_ms <= XTREEMZE_HALCYON_PATTERN_MS_MAX && mod_recent_ms <= XTREEMZE_HALCYON_MOD_RECENT_MS_MAX;
}

static bool settings_store_valid(const xtreemze_halcyon_settings_store_t *store) {
    return store->magic == XTREEMZE_HALCYON_SETTINGS_MAGIC && store->version == XTREEMZE_HALCYON_SETTINGS_STORE_VERSION && timings_valid(store->pattern_frame_ms, store->mod_recent_ms);
}

static void settings_store_set_defaults(void) {
    memset(&settings_store, 0, sizeof(settings_store));
    settings_store.magic            = XTREEMZE_HALCYON_SETTINGS_MAGIC;
    settings_store.version          = XTREEMZE_HALCYON_SETTINGS_STORE_VERSION;
    settings_store.pattern_frame_ms = XTREEMZE_HALCYON_PATTERN_MS_DEFAULT;
    settings_store.mod_recent_ms    = XTREEMZE_HALCYON_MOD_RECENT_MS_DEFAULT;
    memcpy(settings_store.layer_fg, default_layer_fg, sizeof(default_layer_fg));
    memcpy(settings_store.layer_bg, default_layer_bg, sizeof(default_layer_bg));
    memcpy(settings_store.mod_active, default_mod_active, sizeof(default_mod_active));
    settings_store.mod_dim = default_mod_dim;
    settings_store_dirty   = true;
}

static void settings_store_save_local(void) {
    xtreemze_halcyon_settings_store_t persisted;
    eeprom_read_block(&persisted, settings_store_eeprom_address(), sizeof(persisted));
    if (memcmp(&persisted, &settings_store, sizeof(settings_store)) != 0) {
        eeprom_update_block(&settings_store, settings_store_eeprom_address(), sizeof(settings_store));
    }
    settings_store_dirty = false;
}

static void settings_store_ensure_loaded(void) {
    if (settings_store_loaded) {
        return;
    }

    eeprom_read_block(&settings_store, settings_store_eeprom_address(), sizeof(settings_store));
    if (!settings_store_valid(&settings_store)) {
        settings_store_set_defaults();
        settings_store_save_local();
    }
    settings_store_loaded = true;
}

static void schedule_sync(bool persist) {
    sync_generation++;
    sync_chunk   = 0;
    sync_pending = true;
    sync_persist = persist;
}

static void settings_sync_slave_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)out_buflen;
    (void)out_data;

    if (in_buflen != sizeof(xtreemze_halcyon_sync_packet_t)) {
        return;
    }

    const xtreemze_halcyon_sync_packet_t *packet = (const xtreemze_halcyon_sync_packet_t *)in_data;
    if (packet->protocol_version != XTREEMZE_HALCYON_SETTINGS_PROTOCOL_VERSION || packet->size > XTREEMZE_HALCYON_SYNC_CHUNK_DATA || (uint16_t)packet->offset + packet->size > sizeof(incoming_store)) {
        return;
    }

    if (packet->generation != incoming_generation) {
        incoming_generation = packet->generation;
        incoming_mask       = 0;
        incoming_persist    = false;
        memset(&incoming_store, 0, sizeof(incoming_store));
    }

    const uint8_t chunk = packet->offset / XTREEMZE_HALCYON_SYNC_CHUNK_DATA;
    if (chunk >= XTREEMZE_HALCYON_SYNC_CHUNK_COUNT || packet->offset != chunk * XTREEMZE_HALCYON_SYNC_CHUNK_DATA) {
        return;
    }

    memcpy((uint8_t *)&incoming_store + packet->offset, packet->payload, packet->size);
    incoming_mask |= (uint8_t)(1U << chunk);
    incoming_persist = incoming_persist || ((packet->flags & XTREEMZE_HALCYON_SYNC_FLAG_PERSIST) != 0U);

    const uint8_t complete_mask = (uint8_t)((1U << XTREEMZE_HALCYON_SYNC_CHUNK_COUNT) - 1U);
    if (incoming_mask != complete_mask || !settings_store_valid(&incoming_store)) {
        return;
    }

    settings_store        = incoming_store;
    settings_store_loaded = true;
    settings_store_dirty  = !incoming_persist;
    if (incoming_persist) {
        settings_store_save_local();
    }
}

static void sync_housekeeping(void) {
    const bool master    = is_keyboard_master();
    const bool connected = is_transport_connected();

    if (master != last_master_state || connected != last_transport_connected) {
        last_master_state        = master;
        last_transport_connected = connected;
        if (master && connected) {
            schedule_sync(true);
        }
    }

    if (!master || !connected || !sync_pending || timer_elapsed32(last_sync_attempt) < XTREEMZE_HALCYON_SYNC_INTERVAL_MS) {
        return;
    }

    settings_store_ensure_loaded();

    const uint16_t offset    = (uint16_t)sync_chunk * XTREEMZE_HALCYON_SYNC_CHUNK_DATA;
    const uint16_t remaining = sizeof(settings_store) - offset;
    const uint8_t  size      = remaining > XTREEMZE_HALCYON_SYNC_CHUNK_DATA ? XTREEMZE_HALCYON_SYNC_CHUNK_DATA : (uint8_t)remaining;

    xtreemze_halcyon_sync_packet_t packet = {
        .protocol_version = XTREEMZE_HALCYON_SETTINGS_PROTOCOL_VERSION,
        .generation       = sync_generation,
        .flags            = sync_persist ? XTREEMZE_HALCYON_SYNC_FLAG_PERSIST : 0,
        .offset           = (uint8_t)offset,
        .size             = size,
    };
    memcpy(packet.payload, (const uint8_t *)&settings_store + offset, size);

    last_sync_attempt = timer_read32();
    if (!transaction_rpc_send(XTREEMZE_HALCYON_SETTINGS_SYNC, sizeof(packet), &packet)) {
        return;
    }

    sync_chunk++;
    if (sync_chunk >= XTREEMZE_HALCYON_SYNC_CHUNK_COUNT) {
        sync_chunk   = 0;
        sync_pending = false;
        sync_persist = false;
    }
}

bool module_post_init_user(void) {
    settings_store_ensure_loaded();
    transaction_register_rpc(XTREEMZE_HALCYON_SETTINGS_SYNC, settings_sync_slave_handler);
    xtreemze_halcyon_display_protocol_init();
    last_master_state        = is_keyboard_master();
    last_transport_connected = is_transport_connected();
    if (last_master_state && last_transport_connected) {
        schedule_sync(true);
    }
    return true;
}

bool module_housekeeping_task_user(void) {
    sync_housekeeping();
    xtreemze_halcyon_display_protocol_housekeeping();
    return true;
}

#    ifdef HLC_TFT_DISPLAY
bool halcyon_display_color_override_user(uint8_t domain, uint8_t index, bool active, halcyon_display_hsv_t *color) {
    if (color == NULL) {
        return false;
    }
    settings_store_ensure_loaded();

    const xtreemze_halcyon_hsv_t *source = NULL;
    switch (domain) {
        case HALCYON_DISPLAY_COLOR_LAYER_FG:
            if (index < XTREEMZE_HALCYON_LAYER_COUNT) {
                source = &settings_store.layer_fg[index];
            }
            break;
        case HALCYON_DISPLAY_COLOR_LAYER_BG:
            if (index < XTREEMZE_HALCYON_LAYER_COUNT) {
                source = &settings_store.layer_bg[index];
            }
            break;
        case HALCYON_DISPLAY_COLOR_MODIFIER:
            if (active && index < XTREEMZE_HALCYON_MOD_INDICATOR_COUNT) {
                source = &settings_store.mod_active[index];
            } else if (!active) {
                source = &settings_store.mod_dim;
            }
            break;
        default:
            break;
    }

    if (source == NULL) {
        return false;
    }
    color->h = source->h;
    color->s = source->s;
    color->v = source->v;
    return true;
}

uint16_t halcyon_display_pattern_frame_ms_user(void) {
    settings_store_ensure_loaded();
    return settings_store.pattern_frame_ms;
}

uint16_t halcyon_display_mod_recent_ms_user(void) {
    settings_store_ensure_loaded();
    return settings_store.mod_recent_ms;
}
#    endif

static void command_get_capabilities(uint8_t *data, uint8_t length) {
    if (length < 12) {
        data[0] = id_unhandled;
        return;
    }
    data[2] = XTREEMZE_HALCYON_SETTINGS_PROTOCOL_VERSION;
    data[3] = XTREEMZE_HALCYON_CAP_FLAGS;
    data[4] = XTREEMZE_HALCYON_LAYER_COUNT;
    data[5] = XTREEMZE_HALCYON_MOD_INDICATOR_COUNT;
    data[6] = XTREEMZE_HALCYON_SETTINGS_STORE_VERSION;
    data[7] = (uint8_t)(XTREEMZE_HALCYON_PATTERN_MS_MIN / 10U);
    data[8] = (uint8_t)(XTREEMZE_HALCYON_PATTERN_MS_MAX / 10U);
    data[9] = (uint8_t)(XTREEMZE_HALCYON_MOD_RECENT_MS_MAX / 100U);
#    ifdef HLC_TFT_DISPLAY
    data[10] = 1;
#    else
    data[10] = 0;
#    endif
    data[11] = 1; /* deterministic encoder Repeat/Alternate Repeat policy */
}

static void command_get_layer(uint8_t *data, uint8_t length) {
    if (length < 9 || data[2] >= XTREEMZE_HALCYON_LAYER_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    const uint8_t index = data[2];
    data[3]             = settings_store.layer_fg[index].h;
    data[4]             = settings_store.layer_fg[index].s;
    data[5]             = settings_store.layer_fg[index].v;
    data[6]             = settings_store.layer_bg[index].h;
    data[7]             = settings_store.layer_bg[index].s;
    data[8]             = settings_store.layer_bg[index].v;
}

static void command_set_layer(uint8_t *data, uint8_t length) {
    if (length < 9 || data[2] >= XTREEMZE_HALCYON_LAYER_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    const uint8_t index            = data[2];
    settings_store.layer_fg[index] = (xtreemze_halcyon_hsv_t){data[3], data[4], data[5]};
    settings_store.layer_bg[index] = (xtreemze_halcyon_hsv_t){data[6], data[7], data[8]};
    settings_store_dirty           = true;
    schedule_sync(false);
}

static void command_get_modifier(uint8_t *data, uint8_t length) {
    if (length < 6 || data[2] >= XTREEMZE_HALCYON_MOD_INDICATOR_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    const xtreemze_halcyon_hsv_t *color = &settings_store.mod_active[data[2]];
    data[3]                             = color->h;
    data[4]                             = color->s;
    data[5]                             = color->v;
}

static void command_set_modifier(uint8_t *data, uint8_t length) {
    if (length < 6 || data[2] >= XTREEMZE_HALCYON_MOD_INDICATOR_COUNT) {
        data[0] = id_unhandled;
        return;
    }
    settings_store.mod_active[data[2]] = (xtreemze_halcyon_hsv_t){data[3], data[4], data[5]};
    settings_store_dirty               = true;
    schedule_sync(false);
}

static void command_get_timings(uint8_t *data, uint8_t length) {
    if (length < 6) {
        data[0] = id_unhandled;
        return;
    }
    data[2] = (uint8_t)(settings_store.pattern_frame_ms >> 8);
    data[3] = (uint8_t)(settings_store.pattern_frame_ms & 0xFF);
    data[4] = (uint8_t)(settings_store.mod_recent_ms >> 8);
    data[5] = (uint8_t)(settings_store.mod_recent_ms & 0xFF);
}

static void command_set_timings(uint8_t *data, uint8_t length) {
    if (length < 6) {
        data[0] = id_unhandled;
        return;
    }
    const uint16_t pattern = ((uint16_t)data[2] << 8) | data[3];
    const uint16_t recent  = ((uint16_t)data[4] << 8) | data[5];
    if (!timings_valid(pattern, recent)) {
        data[0] = id_unhandled;
        return;
    }
    settings_store.pattern_frame_ms = pattern;
    settings_store.mod_recent_ms    = recent;
    settings_store_dirty            = true;
    schedule_sync(false);
}

static void command_get_dim(uint8_t *data, uint8_t length) {
    if (length < 5) {
        data[0] = id_unhandled;
        return;
    }
    data[2] = settings_store.mod_dim.h;
    data[3] = settings_store.mod_dim.s;
    data[4] = settings_store.mod_dim.v;
}

static void command_set_dim(uint8_t *data, uint8_t length) {
    if (length < 5) {
        data[0] = id_unhandled;
        return;
    }
    settings_store.mod_dim = (xtreemze_halcyon_hsv_t){data[2], data[3], data[4]};
    settings_store_dirty   = true;
    schedule_sync(false);
}

static void command_get_telemetry(uint8_t *data, uint8_t length) {
    if (length < 9) {
        data[0] = id_unhandled;
        return;
    }
#    ifdef HLC_TFT_DISPLAY
    halcyon_host_telemetry_t telemetry;
    if (!halcyon_display_host_telemetry_user(&telemetry)) {
        data[0] = id_unhandled;
        return;
    }
    data[2] = telemetry.os;
    data[3] = telemetry.shortcut_family;
    data[4] = telemetry.source;
    data[5] = telemetry.event;
    data[6] = telemetry.is_master ? 1 : 0;
    data[7] = 1;
#    else
    data[2] = 0;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = is_keyboard_master() ? 1 : 0;
    data[7] = 0;
#    endif
    data[8] = is_transport_connected() ? 1 : 0;
}

bool xtreemze_halcyon_settings_raw_hid_receive(uint8_t *data, uint8_t length) {
    if (xtreemze_halcyon_display_raw_hid_receive(data, length)) {
        return true;
    }
    if (length < 2 || data[0] != XTREEMZE_HALCYON_SETTINGS_COMMAND) {
        return false;
    }

    settings_store_ensure_loaded();

    switch (data[1]) {
        case XTREEMZE_HALCYON_GET_CAPABILITIES:
            command_get_capabilities(data, length);
            break;
        case XTREEMZE_HALCYON_GET_LAYER_STYLE:
            command_get_layer(data, length);
            break;
        case XTREEMZE_HALCYON_SET_LAYER_STYLE:
            command_set_layer(data, length);
            break;
        case XTREEMZE_HALCYON_GET_MOD_STYLE:
            command_get_modifier(data, length);
            break;
        case XTREEMZE_HALCYON_SET_MOD_STYLE:
            command_set_modifier(data, length);
            break;
        case XTREEMZE_HALCYON_GET_TIMINGS:
            command_get_timings(data, length);
            break;
        case XTREEMZE_HALCYON_SET_TIMINGS:
            command_set_timings(data, length);
            break;
        case XTREEMZE_HALCYON_GET_DIM_STYLE:
            command_get_dim(data, length);
            break;
        case XTREEMZE_HALCYON_SET_DIM_STYLE:
            command_set_dim(data, length);
            break;
        case XTREEMZE_HALCYON_GET_TELEMETRY:
            command_get_telemetry(data, length);
            break;
        case XTREEMZE_HALCYON_SAVE:
            if (settings_store_dirty) {
                settings_store_save_local();
            }
            schedule_sync(true);
            break;
        case XTREEMZE_HALCYON_RESET:
            settings_store_set_defaults();
            schedule_sync(false);
            break;
        default:
            data[0] = id_unhandled;
            break;
    }
    return true;
}

#endif /* VIA_ENABLE && HALCYON_ENABLE */
