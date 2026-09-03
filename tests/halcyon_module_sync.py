#!/usr/bin/env python3
"""Exercise Halcyon module identity synchronization and recovery policy."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MODULE = (ROOT / "users/halcyon_modules/splitkb/halcyon.c").read_text()


def function(text: str, signature: str) -> str:
    start = text.index(signature)
    depth = 0
    opened = False
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
            opened = True
        elif text[index] == "}":
            depth -= 1
            if opened and depth == 0:
                return text[start : index + 1]
    raise ValueError(f"unterminated function: {signature}")


housekeeping = function(MODULE, "void housekeeping_task_kb(void)")
interval_match = re.search(r"#define\s+HLC_MODULE_SYNC_INTERVAL_MS\s+(\d+)", MODULE)
assert interval_match, "module sync retry/refresh interval must be explicit"
interval_ms = int(interval_match.group(1))
assert interval_ms == 500, f"unexpected module sync interval: {interval_ms}"

harness = f"""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define HLC_BACKLIGHT_TIMEOUT 300000
#define HLC_MODULE_SYNC_INTERVAL_MS {interval_ms}
#define MODULE_SYNC 7

typedef uint8_t module_t;
enum {{ hlc_none = 0, hlc_encoder = 1, hlc_tft_display = 2 }};

static module_t module_master = hlc_none;
static module_t module = hlc_encoder;
static bool backlight_off = false;
static bool master = true;
static bool transport_connected = false;
static bool rpc_result = true;
static uint32_t now_ms = 0;
static unsigned rpc_calls = 0;
static unsigned wake_calls = 0;
static unsigned suspend_calls = 0;
static unsigned display_calls = 0;
static unsigned module_calls = 0;
static unsigned user_calls = 0;

static bool is_keyboard_master(void) {{ return master; }}
static bool is_transport_connected(void) {{ return transport_connected; }}
static uint32_t timer_read32(void) {{ return now_ms; }}
static uint32_t timer_elapsed32(uint32_t since) {{ return now_ms - since; }}
static uint32_t last_input_activity_elapsed(void) {{ return 0; }}
static bool transaction_rpc_send(int transaction_id, uint8_t size, const void *payload) {{
    assert(transaction_id == MODULE_SYNC);
    assert(size == sizeof(module));
    assert(payload == &module);
    rpc_calls++;
    return rpc_result;
}}
static void backlight_wakeup(void) {{ backlight_off = false; wake_calls++; }}
static void backlight_suspend(void) {{ backlight_off = true; suspend_calls++; }}
static bool display_module_housekeeping_task_kb(bool second_display) {{
    (void)second_display;
    display_calls++;
    return true;
}}
static bool module_housekeeping_task_kb(void) {{ module_calls++; return true; }}
static void housekeeping_task_user(void) {{ user_calls++; }}

{housekeeping}

int main(void) {{
    // Disconnected masters must not attempt a transaction.
    housekeeping_task_kb();
    assert(rpc_calls == 0);

    // A failed first send must not be certified as synchronized, and retries
    // are bounded rather than occurring on every scan.
    transport_connected = true;
    rpc_result = false;
    housekeeping_task_kb();
    assert(rpc_calls == 1 && wake_calls == 0);

    now_ms = HLC_MODULE_SYNC_INTERVAL_MS - 1;
    housekeeping_task_kb();
    assert(rpc_calls == 1);

    now_ms = HLC_MODULE_SYNC_INTERVAL_MS;
    housekeeping_task_kb();
    assert(rpc_calls == 2 && wake_calls == 0);

    // A later successful retry establishes synchronization and wakes once.
    now_ms += HLC_MODULE_SYNC_INTERVAL_MS;
    rpc_result = true;
    housekeeping_task_kb();
    assert(rpc_calls == 3 && wake_calls == 1);

    // Periodic refresh repairs slave-side resets without repeatedly waking the
    // backlight or requiring a visible transport disconnect on the master.
    now_ms += HLC_MODULE_SYNC_INTERVAL_MS - 1;
    housekeeping_task_kb();
    assert(rpc_calls == 3 && wake_calls == 1);

    now_ms += 1;
    housekeeping_task_kb();
    assert(rpc_calls == 4 && wake_calls == 1);

    // A real disconnect invalidates the exchange. Reconnect retries
    // immediately rather than waiting for the previous periodic deadline.
    transport_connected = false;
    now_ms += 10;
    housekeeping_task_kb();
    assert(rpc_calls == 4);

    transport_connected = true;
    now_ms += 1;
    housekeeping_task_kb();
    assert(rpc_calls == 5 && wake_calls == 2);

    // A half that stops being master drops its sender state. If it later
    // becomes master again, it performs a fresh exchange immediately.
    master = false;
    now_ms += 10;
    housekeeping_task_kb();
    assert(rpc_calls == 5);

    master = true;
    now_ms += 1;
    housekeeping_task_kb();
    assert(rpc_calls == 6 && wake_calls == 3);

    assert(suspend_calls == 0);
    assert(display_calls == module_calls && module_calls == user_calls);
    puts("Halcyon module sync: failure retry, periodic refresh, disconnect recovery and master-role recovery pass.");
}}
"""

with tempfile.TemporaryDirectory(prefix="halcyon-module-sync-") as tmp:
    c = Path(tmp) / "module_sync.c"
    exe = Path(tmp) / "module_sync"
    c.write_text(harness)
    subprocess.run(
        [
            "cc",
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            str(c),
            "-o",
            str(exe),
        ],
        check=True,
    )
    subprocess.run([str(exe)], check=True)
