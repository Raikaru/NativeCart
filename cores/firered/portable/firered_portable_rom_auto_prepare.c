#include "global.h"

#include "portable/firered_portable_rom_auto_prepare.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORTABLE

int firered_portable_rom_auto_prepare(const uint8_t *rom, size_t rom_size, uint8_t **out_rom, size_t *out_size)
{
    (void)rom;
    (void)rom_size;
    (void)out_rom;
    (void)out_size;
    return 0;
}

#else

extern char *getenv(const char *name);

#define FIRERED_AUTO_PREP_BUNDLE_READ_MAX (1024u * 1024u)

/* Pokemon Fire Red Omega.gba (16 MiB) — IEEE CRC of file before prep (see tools/capture_portable_rom_profile.py). */
#define FIRERED_AUTO_PREP_OMEGA_16M_CRC32 0xB197A9B9u
#define FIRERED_AUTO_PREP_OMEGA_SPLICE_P 0x01000000u

/* Intro prose string file offsets in the **original** Omega image (docs/rom_backed_runtime.md §8c). */
#define FIRERED_AUTO_PREP_OMEGA_PROSE_STR0 0x71A78Du
#define FIRERED_AUTO_PREP_OMEGA_PROSE_STR1 0x71AA66u
#define FIRERED_AUTO_PREP_OMEGA_PROSE_STR2 0x71A972u

#define GBA_ROM_LOAD_ADDR 0x08000000u

static uint32_t crc32_ieee_full(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;
    int j;

    for (i = 0u; i < size; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc ^ 0xFFFFFFFFu;
}

static int trace_auto_prep(void)
{
    const char *e = getenv("FIRERED_TRACE_ROM_AUTO_PREPARE");

    return (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0);
}

static int auto_prep_disabled(void)
{
    const char *e = getenv("FIRERED_ROM_AUTO_PREPARE_DISABLE");

    return (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0);
}

static int skip_intro_prose_append(void)
{
    const char *e = getenv("FIRERED_AUTO_PREPARE_SKIP_INTRO_PROSE");

    return (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0);
}

static FILE *try_open_omega_bundle(char *path_buf, size_t path_buf_sz)
{
    static const char rel_candidates[][96] = {
        "cores/firered/portable/data/omega_map_layout_rom_companion_bundle.bin",
        "../cores/firered/portable/data/omega_map_layout_rom_companion_bundle.bin",
        "../../cores/firered/portable/data/omega_map_layout_rom_companion_bundle.bin",
        "build/omega_map_layout_rom_companion_bundle.bin",
        "../build/omega_map_layout_rom_companion_bundle.bin",
    };
    const char *e;
    FILE *fp;
    size_t i;

    e = getenv("FIRERED_OMEGA_LAYOUT_COMPANION_BUNDLE_PATH");
    if (e != NULL && e[0] != '\0')
    {
        snprintf(path_buf, path_buf_sz, "%s", e);
        fp = fopen(path_buf, "rb");
        if (fp != NULL)
            return fp;
    }

    e = getenv("FIRERED_ASSET_ROOT");
    if (e != NULL && e[0] != '\0')
    {
        snprintf(path_buf, path_buf_sz, "%s/cores/firered/portable/data/omega_map_layout_rom_companion_bundle.bin", e);
        fp = fopen(path_buf, "rb");
        if (fp != NULL)
            return fp;
        snprintf(path_buf, path_buf_sz, "%s/build/omega_map_layout_rom_companion_bundle.bin", e);
        fp = fopen(path_buf, "rb");
        if (fp != NULL)
            return fp;
    }

    for (i = 0u; i < NELEMS(rel_candidates); i++)
    {
        snprintf(path_buf, path_buf_sz, "%s", rel_candidates[i]);
        fp = fopen(path_buf, "rb");
        if (fp != NULL)
            return fp;
    }

    return NULL;
}

static int try_omega_16m_layout_and_prose(const uint8_t *rom, size_t rom_size, uint8_t **out_rom, size_t *out_size)
{
    uint32_t crc;
    FILE *bf;
    char path_try[512];
    long bundle_sz_long;
    size_t bundle_sz;
    size_t bundle_end;
    size_t image_len;
    size_t final_len;
    size_t table_off;
    uint8_t *buf;
    size_t i;
    static const size_t prose_offs[3] = {
        FIRERED_AUTO_PREP_OMEGA_PROSE_STR0,
        FIRERED_AUTO_PREP_OMEGA_PROSE_STR1,
        FIRERED_AUTO_PREP_OMEGA_PROSE_STR2,
    };

    if (rom == NULL || out_rom == NULL || out_size == NULL)
        return 0;

    *out_rom = NULL;
    *out_size = 0u;

    if (rom_size != (size_t)FIRERED_AUTO_PREP_OMEGA_SPLICE_P)
        return 0;

    crc = crc32_ieee_full(rom, rom_size);
    if (crc != FIRERED_AUTO_PREP_OMEGA_16M_CRC32)
        return 0;

    path_try[0] = '\0';
    bf = try_open_omega_bundle(path_try, sizeof(path_try));
    if (bf == NULL)
    {
        if (trace_auto_prep())
            fprintf(stderr, "RomAutoPrep: Omega CRC match but bundle not found (set FIRERED_ASSET_ROOT or FIRERED_OMEGA_LAYOUT_COMPANION_BUNDLE_PATH)\n");
        return 0;
    }

    if (fseek(bf, 0, SEEK_END) != 0)
    {
        fclose(bf);
        return 0;
    }
    bundle_sz_long = ftell(bf);
    if (bundle_sz_long <= 0 || (unsigned long)bundle_sz_long > FIRERED_AUTO_PREP_BUNDLE_READ_MAX)
    {
        fclose(bf);
        return 0;
    }
    bundle_sz = (size_t)bundle_sz_long;
    if (fseek(bf, 0, SEEK_SET) != 0)
    {
        fclose(bf);
        return 0;
    }

    bundle_end = (size_t)FIRERED_AUTO_PREP_OMEGA_SPLICE_P + bundle_sz;
    image_len = rom_size > bundle_end ? rom_size : bundle_end;
    final_len = image_len;
    if (skip_intro_prose_append() == 0)
        final_len += sizeof(uint32_t) * 3u;

    for (i = 0u; i < NELEMS(prose_offs); i++)
    {
        if (prose_offs[i] >= rom_size)
        {
            fclose(bf);
            if (trace_auto_prep())
                fprintf(stderr, "RomAutoPrep: prose string offset OOB\n");
            return 0;
        }
    }

    buf = (uint8_t *)calloc(final_len, 1u);
    if (buf == NULL)
    {
        fclose(bf);
        return 0;
    }

    memcpy(buf, rom, rom_size);
    if (fread(buf + (size_t)FIRERED_AUTO_PREP_OMEGA_SPLICE_P, 1u, bundle_sz, bf) != bundle_sz)
    {
        free(buf);
        fclose(bf);
        return 0;
    }
    fclose(bf);

    if (skip_intro_prose_append() == 0)
    {
        table_off = image_len;
        for (i = 0u; i < NELEMS(prose_offs); i++)
        {
            uint32_t w = (uint32_t)(GBA_ROM_LOAD_ADDR + prose_offs[i]);
            size_t o = table_off + i * sizeof(uint32_t);

            buf[o + 0u] = (uint8_t)(w & 0xFFu);
            buf[o + 1u] = (uint8_t)((w >> 8) & 0xFFu);
            buf[o + 2u] = (uint8_t)((w >> 16) & 0xFFu);
            buf[o + 3u] = (uint8_t)((w >> 24) & 0xFFu);
        }
    }

    *out_rom = buf;
    *out_size = final_len;

    if (trace_auto_prep())
    {
        uint32_t out_crc = crc32_ieee_full(buf, final_len);

        fprintf(
            stderr,
            "RomAutoPrep: Omega 16MiB -> prepared size=%zu crc32=0x%08X bundle=%s prose=%s\n",
            final_len,
            (unsigned)out_crc,
            path_try[0] != '\0' ? path_try : "(env path)",
            skip_intro_prose_append() != 0 ? "off" : "on");
    }

    return 1;
}

int firered_portable_rom_auto_prepare(const uint8_t *rom, size_t rom_size, uint8_t **out_rom, size_t *out_size)
{
    if (auto_prep_disabled())
        return 0;

    if (try_omega_16m_layout_and_prose(rom, rom_size, out_rom, out_size))
        return 1;

    return 0;
}

#endif /* PORTABLE */
