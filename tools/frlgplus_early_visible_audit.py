#!/usr/bin/env python3
"""
Offline audit: compare stock FireRed vs FRLG+ (baserom + mods/frlgPlus.bps) for
early-visible graphics that the SDL portable build mirrors via *_portable_assets.h.

Does NOT prove where hack text lives; it rules out "title looks vanilla because
portable bypasses art" when the patched ROM still embeds vanilla blobs.

Usage:
  python tools/frlgplus_early_visible_audit.py
  python tools/frlgplus_early_visible_audit.py --root C:\\path\\to\\pokefirered-master
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Reuse BPS applier from sibling tool.
from title_logo_rom_offsets import apply_bps


def check_embed(repo: Path, rom: bytes, rel: str) -> tuple[bool, int]:
    p = repo / rel
    if not p.is_file():
        return False, -1
    data = p.read_bytes()
    if len(data) < 8:
        return False, -1
    i = rom.find(data)
    return (i >= 0 and rom[i : i + len(data)] == data), i


def main() -> int:
    ap = argparse.ArgumentParser(description="FRLG+ vs stock early-visible graphics embed audit")
    ap.add_argument("--root", type=Path, default=None)
    args = ap.parse_args()
    root = args.root or Path(__file__).resolve().parent.parent

    base_path = root / "baserom.gba"
    bps_path = root / "mods" / "frlgPlus.bps"
    if not base_path.is_file():
        print("missing", base_path, file=sys.stderr)
        return 2
    if not bps_path.is_file():
        print("missing", bps_path, file=sys.stderr)
        return 2

    base = base_path.read_bytes()
    patched = apply_bps(base, bps_path.read_bytes())

    print("=== GBA header (0xA0 internal name, 0xAC game code) ===")
    print("  stock: ", base[0xA0:0xAC], base[0xAC:0xB0])
    print("  patch: ", patched[0xA0:0xAC], patched[0xAC:0xB0])

    checks = [
        # Title screen (FireRed) — same files referenced by src/graphics.c
        "graphics/title_screen/firered/game_title_logo.gbapal",
        "graphics/title_screen/firered/game_title_logo.8bpp.lz",
        "graphics/title_screen/firered/game_title_logo.bin.lz",
        "graphics/title_screen/firered/box_art_mon.4bpp.lz",
        "graphics/title_screen/firered/box_art_mon.bin.lz",
        "graphics/title_screen/firered/background.gbapal",
        "graphics/title_screen/copyright_press_start.4bpp.lz",
        "graphics/title_screen/copyright_press_start.bin.lz",
        # Oak speech (shared intro path)
        "graphics/oak_speech/oak_speech_bg.4bpp.lz",
        "graphics/oak_speech/oak_speech_bg.bin.lz",
        "graphics/oak_speech/oak/pic.8bpp.lz",
    ]

    print("\n=== Full-file embed in ROM (vanilla pokefirered graphics file == contiguous ROM bytes) ===")
    print("path | stock_ok@off | pat_ok@off")
    for rel in checks:
        ok_b, ob = check_embed(root, base, rel)
        ok_p, op = check_embed(root, patched, rel)
        print(
            "%s\n  stock %s @ 0x%X\n  pat   %s @ 0x%X"
            % (rel, ok_b, ob if ob >= 0 else 0, ok_p, op if op >= 0 else 0)
        )

    print(
        "\nInterpretation: if both ROMs embed the same vanilla file bytes, changing the SDL\n"
        "build to read from ROM will not change on-screen art for that asset for FRLG+.\n"
        "Next wins are usually **text** (e.g. gOakSpeech_Text_* compiled from data/text)\n"
        "or **scripts/maps** (generated portable data + script pointer resolver), not title GFX."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
