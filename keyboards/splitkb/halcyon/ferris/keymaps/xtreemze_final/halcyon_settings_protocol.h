// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define XTREEMZE_HALCYON_SETTINGS_COMMAND 0xF1
#define XTREEMZE_HALCYON_SETTINGS_PROTOCOL_VERSION 1

/* Returns true when the packet belongs to this protocol. */
bool xtreemze_halcyon_settings_raw_hid_receive(uint8_t *data, uint8_t length);

/* Modal keyboard editor uses the same runtime settings owner. */
void xtreemze_palette_settings_begin(void);
void xtreemze_palette_settings_end(bool save);
void xtreemze_palette_settings_color(uint8_t scope, uint8_t index, uint8_t h, uint8_t s, uint8_t v);
bool xtreemze_palette_process(uint16_t keycode, keyrecord_t *record);
bool xtreemze_palette_is_active(void);
void xtreemze_palette_settings_preview(uint8_t scope, uint8_t index, uint8_t parameter, uint8_t h, uint8_t s);
const char *xtreemze_palette_settings_label(void);
