// Copyright 2025 Carlos Velasco
// SPDX-License-Identifier: GPL-2.0-or-later
//
// RAM-only instrumentation for the ChibiOS USB event path.
//
// Motivation: usb_event_cb() runs in ISR context and only enqueues into a
// 16-entry ring (effective capacity 15). All four enqueue call sites in
// tmk_core/protocol/chibios/usb_main.c discard the return value, so a resume
// storm that outruns the cooperative main loop drops events silently and lets
// the software USB state diverge from the physical one.
//
// Nothing here writes EEPROM or flash. Recording is a handful of RAM stores
// under chSysGetStatusAndLockX(), which is valid from both ISR and thread
// context, so it is safe from the USB callback.

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    XTREEMZE_USB_TRACE_SUSPEND_ENTER = 0,
    XTREEMZE_USB_TRACE_USB_EVENT_SUSPEND,
    XTREEMZE_USB_TRACE_USB_EVENT_WAKEUP,
    XTREEMZE_USB_TRACE_WAKE_HANDLER_RUN,
    XTREEMZE_USB_TRACE_USB_RESET,
    XTREEMZE_USB_TRACE_USB_UNCONFIGURED,
    XTREEMZE_USB_TRACE_USB_CONFIGURED,
    XTREEMZE_USB_TRACE_QUEUE_OVERFLOW,
    XTREEMZE_USB_TRACE_FIRST_HOUSEKEEPING_AFTER_WAKE,
    XTREEMZE_USB_TRACE_SPLIT_TRANSPORT_ALIVE,
    XTREEMZE_USB_TRACE_KIND_COUNT,
} xtreemze_usb_trace_kind_t;

#define XTREEMZE_USB_TRACE_CAPACITY 64

typedef struct {
    uint16_t seq;   // Monotonic across the whole session; ordering is the point.
    uint32_t time;  // timer_read32() at record time, secondary to seq.
    uint8_t  kind;  // xtreemze_usb_trace_kind_t
} xtreemze_usb_trace_entry_t;

// Safe from ISR or thread context.
void xtreemze_usb_trace_record(uint8_t kind);

// Number of entries currently held (saturates at XTREEMZE_USB_TRACE_CAPACITY).
uint8_t  xtreemze_usb_trace_count(void);
// Total events ever recorded, including ones the ring has since overwritten.
uint16_t xtreemze_usb_trace_total(void);
// index 0 is the oldest retained entry. Returns false if index is out of range.
bool     xtreemze_usb_trace_read(uint8_t index, xtreemze_usb_trace_entry_t *entry);

// Types the counters and the retained ring out as keystrokes. Deliberately not
// dependent on the TFT, which is inert in the no-wake-recovery diagnostic build.
void xtreemze_usb_trace_type_report(void);
