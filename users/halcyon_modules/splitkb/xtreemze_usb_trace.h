// Copyright 2025 Carlos Velasco
// SPDX-License-Identifier: GPL-2.0-or-later
//
// RAM-only instrumentation for the ChibiOS USB event path.
//
// Motivation: usb_event_cb() runs in ISR context and only enqueues into a
// 16-entry ring (effective capacity 15, since it reports full at head+1==tail).
// All four enqueue call sites in tmk_core/protocol/chibios/usb_main.c discard
// the return value, so a resume storm that outruns the cooperative main loop
// drops events silently and lets the software USB state diverge from the
// physical one.
//
// Enqueue and apply are recorded as SEPARATE events. "The ISR got this into the
// queue" and "the protocol task dequeued it and updated USB device state" are
// different claims, and the gap between them is the whole hypothesis. A missing
// _APPLIED for a present _ENQUEUED is the signature being hunted.
//
// Nothing here writes EEPROM or flash. Recording is a handful of RAM stores
// under chSysGetStatusAndLockX(), which is valid from both ISR and thread
// context, so it is safe from the USB callback.

#pragma once

#include <stdbool.h>
#include <stdint.h>

// NOTE: these values are mirrored as raw numbers in the usb_main.c patch
// (patches/0002). xtreemze_usb_trace.c carries _Static_asserts pinning them, so
// a reordering here becomes a build error rather than silently mislabelled data.
typedef enum {
    XTREEMZE_USB_TRACE_SUSPEND_ENTER = 0,

    // ISR side: accepted into the queue.
    XTREEMZE_USB_TRACE_SUSPEND_ENQUEUED = 1,
    XTREEMZE_USB_TRACE_WAKEUP_ENQUEUED = 2,
    XTREEMZE_USB_TRACE_RESET_ENQUEUED = 3,
    XTREEMZE_USB_TRACE_UNCONFIGURED_ENQUEUED = 4,
    XTREEMZE_USB_TRACE_CONFIGURED_ENQUEUED = 5,

    // Dropped on the floor because the queue was full. detail = usbevent_t.
    XTREEMZE_USB_TRACE_QUEUE_OVERFLOW = 6,

    // Task side: dequeued and the USB device state actually updated.
    XTREEMZE_USB_TRACE_SUSPEND_APPLIED = 7,
    XTREEMZE_USB_TRACE_WAKEUP_APPLIED = 8,
    XTREEMZE_USB_TRACE_RESET_APPLIED = 9,
    XTREEMZE_USB_TRACE_UNCONFIGURED_APPLIED = 10,
    XTREEMZE_USB_TRACE_CONFIGURED_APPLIED = 11,

    // Userspace side.
    XTREEMZE_USB_TRACE_WAKE_HANDLER_RUN = 12,
    XTREEMZE_USB_TRACE_FIRST_HOUSEKEEPING_AFTER_WAKE = 13,
    XTREEMZE_USB_TRACE_SPLIT_TRANSPORT_ALIVE = 14,

    XTREEMZE_USB_TRACE_KIND_COUNT,
} xtreemze_usb_trace_kind_t;

#define XTREEMZE_USB_TRACE_CAPACITY 64

typedef struct {
    uint16_t seq;     // Monotonic across the session; ordering is the point.
    uint32_t time;    // timer_read32() at record time, secondary to seq.
    uint8_t  kind;    // xtreemze_usb_trace_kind_t
    uint8_t  detail;  // Kind-specific; 0 when unused.
} xtreemze_usb_trace_entry_t;

// Safe from ISR or thread context.
void xtreemze_usb_trace_record(uint8_t kind, uint8_t detail);

// Number of entries currently held (saturates at XTREEMZE_USB_TRACE_CAPACITY).
uint8_t  xtreemze_usb_trace_count(void);
// Total events ever recorded, including ones the ring has since overwritten.
uint16_t xtreemze_usb_trace_total(void);
// index 0 is the oldest retained entry. Returns false if index is out of range.
bool     xtreemze_usb_trace_read(uint8_t index, xtreemze_usb_trace_entry_t *entry);

// Types the counters and the retained ring out as keystrokes. Deliberately not
// dependent on the TFT, which is inert in the no-wake-recovery diagnostic build.
//
// Known limitation: this runs on the keyboard itself, and on the master half
// only (should_process_keypress() is master-gated), so a cooperative-loop stall
// or HID deadlock can leave the trace intact in RAM and still unreachable.
void xtreemze_usb_trace_type_report(void);
