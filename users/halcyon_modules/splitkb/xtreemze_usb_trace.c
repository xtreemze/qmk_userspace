// Copyright 2025 Carlos Velasco
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "atomic_util.h"
#include "xtreemze_usb_trace.h"

#include <stdio.h>

// Counters live in usb_main.c so the enqueue path can touch them directly from
// the USB ISR without a call through this module.
extern volatile uint16_t usb_event_queue_overflow_count;
extern volatile uint8_t  usb_event_queue_last_dropped_event;
extern volatile uint8_t  usb_event_queue_high_watermark;

static xtreemze_usb_trace_entry_t trace_ring[XTREEMZE_USB_TRACE_CAPACITY];
static volatile uint8_t           trace_head  = 0;  // Next slot to write.
static volatile uint8_t           trace_held  = 0;  // Entries retained, <= capacity.
static volatile uint16_t          trace_total = 0;  // Ever recorded, incl. overwritten.

// Pin the numeric values mirrored in the usb_main.c patch (patches/0002).
// Reordering the enum without updating the patch becomes a build error.
_Static_assert(XTREEMZE_USB_TRACE_SUSPEND_ENQUEUED == 1, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_WAKEUP_ENQUEUED == 2, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_RESET_ENQUEUED == 3, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_UNCONFIGURED_ENQUEUED == 4, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_CONFIGURED_ENQUEUED == 5, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_QUEUE_OVERFLOW == 6, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_SUSPEND_APPLIED == 7, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_WAKEUP_APPLIED == 8, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_RESET_APPLIED == 9, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_UNCONFIGURED_APPLIED == 10, "usb_main.c patch mirrors this value");
_Static_assert(XTREEMZE_USB_TRACE_CONFIGURED_APPLIED == 11, "usb_main.c patch mirrors this value");

void xtreemze_usb_trace_record(uint8_t kind, uint8_t detail) {
    if (kind >= XTREEMZE_USB_TRACE_KIND_COUNT) {
        return;
    }

    // Taken outside our own lock, but note this is NOT free: timer_read32()
    // itself takes chSysGetStatusAndLockX() and updates shared tick/ms offset
    // bookkeeping. It is valid from any context, which is what matters here, but
    // each record costs that lock plus the ATOMIC_BLOCK below.
    const uint32_t now = timer_read32();

    // RESTORESTATE, not FORCEON: this is reached from usb_event_cb() in ISR
    // context as well as from the protocol task, and chSysGetStatusAndLockX()
    // is the variant valid in both.
    ATOMIC_BLOCK_RESTORESTATE {
        trace_ring[trace_head].seq    = trace_total;
        trace_ring[trace_head].time   = now;
        trace_ring[trace_head].kind   = kind;
        trace_ring[trace_head].detail = detail;

        trace_head = (uint8_t)((trace_head + 1) % XTREEMZE_USB_TRACE_CAPACITY);
        if (trace_held < XTREEMZE_USB_TRACE_CAPACITY) {
            trace_held++;
        }
        trace_total++;
    }
}

static volatile bool split_probe_pending = false;

void xtreemze_usb_trace_arm_split_probe(void) {
    split_probe_pending = true;
}

void xtreemze_usb_trace_split_success(void) {
    if (!split_probe_pending) {
        return;
    }

    split_probe_pending = false;
    xtreemze_usb_trace_record(XTREEMZE_USB_TRACE_SPLIT_TRANSPORT_ALIVE, 0);
}

uint8_t xtreemze_usb_trace_count(void) {
    uint8_t held;
    ATOMIC_BLOCK_RESTORESTATE {
        held = trace_held;
    }
    return held;
}

uint16_t xtreemze_usb_trace_total(void) {
    uint16_t total;
    ATOMIC_BLOCK_RESTORESTATE {
        total = trace_total;
    }
    return total;
}

bool xtreemze_usb_trace_read(uint8_t index, xtreemze_usb_trace_entry_t *entry) {
    bool ok = false;

    ATOMIC_BLOCK_RESTORESTATE {
        if (index < trace_held) {
            // trace_head is one past the newest, so the oldest retained entry
            // sits at head - held (modulo capacity).
            uint8_t slot = (uint8_t)((trace_head + XTREEMZE_USB_TRACE_CAPACITY - trace_held + index) % XTREEMZE_USB_TRACE_CAPACITY);
            *entry       = trace_ring[slot];
            ok           = true;
        }
    }

    return ok;
}

static const char *const trace_kind_labels[XTREEMZE_USB_TRACE_KIND_COUNT] = {
    "SUSPEND_ENTER",
    "SUSPEND_ENQUEUED",
    "WAKEUP_ENQUEUED",
    "RESET_ENQUEUED",
    "UNCONFIGURED_ENQUEUED",
    "CONFIGURED_ENQUEUED",
    "QUEUE_OVERFLOW",
    "SUSPEND_APPLIED",
    "WAKEUP_APPLIED",
    "RESET_APPLIED",
    "UNCONFIGURED_APPLIED",
    "CONFIGURED_APPLIED",
    "WAKE_HANDLER_RUN",
    "FIRST_HOUSEKEEPING_AFTER_WAKE",
    "SPLIT_TRANSPORT_ALIVE",
};

void xtreemze_usb_trace_type_report(void) {
    char line[80];

    send_string("--- usb trace ---\n");

    // The ring is declared with 16 slots but reports full when head+1 == tail,
    // so the real usable depth is 15.
    snprintf(line, sizeof(line), "overflow=%u last_dropped=%u high_watermark=%u/15\n",
             (unsigned)usb_event_queue_overflow_count,
             (unsigned)usb_event_queue_last_dropped_event,
             (unsigned)usb_event_queue_high_watermark);
    send_string(line);

    const uint8_t  held  = xtreemze_usb_trace_count();
    const uint16_t total = xtreemze_usb_trace_total();

    snprintf(line, sizeof(line), "events=%u retained=%u\n", (unsigned)total, (unsigned)held);
    send_string(line);

    for (uint8_t i = 0; i < held; ++i) {
        xtreemze_usb_trace_entry_t entry;

        if (!xtreemze_usb_trace_read(i, &entry)) {
            continue;
        }

        const char *label = entry.kind < XTREEMZE_USB_TRACE_KIND_COUNT ? trace_kind_labels[entry.kind] : "UNKNOWN";

        // Presence is decided by kind, never by value: QUEUE_OVERFLOW.detail is
        // a raw usbevent_t and CONFIGURED_APPLIED.detail is a configuration
        // number, both of which can legitimately be zero.
        const bool show_detail = entry.kind == XTREEMZE_USB_TRACE_QUEUE_OVERFLOW || entry.kind == XTREEMZE_USB_TRACE_CONFIGURED_APPLIED;

        if (show_detail) {
            snprintf(line, sizeof(line), "%u %lu %s %u\n", (unsigned)entry.seq, (unsigned long)entry.time, label, (unsigned)entry.detail);
        } else {
            snprintf(line, sizeof(line), "%u %lu %s\n", (unsigned)entry.seq, (unsigned long)entry.time, label);
        }
        send_string(line);
    }

    send_string("--- end ---\n");
}
