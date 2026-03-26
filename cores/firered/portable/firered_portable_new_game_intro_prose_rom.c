#include "portable/firered_portable_new_game_intro_prose_rom.h"

#include "portable/firered_portable_rom_asset_bind.h"
#include "portable/firered_portable_rom_compat.h"

#include "characters.h"
#include "engine_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef PORTABLE

bool8 firered_portable_new_game_intro_prose_rom_should_run(void)
{
    return FALSE;
}

unsigned firered_portable_new_game_intro_prose_rom_page_count(void)
{
    return 0u;
}

const u8 *firered_portable_new_game_intro_prose_rom_get_page(unsigned page_index)
{
    (void)page_index;
    return NULL;
}

void firered_portable_new_game_intro_prose_rom_refresh_after_rom_load(void)
{
}

#else

static u8 sProseResolved;
static u8 sProseActive;
static unsigned sProsePageCount;
static const u8 *sProsePages[FIRERED_NEW_GAME_INTRO_PROSE_MAX_PAGES];

typedef struct FireredRomIntroProseProfileRow {
    uint32_t rom_crc32;
    const char *profile_id;
    size_t ptr_table_file_off;
    unsigned page_count;
} FireredRomIntroProseProfileRow;

/* Goldens: tools/build_offline_layout_prose_fixture.py (offline layout + minimal prose table). */
static const FireredRomIntroProseProfileRow s_intro_prose_profile_rows[] = {
    { 0u, "__firered_builtin_intro_prose_profile_none__", 0u, 0u },
    { 0x217A04B1u, NULL, 0x80006u, 3u },
    /* Table at EOF after in-memory splice (see firered_portable_rom_auto_prepare.c). */
    { 0x3479F247u, NULL, 0x1079AD2u, 3u },
};

static int prose_string_terminates_in_rom(const u8 *rom, size_t rom_size, size_t str_off, size_t cap)
{
    size_t i;

    if (rom == NULL || str_off >= rom_size || cap == 0u)
        return 0;
    for (i = 0u; i < cap && str_off + i < rom_size; i++)
    {
        if (rom[str_off + i] == EOS)
            return 1;
    }
    return 0;
}

static void prose_try_resolve(void)
{
    size_t rom_size;
    const u8 *rom;
    size_t table_off;
    size_t count_sz;
    unsigned n;
    unsigned i;

    if (sProseResolved)
        return;
    sProseResolved = 1;
    sProseActive = FALSE;
    sProsePageCount = 0u;
    (void)memset(sProsePages, 0, sizeof(sProsePages));

    if (firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_NEW_GAME_INTRO_PROSE_PTR_TABLE_OFF", &table_off))
    {
        if (!firered_portable_rom_asset_parse_hex_env("FIRERED_ROM_NEW_GAME_INTRO_PROSE_PAGE_COUNT", &count_sz))
            return;
        n = (unsigned)count_sz;
    }
    else
    {
        const FireredRomCompatInfo *c;
        size_t k;
        u8 matched;

        c = firered_portable_rom_compat_get();
        if (c == NULL)
            return;

        matched = 0;
        for (k = 0u; k < NELEMS(s_intro_prose_profile_rows); k++)
        {
            const FireredRomIntroProseProfileRow *r = &s_intro_prose_profile_rows[k];

            if (r->rom_crc32 == 0u && r->profile_id == NULL)
                continue;
            if (r->rom_crc32 != 0u && r->rom_crc32 != c->rom_crc32)
                continue;
            if (r->profile_id != NULL
                && (c->profile_id == NULL || strcmp(r->profile_id, (const char *)c->profile_id) != 0))
                continue;
            if (r->page_count == 0u || r->page_count > FIRERED_NEW_GAME_INTRO_PROSE_MAX_PAGES)
                continue;

            table_off = r->ptr_table_file_off;
            n = r->page_count;
            matched = 1;
            break;
        }
        if (matched == 0)
            return;
    }

    if (n == 0u || n > FIRERED_NEW_GAME_INTRO_PROSE_MAX_PAGES)
        return;

    rom_size = engine_memory_get_loaded_rom_size();
    rom = (const u8 *)(uintptr_t)ENGINE_ROM_ADDR;
    if (rom == NULL || rom_size < 0x200u)
        return;
    if (table_off > rom_size || table_off + (size_t)n * sizeof(u32) > rom_size)
        return;

    for (i = 0u; i < n; i++)
    {
        size_t word_off = table_off + (size_t)i * sizeof(u32);
        u32 p = (u32)rom[word_off] | ((u32)rom[word_off + 1] << 8) | ((u32)rom[word_off + 2] << 16)
            | ((u32)rom[word_off + 3] << 24);
        size_t fo;

        if ((p >> 24) != 0x08u)
            return;
        fo = (size_t)(p - 0x08000000u);
        if (fo >= rom_size)
            return;
        if (!prose_string_terminates_in_rom(rom, rom_size, fo, FIRERED_NEW_GAME_INTRO_PROSE_MAX_STR))
            return;
        sProsePages[i] = rom + fo;
    }

    sProsePageCount = n;
    sProseActive = TRUE;
}

void firered_portable_new_game_intro_prose_rom_refresh_after_rom_load(void)
{
    sProseResolved = 0;
    sProseActive = FALSE;
    sProsePageCount = 0u;
    (void)memset(sProsePages, 0, sizeof(sProsePages));
}

bool8 firered_portable_new_game_intro_prose_rom_should_run(void)
{
    prose_try_resolve();
    return (bool8)(sProseActive && sProsePageCount > 0u);
}

unsigned firered_portable_new_game_intro_prose_rom_page_count(void)
{
    prose_try_resolve();
    return sProsePageCount;
}

const u8 *firered_portable_new_game_intro_prose_rom_get_page(unsigned page_index)
{
    prose_try_resolve();
    if (!sProseActive || page_index >= sProsePageCount)
        return NULL;
    return sProsePages[page_index];
}

#endif /* PORTABLE */
