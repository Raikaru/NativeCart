#!/usr/bin/env python3

from pathlib import Path
import re
import sys


# Source-of-truth input: sound/cry_tables.inc
# Emitted output: portable gCryTable / gCryTable_Reverse data


def write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        old_text = path.read_text(encoding='utf-8')
        if old_text == text:
            return
    path.write_text(text, encoding='utf-8', newline='\n')


LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)::\s*$")
ENTRY_RE = re.compile(r"^\s*(cry|cry_reverse)\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")


def load_tables(path):
    tables = {}
    current = None

    for line_number, raw_line in enumerate(path.read_text(encoding='utf-8').splitlines(), start=1):
        line = raw_line.split('@', 1)[0].strip()
        try:
            if not line or line.startswith('.align'):
                continue

            match = LABEL_RE.match(line)
            if match:
                current = match.group(1)
                tables[current] = []
                continue

            match = ENTRY_RE.match(line)
            if match and current is not None:
                tables[current].append(match.group(1))
        except Exception as exc:
            raise ValueError(f'{path.as_posix()}:{line_number}: {exc}') from exc

    for name in ('gCryTable', 'gCryTable_Reverse'):
        if name not in tables:
            raise ValueError(f'{path.as_posix()}: missing table {name}')

    return tables


def emit_table(name, entries):
    type_value = '0x20' if name == 'gCryTable' else '0x30'
    out = [f'struct ToneData {name}[] = {{']
    for _ in entries:
        out.extend([
            '    {',
            f'        {type_value},',
            '        60,',
            '        0,',
            '        0,',
            '        NULL,',
            '        0xFF,',
            '        0,',
            '        0xFF,',
            '        0,',
            '    },',
        ])
    out.append('};')
    out.append('')
    return out


def generate_source(tables):
    out = [
        '#include "global.h"',
        '#include "gba/m4a_internal.h"',
        '',
    ]

    for name in ('gCryTable', 'gCryTable_Reverse'):
        out.extend(emit_table(name, tables[name]))

    return '\n'.join(out) + '\n'


def main(argv):
    if len(argv) != 3:
        raise SystemExit('usage: generate_portable_cry_data.py <root_dir> <generated_dir>')

    root_dir = Path(argv[1]).resolve()
    generated_dir = Path(argv[2]).resolve()
    tables = load_tables(root_dir / 'sound' / 'cry_tables.inc')
    out_path = generated_dir / 'src' / 'data' / 'cry_tables_portable_data.c'
    write_if_changed(out_path, generate_source(tables))


if __name__ == '__main__':
    main(sys.argv)
