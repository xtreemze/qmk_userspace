#!/usr/bin/env node
// SPDX-License-Identifier: GPL-2.0-or-later
// Translate the canonical Vial export as data; macro text is never executed.
import fs from 'node:fs';
const keymapDir = new URL('../keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/', import.meta.url);
const profile = JSON.parse(fs.readFileSync(new URL('xtreemzeVial.vil', keymapDir), 'utf8'));
const keymapPath = new URL('keymap.c', keymapDir);
const before = fs.readFileSync(keymapPath, 'utf8');
let source = before;
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

const kc = normalizeKeycode;
function replace(pattern, value) {
    if (!pattern.test(source)) throw new Error(`Missing generated region: ${pattern}`);
    source = source.replace(pattern, () => value);
}
function table(name, entries) {
    const pattern = new RegExp(`(${name}\\[[^\\]]*\\] = \\{\\n)[\\s\\S]*?\\n\\};`);
    const start = source.match(pattern)?.[1];
    replace(pattern, `${start}${entries.map(entry => `    ${entry},`).join('\n')}\n};`);
}
if (profile.layout.length !== 13 || profile.encoder_layout.length !== 13) throw new Error('Expected 13 layers.');
for (const name of ['macro', 'tap_dance', 'combo', 'key_override', 'alt_repeat_key']) {
    if (profile[name].length !== 32) throw new Error(`Expected 32 ${name} slots.`);
}
for (const actions of profile.macro) {
    for (const [type, ...args] of actions) {
        if (type === 'text') {
            if (args.length !== 1 || /[\x00-\x1f\x7f-\uffff]/u.test(args[0])) throw new Error('Only printable ASCII macro text is supported.');
        } else if (type === 'tap') {
            if (args.some(key => !/^[A-Z0-9_(), ]+$/.test(key))) throw new Error('Invalid macro keycode.');
        } else {
            throw new Error(`Unsupported macro action: ${type}`);
        }
    }
}
// Regenerate only data blocks. Runtime policy and EEPROM versions are reviewed separately.
replace(/const uint16_t PROGMEM keymaps\[\]\[MATRIX_ROWS\]\[MATRIX_COLS\] = \{\n[\s\S]*?\n\};/,
    'const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {\n' + profile.layout.map((layer, i) =>
        `    [L${i}] = {\n${layer.map(row => `        { ${row.map(key => kc(key).padStart(20)).join(', ')} },`).join('\n')}\n    },`).join('\n') + '\n};');
replace(/const uint16_t PROGMEM encoder_map\[\]\[NUM_ENCODERS\]\[NUM_DIRECTIONS\] = \{\n[\s\S]*?\n\};/,
    'const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {\n' + profile.encoder_layout.map((layer, i) =>
        `    [L${i}] = { ${layer.map(pair => `ENCODER_CCW_CW(${pair.map(key => key === 'KC_TRNS' ? key : kc(key)).join(', ')})`).join(', ')} },`).join('\n') + '\n};');
const td = profile.tap_dance.map(entry => `{ ${entry.slice(0, 4).map(kc).join(', ')}, ${entry[4]} }`);
table('xtreemze_default_tap_dances', td);
table('xtreemze_default_combos', profile.combo.map(entry => `{ { ${entry.slice(0, 4).map(kc).join(', ')} }, ${kc(entry[4])} }`));
table('xtreemze_default_key_overrides', profile.key_override.map(entry => `{ .trigger = ${kc(entry.trigger)}, .replacement = ${kc(entry.replacement)}, .layers = ${entry.layers}, .trigger_mods = ${entry.trigger_mods}, .negative_mod_mask = ${entry.negative_mod_mask}, .suppressed_mods = ${entry.suppressed_mods}, .options = ${entry.options} }`));
table('xtreemze_default_alt_repeat_keys', profile.alt_repeat_key.map(entry => `{ .keycode = ${kc(entry.keycode)}, .alt_keycode = ${kc(entry.alt_keycode)}, .allowed_mods = ${entry.allowed_mods}, .options = ${entry.options} }`));
table('xtreemze_qmk_settings_defaults', Object.entries(profile.settings).map(([id, value]) => `{ ${id}, ${value} }`));
replace(/via_set_layout_options\(\d+U\);/, `via_set_layout_options(${profile.layout_options}U);`);

// Four bytes per tap is an upper bound (basic taps use three), plus all slot terminators.
const macroBudget = profile.macro.reduce((n, actions) => n + 1 + actions.reduce((size, action) => size + (action[0] === 'text' ? action[1].length : 4 * (action.length - 1)), 0), 0);
const macroBody = profile.macro.map((actions, i) => {
    const lines = [`    // M${i}: ${actions.length ? 'canonical profile' : 'empty'}`];
    for (const [type, ...args] of actions) {
        if (type === 'text') lines.push(`    ok &= macro_seed_write_text(&offset, macro_buffer_size, ${JSON.stringify(args[0])});`);
        else for (const key of args) lines.push(`    ok &= macro_seed_write_tap(&offset, macro_buffer_size, ${kc(key)});`);
    }
    lines.push('    ok &= macro_seed_end_slot(&offset, macro_buffer_size);');
    return lines.join('\n');
}).join('\n\n');
replace(/static (?:void|bool) seed_vial_macro_defaults\(void\) \{\n[\s\S]*?\n\}/, `static bool seed_vial_macro_defaults(void) {
    const uint16_t macro_buffer_size = dynamic_keymap_macro_get_buffer_size();
    // Reserve the final zero byte required by the Vial decoder before erasing anything.
    if (macro_buffer_size <= ${macroBudget}) {
        return false;
    }
    dynamic_keymap_macro_reset();
    uint16_t offset = 0;
    bool ok = true;

${macroBody}

    return ok;
}`);
// Keep the optional static fallback tables aligned too; shipping builds use Vial.
replace(/enum combo_events \{[\s\S]*?\n\};\n#endif/, `enum combo_events {\n${profile.combo.map((_, i) => `    CM_${i},`).join('\n')}\n};\n\n` +
    profile.combo.map((entry, i) => `static const uint16_t PROGMEM combo_${i}[] = { ${entry.slice(0, 4).filter(key => key !== 'KC_NO').map(kc).concat('COMBO_END').join(', ')} };`).join('\n') +
    '\n\ncombo_t key_combos[] = {\n' + profile.combo.map((entry, i) => `    [CM_${i}] = COMBO(combo_${i}, ${kc(entry[4])}),`).join('\n') + '\n};\n#endif');
replace(/#define TAP_DANCE_SLOT_COUNT \d+/, '#define TAP_DANCE_SLOT_COUNT 32');
table('tap_dance_entries', td.map((entry, i) => `[${i}] = ${entry}`));
table('tap_dance_actions', profile.tap_dance.map((_, i) => `[${i}] = { .fn = {on_dance, on_dance_finished, on_dance_reset, NULL}, .user_data = (void *)(uintptr_t)${i} }`));
table('alt_repeat_entries', profile.alt_repeat_key.map(entry => `{ ${kc(entry.keycode)}, ${kc(entry.alt_keycode)}, ${entry.allowed_mods}, ${entry.options} }`));
const fallbackMacros = profile.macro.map((actions, i) => {
    if (!actions.length) return '';
    const lines = [`        case ${i}:`];
    for (const [type, ...args] of actions) {
        if (type === 'text') lines.push(`            SEND_STRING(${JSON.stringify(args[0])});`);
        else for (const key of args) lines.push(`            tap_action_keycode(${kc(key)});`);
    }
    return lines.concat('            break;').join('\n');
}).filter(Boolean).join('\n');
replace(/static void run_macro_slot\(uint8_t slot\) \{[\s\S]*?\n\}/, `static void run_macro_slot(uint8_t slot) {
#ifdef VIAL_ENABLE
    // USER0..USER9 are aliases of the current Vial macros, including later edits.
    dynamic_keymap_macro_send(slot);
#else
    switch (slot) {
${fallbackMacros}
        default:
            break;
    }
#endif
}`);
if (process.argv.includes('--check')) {
    if (source !== before) throw new Error('Compiled Vial defaults are stale; run node scripts/generate-vial-defaults.mjs.');
} else {
    fs.writeFileSync(keymapPath, source);
}
