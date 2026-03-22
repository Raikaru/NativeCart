#!/usr/bin/env python3

import ast
import json
import os
import re
import sys


MACRO_START_RE = re.compile(r"^\s*\.macro\s+([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$")
MACRO_END_RE = re.compile(r"^\s*\.endm\s*$")
GLOBAL_LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)::\s*$")
LOCAL_LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*):\s*$")
IDENT_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
HEADER_RE = re.compile(r'^\s*#include\s+"([^"]+)"\s*$')
HEADER_ANY_INCLUDE_RE = re.compile(r'^\s*#include\s+["<]([^">]+)[">]\s*$')
ASM_INCLUDE_RE = re.compile(r'^\s*\.include\s+"([^"]+)"\s*$')
SET_RE = re.compile(r"^\s*\.(?:set|equ)\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*(.+?)\s*$")
ASSIGN_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+?)\s*$")
CPP_IFDEF_RE = re.compile(r"^\s*#ifdef\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")
CPP_IFNDEF_RE = re.compile(r"^\s*#ifndef\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")
CPP_IF_RE = re.compile(r"^\s*#if\s+(.+?)\s*$")
CPP_ELSE_RE = re.compile(r"^\s*#else\s*$")
CPP_ENDIF_RE = re.compile(r"^\s*#endif\s*$")
IFDEF_RE = re.compile(r"^\s*\.ifdef\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")
IFNDEF_RE = re.compile(r"^\s*\.ifndef\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")
IF_RE = re.compile(r"^\s*\.if\s+(.+?)\s*$")
ELSEIF_RE = re.compile(r"^\s*\.elseif\s+(.+?)\s*$")
ELSE_RE = re.compile(r"^\s*\.else\s*$")
ENDIF_RE = re.compile(r"^\s*\.endif\s*$")
IFB_RE = re.compile(r"^\s*\.ifb\s*(.*?)\s*$")
IFNB_RE = re.compile(r"^\s*\.ifnb\s*(.*?)\s*$")
POINTER_TABLE_TYPES = {
    "gSpecialVars": "u16 *const",
    "gSpecials": "u16 (*const)(void)",
    "gStdScripts": "const u8 *const",
    "gFieldEffectScriptPointers": "const u8 *const",
}
PTR_BASE_EVENT = 0x81000000
PTR_BASE_FIELD_EFFECT = 0x82000000
MANUAL_TOKENS = {"NULL", "FALSE", "TRUE"}
BRAILLE_ENCODING = {
    "A": 0x01,
    "B": 0x05,
    "C": 0x03,
    "D": 0x0B,
    "E": 0x09,
    "F": 0x07,
    "G": 0x0F,
    "H": 0x0D,
    "I": 0x06,
    "J": 0x0E,
    "K": 0x11,
    "L": 0x15,
    "M": 0x13,
    "N": 0x1B,
    "O": 0x19,
    "P": 0x17,
    "Q": 0x1F,
    "R": 0x1D,
    "S": 0x16,
    "T": 0x1E,
    "U": 0x31,
    "V": 0x35,
    "W": 0x2E,
    "X": 0x33,
    "Y": 0x3B,
    "Z": 0x39,
    "a": 0x01,
    "b": 0x05,
    "c": 0x03,
    "d": 0x0B,
    "e": 0x09,
    "f": 0x07,
    "g": 0x0F,
    "h": 0x0D,
    "i": 0x06,
    "j": 0x0E,
    "k": 0x11,
    "l": 0x15,
    "m": 0x13,
    "n": 0x1B,
    "o": 0x19,
    "p": 0x17,
    "q": 0x1F,
    "r": 0x1D,
    "s": 0x16,
    "t": 0x1E,
    "u": 0x31,
    "v": 0x35,
    "w": 0x2E,
    "x": 0x33,
    "y": 0x3B,
    "z": 0x39,
    "0": 0x0E,
    "1": 0x01,
    "2": 0x05,
    "3": 0x03,
    "4": 0x0B,
    "5": 0x09,
    "6": 0x07,
    "7": 0x0F,
    "8": 0x0D,
    "9": 0x06,
    " ": 0x00,
    ",": 0x04,
    ".": 0x2C,
    "?": 0x34,
    "!": 0x1C,
    ":": 0x0C,
    ";": 0x14,
    "-": 0x30,
    "/": 0x12,
    "(": 0x3C,
    ")": 0x3C,
    "'": 0x10,
    "$": 0xFF,
}


class Macro:
    def __init__(self, params, defaults, body):
        self.params = params
        self.defaults = defaults
        self.body = body


class Charmap:
    def __init__(self):
        self.char_map = {}
        self.escape_map = {}
        self.constants = {}


def strip_comment(line):
    result = []
    quote = None
    escape = False
    for ch in line:
        if escape:
            result.append(ch)
            escape = False
            continue
        if ch == "\\":
            result.append(ch)
            escape = True
            continue
        if quote:
            result.append(ch)
            if ch == quote:
                quote = None
            continue
        if ch in ('"', "'"):
            quote = ch
            result.append(ch)
            continue
        if ch == "@":
            break
        result.append(ch)
    return "".join(result).rstrip()


def split_args(text):
    args = []
    current = []
    depth = 0
    quote = None
    escape = False
    for ch in text:
        if escape:
            current.append(ch)
            escape = False
            continue
        if ch == "\\":
            current.append(ch)
            escape = True
            continue
        if quote:
            current.append(ch)
            if ch == quote:
                quote = None
            continue
        if ch in ('"', "'"):
            current.append(ch)
            quote = ch
            continue
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == "," and depth == 0:
            args.append("".join(current).strip())
            current = []
            continue
        current.append(ch)
    tail = "".join(current).strip()
    if tail:
        args.append(tail)
    return args


def parse_macro_params(text):
    params = []
    defaults = {}
    if not text.strip():
        return params, defaults
    for item in split_args(text):
        if "=" in item:
            name, default = item.split("=", 1)
            name = name.split(":", 1)[0].strip()
            params.append(name)
            defaults[name] = default.strip()
        else:
            params.append(item.split(":", 1)[0].strip())
    return params, defaults


def replace_tokens(expr, symbols):
    def repl(match):
        name = match.group(0)
        if name in symbols:
            return str(symbols[name])
        return "0"
    return IDENT_RE.sub(repl, expr)


def eval_condition(expr, symbols):
    translated = replace_tokens(expr, symbols)
    translated = translated.replace("&&", " and ").replace("||", " or ")
    translated = re.sub(r"!(?!=)", " not ", translated)
    value = eval(translated, {"__builtins__": {}}, {})
    return bool(value)


def parse_int(text):
    text = text.strip()
    if not text:
        raise ValueError("empty integer")
    suffix = ""
    if text[-1] in ("H", "W"):
        suffix = text[-1]
        text = text[:-1]
    if text.startswith("0x") or text.startswith("0X"):
        value = int(text, 16)
    else:
        value = int(text, 10)
    return value, suffix


def parse_charmap_lhs(lhs):
    lhs = lhs.strip()
    if lhs.startswith("'"):
        if len(lhs) < 3 or not lhs.endswith("'"):
            raise ValueError(f"unsupported char literal: {lhs}")
        inner = lhs[1:-1]
        if inner.startswith("\\"):
            if len(inner) != 2:
                raise ValueError(f"unsupported escaped char literal: {lhs}")
            value = inner[1]
            if value not in ("'", "\\"):
                return ("escape", value)
            return ("char", value)
        if len(inner) != 1:
            raise ValueError(f"unsupported char literal: {lhs}")
        return ("char", inner)
    return ("constant", lhs)


def load_charmap(path):
    charmap = Charmap()
    with open(path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = strip_comment(raw_line.rstrip("\n"))
            if not line.strip():
                continue
            if line.lstrip().startswith("'"):
                quote_start = line.index("'")
                pos = quote_start + 1
                escape = False
                while pos < len(line):
                    ch = line[pos]
                    if escape:
                        escape = False
                    elif ch == "\\":
                        escape = True
                    elif ch == "'":
                        pos += 1
                        break
                    pos += 1
                lhs = line[:pos]
                rhs = line[pos + line[pos:].index("=") + 1:]
            else:
                lhs, rhs = line.split("=", 1)
            lhs_type, lhs_value = parse_charmap_lhs(lhs)
            seq = bytes(int(part, 16) for part in rhs.split())
            if lhs_type == "char":
                charmap.char_map[lhs_value] = seq
            elif lhs_type == "escape":
                charmap.escape_map[lhs_value] = seq
            else:
                charmap.constants[lhs_value] = seq
    return charmap


def canonical_name(name):
    return name.replace("_", "")


def load_layout_ids(root_dir):
    path = os.path.join(root_dir, "data", "layouts", "layouts.json")
    with open(path, "r", encoding="utf-8") as handle:
        data = json.load(handle)
    layout_ids = {}
    for index, entry in enumerate(data.get("layouts", []), 1):
        if "id" in entry:
            layout_ids[entry["id"]] = index
    return layout_ids


def parse_braced_sequence(content, charmap):
    out = bytearray()
    for token in content.split():
        if token in charmap.constants:
            out.extend(charmap.constants[token])
            continue
        value, size_suffix = parse_int(token)
        if size_suffix == "H":
            out.extend((value & 0xFF, (value >> 8) & 0xFF))
        elif size_suffix == "W":
            out.extend((value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF))
        elif value <= 0xFF:
            out.append(value)
        elif value <= 0xFFFF:
            out.extend((value & 0xFF, (value >> 8) & 0xFF))
        else:
            out.extend((value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF))
    return list(out)


def parse_string_literal(text, charmap):
    out = []
    value = text.strip()
    if not (value.startswith('"') and value.endswith('"')):
        raise ValueError(f"invalid string literal: {text}")
    index = 1
    end = len(value) - 1
    while index < end:
        ch = value[index]
        if ch == "{":
            close = value.index("}", index)
            out.extend(parse_braced_sequence(value[index + 1:close], charmap))
            index = close + 1
            continue
        if ch == "\\":
            index += 1
            esc = value[index]
            if esc in ('"', "\\"):
                out.extend(charmap.char_map[esc])
            elif esc in charmap.escape_map:
                out.extend(charmap.escape_map[esc])
            else:
                raise ValueError(f"unknown string escape: \\{esc}")
            index += 1
            continue
        if ch in charmap.char_map:
            out.extend(charmap.char_map[ch])
            index += 1
            continue
        raise ValueError(f"unknown charmap character: {ch!r}")
    return out


def parse_braille_literal(text):
    out = []
    in_number = False
    value = text.strip()
    if not (value.startswith('"') and value.endswith('"')):
        raise ValueError(f"invalid braille literal: {text}")
    index = 1
    end = len(value) - 1
    while index < end:
        ch = value[index]
        if ch == "\\":
            index += 1
            esc = value[index]
            if esc == "n":
                out.append(0xFE)
                index += 1
                continue
            raise ValueError(f"unsupported braille escape: \\{esc}")
        if ch not in BRAILLE_ENCODING:
            raise ValueError(f"invalid braille character: {ch!r}")
        if not in_number and ch.isdigit():
            in_number = True
            out.append(0x3A)
        elif in_number and ch == " ":
            in_number = False
        out.append(BRAILLE_ENCODING[ch])
        index += 1
    return out


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
    if len(args) < len(macro.params) and "," not in raw_args:
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


def expand_line_with_conditionals(line, macros, condition_symbols):
    stripped = line.strip()
    if not stripped:
        return []
    token = stripped.split(None, 1)[0]
    if token not in macros:
        return [stripped]
    macro = macros[token]
    raw_args = stripped[len(token):].strip()
    args = split_args(raw_args)
    if len(args) < len(macro.params) and "," not in raw_args:
        args = raw_args.split(None, len(macro.params) - 1)
    mapping = {}
    for i, name in enumerate(macro.params):
        if i < len(args):
            mapping[name] = args[i]
        elif name in macro.defaults:
            mapping[name] = macro.defaults[name]
        else:
            mapping[name] = ""
    active_stack = []
    taken_stack = []
    expanded = []
    for body_line in macro.body:
        substituted = substitute(body_line, mapping)
        body_stripped = substituted.strip()
        if not body_stripped:
            continue
        ifb_match = IFB_RE.match(body_stripped)
        if ifb_match:
            parent_active = all(active_stack) if active_stack else True
            cond = parent_active and (ifb_match.group(1).strip() == "")
            active_stack.append(cond)
            taken_stack.append(cond)
            continue
        ifnb_match = IFNB_RE.match(body_stripped)
        if ifnb_match:
            parent_active = all(active_stack) if active_stack else True
            cond = parent_active and (ifnb_match.group(1).strip() != "")
            active_stack.append(cond)
            taken_stack.append(cond)
            continue
        if_match = IF_RE.match(body_stripped)
        if if_match:
            parent_active = all(active_stack) if active_stack else True
            cond = parent_active and eval_condition(if_match.group(1), condition_symbols)
            active_stack.append(cond)
            taken_stack.append(cond)
            continue
        elseif_match = ELSEIF_RE.match(body_stripped)
        if elseif_match:
            parent_active = all(active_stack[:-1]) if len(active_stack) > 1 else True
            cond = parent_active and (not taken_stack[-1]) and eval_condition(elseif_match.group(1), condition_symbols)
            active_stack[-1] = cond
            taken_stack[-1] = taken_stack[-1] or cond
            continue
        if ELSE_RE.match(body_stripped):
            parent_active = all(active_stack[:-1]) if len(active_stack) > 1 else True
            cond = parent_active and (not taken_stack[-1])
            active_stack[-1] = cond
            taken_stack[-1] = True
            continue
        if ENDIF_RE.match(body_stripped):
            active_stack.pop()
            taken_stack.pop()
            continue
        if active_stack and not all(active_stack):
            continue
        expanded.extend(expand_line_with_conditionals(substituted, macros, condition_symbols))
    return expanded


def is_pointer_expr(expr):
    expr = expr.strip()
    if expr in {"0", "NULL", "FALSE", "TRUE"}:
        return False
    idents = IDENT_RE.findall(expr)
    if not idents:
        return False
    for ident in idents:
        if ident in MANUAL_TOKENS:
            continue
        if ident.startswith(("EventScript_", "Text_", "CableClub_", "Help_", "Movement_", "Common_", "TrainerTower_", "Braille_", "g", "s", "c")):
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
        ref = symbol if symbol in local_symbols or symbol.startswith(("s", "c")) else f"&{symbol}"
        if match.group(2):
            return f"((const void *)((uintptr_t){ref} {match.group(3)} {match.group(4)}))"
        return f"((const void *)(uintptr_t){ref})"
    return f"((const void *)(uintptr_t)({expr}))"


def c_function_ptr_expr(expr, local_symbols):
    expr = expr.strip()
    match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)(\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+))?", expr)
    if match:
        symbol = match.group(1)
        ref = symbol if symbol in local_symbols or symbol.startswith(("s", "c")) else f"&{symbol}"
        if match.group(2):
            return f"(u16 (*)(void))((uintptr_t){ref} {match.group(3)} {match.group(4)})"
        return f"(u16 (*)(void))(uintptr_t){ref}"
    return f"(u16 (*)(void))(uintptr_t)({expr})"


def emit_u16(expr):
    if expr.strip() == "NULL":
        expr = "0"
    return [f"U16_LO({expr})", f"U16_HI({expr})"]


def emit_u32(expr):
    if expr.strip() == "NULL":
        expr = "0"
    return [f"U32_B0({expr})", f"U32_B1({expr})", f"U32_B2({expr})", f"U32_B3({expr})"]


def pointer_alias_block(alias, target, count):
    byte_count_32 = count * 4
    byte_count_64 = count * 8
    return [
        "#if UINTPTR_MAX > 0xFFFFFFFFu",
        f'__asm__(".globl {alias}\\n.set {alias}, {target} + {byte_count_64}");',
        "#else",
        f'__asm__(".globl {alias}\\n.set {alias}, {target} + {byte_count_32}");',
        "#endif",
    ]


def parse_source(root_path, root_dir, charmap, ptr_base, pointer_table_labels, predefined_symbols, ptr_array_name, ptr_count_name, extra_includes=None):
    macros = {}
    symbol_defs = {}
    condition_symbols = dict(predefined_symbols)
    if extra_includes is None:
        extra_includes = []
    c_includes = []
    collected_lines = []
    visited = set()
    parsed_headers = set()
    declared_names = set()
    header_symbol_names = {}
    layout_ids = load_layout_ids(root_dir)
    layout_aliases = {canonical_name(name): (name, value) for name, value in layout_ids.items()}

    def load_header_symbols(include_name):
        header_path = os.path.normpath(os.path.join(root_dir, include_name))
        if not os.path.exists(header_path):
            header_path = os.path.normpath(os.path.join(root_dir, "include", include_name))
        if not os.path.exists(header_path):
            return
        if header_path in parsed_headers:
            return
        parsed_headers.add(header_path)
        define_re = re.compile(r"^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)(\([^)]*\))?\s+(.+?)\s*$")
        with open(header_path, "r", encoding="utf-8") as header:
            header_lines = header.readlines()
        for raw_header_line in header_lines:
            include_match = HEADER_ANY_INCLUDE_RE.match(raw_header_line.strip())
            if include_match:
                load_header_symbols(include_match.group(1))
            line = raw_header_line.split("//", 1)[0].strip()
            match = define_re.match(line)
            if not match or match.group(2):
                continue
            name = match.group(1)
            expr = match.group(3).strip()
            expr = re.sub(r"/\*.*?\*/", "", expr).strip()
            header_symbol_names[name] = expr
            try:
                condition_symbols[name] = int(replace_tokens(expr, condition_symbols), 0)
            except Exception:
                pass
            continue

        decl_re = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*(?:\([^;{}]*\)|\[[^;{}]*\])\s*;")
        for raw_header_line in header_lines:
            line = raw_header_line.split("//", 1)[0]
            if "(" in line or line.strip().startswith("extern"):
                for match in decl_re.finditer(line):
                    declared_names.add(match.group(1))

    def resolve_alias(name):
        if name.startswith("LAYOUT_"):
            alias = layout_aliases.get(canonical_name(name))
            if alias is not None:
                actual_name, actual_value = alias
                symbol_defs[name] = str(actual_value) if actual_name == name else actual_name
                condition_symbols[name] = actual_value
        elif name.startswith("MAP_"):
            candidates = [sym for sym in header_symbol_names if sym.startswith("MAP_")]
            target = canonical_name(name)
            for candidate in candidates:
                if canonical_name(candidate) == target:
                    symbol_defs[name] = candidate
                    try:
                        condition_symbols[name] = int(replace_tokens(header_symbol_names.get(candidate, candidate), condition_symbols), 0)
                    except Exception:
                        pass
                    break

    load_header_symbols("global.h")
    for include in extra_includes:
        load_header_symbols(include)

    def process_file(path):
        norm_path = os.path.normpath(path)
        if norm_path in visited:
            return
        visited.add(norm_path)
        with open(path, "r", encoding="utf-8") as handle:
            raw_lines = handle.readlines()
        active_stack = [True]
        index = 0
        while index < len(raw_lines):
            raw_line = raw_lines[index].rstrip("\n")
            header_match = HEADER_RE.match(raw_line)
            if header_match and all(active_stack):
                include = header_match.group(1)
                if include not in c_includes:
                    c_includes.append(include)
                load_header_symbols(include)
                index += 1
                continue
            line = strip_comment(raw_line)
            if not line.strip():
                index += 1
                continue
            ifdef_match = CPP_IFDEF_RE.match(line) or IFDEF_RE.match(line)
            if ifdef_match:
                active_stack.append(all(active_stack) and (ifdef_match.group(1) in condition_symbols))
                index += 1
                continue
            ifndef_match = CPP_IFNDEF_RE.match(line) or IFNDEF_RE.match(line)
            if ifndef_match:
                active_stack.append(all(active_stack) and (ifndef_match.group(1) not in condition_symbols))
                index += 1
                continue
            if_match = CPP_IF_RE.match(line) or IF_RE.match(line)
            if if_match:
                active_stack.append(all(active_stack) and eval_condition(if_match.group(1), condition_symbols))
                index += 1
                continue
            if CPP_ELSE_RE.match(line) or ELSE_RE.match(line):
                if len(active_stack) < 2:
                    raise ValueError(f"unexpected .else in {path}")
                parent_active = all(active_stack[:-1])
                active_stack[-1] = parent_active and not active_stack[-1]
                index += 1
                continue
            if CPP_ENDIF_RE.match(line) or ENDIF_RE.match(line):
                if len(active_stack) < 2:
                    raise ValueError(f"unexpected .endif in {path}")
                active_stack.pop()
                index += 1
                continue
            if not all(active_stack):
                index += 1
                continue
            macro_match = MACRO_START_RE.match(line)
            if macro_match:
                name = macro_match.group(1)
                params, defaults = parse_macro_params(macro_match.group(2))
                index += 1
                body = []
                while index < len(raw_lines):
                    body_line = strip_comment(raw_lines[index].rstrip("\n"))
                    if MACRO_END_RE.match(body_line):
                        break
                    if body_line.strip():
                        body.append(body_line)
                    index += 1
                macros[name] = Macro(params, defaults, body)
                index += 1
                continue
            asm_include_match = ASM_INCLUDE_RE.match(line)
            if asm_include_match:
                include_path = os.path.normpath(os.path.join(os.path.dirname(path), asm_include_match.group(1)))
                if not os.path.exists(include_path):
                    include_path = os.path.normpath(os.path.join(root_dir, asm_include_match.group(1)))
                process_file(include_path)
                index += 1
                continue
            set_match = SET_RE.match(line)
            if set_match:
                name = set_match.group(1)
                expr = set_match.group(2).strip()
                symbol_defs[name] = expr
                try:
                    condition_symbols[name] = int(replace_tokens(expr, condition_symbols), 0)
                except Exception:
                    condition_symbols[name] = 1
                index += 1
                continue
            assign_match = ASSIGN_RE.match(line)
            if assign_match:
                name = assign_match.group(1)
                expr = assign_match.group(2).strip()
                symbol_defs[name] = expr
                try:
                    condition_symbols[name] = int(replace_tokens(expr, condition_symbols), 0)
                except Exception:
                    condition_symbols[name] = 1
                index += 1
                continue
            stripped = line.strip()
            if stripped.startswith("create_movement_action "):
                args = split_args(stripped[len("create_movement_action "):])
                if len(args) == 2:
                    name = args[0].strip()
                    value = args[1].strip()
                    macros[name] = Macro([], {}, [f".byte {value}"])
                    index += 1
                    continue
            if line.lstrip().startswith(".section"):
                index += 1
                continue
            collected_lines.append(line)
            index += 1

    process_file(root_path)

    labels = {}
    current = None
    for line in collected_lines:
        global_match = GLOBAL_LABEL_RE.match(line)
        if global_match:
            current = global_match.group(1)
            labels[current] = []
            continue
        local_match = LOCAL_LABEL_RE.match(line)
        if local_match:
            name = local_match.group(1)
            if current is None:
                current = name
                labels[current] = []
            else:
                labels[current].append(("label", name))
            continue
        if current is None:
            continue
        labels[current].extend(expand_line_with_conditionals(line, macros, condition_symbols))

    token_order = []
    token_values = {}

    def token_for(expr):
        if expr not in token_values:
            token_values[expr] = f"(FIRERED_PORTABLE_PTR_TOKEN_BASE + {len(token_order)}u)"
            token_order.append(expr)
        return token_values[expr]

    all_labels = set(labels)
    for entries in labels.values():
        for entry in entries:
            if isinstance(entry, tuple) and entry[0] == "label":
                all_labels.add(entry[1])

    for entries in labels.values():
        for entry in entries:
            if isinstance(entry, tuple):
                continue
            op = str(entry)
            set_match = SET_RE.match(op)
            if not set_match:
                continue
            name = set_match.group(1)
            expr = set_match.group(2).strip()
            symbol_defs[name] = expr
            try:
                condition_symbols[name] = int(replace_tokens(expr, condition_symbols), 0)
            except Exception:
                condition_symbols[name] = 1

    for name in list(symbol_defs):
        resolve_alias(name)

    for entries in labels.values():
        for entry in entries:
            if isinstance(entry, tuple):
                continue
            for ident in IDENT_RE.findall(str(entry)):
                resolve_alias(ident)

    if "gSpecials" in labels:
        special_index = 0
        for entry in labels["gSpecials"]:
            if isinstance(entry, tuple):
                continue
            op = str(entry)
            set_match = SET_RE.match(op)
            if set_match and set_match.group(1).startswith("SPECIAL_"):
                symbol_defs[set_match.group(1)] = str(special_index)
                continue
            if op.startswith(".4byte"):
                special_index += len(split_args(op.split(None, 1)[1]))

    external_ptr_symbols = set()
    external_func_symbols = set()
    simple_ptr_re = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)(\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+))?$")

    def collect_external(expr):
        match = simple_ptr_re.fullmatch(expr.strip())
        if match:
            symbol = match.group(1)
            if symbol.startswith("g") and symbol not in all_labels and not symbol.isupper() and symbol not in symbol_defs:
                external_ptr_symbols.add(symbol)
            elif symbol not in all_labels and not symbol.isupper() and symbol not in symbol_defs:
                external_func_symbols.add(symbol)

    for label, entries in labels.items():
        for entry in entries:
            if isinstance(entry, tuple):
                continue
            op = str(entry)
            if op.startswith(".4byte"):
                for arg in split_args(op.split(None, 1)[1]):
                    if label in pointer_table_labels or is_pointer_expr(arg):
                        collect_external(arg)

    generated = []
    generated.append('#include "global.h"')
    for include in extra_includes:
        generated.append(f'#include "{include}"')
    for include in c_includes:
        generated.append(f'#include "{include}"')
    generated.append("")
    generated.append("#include <stdint.h>")
    generated.append("")
    generated.append("#define U16_LO(x) ((u8)((u16)(x) & 0xFF))")
    generated.append("#define U16_HI(x) ((u8)(((u16)(x) >> 8) & 0xFF))")
    generated.append("#define U32_B0(x) ((u8)((u32)(x) & 0xFF))")
    generated.append("#define U32_B1(x) ((u8)(((u32)(x) >> 8) & 0xFF))")
    generated.append("#define U32_B2(x) ((u8)(((u32)(x) >> 16) & 0xFF))")
    generated.append("#define U32_B3(x) ((u8)(((u32)(x) >> 24) & 0xFF))")
    generated.append(f"#define FIRERED_PORTABLE_PTR_TOKEN_BASE 0x{ptr_base:08X}u")
    generated.append("")
    for name in sorted(symbol_defs):
        if name in {"TRUE", "FALSE"} or name.startswith("__"):
            continue
        if name in header_symbol_names:
            continue
        generated.append(f"#define {name} ({symbol_defs[name]})")
    if symbol_defs:
        generated.append("")
    for label in sorted(all_labels):
        if label in pointer_table_labels:
            continue
        if label not in declared_names:
            generated.append(f"extern const u8 {label}[];")
    for symbol in sorted(external_ptr_symbols):
        if symbol in pointer_table_labels:
            continue
        if symbol in declared_names:
            continue
        if symbol.startswith("gSpecialVar_"):
            generated.append(f"extern u16 {symbol};")
        else:
            generated.append(f"extern u8 {symbol}[];")
    for symbol in sorted(external_func_symbols):
        if symbol not in declared_names:
            generated.append(f"extern void {symbol}(void);")
    generated.append("")

    alias_blocks = []
    pointer_table_counts = {}

    for label, entries in labels.items():
        if label in pointer_table_labels:
            decl = POINTER_TABLE_TYPES[label]
            pointer_exprs = []
            element_count = 0
            for entry in entries:
                if isinstance(entry, tuple) and entry[0] == "label":
                    alias_blocks.extend(pointer_alias_block(entry[1], label, element_count))
                    continue
                op = str(entry)
                if op.startswith(".align"):
                    continue
                if op.startswith(".global"):
                    continue
                set_match = SET_RE.match(op)
                if set_match:
                    name = set_match.group(1)
                    if name.startswith("SPECIAL_"):
                        symbol_defs[name] = str(element_count)
                    continue
                if not op.startswith(".4byte"):
                    raise ValueError(f"unsupported pointer-table op in {label}: {op}")
                for arg in split_args(op.split(None, 1)[1]):
                    pointer_exprs.append(arg)
                    element_count += 1
            pointer_table_counts[label] = element_count
            if label == "gSpecials":
                generated.append(f"u16 (*const {label}[])(void) = {{")
            else:
                generated.append(f"{decl} {label}[] = {{")
            for expr in pointer_exprs:
                if label == "gSpecials":
                    generated.append(f"    {c_function_ptr_expr(expr, all_labels)},")
                else:
                    generated.append(f"    {c_ptr_expr(expr, all_labels)},")
            generated.append("};")
            generated.append("")
            continue

        items = []
        size = 0
        alias_offsets = {}
        for entry in entries:
            if isinstance(entry, tuple) and entry[0] == "label":
                alias_offsets[entry[1]] = size
                continue
            op = str(entry)
            if op.startswith(".global") or op.startswith(".set"):
                continue
            if op.startswith(".align"):
                align = 1 << int(op.split(None, 1)[1], 0)
                while size % align != 0:
                    items.append("0")
                    size += 1
            elif op.startswith(".byte"):
                for arg in split_args(op.split(None, 1)[1]):
                    items.append(f"((u8)({arg}))")
                    size += 1
            elif op.startswith(".2byte"):
                for arg in split_args(op.split(None, 1)[1]):
                    items.extend(emit_u16(arg))
                    size += 2
            elif op.startswith(".4byte"):
                for arg in split_args(op.split(None, 1)[1]):
                    items.extend(emit_u32(token_for(arg) if is_pointer_expr(arg) else arg))
                    size += 4
            elif op.startswith(".string"):
                string_bytes = parse_string_literal(op.split(None, 1)[1], charmap)
                items.extend(str(byte) for byte in string_bytes)
                size += len(string_bytes)
            elif op.startswith(".braille"):
                braille_bytes = parse_braille_literal(op.split(None, 1)[1])
                items.extend(str(byte) for byte in braille_bytes)
                size += len(braille_bytes)
            else:
                raise ValueError(f"unsupported op in {label}: {op}")
        generated.append(f"const u8 {label}[] __attribute__((aligned(4))) = {{")
        for item in items:
            generated.append(f"    {item},")
        generated.append("};")
        generated.append("")
        for alias, offset in alias_offsets.items():
            alias_blocks.append(f'__asm__(".globl {alias}\\n.set {alias}, {label} + {offset}");')

    generated.append(f"const void *const {ptr_array_name}[] = {{")
    for expr in token_order:
        generated.append(f"    {c_ptr_expr(expr, all_labels)},")
    generated.append("};")
    generated.append(f"const u32 {ptr_count_name} = (u32)(sizeof({ptr_array_name}) / sizeof({ptr_array_name}[0]));")
    generated.append("")
    generated.extend(alias_blocks)
    if alias_blocks:
        generated.append("")
    return "\n".join(generated) + "\n"


def write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(content)


def main(argv):
    if len(argv) != 3:
        raise SystemExit("usage: generate_portable_event_script_data.py <root_dir> <generated_dir>")
    root_dir = os.path.abspath(argv[1])
    generated_dir = os.path.abspath(argv[2])
    charmap = load_charmap(os.path.join(root_dir, "charmap.txt"))
    predefined_symbols = {
        "FIRERED": 1,
        "REVISION": 0,
        "TRUE": 1,
        "FALSE": 0,
    }
    event_source = parse_source(
        os.path.join(root_dir, "data", "event_scripts.s"),
        root_dir,
        charmap,
        PTR_BASE_EVENT,
        {"gSpecialVars", "gSpecials", "gStdScripts"},
        predefined_symbols,
        "gFireredPortableEventScriptPtrs",
        "gFireredPortableEventScriptPtrCount",
        ["event_data.h", "item_menu.h"],
    )
    field_effect_source = parse_source(
        os.path.join(root_dir, "data", "field_effect_scripts.s"),
        root_dir,
        charmap,
        PTR_BASE_FIELD_EFFECT,
        {"gFieldEffectScriptPointers"},
        predefined_symbols,
        "gFireredPortableFieldEffectScriptPtrs",
        "gFireredPortableFieldEffectScriptPtrCount",
        [],
    )
    write_file(os.path.join(generated_dir, "src", "data", "event_scripts_portable_data.c"), event_source)
    write_file(os.path.join(generated_dir, "src", "data", "field_effect_scripts_portable_data.c"), field_effect_source)


if __name__ == "__main__":
    main(sys.argv)
