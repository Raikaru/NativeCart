#!/usr/bin/env python3
"""
Scan a GBA ROM for early new-game help / Pikachu intro strings (same bytes as
compiled from data/text/new_game_intro.inc via charmap.txt).

Usage:
  python tools/find_early_new_game_help_text_in_rom.py path/to.rom

Exit 0. Prints per-label: first offset (hex), hit count, or MISSING.
"""
from __future__ import annotations

import sys
from pathlib import Path

EOS = 0xFF
CHAR_NEWLINE = 0xFE


def load_charmap(path: Path) -> dict[str, bytes]:
    """Map single UTF-8 character (as str len 1) or escape name to encoded bytes."""
    single: dict[str, bytes] = {}
    with path.open(encoding="utf-8") as f:
        for raw in f:
            line = raw.split("@", 1)[0].strip()
            if not line or "=" not in line:
                continue
            # Use rightmost '=' so keys like '=' (equals sign) parse correctly.
            left, right = line.rsplit("=", 1)
            left = left.strip()
            right = right.strip()
            parts = right.split()
            b = bytes(int(x, 16) & 0xFF for x in parts)
            if not b:
                continue
            if left.startswith("'") and left.endswith("'") and len(left) >= 3:
                inner = left[1:-1]
                if inner == "\\n":
                    single["\n"] = b
                elif inner == "\\l":
                    single["\l"] = b
                elif inner == "\\p":
                    single["\p"] = b
                else:
                    # 'A' or '\''
                    if inner.startswith("\\") and len(inner) == 2:
                        single[inner[1:2]] = b
                    else:
                        single[inner] = b
            else:
                name = left.strip()
                single[name] = b
    return single


def encode_gba_string(charmap: dict[str, bytes], text: str) -> bytes:
    out = bytearray()
    for ch in text:
        if ch not in charmap:
            raise KeyError(f"No charmap entry for {ch!r} in text fragment {text[:40]!r}...")
        out.extend(charmap[ch])
    return bytes(out)


def concat_with_eos(parts: list[str], charmap: dict[str, bytes]) -> bytes:
    buf = bytearray()
    for i, p in enumerate(parts):
        buf.extend(encode_gba_string(charmap, p))
        if i < len(parts) - 1:
            buf.append(CHAR_NEWLINE)
    buf.append(EOS)
    return bytes(buf)


# Fragments from data/text/new_game_intro.inc (concatenated .string lines per label;
# internal newlines are FE between assembler .string directives).
EARLY_NG_LABELS: list[tuple[str, list[str]]] = [
    (
        "Intro",
        [
            "The various buttons will be explained in",
            "the order of their importance.",
        ],
    ),
    (
        "DPad",
        [
            "Moves the main character.",
            "Also used to choose various data",
            "headings.",
        ],
    ),
    (
        "AButton",
        [
            "Used to confirm a choice, check",
            "things, chat, and scroll text.",
        ],
    ),
    (
        "BButton",
        [
            "Used to exit, cancel a choice,",
            "and cancel a mode.",
        ],
    ),
    (
        "StartButton",
        [
            "Press this button to open the",
            "MENU.",
        ],
    ),
    (
        "SelectButton",
        [
            "Used to shift items and to use",
            "a registered item.",
        ],
    ),
    (
        "LRButtons",
        [
            "If you need help playing the",
            "game, or on how to do things,",
            "press the L or R Button.",
        ],
    ),
    (
        "PikachuPage1",
        [
            "In the world which you are about to",
            "enter, you will embark on a grand",
            "adventure with you as the hero.",
            "",
            "Speak to people and check things",
            "wherever you go, be it towns, roads,",
            "or caves. Gather information and",
            "hints from every source.",
        ],
    ),
    (
        "PikachuPage2",
        [
            "New paths will open to you by helping",
            "people in need, overcoming challenges,",
            "and solving mysteries.",
            "",
            "At times, you will be challenged by",
            "others and attacked by wild creatures.",
            "Be brave and keep pushing on.",
        ],
    ),
    (
        "PikachuPage3",
        [
            "Through your adventure, we hope",
            "that you will interact with all sorts",
            "of people and achieve personal growth.",
            "That is our biggest objective.",
            "",
            "Press the A Button, and let your",
            "adventure begin!",
        ],
    ),
]

ENV_KEYS = [
    "FIRERED_ROM_CTRL_GUIDE_TX_Intro",
    "FIRERED_ROM_CTRL_GUIDE_TX_DPad",
    "FIRERED_ROM_CTRL_GUIDE_TX_AButton",
    "FIRERED_ROM_CTRL_GUIDE_TX_BButton",
    "FIRERED_ROM_CTRL_GUIDE_TX_StartButton",
    "FIRERED_ROM_CTRL_GUIDE_TX_SelectButton",
    "FIRERED_ROM_CTRL_GUIDE_TX_LRButtons",
    "FIRERED_ROM_PIKACHU_INTRO_TX_Page1",
    "FIRERED_ROM_PIKACHU_INTRO_TX_Page2",
    "FIRERED_ROM_PIKACHU_INTRO_TX_Page3",
]


def find_all(hay: bytes, needle: bytes) -> list[int]:
    if not needle:
        return []
    hits: list[int] = []
    start = 0
    while True:
        i = hay.find(needle, start)
        if i < 0:
            break
        hits.append(i)
        start = i + 1
    return hits


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: find_early_new_game_help_text_in_rom.py <rom.gba>", file=sys.stderr)
        return 2
    rom_path = Path(sys.argv[1])
    if not rom_path.is_file():
        print(f"not a file: {rom_path}", file=sys.stderr)
        return 2
    root = Path(__file__).resolve().parents[1]
    charmap = load_charmap(root / "charmap.txt")

    needles: list[tuple[str, str, bytes]] = []
    for short, parts in EARLY_NG_LABELS:
        blob = concat_with_eos(parts, charmap)
        needles.append((short, ENV_KEYS[len(needles)], blob))

    rom = rom_path.read_bytes()
    print(f"ROM {rom_path} size=0x{len(rom):X}\n")

    for short, envk, blob in needles:
        hits = find_all(rom, blob)
        if not hits:
            print(f"{short:14} MISSING  (env {envk})  needle_len={len(blob)}")
        elif len(hits) == 1:
            print(f"{short:14} 0x{hits[0]:08X}  single hit  {envk}=0x{hits[0]:X}")
        else:
            print(f"{short:14} first=0x{hits[0]:08X}  hits={len(hits)}  {envk}=0x{hits[0]:X}  # ambiguous")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
