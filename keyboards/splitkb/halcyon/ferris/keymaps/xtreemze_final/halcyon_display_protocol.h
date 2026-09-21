// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define XTREEMZE_HALCYON_DISPLAY_COMMAND 0xF2
#define XTREEMZE_HALCYON_DISPLAY_PROTOCOL_VERSION 1

void xtreemze_halcyon_display_protocol_init(void);
void xtreemze_halcyon_display_protocol_housekeeping(void);

/* Returns true when the packet belongs to the TFT display protocol. */
bool xtreemze_halcyon_display_raw_hid_receive(uint8_t *data, uint8_t length);
