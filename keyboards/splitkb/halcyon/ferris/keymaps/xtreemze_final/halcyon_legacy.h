/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2023 splitkb.com <support@splitkb.com> */

#pragma once

/* Legacy matrix/layout from splitkb/qmk_userspace e0900a55dbb08ff5770236dd6129fedacab620e4.
 * Keep the custom Vial UID, layer count and EEPROM settings in config.h. */
#define HALCYON_LEGACY

#undef MATRIX_ROWS
#define MATRIX_ROWS 10
#define LAYOUT_ferris_hlc(k0E, k0D, k0C, k0B, k0A, k5A, k5B, k5C, k5D, k5E, k1E, k1D, k1C, k1B, k1A, k6A, k6B, k6C, k6D, k6E, k2E, k2D, k2C, k2B, k2A, k7A, k7B, k7C, k7D, k7E, k3B, k3A, k8A, k8B, k4A, k4B, k4C, k4D, k4E, k9A, k9B, k9C, k9D, k9E) { \
    {k0A, k0B, k0C, k0D, k0E}, \
    {k1A, k1B, k1C, k1D, k1E}, \
    {k2A, k2B, k2C, k2D, k2E}, \
    {k3A, k3B, KC_NO, KC_NO, KC_NO}, \
    {k4A, k4B, k4C, k4D, k4E}, \
    {k5A, k5B, k5C, k5D, k5E}, \
    {k6A, k6B, k6C, k6D, k6E}, \
    {k7A, k7B, k7C, k7D, k7E}, \
    {k8A, k8B, KC_NO, KC_NO, KC_NO}, \
    {k9A, k9B, k9C, k9D, k9E} \
}
