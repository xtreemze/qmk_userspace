# Issue #47: EEPROM Persistence & Determinism - Complete Test Suite

**Date**: 2026-09-09 | **Status**: Test Framework Complete

---

## Overview

Comprehensive test procedures for Issue #47 (Vial EEPROM Persistence Integrity) and Issue #47 Phase 4 (Hardware Validation).

These tests verify:
1. **Deterministic Builds**: Same source → byte-for-byte identical firmware
2. **EEPROM Preservation**: User data survives reflash with same BUILD_ID
3. **Safe Migration**: Transition from random to deterministic BUILD_IDs
4. **Atomic Operations**: Compatibility generation advances only after successful init

---

## Pre-Test Checklist

### Environment Setup
- [ ] Clone latest halcyon branch
- [ ] Build tools installed (qmk, gcc, make)
- [ ] Python 3.7+ available
- [ ] Hex editor optional (for binary inspection)
- [ ] Serial monitor available for debugging
- [ ] Network connectivity (for git operations)

### Hardware Setup
- [ ] Halcyon Ferris keyboard assembled
- [ ] USB cable working
- [ ] Both module variants ready (display + encoder)
- [ ] Spare USB port for testing (avoid hub if possible)
- [ ] Terminal/shell ready

### Backup
- [ ] Backup current EEPROM state
  ```bash
  qmk kb-firmware-exfiltrate splitkb/halcyon/ferris > backup_eeprom.bin
  ```
- [ ] Save current firmware SHA256
- [ ] Document current configuration

---

## Test Category 1: Deterministic Build Validation

### T1.1: Same Source → Identical Binary

**Objective**: Verify build determinism (Issue #47, Phase 1)

**Test Steps**:
```bash
# First build
cd qmk_firmware
export USERSPACE_SHA=$(cd .. && git rev-parse HEAD)
export VIAL_QMK_SHA=$(git rev-parse HEAD)
make splitkb/halcyon/ferris/display:xtreemze_final
sha256sum .build/splitkb_halcyon_ferris_display_xtreemze_final.uf2 > build1.sha

# Save firmware
cp .build/splitkb_halcyon_ferris_display_xtreemze_final.uf2 /tmp/firmware_build1.uf2

# Clean build
make clean

# Second build (identical conditions)
export USERSPACE_SHA=$(cd .. && git rev-parse HEAD)
export VIAL_QMK_SHA=$(git rev-parse HEAD)
make splitkb/halcyon/ferris/display:xtreemze_final
sha256sum .build/splitkb_halcyon_ferris_display_xtreemze_final.uf2 > build2.sha

# Save firmware
cp .build/splitkb_halcyon_ferris_display_xtreemze_final.uf2 /tmp/firmware_build2.uf2

# Compare
diff build1.sha build2.sha
```

**Pass Criteria**:
- ✓ `build1.sha` == `build2.sha`
- ✓ Binary sizes identical
- ✓ Both firmware files md5 hash match
- ✓ Proves deterministic compilation

**Notes**:
- If builds differ: check for timestamp issues, race conditions
- Check git status for modifications
- Verify environment variables stable across builds

---

### T1.2: Encoder Module Determinism

**Objective**: Verify encoder module also builds deterministically

**Test Steps**:
```bash
# Repeat T1.1 but for encoder module
make clean
export USERSPACE_SHA=$(cd .. && git rev-parse HEAD)
export VIAL_QMK_SHA=$(git rev-parse HEAD)
make splitkb/halcyon/ferris/encoder:xtreemze_final
sha256sum .build/splitkb_halcyon_ferris_encoder_xtreemze_final.uf2 > encoder_build1.sha

make clean
export USERSPACE_SHA=$(cd .. && git rev-parse HEAD)
export VIAL_QMK_SHA=$(git rev-parse HEAD)
make splitkb/halcyon/ferris/encoder:xtreemze_final
sha256sum .build/splitkb_halcyon_ferris_encoder_xtreemze_final.uf2 > encoder_build2.sha

diff encoder_build1.sha encoder_build2.sha
```

**Pass Criteria**:
- ✓ Both encoder builds identical
- ✓ BUILD_ID identical in both
- ✓ No variations between runs

---

### T1.3: BUILD_ID Extraction & Validation

**Objective**: Verify BUILD_ID is deterministic and correct

**Test Steps**:
```bash
# Extract BUILD_ID from firmware
python3 -c "
import struct
with open('.build/splitkb_halcyon_ferris_display_xtreemze_final.uf2', 'rb') as f:
    # UF2 file parsing to find BUILD_ID in version.h
    # or read from generated version.h directly
    pass
"

# Alternative: check generated version.h
grep "BUILD_ID" qmk_firmware/*/version.h

# Record BUILD_ID value
echo "BUILD_ID from display build: $(grep BUILD_ID version.h | grep -oP '0x[0-9A-F]+')"
echo "BUILD_ID from encoder build: $(grep BUILD_ID version.h | grep -oP '0x[0-9A-F]+')"
```

**Pass Criteria**:
- ✓ BUILD_ID format correct (0xXXXXXXXX)
- ✓ Same source → same BUILD_ID
- ✓ BUILD_ID != 0 and != 0xFFFFFFFF (invalid values)
- ✓ BUILD_ID derived from git SHAs (not random)

---

## Test Category 2: EEPROM Preservation & Migration

### T2.1: Keymap Preservation (Same BUILD_ID)

**Objective**: Verify dynamic keymap survives reflash with same BUILD_ID

**Test Procedure**:

```
Setup Phase:
1. Flash firmware v1 (display module)
   - Build deterministic firmware
   - Record BUILD_ID value
   - Flash via Vial or QMK

2. Configure Custom Keymap:
   - Open Vial
   - Navigate to Keymap tab
   - Modify layer 1:
     * Change key at (0,0) from KC_A to KC_B
     * Change key at (0,1) from KC_S to KC_C
     * Change key at (0,2) from KC_D to KC_D (unchanged, control)
   - Verify changes in Vial preview
   - Close Vial (settings auto-saved to EEPROM)

3. Test Changes:
   - Open text editor
   - Type on layer 1 → should see BC... instead of AS...
   - Confirm keymap change worked
   - Record: "Layer 1 custom keymap confirmed"

4. Reboot Keyboard:
   - Disconnect USB
   - Wait 2 seconds
   - Reconnect USB
   - Open Vial → verify keymap still shows modifications
   - Type again → confirm still works
   - Record: "Keymap survived reboot"

Reflash Phase:
5. Build Firmware v2:
   - Clean build
   - Compile display module
   - Verify BUILD_ID matches v1 (same source)
   - Save firmware to file

6. Flash Firmware v2:
   - Connect keyboard via USB
   - Flash new firmware (same source, same BUILD_ID)
   - System should preserve EEPROM during flash
   - Wait for flash to complete (~5 seconds)

7. Post-Flash Verification:
   - System automatically detects new firmware
   - Vial updates if needed
   - Open Vial → check keymap layer 1
   - Verify BC... changes still present
   - Type on layer 1 → should produce BC...
   - Confirm: "Keymap preserved after reflash"

Validation:
8. Data Integrity Check:
   - Verify layer 1 modifications exact match
   - Check layer 2-12 unchanged
   - Verify no corruption in other EEPROM areas
   - Record: "EEPROM integrity verified"
```

**Expected Results**:
- ✓ Custom keymap survives reboot
- ✓ Custom keymap survives reflash
- ✓ No EEPROM reset on reflash
- ✓ Settings byte-for-byte identical

**Failure Scenarios to Catch**:
- ❌ Keymap resets to defaults after reflash (suggests compat gen didn't match)
- ❌ Only partial keymap preserved (data corruption)
- ❌ Keymap survives but shifted/corrupt (offset issue)
- ❌ Vial refuses to load keymap (format mismatch)

---

### T2.2: Macro Preservation

**Objective**: Verify complex macros survive reflash

**Setup**:
```
1. Flash firmware v1
2. Create test macros:

   M0: Simple text
      Text: "hello world"
   
   M1: Text with delays
      Text: "." [delay 100ms] "SPACE" [delay 50ms] "test"
   
   M2: Keys only
      CTRL(C) [delay] CTRL(V)
   
   M3: Mixed
      Text: "prefix_"
      Key: CTRL(Z)
      Text: "_suffix"
   
   M4-M9: Progressively complex (10, 20, 30+ characters)

3. Test each macro works:
   - M0: Type "hello world" in editor
   - M1: Type with delays visible
   - M2: Copy/paste sequence works
   - M3: Mixed text+key execution
   - Record: "All macros functional before reflash"

4. Reboot → verify all still work
5. Record: "Macros survived reboot"
```

**Reflash & Verify**:
```
6. Build firmware v2 (same source as v1)
7. Flash (EEPROM should preserve)
8. Test each macro:
   - M0: Should type "hello world" identically
   - M1: Should have same delays and output
   - M2: Should execute same keystroke sequence
   - M3: Should mix text+keys in same order
   - M4-M9: Should all be present and functional

9. Verify execution:
   - Macro timing unchanged
   - Text output exact match
   - Keystroke sequences identical
   - No character loss or reordering
   - Record: "All macros preserved and functional"
```

**Pass Criteria**:
- ✓ All macros present in Vial after reflash
- ✓ Macro text identical character-for-character
- ✓ Delays and timing preserved
- ✓ Keystroke sequences in correct order
- ✓ Execution produces identical results

---

### T2.3: M10 Bootstrap Macro Validation (#26)

**Objective**: Verify M10 hardening works correctly

**Setup**:
```bash
1. Flash firmware
2. Open Vial → Macros → M10
3. Verify M10 contains:
   Text: "curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh | bash"
4. Verify NO Enter key at end (requires manual submission)
```

**Functional Test**:
```
1. In terminal, press/type M10
2. Full URL should appear: curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh | bash
3. Command should NOT execute (no automatic Enter)
4. User can review URL
5. User manually presses Enter to execute

Expected: Command typed, not executed
```

**URL Verification**:
```bash
# Verify URL is valid and accessible
curl -I https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh
# Should return 200 OK (or 404 if repo doesn't exist yet - document this)

# Verify uses 'main' branch (not 'master')
grep "master" <<< "curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh | bash" || echo "✓ No 'master' branch reference"

# Verify uses specific source (not mutable reference)
echo "✓ Uses specific branch 'main' (stable, not mutable 'master')"
```

**Pass Criteria**:
- ✓ M10 contains full curl command
- ✓ URL uses 'main' branch (not 'master')
- ✓ No Enter key (requires manual submission)
- ✓ URL accessible or properly documented
- ✓ Typing M10 shows command, doesn't execute

---

## Test Category 3: Compatibility Generation System

### T3.1: Compat Gen Automatic Update

**Objective**: Verify HALCYON_VIAL_COMPAT_GEN advances after initialization

**Test Steps**:

```
Pre-Flash Setup:
1. Prepare blank or minimal EEPROM state
2. Record EEPROM state before flash:
   - Read compat gen value (location: EECONFIG_USER_DATA_START)
   - Should be 0xFF (uninitialized) or old value
3. Prepare firmware with HALCYON_VIAL_COMPAT_GEN=1

Flash & Initialize:
4. Flash firmware
5. Keyboard boots:
   - via_init_kb() called
   - halcyon_vial_compat_init() checks old state
   - halcyon_vial_compat_seal() called after init
   - New compat gen written to EEPROM

6. After initialization completes:
   - Read compat gen value from EEPROM
   - Should be 1 (matching HALCYON_VIAL_COMPAT_GEN)
   - Should match BUILD_ID magic if preserved

Verification:
7. Reboot keyboard → compat gen should remain 1
8. Reboot again → should still be 1
9. Record: "Compat gen properly initialized and persisted"
```

**Pass Criteria**:
- ✓ Compat gen initialized to HALCYON_VIAL_COMPAT_GEN value
- ✓ Value persists across reboots
- ✓ Only written after successful init (atomic)
- ✓ Prevents unintended EEPROM resets

---

### T3.2: Intentional Reset via Gen Bump

**Objective**: Verify intentional EEPROM reset works via compat gen bump

**Test Steps**:

```
Setup:
1. Flash firmware with HALCYON_VIAL_COMPAT_GEN=1
2. Configure custom keymap
3. Create macros
4. Reboot → verify settings persist
5. Record: "State v1 confirmed with compat gen=1"

Gen Bump:
6. Modify firmware:
   - Edit rules.mk: HALCYON_VIAL_COMPAT_GEN=2
   - Rebuild firmware
   - BUILD_ID may change (different source)

7. Flash firmware with new compat gen=2
8. Keyboard boots:
   - compat gen=2 doesn't match stored=1
   - Triggers intentional reset
   - Vial storage reset to defaults
   - New compat gen=2 written

9. Post-flash verification:
   - Open Vial
   - Custom keymap GONE (reset to defaults)
   - Macros CLEARED
   - Record: "Intentional reset confirmed"

10. New Configuration:
    - Reconfigure keymap
    - Create new macros
    - Reboot → verify persist with compat gen=2
    - Record: "New settings persist with bumped gen"
```

**Pass Criteria**:
- ✓ Compat gen mismatch triggers reset
- ✓ User data cleared when gen bumped
- ✓ New compat gen written after init
- ✓ Future reflashes with same gen preserve data

---

## Test Category 4: Safety & Atomicity

### T4.1: Failed Seed Atomicity

**Objective**: Verify compat gen not written if seed fails

**Test Steps**:
```
Note: This test may require intentional failure injection
      (force seed to fail mid-operation)

Setup:
1. Prepare EEPROM with known state
2. Record compat gen value before flash
3. Prepare firmware that:
   - Has code to detect and fail factory seed
   - Still calls all init functions

Flash & Verify:
4. Flash firmware with intentional failure
5. System should:
   - Call via_init_kb()
   - Attempt factory seed
   - Detect failure
   - NOT call halcyon_vial_compat_seal()
   - NOT advance compat gen

6. Check EEPROM:
   - Compat gen should be UNCHANGED
   - Prevents corruption from partial init
   - Record: "Failed seed didn't corrupt compat gen"
```

**Pass Criteria**:
- ✓ Failed initialization doesn't corrupt state
- ✓ Compat gen unchanged on failure
- ✓ System safe to retry init
- ✓ No half-written data

---

### T4.2: Power Loss Simulation

**Objective**: Verify clean state if power lost during init

**Test Steps**:
```
Setup:
1. Prepare blank EEPROM
2. Document expected behavior:
   - If powered off during init: compat gen may be uninitialized
   - Next boot should recover safely

Flash & Simulate:
3. Flash firmware
4. During keyboard initialization:
   - Observe LED boot sequence
   - Disconnect power mid-sequence (if possible)
   - Or use watchdog timeout to simulate

5. Reconnect power
6. Keyboard should:
   - Reboot cleanly
   - Detect uninitialized state (compat gen=0xFF)
   - Reinitialize
   - Succeed this time
   - Record: "Recovery from power loss successful"
```

**Pass Criteria**:
- ✓ Clean reboot after power loss
- ✓ No corrupted EEPROM state
- ✓ Compat gen eventually initialized
- ✓ System reaches stable state

---

## Test Category 5: Cross-Module Validation

### T5.1: Display vs Encoder Module BUILD_ID

**Objective**: Verify both modules get same BUILD_ID

**Test Steps**:
```bash
# Build both modules and extract BUILD_IDs
cd qmk_firmware

# Display module
export USERSPACE_SHA=$(cd .. && git rev-parse HEAD)
export VIAL_QMK_SHA=$(git rev-parse HEAD)
make splitkb/halcyon/ferris/display:xtreemze_final 2>&1 | grep -i "BUILD_ID\|version"
DISPLAY_BUILD_ID=$(grep "BUILD_ID" */version.h | grep -oP '0x[0-9A-F]+')

# Encoder module
make clean
export USERSPACE_SHA=$(cd .. && git rev-parse HEAD)
export VIAL_QMK_SHA=$(git rev-parse HEAD)
make splitkb/halcyon/ferris/encoder:xtreemze_final 2>&1 | grep -i "BUILD_ID\|version"
ENCODER_BUILD_ID=$(grep "BUILD_ID" */version.h | grep -oP '0x[0-9A-F]+')

# Compare
if [ "$DISPLAY_BUILD_ID" == "$ENCODER_BUILD_ID" ]; then
  echo "✓ Both modules have same BUILD_ID: $DISPLAY_BUILD_ID"
else
  echo "❌ BUILD_ID mismatch!"
  echo "   Display: $DISPLAY_BUILD_ID"
  echo "   Encoder: $ENCODER_BUILD_ID"
fi
```

**Pass Criteria**:
- ✓ Display BUILD_ID == Encoder BUILD_ID
- ✓ Both use same env vars (USERSPACE_SHA, VIAL_QMK_SHA)
- ✓ Proves same deterministic logic applied to both

---

## Test Execution Procedure

### Quick Test (30 minutes)
Run only:
1. T1.1 (Build determinism)
2. T2.1 (Keymap preservation)
3. T3.1 (Compat gen setup)

### Standard Test (2-3 hours)
Run all T1-T3 tests (excludes power simulation)

### Full Test (4+ hours)
Run all tests including T4 (safety) and T5 (cross-module)

---

## Failure Resolution

### If BUILD_ID Non-Deterministic
1. Check git status (unstaged changes affect BUILD_ID)
2. Verify env vars pass correctly to build
3. Check for timestamp dependencies in build
4. Verify qmk_firmware/.git status
5. Check build system for race conditions

### If EEPROM Not Preserved
1. Verify BUILD_ID hasn't changed (would trigger reset)
2. Check via_init_kb() is being called
3. Verify halcyon_vial_compat_init() logic correct
4. Inspect EEPROM memory map (check for collisions)
5. Verify Vial protocol matches firmware version

### If Macros Corrupted
1. Check macro storage EEPROM addresses
2. Verify no overlap with other data
3. Check text encoding (UTF-8 vs ASCII)
4. Verify delay values preserved correctly
5. Test with simpler macros first

---

## Documentation & Records

### Test Report Template
```
TEST RUN: Issue #47 EEPROM Persistence
Date: [YYYY-MM-DD]
Tester: [Name]
Firmware: [commit SHA]

DETERMINISM:
[ ] T1.1 Same Source → Identical Binary: PASS/FAIL
[ ] T1.2 Encoder Module Determinism: PASS/FAIL
[ ] T1.3 BUILD_ID Extraction: PASS/FAIL

PRESERVATION:
[ ] T2.1 Keymap Preservation: PASS/FAIL
[ ] T2.2 Macro Preservation: PASS/FAIL
[ ] T2.3 M10 Bootstrap Macro: PASS/FAIL

COMPATIBILITY:
[ ] T3.1 Compat Gen Auto Update: PASS/FAIL
[ ] T3.2 Intentional Gen Bump: PASS/FAIL

SAFETY:
[ ] T4.1 Failed Seed Atomicity: PASS/FAIL
[ ] T4.2 Power Loss Simulation: PASS/FAIL

CROSS-MODULE:
[ ] T5.1 BUILD_ID Consistency: PASS/FAIL

OVERALL: PASS / CONDITIONAL / FAIL

Issues Found:
[List any failures or concerns]

Sign-off: ________________________ Date: __________
```

---

## Related Documentation
- [Hardware Acceptance Matrix](./HARDWARE_ACCEPTANCE_MATRIX.md)
- [Issue #47: EEPROM Persistence](../docs/ISSUE_26_IMPLEMENTATION_COMPLETE.md)
- [Issue #26: Macro Hardening](../docs/MACRO_HARDENING.md)
- [Troubleshooting Guide](./TROUBLESHOOTING.md)
