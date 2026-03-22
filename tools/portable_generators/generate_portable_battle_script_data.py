#!/usr/bin/env python3

import os
import re
import sys


MACRO_START_RE = re.compile(r"^\s*\.macro\s+([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$")
MACRO_END_RE = re.compile(r"^\s*\.endm\s*$")
LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)::\s*$")
INLINE_LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*):\s*$")
IDENT_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
POINTER_TABLE_LABELS = {
    "gBattleScriptsForMoveEffects",
    "gBattlescriptsForBallThrow",
    "gBattlescriptsForRunningByItem",
    "gBattlescriptsForUsingItem",
    "gBattlescriptsForSafariActions",
}
TERMINATING_MACROS = frozenset({"end", "end2", "end3", "goto", "return"})
POINTER_PREFIXES = (
    "BattleScript_",
    "EventScript_",
    "Text_",
    "CableClub_",
    "Help_",
    "Movement_",
    "TrainerTower_",
    "PalletTown_",
    "g",
    "s",
    "c",
)


class Macro:
    def __init__(self, params, defaults, body):
        self.params = params
        self.defaults = defaults
        self.body = body


def strip_comment(line):
    if "@" in line:
        line = line.split("@", 1)[0]
    return line.rstrip()


def split_args(text):
    args = []
    current = []
    depth = 0
    for ch in text:
        if ch == ',' and depth == 0:
            args.append(''.join(current).strip())
            current = []
            continue
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        current.append(ch)
    tail = ''.join(current).strip()
    if tail:
        args.append(tail)
    return args


def parse_macro_params(text):
    params = []
    defaults = {}
    if not text.strip():
        return params, defaults
    for item in split_args(text):
        if '=' in item:
            name, default = item.split('=', 1)
            name = name.split(':', 1)[0].strip()
            params.append(name)
            defaults[name] = default.strip()
        else:
            params.append(item.split(':', 1)[0].strip())
    return params, defaults


def load_macros(path):
    macros = {}
    with open(path, 'r', encoding='utf-8') as handle:
        lines = handle.readlines()
    index = 0
    while index < len(lines):
        match = MACRO_START_RE.match(lines[index])
        if not match:
            index += 1
            continue
        name = match.group(1)
        params, defaults = parse_macro_params(match.group(2))
        index += 1
        body = []
        while index < len(lines) and not MACRO_END_RE.match(lines[index]):
            body.append(strip_comment(lines[index].rstrip('\n')))
            index += 1
        macros[name] = Macro(params, defaults, body)
        index += 1
    return macros


def substitute(body_line, mapping):
    result = body_line
    for name, value in mapping.items():
        result = result.replace(f"\\{name}", value)
    return result


def expand_line(line, macros):
    stripped = line.strip()
    if not stripped:
        return []
    token = stripped.split(None, 1)[0]
    if token not in macros:
        return [stripped]
    macro = macros[token]
    raw_args = stripped[len(token):].strip()
    args = split_args(raw_args)
    if len(args) < len(macro.params) and ',' not in raw_args:
        args = raw_args.split(None, len(macro.params) - 1)
    mapping = {}
    for i, name in enumerate(macro.params):
        if i < len(args):
            mapping[name] = args[i]
        elif name in macro.defaults:
            mapping[name] = macro.defaults[name]
        else:
            raise ValueError(f"missing arg for {token}: {name}")
    expanded = []
    for body_line in macro.body:
        expanded.extend(expand_line(substitute(body_line, mapping), macros))
    return expanded


def load_labels(path, macros):
    labels = {}
    label_order = []
    last_macro = {}
    current = None
    with open(path, 'r', encoding='utf-8') as handle:
        for raw_line in handle:
            line = strip_comment(raw_line.rstrip('\n'))
            if not line.strip():
                continue
            stripped = line.lstrip()
            if stripped.startswith('#include') or stripped.startswith('.include') or stripped.startswith('.set') or stripped.startswith('.section'):
                continue
            match = LABEL_RE.match(line)
            if match:
                current = match.group(1)
                labels[current] = []
                label_order.append(current)
                continue
            inline_match = INLINE_LABEL_RE.match(line)
            if inline_match:
                name = inline_match.group(1)
                if current is None:
                    current = name
                    labels[current] = []
                    label_order.append(current)
                else:
                    labels[current].append(("label", name))
                continue
            if current is None:
                continue
            token = stripped.split(None, 1)[0]
            if not stripped.startswith('.'):
                last_macro[current] = token
            labels[current].extend(expand_line(line, macros))
    return labels, label_order, last_macro


def add_fallthroughs(labels, label_order, last_macro):
    """Append explicit goto to labels that fall through to the next label."""
    for i, label in enumerate(label_order):
        if label in POINTER_TABLE_LABELS:
            continue
        macro = last_macro.get(label)
        if macro is None or macro in TERMINATING_MACROS:
            continue
        j = i + 1
        while j < len(label_order) and label_order[j] in POINTER_TABLE_LABELS:
            j += 1
        if j >= len(label_order):
            continue
        next_label = label_order[j]
        labels[label].append('.byte 0x28')
        labels[label].append(f'.4byte {next_label}')


def is_pointer_expr(expr):
    expr = expr.strip()
    if expr in {"0", "NULL"}:
        return False
    idents = IDENT_RE.findall(expr)
    if not idents:
        return False
    for ident in idents:
        if ident in {"NULL", "TRUE", "FALSE"}:
            continue
        if ident[0].islower():
            return True
        if ident.startswith(POINTER_PREFIXES):
            return True
        if not ident.isupper():
            return True
    return False


def sanitize(expr):
    return re.sub(r"[^A-Za-z0-9]+", "_", expr).strip('_')


def c_ptr_expr(expr, local_symbols):
    expr = expr.strip()
    if expr == "NULL":
        return "NULL"
    match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)(\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+))?", expr)
    if match:
        symbol = match.group(1)
        ref = symbol if symbol in local_symbols or symbol.startswith(('s', 'c')) else f"&{symbol}"
        if match.group(2):
            return f"((const void *)((uintptr_t){ref} {match.group(3)} {match.group(4)}))"
        return f"((const void *)(uintptr_t){ref})"
    return f"((const void *)(uintptr_t)({expr}))"


def emit_u16(expr):
    if expr.strip() == 'NULL':
        expr = '0'
    return [f"U16_LO({expr})", f"U16_HI({expr})"]


def emit_u32(expr):
    if expr.strip() == 'NULL':
        expr = '0'
    return [f"U32_B0({expr})", f"U32_B1({expr})", f"U32_B2({expr})", f"U32_B3({expr})"]


def generate_source(labels):
    token_order = []
    token_values = {}

    def token_for(expr):
        if expr not in token_values:
            token_values[expr] = f"(FIRERED_BATTLE_PTR_TOKEN_BASE + {len(token_order)}u)"
            token_order.append(expr)
        return token_values[expr]

    out = []
    out.append('#include "global.h"')
    out.append('#include "battle_scripts.h"')
    out.append('#include "constants/global.h"')
    out.append('#include "constants/moves.h"')
    out.append('#include "constants/battle.h"')
    out.append('#include "constants/battle_move_effects.h"')
    out.append('#include "constants/battle_script_commands.h"')
    out.append('#include "constants/battle_anim.h"')
    out.append('#include "constants/items.h"')
    out.append('#include "constants/abilities.h"')
    out.append('#include "constants/species.h"')
    out.append('#include "constants/pokemon.h"')
    out.append('#include "constants/songs.h"')
    out.append('#include "constants/trainers.h"')
    out.append('#include "constants/game_stat.h"')
    out.append('#include "constants/battle_string_ids.h"')
    out.append('')
    out.append('#define U16_LO(x) ((u8)((u16)(x) & 0xFF))')
    out.append('#define U16_HI(x) ((u8)(((u16)(x) >> 8) & 0xFF))')
    out.append('#define U32_B0(x) ((u8)((u32)(x) & 0xFF))')
    out.append('#define U32_B1(x) ((u8)(((u32)(x) >> 8) & 0xFF))')
    out.append('#define U32_B2(x) ((u8)(((u32)(x) >> 16) & 0xFF))')
    out.append('#define U32_B3(x) ((u8)(((u32)(x) >> 24) & 0xFF))')
    out.append('#define FIRERED_BATTLE_PTR_TOKEN_BASE 0x80000000u')
    out.append('')
    all_labels = set(labels)
    for entries in labels.values():
        for entry in entries:
            if isinstance(entry, tuple) and entry[0] == 'label':
                all_labels.add(entry[1])
    for label in sorted(all_labels):
        if label in POINTER_TABLE_LABELS:
            out.append(f'extern const u8 *const {label}[];')
        else:
            out.append(f'extern const u8 {label}[];')
    out.append('')

    alias_blocks = []
    for label, entries in labels.items():
        if label in POINTER_TABLE_LABELS:
            ptr_entries = []
            for op in entries:
                if op.startswith('.4byte'):
                    ptr_entries.extend(split_args(op.split(None, 1)[1]))
            out.append(f'const u8 *const {label}[] = {{')
            for entry in ptr_entries:
                out.append(f'    {entry},')
            out.append('};')
            out.append('')
            continue

        items = []
        size = 0
        alias_offsets = {}
        for entry in entries:
            if isinstance(entry, tuple) and entry[0] == 'label':
                alias_offsets[entry[1]] = size
                continue
            if not isinstance(entry, str):
                raise ValueError(f"unsupported entry: {entry}")
            op = entry
            if op.startswith('.align'):
                align = 1 << int(op.split(None, 1)[1], 0)
                while size % align != 0:
                    items.append('0')
                    size += 1
            elif op.startswith('.byte'):
                for arg in split_args(op.split(None, 1)[1]):
                    items.append(f'((u8)({arg}))')
                    size += 1
            elif op.startswith('.2byte'):
                for arg in split_args(op.split(None, 1)[1]):
                    items.extend(emit_u16(arg))
                    size += 2
            elif op.startswith('.4byte'):
                for arg in split_args(op.split(None, 1)[1]):
                    items.extend(emit_u32(token_for(arg) if is_pointer_expr(arg) else arg))
                    size += 4
            else:
                raise ValueError(f"unsupported op: {op}")

        out.append(f'const u8 {label}[] __attribute__((aligned(4))) = {{')
        for item in items:
            out.append(f'    {item},')
        out.append('};')
        out.append('')
        for alias, offset in alias_offsets.items():
            alias_blocks.append(f'__asm__(".globl {alias}\\n.set {alias}, {label} + {offset}");')

    external_ptr_symbols = set()
    simple_ptr_re = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)(\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+))?$")
    needs_battle_scripting = False
    needs_battle_communication = False
    for expr in token_order:
        match = simple_ptr_re.fullmatch(expr.strip())
        if not match:
            continue
        symbol = match.group(1)
        if symbol.startswith('s'):
            needs_battle_scripting = True
            continue
        if symbol.startswith('c'):
            needs_battle_communication = True
            continue
        if symbol == 'gBattleScripting':
            needs_battle_scripting = True
            continue
        if symbol == 'gBattleCommunication':
            needs_battle_communication = True
            continue
        if symbol not in all_labels:
            external_ptr_symbols.add(symbol)
    if needs_battle_scripting:
        out.append('extern u8 gBattleScripting[];')
    if needs_battle_communication:
        out.append('extern u8 gBattleCommunication[];')
    for symbol in sorted(external_ptr_symbols):
        out.append(f'extern u8 {symbol};')
    if needs_battle_scripting or needs_battle_communication or external_ptr_symbols:
        out.append('')

    out.append('const void *const gFireredPortableBattleScriptPtrs[] = {')
    for expr in token_order:
        out.append(f'    {c_ptr_expr(expr, all_labels)},')
    out.append('};')
    out.append('const u32 gFireredPortableBattleScriptPtrCount = (u32)(sizeof(gFireredPortableBattleScriptPtrs) / sizeof(gFireredPortableBattleScriptPtrs[0]));')
    out.append('')
    out.extend(alias_blocks)
    if alias_blocks:
        out.append('')
    return '\n'.join(out) + '\n'


def main(argv):
    if len(argv) != 3:
        raise SystemExit('usage: generate_portable_battle_script_data.py <root_dir> <generated_dir>')
    root_dir = os.path.abspath(argv[1])
    generated_dir = os.path.abspath(argv[2])
    macros = load_macros(os.path.join(root_dir, 'asm', 'macros', 'battle_script.inc'))
    labels = {}
    for src_file in ('battle_scripts_1.s', 'battle_scripts_2.s'):
        file_labels, file_order, file_last_macro = load_labels(
            os.path.join(root_dir, 'data', src_file), macros)
        add_fallthroughs(file_labels, file_order, file_last_macro)
        labels.update(file_labels)
    out_path = os.path.join(generated_dir, 'src', 'data', 'battle_scripts_portable_data.c')
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, 'w', encoding='utf-8', newline='\n') as handle:
        handle.write(generate_source(labels))


if __name__ == '__main__':
    main(sys.argv)
