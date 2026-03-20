#include "global.h"
#include "window.h"
#include "text.h"

#ifdef PORTABLE
#include <stdarg.h>
#include <stdio.h>

extern void firered_runtime_trace_external(const char *message);

static u8 sTextPrinterTraceCount;

static void TraceTextPrinter(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    if (sTextPrinterTraceCount >= 64)
        return;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    firered_runtime_trace_external(buffer);
    fflush(stdout);
    sTextPrinterTraceCount++;
}
#endif

static EWRAM_DATA struct TextPrinter sTempTextPrinter = {0};
static EWRAM_DATA struct TextPrinter sTextPrinters[NUM_TEXT_PRINTERS] = {0};
#ifdef PORTABLE
static EWRAM_DATA u8 sPortableTextPrinterBuffers[NUM_TEXT_PRINTERS][1200] = {0};
#endif

static u16 sFontHalfRowLookupTable[0x51];
static u16 sLastTextBgColor;
static u16 sLastTextFgColor;
static u16 sLastTextShadowColor;

COMMON_DATA const struct FontInfo *gFonts = NULL;
COMMON_DATA struct GlyphInfo gGlyphInfo = {0};

static const u8 sFontHalfRowOffsets[] =
{
    0x00, 0x01, 0x02, 0x00, 0x03, 0x04, 0x05, 0x03, 0x06, 0x07, 0x08, 0x06, 0x00, 0x01, 0x02, 0x00,
    0x09, 0x0A, 0x0B, 0x09, 0x0C, 0x0D, 0x0E, 0x0C, 0x0F, 0x10, 0x11, 0x0F, 0x09, 0x0A, 0x0B, 0x09,
    0x12, 0x13, 0x14, 0x12, 0x15, 0x16, 0x17, 0x15, 0x18, 0x19, 0x1A, 0x18, 0x12, 0x13, 0x14, 0x12,
    0x00, 0x01, 0x02, 0x00, 0x03, 0x04, 0x05, 0x03, 0x06, 0x07, 0x08, 0x06, 0x00, 0x01, 0x02, 0x00,
    0x1B, 0x1C, 0x1D, 0x1B, 0x1E, 0x1F, 0x20, 0x1E, 0x21, 0x22, 0x23, 0x21, 0x1B, 0x1C, 0x1D, 0x1B,
    0x24, 0x25, 0x26, 0x24, 0x27, 0x28, 0x29, 0x27, 0x2A, 0x2B, 0x2C, 0x2A, 0x24, 0x25, 0x26, 0x24,
    0x2D, 0x2E, 0x2F, 0x2D, 0x30, 0x31, 0x32, 0x30, 0x33, 0x34, 0x35, 0x33, 0x2D, 0x2E, 0x2F, 0x2D,
    0x1B, 0x1C, 0x1D, 0x1B, 0x1E, 0x1F, 0x20, 0x1E, 0x21, 0x22, 0x23, 0x21, 0x1B, 0x1C, 0x1D, 0x1B,
    0x36, 0x37, 0x38, 0x36, 0x39, 0x3A, 0x3B, 0x39, 0x3C, 0x3D, 0x3E, 0x3C, 0x36, 0x37, 0x38, 0x36,
    0x3F, 0x40, 0x41, 0x3F, 0x42, 0x43, 0x44, 0x42, 0x45, 0x46, 0x47, 0x45, 0x3F, 0x40, 0x41, 0x3F,
    0x48, 0x49, 0x4A, 0x48, 0x4B, 0x4C, 0x4D, 0x4B, 0x4E, 0x4F, 0x50, 0x4E, 0x48, 0x49, 0x4A, 0x48,
    0x36, 0x37, 0x38, 0x36, 0x39, 0x3A, 0x3B, 0x39, 0x3C, 0x3D, 0x3E, 0x3C, 0x36, 0x37, 0x38, 0x36,
    0x00, 0x01, 0x02, 0x00, 0x03, 0x04, 0x05, 0x03, 0x06, 0x07, 0x08, 0x06, 0x00, 0x01, 0x02, 0x00,
    0x09, 0x0A, 0x0B, 0x09, 0x0C, 0x0D, 0x0E, 0x0C, 0x0F, 0x10, 0x11, 0x0F, 0x09, 0x0A, 0x0B, 0x09,
    0x12, 0x13, 0x14, 0x12, 0x15, 0x16, 0x17, 0x15, 0x18, 0x19, 0x1A, 0x18, 0x12, 0x13, 0x14, 0x12,
    0x00, 0x01, 0x02, 0x00, 0x03, 0x04, 0x05, 0x03, 0x06, 0x07, 0x08, 0x06, 0x00, 0x01, 0x02, 0x00
};

void SetFontsPointer(const struct FontInfo *fonts)
{
    gFonts = fonts;
}

void DeactivateAllTextPrinters (void)
{
    int printer;
    for (printer = 0; printer < NUM_TEXT_PRINTERS; ++printer)
        sTextPrinters[printer].active = 0;
}

#ifdef PORTABLE
static const u8 sPortableAsciiToGba[128] = {
    /* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x08 */ 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x10 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x18 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x20 */ 0x00, 0xAB, 0x00, 0x00, 0x00, 0x5B, 0x2D, 0xB4,
    /* 0x28 */ 0x5C, 0x5D, 0x00, 0x2E, 0xB8, 0xAE, 0xAD, 0xBA,
    /* 0x30 */ 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
    /* 0x38 */ 0xA9, 0xAA, 0xF0, 0x36, 0x85, 0x35, 0x86, 0xAC,
    /* 0x40 */ 0x00, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1,
    /* 0x48 */ 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
    /* 0x50 */ 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1,
    /* 0x58 */ 0xD2, 0xD3, 0xD4, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x60 */ 0x00, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB,
    /* 0x68 */ 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3,
    /* 0x70 */ 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB,
    /* 0x78 */ 0xEC, 0xED, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static u8 PortableAsciiToGba(u8 c)
{
    if (c < 0x80)
        return sPortableAsciiToGba[c];
    return c;
}

static bool8 PortableTryGetUtf8SequenceLength(const u8 *ptr, u8 *outLength)
{
    if (ptr[0] >= 0xC2 && ptr[0] <= 0xDF)
    {
        if ((ptr[1] & 0xC0) == 0x80)
        {
            *outLength = 2;
            return TRUE;
        }
        return FALSE;
    }

    if (ptr[0] >= 0xE0 && ptr[0] <= 0xEF)
    {
        if ((ptr[1] & 0xC0) == 0x80 && (ptr[2] & 0xC0) == 0x80)
        {
            *outLength = 3;
            return TRUE;
        }
        return FALSE;
    }

    if (ptr[0] >= 0xF0 && ptr[0] <= 0xF4)
    {
        if ((ptr[1] & 0xC0) == 0x80 && (ptr[2] & 0xC0) == 0x80 && (ptr[3] & 0xC0) == 0x80)
        {
            *outLength = 4;
            return TRUE;
        }
        return FALSE;
    }

    return FALSE;
}

static bool8 PortableStringLooksLikeCString(const u8 *src)
{
    u32 i;

    if (src == NULL)
        return FALSE;
    if (src[0] == '\0')
        return FALSE;

    for (i = 0; i < 128; )
    {
        u8 utf8Length;

        if (src[i] == EOS)
            return FALSE;
        if (src[i] == '\0')
            return TRUE;
        if (src[i] < 0x80)
        {
            i++;
            continue;
        }
        if (!PortableTryGetUtf8SequenceLength(&src[i], &utf8Length))
            return FALSE;
        i += utf8Length;
    }

    return FALSE;
}

static bool8 TryDecodePortableUtf8Char(const u8 *ptr, u8 *outChar, u8 *outConsumed)
{
    if (ptr[0] == 0xC2)
    {
        switch (ptr[1])
        {
        case 0xA5:
            *outChar = CHAR_CURRENCY;
            *outConsumed = 2;
            return TRUE;
        }
    }
    else if (ptr[0] == 0xC3)
    {
        switch (ptr[1])
        {
        case 0x89:
        case 0xA9:
            *outChar = CHAR_E_ACUTE;
            *outConsumed = 2;
            return TRUE;
        }
    }
    else if (ptr[0] == 0xE2 && ptr[1] == 0x80)
    {
        switch (ptr[2])
        {
        case 0x98:
            *outChar = CHAR_SGL_QUOTE_LEFT;
            *outConsumed = 3;
            return TRUE;
        case 0x99:
            *outChar = CHAR_SGL_QUOTE_RIGHT;
            *outConsumed = 3;
            return TRUE;
        case 0x9C:
            *outChar = CHAR_DBL_QUOTE_LEFT;
            *outConsumed = 3;
            return TRUE;
        case 0x9D:
            *outChar = CHAR_DBL_QUOTE_RIGHT;
            *outConsumed = 3;
            return TRUE;
        case 0xA6:
            *outChar = CHAR_ELLIPSIS;
            *outConsumed = 3;
            return TRUE;
        }
    }

    return FALSE;
}

static const u8 *PortableNormalizeTextControlEscapes(const u8 *src, u8 windowId)
{
    u32 srcIdx = 0;
    u32 dstIdx = 0;
    u8 *dst;

    if (src == NULL || windowId >= NUM_TEXT_PRINTERS)
        return src;

    if (!PortableStringLooksLikeCString(src))
        return src;

    dst = sPortableTextPrinterBuffers[windowId];
    srcIdx = 0;
    while (src[srcIdx] != '\0' && dstIdx + 1 < sizeof(sPortableTextPrinterBuffers[windowId]))
    {
        u8 decoded;
        u8 consumed;

        if (TryDecodePortableUtf8Char(&src[srcIdx], &decoded, &consumed))
        {
            dst[dstIdx++] = decoded;
            srcIdx += consumed;
            continue;
        }
        if (src[srcIdx] == '\\')
        {
            u8 next = src[srcIdx + 1];
            if (next == 'p')
            {
                dst[dstIdx++] = CHAR_PROMPT_CLEAR;
                srcIdx += 2;
                continue;
            }
            if (next == 'l')
            {
                dst[dstIdx++] = CHAR_PROMPT_SCROLL;
                srcIdx += 2;
                continue;
            }
            if (next == 'n')
            {
                dst[dstIdx++] = CHAR_NEWLINE;
                srcIdx += 2;
                continue;
            }
        }
        if (src[srcIdx] < 0x80)
            dst[dstIdx++] = PortableAsciiToGba(src[srcIdx++]);
        else
            dst[dstIdx++] = src[srcIdx++];
    }

    dst[dstIdx] = EOS;
    return dst;
}
#endif

u16 AddTextPrinterParameterized(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16))
{
    struct TextPrinterTemplate printerTemplate;

#ifdef PORTABLE
    str = PortableNormalizeTextControlEscapes(str, windowId);
#endif
    printerTemplate.currentChar = str;
    printerTemplate.windowId = windowId;
    printerTemplate.fontId = fontId;
    printerTemplate.x = x;
    printerTemplate.y = y;
    printerTemplate.currentX = x;
    printerTemplate.currentY = y;
    printerTemplate.letterSpacing = GetFontAttribute(fontId, FONTATTR_LETTER_SPACING);
    printerTemplate.lineSpacing = GetFontAttribute(fontId, FONTATTR_LINE_SPACING);
    printerTemplate.unk = GetFontAttribute(fontId, FONTATTR_UNKNOWN);
    printerTemplate.fgColor = GetFontAttribute(fontId, FONTATTR_COLOR_FOREGROUND);
    printerTemplate.bgColor = GetFontAttribute(fontId, FONTATTR_COLOR_BACKGROUND);
    printerTemplate.shadowColor = GetFontAttribute(fontId, FONTATTR_COLOR_SHADOW);
    return AddTextPrinter(&printerTemplate, speed, callback);
}

bool16 AddTextPrinter(struct TextPrinterTemplate *textSubPrinter, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16))
{
    int i;
    u16 j;

    if (!gFonts)
        return FALSE;

    sTempTextPrinter.active = TRUE;
    sTempTextPrinter.state = RENDER_STATE_HANDLE_CHAR;
    sTempTextPrinter.textSpeed = speed;
    sTempTextPrinter.delayCounter = 0;
    sTempTextPrinter.scrollDistance = 0;

    for (i = 0; i < (int)ARRAY_COUNT(sTempTextPrinter.subUnion.fields); ++i)
        sTempTextPrinter.subUnion.fields[i] = 0;

    sTempTextPrinter.printerTemplate = *textSubPrinter;
#ifdef PORTABLE
    sTempTextPrinter.printerTemplate.currentChar = PortableNormalizeTextControlEscapes(sTempTextPrinter.printerTemplate.currentChar,
                                                                                       sTempTextPrinter.printerTemplate.windowId);
#endif
    sTempTextPrinter.callback = callback;
    sTempTextPrinter.minLetterSpacing = 0;
    sTempTextPrinter.japanese = 0;

    GenerateFontHalfRowLookupTable(textSubPrinter->fgColor, textSubPrinter->bgColor, textSubPrinter->shadowColor);
    if (speed != TEXT_SKIP_DRAW && speed != 0)
    {
        --sTempTextPrinter.textSpeed;
        sTextPrinters[textSubPrinter->windowId] = sTempTextPrinter;
    }
    else
    {
        sTempTextPrinter.textSpeed = 0;
        
        // Render all text (up to limit) at once
        for (j = 0; j < 0x400; ++j)
        {
            if (RenderFont(&sTempTextPrinter) == RENDER_FINISH)
                break;
        }

        // All the text is rendered to the window but don't draw it yet.
        if (speed != TEXT_SKIP_DRAW)
          CopyWindowToVram(sTempTextPrinter.printerTemplate.windowId, COPYWIN_GFX);
        sTextPrinters[textSubPrinter->windowId].active = FALSE;
    }
    return TRUE;
}

void RunTextPrinters(void)
{
    int i;

    for (i = 0; i < NUM_TEXT_PRINTERS; ++i)
    {
        if (sTextPrinters[i].active)
        {
#ifdef PORTABLE
            TraceTextPrinter("TextPrinter: run id=%d font=%d x=%d y=%d char=%u", i, sTextPrinters[i].printerTemplate.fontId, sTextPrinters[i].printerTemplate.currentX, sTextPrinters[i].printerTemplate.currentY, *sTextPrinters[i].printerTemplate.currentChar);
#endif
            u16 renderCmd = RenderFont(&sTextPrinters[i]);
#ifdef PORTABLE
            TraceTextPrinter("TextPrinter: render id=%d cmd=%u next=%u", i, renderCmd, *sTextPrinters[i].printerTemplate.currentChar);
#endif
            switch (renderCmd)
            {
            case RENDER_PRINT:
                CopyWindowToVram(sTextPrinters[i].printerTemplate.windowId, COPYWIN_GFX);
            case RENDER_UPDATE:
                if (sTextPrinters[i].callback != NULL)
                    sTextPrinters[i].callback(&sTextPrinters[i].printerTemplate, renderCmd);
                break;
            case RENDER_FINISH:
                sTextPrinters[i].active = FALSE;
                break;
            }
        }
    }
}

bool16 IsTextPrinterActive(u8 id)
{
    return sTextPrinters[id].active;
}

u32 RenderFont(struct TextPrinter *textPrinter)
{
    u32 ret;
    while (TRUE)
    {
        ret = gFonts[textPrinter->printerTemplate.fontId].fontFunction(textPrinter);
        if (ret != 2)
            return ret;
    }
}

void GenerateFontHalfRowLookupTable(u8 fgColor, u8 bgColor, u8 shadowColor)
{
    int lutIndex;
    int i, j, k, l;
    const u32 colors[] = {bgColor, fgColor, shadowColor};

    sLastTextBgColor = bgColor;
    sLastTextFgColor = fgColor;
    sLastTextShadowColor = shadowColor;

    lutIndex = 0;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            for (k = 0; k < 3; k++)
                for (l = 0; l < 3; l++)
                    sFontHalfRowLookupTable[lutIndex++] = (colors[l] << 12) | (colors[k] << 8) | (colors[j] << 4) | colors[i];
}

void SaveTextColors(u8 *fgColor, u8 *bgColor, u8 *shadowColor)
{
    *bgColor = sLastTextBgColor;
    *fgColor = sLastTextFgColor;
    *shadowColor = sLastTextShadowColor;
}

void RestoreTextColors(u8 *fgColor, u8 *bgColor, u8 *shadowColor)
{
    GenerateFontHalfRowLookupTable(*fgColor, *bgColor, *shadowColor);
}

void DecompressGlyphTile(const u16 *src, u16 *dest)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        int offsetIndex = (i & 1) ? (u8)*src++ : (*src >> 8);
        dest[i] = sFontHalfRowLookupTable[sFontHalfRowOffsets[offsetIndex]];
    }
}

u8 GetLastTextColor(u8 colorType)
{
    switch (colorType)
    {
        case 0:
            return sLastTextFgColor;
        case 2:
            return sLastTextBgColor;
        case 1:
            return sLastTextShadowColor;
        default:
            return 0;
    }
}

#define GLYPH_COPY(widthOffset, heightOffset, width, height, tilesDest, left, top, sizeX)                                                    \
{                                                                                                                                            \
    int xAdd, xpos, yAdd, ypos, toOrr, bits;                                                                                                 \
    u8 * src, * dst;                                                                                                                         \
    u32 _8pixbuf;                                                                                                                            \
                                                                                                                                             \
    src = gGlyphInfo.pixels + (heightOffset / 8 * 0x40) + (widthOffset / 8 * 0x20);                                                          \
    for (yAdd = 0, ypos = top + heightOffset; yAdd < height; yAdd++, ypos++)                                                                 \
    {                                                                                                                                        \
        _8pixbuf = *(u32 *)src;                                                                                                              \
        for (xAdd = 0, xpos = left + widthOffset; xAdd < width; xAdd++, xpos++)                                                              \
        {                                                                                                                                    \
            dst = (u8 *)((tilesDest) + ((xpos >> 1) & 3) + ((xpos >> 3) << 5) + (((ypos >> 3) * (sizeX)) << 5) + ((u32)(ypos << 29) >> 27)); \
            toOrr = (_8pixbuf >> (xAdd * 4)) & 0xF;                                                                                          \
            if (toOrr != 0)                                                                                                                  \
            {                                                                                                                                \
                bits = (xpos & 1) * 4;                                                                                                       \
                *dst = (toOrr << bits) | (*dst & (0xF0 >> bits));                                                                            \
            }                                                                                                                                \
        }                                                                                                                                    \
        src += 4;                                                                                                                            \
    }                                                                                                                                        \
}

void CopyGlyphToWindow(struct TextPrinter *textPrinter)
{
    int glyphWidth, glyphHeight;
    u8 sizeType;
    u8 windowId = textPrinter->printerTemplate.windowId;
    u8 width = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u8 height = GetWindowAttribute(windowId, WINDOW_HEIGHT);
    void *tileData = GetWindowTileDataAddress(windowId);
    u32 stride = ((width * 8 + ((width * 8) & 7)) >> 3);
    
    if (width * 8 - textPrinter->printerTemplate.currentX < gGlyphInfo.width)
        glyphWidth = width * 8 - textPrinter->printerTemplate.currentX;
    else
        glyphWidth = gGlyphInfo.width;
    if (height * 8 - textPrinter->printerTemplate.currentY < gGlyphInfo.height)
        glyphHeight = height * 8 - textPrinter->printerTemplate.currentY;
    else
        glyphHeight = gGlyphInfo.height;

    sizeType = 0;
    if (glyphWidth > 8)
        sizeType |= 1;
    if (glyphHeight > 8)
        sizeType |= 2;
    
    switch (sizeType)
    {
        case 0: // ≤ 8x8
            GLYPH_COPY(0, 0, glyphWidth, glyphHeight, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            return;
        case 1: // ≤ 16x8
            GLYPH_COPY(0, 0, 8, glyphHeight, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            GLYPH_COPY(8, 0, glyphWidth - 8, glyphHeight, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            return;
        case 2: // ≤ 8x16
            GLYPH_COPY(0, 0, glyphWidth, 8, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            GLYPH_COPY(0, 8, glyphWidth, glyphHeight - 8, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            return;
        case 3: // ≤ 16x16
            GLYPH_COPY(0, 0, 8, 8, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            GLYPH_COPY(8, 0, glyphWidth - 8, 8, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            GLYPH_COPY(0, 8, 8, glyphHeight - 8, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            GLYPH_COPY(8, 8, glyphWidth - 8, glyphHeight - 8, tileData, textPrinter->printerTemplate.currentX, textPrinter->printerTemplate.currentY, stride);
            return;
    }
}

// Unused
static void CopyGlyphToWindow_Parameterized(void *tileData, u16 currentX, u16 currentY, u16 width, u16 height)
{
    int glyphWidth, glyphHeight;
    u8 sizeType;
    u16 sizeX;
    
    if (width - currentX < gGlyphInfo.width)
        glyphWidth = width - currentX;
    else
        glyphWidth = gGlyphInfo.width;
    if (height - currentY < gGlyphInfo.height)
        glyphHeight = height - currentY;
    else
        glyphHeight = gGlyphInfo.height;
    
    sizeType = 0;
    sizeX  = (width + (width & 7)) >> 3;
    if (glyphWidth > 8)
        sizeType |= 1;
    if (glyphHeight > 8)
        sizeType |= 2;
    
    switch (sizeType)
    {
        case 0:
            GLYPH_COPY(0, 0, glyphWidth, glyphHeight, tileData, currentX, currentY, sizeX);
            return;
        case 1:
            GLYPH_COPY(0, 0, 8, glyphHeight, tileData, currentX, currentY, sizeX);
            GLYPH_COPY(8, 0, glyphWidth - 8, glyphHeight, tileData, currentX, currentY, sizeX);
            return;
        case 2:
            GLYPH_COPY(0, 0, glyphWidth, 8, tileData, currentX, currentY, sizeX);
            GLYPH_COPY(0, 8, glyphWidth, glyphHeight - 8, tileData, currentX, currentY, sizeX);
            return;
        case 3:
            GLYPH_COPY(0, 0, 8, 8, tileData, currentX, currentY, sizeX);
            GLYPH_COPY(8, 0, glyphWidth - 8, 8, tileData, currentX, currentY, sizeX);
            GLYPH_COPY(0, 8, 8, glyphHeight - 8, tileData, currentX, currentY, sizeX);
            GLYPH_COPY(8, 8, glyphWidth - 8, glyphHeight - 8, tileData, currentX, currentY, sizeX);
            return;
    }
}

void ClearTextSpan(struct TextPrinter *textPrinter, u32 width)
{
}
