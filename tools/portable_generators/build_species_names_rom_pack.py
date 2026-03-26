#!/usr/bin/env python3
"""Emit FIRERED_ROM_SPECIES_NAMES_PACK_OFF blob from src_transformed/species_names_portable.h (gSpeciesNames_Compiled)."""

import re
import struct
import sys
from pathlib import Path

POKEMON_NAME_LENGTH = 10
STRIDE = POKEMON_NAME_LENGTH + 1


def main() -> int:
    if len(sys.argv) != 3:
        print('usage: build_species_names_rom_pack.py <repo_root> <out.bin>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    out = Path(sys.argv[2]).resolve()
    text = (root / 'src_transformed' / 'species_names_portable.h').read_text(encoding='utf-8')
    rows = []
    for m in re.finditer(r'\[SPECIES_[^\]]+\]\s*=\s*\{([^}]+)\}', text):
        nums = [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]
        if len(nums) != STRIDE:
            print(f'bad row len {len(nums)} (want {STRIDE})', file=sys.stderr)
            return 1
        rows.append(nums)
    flat = [b for row in rows for b in row]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(struct.pack('<' + 'B' * len(flat), *flat))
    print(f'Wrote {out.stat().st_size} bytes ({len(rows)} rows) to {out}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
