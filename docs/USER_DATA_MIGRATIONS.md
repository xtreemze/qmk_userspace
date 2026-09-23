# User-data schema and migration policy

The Ferris userspace stores fork-owned state in QMK's 128-byte user datablock. This schema is independent of Vial's dynamic-keymap storage and the factory-default marker.

## Schema history

| Version | Status | Layout / migration |
| --- | --- | --- |
| `0x02` | Current; first versioned schema in repository history | `defaults_marker`, host-family byte, chord timing, layer/modifier/chord RGB profiles. No older versioned layout exists to migrate. |

There is no synthetic `0x01` migration. Repository history introduced the versioned user datablock at `0x02`.

## Compatibility policy

A valid datablock with the xtreemze magic and the current version is writable.

A valid datablock with the xtreemze magic but an unsupported version is treated as potentially belonging to newer firmware. Firmware uses safe in-RAM defaults for that boot, but it must not overwrite that raw user datablock and must not trigger compiled Vial factory reseeding. This makes downgrade/forward-version encounters non-destructive by default.

A datablock with invalid QMK validity metadata is initialized normally. A valid datablock with foreign/invalid xtreemze magic is treated as unowned/corrupt and is reinitialized.

All full-block reads and writes must transfer exactly `sizeof(xtreemze_user_data_t)`. The type is compile-time bounded by `EECONFIG_USER_DATA_SIZE`.

## Changing the schema

`XTREEMZE_USER_DATA_VERSION` is intentionally pinned by regression coverage. Any version change must update that regression and this table in the same PR, and must choose one of these paths:

1. add an explicit field-preserving migration from every supported prior schema; or
2. document why a destructive migration is required and what persisted fields are intentionally discarded.

A schema bump must not accidentally clear `defaults_marker` and thereby reset Vial keymaps, macros, tap dances, combos, overrides, or settings. Factory-profile reseeding is a separate migration decision.
