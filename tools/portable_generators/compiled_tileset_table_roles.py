"""
Offline helpers: **`gFireredPortableCompiledTilesetTable`** order + **`Tileset::isSecondary`**
from generated / hand-maintained C sources.

Used by **`validate_compiled_tileset_asset_profile_directory.py`** (Direction D role-based
**`tile_uncomp_bytes`** caps). Keep paths in sync with **`generate_portable_map_data.py`** output.
"""

from __future__ import annotations

import re
from pathlib import Path


def parse_compiled_tileset_table_order(c_source: str) -> list[str]:
    """
    Return symbol names (without leading &) in **`gFireredPortableCompiledTilesetTable[]`** order.
    Stops at the closing **`};`** of the initializer.
    """
    lines = c_source.splitlines()
    start = None
    for i, line in enumerate(lines):
        if "gFireredPortableCompiledTilesetTable[]" in line and "=" in line:
            start = i
            break
    if start is None:
        raise ValueError("gFireredPortableCompiledTilesetTable[] initializer not found")

    syms: list[str] = []
    for line in lines[start + 1 :]:
        s = line.strip()
        if s.startswith("};"):
            break
        if not s or s.startswith("//"):
            continue
        m = re.search(r"&\s*(gTileset_\w+)", s)
        if m:
            syms.append(m.group(1))
            continue
        if "NULL" in s:
            syms.append("NULL")
    return syms


def parse_tileset_is_secondary(headers_source: str) -> dict[str, bool]:
    """
    Map **`gTileset_*`** symbol → **True** if `.isSecondary = TRUE`, else **False**.
    """
    out: dict[str, bool] = {}
    for m in re.finditer(
        r"const\s+struct\s+Tileset\s+(gTileset_\w+)\s*=\s*\{([^}]*)\};",
        headers_source,
        re.MULTILINE | re.DOTALL,
    ):
        body = m.group(2)
        if re.search(r"\.isSecondary\s*=\s*TRUE\b", body):
            out[m.group(1)] = True
        elif re.search(r"\.isSecondary\s*=\s*FALSE\b", body):
            out[m.group(1)] = False
        else:
            raise ValueError(f"{m.group(1)}: missing .isSecondary = TRUE/FALSE")
    return out


def tile_uncomp_cap_per_table_index(
    table_symbols: list[str],
    is_secondary_by_tileset: dict[str, bool],
    *,
    pri_cap: int,
    sec_cap: int,
    combined_max: int,
) -> list[int]:
    """
    Per compiled-table index, max allowed **`tile_uncomp_bytes`** (0 means “skip check” for that row).
    **NULL** entries use **`combined_max`**.
    Unknown symbols raise **ValueError**.
    """
    caps: list[int] = []
    for sym in table_symbols:
        if sym == "NULL":
            caps.append(combined_max)
            continue
        if sym not in is_secondary_by_tileset:
            raise ValueError(f"tileset {sym} not found in headers (typo or stale map_data_portable.c?)")
        caps.append(sec_cap if is_secondary_by_tileset[sym] else pri_cap)
    return caps


def default_map_data_portable_path(repo_root: Path) -> Path:
    return repo_root / "pokefirered_core" / "generated" / "src" / "data" / "map_data_portable.c"


def default_tileset_headers_path(repo_root: Path) -> Path:
    return repo_root / "src" / "data" / "tilesets" / "headers.h"


def build_role_tile_caps_for_enforcement(
    *,
    count: int,
    repo_root: Path,
    compiled_tileset_table_c: Path | None = None,
    tileset_headers: Path | None = None,
    pri_cap: int | None = None,
    sec_cap: int | None = None,
) -> tuple[list[int], list[str]]:
    """
    Resolve per-index **`tile_uncomp_bytes`** / raw-blob ceilings for **`gFireredPortableCompiledTilesetTable`**
    order. Raises **FileNotFoundError**, **ValueError** on mismatch / parse errors.
    """
    from map_visual_policy_constants import (
        DEFAULT_PRI_UNCOMPRESSED_BYTES,
        DEFAULT_SEC_UNCOMPRESSED_BYTES,
        MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES,
    )

    table_c = compiled_tileset_table_c if compiled_tileset_table_c is not None else default_map_data_portable_path(repo_root)
    headers_p = tileset_headers if tileset_headers is not None else default_tileset_headers_path(repo_root)
    if not table_c.is_file():
        raise FileNotFoundError(str(table_c))
    if not headers_p.is_file():
        raise FileNotFoundError(str(headers_p))
    syms = parse_compiled_tileset_table_order(table_c.read_text(encoding="utf-8"))
    if len(syms) != count:
        raise ValueError(
            f"compiled table has {len(syms)} entries but --count {count} "
            "(fix --count or regenerate map_data_portable.c)"
        )
    roles = parse_tileset_is_secondary(headers_p.read_text(encoding="utf-8"))
    pc = pri_cap if pri_cap is not None else DEFAULT_PRI_UNCOMPRESSED_BYTES
    sc = sec_cap if sec_cap is not None else DEFAULT_SEC_UNCOMPRESSED_BYTES
    caps = tile_uncomp_cap_per_table_index(
        syms,
        roles,
        pri_cap=pc,
        sec_cap=sc,
        combined_max=MAP_BG_FIELD_TILESET_CHAR_VRAM_BYTES,
    )
    return caps, syms
