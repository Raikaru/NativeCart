#!/usr/bin/env python3

import os
import re
import sys


HEADER_RE = re.compile(r'^\s*#include\s+"([^"]+)"\s*$')
TOKEN_RE = re.compile(r'\b[A-Z][A-Z0-9_]+\b')
MANUAL_TOKENS = {"NULL", "FALSE", "TRUE"}
GLOBAL_LABEL_RE = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*)::\s*$')


def strip_comment(line):
    if "@" in line:
        line = line.split("@", 1)[0]
    return line.rstrip()


def escape_c_string(line):
    return line.replace("\\", "\\\\").replace('"', '\\"')


def parse_asm_source(source_path):
    includes = []
    asm_lines = []
    tokens = set()

    with open(source_path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.rstrip("\n")
            match = HEADER_RE.match(line)
            if match:
                includes.append(match.group(1))
                continue

            stripped = strip_comment(line)

            if stripped == '\t.include "asm/macros/battle_script.inc"':
                stripped = '\t.include "pokefirered_core/generated/src/data/battle_script_macros_portable.inc"'
            elif stripped == '\t.section script_data, "aw", %progbits':
                stripped = '\t.section .rdata,"dr"'

            label_match = GLOBAL_LABEL_RE.match(stripped)
            if label_match:
                name = label_match.group(1)
                asm_lines.append(f"\t.global {name}")
                asm_lines.append(f"{name}:")
                continue

            asm_lines.append(stripped)
            for token in TOKEN_RE.findall(stripped):
                if token not in MANUAL_TOKENS:
                    tokens.add(token)

    return includes, asm_lines, sorted(tokens)


def write_wrapper(source_path, target_path):
    includes, asm_lines, tokens = parse_asm_source(source_path)

    lines = []
    lines.append("#include \"global.h\"")
    for include in includes:
        lines.append(f'#include "{include}"')
    lines.append("")
    lines.append("#define ASM_STR2(x) #x")
    lines.append("#define ASM_STR(x) ASM_STR2(x)")
    lines.append("")
    lines.append("asm(")
    lines.append('".set NULL, 0\\n"')
    lines.append('".set FALSE, 0\\n"')
    lines.append('".set TRUE, 1\\n"')
    for token in tokens:
        lines.append(f"#ifdef {token}")
        lines.append(f'".set {token}, " ASM_STR({token}) "\\n"')
        lines.append("#endif")
    for asm_line in asm_lines:
        lines.append(f'"{escape_c_string(asm_line)}\\n"')
    lines.append(");")
    lines.append("")

    os.makedirs(os.path.dirname(target_path), exist_ok=True)
    with open(target_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))


def write_macro_include(source_path, target_path):
    os.makedirs(os.path.dirname(target_path), exist_ok=True)
    with open(source_path, "r", encoding="utf-8") as src:
        lines = [strip_comment(line.rstrip("\n")) for line in src]
    with open(target_path, "w", encoding="utf-8", newline="\n") as dst:
        dst.write("\n".join(lines) + "\n")


def main(argv):
    if len(argv) != 3:
        raise SystemExit("usage: generate_portable_script_asm_wrappers.py <root_dir> <generated_dir>")

    root_dir = os.path.abspath(argv[1])
    generated_dir = os.path.abspath(argv[2])
    generated_src_dir = os.path.join(generated_dir, "src", "data")

    write_macro_include(
        os.path.join(root_dir, "asm", "macros", "battle_script.inc"),
        os.path.join(generated_src_dir, "battle_script_macros_portable.inc"),
    )

    write_wrapper(
        os.path.join(root_dir, "data", "battle_scripts_1.s"),
        os.path.join(generated_src_dir, "battle_scripts_1_portable.c"),
    )
    write_wrapper(
        os.path.join(root_dir, "data", "battle_scripts_2.s"),
        os.path.join(generated_src_dir, "battle_scripts_2_portable.c"),
    )


if __name__ == "__main__":
    main(sys.argv)
