#ifndef GUARD_FIRERED_PORTABLE_ROM_U32_TABLE_H
#define GUARD_FIRERED_PORTABLE_ROM_U32_TABLE_H

#include "global.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Shared plumbing for small **ROM-backed u32[]** pointer tables (env + profile + LE read).
 * Family modules keep pointer **semantics** (ROM script vs EWRAM, etc.).
 */

#define FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES 0x200u

typedef struct FireredRomU32TableProfileRow {
    uint32_t rom_crc32;
    const char *profile_id;
    size_t table_file_off;
} FireredRomU32TableProfileRow;

bool8 firered_portable_rom_u32_table_profile_lookup(const FireredRomU32TableProfileRow *rows, size_t row_count,
    size_t *out_off);

/* env hex first (firered_portable_rom_asset_parse_hex_env), then profile table. */
bool8 firered_portable_rom_u32_table_resolve_base_off(const char *env_var_name,
    const FireredRomU32TableProfileRow *profile_rows, size_t profile_row_count, size_t *out_off);

/*
 * Read one little-endian u32 from rom + table_off + 4*entry_index.
 * Fails if rom is NULL, rom_size < FIRERED_ROM_U32_TABLE_MIN_ROM_BYTES, entry_index >= entry_count,
 * or the full table would extend past rom_size.
 */
bool8 firered_portable_rom_u32_table_read_entry(size_t rom_size, const u8 *rom, size_t table_off, size_t entry_count,
    size_t entry_index, u32 *out_word);

#endif /* GUARD_FIRERED_PORTABLE_ROM_U32_TABLE_H */
