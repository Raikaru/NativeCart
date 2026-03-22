# Next structural ROM seams (after `gBattleMoves`)

Portable already mirrors **`FIRERED_ROM_BATTLE_MOVES_TABLE_OFF`** (`MOVES_COUNT × 9` bytes, no pointers). The items below are **likely next** when you extend ROM-backed data—either because they sit in the same “flat data” family or because hacks often touch them.

Use **`tools/rom_blob_inspect.py`** for read-only slices before touching runtime. See **`docs/rom_blob_inspection_playbook.md`**.

## Checklist (audit order)

| Candidate | Shape | Tool / flags | Notes |
|-----------|--------|----------------|-------|
| **`gEggMoves[]`** | **Variable-length** `u16` stream: `(species + 20000)`, move IDs…, final **`0xFFFF`** | `--validate-egg-moves-u16` (optional `--egg-species-offset`, `--egg-num-species`, `--egg-moves-count`) | Matches `src/data/pokemon/egg_moves.h`. Wrong slice often fails header range or move-id bounds. |
| **Level-up learnsets** | Portable: **`FIRERED_ROM_LEVEL_UP_LEARNSETS_PACK_OFF`** — **`LUL1`** header + **`NUM_SPECIES`** LE **`u32`** blob-relative offsets + **`u16`** blob (**`LEVEL_UP_MOVE` / `LEVEL_UP_END`**). See **`docs/portable_rom_level_up_learnsets_family.md`**. | Native ROM: **`NUM_SPECIES` GBA `u32`** bus pointers + same **`u16`** encoding — `-n $((NUM_SPECIES*4)) --struct-stride 4 --gba-u32-pointers`. | Retail uses **`0x08……`** pointers; portable pack uses **offsets** into one blob to avoid half-migration. |
| **`gLevelUpLearnsets` target blobs** | Rows are **not** fixed width; each ends with **`0xFFFF`** | Scan candidate regions: count **`0xFFFF`** vs `NUM_SPECIES`, or diff against build. | Easy to grab wrong bank or a truncated dump. |
| **`gItems` / hold effects / friendship** | Often **structs** with **`u8`/`u16`** fields (price, hold effect, **friendship** delta, fling, pocket…) | `--u8-cols STRIDE` once you know **wire row size** for your fork; `--struct-stride` for alignment | **Friendship (FP)** and **evolution-by-friendship** depend on item rows matching the engine’s struct layout—forks that repack fields need a new stride and validators. |
| **`gStdScripts`** (already portable) | **9 × `u32`** script pointers | `--struct-stride 4`, `--gba-u32-pointers` | Listed here as the **small pointer table** pattern next to battle data in many maps. |

## Related docs (implemented slices)

- Battle moves: **`docs/portable_rom_battle_moves_table.md`**
- TM/HM masks, evolution, species info: **`docs/rom_backed_runtime.md`** §5k–§5m
- Std scripts / special vars: **`docs/portable_rom_std_scripts_table.md`**, **`docs/portable_rom_special_vars_table.md`**
