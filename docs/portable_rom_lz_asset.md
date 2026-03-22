# ROM LZ77 asset helpers (portable)

`cores/firered/portable/firered_portable_rom_lz_asset.{h,c}` provide **generic**
utilities for ROM-backed **GBA LZ77 (type `0x10`)** blobs:

- `firered_portable_rom_lz77_compressed_size` — walk a stream and return total
  compressed length, or `0` if invalid / truncated (aligned with
  `engine_lz77_uncomp` behavior).
- `firered_portable_rom_find_lz_followup` — after a parent compressed asset
  (e.g. tiles), search a bounded region for a **follow-up** LZ blob: first a
  strict `memcmp` signature pass (4-byte step), then a relaxed pass matching
  declared decompressed size + parseable stream.

Callers supply signatures and expected relaxed decompressed sizes; the module
does not hard-code one hack or screen.

## Consumers

- Title screen ROM binders (`firered_portable_title_screen_rom.c`) use both APIs
  for game logo and copyright / PRESS START tilemap discovery after tiles.

- Oak speech main BG (`firered_portable_oak_speech_bg_rom.c`) — `oak_speech_bg`
  tiles LZ then tilemap LZ follow-up.

## Non-PORTABLE builds

Stubs return `0` / `NULL` — only the portable/SDL path links real
implementations.

## Related

- Optional scan policy vs compiled mirrors:
  `firered_portable_rom_compat_should_attempt_rom_asset_scan()` in
  [`portable_rom_compat.md`](portable_rom_compat.md).

- Shared env → scan → fallback **flow** (not LZ math):
  [`portable_rom_asset_bind.md`](portable_rom_asset_bind.md).
