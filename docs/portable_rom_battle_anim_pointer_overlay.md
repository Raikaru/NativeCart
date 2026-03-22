# ROM-backed battle animation pointer overlay (`0x84000000` resolver)

## Purpose

Portable bytecode can encode **`0x84000000 + i`** → **`gFireredPortableBattleAnimPtrs[i]`** (`battle_anim_scripts_portable_data.c`). This overlay supplies an optional parallel **`u32[]`** of **GBA ROM addresses** so **`firered_portable_resolve_script_ptr`** can follow the **loaded cartridge** for relocated ROM-resident scripts or templates while keeping **compiled fallback** for host-only slots.

## Not `gBattleAnims_General` / `LaunchBattleAnimation`

| Mechanism | Env | Semantics |
|-----------|-----|-----------|
| **`LaunchBattleAnimation(..., gBattleAnims_General, id)`** | **`FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF`** | **28** entries, **`B_ANIM_*`** order (`docs/portable_rom_battle_anims_general_table.md`) |
| **This resolver overlay** | **`FIRERED_ROM_BATTLE_ANIM_PTRS_OFF`** | **`gFireredPortableBattleAnimPtrCount`** entries, **portable token** order (`generate_portable_battle_anim_data.py`) |

Do not point both env vars at the same offset unless a tool built **two** different aligned tables.

## Mixed table (host vs ROM)

`gFireredPortableBattleAnimPtrs` interleaves **`AnimTask_*` host functions**, **battle-anim script** labels (bytecode), **`&g*SpriteTemplate`** (often ROM templates), etc.

**Rule:** only **non-zero** **`0x08000000`–`0x09FFFFFF`** pointers **within the loaded ROM** are accepted. Anything else → **compiled** `gFireredPortableBattleAnimPtrs[i]`.

**Host `AnimTask_*` slots:** use ROM word **`0`** (or any non-ROM value) so the resolver keeps the **compiled** host pointer.

## Configuration

1. **`FIRERED_ROM_BATTLE_ANIM_PTRS_OFF`** — hex file offset to **`gFireredPortableBattleAnimPtrCount`** little-endian `u32` values, index-aligned with `gFireredPortableBattleAnimPtrs`.
2. **Profile rows** (optional): `s_battle_anim_ptr_overlay_profile_rows` in `firered_portable_rom_battle_anim_pointer_overlay.c`.

## Related

- `cores/firered/portable/firered_portable_rom_battle_anim_pointer_overlay.{h,c}`
- `cores/firered/portable/portable_script_pointer_resolver.c`
- `docs/rom_backed_runtime.md` §6e, §6l
