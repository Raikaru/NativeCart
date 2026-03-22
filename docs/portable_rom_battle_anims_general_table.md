# ROM-backed general battle animation script pointer table (PORTABLE / SDL)

## Purpose

`gBattleAnims_General` holds **28** pointers to battle animation bytecode (`General_*` scripts, `B_ANIM_CASTFORM_CHANGE` … `B_ANIM_SAFARI_REACTION`). Portable builds mirror this in `battle_anim_scripts_portable_data.c`.

When a hack **relocates** these scripts in ROM but keeps **retail-style** animation command streams (ROM addresses / existing `firered_portable_resolve_script_ptr` behavior for embedded tokens), you can point the engine at the **cartridge** pointer block instead of the compiled table.

## Configuration

1. **`FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF`** — hex file offset to **28** consecutive little-endian `u32` GBA ROM addresses (script starts), same order as `gBattleAnims_General` in `constants/battle_anim.h` (`B_ANIM_*` 0–27).
2. **Profile rows** (optional): `s_battle_anims_general_profile_rows` in `firered_portable_rom_battle_anims_general_table.c`.

## Runtime

- **`LaunchBattleAnimation`** (PORTABLE): if `animsTable == gBattleAnims_General`, tries ROM; on failure uses compiled `gBattleAnims_General[tableId]`.
- Move animations (`gBattleAnims_Moves`) and other tables are unchanged.

## Testing (env override)

- **Vanilla / default:** do **not** set the variable → helper returns NULL → compiled `gBattleAnims_General` is used (same as before this seam).
- **ROM table:** set the hex **file offset** (not a GBA bus address) to the first byte of the 28×`u32` block in the **loaded** `.gba` image.

PowerShell example (adjust offset after extracting the table from your ROM, e.g. via sym/map or a script):

```powershell
$env:FIRERED_ROM_BATTLE_ANIMS_GENERAL_PTRS_OFF = "0x12345678"
.\decomp_engine_sdl.exe
```

Syntax matches other `FIRERED_ROM_*_OFF` vars (`strtoul`-style hex). If the env is set but the table is OOB, a word is zero, or a pointer is not in loaded ROM, that entry falls back to the compiled pointer.

## Related

- `firered_portable_rom_u32_table.{h,c}` — shared env/profile + bounds + LE read.
- `docs/rom_backed_runtime.md` §6e.
- **`0x84000000` resolver tokens** use **`FIRERED_ROM_BATTLE_ANIM_PTRS_OFF`** (different table) — `docs/portable_rom_battle_anim_pointer_overlay.md`.
