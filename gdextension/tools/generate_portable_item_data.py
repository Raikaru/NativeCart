#!/usr/bin/env python3

import json
from pathlib import Path
import sys


# Source-of-truth input: src/data/items.json
# Emitted output: portable gItems data and local item descriptions


def write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        old_text = path.read_text(encoding='utf-8')
        if old_text == text:
            return
    path.write_text(text, encoding='utf-8', newline='\n')


def c_string(value):
    return json.dumps(value, ensure_ascii=False).replace('\\n', '\n')


def load_items(path):
    return json.loads(path.read_text(encoding='utf-8'))['items']


def generate_source(items):
    funcs = set()
    move_descs = set()
    out = [
        '#include "global.h"',
        '#include "item.h"',
        '#include "constants/hold_effects.h"',
        '#include "constants/items.h"',
        '',
    ]

    for item in items:
        for key in ('fieldUseFunc', 'battleUseFunc'):
            value = item[key]
            if value != 'NULL':
                funcs.add(value)
        if item['pocket'] == 'POCKET_TM_CASE':
            move_descs.add(item['moveId'])

    for move_id in sorted(move_descs):
        out.append(f'extern const u8 gMoveDescription_{move_id}[];')
    for func in sorted(funcs):
        out.append(f'void {func}(u8);')
    if move_descs or funcs:
        out.append('')

    for item in items:
        if item['itemId'] == 'ITEM_NONE':
            continue
        out.append(f'const u8 gItemDescription_{item["itemId"]}[] = _({c_string(item["description_english"])});')
    out.append('const u8 gItemDescription_ITEM_NONE[] = _("?????");')
    out.append('')
    out.append('const struct Item gItems[] = {')
    for item in items:
        if item['pocket'] == 'POCKET_TM_CASE':
            description = f'gMoveDescription_{item["moveId"]}'
        else:
            description = f'gItemDescription_{item["itemId"]}'
        out.extend([
            '    {',
            f'        .name = _({c_string(item["english"])}),',
            f'        .itemId = {item["itemId"]},',
            f'        .price = {item["price"]},',
            f'        .holdEffect = {item["holdEffect"]},',
            f'        .holdEffectParam = {item["holdEffectParam"]},',
            f'        .description = {description},',
            f'        .importance = {item["importance"]},',
            f'        .registrability = {item["registrability"]},',
            f'        .pocket = {item["pocket"]},',
            f'        .type = {item["type"]},',
            f'        .fieldUseFunc = {item["fieldUseFunc"]},',
            f'        .battleUsage = {item["battleUsage"]},',
            f'        .battleUseFunc = {item["battleUseFunc"]},',
            f'        .secondaryId = {item["secondaryId"]},',
            '    },',
        ])
    out.append('};')
    out.append('')
    return '\n'.join(out)


def main(argv):
    if len(argv) != 3:
        raise SystemExit('usage: generate_portable_item_data.py <root_dir> <generated_dir>')

    root_dir = Path(argv[1]).resolve()
    generated_dir = Path(argv[2]).resolve()
    items = load_items(root_dir / 'src' / 'data' / 'items.json')
    out_path = generated_dir / 'src' / 'data' / 'items_portable_data.c'
    write_if_changed(out_path, generate_source(items))


if __name__ == '__main__':
    main(sys.argv)
