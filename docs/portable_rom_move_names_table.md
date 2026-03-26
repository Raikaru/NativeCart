# Portable ROM: move names (`gMoveNames`)

Compiled data: **`gMoveNames_Compiled[MOVES_COUNT][MOVE_NAME_LENGTH + 1]`** (`src/data/text/move_names.h`). Each row is a fixed-width GBA text buffer (**`MOVE_NAME_LENGTH + 1`** bytes, **`13`** in vanilla) containing an **`EOS` (`0xFF`)**-terminated name.

Portable access: **`include/data.h`** defines **`gMoveNames`** as a cast of **`FireredMoveNamesBytes()`** (see **`FireredMoveNamesBytes`** in `cores/firered/portable/firered_portable_rom_move_names_table.c`). Static initializers that require link-time constants (e.g. **`src/data/party_menu.h`** field-move labels, **`rom_header_gf`**) use **`gMoveNames_Compiled[...]`** directly so they stay valid at compile time.

## Pack (`FIRERED_ROM_MOVE_NAMES_PACK_OFF`)

Env hex + optional profile row **`__firered_builtin_move_names_profile_none__`**.

| Offset | Size | Field |
|--------|------|--------|
| **0** | **4** | Magic **`0x564D4E4D`** (`MNMV` LE) |
| **4** | **4** | Version **`1`** |
| **8** | **4** | **`rows`** — must equal **`MOVES_COUNT`** |
| **12** | **4** | **`row_bytes`** — must equal **`MOVE_NAME_LENGTH + 1`** (**13**) |
| **16** | **`rows × row_bytes`** | Contiguous row-major payload |

**Validation:** total size matches **`MOVES_COUNT × 13`**; every row contains at least one **`EOS`** byte in its **`row_bytes`**. On any failure, the cache stays inactive and **`FireredMoveNamesBytes()`** returns the compiled flat buffer.

**Refresh:** **`firered_portable_rom_move_names_table_refresh_after_rom_load()`** from **`engine_backend_init`** immediately after battle moves.

Offline: **`tools/portable_generators/validate_move_names_rom_pack.py`** (`tools/docs/README_validators.md`).
