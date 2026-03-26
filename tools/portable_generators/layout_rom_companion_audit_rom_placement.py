#!/usr/bin/env python3
"""
Read-only checks for **layout companion bundle** placement at file offset **P** in a **.gba** (or any
host image). Does **not** modify the ROM.

**What this can prove**
  - **P + bundle_size ≤ ROM size** (hard requirement for splice).
  - Simple **byte statistics** on the slice (min/max, fraction **0x00** / **0xFF**, distinct byte
    count) so you can compare with a padding region you already trust.

**What it cannot prove**
  - That no game code or assets ever read **P … P+size**. Only your map / linker / upstream tool
    chain can assert that.
  - That a long run of **0xFF** (or **0x00**) is “free”; compressed or random-looking data can
    still match runs by accident.

**Heuristic (optional):** ``--suggest-tail-padding`` looks at the **trailing** constant fill (default
byte **0xFF**) from EOF backward — common on cartridge images, **not guaranteed** for every hack.
If the tail run is long enough, it prints a candidate **P** aligned with ``--align`` (default **4**)
when possible. Treat as a **starting point**, not approval.

**Bundle size:** pass ``--manifest`` (preferred), ``--bundle`` (reads sibling ``*.manifest.txt``), or
``--byte-count``.

Usage:
  python layout_rom_companion_audit_rom_placement.py --rom hack.gba --offset 0xP --manifest bundle.manifest.txt
  python layout_rom_companion_audit_rom_placement.py --rom hack.gba --suggest-tail-padding --bundle build/offline_map_layout_rom_companion_bundle.bin
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def parse_manifest(path: Path) -> dict[str, int]:
    out: dict[str, int] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or "=" not in line:
            continue
        k, v = line.split("=", 1)
        k, v = k.strip(), v.strip()
        try:
            out[k] = int(v.replace("_", ""), 0)
        except ValueError as e:
            raise ValueError(f"{path}: bad line {raw!r} ({e})") from e
    return out


def resolve_byte_count(
    *,
    byte_count: int | None,
    bundle: Path | None,
    manifest: Path | None,
) -> tuple[int, Path | None]:
    if byte_count is not None:
        if byte_count <= 0:
            raise ValueError("--byte-count must be positive")
        return byte_count, None
    if manifest is not None:
        mp = manifest.resolve()
        d = parse_manifest(mp)
        if "TOTAL_BYTES" not in d:
            raise ValueError(f"{mp} missing TOTAL_BYTES")
        return d["TOTAL_BYTES"], mp
    if bundle is not None:
        bp = bundle.resolve()
        mp = bp.with_suffix(bp.suffix + ".manifest.txt")
        if not mp.is_file():
            raise ValueError(f"missing manifest {mp} (pass --manifest or --byte-count)")
        d = parse_manifest(mp)
        if "TOTAL_BYTES" not in d:
            raise ValueError(f"{mp} missing TOTAL_BYTES")
        return d["TOTAL_BYTES"], mp
    raise ValueError("need one of --byte-count, --manifest, or --bundle")


def align_up(x: int, a: int) -> int:
    if a <= 0:
        raise ValueError("align must be positive")
    return (x + a - 1) // a * a


def trailing_constant_prefix_start(rom: bytes, fill: int) -> int:
    """Index of first byte in the longest suffix where every byte == fill (empty ROM => 0)."""
    n = len(rom)
    i = n
    while i > 0 and rom[i - 1] == fill:
        i -= 1
    return i


def suggest_tail_p(rom: bytes, need: int, fill: int, align: int) -> tuple[int | None, int, int]:
    """
    Returns (P_or_none, pad_start, suffix_len).
    pad_start = first index of trailing fill run; suffix_len = n - pad_start.
    """
    n = len(rom)
    pad_start = trailing_constant_prefix_start(rom, fill)
    suffix_len = n - pad_start
    if suffix_len < need:
        return None, pad_start, suffix_len
    hi = n - need
    lo = pad_start
    if lo > hi:
        return None, pad_start, suffix_len
    p = align_up(lo, align)
    if p > hi:
        p = lo
    if p + need > n:
        return None, pad_start, suffix_len
    if not all(b == fill for b in rom[p : p + need]):
        return None, pad_start, suffix_len
    return p, pad_start, suffix_len


def slice_stats(blob: bytes) -> None:
    if not blob:
        print("slice: (empty)")
        return
    n = len(blob)
    u = len(set(blob))
    n0 = sum(1 for b in blob if b == 0)
    nf = sum(1 for b in blob if b == 255)
    print(f"slice bytes={n} distinct_u8={u} pct_0x00={100.0 * n0 / n:.2f}% pct_0xFF={100.0 * nf / n:.2f}%")
    print(f"slice min=0x{min(blob):02x} max=0x{max(blob):02x}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--rom", type=Path, required=True, help="host ROM path (read-only)")
    ap.add_argument("--offset", type=lambda s: int(s.replace("_", ""), 0), default=None, help="file offset P to audit")
    ap.add_argument(
        "--suggest-tail-padding",
        action="store_true",
        help="heuristic: suggest P inside trailing constant fill from EOF (see docstring)",
    )
    ap.add_argument("--manifest", type=Path, default=None)
    ap.add_argument("--bundle", type=Path, default=None)
    ap.add_argument("--byte-count", type=int, default=None)
    ap.add_argument(
        "--fill-byte",
        type=lambda s: int(s.replace("_", ""), 0) & 0xFF,
        default=0xFF,
        help="constant fill byte for --suggest-tail-padding (default 0xFF)",
    )
    ap.add_argument("--align", type=int, default=4, help="try to align suggested P (default 4)")
    args = ap.parse_args()

    if args.suggest_tail_padding == (args.offset is not None):
        print("error: pass exactly one of --offset P or --suggest-tail-padding", file=sys.stderr)
        return 2

    rom_path = args.rom.resolve()
    if not rom_path.is_file():
        print(f"missing --rom {rom_path}", file=sys.stderr)
        return 1

    try:
        need, man_used = resolve_byte_count(
            byte_count=args.byte_count,
            bundle=args.bundle,
            manifest=args.manifest,
        )
    except ValueError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    rom = rom_path.read_bytes()
    n = len(rom)
    print(f"rom: {rom_path}")
    print(f"rom_size=0x{n:x} ({n} bytes)")
    print(f"bundle_bytes={need} (0x{need:x})")
    if man_used is not None:
        print(f"manifest: {man_used}")
        try:
            d = parse_manifest(man_used)
            mpb = d.get("METATILE_PACK_BASE_OFFSET")
            if mpb is not None:
                print(f"manifest METATILE_PACK_BASE_OFFSET=0x{mpb:x} (must equal splice P)")
        except ValueError:
            pass

    if args.suggest_tail_padding:
        print()
        print("### HEURISTIC: trailing constant fill (NOT proof of free space)")
        p, pad_start, suffix_len = suggest_tail_p(rom, need, args.fill_byte, args.align)
        print(
            f"fill_byte=0x{args.fill_byte:02x} align={args.align} "
            f"tail_run_start=0x{pad_start:x} tail_run_len=0x{suffix_len:x} ({suffix_len} bytes)"
        )
        if p is None:
            print(
                "result: no candidate P (tail run too short, or alignment does not fit). "
                "Choose P from your linker map / hack docs / hex editor."
            )
            return 0
        print(f"suggested P=0x{p:x} (verify manually; then build bundle with --metatile-pack-base-offset 0x{p:x})")
        print()
        print("Follow-up (read-only):")
        print(
            f'  python tools/rom_blob_inspect.py "{rom_path}" -o 0x{p:x} -n {min(need, 256)} --hexdump --hex-lines 8'
        )
        print(
            f"  python tools/portable_generators/layout_rom_companion_audit_rom_placement.py "
            f'--rom "{rom_path}" --offset 0x{p:x} --byte-count {need}'
        )
        return 0

    p = args.offset
    assert p is not None
    end = p + need
    print()
    print(f"audit offset P=0x{p:x} end_exclusive=0x{end:x}")
    if p < 0:
        print("error: P must be >= 0", file=sys.stderr)
        return 2
    if end > n:
        print(
            f"error: P + bundle_size exceeds ROM (0x{end:x} > 0x{n:x}) - cannot splice here",
            file=sys.stderr,
        )
        return 1

    if p < 0x200:
        print(
            "warning: P is inside the first 0x200 bytes (GBA exception vectors / header). "
            "Almost never appropriate for a layout bundle on a real game ROM."
        )

    blob = rom[p:end]
    slice_stats(blob)

    if all(b == args.fill_byte for b in blob):
        print(f"note: entire slice is constant 0x{args.fill_byte:02x} (necessary but not sufficient for safety).")
    else:
        print(
            "note: slice is not uniformly 0xFF - if you expected padding, re-check P or inspect "
            "with tools/rom_blob_inspect.py before overwriting."
        )

    print()
    print("Follow-up:")
    print(
        f'  python tools/rom_blob_inspect.py "{rom_path}" -o 0x{p:x} -n {min(need, 512)} --hexdump --hex-lines 16'
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
