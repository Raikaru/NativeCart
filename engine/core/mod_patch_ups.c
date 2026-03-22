#include "mod_patch_ups.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOD_PATCH_MAX_OUTPUT (128u * 1024u * 1024u)

static void set_err(char *err, size_t err_cap, const char *msg)
{
    if (err != NULL && err_cap > 0)
    {
        snprintf(err, err_cap, "%s", msg);
        err[err_cap - 1] = '\0';
    }
}

/* Same CRC-32 as BPS / Flips (reflected, poly 0xEDB88320). */
static uint32_t ups_crc32(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;
    int j;

    for (i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc ^ 0xFFFFFFFFu;
}

static uint32_t ups_read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/*
 * UPS variable-length size (Flips libups decodeto), NOT the same as BPS beat integers.
 */
static int ups_decode_size(const uint8_t *patch, size_t *pos, size_t max_pos, size_t *out_sz)
{
    uint64_t var = 0;
    unsigned shift = 0;

    for (;;)
    {
        uint8_t next;
        uint64_t low;
        uint64_t addthis;
        uint64_t inc;

        if (*pos >= max_pos)
            return 0;
        next = patch[(*pos)++];
        low = (uint64_t)(next & 0x7Fu);
        if (low > (UINT64_MAX >> (int)shift))
            return 0;
        addthis = low << shift;
        if (var > UINT64_MAX - addthis)
            return 0;
        var += addthis;
        if ((next & 0x80u) != 0)
            break;
        shift += 7;
        if (shift > 56)
            return 0;
        inc = 1ull << shift;
        if (var > UINT64_MAX - inc)
            return 0;
        var += inc;
    }
    if (var > (uint64_t)SIZE_MAX)
        return 0;
    *out_sz = (size_t)var;
    return 1;
}

int mod_patch_ups_apply(
    const uint8_t *source,
    size_t source_len,
    const uint8_t *patch,
    size_t patch_len,
    uint8_t **out_rom,
    size_t *out_len,
    char *err,
    size_t err_cap)
{
    size_t p;
    size_t patch_data_end;
    size_t inlen;
    size_t outlen;
    int backwards = 0;
    uint8_t *out = NULL;
    size_t in_pos = 0;
    size_t out_pos = 0;
    uint32_t crc_in_e;
    uint32_t crc_out_e;
    uint32_t crc_patch_e;
    uint32_t crc_in_a;
    uint32_t crc_out_a;
    uint32_t crc_patch_a;

    *out_rom = NULL;
    *out_len = 0;

    if (source == NULL || patch == NULL)
    {
        set_err(err, err_cap, "UPS: null source or patch buffer");
        return 0;
    }

    /* Magic + minimal payload + 12-byte footer. */
    if (patch_len < 4u + 2u + 12u)
    {
        set_err(err, err_cap, "UPS: patch file too small");
        return 0;
    }

    if (memcmp(patch, "UPS1", 4) != 0)
    {
        set_err(err, err_cap, "UPS: missing UPS1 magic (not a UPS patch?)");
        return 0;
    }

    patch_data_end = patch_len - 12u;
    p = 4;
    if (!ups_decode_size(patch, &p, patch_data_end, &inlen))
    {
        set_err(err, err_cap, "UPS: invalid input size encoding");
        return 0;
    }
    if (!ups_decode_size(patch, &p, patch_data_end, &outlen))
    {
        set_err(err, err_cap, "UPS: invalid output size encoding");
        return 0;
    }
    if (inlen != source_len)
    {
        size_t tmp = inlen;

        inlen = outlen;
        outlen = tmp;
        backwards = 1;
    }
    if (inlen != source_len)
    {
        set_err(err, err_cap, "UPS: source file size does not match patch (wrong base ROM?)");
        return 0;
    }

    if (outlen == 0 || outlen > MOD_PATCH_MAX_OUTPUT)
    {
        set_err(err, err_cap, "UPS: unreasonable output size");
        return 0;
    }

    out = (uint8_t *)malloc(outlen);
    if (out == NULL)
    {
        set_err(err, err_cap, "UPS: out of memory allocating output ROM");
        return 0;
    }
    memset(out, 0, outlen);

    while (p < patch_data_end)
    {
        size_t skip;
        uint8_t pb;
        uint8_t ib;

        if (!ups_decode_size(patch, &p, patch_data_end, &skip))
        {
            set_err(err, err_cap, "UPS: malformed patch (skip decode)");
            free(out);
            return 0;
        }

        while (skip > 0)
        {
            if (in_pos >= source_len)
                ib = 0;
            else
                ib = source[in_pos++];
            if (out_pos < outlen)
                out[out_pos++] = ib;
            skip--;
        }

        for (;;)
        {
            if (p >= patch_data_end)
            {
                set_err(err, err_cap, "UPS: malformed patch (truncated XOR block)");
                free(out);
                return 0;
            }
            pb = patch[p++];
            if (in_pos >= source_len)
                ib = 0;
            else
                ib = source[in_pos++];
            if (out_pos < outlen)
                out[out_pos++] = (uint8_t)(ib ^ pb);
            if (pb == 0)
                break;
        }
    }

    if (p != patch_data_end)
    {
        set_err(err, err_cap, "UPS: malformed patch (extra data before footer)");
        free(out);
        return 0;
    }

    while (out_pos < outlen && in_pos < source_len)
        out[out_pos++] = source[in_pos++];
    while (out_pos < outlen)
        out[out_pos++] = 0;
    while (in_pos < source_len)
        in_pos++;

    crc_in_e = ups_read_u32_le(patch + patch_len - 12);
    crc_out_e = ups_read_u32_le(patch + patch_len - 8);
    crc_patch_e = ups_read_u32_le(patch + patch_len - 4);

    crc_in_a = ups_crc32(source, source_len);
    crc_out_a = ups_crc32(out, outlen);
    crc_patch_a = ups_crc32(patch, patch_len - 4u);

    /* Flips compares logical sizes after optional swap. */
    if (inlen == outlen)
    {
        int ok_forward = (crc_in_a == crc_in_e && crc_out_a == crc_out_e);
        int ok_swap = (crc_in_a == crc_out_e && crc_out_a == crc_in_e);

        if (!ok_forward && !ok_swap)
        {
            set_err(err, err_cap, "UPS: CRC mismatch (wrong base ROM or corrupt patch?)");
            free(out);
            return 0;
        }
    }
    else
    {
        if (!backwards)
        {
            if (crc_in_a != crc_in_e || crc_out_a != crc_out_e)
            {
                set_err(err, err_cap, "UPS: CRC mismatch (wrong base ROM or corrupt patch?)");
                free(out);
                return 0;
            }
        }
        else
        {
            if (crc_in_a != crc_out_e || crc_out_a != crc_in_e)
            {
                set_err(err, err_cap, "UPS: CRC mismatch (wrong base ROM or corrupt patch?)");
                free(out);
                return 0;
            }
        }
    }

    if (crc_patch_a != crc_patch_e && getenv("FIRERED_UPS_IGNORE_PATCH_CRC") == NULL)
        fprintf(stderr, "UPS warning: patch file CRC mismatch (set FIRERED_UPS_IGNORE_PATCH_CRC=1 to ignore)\n");

    *out_rom = out;
    *out_len = outlen;
    if (err != NULL && err_cap > 0)
        err[0] = '\0';
    return 1;
}
