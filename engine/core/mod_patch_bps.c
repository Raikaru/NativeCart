#include "mod_patch_bps.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOD_PATCH_MAX_OUTPUT (128u * 1024u * 1024u)

/* FIRERED_TRACE_MOD_LAUNCH=1 — BPS-only diagnostics (stderr). */
static int mod_bps_trace_on(void)
{
    static int tri = -1;
    const char *e;

    if (tri >= 0)
        return tri;
    e = getenv("FIRERED_TRACE_MOD_LAUNCH");
    tri = (e != NULL && e[0] != '\0' && strcmp(e, "0") != 0);
    return tri;
}

static void mod_bps_tracef(const char *fmt, ...)
{
    va_list ap;
    char buf[896];

    if (!mod_bps_trace_on())
        return;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "[firered BPS] %s\n", buf);
    fflush(stderr);
}

static void mod_bps_trace_fail(const char *err, size_t err_cap)
{
    if (!mod_bps_trace_on())
        return;
    if (err != NULL && err_cap > 0 && err[0] != '\0')
        mod_bps_tracef("FAIL: %s", err);
    else
        mod_bps_tracef("FAIL: (no error message buffer)");
}

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

static uint32_t bps_read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
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
    /*
     * Three cursors (Flips libbps / beat spec — do not alias):
     * - target_write: output length / write head (Flips outat - outstart).
     * - source_copy_pos: ROM read cursor for SourceCopy only (Flips inreadat - instart).
     * - target_copy_pos: read cursor for TargetCopy (Flips outreadat - outstart).
     *   TargetCopy bounds (libbps): outreadat < outat initially; outreadat+length <= outend.
     *   Do NOT require outreadat+length <= outat — the byte loop may read bytes written earlier
     *   in the same command (overlap past the initial write head).
     * SourceRead copies source[target_write..] -> target (same index as output progress).
     */
    size_t target_write;
    size_t source_copy_pos;
    size_t target_copy_pos;

    *out_rom = NULL;
    *out_len = 0;

    mod_bps_tracef("apply enter source_len=%zu patch_len=%zu", source_len, patch_len);
    /* Fingerprint: if you still see TargetCopy OOB with an old binary, this line will be missing. */
    mod_bps_tracef("bps_apply_impl=split_src_tgt_copy_cursor");

    if (source == NULL || patch == NULL)
    {
        set_err(err, err_cap, "BPS: null source or patch buffer");
        mod_bps_trace_fail(err, err_cap);
        return 0;
    }

    if (patch_len < 4 + 12)
    {
        set_err(err, err_cap, "BPS: patch file too small");
        mod_bps_trace_fail(err, err_cap);
        return 0;
    }

    if (memcmp(patch, "BPS1", 4) != 0)
    {
        set_err(err, err_cap, "BPS: missing BPS1 magic (not a BPS patch?)");
        mod_bps_trace_fail(err, err_cap);
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
        mod_bps_trace_fail(err, err_cap);
        return 0;
    }
    pos += metadata_len;

    mod_bps_tracef(
        "decoded header expect_source_size=%zu target_size=%zu metadata_len=%zu (pos after meta=%zu)",
        source_size_expect,
        target_size,
        metadata_len,
        pos);

    if (target_size == 0 || target_size > MOD_PATCH_MAX_OUTPUT)
    {
        set_err(err, err_cap, "BPS: unreasonable output size");
        mod_bps_trace_fail(err, err_cap);
        return 0;
    }

    if (source_len != source_size_expect)
    {
        if (err != NULL && err_cap > 0)
            snprintf(err, err_cap,
                "BPS: base ROM size mismatch (have %zu bytes, patch expects %zu)",
                source_len, source_size_expect);
        mod_bps_trace_fail(err, err_cap);
        return 0;
    }

    /* Footer: source, target, patch CRC32 each little-endian (Flips libbps, beat/BPS spec). */
    crc_pos = patch_len - 12;
    expect_source_crc = bps_read_u32_le(patch + crc_pos);
    expect_target_crc = bps_read_u32_le(patch + crc_pos + 4);
    expect_patch_crc = bps_read_u32_le(patch + crc_pos + 8);

    /* Patch-file CRC is over all bytes except the last 4 (the patch CRC field), not minus 12. */
    calc_patch_crc = crc32_bytes(patch, patch_len - 4u);
    {
        uint32_t calc_source_crc = crc32_bytes(source, source_len);

        mod_bps_tracef(
            "footer expect_crc src=0x%08x tgt=0x%08x patch=0x%08x",
            (unsigned)expect_source_crc,
            (unsigned)expect_target_crc,
            (unsigned)expect_patch_crc);
        mod_bps_tracef(
            "calc_crc source=0x%08x patch_file=0x%08x",
            (unsigned)calc_source_crc,
            (unsigned)calc_patch_crc);

        if (calc_patch_crc != expect_patch_crc && getenv("FIRERED_BPS_IGNORE_PATCH_CRC") == NULL)
        {
            mod_bps_tracef("WARN patch_file_crc mismatch (non-fatal; set FIRERED_BPS_IGNORE_PATCH_CRC=1 to silence)");
            fprintf(stderr, "BPS warning: patch file CRC mismatch (set FIRERED_BPS_IGNORE_PATCH_CRC=1 to ignore)\n");
        }

        if (calc_source_crc != expect_source_crc)
        {
            set_err(err, err_cap, "BPS: base ROM CRC does not match patch (wrong clean ROM?)");
            mod_bps_trace_fail(err, err_cap);
            return 0;
        }
    }

    target = (uint8_t *)malloc(target_size);
    if (target == NULL)
    {
        set_err(err, err_cap, "BPS: out of memory allocating output ROM");
        mod_bps_trace_fail(err, err_cap);
        return 0;
    }

    target_write = 0;
    source_copy_pos = 0;
    target_copy_pos = 0;

    while (pos < max_pos && target_write < target_size)
    {
        uint64_t action_data = bps_decode_var(patch, &pos, max_pos);
        unsigned action = (unsigned)(action_data & 3u);
        uint64_t length = (action_data >> 2) + 1;
        size_t len = (size_t)length;
        uint64_t off_enc;
        int64_t off;

        if (len > target_size - target_write)
        {
            set_err(err, err_cap, "BPS: decode overflow (length past end of output)");
            mod_bps_trace_fail(err, err_cap);
            free(target);
            return 0;
        }

        switch (action)
        {
        case BPS_SOURCE_READ:
            if (target_write + len > source_len)
            {
                set_err(err, err_cap, "BPS: SourceRead out of bounds");
                mod_bps_trace_fail(err, err_cap);
                free(target);
                return 0;
            }
            memcpy(target + target_write, source + target_write, len);
            target_write += len;
            break;

        case BPS_TARGET_READ:
            if (pos + len > max_pos)
            {
                set_err(err, err_cap, "BPS: TargetRead out of bounds");
                mod_bps_trace_fail(err, err_cap);
                free(target);
                return 0;
            }
            memcpy(target + target_write, patch + pos, len);
            pos += len;
            target_write += len;
            break;

        case BPS_SOURCE_COPY:
            off_enc = bps_decode_var(patch, &pos, max_pos);
            off = (off_enc & 1u) ? -(int64_t)(off_enc >> 1) : (int64_t)(off_enc >> 1);
            if (off < 0)
            {
                size_t u = (size_t)(-off);

                if (u > source_copy_pos)
                {
                    set_err(err, err_cap, "BPS: SourceCopy negative offset underflow");
                    mod_bps_trace_fail(err, err_cap);
                    free(target);
                    return 0;
                }
                source_copy_pos -= u;
            }
            else
                source_copy_pos += (size_t)off;

            if (source_copy_pos + len > source_len)
            {
                set_err(err, err_cap, "BPS: SourceCopy out of bounds");
                mod_bps_trace_fail(err, err_cap);
                free(target);
                return 0;
            }
            memcpy(target + target_write, source + source_copy_pos, len);
            source_copy_pos += len;
            target_write += len;
            break;

        case BPS_TARGET_COPY:
            off_enc = bps_decode_var(patch, &pos, max_pos);
            off = (off_enc & 1u) ? -(int64_t)(off_enc >> 1) : (int64_t)(off_enc >> 1);
            if (off < 0)
            {
                size_t u = (size_t)(-off);

                if (u > target_copy_pos)
                {
                    set_err(err, err_cap, "BPS: TargetCopy negative offset underflow");
                    mod_bps_trace_fail(err, err_cap);
                    free(target);
                    return 0;
                }
                target_copy_pos -= u;
            }
            else
                target_copy_pos += (size_t)off;

            /* Alcaro Flips libbps: outreadat < outat; outreadat+length <= outend (not <= outat). */
            if (target_copy_pos >= target_write)
            {
                set_err(err, err_cap, "BPS: TargetCopy read offset at or past write head");
                mod_bps_trace_fail(err, err_cap);
                free(target);
                return 0;
            }
            if (target_copy_pos + len > target_size)
            {
                set_err(err, err_cap, "BPS: TargetCopy out of bounds");
                mod_bps_trace_fail(err, err_cap);
                free(target);
                return 0;
            }
            {
                size_t i;

                for (i = 0; i < len; i++)
                    target[target_write + i] = target[target_copy_pos + i];
            }
            target_copy_pos += len;
            target_write += len;
            break;

        default:
            set_err(err, err_cap, "BPS: unknown action");
            mod_bps_trace_fail(err, err_cap);
            free(target);
            return 0;
        }
    }

    if (target_write != target_size)
    {
        set_err(err, err_cap, "BPS: output size mismatch after apply");
        mod_bps_trace_fail(err, err_cap);
        free(target);
        return 0;
    }

    if (crc32_bytes(target, target_size) != expect_target_crc)
    {
        set_err(err, err_cap, "BPS: output CRC mismatch (corrupt patch or wrong base?)");
        mod_bps_tracef(
            "calc output_crc=0x%08x expect_target_crc=0x%08x",
            (unsigned)crc32_bytes(target, target_size),
            (unsigned)expect_target_crc);
        mod_bps_trace_fail(err, err_cap);
        free(target);
        return 0;
    }

    *out_rom = target;
    *out_len = target_size;
    if (err != NULL && err_cap > 0)
        err[0] = '\0';
    mod_bps_tracef("apply OK output_len=%zu output_crc=0x%08x", target_size, (unsigned)expect_target_crc);
    return 1;
}
