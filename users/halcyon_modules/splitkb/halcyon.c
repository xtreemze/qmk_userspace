// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "gpio.h"
#include "halcyon.h"
#include "transactions.h"
#include "split_util.h"
#include "_wait.h"

__attribute__((weak)) void module_suspend_power_down_kb(void);
__attribute__((weak)) void module_suspend_wakeup_init_kb(void);

__attribute__((weak)) bool module_post_init_kb(void) {
    return module_post_init_user();
}
__attribute__((weak)) bool module_housekeeping_task_kb(void) {
    return module_housekeeping_task_user();
}
__attribute__((weak)) bool display_module_housekeeping_task_kb(bool second_display) {
    return display_module_housekeeping_task_user(second_display);
}

__attribute__((weak)) bool module_post_init_user(void) {
    return true;
}
__attribute__((weak)) bool module_housekeeping_task_user(void) {
    return true;
}
__attribute__((weak)) bool display_module_housekeeping_task_user(bool second_display) {
    return true;
}

module_t module_master;
module_t module;
#ifdef HLC_NONE
    module_t module = hlc_none;
#endif
#ifdef HLC_CIRQUE_TRACKPAD
    module_t module = hlc_cirque_trackpad;
#endif
#ifdef HLC_ENCODER
    module_t module = hlc_encoder;
#endif
#ifdef HLC_ENCODER_REV2
    module_t module = hlc_encoder;
#endif
#ifdef HLC_TFT_DISPLAY
    module_t module = hlc_tft_display;
#endif

bool backlight_off = false;

#ifndef BACKLIGHT_ON_STATE
#    define BACKLIGHT_ON_STATE 1
#endif

static void halcyon_backlight_set(bool enabled) {
#ifdef BACKLIGHT_ENABLE
    // Only the USB master owns brightness. QMK synchronizes its effective level
    // to the other half, including the transient zero used during inactivity.
    if (!is_keyboard_master()) {
        return;
    }
    if (enabled) {
        // Restore the user's saved level AND enable flag, including manual off.
        backlight_init();
    } else {
        // Do not persist an idle/suspend event as the user's brightness choice.
        backlight_level_noeeprom(0);
    }
#elif defined(HLC_TFT_DISPLAY) && defined(BACKLIGHT_PIN)
    gpio_set_pin_output(BACKLIGHT_PIN);
    gpio_write_pin(BACKLIGHT_PIN, enabled ? BACKLIGHT_ON_STATE : !BACKLIGHT_ON_STATE);
#endif
}

void backlight_wakeup(void) {
    backlight_off = false;
    halcyon_backlight_set(true);
}

void backlight_suspend(void) {
    backlight_off = true;
    halcyon_backlight_set(false);
}

// Upstream's module-button implementation now owns process_record_kb(). Use the
// earlier QMK pre-process hook for the fork's wake-on-key behavior so both paths
// compose without duplicate keyboard hooks.
bool pre_process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed && backlight_off) {
        backlight_wakeup();
    }
    return pre_process_record_user(keycode, record);
}

void module_sync_slave_handler(uint8_t initiator2target_buffer_size, const void* initiator2target_buffer, uint8_t target2initiator_buffer_size, void* target2initiator_buffer) {
    if (initiator2target_buffer_size == sizeof(module)) {
        memcpy(&module_master, initiator2target_buffer, sizeof(module_master));
    }
}

void suspend_power_down_kb(void) {
    backlight_suspend();
    module_suspend_power_down_kb();
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    // QMK has restored the saved backlight state. Re-evaluate the idle timeout
    // in housekeeping without touching PWM, EEPROM or the display in this hook.
    backlight_off = false;
    module_suspend_wakeup_init_kb();
    suspend_wakeup_init_user();
}

void keyboard_post_init_kb(void) {
    transaction_register_rpc(MODULE_SYNC, module_sync_slave_handler);
    module_post_init_kb();
    keyboard_post_init_user();
}

void housekeeping_task_kb(void) {
    if (is_keyboard_master()) {
        static bool synced = false;

        if (!synced && is_transport_connected()) {
            transaction_rpc_send(MODULE_SYNC, sizeof(module), &module);
            wait_ms(10);
            // Good moment to make sure the backlight wakes up after boot for both halves.
            backlight_wakeup();
            synced = true;
        }
    }

    // Keep wake/suspend brightness state current before any display housekeeping.
    // The TFT path may use that state while recovering or drawing this pass.
    if (last_input_activity_elapsed() <= HLC_BACKLIGHT_TIMEOUT) {
        if (backlight_off) {
            backlight_wakeup();
        }
    } else if (!backlight_off) {
        backlight_suspend();
    }

    if (is_keyboard_master()) {
        display_module_housekeeping_task_kb(false);
    } else {
        display_module_housekeeping_task_kb(module_master == hlc_tft_display);
    }

    module_housekeeping_task_kb();
    housekeeping_task_user();
}

#ifdef POINTING_DEVICE_ENABLE
report_mouse_t pointing_device_task_combined_kb(report_mouse_t left_report, report_mouse_t right_report) {
    // Only runs on master. If master is right and master is not a Cirque
    // trackpad, the input would otherwise be inverted.
    if (module != hlc_cirque_trackpad && !is_keyboard_left()) {
        mouse_xy_report_t x = left_report.x;
        mouse_xy_report_t y = left_report.y;
        left_report.x = -x;
        left_report.y = -y;
    }
    return pointing_device_task_combined_user(left_report, right_report);
}
#endif
