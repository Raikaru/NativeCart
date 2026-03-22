#ifndef GUARD_FIRERED_PORTABLE_ROM_HEADER_H
#define GUARD_FIRERED_PORTABLE_ROM_HEADER_H

/*
 * After engine_memory_init maps the loaded ROM at ENGINE_ROM_ADDR, refresh
 * RomHeaderGameCode and RomHeaderSoftwareVersion from cartridge offsets (same
 * layout as retail GBA header at 0x08000000).
 */
void firered_portable_sync_rom_header_from_cartridge(void);

#endif /* GUARD_FIRERED_PORTABLE_ROM_HEADER_H */
