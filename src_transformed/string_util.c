#include "global.h"
#include "gflib.h"

#ifdef PORTABLE
#include <stdarg.h>
#include <stdio.h>

extern void firered_runtime_trace_external(const char *message);
extern struct SaveBlock2 gSaveBlock2;
extern struct SaveBlock1 gSaveBlock1;

static u8 sStringTraceCount;
static u8 sPortablePlayerName[PLAYER_NAME_LENGTH + 1];
static u8 sPortableRivalName[PLAYER_NAME_LENGTH + 1];

static void TraceStringExpand(const char *fmt, ...)
{
    char buffer[256];
    va_list args;

    if (sStringTraceCount >= 64)
        return;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    firered_runtime_trace_external(buffer);
    fflush(stdout);
    sStringTraceCount++;
}

static bool8 IsPlayerNamePointerValid(const u8 *ptr)
{
    const u8 *base = (const u8 *)&gSaveBlock2;
    const u8 *end = base + sizeof(gSaveBlock2) + 128;

    return ptr >= base && ptr < end;
}

static bool8 IsRivalNamePointerValid(const u8 *ptr)
{
    const u8 *base = (const u8 *)&gSaveBlock1;
    const u8 *end = base + sizeof(gSaveBlock1) + 128;

    return ptr >= base && ptr < end;
}

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

static bool8 PortableTryDecodeUtf8Char(const u8 *src, u8 *outChar, u8 *outConsumed)
{
    if (src[0] == 0xC2)
    {
        switch (src[1])
        {
        case 0xA5:
            *outChar = CHAR_CURRENCY;
            *outConsumed = 2;
            return TRUE;
        }
    }
    else if (src[0] == 0xC3)
    {
        switch (src[1])
        {
        case 0x97:
            *outChar = CHAR_MULT_SIGN;
            *outConsumed = 2;
            return TRUE;
        case 0x89:
        case 0xA9:
            *outChar = CHAR_E_ACUTE;
            *outConsumed = 2;
            return TRUE;
        }
    }
    else if (src[0] == 0xE2 && src[1] == 0x80)
    {
        switch (src[2])
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

static bool8 PortableStringLooksLikeCString(const u8 *src)
{
    u32 i;

    if (src == NULL)
        return FALSE;
    if (src[0] == '\0')
        return FALSE;

    for (i = 0; i < 128; )
    {
        u8 decoded;
        u8 consumed;

        if (src[i] == EOS)
            return FALSE;
        if (src[i] == '\0')
            return TRUE;
        if (src[i] < 0x80)
        {
            i++;
            continue;
        }
        if (!PortableTryDecodeUtf8Char(&src[i], &decoded, &consumed))
            return FALSE;
        i += consumed;
    }

    return FALSE;
}

static bool8 PortableTryEncodePlaceholderToken(const u8 *src, u8 *outBytes, u8 *outConsumed, u8 *outProduced)
{
    struct PlaceholderToken
    {
        const char *token;
        u8 id;
    };
    static const struct PlaceholderToken sTokens[] = {
        { "{PLAYER}", PLACEHOLDER_ID_PLAYER },
        { "{RIVAL}", PLACEHOLDER_ID_RIVAL },
        { "{STR_VAR_1}", PLACEHOLDER_ID_STRING_VAR_1 },
        { "{STR_VAR_2}", PLACEHOLDER_ID_STRING_VAR_2 },
        { "{STR_VAR_3}", PLACEHOLDER_ID_STRING_VAR_3 },
    };
    u32 i;

    if (strncmp((const char *)src, "{LV_2}", strlen("{LV_2}")) == 0)
    {
        outBytes[0] = CHAR_EXTRA_SYMBOL;
        outBytes[1] = CHAR_LV_2;
        *outConsumed = (u8)strlen("{LV_2}");
        *outProduced = 2;
        return TRUE;
    }

    for (i = 0; i < ARRAY_COUNT(sTokens); i++)
    {
        size_t len = strlen(sTokens[i].token);
        if (strncmp((const char *)src, sTokens[i].token, len) == 0)
        {
            outBytes[0] = PLACEHOLDER_BEGIN;
            outBytes[1] = sTokens[i].id;
            *outConsumed = (u8)len;
            *outProduced = 2;
            return TRUE;
        }
    }

    return FALSE;
}

static u8 *PortableNormalizeTemplateString(u8 *dest, const u8 *src)
{
    while (*src != '\0' && *src != EOS)
    {
        u8 decoded;
        u8 consumed;
        u8 produced;
        u8 bytes[4];

        if (*src == '{' && PortableTryEncodePlaceholderToken(src, bytes, &consumed, &produced))
        {
            u8 i;
            for (i = 0; i < produced; i++)
                *dest++ = bytes[i];
            src += consumed;
            continue;
        }

        if (PortableTryDecodeUtf8Char(src, &decoded, &consumed))
        {
            *dest++ = decoded;
            src += consumed;
            continue;
        }

        if (*src == '\n')
        {
            *dest++ = CHAR_NEWLINE;
            src++;
            continue;
        }

        if (*src == '\\')
        {
            if (src[1] == 'p')
            {
                *dest++ = CHAR_PROMPT_CLEAR;
                src += 2;
                continue;
            }
            if (src[1] == 'l')
            {
                *dest++ = CHAR_PROMPT_SCROLL;
                src += 2;
                continue;
            }
            if (src[1] == 'n')
            {
                *dest++ = CHAR_NEWLINE;
                src += 2;
                continue;
            }
        }

        *dest++ = PortableAsciiToGba(*src++);
    }

    *dest = EOS;
    return dest;
}

static u8 *PortableCopyCStringAsGba(u8 *dest, const u8 *src)
{
    while (*src != '\0' && *src != EOS)
    {
        u8 decoded;
        u8 consumed;

        if (PortableTryDecodeUtf8Char(src, &decoded, &consumed))
        {
            *dest++ = decoded;
            src += consumed;
            continue;
        }

        *dest++ = PortableAsciiToGba(*src++);
    }

    *dest = EOS;
    return dest;
}

static u8 *PortableCopyGbaString(u8 *dest, const u8 *src)
{
    while (*src != EOS)
        *dest++ = *src++;

    *dest = EOS;
    return dest;
}
#endif

#define STRING_IS_TERMINATOR(ch) ((ch) == EOS)

EWRAM_DATA u8 gStringVar1[32] = {};
EWRAM_DATA u8 gStringVar2[20] = {};
EWRAM_DATA u8 gStringVar3[20] = {};
EWRAM_DATA u8 gStringVar4[1000] = {};
EWRAM_DATA u8 gUnknownStringVar[16] = {0};

static const u8 sDigits[] =
{
    CHAR_0,
    CHAR_1,
    CHAR_2,
    CHAR_3,
    CHAR_4,
    CHAR_5,
    CHAR_6,
    CHAR_7,
    CHAR_8,
    CHAR_9,
    CHAR_A,
    CHAR_B,
    CHAR_C,
    CHAR_D,
    CHAR_E,
    CHAR_F,
};

static const s32 sPowersOfTen[] =
{
             1,
            10,
           100,
          1000,
         10000,
        100000,
       1000000,
      10000000,
     100000000,
    1000000000,
};

extern u8 gExpandedPlaceholder_Empty[];
extern u8 gExpandedPlaceholder_Kun[];
extern u8 gExpandedPlaceholder_Chan[];
extern u8 gExpandedPlaceholder_Sapphire[];
extern u8 gExpandedPlaceholder_Ruby[];
extern u8 gExpandedPlaceholder_Aqua[];
extern u8 gExpandedPlaceholder_Magma[];
extern u8 gExpandedPlaceholder_Archie[];
extern u8 gExpandedPlaceholder_Maxie[];
extern u8 gExpandedPlaceholder_Kyogre[];
extern u8 gExpandedPlaceholder_Groudon[];
extern u8 gExpandedPlaceholder_Red[];
extern u8 gExpandedPlaceholder_Green[];

u8 *StringCopy_Nickname(u8 *dest, const u8 *src)
{
    u8 i;
    u32 limit = POKEMON_NAME_LENGTH;

    for (i = 0; i < limit; i++)
    {
        dest[i] = src[i];

        if (dest[i] == EOS)
            return &dest[i];
    }

    dest[i] = EOS;
    return &dest[i];
}

u8 *StringGet_Nickname(u8 *str)
{
    u8 i;
    u32 limit = POKEMON_NAME_LENGTH;

    for (i = 0; i < limit; i++)
        if (str[i] == EOS)
            return &str[i];

    str[i] = EOS;
    return &str[i];
}

u8 *StringCopy_PlayerName(u8 *dest, const u8 *src)
{
    s32 i;
    s32 limit = PLAYER_NAME_LENGTH;

    for (i = 0; i < limit; i++)
    {
        dest[i] = src[i];

        if (dest[i] == EOS)
            return &dest[i];
    }

    dest[i] = EOS;
    return &dest[i];
}

u8 *StringCopy(u8 *dest, const u8 *src)
{
#ifdef PORTABLE
    if (PortableStringLooksLikeCString(src))
        return PortableCopyCStringAsGba(dest, src);
#endif
    while (!STRING_IS_TERMINATOR(*src))
    {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = EOS;
    return dest;
}

u8 *StringAppend(u8 *dest, const u8 *src)
{
    while (!STRING_IS_TERMINATOR(*dest))
        dest++;

#ifdef PORTABLE
    if (PortableStringLooksLikeCString(src))
        return PortableCopyCStringAsGba(dest, src);
#endif

    return StringCopy(dest, src);
}

u8 *StringCopyN(u8 *dest, const u8 *src, u8 n)
{
    u16 i;

    for (i = 0; i < n; i++)
        dest[i] = src[i];

    return &dest[n];
}

u8 *StringAppendN(u8 *dest, const u8 *src, u8 n)
{
    while (!STRING_IS_TERMINATOR(*dest))
        dest++;

    return StringCopyN(dest, src, n);
}

u16 StringLength(const u8 *str)
{
    u16 length = 0;

    while (!STRING_IS_TERMINATOR(str[length]))
        length++;

    return length;
}

s32 StringCompare(const u8 *str1, const u8 *str2)
{
    while (*str1 == *str2)
    {
        if (STRING_IS_TERMINATOR(*str1))
            return 0;
        str1++;
        str2++;
    }

    return *str1 - *str2;
}

s32 StringCompareN(const u8 *str1, const u8 *str2, u32 n)
{
    while (*str1 == *str2)
    {
        if (*str1 == EOS)
            return 0;
        str1++;
        str2++;
        if (--n == 0)
            return 0;
    }

    return *str1 - *str2;
}

u8 *ConvertIntToDecimalStringN(u8 *dest, s32 value, enum StringConvertMode mode, u8 n)
{
    enum { WAITING_FOR_NONZERO_DIGIT, WRITING_DIGITS, WRITING_SPACES } state;
    s32 powerOfTen;
    s32 largestPowerOfTen = sPowersOfTen[n - 1];

    state = WAITING_FOR_NONZERO_DIGIT;

    if (mode == STR_CONV_MODE_RIGHT_ALIGN)
        state = WRITING_SPACES;

    if (mode == STR_CONV_MODE_LEADING_ZEROS)
        state = WRITING_DIGITS;

    for (powerOfTen = largestPowerOfTen; powerOfTen > 0; powerOfTen /= 10)
    {
        u8 *out;
        u8 c;
        u16 digit = value / powerOfTen;
        s32 temp = value - (powerOfTen * digit);

        if (state == WRITING_DIGITS)
        {
            out = dest++;

            if (digit <= 9)
                c = sDigits[digit];
            else
                c = CHAR_QUESTION_MARK;

            *out = c;
        }
        else if (digit != 0 || powerOfTen == 1)
        {
            state = WRITING_DIGITS;
            out = dest++;

            if (digit <= 9)
                c = sDigits[digit];
            else
                c = CHAR_QUESTION_MARK;

            *out = c;
        }
        else if (state == WRITING_SPACES)
        {
            *dest++ = CHAR_SPACE;
        }

        value = temp;
    }

    *dest = EOS;
    return dest;
}

u8 *ConvertIntToHexStringN(u8 *dest, s32 value, enum StringConvertMode mode, u8 n)
{
    enum { WAITING_FOR_NONZERO_DIGIT, WRITING_DIGITS, WRITING_SPACES } state;
    u8 i;
    s32 powerOfSixteen;
    s32 largestPowerOfSixteen = 1;

    for (i = 1; i < n; i++)
        largestPowerOfSixteen *= 16;

    state = WAITING_FOR_NONZERO_DIGIT;

    if (mode == STR_CONV_MODE_RIGHT_ALIGN)
        state = WRITING_SPACES;

    if (mode == STR_CONV_MODE_LEADING_ZEROS)
        state = WRITING_DIGITS;

    for (powerOfSixteen = largestPowerOfSixteen; powerOfSixteen > 0; powerOfSixteen /= 16)
    {
        u8 *out;
        u8 c;
        u32 digit = value / powerOfSixteen;
        s32 temp = value % powerOfSixteen;

        if (state == WRITING_DIGITS)
        {
            out = dest++;

            if (digit <= 0xF)
                c = sDigits[digit];
            else
                c = CHAR_QUESTION_MARK;

            *out = c;
        }
        else if (digit != 0 || powerOfSixteen == 1)
        {
            state = WRITING_DIGITS;
            out = dest++;

            if (digit <= 0xF)
                c = sDigits[digit];
            else
                c = CHAR_QUESTION_MARK;

            *out = c;
        }
        else if (state == WRITING_SPACES)
        {
            *dest++ = CHAR_SPACE;
        }

        value = temp;
    }

    *dest = EOS;
    return dest;
}

u8 *StringExpandPlaceholders(u8 *dest, const u8 *src)
{
#ifdef PORTABLE
    static u8 sPortableExpandedTemplate[1000];

    if (PortableStringLooksLikeCString(src))
    {
        PortableNormalizeTemplateString(sPortableExpandedTemplate, src);
        src = sPortableExpandedTemplate;
    }
#endif
    for (;;)
    {
        u8 c = *src++;
        u8 placeholderId;
        u8 *expandedString;

        switch (c)
        {
            case PLACEHOLDER_BEGIN:
                placeholderId = *src++;
#ifdef PORTABLE
                TraceStringExpand("StrExpand: placeholder=%u", placeholderId);
#endif
                expandedString = GetExpandedPlaceholder(placeholderId);
#ifdef PORTABLE
                TraceStringExpand("StrExpand: expanded=%p", expandedString);
                if (placeholderId == PLACEHOLDER_ID_PLAYER
                 || placeholderId == PLACEHOLDER_ID_RIVAL
                 || placeholderId == PLACEHOLDER_ID_STRING_VAR_1
                 || placeholderId == PLACEHOLDER_ID_STRING_VAR_2
                 || placeholderId == PLACEHOLDER_ID_STRING_VAR_3)
                {
                    dest = PortableCopyGbaString(dest, expandedString);
                    break;
                }
#endif
                dest = StringExpandPlaceholders(dest, expandedString);
                break;
            case EXT_CTRL_CODE_BEGIN:
                *dest++ = c;
                c = *src++;
                *dest++ = c;

                switch (c)
                {
                    case 0x07:
                    case 0x09:
                    case 0x0F:
                    case 0x15:
                    case 0x16:
                    case 0x17:
                    case 0x18:
                        break;
                    case 0x04:
                        *dest++ = *src++;
                    case 0x0B:
                        *dest++ = *src++;
                    default:
                        *dest++ = *src++;
                }
                break;
            case EOS:
                *dest = EOS;
                return dest;
            case 0xFA:
            case 0xFB:
            case 0xFE:
            default:
                *dest++ = c;
        }
    }
}

u8 *StringBraille(u8 *dest, const u8 *src)
{
    u8 setBrailleFont[] = { 0xFC, 0x06, 0x06, 0xFF };
    u8 gotoLine2[] = { 0xFE, 0xFC, 0x0E, 0x02, 0xFF };

    dest = StringCopy(dest, setBrailleFont);

    for (;;)
    {
        u8 c = *src++;

        switch (c)
        {
            case EOS:
                *dest = c;
                return dest;
            case 0xFE:
                dest = StringCopy(dest, gotoLine2);
                break;
            default:
                *dest++ = c;
                *dest++ = c + 0x40;
                break;
        }
    }
}

static u8 *ExpandPlaceholder_UnknownStringVar(void)
{
    return gUnknownStringVar;
}

static u8 *ExpandPlaceholder_PlayerName(void)
{
#ifdef PORTABLE
    const u8 *playerName = gSaveBlock2Ptr != NULL ? gSaveBlock2Ptr->playerName : NULL;

    if (!IsPlayerNamePointerValid(playerName))
    {
        TraceStringExpand("StrExpand: invalid playerName ptr=%p save2ptr=%p", playerName, gSaveBlock2Ptr);
        return gExpandedPlaceholder_Empty;
    }
    StringCopy_PlayerName(sPortablePlayerName, playerName);
    return sPortablePlayerName;
#else
    return gSaveBlock2Ptr->playerName;
#endif
}

static u8 *ExpandPlaceholder_StringVar1(void)
{
    return gStringVar1;
}

static u8 *ExpandPlaceholder_StringVar2(void)
{
    return gStringVar2;
}

static u8 *ExpandPlaceholder_StringVar3(void)
{
    return gStringVar3;
}

static u8 *ExpandPlaceholder_KunChan(void)
{
    if (gSaveBlock2Ptr->playerGender == MALE)
        return gExpandedPlaceholder_Kun;
    else
        return gExpandedPlaceholder_Chan;
}

static u8 *ExpandPlaceholder_RivalName(void)
{
#ifdef PORTABLE
    const u8 *rivalName = gSaveBlock1Ptr != NULL ? gSaveBlock1Ptr->rivalName : NULL;

    if (!IsRivalNamePointerValid(rivalName))
    {
        TraceStringExpand("StrExpand: invalid rivalName ptr=%p save1ptr=%p", rivalName, gSaveBlock1Ptr);
        return gExpandedPlaceholder_Green;
    }
#endif
    if (gSaveBlock1Ptr->rivalName[0] == EOS)
    {
        if (gSaveBlock2Ptr->playerGender == MALE)
            return gExpandedPlaceholder_Green;
        else
            return gExpandedPlaceholder_Red;
    }
    else
    {
#ifdef PORTABLE
        StringCopy_PlayerName(sPortableRivalName, rivalName);
        return sPortableRivalName;
#else
        return gSaveBlock1Ptr->rivalName;
#endif
    }
}

static u8 *ExpandPlaceholder_Version(void)
{
#if defined(FIRERED)
    return gExpandedPlaceholder_Ruby;
#elif defined(LEAFGREEN)
    return gExpandedPlaceholder_Sapphire;
#endif
}

static u8 *ExpandPlaceholder_Magma(void)
{
#if defined(FIRERED)
    return gExpandedPlaceholder_Magma;
#elif defined(LEAFGREEN)
    return gExpandedPlaceholder_Aqua;
#endif
}

static u8 *ExpandPlaceholder_Aqua(void)
{
#if defined(FIRERED)
    return gExpandedPlaceholder_Aqua;
#elif defined(LEAFGREEN)
    return gExpandedPlaceholder_Magma;
#endif
}

static u8 *ExpandPlaceholder_Maxie(void)
{
#if defined(FIRERED)
    return gExpandedPlaceholder_Maxie;
#elif defined(LEAFGREEN)
    return gExpandedPlaceholder_Archie;
#endif
}

static u8 *ExpandPlaceholder_Archie(void)
{
#if defined(FIRERED)
    return gExpandedPlaceholder_Archie;
#elif defined(LEAFGREEN)
    return gExpandedPlaceholder_Maxie;
#endif
}

static u8 *ExpandPlaceholder_Groudon(void)
{
#if defined(FIRERED)
    return gExpandedPlaceholder_Groudon;
#elif defined(LEAFGREEN)
    return gExpandedPlaceholder_Kyogre;
#endif
}

static u8 *ExpandPlaceholder_Kyogre(void)
{
#if defined(FIRERED)
    return gExpandedPlaceholder_Kyogre;
#elif defined(LEAFGREEN)
    return gExpandedPlaceholder_Groudon;
#endif
}

u8 *GetExpandedPlaceholder(u32 id)
{
    typedef u8 *(*ExpandPlaceholderFunc)(void);

    static const ExpandPlaceholderFunc funcs[] =
    {
        [PLACEHOLDER_ID_UNKNOWN]      = ExpandPlaceholder_UnknownStringVar,
        [PLACEHOLDER_ID_PLAYER]       = ExpandPlaceholder_PlayerName,
        [PLACEHOLDER_ID_STRING_VAR_1] = ExpandPlaceholder_StringVar1,
        [PLACEHOLDER_ID_STRING_VAR_2] = ExpandPlaceholder_StringVar2,
        [PLACEHOLDER_ID_STRING_VAR_3] = ExpandPlaceholder_StringVar3,
        [PLACEHOLDER_ID_KUN]          = ExpandPlaceholder_KunChan,
        [PLACEHOLDER_ID_RIVAL]        = ExpandPlaceholder_RivalName,
        [PLACEHOLDER_ID_VERSION]      = ExpandPlaceholder_Version,
        [PLACEHOLDER_ID_MAGMA]        = ExpandPlaceholder_Magma,
        [PLACEHOLDER_ID_AQUA]         = ExpandPlaceholder_Aqua,
        [PLACEHOLDER_ID_MAXIE]        = ExpandPlaceholder_Maxie,
        [PLACEHOLDER_ID_ARCHIE]       = ExpandPlaceholder_Archie,
        [PLACEHOLDER_ID_GROUDON]      = ExpandPlaceholder_Groudon,
        [PLACEHOLDER_ID_KYOGRE]       = ExpandPlaceholder_Kyogre,
    };

    if (id >= NELEMS(funcs))
        return gExpandedPlaceholder_Empty;
    else
        return funcs[id]();
}

u8 *StringFill(u8 *dest, u8 c, u16 n)
{
    u16 i;

    for (i = 0; i < n; i++)
        *dest++ = c;

    *dest = EOS;
    return dest;
}

u8 *StringCopyPadded(u8 *dest, const u8 *src, u8 c, u16 n)
{
    while (*src != EOS)
    {
        *dest++ = *src++;

        if (n)
            n--;
    }

    n--;

    while (n != (u16)-1)
    {
        *dest++ = c;
        n--;
    }

    *dest = EOS;
    return dest;
}

u8 *StringFillWithTerminator(u8 *dest, u16 n)
{
    return StringFill(dest, EOS, n);
}

u8 *StringCopyN_Multibyte(u8 *dest, const u8 *src, u32 n)
{
    u32 i;

    for (i = n - 1; i != -1u; i--)
    {
        if (*src == EOS)
        {
            break;
        }
        else
        {
            *dest++ = *src++;
            if (*(src - 1) == 0xF9)
                *dest++ = *src++;
        }
    }

    *dest = EOS;
    return dest;
}

u32 StringLength_Multibyte(const u8 *str)
{
    u32 length = 0;

    while (*str != EOS)
    {
        if (*str == 0xF9)
            str++;
        str++;
        length++;
    }

    return length;
}

u8 *WriteColorChangeControlCode(u8 *dest, u32 colorType, u8 color)
{
    *dest = 0xFC;
    dest++;

    switch (colorType)
    {
    case 0:
        *dest = 1;
        dest++;
        break;
    case 1:
        *dest = 3;
        dest++;
        break;
    case 2:
        *dest = 2;
        dest++;
        break;
    }

    *dest = color;
    dest++;
    *dest = EOS;
    return dest;
}

u8 GetExtCtrlCodeLength(u8 code)
{
    static const u8 lengths[] =
    {
        1,
        2,
        2,
        2,
        4,
        2,
        2,
        1,
        2,
        1,
        1,
        3,
        2,
        2,
        2,
        1,
        3,
        2,
        2,
        2,
        2,
        1,
        1,
        1,
        1,
    };

    u8 length = 0;
    if (code < NELEMS(lengths))
        length = lengths[code];
    return length;
}

static const u8 *SkipExtCtrlCode(const u8 *s)
{
    while (*s == 0xFC)
    {
        s++;
        s += GetExtCtrlCodeLength(*s);
    }

    return s;
}

s32 StringCompareWithoutExtCtrlCodes(const u8 *str1, const u8 *str2)
{
    s32 retVal = 0;

    while (1)
    {
        str1 = SkipExtCtrlCode(str1);
        str2 = SkipExtCtrlCode(str2);

        if (*str1 > *str2)
            break;

        if (*str1 < *str2)
        {
            retVal = -1;
            if (*str2 == 0xFF)
                retVal = 1;
        }

        if (*str1 == 0xFF)
            return retVal;

        str1++;
        str2++;
    }

    retVal = 1;

    if (*str1 == 0xFF)
        retVal = -1;

    return retVal;
}

void ConvertInternationalString(u8 *s, u8 language)
{
    if (language == LANGUAGE_JAPANESE)
    {
        u8 i;

        StripExtCtrlCodes(s);
        i = StringLength(s);
        s[i++] = 0xFC;
        s[i++] = 22;
        s[i++] = 0xFF;

        i--;

        while (i != (u8)-1)
        {
            s[i + 2] = s[i];
            i--;
        }

        s[0] = 0xFC;
        s[1] = 21;
    }
}

void StripExtCtrlCodes(u8 *str)
{
    u16 srcIndex = 0;
    u16 destIndex = 0;
    while (str[srcIndex] != 0xFF)
    {
        if (str[srcIndex] == 0xFC)
        {
            srcIndex++;
            srcIndex += GetExtCtrlCodeLength(str[srcIndex]);
        }
        else
        {
            str[destIndex++] = str[srcIndex++];
        }
    }
    str[destIndex] = 0xFF;
}
