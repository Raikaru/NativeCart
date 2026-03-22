#!/usr/bin/env python3
"""
Inspect a byte slice of a GBA ROM image (vanilla or patched .gba).

Useful when validating FIRERED_ROM_* file offsets and documented blob sizes before
wiring portable env vars — see docs/rom_blob_inspection_playbook.md.

Examples:
  python tools/rom_blob_inspect.py baserom.gba -o 0x123456 -n 64 --hexdump
  python tools/rom_blob_inspect.py hack.gba -o 0xABCDEF -n 18 --struct-stride 2
  python tools/rom_blob_inspect.py baserom.gba -o 0x0 -n 0x100 --gba-u32-pointers
  python tools/rom_blob_inspect.py baserom.gba -o 0x... -n 0x... --u8-cols 9
  python tools/rom_blob_inspect.py baserom.gba -o 0x... -n 0x... --validate-egg-moves-u16
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path
from typing import Iterable


def parse_int(s: str) -> int:
    return int(s.strip().replace("_", ""), 0)


def hexdump(data: bytes, base_off: int = 0, max_lines: int | None = None) -> None:
    width = 16
    lines = 0
    for i in range(0, len(data), width):
        chunk = data[i : i + width]
        hex_part = " ".join(f"{b:02x}" for b in chunk)
        ascii_part = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        print(f"{base_off + i:08x}  {hex_part:<{width * 3 - 1}}  |{ascii_part}|")
        lines += 1
        if max_lines is not None and lines >= max_lines:
            rest = len(data) - (i + width)
            if rest > 0:
                print(f"... ({rest} more bytes; increase --hex-lines or omit limit)")
            break


def u16_words(data: bytes) -> list[int]:
    if len(data) % 2:
        raise ValueError("u16 view requires even byte length")
    return list(struct.unpack(f"<{len(data) // 2}H", data))


def u32_words(data: bytes) -> list[int]:
    if len(data) % 4:
        raise ValueError("u32 view requires length multiple of 4")
    return list(struct.unpack(f"<{len(data) // 4}I", data))


def summarize_u16(words: Iterable[int]) -> None:
    w = list(words)
    if not w:
        print("u16: (empty)")
        return
    print(
        "u16 LE: count=%d min=0x%04x max=0x%04x zeros=%d"
        % (len(w), min(w), max(w), sum(1 for x in w if x == 0))
    )


def summarize_u32(words: Iterable[int]) -> None:
    w = list(words)
    if not w:
        print("u32 LE: (empty)")
        return
    print(
        "u32 LE: count=%d min=0x%08x max=0x%08x zeros=%d"
        % (len(w), min(w), max(w), sum(1 for x in w if x == 0))
    )


def summarize_u8_columns(blob: bytes, stride: int) -> None:
    """Per-byte-column stats across fixed-width rows (e.g. BattleMove 9-byte rows, type triplets)."""
    if stride <= 0:
        raise ValueError("stride must be positive")
    if len(blob) % stride != 0:
        raise ValueError(f"length {len(blob)} not a multiple of u8 row stride {stride}")
    nrows = len(blob) // stride
    print(f"u8 fixed rows: stride={stride} rows={nrows}")
    for col in range(stride):
        vals = [blob[r * stride + col] for r in range(nrows)]
        print(
            "  col[%d]: min=0x%02x max=0x%02x zeros=%d"
            % (col, min(vals), max(vals), sum(1 for v in vals if v == 0))
        )


def validate_egg_moves_u16_stream(
    blob: bytes,
    *,
    species_offset: int,
    num_species: int,
    moves_count: int,
) -> tuple[int, list[tuple[int, int]]]:
    """
    Parse FireRed-style gEggMoves[] as little-endian u16:
    blocks of (species + species_offset) then move IDs until the next header or final 0xFFFF.
    Returns (terminator_index, list of (species_id, n_moves)).
    """
    if len(blob) % 2:
        raise ValueError("egg-moves stream requires even byte length")
    words = list(struct.unpack(f"<{len(blob) // 2}H", blob))
    if not words:
        raise ValueError("empty u16 stream")
    lo = species_offset + 1
    hi_excl = species_offset + num_species
    blocks: list[tuple[int, int]] = []
    i = 0
    n = len(words)
    while i < n:
        w = words[i]
        if w == 0xFFFF:
            if i != n - 1:
                raise ValueError(f"0xFFFF at word index {i} but not final word (len={n})")
            return i, blocks
        if not (lo <= w < hi_excl):
            raise ValueError(
                f"expected species header in [{lo:#x}, {hi_excl:#x}) at index {i}, got 0x{w:04x}"
            )
        species_id = w - species_offset
        i += 1
        move_count = 0
        while i < n:
            w = words[i]
            if w == 0xFFFF:
                if i != n - 1:
                    raise ValueError(f"0xFFFF at word index {i} is not the final stream terminator")
                blocks.append((species_id, move_count))
                return i, blocks
            if lo <= w < hi_excl:
                break
            if w >= moves_count:
                raise ValueError(
                    f"move id out of range at index {i}: 0x{w:04x} (moves_count={moves_count})"
                )
            i += 1
            move_count += 1
        blocks.append((species_id, move_count))
    raise ValueError("stream ended without final 0xFFFF terminator")


def summarize_gba_rom_pointers(words: list[int], rom_size: int) -> None:
    """Counts u32 values that look like ROM addresses in the loaded image (0x08xxxxxx)."""
    rom_lo = 0x08000000
    rom_hi_excl = 0x08000000 + rom_size
    in_img = out_win = zero = other = 0
    for v in words:
        if v == 0:
            zero += 1
        elif rom_lo <= v < rom_hi_excl:
            in_img += 1
        elif rom_lo <= v < 0x0A000000:
            out_win += 1
        else:
            other += 1
    print(
        "u32 as GBA ROM ptrs: zero=%d in_image=[0x08.., <rom_end)=%d "
        "gba_rom_outside_image=%d other=%d"
        % (zero, in_img, out_win, other)
    )


def main() -> int:
    p = argparse.ArgumentParser(description="Hex/stats for a ROM file slice (portable blob validation).")
    p.add_argument("rom", type=Path, help="Path to .gba (or any binary ROM image)")
    p.add_argument(
        "-o",
        "--offset",
        required=True,
        help="Start offset into file (e.g. 0x12345 or 12345)",
    )
    p.add_argument(
        "-n",
        "--length",
        required=True,
        help="Number of bytes to read (e.g. 0x40 or 64)",
    )
    p.add_argument("--hexdump", action="store_true", help="Print hex + ASCII lines")
    p.add_argument(
        "--hex-lines",
        type=int,
        default=16,
        metavar="N",
        help="Max hexdump lines (default 16); use 0 for full slice",
    )
    p.add_argument("--no-stats", action="store_true", help="Skip u16/u32 summaries")
    p.add_argument(
        "--crc32",
        action="store_true",
        help="Print IEEE CRC32 of the slice (matches common ROM tooling)",
    )
    p.add_argument(
        "--struct-stride",
        type=str,
        metavar="BYTES",
        help="Require slice length %% BYTES == 0 (e.g. 26 for SpeciesInfo rows)",
    )
    p.add_argument(
        "--expect-len",
        type=str,
        metavar="BYTES",
        help="Fail if slice length != BYTES (after bounds check)",
    )
    p.add_argument(
        "--gba-u32-pointers",
        action="store_true",
        help="After u32 summary, classify words as GBA ROM pointers vs zero/other",
    )
    p.add_argument(
        "--u8-cols",
        type=str,
        metavar="STRIDE",
        help="Treat slice as fixed u8 rows of STRIDE bytes; print per-column min/max/zero counts",
    )
    p.add_argument(
        "--validate-egg-moves-u16",
        action="store_true",
        help="Validate FireRed-style gEggMoves u16 stream (species+offset headers, move IDs, trailing 0xFFFF)",
    )
    p.add_argument(
        "--egg-species-offset",
        type=str,
        default="20000",
        metavar="N",
        help="With --validate-egg-moves-u16: same as EGG_MOVES_SPECIES_OFFSET (default 20000)",
    )
    p.add_argument(
        "--egg-num-species",
        type=str,
        default="412",
        metavar="N",
        help="With --validate-egg-moves-u16: NUM_SPECIES for header range check (default 412)",
    )
    p.add_argument(
        "--egg-moves-count",
        type=str,
        default="355",
        metavar="N",
        help="With --validate-egg-moves-u16: max exclusive move id (default 355 = MOVES_COUNT)",
    )
    args = p.parse_args()

    try:
        off = parse_int(args.offset)
        nbytes = parse_int(args.length)
    except ValueError as e:
        print(f"error: bad offset/length: {e}", file=sys.stderr)
        return 2

    rom_path = args.rom
    if not rom_path.is_file():
        print(f"error: ROM not found: {rom_path}", file=sys.stderr)
        return 2

    rom = rom_path.read_bytes()
    rom_size = len(rom)
    if off < 0 or nbytes < 0 or off + nbytes > rom_size:
        print(
            f"error: slice [{off:#x}, {off + nbytes:#x}) out of range for file size {rom_size:#x}",
            file=sys.stderr,
        )
        return 2

    blob = rom[off : off + nbytes]

    if args.expect_len is not None:
        try:
            elen = parse_int(args.expect_len)
        except ValueError:
            print("error: --expect-len must be an integer", file=sys.stderr)
            return 2
        if len(blob) != elen:
            print(
                f"error: slice length {len(blob)} != --expect-len {elen}",
                file=sys.stderr,
            )
            return 1

    if args.struct_stride is not None:
        try:
            stride = parse_int(args.struct_stride)
        except ValueError:
            print("error: --struct-stride must be an integer", file=sys.stderr)
            return 2
        if stride <= 0:
            print("error: --struct-stride must be positive", file=sys.stderr)
            return 2
        if len(blob) % stride != 0:
            print(
                f"error: length {len(blob)} not a multiple of struct stride {stride}",
                file=sys.stderr,
            )
            return 1

    print(f"rom={rom_path} file_size=0x{rom_size:x} slice=[0x{off:x}, 0x{off + len(blob):x}) len={len(blob)}")

    if args.u8_cols is not None:
        try:
            cs = parse_int(args.u8_cols)
        except ValueError:
            print("error: --u8-cols must be an integer", file=sys.stderr)
            return 2
        try:
            summarize_u8_columns(blob, cs)
        except ValueError as e:
            print(f"error: --u8-cols: {e}", file=sys.stderr)
            return 1

    if args.validate_egg_moves_u16:
        try:
            esp = parse_int(args.egg_species_offset)
            ens = parse_int(args.egg_num_species)
            emc = parse_int(args.egg_moves_count)
        except ValueError:
            print("error: egg-* options must be integers", file=sys.stderr)
            return 2
        try:
            term_i, blocks = validate_egg_moves_u16_stream(
                blob, species_offset=esp, num_species=ens, moves_count=emc
            )
            print(
                "egg_moves u16: ok blocks=%d terminator_word_index=%d total_u16_words=%d"
                % (len(blocks), term_i, len(blob) // 2)
            )
            if blocks:
                move_counts = [m for _, m in blocks]
                print(
                    "  moves per block: min=%d max=%d total_move_slots=%d"
                    % (min(move_counts), max(move_counts), sum(move_counts))
                )
        except ValueError as e:
            print(f"error: --validate-egg-moves-u16: {e}", file=sys.stderr)
            return 1

    if args.crc32:
        print("crc32_ieee(slice)=0x%08x" % (zlib.crc32(blob) & 0xFFFFFFFF))

    if args.hexdump:
        limit = None if args.hex_lines == 0 else args.hex_lines
        hexdump(blob, base_off=off, max_lines=limit)

    if not args.no_stats:
        try:
            summarize_u16(u16_words(blob))
        except ValueError as e:
            print(f"u16: skipped ({e})")

    want_u32 = (not args.no_stats) or args.gba_u32_pointers
    if want_u32:
        try:
            words32 = u32_words(blob)
            if not args.no_stats:
                summarize_u32(words32)
            if args.gba_u32_pointers:
                summarize_gba_rom_pointers(words32, rom_size)
        except ValueError as e:
            print(f"u32: skipped ({e})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
