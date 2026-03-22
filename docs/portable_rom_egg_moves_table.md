# Portable ROM: egg moves (`gEggMoves`)

FireRed **`gEggMoves`** is a **little-endian `u16` stream**: blocks of **`(species + 20000)`** then move IDs until the next species header or the final **`0xFFFF`** terminator. See **`tools/rom_blob_inspect.py`** **`--validate-egg-moves-u16`** (same rules as **`validate_egg_moves_u16_stream`** in Python).

## Runtime pack (**`FIRERED_ROM_EGG_MOVES_PACK_OFF`**)

Resolved like other portable tables (**env hex** → **`FireredRomU32TableProfileRow`** with placeholder **`__firered_builtin_egg_moves_profile_none__`**).

| Offset | Size | Content |
|--------|------|---------|
| **+0** | **4** | Magic **`0x45474745`** (`EGGE` LE) |
| **+4** | **4** | Version **`1`** (LE **`u32`**) |
| **+8** | **4** | **`byte_len`** (LE **`u32`**, even, **≤ 131072**) |
| **+12** | **`byte_len`** | LE **`u16`** stream; must pass the same structural checks as **`--validate-egg-moves-u16`** (**`NUM_SPECIES`**, **`MOVES_COUNT`**, **`EGG_MOVES_SPECIES_OFFSET` 20000**). |

**Refresh:** **`firered_portable_rom_egg_moves_table_refresh_after_rom_load()`** from **`engine_backend_init`** (after TM/HM learnsets, before evolution table).

**Call sites:** **`GetEggMoves`** in **`daycare.c`** uses **`FireredEggMovesTable()`** and **`FireredEggMovesTableWordCount()`** instead of **`gEggMoves`** / **`NELEMS(gEggMoves)`**.

**Fallback:** Invalid pack or unset offset → compiled **`gEggMoves`** (vanilla behavior).
