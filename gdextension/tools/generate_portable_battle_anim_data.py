#!/usr/bin/env python3

from pathlib import Path
import re
import sys


# Source-of-truth inputs:
# - asm/macros/battle_anim_script.inc
# - data/battle_anim_scripts.s
# Emitted output:
# - portable battle animation blobs, pointer tables, and token resolver table


MACRO_START_RE = re.compile(r"^\s*\.macro\s+([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$")
MACRO_END_RE = re.compile(r"^\s*\.endm\s*$")
LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)::\s*$")
INLINE_LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*:\s*$")
HEADER_ANY_INCLUDE_RE = re.compile(r'^\s*#include\s+["<]([^">]+)[">]\s*$')
IDENT_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
POINTER_TABLE_LABELS = {
    "gBattleAnims_Moves",
    "gBattleAnims_StatusConditions",
    "gBattleAnims_General",
    "gBattleAnims_Special",
}
POINTER_PREFIXES = (
    "Move_",
    "Status_",
    "General_",
    "Special_",
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


def write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        old_text = path.read_text(encoding='utf-8')
        if old_text == text:
            return
    path.write_text(text, encoding='utf-8', newline='\n')


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
        is_vararg = ':vararg' in item
        if '=' in item:
            name, default = item.split('=', 1)
            name = name.split(':', 1)[0].strip()
            params.append(name)
            defaults[name] = default.strip()
        else:
            name = item.split(':', 1)[0].strip()
            params.append(name)
            if is_vararg:
                defaults[name] = ''
    return params, defaults


def load_macros(path):
    macros = {}
    lines = path.read_text(encoding='utf-8').splitlines()
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
            body.append(strip_comment(lines[index]))
            index += 1
        macros[name] = Macro(params, defaults, body)
        index += 1
    return macros


def substitute(body_line, mapping):
    result = body_line
    for name, value in mapping.items():
        result = result.replace(f"\\{name}", value)
    return result


def eval_macro_if(expr):
    expr = expr.strip()
    if '==' in expr:
        left, right = expr.split('==', 1)
        return left.strip() == right.strip()
    if '!=' in expr:
        left, right = expr.split('!=', 1)
        return left.strip() != right.strip()
    return False


def expand_line(line, macros):
    stripped = line.strip()
    if not stripped:
        return []
    token = stripped.split(None, 1)[0]
    raw_args = stripped[len(token):].strip()

    if token == 'createsprite':
        args = split_args(raw_args)
        template = args[0]
        anim_battler = args[1]
        subpriority_offset = args[2]
        argv = args[3:]
        battler_byte = f'ANIMSPRITE_IS_TARGET | ({subpriority_offset} & 0x7F)' if anim_battler.strip() == 'ANIM_TARGET' else f'({subpriority_offset} & 0x7F)'
        lines = ['.byte 0x02', f'.4byte {template}', f'.byte {battler_byte}', f'.byte {len(argv)}']
        for arg in argv:
            lines.append(f'.2byte {arg}')
        return lines

    if token == 'createvisualtask':
        args = split_args(raw_args)
        addr = args[0]
        priority = args[1]
        argv = args[2:]
        lines = ['.byte 0x03', f'.4byte {addr}', f'.byte {priority}', f'.byte {len(argv)}']
        for arg in argv:
            lines.append(f'.2byte {arg}')
        return lines

    if token == 'createsoundtask':
        args = split_args(raw_args)
        addr = args[0]
        argv = args[1:]
        lines = ['.byte 0x1F', f'.4byte {addr}', f'.byte {len(argv)}']
        for arg in argv:
            lines.append(f'.2byte {arg}')
        return lines

    if token not in macros:
        return [stripped]
    macro = macros[token]
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
            raise ValueError(f"macro {token}: missing arg for parameter {name}")
    expanded = []
    active_stack = [True]
    for body_line in macro.body:
        substituted = substitute(body_line, mapping).strip()
        if substituted.startswith('.if '):
            active_stack.append(active_stack[-1] and eval_macro_if(substituted[4:]))
            continue
        if substituted == '.else':
            prev = active_stack.pop()
            active_stack.append(active_stack[-1] and not prev)
            continue
        if substituted == '.endif':
            active_stack.pop()
            continue
        if all(active_stack):
            expanded.extend(expand_line(substituted, macros))
    return expanded


def load_labels(path, macros):
    labels = {}
    current = None
    for line_number, raw_line in enumerate(path.read_text(encoding='utf-8').splitlines(), start=1):
        try:
            line = strip_comment(raw_line)
            if not line.strip():
                continue
            stripped = line.lstrip()
            if stripped.startswith('#include') or stripped.startswith('.include') or stripped.startswith('.section'):
                continue
            match = LABEL_RE.match(line)
            if match:
                current = match.group(1)
                labels[current] = []
                continue
            inline_match = INLINE_LABEL_RE.match(line)
            if inline_match:
                name = inline_match.group(1)
                if current is None or current in POINTER_TABLE_LABELS or current == 'gMovesWithQuietBGM':
                    current = name
                    labels[current] = []
                else:
                    labels[current].append(("label", name))
                continue
            if current is None:
                continue
            labels[current].extend(expand_line(line, macros))
        except Exception as exc:
            raise ValueError(f'{path.as_posix()}:{line_number}: {exc}') from exc
    return labels


def load_declared_names(root_dir, include_names):
    declared_names = set()
    visited = set()
    decl_re = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*(?:\([^;{}]*\)|\[[^;{}]*\])\s*;")
    object_re = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*;")

    def visit(include_name):
        header_path = (root_dir / include_name)
        if not header_path.exists():
            header_path = root_dir / 'include' / include_name
        if not header_path.exists() or header_path in visited:
            return
        visited.add(header_path)
        lines = header_path.read_text(encoding='utf-8').splitlines()
        for raw_line in lines:
            match = HEADER_ANY_INCLUDE_RE.match(raw_line.strip())
            if match:
                visit(match.group(1))
        for raw_line in lines:
            line = raw_line.split('//', 1)[0]
            if '(' in line or line.strip().startswith('extern'):
                for match in decl_re.finditer(line):
                    declared_names.add(match.group(1))
            stripped = line.strip()
            if stripped.startswith('extern') and '(' not in stripped:
                match = object_re.search(stripped)
                if match:
                    declared_names.add(match.group(1))

    for include_name in include_names:
        visit(include_name)
    return declared_names


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


def generate_source(labels, declared_names):
    token_order = []
    token_values = {}

    def token_for(expr):
        if expr not in token_values:
            token_values[expr] = f"(FIRERED_BATTLE_ANIM_PTR_TOKEN_BASE + {len(token_order)}u)"
            token_order.append(expr)
        return token_values[expr]

    out = []
    out.append('#include "global.h"')
    out.append('#include "battle_anim.h"')
    out.append('#include "data.h"')
    out.append('#include "constants/battle.h"')
    out.append('#include "constants/battle_anim.h"')
    out.append('#include "constants/battle_string_ids.h"')
    out.append('#include "constants/rgb.h"')
    out.append('#include "constants/songs.h"')
    out.append('#include "constants/sound.h"')
    out.append('#include "constants/moves.h"')
    out.append('')
    out.append('#define U16_LO(x) ((u8)((u16)(x) & 0xFF))')
    out.append('#define U16_HI(x) ((u8)(((u16)(x) >> 8) & 0xFF))')
    out.append('#define U32_B0(x) ((u8)((u32)(x) & 0xFF))')
    out.append('#define U32_B1(x) ((u8)(((u32)(x) >> 8) & 0xFF))')
    out.append('#define U32_B2(x) ((u8)(((u32)(x) >> 16) & 0xFF))')
    out.append('#define U32_B3(x) ((u8)(((u32)(x) >> 24) & 0xFF))')
    out.append('#define FIRERED_BATTLE_ANIM_PTR_TOKEN_BASE 0x84000000u')
    out.append('')

    all_labels = set(labels)
    for entries in labels.values():
        for entry in entries:
            if isinstance(entry, tuple) and entry[0] == 'label':
                all_labels.add(entry[1])

    simple_ptr_re = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)(\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+))?$")

    pointer_table_external_symbols = set()

    for label in POINTER_TABLE_LABELS:
        if label not in labels:
            continue
        for op in labels[label]:
            if isinstance(op, str) and op.startswith('.4byte'):
                for arg in split_args(op.split(None, 1)[1]):
                    match = simple_ptr_re.fullmatch(arg.strip())
                    if not match:
                        continue
                    symbol = match.group(1)
                    if symbol not in all_labels and symbol not in declared_names:
                        pointer_table_external_symbols.add(symbol)

    for label in sorted(all_labels):
        if label in declared_names:
            continue
        out.append(f'extern const u8 {label}[];')
    for symbol in sorted(pointer_table_external_symbols):
        out.append(f'extern u8 {symbol};')
    out.append('')

    alias_blocks = []
    for label, entries in labels.items():
        if label in POINTER_TABLE_LABELS:
            ptr_entries = []
            for op in entries:
                if isinstance(op, str) and op.startswith('.4byte'):
                    ptr_entries.extend(split_args(op.split(None, 1)[1]))
            out.append(f'const u8 *const {label}[] = {{')
            for entry in ptr_entries:
                out.append(f'    (const u8 *){c_ptr_expr(entry, all_labels)},')
            out.append('};')
            out.append('')
            continue

        if label == 'gMovesWithQuietBGM':
            values = []
            for op in entries:
                if isinstance(op, str) and op.startswith('.2byte'):
                    values.extend(split_args(op.split(None, 1)[1]))
            out.append('const u16 gMovesWithQuietBGM[] = {')
            for value in values:
                out.append(f'    {value},')
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

    def collect_external(expr):
        match = simple_ptr_re.fullmatch(expr.strip())
        if not match:
            return
        symbol = match.group(1)
        if symbol not in all_labels and symbol not in declared_names:
            external_ptr_symbols.add(symbol)

    for expr in token_order:
        collect_external(expr)
    for symbol in sorted(external_ptr_symbols):
        out.append(f'extern u8 {symbol};')
    if external_ptr_symbols:
        out.append('')

    out.append('const void *const gFireredPortableBattleAnimPtrs[] = {')
    for expr in token_order:
        out.append(f'    {c_ptr_expr(expr, all_labels)},')
    out.append('};')
    out.append('const u32 gFireredPortableBattleAnimPtrCount = (u32)(sizeof(gFireredPortableBattleAnimPtrs) / sizeof(gFireredPortableBattleAnimPtrs[0]));')
    out.append('')
    out.extend(alias_blocks)
    if alias_blocks:
        out.append('')
    return '\n'.join(out) + '\n'


def main(argv):
    if len(argv) != 3:
        raise SystemExit('usage: generate_portable_battle_anim_data.py <root_dir> <generated_dir>')
    root_dir = Path(argv[1]).resolve()
    generated_dir = Path(argv[2]).resolve()
    macros = load_macros(root_dir / 'asm' / 'macros' / 'battle_anim_script.inc')
    labels = load_labels(root_dir / 'data' / 'battle_anim_scripts.s', macros)
    declared_names = load_declared_names(root_dir, ['battle_anim.h', 'data.h'])
    out_path = generated_dir / 'src' / 'data' / 'battle_anim_scripts_portable_data.c'
    write_if_changed(out_path, generate_source(labels, declared_names))


if __name__ == '__main__':
    main(sys.argv)
