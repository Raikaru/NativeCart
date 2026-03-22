#include "global.h"

#include "portable/firered_portable_rom_lz_asset.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PORTABLE

size_t firered_portable_rom_lz77_compressed_size(const u8 *src, size_t avail_in)
{
    (void)src;
    (void)avail_in;
    return 0u;
}

const u8 *firered_portable_rom_find_lz_followup(
    const u8 *region_start,
    size_t region_byte_len,
    size_t search_cap,
    const u8 *strict_sig,
    size_t strict_sig_len,
    u32 relaxed_decomp_size,
    FireredRomLzFollowupKind *kind_out)
{
    (void)region_start;
    (void)region_byte_len;
    (void)search_cap;
    (void)strict_sig;
    (void)strict_sig_len;
    (void)relaxed_decomp_size;
    if (kind_out != NULL)
        *kind_out = FIRERED_ROM_LZ_FOLLOWUP_NONE;
    return NULL;
}

#else

size_t firered_portable_rom_lz77_compressed_size(const u8 *src, size_t avail_in)
{
    u8 dest[65536];
    u32 remaining;
    u32 written = 0;
    const u8 *base = src;
    u8 header;

    if (avail_in < 4)
        return 0;

    header = *src++;
    if (header != 0x10)
        return 0;

    remaining = (u32)src[0] | ((u32)src[1] << 8) | ((u32)src[2] << 16);
    src += 3;

    if (remaining > sizeof(dest))
        return 0;

    while (written < remaining)
    {
        u8 flags;
        s32 i;

        if ((size_t)(src - base) >= avail_in)
            return 0;
        flags = *src++;

        for (i = 0; i < 8 && written < remaining; i++, flags <<= 1)
        {
            if (flags & 0x80)
            {
                u8 hi, lo;
                u32 count, disp;

                if ((size_t)(src - base) + 2 > avail_in)
                    return 0;
                hi = *src++;
                lo = *src++;
                count = (u32)(hi >> 4) + 3u;
                disp = ((((u32)hi & 0x0Fu) << 8) | (u32)lo) + 1u;

                while (count-- != 0 && written < remaining)
                {
                    dest[written] = (disp <= written) ? dest[written - disp] : 0;
                    written++;
                }
            }
            else
            {
                if ((size_t)(src - base) >= avail_in)
                    return 0;
                dest[written++] = *src++;
            }
        }
    }

    return (size_t)(src - base);
}

const u8 *firered_portable_rom_find_lz_followup(
    const u8 *region_start,
    size_t region_byte_len,
    size_t search_cap,
    const u8 *strict_sig,
    size_t strict_sig_len,
    u32 relaxed_decomp_size,
    FireredRomLzFollowupKind *kind_out)
{
    size_t scan_len;
    size_t i;
    const u8 *rom_end;

    if (kind_out != NULL)
        *kind_out = FIRERED_ROM_LZ_FOLLOWUP_NONE;

    if (region_start == NULL || region_byte_len == 0u || strict_sig == NULL || strict_sig_len == 0u)
        return NULL;

    rom_end = region_start + region_byte_len;
    scan_len = search_cap;
    if (region_byte_len < scan_len)
        scan_len = region_byte_len;

    for (i = 0; i + strict_sig_len <= scan_len; i += 4)
    {
        const u8 *p = region_start + i;

        if (memcmp(p, strict_sig, strict_sig_len) == 0)
        {
            if (kind_out != NULL)
                *kind_out = FIRERED_ROM_LZ_FOLLOWUP_STRICT_SIG;
            return p;
        }
    }

    for (i = 0; i + 4 <= scan_len; i += 4)
    {
        const u8 *p = region_start + i;
        size_t avail = (size_t)(rom_end - p);
        u32 dec;
        size_t clen;

        if (avail < 4)
            break;
        if (p[0] != 0x10)
            continue;
        dec = (u32)p[1] | ((u32)p[2] << 8) | ((u32)p[3] << 16);
        if (dec != relaxed_decomp_size)
            continue;
        clen = firered_portable_rom_lz77_compressed_size(p, avail);
        if (clen == 0)
            continue;
        if (kind_out != NULL)
            *kind_out = FIRERED_ROM_LZ_FOLLOWUP_RELAXED_DEC;
        return p;
    }

    return NULL;
}

#endif /* PORTABLE */
