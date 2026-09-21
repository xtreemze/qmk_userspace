# Host configurator protocol contract

The Ferris `xtreemze_final` firmware exposes a small xtreemze-specific host API in addition to standard Vial/VIA behavior. This document defines the compatibility boundary for desktop and browser configurators.

The machine-readable source of truth is [`configurator-protocol-v1.json`](configurator-protocol-v1.json). CI checks that its command IDs, protocol versions, and operation numbers continue to match the firmware C sources.

## Framing

The custom API uses VIA raw HID packets.

- byte 0 is the custom command namespace;
- byte 1 is the operation within that namespace;
- request prefix bytes are kept intact so Vial/VIA command echo validation can continue to work;
- multi-byte integers in the existing xtreemze protocols are encoded big-endian;
- an unsupported namespace or operation is rejected through the firmware's existing `id_unhandled` path.

Clients must probe capabilities instead of inferring support from the firmware name, keyboard name, repository revision, or UI version.

## Active namespaces

### `0xF0` — RGB profiles, protocol v1

Implemented by `rgb_profile_protocol.c`.

Operations:

| Operation | Value |
| --- | ---: |
| `GET_CAPABILITIES` | `0x01` |
| `GET_PROFILE` | `0x02` |
| `SET_PROFILE` | `0x03` |
| `SAVE` | `0x04` |
| `PREVIEW` | `0x05` |
| `GET_COMBO_DURATION` | `0x06` |
| `SET_COMBO_DURATION` | `0x07` |
| `CANCEL_PREVIEW` | `0x08` |

The capability response remains the authority for available scopes, profile counts, brightness/effect bounds, and supported fields. Clients must not hard-code those values merely because the current Ferris profile has known defaults.

### `0xF1` — Halcyon settings and telemetry, protocol v1

Implemented by `halcyon_settings_protocol.c/.h`.

Operations:

| Operation | Value |
| --- | ---: |
| `GET_CAPABILITIES` | `0x01` |
| `GET_LAYER_STYLE` | `0x02` |
| `SET_LAYER_STYLE` | `0x03` |
| `GET_MOD_STYLE` | `0x04` |
| `SET_MOD_STYLE` | `0x05` |
| `GET_TIMINGS` | `0x06` |
| `SET_TIMINGS` | `0x07` |
| `GET_DIM_STYLE` | `0x08` |
| `SET_DIM_STYLE` | `0x09` |
| `GET_TELEMETRY` | `0x0A` |
| `SAVE` | `0x0B` |
| `RESET` | `0x0C` |

The capability response is authoritative for layer/modifier counts, timing bounds, TFT presence, split-replication capability, and deterministic Repeat policy.

Runtime edits and persistence are intentionally separate: setters update live state; `SAVE` makes the current state durable and schedules persistent split replication. `RESET` restores compiled defaults in live state and does not silently redefine the protocol version.

### `0xF2` — Halcyon display labels and procedural patterns, protocol v1

Implemented by `halcyon_display_protocol.c/.h`.

Operations:

| Operation | Value |
| --- | ---: |
| `GET_CAPABILITIES` | `0x01` |
| `GET_LAYER` | `0x02` |
| `SET_LAYER` | `0x03` |
| `GET_MODIFIER_LABEL` | `0x04` |
| `SET_MODIFIER_LABEL` | `0x05` |
| `SAVE` | `0x06` |
| `RESET` | `0x07` |

The capability response requires at least 14 bytes. Bytes 2–13 report, in order: protocol version, capability flags, layer count, modifier count, motif count, maximum layer-label characters, maximum modifier-label characters, minimum and maximum tile size, maximum motion amplitude, maximum pulse amplitude, and store version.

Layer read/write packets require at least 17 bytes. Byte 2 is the layer index; bytes 3–11 contain a nine-byte NUL-terminated printable-ASCII label field; bytes 12–16 contain motif, tile width, tile height, motion amplitude, and pulse amplitude. Clients must use the capability response for current counts and bounds rather than assuming the compiled defaults.

Modifier-label packets require at least eight bytes. Byte 2 is the modifier index and bytes 3–7 contain a five-byte NUL-terminated printable-ASCII label field.

Runtime writes update live display state and replicate to the split peer without forcing persistence. `SAVE` persists the current display store and schedules persistent split replication. `RESET` restores compiled defaults in live state. The display store schema/version remains independent from the host protocol version.

## Compatibility rules

1. Do not reuse an existing operation number for an incompatible payload.
2. Bump the protocol version for an incompatible shape or semantic change.
3. Keep host protocol versioning independent from EEPROM/store schema versioning.
4. Prefer capability bits/counts/bounds over client-side firmware-name checks.
5. Preserve unknown-operation rejection so one custom namespace cannot be misinterpreted as another.
6. Add or update golden vectors before a non-Python client depends on a new operation.
7. Keep Python Vial, WebHID, and Tauri/native HID clients behaviorally equivalent at the protocol layer.

## Migration relevance

This contract is the stable seam for the planned web-native Vial frontend and Tauri desktop client. The frontend may change framework, component library, or transport implementation without requiring a firmware redesign as long as the packet contract remains compatible.
