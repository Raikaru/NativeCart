#!/usr/bin/env python3
"""ROM audit: vanilla early-ng control needles — all hits + 32-bit ROM pointer ref counts."""
from __future__ import annotations

import importlib.util
from pathlib import Path


def _load_find_mod(repo: Path):
    spec = importlib.util.spec_from_file_location(
        "find_early", repo / "tools/find_early_new_game_help_text_in_rom.py"
    )
    m = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(m)
    return m


def _find_all(hay: bytes, needle: bytes) -> list[int]:
    out: list[int] = []
    i = 0
    while True:
        j = hay.find(needle, i)
        if j < 0:
            break
        out.append(j)
        i = j + 1
    return out


def _ptr_pat(rom_off: int) -> bytes:
    return (0x08000000 + rom_off).to_bytes(4, "little")


def _count_ptr(rom: bytes, rom_off: int) -> int:
    pat = _ptr_pat(rom_off)
    return rom.count(pat)


def audit_rom(repo: Path, label: str, rom: bytes) -> None:
    m = _load_find_mod(repo)
    charmap = m.load_charmap(repo / "charmap.txt")

    def needle_for(short: str) -> bytes:
        for name, parts in m.EARLY_NG_LABELS:
            if name == short:
                return m.concat_with_eos(parts, charmap)
        raise KeyError(short)

    controls = ["Intro", "DPad", "AButton", "BButton", "StartButton", "SelectButton", "LRButtons"]
    print(f"{label} size=0x{len(rom):X}\n")
    for name in controls:
        nd = needle_for(name)
        hits = _find_all(rom, nd)
        print(f"{name}: needle_len={len(nd)} hits={len(hits)}")
        for h in hits[:12]:
            print(f"  off=0x{h:08X} ptr32_refs={_count_ptr(rom, h)}")
        if len(hits) > 12:
            print(f"  ... +{len(hits) - 12} more hits")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    for path in (repo / "build/omega_layout_test.gba", repo / "Pokemon Fire Red Omega.gba"):
        if not path.is_file():
            print(f"skip missing {path}\n")
            continue
        rom = path.read_bytes()
        audit_rom(repo, str(path), rom)
        print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
