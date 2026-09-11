// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define XTREEMZE_HALCYON_SETTINGS_COMMAND 0xF1
#define XTREEMZE_HALCYON_SETTINGS_PROTOCOL_VERSION 1

/* Returns true when the packet belongs to this protocol. */
bool xtreemze_halcyon_settings_raw_hid_receive(uint8_t *data, uint8_t length);
