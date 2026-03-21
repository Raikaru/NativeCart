#!/usr/bin/env python3
"""
Compare two Pokémon FireRed raw flash save images (e.g. mGBA .sav vs portable firered_save.sav).

Layout matches include/save.h and the 128 KiB image in src_transformed/agb_flash.c
(FLASH_ROM_SIZE_1M = 131072 bytes, 32 × 4096-byte sectors).

Usage:
  python tools/compare_saves.py mgba.sav port.sav
  python tools/compare_saves.py a.sav b.sav -v --data-bytes 64
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# include/save.h
SECTOR_DATA_SIZE = 3968
SECTOR_FOOTER_SIZE = 128
SECTOR_SIZE = SECTOR_DATA_SIZE + SECTOR_FOOTER_SIZE  # 4096
SECTORS_COUNT = 32
SECTOR_SIGNATURE = 0x08012025
FLASH_SIZE = 131072

# Footer fields start after data + unused (128 - 12 = 116 bytes unused)
FOOTER_OFFSET = SECTOR_DATA_SIZE + (SECTOR_FOOTER_SIZE - 12)  # 4084


def sector_role(index: int) -> str:
    """Human-readable sector purpose (physical index 0..31)."""
    if index < 14:
        slot = 1
        i = index
    elif index < 28:
        slot = 2
        i = index - 14
    else:
        names = {
            28: "Hall of Fame A",
            29: "Hall of Fame B",
            30: "Trainer Tower A",
            31: "Trainer Tower B",
        }
        return names.get(index, f"unknown ({index})")

    if i == 0:
        return f"Save slot {slot}: SaveBlock2"
    if 1 <= i <= 4:
        return f"Save slot {slot}: SaveBlock1 part {i}/4"
    if 5 <= i <= 13:
        return f"Save slot {slot}: Pokémon Storage box group {i - 5 + 1}/9"
    return f"Save slot {slot}: sector {i}"


def parse_footer(sector: bytes) -> tuple[int, int, int, int]:
    if len(sector) < SECTOR_SIZE:
        raise ValueError(f"sector length {len(sector)} < {SECTOR_SIZE}")
    raw = sector[FOOTER_OFFSET : FOOTER_OFFSET + 12]
    id_ = int.from_bytes(raw[0:2], "little")
    checksum = int.from_bytes(raw[2:4], "little")
    signature = int.from_bytes(raw[4:8], "little")
    counter = int.from_bytes(raw[8:12], "little")
    return id_, checksum, signature, counter


def load_save(path: Path) -> bytes:
    data = path.read_bytes()
    if len(data) == FLASH_SIZE:
        return data
    if len(data) < FLASH_SIZE:
        # Portable may have short read; game pads with 0xFF
        padded = bytearray([0xFF]) * FLASH_SIZE
        padded[: len(data)] = data
        return bytes(padded)
    print(f"warning: {path} is {len(data)} bytes (expected {FLASH_SIZE}); truncating", file=sys.stderr)
    return data[:FLASH_SIZE]


def sector_digest(data: bytes, index: int) -> dict:
    off = index * SECTOR_SIZE
    chunk = data[off : off + SECTOR_SIZE]
    id_, checksum, signature, counter = parse_footer(chunk)
    return {
        "index": index,
        "id": id_,
        "checksum": checksum,
        "signature": signature,
        "counter": counter,
        "valid_sig": signature == SECTOR_SIGNATURE,
        "data": chunk[:SECTOR_DATA_SIZE],
        "raw": chunk,
    }


def print_row(label: str, d: dict) -> None:
    sig = "ok" if d["valid_sig"] else f"0x{d['signature']:08X}"
    print(
        f"  [{d['index']:2d}] {label[:42]:42} "
        f"id=0x{d['id']:04X} chk=0x{d['checksum']:04X} sig={sig} ctr={d['counter']}"
    )


def first_data_diff(a: bytes, b: bytes, max_offsets: int) -> list[tuple[int, int, int]]:
    """List (offset, byte_a, byte_b) for first differing bytes in data region."""
    out: list[tuple[int, int, int]] = []
    for i in range(min(len(a), len(b), SECTOR_DATA_SIZE)):
        if a[i] != b[i]:
            out.append((i, a[i], b[i]))
            if len(out) >= max_offsets:
                break
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare FireRed 128KiB flash save files.")
    ap.add_argument("file_a", type=Path, help="First .sav (e.g. mGBA)")
    ap.add_argument("file_b", type=Path, help="Second .sav (e.g. portable export)")
    ap.add_argument("-v", "--verbose", action="store_true", help="Print footer for every sector")
    ap.add_argument(
        "--data-bytes",
        type=int,
        default=32,
        metavar="N",
        help="Max differing data-byte samples to show per sector (default 32, 0=skip)",
    )
    ap.add_argument(
        "--summary-only",
        action="store_true",
        help="Only print mismatch count and exit",
    )
    args = ap.parse_args()

    for p in (args.file_a, args.file_b):
        if not p.is_file():
            print(f"error: not a file: {p}", file=sys.stderr)
            return 2

    raw_a = load_save(args.file_a)
    raw_b = load_save(args.file_b)

    print(f"A: {args.file_a} ({len(raw_a)} bytes)")
    print(f"B: {args.file_b} ({len(raw_b)} bytes)")
    print()

    mismatches = 0
    footer_mismatches = 0

    for idx in range(SECTORS_COUNT):
        da = sector_digest(raw_a, idx)
        db = sector_digest(raw_b, idx)
        label = sector_role(idx)

        same_raw = da["raw"] == db["raw"]
        same_footer = (
            da["id"] == db["id"]
            and da["checksum"] == db["checksum"]
            and da["signature"] == db["signature"]
            and da["counter"] == db["counter"]
        )

        if args.verbose:
            print(f"{label}:")
            print_row("A", da)
            print_row("B", db)
            print()

        if same_raw:
            continue

        mismatches += 1
        if not same_footer:
            footer_mismatches += 1

        if args.summary_only:
            continue

        print(f"*** DIFF sector {idx}: {label}")
        print_row("A", da)
        print_row("B", db)

        if args.data_bytes > 0:
            samples = first_data_diff(da["data"], db["data"], args.data_bytes)
            if samples:
                parts = [f"+0x{o:04X}: {va:02X} vs {vb:02X}" for o, va, vb in samples]
                print("    data: " + ", ".join(parts))
        print()

    print("---")
    print(f"Sectors with any byte difference: {mismatches} / {SECTORS_COUNT}")
    print(f"Sectors where footer (id/chk/sig/counter) differs: {footer_mismatches}")

    if mismatches == 0:
        print("Files match (full 128 KiB image).")
        return 0

    print(
        "\nTip: matching footers but different data usually means different game state;\n"
        "      invalid signature (not 0x08012025) often means empty/erased (0xFF) sector.",
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
