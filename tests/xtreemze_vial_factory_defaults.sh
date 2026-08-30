#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
keymap_dir="$repo_root/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final"

node - "$keymap_dir/xtreemzeVial.vil" "$keymap_dir/keymap.c" "$keymap_dir/config.h" "$keymap_dir/rules.mk" "$keymap_dir/vial.json" <<'NODE'
const fs = require('fs');

const [vilPath, keymapPath, configPath, rulesPath, vialPath] = process.argv.slice(2);
const profile = JSON.parse(fs.readFileSync(vilPath, 'utf8'));
const source = fs.readFileSync(keymapPath, 'utf8');
const config = fs.readFileSync(configPath, 'utf8');
const rules = fs.readFileSync(rulesPath, 'utf8');
const definition = JSON.parse(fs.readFileSync(vialPath, 'utf8'));

function fail(message) {
    throw new Error(message);
}

function assert(condition, message) {
    if (!condition) fail(message);
}

function normalizeKeycode(value) {
    if (value === -1) return 'KC_NO';

    const direct = {
        KC_TRNS: '_______',
        BL_INC: 'BL_UP',
        BL_DEC: 'BL_DOWN',
        RGB_TOG: 'RM_TOGG',
        RGB_SAD: 'RM_SATD',
        RGB_SAI: 'RM_SATU',
        RGB_VAD: 'RM_VALD',
        RGB_VAI: 'RM_VALU',
        RGB_SPD: 'RM_SPDD',
        RGB_SPI: 'RM_SPDU',
        RGB_RMOD: 'RM_PREV',
        RGB_MOD: 'RM_NEXT',
        RGB_HUD: 'RM_HUED',
        RGB_HUI: 'RM_HUEU',
        KC_LCTRL: 'KC_LCTL',
        KC_LSHIFT: 'KC_LSFT',
        KC_ESCAPE: 'KC_ESC',
        KC_ENTER: 'KC_ENT',
        KC_BSPACE: 'KC_BSPC',
        KC_DELETE: 'KC_DEL',
        KC_GRAVE: 'KC_GRV',
        KC_QUOTE: 'KC_QUOT',
        KC_SCOLON: 'KC_SCLN',
        KC_COMMA: 'KC_COMM',
        KC_MINUS: 'KC_MINS',
        KC_LBRACKET: 'KC_LBRC',
        KC_RBRACKET: 'KC_RBRC',
        KC_BSLASH: 'KC_BSLS',
        KC_EQUAL: 'KC_EQL',
        KC_RIGHT: 'KC_RGHT',
        KC_PGDOWN: 'KC_PGDN',
        KC_KP_PLUS: 'KC_PPLS',
        KC_KP_MINUS: 'KC_PMNS',
        KC_KP_ASTERISK: 'KC_PAST',
        KC_KP_SLASH: 'KC_PSLS',
        KC_KP_COMMA: 'KC_PCMM',
        KC_KP_DOT: 'KC_PDOT',
        KC_LSPO: 'SC_LSPO',
        KC_RSPC: 'SC_RSPC',
        KC_ASTG: 'AS_TOGG',
        KC_MS_L: 'MS_LEFT',
        KC_MS_D: 'MS_DOWN',
        KC_MS_U: 'MS_UP',
        KC_MS_R: 'MS_RGHT',
        KC_WH_U: 'MS_WHLU',
        KC_WH_D: 'MS_WHLD',
        KC__VOLUP: 'KC_KB_VOLUME_UP',
        KC__VOLDOWN: 'KC_KB_VOLUME_DOWN',
    };
    if (direct[value]) return direct[value];

    const customNames = ['XM_0', 'XM_1', 'XM_2', 'XM_3', 'XM_4', 'XM_5', 'XM_6', 'XM_7', 'XM_8', 'XM_9', 'RGB_SLAY', 'RGB_SMOD', 'RGB_SCHD', 'RGB_TCHD', 'OS_REDO', 'OS_PSTE', 'OS_COPY', 'OS_CUT', 'OS_UNDO', 'OS_SALL', 'OS_TRACE_VIEW'];
    let match = /^USER(\d+)$/.exec(value);
    if (match) return customNames[Number(match[1])];

    match = /^M(\d+)$/.exec(value);
    if (match) return `QK_MACRO_${Number(match[1])}`;

    match = /^LT(\d+)\((.*)\)$/.exec(value);
    if (match) return `LT(${match[1]}, ${match[2] === 'KC_SPACE' ? 'KC_SPC' : normalizeKeycode(match[2])})`;

    match = /^(LCTL_T|LGUI_T|LSFT_T|LALT_T|RCTL_T|RGUI_T|RSFT_T|RALT_T)\((.*)\)$/.exec(value);
    if (match) return `${match[1]}(${match[2] === 'KC_SPACE' ? 'KC_SPC' : normalizeKeycode(match[2])})`;

    match = /^(LSFT|RSFT|LCTL|LALT|LGUI|RCTL|RALT|RGUI|SGUI|HYPR|MEH)\((.*)\)$/.exec(value);
    if (match) return `${match[1]}(${normalizeKeycode(match[2])})`;

    match = /^C_S\((.*)\)$/.exec(value);
    if (match) return `LCTL(LSFT(${normalizeKeycode(match[1])}))`;

    return value;
}

function splitTopLevel(value) {
    const result = [];
    let depth = 0;
    let start = 0;
    for (let i = 0; i < value.length; i++) {
        if (value[i] === '(') depth++;
        if (value[i] === ')') depth--;
        if (value[i] === ',' && depth === 0) {
            result.push(value.slice(start, i).trim());
            start = i + 1;
        }
    }
    result.push(value.slice(start).trim());
    return result;
}

assert(/"uid"\s*:\s*3946455230411839832[,}]/.test(fs.readFileSync(vilPath, 'utf8')), 'Unexpected Vial keyboard UID.');
assert(profile.layout.length === 13, 'Factory profile must contain all 13 layers.');
assert(profile.encoder_layout.length === 13, 'Factory profile must contain encoder settings for all 13 layers.');
assert(profile.combo.length === 32 && profile.tap_dance.length === 32 && profile.macro.length === 32 && profile.key_override.length === 32 && profile.alt_repeat_key.length === 32, 'Factory profile must preserve all 32 Vial dynamic slots.');

for (const feature of ['VIA_ENABLE = yes', 'VIAL_ENABLE = yes', 'VIALRGB_ENABLE = yes', 'QMK_SETTINGS = yes', 'COMBO_ENABLE = yes', 'TAP_DANCE_ENABLE = yes', 'KEY_OVERRIDE_ENABLE = yes', 'REPEAT_KEY_ENABLE = yes', 'ENCODER_MAP_ENABLE = yes']) {
    assert(rules.includes(feature), `Missing firmware feature gate: ${feature}`);
}
for (const capacity of ['DYNAMIC_KEYMAP_LAYER_COUNT 13', 'DYNAMIC_KEYMAP_MACRO_COUNT 32', 'VIAL_TAP_DANCE_ENTRIES 32', 'VIAL_COMBO_ENTRIES 32', 'VIAL_KEY_OVERRIDE_ENTRIES 32', 'VIAL_ALT_REPEAT_KEY_ENTRIES 32']) {
    assert(config.includes(capacity), `Missing Vial capacity: ${capacity}`);
}
assert(definition.lighting === 'vialrgb', 'VialRGB must be exposed by the keyboard definition.');
assert((definition.keycodes || []).includes('qmk_lighting'), 'QMK RGB keycodes must be visible in Vial.');
assert((definition.menus || []).includes('qmk_rgb_matrix'), 'RGB Matrix controls must be visible in Vial.');

function assertDeterministicPair(primary, alternate) {
    const entry = profile.alt_repeat_key.find(candidate => candidate.keycode === primary && candidate.alt_keycode === alternate);
    assert(entry, `Missing deterministic pair ${primary} (CW) <-> ${alternate} (CCW).`);
    assert((entry.options & 0x08) !== 0, `Deterministic pair ${primary} <-> ${alternate} must be enabled.`);
    assert((entry.options & 0x02) !== 0, `Deterministic pair ${primary} <-> ${alternate} must be bidirectional.`);
}

for (const [clockwise, counterClockwise] of [
    ['KC_RIGHT', 'KC_LEFT'],
    ['KC_UP', 'KC_DOWN'],
    ['TD(11)', 'TD(10)'],
    ['TD(12)', 'TD(9)'],
    ['KC_PGUP', 'KC_PGDOWN'],
    ['KC_END', 'KC_HOME'],
    ['KC_K', 'KC_J'],
    ['KC_L', 'KC_H'],
    ['LCTL(KC_U)', 'LCTL(KC_D)'],
    ['KC_W', 'KC_B'],
    ['KC_TAB', 'LSFT(KC_TAB)'],
    ['LGUI(KC_G)', 'SGUI(KC_G)'],
    ['KC_DELETE', 'KC_BSPACE'],
    ['KC_RBRACKET', 'KC_LBRACKET'],
    ['LSFT(KC_RBRACKET)', 'LSFT(KC_LBRACKET)'],
    ['LSFT(KC_DOT)', 'LSFT(KC_COMMA)'],
    ['KC_RSPC', 'KC_LSPO'],
    ['LSFT(KC_0)', 'LSFT(KC_9)'],
]) {
    assertDeterministicPair(clockwise, counterClockwise);
}
assert(config.includes('XTREEMZE_DEFAULTS_EE_MARKER 0xB0') || source.includes('XTREEMZE_DEFAULTS_EE_MARKER 0xB0'), 'Factory seed marker must be bumped for the deterministic direction profile.');

const keymapBlock = source.slice(source.indexOf('const uint16_t PROGMEM keymaps'), source.indexOf('#if defined(ENCODER_MAP_ENABLE)'));
const layerMatches = [...keymapBlock.matchAll(/^    \[L(\d+)\] = \{\n([\s\S]*?)^    \},$/gm)];
assert(layerMatches.length === 13, `Expected 13 compiled keymap layers, found ${layerMatches.length}.`);
for (const match of layerMatches) {
    const layer = Number(match[1]);
    const rows = [...match[2].matchAll(/^        \{(.*)\},$/gm)].map(row => splitTopLevel(row[1]));
    const expected = profile.layout[layer].map(row => row.map(normalizeKeycode));
    assert(JSON.stringify(rows) === JSON.stringify(expected), `Compiled keymap layer ${layer} differs from xtreemzeVial.vil.\nactual=${JSON.stringify(rows)}\nexpected=${JSON.stringify(expected)}`);
}

const encoderBlock = source.slice(source.indexOf('const uint16_t PROGMEM encoder_map'), source.indexOf('#if defined(COMBO_ENABLE)'));
const encoderMatches = [...encoderBlock.matchAll(/\[L(\d+)\] = \{ (.*) \},/g)];
assert(encoderMatches.length === 13, `Expected 13 compiled encoder layers, found ${encoderMatches.length}.`);
for (const match of encoderMatches) {
    const layer = Number(match[1]);
    const actual = splitTopLevel(match[2]).map(entry => splitTopLevel(entry.slice('ENCODER_CCW_CW('.length, -1)));
    const expected = profile.encoder_layout[layer].map(entry => entry.map(value => value === 'KC_TRNS' ? 'KC_TRNS' : normalizeKeycode(value)));
    assert(JSON.stringify(actual) === JSON.stringify(expected), `Compiled encoder layer ${layer} differs from xtreemzeVial.vil.\nactual=${JSON.stringify(actual)}\nexpected=${JSON.stringify(expected)}`);
}

// Compare slot order and exact table length, including every cleared slot.
function assertTable(name, expected) {
    const block = source.match(new RegExp(`${name}\\[\\] = \\{\\n([\\s\\S]*?)\\n\\};`));
    assert(block, `Missing ${name}.`);
    const actual = block[1].split('\n').map(line => line.trim()).filter(Boolean);
    assert(JSON.stringify(actual) === JSON.stringify(expected), `${name} differs from xtreemzeVial.vil (including slot order/count).`);
}
assertTable('xtreemze_default_tap_dances', profile.tap_dance.map(entry => `{ ${entry.slice(0, 4).map(normalizeKeycode).join(', ')}, ${entry[4]} },`));
assertTable('xtreemze_default_combos', profile.combo.map(entry => `{ { ${entry.slice(0, 4).map(normalizeKeycode).join(', ')} }, ${normalizeKeycode(entry[4])} },`));
assertTable('xtreemze_default_key_overrides', profile.key_override.map(entry => `{ .trigger = ${normalizeKeycode(entry.trigger)}, .replacement = ${normalizeKeycode(entry.replacement)}, .layers = ${entry.layers}, .trigger_mods = ${entry.trigger_mods}, .negative_mod_mask = ${entry.negative_mod_mask}, .suppressed_mods = ${entry.suppressed_mods}, .options = ${entry.options} },`));
assertTable('xtreemze_default_alt_repeat_keys', profile.alt_repeat_key.map(entry => `{ .keycode = ${normalizeKeycode(entry.keycode)}, .alt_keycode = ${normalizeKeycode(entry.alt_keycode)}, .allowed_mods = ${entry.allowed_mods}, .options = ${entry.options} },`));
assertTable('xtreemze_qmk_settings_defaults', Object.entries(profile.settings).map(([id, value]) => `{ ${id}, ${value} },`));

const macroStart = source.lastIndexOf('static bool seed_vial_macro_defaults(void)');
const macroBlock = source.slice(macroStart, source.indexOf('#ifdef QMK_SETTINGS', macroStart));
let macroTerminators = 0;
const compiledMacroSlots = [];
const compiledMacros = Array.from({length: 32}, () => []);
let compiledMacroIndex = -1;
for (const line of macroBlock.split('\n')) {
    if (line.includes('ok &= macro_seed_end_slot(&offset, macro_buffer_size);')) macroTerminators++;
    let match = /\/\/ M(\d+):/.exec(line);
    if (match) {
        compiledMacroIndex = Number(match[1]);
        compiledMacroSlots.push(compiledMacroIndex);
        continue;
    }
    match = /macro_seed_write_tap\(&offset, macro_buffer_size, (.*)\);/.exec(line);
    if (match && compiledMacroIndex >= 0 && compiledMacroIndex < compiledMacros.length) {
        compiledMacros[compiledMacroIndex].push(['tap', match[1]]);
        continue;
    }
    match = /macro_seed_write_text\(&offset, macro_buffer_size, (".*")\);/.exec(line);
    if (match && compiledMacroIndex >= 0 && compiledMacroIndex < compiledMacros.length) {
        compiledMacros[compiledMacroIndex].push(['text', JSON.parse(match[1])]);
    }
}
for (let index = 0; index < 32; index++) {
    const expected = [];
    for (const action of profile.macro[index]) {
        if (action[0] === 'tap') {
            for (const keycode of action.slice(1)) expected.push(['tap', normalizeKeycode(keycode)]);
        } else if (action[0] === 'text') {
            expected.push(['text', action[1]]);
        } else {
            fail(`Untested macro action: ${action[0]}`);
        }
    }
    assert(JSON.stringify(compiledMacros[index]) === JSON.stringify(expected), `Macro ${index} differs from xtreemzeVial.vil.`);
}
assert(macroTerminators === 32 && compiledMacroSlots.every((slot, i) => slot === i) && compiledMacroSlots.length === 32, 'All 32 macro slots, including empty ones, must be explicitly terminated.');
assert(source.includes('dynamic_keymap_macro_send(slot);'), 'Legacy USER macro aliases must use current Vial macro data.');
assert(source.includes(`via_set_layout_options(${profile.layout_options}U);`), 'Halcyon module layout options differ from xtreemzeVial.vil.');
NODE

node "$repo_root/scripts/generate-vial-defaults.mjs" --check
python3 "$repo_root/tests/xtreemze_vial_macro_encoding.py"
