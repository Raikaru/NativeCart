#include "mod_patch_bps.h"

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

/* IEEE / BPS-style CRC-32 (reflected, poly 0xEDB88320) */
static uint32_t crc32_bytes(const uint8_t *data, size_t size)
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

/*
 * BPS / beat variable-length integers (docs/bps_spec.md, byuu):
 * - Low 7 bits of each byte are payload; bit 7 set means this is the last byte.
 * - Between bytes: shift <<= 7 and add shift to the accumulator (not plain bit-packing).
 * This differs from MIDI-style VLQ (continuation when bit 7 is set).
 */
static uint64_t bps_decode_var(const uint8_t *data, size_t *pos, size_t max_pos)
{
    uint64_t out = 0;
    uint64_t shift = 1;
    unsigned iter = 0;

    while (*pos < max_pos && iter < 16)
    {
        uint8_t x = data[(*pos)++];

        iter++;
        out += (uint64_t)(x & 0x7Fu) * shift;
        if ((x & 0x80u) != 0)
            return out;
        if (shift > (UINT64_MAX >> 7))
            break;
        shift <<= 7;
        out += shift;
    }
    return out;
}

enum {
    BPS_SOURCE_READ = 0,
    BPS_TARGET_READ = 1,
    BPS_SOURCE_COPY = 2,
    BPS_TARGET_COPY = 3
};

int mod_patch_bps_apply(
    const uint8_t *source,
    size_t source_len,
    const uint8_t *patch,
    size_t patch_len,
    uint8_t **out_rom,
    size_t *out_len,
    char *err,
    size_t err_cap)
{
    size_t pos;
    size_t max_pos;
    size_t source_size_expect;
    size_t target_size;
    size_t metadata_len;
    size_t crc_pos;
    uint32_t expect_source_crc;
    uint32_t expect_target_crc;
    uint32_t expect_patch_crc;
    uint32_t calc_patch_crc;
    uint8_t *target = NULL;
    /* Match gdextension/memory/rom_overlay.cpp: source_offset is ROM index for
     * Source* ops and is repurposed as the target-buffer read index for TargetCopy. */
    size_t source_offset;
    size_t target_offset;

    *out_rom = NULL;
    *out_len = 0;

    if (source == NULL || patch == NULL)
    {
        set_err(err, err_cap, "BPS: null source or patch buffer");
        return 0;
    }

    if (patch_len < 4 + 12)
    {
        set_err(err, err_cap, "BPS: patch file too small");
        return 0;
    }

    if (memcmp(patch, "BPS1", 4) != 0)
    {
        set_err(err, err_cap, "BPS: missing BPS1 magic (not a BPS patch?)");
        return 0;
    }

    max_pos = patch_len - 12;
    pos = 4;
    source_size_expect = (size_t)bps_decode_var(patch, &pos, max_pos);
    target_size = (size_t)bps_decode_var(patch, &pos, max_pos);
    metadata_len = (size_t)bps_decode_var(patch, &pos, max_pos);

    if (pos + metadata_len > max_pos)
    {
        set_err(err, err_cap, "BPS: invalid metadata length");
        return 0;
    }
    pos += metadata_len;

    if (target_size == 0 || target_size > MOD_PATCH_MAX_OUTPUT)
    {
        set_err(err, err_cap, "BPS: unreasonable output size");
        return 0;
    }

    if (source_len != source_size_expect)
    {
        if (err != NULL && err_cap > 0)
            snprintf(err, err_cap,
                "BPS: base ROM size mismatch (have %zu bytes, patch expects %zu)",
                source_len, source_size_expect);
        return 0;
    }

    crc_pos = patch_len - 12;
    expect_source_crc = ((uint32_t)patch[crc_pos] << 24) | ((uint32_t)patch[crc_pos + 1] << 16)
        | ((uint32_t)patch[crc_pos + 2] << 8) | (uint32_t)patch[crc_pos + 3];
    expect_target_crc = ((uint32_t)patch[crc_pos + 4] << 24) | ((uint32_t)patch[crc_pos + 5] << 16)
        | ((uint32_t)patch[crc_pos + 6] << 8) | (uint32_t)patch[crc_pos + 7];
    expect_patch_crc = ((uint32_t)patch[crc_pos + 8] << 24) | ((uint32_t)patch[crc_pos + 9] << 16)
        | ((uint32_t)patch[crc_pos + 10] << 8) | (uint32_t)patch[crc_pos + 11];

    calc_patch_crc = crc32_bytes(patch, crc_pos);
    if (calc_patch_crc != expect_patch_crc && getenv("FIRERED_BPS_IGNORE_PATCH_CRC") == NULL)
    {
        fprintf(stderr, "BPS warning: patch file CRC mismatch (set FIRERED_BPS_IGNORE_PATCH_CRC=1 to ignore)\n");
    }

    if (crc32_bytes(source, source_len) != expect_source_crc)
    {
        set_err(err, err_cap, "BPS: base ROM CRC does not match patch (wrong clean ROM?)");
        return 0;
    }

    target = (uint8_t *)malloc(target_size);
    if (target == NULL)
    {
        set_err(err, err_cap, "BPS: out of memory allocating output ROM");
        return 0;
    }

    source_offset = 0;
    target_offset = 0;

    while (pos < max_pos && target_offset < target_size)
    {
        uint64_t action_data = bps_decode_var(patch, &pos, max_pos);
        unsigned action = (unsigned)(action_data & 3u);
        uint64_t length = (action_data >> 2) + 1;
        size_t len = (size_t)length;
        uint64_t off_enc;
        int64_t off;

        if (len > target_size - target_offset)
        {
            set_err(err, err_cap, "BPS: decode overflow (length past end of output)");
            free(target);
            return 0;
        }

        switch (action)
        {
        case BPS_SOURCE_READ:
            if (source_offset + len > source_len)
            {
                set_err(err, err_cap, "BPS: SourceRead out of bounds");
                free(target);
                return 0;
            }
            memcpy(target + target_offset, source + source_offset, len);
            source_offset += len;
            target_offset += len;
            break;

        case BPS_TARGET_READ:
            if (pos + len > max_pos)
            {
                set_err(err, err_cap, "BPS: TargetRead out of bounds");
                free(target);
                return 0;
            }
            memcpy(target + target_offset, patch + pos, len);
            pos += len;
            target_offset += len;
            break;

        case BPS_SOURCE_COPY:
            off_enc = bps_decode_var(patch, &pos, max_pos);
            off = (off_enc & 1u) ? -(int64_t)(off_enc >> 1) : (int64_t)(off_enc >> 1);
            if (off < 0)
            {
                size_t u = (size_t)(-off);

                if (u > source_offset)
                {
                    set_err(err, err_cap, "BPS: SourceCopy negative offset underflow");
                    free(target);
                    return 0;
                }
                source_offset -= u;
            }
            else
                source_offset += (size_t)off;

            if (source_offset + len > source_len)
            {
                set_err(err, err_cap, "BPS: SourceCopy out of bounds");
                free(target);
                return 0;
            }
            memcpy(target + target_offset, source + source_offset, len);
            source_offset += len;
            target_offset += len;
            break;

        case BPS_TARGET_COPY:
            off_enc = bps_decode_var(patch, &pos, max_pos);
            off = (off_enc & 1u) ? -(int64_t)(off_enc >> 1) : (int64_t)(off_enc >> 1);
            if (off < 0)
            {
                size_t u = (size_t)(-off);

                if (u > source_offset)
                {
                    set_err(err, err_cap, "BPS: TargetCopy negative offset underflow");
                    free(target);
                    return 0;
                }
                source_offset -= u;
            }
            else
                source_offset += (size_t)off;

            if (source_offset + len > target_offset)
            {
                set_err(err, err_cap, "BPS: TargetCopy out of bounds");
                free(target);
                return 0;
            }
            {
                size_t i;

                for (i = 0; i < len; i++)
                    target[target_offset + i] = target[source_offset + i];
            }
            source_offset += len;
            target_offset += len;
            break;

        default:
            set_err(err, err_cap, "BPS: unknown action");
            free(target);
            return 0;
        }
    }

    if (target_offset != target_size)
    {
        set_err(err, err_cap, "BPS: output size mismatch after apply");
        free(target);
        return 0;
    }

    if (crc32_bytes(target, target_size) != expect_target_crc)
    {
        set_err(err, err_cap, "BPS: output CRC mismatch (corrupt patch or wrong base?)");
        free(target);
        return 0;
    }

    *out_rom = target;
    *out_len = target_size;
    if (err != NULL && err_cap > 0)
        err[0] = '\0';
    return 1;
}
