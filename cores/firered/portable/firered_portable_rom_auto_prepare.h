#ifndef GUARD_FIRERED_PORTABLE_ROM_AUTO_PREPARE_H
#define GUARD_FIRERED_PORTABLE_ROM_AUTO_PREPARE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Optional **in-memory ROM preparation** before `engine_memory_init` (PORTABLE).
 *
 * When the loaded image matches a small built-in table (exact IEEE CRC32 + optional size),
 * splice the map layout companion bundle at a fixed offset and optionally append the generic
 * intro-prose pointer table so existing **profile rows** (CRC-keyed) activate without env vars.
 *
 * Returns 0: caller uses the original `rom` / `rom_size`.
 * Returns 1: `*out_rom` is a malloc'd buffer of `*out_size` bytes; caller must `free(*out_rom)`.
 */
int firered_portable_rom_auto_prepare(const uint8_t *rom, size_t rom_size, uint8_t **out_rom, size_t *out_size);

#endif /* GUARD_FIRERED_PORTABLE_ROM_AUTO_PREPARE_H */
