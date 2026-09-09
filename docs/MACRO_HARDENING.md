# Macro Hardening: Vial M10 Bootstrap Macro

**Issue**: #26 - Configuration hardening: avoid mutable curl-to-shell target in M10

---

## Overview

This document explains the recommended configuration for Vial macro M10 and the security considerations around using convenience setup macros.

## The M10 Macro: Bootstrap Setup Helper

Macro M10 is a convenience macro that types a shell command to set up your development environment. The macro is **not a keybinding** — it's stored in Vial's dynamic macro system and requires **manual submission** (no Enter key included in the macro).

### Purpose
Typing M10 inserts a bootstrap script command that you can review and manually run in a terminal. This keeps the firmware from executing arbitrary code while still providing setup convenience.

### Security Principle
The macro **requires manual submission by the user**, which creates a safety boundary:
- You see the command before running it
- You can review it against the current .dotfiles repository state
- You can choose to run or skip it

## Recommended M10 Configuration

### Safe Setup with Pinned Source

Use a stable branch or commit-SHA-pinned URL instead of `master` branch:

```bash
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh | bash
```

**Why stable branch instead of mutable `master`**:
- ✅ More predictable: `main` is the stable default on most projects
- ✅ Consistent: Fewer surprise changes
- ✅ Traceable: Can verify bootstrap.sh hasn't drastically changed
- ✅ Safe: Future .dotfiles changes are intentional and reviewed

**Future: Commit SHA Pinning**:
For maximum reproducibility, use a specific commit SHA:
```bash
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/abc123def456/bootstrap.sh | bash
```

## Setting Up M10 in Vial

### Option A: Use Default (Recommended)
If your Vial profile includes M10 preconfigured, just use it as-is.

### Option B: Configure Manually
If you want to customize M10:

1. Open Vial on your keyboard
2. Go to Macros
3. Click on macro slot M10
4. Paste:
   ```
   curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh | bash
   ```
5. **Important**: Do NOT include Enter/Return key at the end

### Verifying Your Setup
To test that M10 is correctly configured:

1. Press/type M10 in any application
2. You should see the full URL appear (not executed)
3. Review the command in your terminal
4. Manually press Enter only if you want to run it

## Why NOT to Use `master` Branch

⚠️ **Never use `curl ... master/...`** because:

1. **Mutable**: The `master` branch can change anytime
2. **Untested**: Changes to bootstrap.sh might not work with your firmware
3. **Surprising**: M10 might offer different setup today vs. tomorrow
4. **Untracked**: Release notes won't document M10's actual behavior

## When to Update M10

Update M10 when:
- ✅ .dotfiles bootstrap.sh has important security fixes
- ✅ .dotfiles adds support for new OS or tools
- ✅ Firmware release notes mention M10 changes

Do NOT need to update if:
- ❌ Minor .dotfiles changes unrelated to keyboard setup
- ❌ .dotfiles adds features you don't need
- ❌ You have a custom workflow

## Updating M10 to a New SHA

When a new firmware release includes an updated M10:

1. Check release notes for the new bootstrap source
2. Find the Vial macro documentation in the release
3. Update M10 in Vial with the new URL (if using manual config)
4. Verify the URL matches exactly
5. Test by typing M10 and reviewing the command

## Advanced: Verifying the Script

If you want extra security verification:

```bash
# Before running the bootstrap, verify it hasn't been tampered with
# Get the script first without running:
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh

# Review the output, then run it:
curl -fsSL https://raw.githubusercontent.com/xtreemze/.dotfiles/main/bootstrap.sh | bash
```

## Troubleshooting

### "M10 doesn't type anything"
- Check that M10 is assigned in Vial
- Verify the macro contains the curl command
- Confirm the macro doesn't include Enter key (should be just text)

### "URL is incorrect/outdated"
- Check the release notes for the current bootstrap source
- Update the URL in Vial to match
- Verify no typos in the URL

### "Bootstrap.sh isn't found (404 error)"
- Verify the source is accessible (check .dotfiles repo)
- Confirm you're using the correct URL format
- Check that the .dotfiles repository is public

## Release Notes Format

When M10 is updated, release notes should include:

```markdown
### M10 Bootstrap Macro Updated
Updated the M10 convenient setup macro to use the latest .dotfiles bootstrap script.

M10 is a convenience macro that types this command without executing it.
Review the command and manually press Enter in your terminal to run it.

See [`docs/MACRO_HARDENING.md`](./docs/MACRO_HARDENING.md) for more details.
```

## Security Model

This configuration follows the principle of **defense in depth**:

1. **Firmware**: Doesn't execute anything, just types text
2. **Vial Macro**: Is just a convenience, user has full control
3. **Script Source**: Uses stable branch, can't be accidentally changed
4. **User Review**: Must manually approve execution
5. **Transparency**: Release notes document any changes

---

## See Also
- Issue #26: Configuration hardening
- Issue #25: Release provenance (why reproducibility matters)
- Issue #47: EEPROM persistence (why firmware determinism matters)
