# Issue #7: Hardware Acceptance Matrix
## Complete Physical Validation for Halcyon Ferris

**Status**: Testing Framework | **Date**: 2026-09-09

---

## Overview

This document defines the complete physical acceptance testing matrix for the Halcyon Ferris keyboard. All tests must pass on both master orientations before any firmware release is considered stable.

---

## Test Environment Setup

### Required Equipment
- Halcyon Ferris keyboard (both display and encoder modules)
- USB cable and power
- Computer running macOS/Linux/Windows
- Vial configurator software
- QMK CLI tools
- Terminal access

### Pre-Test Checklist
- [ ] Firmware built successfully
- [ ] Both module variants available (display + encoder)
- [ ] Backup existing EEPROM (optional but recommended)
- [ ] Clear workspace, good lighting
- [ ] All connections secure and clean

---

## Test Matrix: Critical Behaviors

### A. Cold Boot & Basic Operation (P0 - Critical)

#### A.1: USB Enumeration
**Objective**: Verify keyboard initializes on power-up

```
Test Steps:
1. Connect keyboard to computer via USB
2. Observe LED behavior (should indicate boot)
3. Verify keyboard appears in QMK tools
4. Check system detects HID device

Pass Criteria:
✓ Appears within 2 seconds
✓ QMK recognizes it
✓ No enumeration errors in system logs
✓ LED boot sequence completes
```

#### A.2: Basic Typing
**Objective**: Verify core input path works

```
Test Steps:
1. Open text editor
2. Type on all layers (L0-L3 minimum)
3. Verify characters appear correctly
4. Test modifier combinations (Shift, Ctrl, Cmd)

Pass Criteria:
✓ All keys register
✓ Modifiers work correctly
✓ No key ghosting or missed inputs
✓ No duplicate characters
```

#### A.3: Master Orientation (Both Sides)
**Objective**: Verify both master/secondary configurations work

```
Test Steps:
A. Left as master:
   1. Connect left half first
   2. Connect right half
   3. Test typing on all keys
   4. Test encoder/buttons on right

B. Right as master:
   1. Connect right half first
   2. Connect left half
   3. Test typing on all keys
   4. Test encoder/buttons on left

Pass Criteria:
✓ Both orientations function fully
✓ Split works regardless of connection order
✓ No key loss or phantom inputs
✓ Both module variants work in both orientations
```

---

### B. Display Module Tests (P0 - Critical)

#### B.1: Display Initialization
**Objective**: TFT display powers up and shows content

```
Test Steps:
1. Boot with display module connected
2. Observe TFT initialization
3. Verify graphics render clearly
4. Check no visual glitches or corruption

Pass Criteria:
✓ Display powers on within 1 second
✓ No flickering or artifacts
✓ Text is readable
✓ Color rendering correct
✓ Refresh smooth (no tearing)
```

#### B.2: Display Animations
**Objective**: Verify animated effects work smoothly

```
Test Steps:
1. Trigger layer change → observe layer indicator animation
2. Trigger RGB change → observe color animation
3. Scroll through menu → observe transition smoothness
4. Run for 5+ minutes → check for lockup or degradation

Pass Criteria:
✓ Animations smooth at 30fps
✓ No freezing during transitions
✓ RAM doesn't leak over time
✓ No display corruption under load
```

#### B.3: Backlight Control
**Objective**: Brightness adjustment works correctly

```
Test Steps:
1. Press backlight brightness up key (multiple times)
2. Verify brightness increases in 5-10 steps
3. Press backlight brightness down key
4. Verify brightness decreases smoothly
5. Test backlight off
6. Reboot → verify brightness persists

Pass Criteria:
✓ Smooth brightness adjustment
✓ Level 1 clearly visible (not off)
✓ Level 10 clearly brighter than level 1
✓ Setting persists after reboot
✓ No flickering during adjustment
```

---

### C. Encoder Module Tests (P0 - Critical)

#### C.1: Encoder Rotation
**Objective**: Encoder reads rotation in both directions

```
Test Steps:
1. Slow rotation clockwise → count presses (should be 1 per detent)
2. Slow rotation counterclockwise → count presses
3. Fast rotation both directions
4. Rapid alternating rotation

Pass Criteria:
✓ Clockwise increments (volume up, scroll up, etc.)
✓ Counterclockwise decrements
✓ No missed or duplicate presses
✓ Smooth tracking at all speeds
✓ Each detent = exactly 1 keystroke
```

#### C.2: Encoder Button Press
**Objective**: Encoder button click functions correctly

```
Test Steps:
1. Press encoder button (should trigger layer/action)
2. Hold encoder button (should lock/toggle)
3. Double-tap encoder button
4. Long press encoder button

Pass Criteria:
✓ Single press registers as configured
✓ No bouncing (multiple quick presses)
✓ Long press distinguishable from short
✓ Click is distinct from rotation
```

#### C.3: Encoder Acceleration
**Objective**: Fast rotation provides acceleration if configured

```
Test Steps:
1. Rotate slowly → should increment by 1
2. Rotate fast → should increment by 3-5
3. Verify acceleration is smooth
4. Acceleration should match configuration

Pass Criteria:
✓ Acceleration curves smooth
✓ No acceleration on slow rotation
✓ Noticeable acceleration on fast
✓ Configurable via Vial
```

---

### D. Split Synchronization (P1 - High Priority)

#### D.1: Split Disconnect/Reconnect
**Objective**: Keyboard handles split disconnection gracefully

```
Test Steps:
1. Boot keyboard fully (both halves connected)
2. Type on both halves
3. Disconnect one half (unplug from other)
4. Attempt to type (should work on master only)
5. Reconnect half
6. Verify synchronization resumes
7. Test typing both halves again

Pass Criteria:
✓ Master half continues working without slave
✓ No crash or lockup when slave disconnects
✓ No key repeat or ghosting while disconnected
✓ Reconnection is smooth and quick
✓ Full functionality resumes after reconnect
```

#### D.2: One Half Reset While Other Powered
**Objective**: One module reset doesn't kill the other

```
Test Steps:
1. Boot keyboard fully
2. Press RESET on one half only
3. Observe other half continues working
4. Reconnected module initializes properly

Pass Criteria:
✓ Powered half continues working
✓ Reset half reappears in ~2 seconds
✓ No partial keys lost
✓ Split communication re-establishes
```

#### D.3: Full Power Cycle
**Objective**: Full disconnect and reconnect works

```
Test Steps:
1. Unplug both halves
2. Wait 2 seconds
3. Plug in one half
4. Plug in other half
5. Wait for full boot
6. Test typing

Pass Criteria:
✓ Both halves enumerate
✓ Split initializes correctly
✓ Full functionality works
✓ No stale state from prior session
```

---

### E. Suspend/Resume (P1 - High Priority)

#### E.1: Sleep/Wake Transitions
**Objective**: Keyboard handles system suspend gracefully

```
Test Steps:
1. Boot keyboard fully
2. Put system to sleep (Cmd+Option+Eject on Mac)
3. Observe keyboard LEDs (should dim or turn off)
4. Press any key → system should wake
5. Keyboard should work immediately after wake

Pass Criteria:
✓ Keyboard enters low-power state
✓ Key press wakes system
✓ No lag after wake (typing works immediately)
✓ No key repeat on wake
✓ Split reconnects after system wake if needed
```

#### E.2: Wake Recovery
**Objective**: EEPROM state preserved after suspend

```
Test Steps:
1. Set custom brightness level
2. Activate specific layer
3. System sleep 30 seconds
4. System wake
5. Check brightness (should match pre-sleep)
6. Check layer (should match pre-sleep)

Pass Criteria:
✓ All EEPROM settings preserved
✓ Layer state preserved
✓ No configuration reset
✓ Full function restored within 1 second
```

#### E.3: USB Reconnect During Sleep
**Objective**: USB reconnection during sleep handled correctly

```
Test Steps:
1. Boot keyboard
2. System sleep
3. Disconnect USB cable
4. Wait 5 seconds
5. Reconnect USB cable
6. System wake
7. Test typing

Pass Criteria:
✓ Keyboard re-enumerates cleanly
✓ No stale state issues
✓ Full functionality after reconnect
✓ Split re-synchronizes if needed
```

---

### F. EEPROM Persistence (P1 - High Priority, Issue #47 Validation)

#### F.1: Keymap Preservation Across Reflash
**Objective**: User keymaps survive firmware update

```
Test Steps:
1. Boot firmware v1
2. Set custom keymap in Vial
3. Save complex macro (20+ characters)
4. Reboot → verify still there
5. Build firmware v2 (same source)
6. Flash firmware v2 while powered
7. System should preserve EEPROM
8. Verify keymap still present
9. Verify macro still present and works

Pass Criteria:
✓ Dynamic keymap survives reflash
✓ Macros preserved exactly
✓ No EEPROM reset on reflash
✓ Vial still recognizes keymap
✓ Macro works identically
```

#### F.2: Macro Preservation
**Objective**: Complex macros survive reflash

```
Test Steps:
1. Create macros in all slots (M0-M15)
2. Test all macros work
3. Create varying macro types:
   - Simple text: "hello"
   - Text with delays: "." [delay] "space"
   - Key presses: "CTRL(C)" "CTRL(V)"
   - Mixed: text + keys
4. Reflash firmware
5. Test all macros still work
6. Test macro execution exactly matches

Pass Criteria:
✓ All macros preserved
✓ Text macros execute identically
✓ Delays preserved
✓ Key sequences work
✓ Mixed macros execute in correct order
```

#### F.3: M10 Bootstrap Macro
**Objective**: M10 macro hardening works (#26 validation)

```
Test Steps:
1. Flash firmware with default config
2. Open Vial → check M10
3. M10 should contain: curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh | bash
4. Press M10 in terminal → verify command appears (not executed)
5. Review command → should point to 'main' branch, not 'master'

Pass Criteria:
✓ M10 macro present
✓ Uses 'main' branch (not 'master')
✓ No Enter key (requires manual submission)
✓ URL accessible (returns 200 OK)
✓ No execution without manual press
```

---

### G. Multi-Module Consistency (P1)

#### G.1: Display vs Encoder Module Identical Config
**Objective**: Both modules behave identically

```
Test Steps:
1. Flash display module → test L0-L12 layers
2. Test all keycodes work
3. Record layer transition smoothness
4. Reflash encoder module
5. Test identical layers
6. Test identical keycodes
7. Compare feel/response

Pass Criteria:
✓ Both modules have identical keymap
✓ Layer transitions feel the same
✓ Keystroke latency identical
✓ Macro execution identical
✓ EEPROM behavior identical
```

#### G.2: Firmware Determinism Validation (#47)
**Objective**: Same source produces identical binaries

```
Test Steps:
1. Build display module
2. SHA256 firmware_display.uf2 → save hash1
3. Clean build
4. Build display module again
5. SHA256 firmware_display.uf2 → save hash2
6. Compare hash1 vs hash2

Pass Criteria:
✓ hash1 == hash2 (byte-for-byte identical)
✓ Proves deterministic build
✓ Same applies to encoder module
✓ Proves release reproducibility
```

---

### H. RGB Matrix (if enabled)

#### H.1: RGB Lighting Control
**Objective**: LED control works smoothly

```
Test Steps:
1. Cycle RGB modes (RGB_MOD)
2. Adjust brightness (RGB_VAI / RGB_VAD)
3. Adjust saturation (RGB_SAI / RGB_SAD)
4. Adjust hue (RGB_HUI / RGB_HUD)
5. Test various lighting effects
6. Verify smooth transitions

Pass Criteria:
✓ All RGB modes show no flicker
✓ Brightness changes smoothly
✓ No LED glitches or stuck pixels
✓ Settings persist after reboot
✓ No lag during typing (RGB doesn't block input)
```

---

## Safety Validation Checklist

### Before Every Test Session
- [ ] Keyboard fully charged/powered
- [ ] All connections secure
- [ ] Backup latest EEPROM state
- [ ] Clear any prior firmware crashes
- [ ] Test environment at comfortable temperature

### During Testing
- [ ] Monitor for any warnings or errors
- [ ] Note any unusual behavior immediately
- [ ] Stop testing if system crashes
- [ ] Do not force-reboot mid-test

### After Each Test Category
- [ ] Document results (pass/fail)
- [ ] Note any observations
- [ ] Save serial output if issues found
- [ ] Check for data corruption

---

## Test Results Template

```markdown
## Hardware Acceptance Test Results

**Firmware Version**: [commit SHA]
**Date**: [YYYY-MM-DD]
**Tester**: [Name]

### A. Cold Boot & Basic Operation
- [ ] A.1 USB Enumeration: PASS / FAIL / PARTIAL
- [ ] A.2 Basic Typing: PASS / FAIL / PARTIAL
- [ ] A.3 Master Orientation: PASS / FAIL / PARTIAL

### B. Display Module
- [ ] B.1 Display Initialization: PASS / FAIL / PARTIAL
- [ ] B.2 Display Animations: PASS / FAIL / PARTIAL
- [ ] B.3 Backlight Control: PASS / FAIL / PARTIAL

### C. Encoder Module
- [ ] C.1 Encoder Rotation: PASS / FAIL / PARTIAL
- [ ] C.2 Encoder Button: PASS / FAIL / PARTIAL
- [ ] C.3 Encoder Acceleration: PASS / FAIL / PARTIAL

### D. Split Synchronization
- [ ] D.1 Disconnect/Reconnect: PASS / FAIL / PARTIAL
- [ ] D.2 One Half Reset: PASS / FAIL / PARTIAL
- [ ] D.3 Full Power Cycle: PASS / FAIL / PARTIAL

### E. Suspend/Resume
- [ ] E.1 Sleep/Wake: PASS / FAIL / PARTIAL
- [ ] E.2 Wake Recovery: PASS / FAIL / PARTIAL
- [ ] E.3 USB Reconnect: PASS / FAIL / PARTIAL

### F. EEPROM Persistence (#47 Validation)
- [ ] F.1 Keymap Preservation: PASS / FAIL / PARTIAL
- [ ] F.2 Macro Preservation: PASS / FAIL / PARTIAL
- [ ] F.3 M10 Macro (#26): PASS / FAIL / PARTIAL

### G. Multi-Module Consistency
- [ ] G.1 Module Identical Config: PASS / FAIL / PARTIAL
- [ ] G.2 Firmware Determinism: PASS / FAIL / PARTIAL

### Overall Result
**ACCEPTED** / **CONDITIONAL** (issues noted) / **REJECTED** (blocking issues)

**Issues Found**:
[List any failures or concerns]

**Sign-off**:
Tester: _________________ Date: __________
```

---

## Release Gate Criteria

Firmware is **ACCEPTED FOR RELEASE** when:
- ✅ All P0 (Critical) tests PASS
- ✅ All P1 (High Priority) tests PASS or have documented workarounds
- ✅ Both module variants tested
- ✅ Both master orientations tested
- ✅ #47 EEPROM persistence validated
- ✅ #26 M10 macro verified
- ✅ Firmware determinism confirmed (same binary)
- ✅ No blocking security issues
- ✅ Signed off by tester

---

## Known Limitations & Workarounds

### Display Recovery (Post-Wake)
**Status**: Known issue, non-blocking

If display doesn't wake after system sleep:
- Press any key to wake system
- LED should flash and display should initialize
- If not: reboot keyboard (disconnect/reconnect)

### Split Reconnect Lag
**Status**: Normal, expected behavior

First keypress after split reconnect may have 50-100ms lag as synchronization completes. This is expected and not a failure.

### EEPROM Backup Recommendation
Always backup EEPROM before experimental firmware:
```bash
qmk kb-firmware-exfiltrate splitkb/halcyon/ferris
```

---

## Sign-Off & Approval

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Hardware Tester | | | |
| Firmware Lead | | | |
| Release Manager | | | |

---

## See Also
- [Issue #47: EEPROM Persistence](./ISSUE_26_IMPLEMENTATION_COMPLETE.md)
- [Issue #26: Configuration Hardening](./ISSUE_26_IMPLEMENTATION_COMPLETE.md)
- [Build Process](./DEVELOPMENT.md)
- [Troubleshooting Guide](./TROUBLESHOOTING.md)
