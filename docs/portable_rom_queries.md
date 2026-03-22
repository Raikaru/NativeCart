# Portable ROM queries (hack-agnostic)

`cores/firered/portable/firered_portable_rom_queries.c` exposes **read-only**
introspection of the **currently loaded** ROM image in portable builds:

| API | Meaning |
|-----|--------|
| `firered_portable_rom_crc32_ieee_full()` | CRC-32 of the **entire** ROM (IEEE / reflected, poly `0xEDB88320`, same as BPS tooling in-tree). |
| `firered_portable_rom_queries_copy_cart_title12()` | Internal cart title at **0xA0** (12 bytes). |
| `firered_portable_rom_queries_copy_game_code4()` | Game code at **0xAC** (4 bytes, e.g. `BPRE`). |
| `firered_portable_rom_queries_software_version()` | Byte at **0xBC**. |

Results are **cached** until `firered_portable_rom_queries_invalidate_cache()` runs.
The engine calls invalidate once per successful `engine_backend_init()` (after
`firered_portable_sync_rom_header_from_cartridge()`), so each ROM load gets a
fresh CRC.

## Using this for hacks

- Compare `firered_portable_rom_crc32_ieee_full()` to known vanilla or hack
  build CRCs in **your** feature code or external tooling—avoid baking one
  hack’s menu or Key UI into the core runtime.
- Pair with `RomHeaderGameCode` / `RomHeaderSoftwareVersion` (synced from the
  same header) when you only need the standard fields without scanning the ROM.
- For a **cached compatibility snapshot** (kind, flags, tiny known-profile
  table), use `firered_portable_rom_compat_*` — see `docs/portable_rom_compat.md`.

## Optional main-menu trace

With `FIRERED_TRACE_MAIN_MENU=1`, `src_transformed/main_menu.c` logs a single
line with ROM CRC and header summary (debug / SDL builds). This is generic and
not tied to a specific hack.
