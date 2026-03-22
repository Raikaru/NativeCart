# Oak speech main background (ROM-backed, portable)

`firered_portable_oak_speech_try_bind_main_bg()` in
`cores/firered/portable/firered_portable_oak_speech_bg_rom.c` (via
`firered_portable_rom_asset_bind_run`) resolves
`oak_speech_bg.4bpp.lz` + `oak_speech_bg.bin.lz` from the loaded ROM for the
Oak intro / naming-return BG1 path in `src_transformed/oak_speech.c`.

## Policy

- **Env first** (works on vanilla): `FIRERED_ROM_OAK_SPEECH_BG_TILES_OFF`,
  `FIRERED_ROM_OAK_SPEECH_BG_MAP_OFF` (hex, required together); optional
  `FIRERED_ROM_OAK_SPEECH_BG_PAL_OFF` (512-byte `bg_tiles`-style palette).
- **LZ scan** only if `firered_portable_rom_compat_should_attempt_rom_asset_scan()`
  is TRUE (vanilla 16 MiB retail-like skips scan; see
  [`portable_rom_compat.md`](portable_rom_compat.md)).
- Scan uses shared helpers from [`portable_rom_lz_asset.md`](portable_rom_lz_asset.md).

Stock ROM layout does **not** place `bg_tiles.gbapal` immediately before
`oak_speech_bg` tiles, so scan mode binds **tiles + map** and leaves palette to
compiled `*_Portable` unless `PAL_OFF` is set.

## Trace

`FIRERED_TRACE_OAK_BG_ROM=1` — stderr lines prefixed `[firered oak-bg-rom]`.
