#!/usr/bin/env python3
"""
Read a ``build_map_layout_rom_companion_bundle.py`` manifest (``*.manifest.txt``) and emit
**host-ROM file offsets** for the three layout companion directories plus copy-paste snippets for
runtime env vars and ``rom_blob_inspect.py``.

**Formula:** ``FIRERED_ROM_*_OFF = bundle_base_offset + METATILES_OFF`` (and likewise for
``DIMENSIONS_OFF``, ``TILESET_INDICES_OFF`` from the manifest). Manifest offsets are relative to the
first byte of the bundle.

**Metatile blob pointers:** the companion manifest records ``METATILE_PACK_BASE_OFFSET`` (from
``build_map_layout_rom_companion_bundle.py --metatile-pack-base-offset``). That value must equal the
host file offset **P** where the bundle starts (e.g. after
``layout_rom_companion_splice_bundle.py --offset P``). If ``B > 0`` and the metatile segment was built
with pack base ``0``, ``border_off`` / ``map_off`` are wrong for the host image unless you rebuild
with matching ``--pack-base-offset``.

Usage:
  python layout_rom_companion_emit_embed_env.py <manifest.txt> [--bundle-base-offset 0xP] [--rom hack.gba]

Options:
  --bundle-base-offset / -b   file offset where the bundle starts in the host ROM (default 0)
  --rom                       ROM path for concrete inspect/validate command lines (optional)
  --bash-only                 only print POSIX ``export`` lines
  --powershell-only           only print PowerShell ``$env:`` lines
  --inspect-only              only print ``rom_blob_inspect.py`` suggestions
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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("manifest", type=Path, help="path to *.manifest.txt from companion bundle")
    ap.add_argument(
        "-b",
        "--bundle-base-offset",
        type=lambda s: int(s.replace("_", ""), 0),
        default=0,
        help="file offset of bundle start in host ROM (default 0)",
    )
    ap.add_argument("--rom", type=Path, default=None, help="host .gba path for example commands")
    ap.add_argument("--bash-only", action="store_true")
    ap.add_argument("--powershell-only", action="store_true")
    ap.add_argument("--inspect-only", action="store_true")
    args = ap.parse_args()

    man = args.manifest.resolve()
    if not man.is_file():
        print(f"missing manifest {man}", file=sys.stderr)
        return 1

    d = parse_manifest(man)
    need = (
        "METATILES_OFF",
        "DIMENSIONS_OFF",
        "TILESET_INDICES_OFF",
        "METATILES_BYTES",
        "DIMENSIONS_BYTES",
        "TILESET_INDICES_BYTES",
        "TOTAL_BYTES",
    )
    for k in need:
        if k not in d:
            print(f"manifest missing {k}", file=sys.stderr)
            return 1

    b = args.bundle_base_offset
    if b < 0:
        print("bundle-base-offset must be >= 0", file=sys.stderr)
        return 2

    abs_meta = b + d["METATILES_OFF"]
    abs_dim = b + d["DIMENSIONS_OFF"]
    abs_ts = b + d["TILESET_INDICES_OFF"]

    rom_s = str(args.rom) if args.rom is not None else "YOUR_ROM.gba"
    hx = lambda n: hex(n)

    if sum(1 for x in (args.bash_only, args.powershell_only, args.inspect_only) if x) > 1:
        print("at most one of --bash-only / --powershell-only / --inspect-only", file=sys.stderr)
        return 2

    want_bash = args.bash_only or not (args.powershell_only or args.inspect_only)
    want_ps = args.powershell_only or not (args.bash_only or args.inspect_only)
    want_insp = args.inspect_only or not (args.bash_only or args.powershell_only)

    mpb = d.get("METATILE_PACK_BASE_OFFSET")
    if b != 0:
        if mpb is not None and mpb == b:
            pass
        elif mpb is not None:
            print(
                f"warning: bundle-base-offset {hex(b)} != manifest METATILE_PACK_BASE_OFFSET "
                f"{hex(mpb)}; metatile pointers may not match the host ROM.\n",
                file=sys.stderr,
            )
        else:
            print(
                "note: bundle-base-offset != 0 but manifest has no METATILE_PACK_BASE_OFFSET "
                "(legacy bundle); metatile directory u32s assume pack-base 0 — rebuild the companion "
                "bundle with --metatile-pack-base-offset matching the splice offset.\n",
                file=sys.stderr,
            )

    if want_bash:
        print("# Bash / sh - copy into shell before launching SDL portable build")
        print(f"export FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF={hx(abs_meta)}")
        print(f"export FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF={hx(abs_dim)}")
        print(f"export FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF={hx(abs_ts)}")
        print()

    if want_ps:
        print("# PowerShell")
        print(f'$env:FIRERED_ROM_MAP_LAYOUT_METATILES_DIRECTORY_OFF = "{hx(abs_meta)}"')
        print(f'$env:FIRERED_ROM_MAP_LAYOUT_DIMENSIONS_DIRECTORY_OFF = "{hx(abs_dim)}"')
        print(f'$env:FIRERED_ROM_MAP_LAYOUT_TILESET_INDICES_DIRECTORY_OFF = "{hx(abs_ts)}"')
        print()

    if want_insp:
        print("# rom_blob_inspect.py - first 64 bytes at each directory base (repo root as cwd)")
        print(
            f'python tools/rom_blob_inspect.py "{rom_s}" -o {hx(abs_meta)} -n 64 --hexdump'
        )
        print(
            f'python tools/rom_blob_inspect.py "{rom_s}" -o {hx(abs_dim)} -n 64 --hexdump'
        )
        print(
            f'python tools/rom_blob_inspect.py "{rom_s}" -o {hx(abs_ts)} -n 64 --hexdump'
        )
        print()
        print("# Re-validate the host ROM slice with the same offsets (optional)")
        print(
            f'python tools/portable_generators/validate_map_layout_rom_packs.py "{rom_s}" \\'
        )
        print(f"  --metatiles-off {hx(abs_meta)} \\")
        print(f"  --dimensions-off {hx(abs_dim)} \\")
        print(f"  --tileset-indices-off {hx(abs_ts)} \\")
        print("  --layouts-json data/layouts/layouts.json --validate-block-words")
        print()

    print("# ---")
    print(f"# manifest: {man}")
    print(f"# bundle_base_offset={hx(b)}")
    print(
        f"# abs: METATILES_DIRECTORY={hx(abs_meta)} DIMENSIONS={hx(abs_dim)} "
        f"TILESET_INDICES={hx(abs_ts)}"
    )
    print(
        f"# part_bytes: meta={d['METATILES_BYTES']} dim={d['DIMENSIONS_BYTES']} "
        f"ts={d['TILESET_INDICES_BYTES']} total={d['TOTAL_BYTES']}"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
