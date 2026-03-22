#!/usr/bin/env python3
"""
Offline search for FireRed title-logo (game title BG) palette / LZ tiles / LZ tilemap
offsets in a .gba ROM, for use with FIRERED_ROM_TITLE_LOGO_*_OFF.

Reference assets: graphics/title_screen/firered/game_title_logo.*

Usage:
  python tools/title_logo_rom_offsets.py --base baserom.gba --bps mods/frlgPlus.bps --map-dec-size 0x500
  python tools/title_logo_rom_offsets.py --rom path/to/patched.gba --map-dec-size 0x500

Use --map-dec-size 0x500 when the title tilemap is still 32x20 (1280 bytes): the first valid LZ
blob after the tiles stream is often an unrelated asset (e.g. dec 0x1100); locking decompressed
size finds the real logo map and maximizes similarity to graphics/title_screen/firered/*. For hacks
that change palette only, try --pal-min-sim 0.95 (or lower) while keeping tile signature hits.

Options:
  --root   repo root (default: parent of tools/)
  --search-after-tiles-bytes N   max bytes after tiles LZ to score map candidates (default 2MiB)
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


def crc32_ieee(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
    return crc ^ 0xFFFFFFFF


def bps_read_u32_le(p: bytes, i: int) -> int:
    return p[i] | (p[i + 1] << 8) | (p[i + 2] << 16) | (p[i + 3] << 24)


def bps_decode_var(data: bytes, pos: list) -> int:
    out = 0
    shift = 1
    p = pos[0]
    while p < len(data):
        x = data[p]
        p += 1
        out += (x & 0x7F) * shift
        if x & 0x80:
            pos[0] = p
            return out
        shift <<= 7
        out += shift
    raise ValueError("bps_decode_var: truncated")


def apply_bps(source: bytes, patch: bytes) -> bytes:
    if len(patch) < 16 or patch[:4] != b"BPS1":
        raise ValueError("not a BPS1 patch")
    max_pos = len(patch) - 12
    pos = [4]
    source_size_expect = bps_decode_var(patch, pos)
    target_size = bps_decode_var(patch, pos)
    metadata_len = bps_decode_var(patch, pos)
    if pos[0] + metadata_len > max_pos:
        raise ValueError("metadata OOB")
    pos[0] += metadata_len

    crc_pos = len(patch) - 12
    expect_source_crc = bps_read_u32_le(patch, crc_pos)
    expect_target_crc = bps_read_u32_le(patch, crc_pos + 4)
    expect_patch_crc = bps_read_u32_le(patch, crc_pos + 8)
    calc_patch = crc32_ieee(patch[:-4])
    if calc_patch != expect_patch_crc:
        print(
            "warning: patch file CRC mismatch (expected 0x%08x got 0x%08x); continuing anyway"
            % (expect_patch_crc, calc_patch),
            file=sys.stderr,
        )

    if len(source) != source_size_expect:
        raise ValueError(
            "base ROM size mismatch: have %d bytes, patch expects %d"
            % (len(source), source_size_expect)
        )
    if crc32_ieee(source) != expect_source_crc:
        raise ValueError(
            "base ROM CRC mismatch (wrong clean ROM for this BPS? expected 0x%08x)"
            % expect_source_crc
        )

    target = bytearray(target_size)
    target_write = 0
    source_copy_pos = 0
    target_copy_pos = 0

    while pos[0] < max_pos and target_write < target_size:
        action_data = bps_decode_var(patch, pos)
        action = action_data & 3
        length = (action_data >> 2) + 1

        if target_write + length > target_size:
            raise ValueError("BPS overflow")

        if action == 0:  # SourceRead
            if target_write + length > len(source):
                raise ValueError("SourceRead OOB")
            target[target_write : target_write + length] = source[
                target_write : target_write + length
            ]
            target_write += length
        elif action == 1:  # TargetRead
            if pos[0] + length > max_pos:
                raise ValueError("TargetRead OOB")
            target[target_write : target_write + length] = patch[pos[0] : pos[0] + length]
            pos[0] += length
            target_write += length
        elif action == 2:  # SourceCopy
            off_enc = bps_decode_var(patch, pos)
            off = -(off_enc >> 1) if (off_enc & 1) else (off_enc >> 1)
            if off < 0:
                u = -off
                if u > source_copy_pos:
                    raise ValueError("SourceCopy underflow")
                source_copy_pos -= u
            else:
                source_copy_pos += off
            if source_copy_pos + length > len(source):
                raise ValueError("SourceCopy OOB")
            target[target_write : target_write + length] = source[
                source_copy_pos : source_copy_pos + length
            ]
            source_copy_pos += length
            target_write += length
        elif action == 3:  # TargetCopy
            off_enc = bps_decode_var(patch, pos)
            off = -(off_enc >> 1) if (off_enc & 1) else (off_enc >> 1)
            if off < 0:
                u = -off
                if u > target_copy_pos:
                    raise ValueError("TargetCopy underflow")
                target_copy_pos -= u
            else:
                target_copy_pos += off
            if target_copy_pos >= target_write:
                raise ValueError("TargetCopy read at/ past write head")
            if target_copy_pos + length > target_size:
                raise ValueError("TargetCopy OOB")
            for i in range(length):
                target[target_write + i] = target[target_copy_pos + i]
            target_copy_pos += length
            target_write += length
        else:
            raise ValueError("unknown BPS action")

    if target_write != target_size:
        raise ValueError("BPS size mismatch after decode")
    out = bytes(target)
    if crc32_ieee(out) != expect_target_crc:
        raise ValueError(
            "output CRC mismatch (expected 0x%08x got 0x%08x)"
            % (expect_target_crc, crc32_ieee(out))
        )
    return out


def lz77_decompress_gba(data: bytes) -> tuple[bytes | None, int]:
    """
    Returns (decompressed, compressed_byte_len) or (None, 0) on failure.
    """
    if len(data) < 4 or data[0] != 0x10:
        return None, 0
    dec_size = data[1] | (data[2] << 8) | (data[3] << 16)
    if dec_size > 0x20000:
        return None, 0
    out = bytearray(dec_size)
    si = 4
    di = 0
    base = 0
    try:
        while di < dec_size:
            if si >= len(data):
                return None, 0
            flags = data[si]
            si += 1
            for _ in range(8):
                if di >= dec_size:
                    break
                if flags & 0x80:
                    if si + 2 > len(data):
                        return None, 0
                    hi, lo = data[si], data[si + 1]
                    si += 2
                    count = (hi >> 4) + 3
                    disp = (((hi & 0x0F) << 8) | lo) + 1
                    for _ in range(count):
                        if di >= dec_size:
                            break
                        out[di] = out[di - disp] if disp <= di else 0
                        di += 1
                else:
                    if si >= len(data):
                        return None, 0
                    out[di] = data[si]
                    si += 1
                    di += 1
                flags = (flags << 1) & 0xFF
    except (IndexError, MemoryError):
        return None, 0
    return bytes(out), si


def load_reference_firered(root: Path) -> tuple[bytes, bytes, bytes, bytes]:
    """Returns (pal512, tiles_lz, map_lz, tile_sig_32)."""
    g = root / "graphics" / "title_screen" / "firered"
    pal = (g / "game_title_logo.gbapal").read_bytes()
    tiles_lz = (g / "game_title_logo.8bpp.lz").read_bytes()
    map_lz = (g / "game_title_logo.bin.lz").read_bytes()
    if len(pal) != 512:
        raise ValueError("expected 512-byte palette")
    tile_sig = tiles_lz[:32]
    return pal, tiles_lz, map_lz, tile_sig


def map_match_score(dec: bytes, ref_map_dec: bytes) -> float:
    if len(dec) != len(ref_map_dec):
        return 0.0
    same = sum(1 for a, b in zip(dec, ref_map_dec) if a == b)
    return same / len(ref_map_dec)


def find_all(hay: bytes, needle: bytes, align: int = 1) -> list[int]:
    out = []
    i = 0
    while True:
        j = hay.find(needle, i)
        if j < 0:
            break
        if j % align == 0:
            out.append(j)
        i = j + 1
    return out


def pal_similarity(a: bytes, b: bytes) -> float:
    if len(a) != len(b):
        return 0.0
    same = sum(1 for x, y in zip(a, b) if x == y)
    return same / len(a)


def analyze_rom(
    rom: bytes,
    ref_pal: bytes,
    ref_tiles_lz: bytes,
    ref_map_lz: bytes,
    tile_sig: bytes,
    search_span: int,
    pal_min_sim: float,
    tiles_min_sim: float,
    map_dec_size: int | None,
):
    ref_tiles_dec, _ = lz77_decompress_gba(ref_tiles_lz)
    ref_map_dec, _ = lz77_decompress_gba(ref_map_lz)
    assert ref_tiles_dec is not None and ref_map_dec is not None
    want_map_len = map_dec_size if map_dec_size is not None else len(ref_map_dec)

    print("Reference: pal=%d bytes tiles_lz=%d map_lz=%d tiles_dec=%d map_dec=%d" % (
        len(ref_pal), len(ref_tiles_lz), len(ref_map_lz), len(ref_tiles_dec), len(ref_map_dec),
    ))
    print(
        "Filters: pal_min_sim=%.2f tiles_min_sim=%.2f map_dec_size_filter=%s"
        % (pal_min_sim, tiles_min_sim, "0x%X" % want_map_len if map_dec_size else "any (prefer 0x500)")
    )

    tile_hits = find_all(rom, tile_sig, align=4)
    print("Tile LZ signature (32 bytes, 4-aligned) hits: %d" % len(tile_hits))
    for off in tile_hits[:40]:
        print("  0x%X" % off)

    triples = []

    for toff in tile_hits:
        if toff < 512:
            continue
        pal_off = toff - 512
        pal_chunk = rom[pal_off : pal_off + 512]
        psim = pal_similarity(pal_chunk, ref_pal)
        if psim < pal_min_sim:
            continue
        tslice = rom[toff:]
        tiles_dec, tclen = lz77_decompress_gba(tslice)
        if tiles_dec is None or tclen == 0:
            continue
        if tiles_dec != ref_tiles_dec:
            if len(tiles_dec) != len(ref_tiles_dec):
                continue
            tsim = sum(1 for a, b in zip(tiles_dec, ref_tiles_dec) if a == b) / len(ref_tiles_dec)
            if tsim < tiles_min_sim:
                continue
        else:
            tsim = 1.0

        region_start = toff + tclen
        end = min(len(rom), region_start + search_span)
        best = None  # (pri, score, moff, mdec, mclen, dec_size)

        pos = region_start
        while pos + 4 <= end:
            if rom[pos] != 0x10:
                pos += 1
                continue
            dec_hdr = rom[pos + 1] | (rom[pos + 2] << 8) | (rom[pos + 3] << 16)
            chunk = rom[pos : min(len(rom), pos + min(len(rom) - pos, 65536))]
            mdec, mclen = lz77_decompress_gba(chunk)
            if mdec is None or mclen == 0:
                pos += 1
                continue
            if map_dec_size is not None and len(mdec) != want_map_len:
                pos += 1
                continue
            score = map_match_score(mdec, ref_map_dec)
            # Strongly prefer exact stock map length when filter is off
            if map_dec_size is None and len(mdec) == len(ref_map_dec):
                pri = score + 0.25
            elif map_dec_size is not None:
                pri = score + 0.05
            else:
                pri = score * 0.5  # deprioritize wrong-size maps
            cand = (pri, score, pos, mdec, mclen, len(mdec), dec_hdr)
            if best is None or cand[0] > best[0]:
                best = cand
            pos += 1

        if best is None:
            triples.append(
                {
                    "pal": pal_off,
                    "pal_sim": psim,
                    "tiles": toff,
                    "map": None,
                    "tiles_sim": tsim,
                    "map_score": 0.0,
                    "note": "no matching LZ map in window (check --map-dec-size / span)",
                }
            )
            continue

        _, map_score, moff, _, mclen, actual_dec, dec_hdr = best
        triples.append(
            {
                "pal": pal_off,
                "pal_sim": psim,
                "tiles": toff,
                "map": moff,
                "tiles_sim": tsim,
                "map_score": map_score,
                "map_dec_size": actual_dec,
                "map_hdr_dec": dec_hdr,
                "map_clen": mclen,
            }
        )

    print("\n--- Candidate triples (pal+tile filters; map: best match to stock tilemap) ---")
    if not triples:
        print("(none)")
        return

    def sort_key(t):
        if t["map"] is None:
            return (-1, 0, 0)
        return (
            t["map_score"] + 0.02 * t["tiles_sim"] + 0.01 * t.get("pal_sim", 0),
            t["map_score"],
            t.get("pal_sim", 0),
        )

    triples.sort(key=sort_key, reverse=True)
    for i, t in enumerate(triples[:12]):
        print("\nCandidate #%d:" % (i + 1))
        print("  FIRERED_ROM_TITLE_LOGO_PAL_OFF=0x%X  (pal_sim_vs_stock=%.4f)" % (t["pal"], t.get("pal_sim", 0)))
        print("  FIRERED_ROM_TITLE_LOGO_TILES_OFF=0x%X" % t["tiles"])
        if t["map"] is not None:
            print("  FIRERED_ROM_TITLE_LOGO_MAP_OFF=0x%X" % t["map"])
            print(
                "  map_match_vs_stock=%.4f map_dec=0x%X map_hdr_dec=0x%X map_comp_len=0x%X tiles_sim=%.4f"
                % (
                    t["map_score"],
                    t.get("map_dec_size", 0),
                    t.get("map_hdr_dec", 0),
                    t.get("map_clen", 0),
                    t["tiles_sim"],
                )
            )
        else:
            print("  MAP: not found (%s)" % t.get("note", "?"))
            print("  tiles_sim=%.4f pal_sim=%.4f" % (t["tiles_sim"], t.get("pal_sim", 0)))

    best = triples[0]
    if best["map"] is not None and best["map_score"] > 0.01:
        print("\n*** Recommended env (best map similarity to stock) ***")
        print("set FIRERED_ROM_TITLE_LOGO_PAL_OFF=0x%X" % best["pal"])
        print("set FIRERED_ROM_TITLE_LOGO_TILES_OFF=0x%X" % best["tiles"])
        print("set FIRERED_ROM_TITLE_LOGO_MAP_OFF=0x%X" % best["map"])
    elif best["map"] is not None:
        print(
            "\n*** Warning: best map still low similarity to stock; verify visually. Provisional env: ***",
            file=sys.stderr,
        )
        print("set FIRERED_ROM_TITLE_LOGO_PAL_OFF=0x%X" % best["pal"])
        print("set FIRERED_ROM_TITLE_LOGO_TILES_OFF=0x%X" % best["tiles"])
        print("set FIRERED_ROM_TITLE_LOGO_MAP_OFF=0x%X" % best["map"])


def main() -> int:
    ap = argparse.ArgumentParser(description="Find title logo ROM offsets for FIRERED_ROM_TITLE_LOGO_*_OFF")
    ap.add_argument("--root", type=Path, default=None, help="repo root (default: .. from this script)")
    ap.add_argument("--rom", type=Path, default=None, help="patched or vanilla .gba")
    ap.add_argument("--base", type=Path, default=None, help="clean base ROM for BPS")
    ap.add_argument("--bps", type=Path, default=None, help="BPS patch to apply to --base")
    ap.add_argument(
        "--search-after-tiles-bytes",
        type=int,
        default=2 * 1024 * 1024,
        help="max bytes after tiles LZ to search for map candidates",
    )
    ap.add_argument(
        "--pal-min-sim",
        type=float,
        default=1.0,
        help="min palette byte similarity vs stock .gbapal (0-1). Use <1 for hacks that replace pal only.",
    )
    ap.add_argument(
        "--tiles-min-sim",
        type=float,
        default=0.85,
        help="min tiles pixel similarity vs stock when not byte-identical (0-1)",
    )
    ap.add_argument(
        "--map-dec-size",
        type=lambda x: int(x, 0),
        default=None,
        help="only consider LZ maps whose decompressed size equals this (e.g. 0x500). Default: any, prefer stock length.",
    )
    args = ap.parse_args()

    root = args.root
    if root is None:
        root = Path(__file__).resolve().parent.parent

    ref_pal, ref_tiles_lz, ref_map_lz, tile_sig = load_reference_firered(root)

    if args.rom is not None:
        rom = args.rom.read_bytes()
        print("Loaded ROM: %s (%d bytes)" % (args.rom, len(rom)))
    elif args.base is not None and args.bps is not None:
        base = args.base.read_bytes()
        patch = args.bps.read_bytes()
        print("Applying BPS: %s onto base %s" % (args.bps, args.base))
        rom = apply_bps(base, patch)
        print("Patched ROM size: %d bytes" % len(rom))
    else:
        print("Provide --rom FILE.gba OR (--base baserom.gba --bps patch.bps)", file=sys.stderr)
        return 2

    analyze_rom(
        rom,
        ref_pal,
        ref_tiles_lz,
        ref_map_lz,
        tile_sig,
        args.search_after_tiles_bytes,
        args.pal_min_sim,
        args.tiles_min_sim,
        args.map_dec_size,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
