# Portable ROM compatibility profile (generic hacks)

`cores/firered/portable/firered_portable_rom_compat.c` builds a **small cached
snapshot** after each ROM load, on top of `firered_portable_rom_queries`
(CRC-32, cart title, game code, version).

## API

| Function | Role |
|----------|------|
| `firered_portable_rom_compat_refresh_after_rom_load()` | Recompute snapshot (called from `engine_backend_init` after query cache invalidate). |
| `firered_portable_rom_compat_get()` | `const FireredRomCompatInfo *` — kind, flags, `profile_id`, `display_label`, CRC, size, header bytes. |
| `firered_portable_rom_compat_format_summary(buf, cap)` | One-line string for tools / UI. |
| `firered_portable_rom_compat_should_attempt_rom_asset_scan()` | Whether optional ROM LZ / signature scans should run (`FALSE` only for vanilla 16 MiB retail-like FireRed; override `FIRERED_ROM_ASSET_SCAN_FORCE=1`). |

## `FireredRomCompatKind`

- `FIRERED_ROM_COMPAT_KIND_VANILLA_FIRE_RED` — matched **known** retail-like BPRE/BPGE + title needle (see table), **16 MiB** image.
- `FIRERED_ROM_COMPAT_KIND_KNOWN_FIRE_RED_HACK` — matched a **named** row (e.g. FRLG+, Unbound).
- `FIRERED_ROM_COMPAT_KIND_UNKNOWN_PATCHED` — BPRE/BPGE but no table hit, or expanded ROM with stock-like header.
- `FIRERED_ROM_COMPAT_KIND_NONSTANDARD` — image too small or game code not BPRE/BPGE.

## Flags (bitmask)

- `FIRERED_ROM_COMPAT_FLAG_VANILLA_LIKE`
- `FIRERED_ROM_COMPAT_FLAG_EXPANDED_ROM` — size &gt; 16 MiB
- `FIRERED_ROM_COMPAT_FLAG_KNOWN_PROFILE`
- `FIRERED_ROM_COMPAT_FLAG_REQUIRES_RUNTIME_SHIMS`
- `FIRERED_ROM_COMPAT_FLAG_LIKELY_DECOMP_INCOMPATIBLE`

Future ROM-backed features should branch on **kind + flags**, not raw strings.

## Built-in known profiles (tiny, local table)

Rows are ordered **most specific first**. `crc32 == 0` means “ignore CRC” (add
exact CRC later when you want a stricter match).

| `profile_id` | Match hint | Notes |
|--------------|------------|--------|
| `frlg_plus` | `FRLG` in title, BPRE or BPGE | Known derivative. |
| `unbound` | `UNBO` or `UNBD` in title, BPRE | Common Unbound title patterns. |
| `vanilla_fire_red_us` | `FIRE` in title, BPRE | Stock-like US FireRed. |
| `vanilla_leaf_green_us` | `LEAF` in title, BPGE | Stock-like US LeafGreen. |

If a row matches **vanilla** but the ROM is **expanded** (&gt; 16 MiB), the
result is **upgraded** to `UNKNOWN_PATCHED` with profile
`expanded_stock_like_header` — avoids calling a 32 MiB hack “vanilla”.

## Debug trace

Set **`FIRERED_TRACE_ROM_COMPAT=1`** to emit one line via
`engine_backend_trace_external` after each refresh (profile id, kind, flags,
CRC, size, version).

## Consumers (examples)

- **Title screen copyright / “PRESS START” strip (BG2)** —
  `firered_portable_title_screen_try_bind_copyright_press_start` skips the ROM
  LZ signature scan when the profile is **vanilla 16 MiB retail-like** and uses
  compiled `*_Portable` graphics instead. Non-vanilla / expanded / unknown
  BPRE-family ROMs attempt ROM-backed tiles+map (env overrides or scan). Force
  scan on vanilla for debugging: `FIRERED_ROM_COPYRIGHT_PRESS_START_SCAN=1` or
  global force `FIRERED_ROM_ASSET_SCAN_FORCE=1`.

- **Shared LZ follow-up helpers** — see [`portable_rom_lz_asset.md`](portable_rom_lz_asset.md).

- **Oak speech main BG (oak_speech_bg tiles + map)** —
  [`portable_rom_oak_speech_bg.md`](portable_rom_oak_speech_bg.md); same scan
  policy as other optional ROM asset scans. Implemented via
  [`portable_rom_asset_bind.md`](portable_rom_asset_bind.md).

- **Shared bind flow** — [`portable_rom_asset_bind.md`](portable_rom_asset_bind.md).

## See also

- [`cfru_integration_playbook.md`](cfru_integration_playbook.md) — CFRU / CFRU-derivative **native** port planning (battle, save, scripts, data width); `profile_id` rows here are **inputs** to that work, not implementations.
- [`cfru_upstream_reference.md`](cfru_upstream_reference.md) — CFRU/DPE repos, wiki, ROM insertion build model, README terms pointer.

## Non-goals

- No remote catalog, no large registry.
- No hack-specific UI in this module — only structured data + optional trace.
