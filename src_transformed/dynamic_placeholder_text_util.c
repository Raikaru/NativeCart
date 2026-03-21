#include "global.h"
#include "gflib.h"
#include "constants/event_objects.h"

static EWRAM_DATA const u8 *sStringPointers[8] = {0};

#ifdef PORTABLE
static const u8 sPortableAsciiToGba[128] =
{
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
        case 0xB7:
            *outChar = CHAR_BULLET;
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

static bool8 PortableDynamicStringLooksLikeCString(const u8 *src)
{
    u32 i;

    if (src == NULL || src[0] == '\0')
        return FALSE;

    for (i = 0; i < 256;)
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

static bool8 PortableTryReadHexByte(const u8 *src, u8 *outValue, u8 *outConsumed)
{
    u32 i = 0;
    u32 digits = 0;
    u8 value = 0;

    if (src[0] == '0' && (src[1] == 'x' || src[1] == 'X'))
        i = 2;

    while (digits < 2)
    {
        u8 ch = src[i];
        u8 nibble;

        if (ch >= '0' && ch <= '9')
            nibble = ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            nibble = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F')
            nibble = ch - 'A' + 10;
        else
            break;

        value = (value << 4) | nibble;
        i++;
        digits++;
    }

    if (digits == 0)
        return FALSE;

    *outValue = value;
    *outConsumed = i;
    return TRUE;
}

static bool8 PortableTryMatchLiteralToken(const u8 *src, const char *token)
{
    while (*token != '\0')
    {
        if (*src++ != (u8)*token++)
            return FALSE;
    }
    return TRUE;
}

static bool8 PortableTryEncodeTextColor(const u8 *src, u8 *outColor, u8 *outConsumed)
{
    struct ColorToken
    {
        const char *name;
        u8 value;
    };
    static const struct ColorToken sColorTokens[] =
    {
        { "TRANSPARENT", TEXT_COLOR_TRANSPARENT },
        { "WHITE", TEXT_COLOR_WHITE },
        { "DARK_GRAY", TEXT_COLOR_DARK_GRAY },
        { "LIGHT_GRAY", TEXT_COLOR_LIGHT_GRAY },
        { "RED", TEXT_COLOR_RED },
        { "LIGHT_RED", TEXT_COLOR_LIGHT_RED },
        { "GREEN", TEXT_COLOR_GREEN },
        { "LIGHT_GREEN", TEXT_COLOR_LIGHT_GREEN },
        { "BLUE", TEXT_COLOR_BLUE },
        { "LIGHT_BLUE", TEXT_COLOR_LIGHT_BLUE },
        { "DYNAMIC_COLOR1", TEXT_DYNAMIC_COLOR_1 },
        { "DYNAMIC_COLOR2", TEXT_DYNAMIC_COLOR_2 },
        { "DYNAMIC_COLOR3", TEXT_DYNAMIC_COLOR_3 },
        { "DYNAMIC_COLOR4", TEXT_DYNAMIC_COLOR_4 },
        { "DYNAMIC_COLOR5", TEXT_DYNAMIC_COLOR_5 },
        { "DYNAMIC_COLOR6", TEXT_DYNAMIC_COLOR_6 },
    };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sColorTokens); i++)
    {
        size_t len = strlen(sColorTokens[i].name);
        if (strncmp((const char *)src, sColorTokens[i].name, len) == 0)
        {
            *outColor = sColorTokens[i].value;
            *outConsumed = (u8)len;
            return TRUE;
        }
    }

    return FALSE;
}

static bool8 PortableTryEncodeDynamicPlaceholderToken(const u8 *src, u8 *outBytes, u8 *outConsumed, u8 *outProduced)
{
    u8 value;
    u8 consumed;

    if (PortableTryMatchLiteralToken(src, "{LV_2}"))
    {
        outBytes[0] = CHAR_EXTRA_SYMBOL;
        outBytes[1] = CHAR_LV_2;
        *outConsumed = (u8)strlen("{LV_2}");
        *outProduced = 2;
        return TRUE;
    }

    if (PortableTryMatchLiteralToken(src, "{DYNAMIC "))
    {
        if (PortableTryReadHexByte(src + strlen("{DYNAMIC "), &value, &consumed)
         && src[strlen("{DYNAMIC ") + consumed] == '}')
        {
            outBytes[0] = CHAR_DYNAMIC;
            outBytes[1] = value;
            *outConsumed = (u8)(strlen("{DYNAMIC ") + consumed + 1);
            *outProduced = 2;
            return TRUE;
        }
    }

    if (PortableTryMatchLiteralToken(src, "{COLOR "))
    {
        if (PortableTryEncodeTextColor(src + strlen("{COLOR "), &value, &consumed)
         && src[strlen("{COLOR ") + consumed] == '}')
        {
            outBytes[0] = EXT_CTRL_CODE_BEGIN;
            outBytes[1] = EXT_CTRL_CODE_COLOR;
            outBytes[2] = value;
            *outConsumed = (u8)(strlen("{COLOR ") + consumed + 1);
            *outProduced = 3;
            return TRUE;
        }
    }

    if (PortableTryMatchLiteralToken(src, "{HIGHLIGHT "))
    {
        if (PortableTryEncodeTextColor(src + strlen("{HIGHLIGHT "), &value, &consumed)
         && src[strlen("{HIGHLIGHT ") + consumed] == '}')
        {
            outBytes[0] = EXT_CTRL_CODE_BEGIN;
            outBytes[1] = EXT_CTRL_CODE_HIGHLIGHT;
            outBytes[2] = value;
            *outConsumed = (u8)(strlen("{HIGHLIGHT ") + consumed + 1);
            *outProduced = 3;
            return TRUE;
        }
    }

    if (PortableTryMatchLiteralToken(src, "{SHADOW "))
    {
        if (PortableTryEncodeTextColor(src + strlen("{SHADOW "), &value, &consumed)
         && src[strlen("{SHADOW ") + consumed] == '}')
        {
            outBytes[0] = EXT_CTRL_CODE_BEGIN;
            outBytes[1] = EXT_CTRL_CODE_SHADOW;
            outBytes[2] = value;
            *outConsumed = (u8)(strlen("{SHADOW ") + consumed + 1);
            *outProduced = 3;
            return TRUE;
        }
    }

    if (PortableTryMatchLiteralToken(src, "{CLEAR_TO "))
    {
        if (PortableTryReadHexByte(src + strlen("{CLEAR_TO "), &value, &consumed)
         && src[strlen("{CLEAR_TO ") + consumed] == '}')
        {
            outBytes[0] = EXT_CTRL_CODE_BEGIN;
            outBytes[1] = EXT_CTRL_CODE_CLEAR_TO;
            outBytes[2] = value;
            *outConsumed = (u8)(strlen("{CLEAR_TO ") + consumed + 1);
            *outProduced = 3;
            return TRUE;
        }
    }

    if (PortableTryMatchLiteralToken(src, "{PAUSE_UNTIL_PRESS}"))
    {
        outBytes[0] = EXT_CTRL_CODE_BEGIN;
        outBytes[1] = EXT_CTRL_CODE_PAUSE_UNTIL_PRESS;
        *outConsumed = (u8)strlen("{PAUSE_UNTIL_PRESS}");
        *outProduced = 2;
        return TRUE;
    }

    if (PortableTryMatchLiteralToken(src, "{PAUSE_MUSIC}"))
    {
        outBytes[0] = EXT_CTRL_CODE_BEGIN;
        outBytes[1] = EXT_CTRL_CODE_PAUSE_MUSIC;
        *outConsumed = (u8)strlen("{PAUSE_MUSIC}");
        *outProduced = 2;
        return TRUE;
    }

    if (PortableTryMatchLiteralToken(src, "{RESUME_MUSIC}"))
    {
        outBytes[0] = EXT_CTRL_CODE_BEGIN;
        outBytes[1] = EXT_CTRL_CODE_RESUME_MUSIC;
        *outConsumed = (u8)strlen("{RESUME_MUSIC}");
        *outProduced = 2;
        return TRUE;
    }

    if (PortableTryMatchLiteralToken(src, "{FONT_SMALL}"))
    {
        outBytes[0] = EXT_CTRL_CODE_BEGIN;
        outBytes[1] = EXT_CTRL_CODE_FONT;
        outBytes[2] = FONT_SMALL;
        *outConsumed = (u8)strlen("{FONT_SMALL}");
        *outProduced = 3;
        return TRUE;
    }

    if (PortableTryMatchLiteralToken(src, "{FONT_NORMAL}"))
    {
        outBytes[0] = EXT_CTRL_CODE_BEGIN;
        outBytes[1] = EXT_CTRL_CODE_FONT;
        outBytes[2] = FONT_NORMAL;
        *outConsumed = (u8)strlen("{FONT_NORMAL}");
        *outProduced = 3;
        return TRUE;
    }

    if (PortableTryMatchLiteralToken(src, "{PAUSE "))
    {
        if (PortableTryReadHexByte(src + strlen("{PAUSE "), &value, &consumed)
         && src[strlen("{PAUSE ") + consumed] == '}')
        {
            outBytes[0] = EXT_CTRL_CODE_BEGIN;
            outBytes[1] = EXT_CTRL_CODE_PAUSE;
            outBytes[2] = value;
            *outConsumed = (u8)(strlen("{PAUSE ") + consumed + 1);
            *outProduced = 3;
            return TRUE;
        }
    }

    return FALSE;
}

static const u8 *DynamicPlaceholderTextUtil_NormalizePortableSrc(const u8 *src)
{
    static u8 sPortableNormalizedSrc[1024];
    u8 *dest;

    if (!PortableDynamicStringLooksLikeCString(src))
        return src;

    dest = sPortableNormalizedSrc;
    while (*src != '\0' && *src != EOS)
    {
        u8 decoded;
        u8 consumed;
        u8 produced;
        u8 bytes[4];
        u8 i;

        if (*src == '{' && PortableTryEncodeDynamicPlaceholderToken(src, bytes, &consumed, &produced))
        {
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
            if (src[1] == 'n')
            {
                *dest++ = CHAR_NEWLINE;
                src += 2;
                continue;
            }
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
        }

        *dest++ = PortableAsciiToGba(*src++);
    }

    *dest = EOS;
    return sPortableNormalizedSrc;
}
#endif

#define COLORS(a, b)((a) | (b << 4))

static const u8 sTextColorTable[] =
{
 // [LOW_NYBBLE / 2]                            = 0xXY, // HIGH_NYBBLE
    [OBJ_EVENT_GFX_RED_NORMAL / 2]              = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_RED_BIKE
    [OBJ_EVENT_GFX_RED_SURF / 2]                = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_RED_FIELD_MOVE
    [OBJ_EVENT_GFX_RED_FISH / 2]                = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_RED_VS_SEEKER
    [OBJ_EVENT_GFX_RED_VS_SEEKER_BIKE / 2]      = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_GREEN_NORMAL
    [OBJ_EVENT_GFX_GREEN_BIKE / 2]              = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_GREEN_SURF
    [OBJ_EVENT_GFX_GREEN_FIELD_MOVE / 2]        = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_GREEN_FISH
    [OBJ_EVENT_GFX_GREEN_VS_SEEKER / 2]         = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_GREEN_VS_SEEKER_BIKE
    [OBJ_EVENT_GFX_RS_BRENDAN / 2]              = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_RS_MAY
    [OBJ_EVENT_GFX_LITTLE_BOY / 2]              = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_LITTLE_GIRL
    [OBJ_EVENT_GFX_YOUNGSTER / 2]               = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_BOY
    [OBJ_EVENT_GFX_BUG_CATCHER / 2]             = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_SITTING_BOY
    [OBJ_EVENT_GFX_LASS / 2]                    = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_WOMAN_1
    [OBJ_EVENT_GFX_BATTLE_GIRL / 2]             = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_MAN
    [OBJ_EVENT_GFX_ROCKER / 2]                  = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_FAT_MAN
    [OBJ_EVENT_GFX_WOMAN_2 / 2]                 = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_BEAUTY
    [OBJ_EVENT_GFX_BALDING_MAN / 2]             = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_WOMAN_3
    [OBJ_EVENT_GFX_OLD_MAN_1 / 2]               = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_OLD_MAN_2
    [OBJ_EVENT_GFX_OLD_MAN_LYING_DOWN / 2]      = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_OLD_WOMAN
    [OBJ_EVENT_GFX_TUBER_M_WATER / 2]           = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_TUBER_F
    [OBJ_EVENT_GFX_TUBER_M_LAND / 2]            = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_CAMPER
    [OBJ_EVENT_GFX_PICNICKER / 2]               = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_COOLTRAINER_M
    [OBJ_EVENT_GFX_COOLTRAINER_F / 2]           = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_SWIMMER_M_WATER
    [OBJ_EVENT_GFX_SWIMMER_F_WATER / 2]         = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_SWIMMER_M_LAND
    [OBJ_EVENT_GFX_SWIMMER_F_LAND / 2]          = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_WORKER_M
    [OBJ_EVENT_GFX_WORKER_F / 2]                = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_ROCKET_M
    [OBJ_EVENT_GFX_ROCKET_F / 2]                = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_GBA_KID
    [OBJ_EVENT_GFX_SUPER_NERD / 2]              = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_BIKER
    [OBJ_EVENT_GFX_BLACKBELT / 2]               = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_SCIENTIST
    [OBJ_EVENT_GFX_HIKER / 2]                   = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_FISHER
    [OBJ_EVENT_GFX_CHANNELER / 2]               = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_CHEF
    [OBJ_EVENT_GFX_POLICEMAN / 2]               = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_GENTLEMAN
    [OBJ_EVENT_GFX_SAILOR / 2]                  = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_CAPTAIN
    [OBJ_EVENT_GFX_NURSE / 2]                   = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_CABLE_CLUB_RECEPTIONIST
    [OBJ_EVENT_GFX_UNION_ROOM_RECEPTIONIST / 2] = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_UNUSED_MALE_RECEPTIONIST
    [OBJ_EVENT_GFX_CLERK / 2]                   = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_MG_DELIVERYMAN
    [OBJ_EVENT_GFX_TRAINER_TOWER_DUDE / 2]      = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_PROF_OAK
    [OBJ_EVENT_GFX_BLUE / 2]                    = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_BILL
    [OBJ_EVENT_GFX_LANCE / 2]                   = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_AGATHA
    [OBJ_EVENT_GFX_DAISY / 2]                   = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_LORELEI
    [OBJ_EVENT_GFX_MR_FUJI / 2]                 = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_BRUNO
    [OBJ_EVENT_GFX_BROCK / 2]                   = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_MISTY
    [OBJ_EVENT_GFX_LT_SURGE / 2]                = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_ERIKA
    [OBJ_EVENT_GFX_KOGA / 2]                    = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_FEMALE), // OBJ_EVENT_GFX_SABRINA
    [OBJ_EVENT_GFX_BLAINE / 2]                  = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_GIOVANNI
    [OBJ_EVENT_GFX_MOM / 2]                     = COLORS(NPC_TEXT_COLOR_FEMALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_CELIO
    [OBJ_EVENT_GFX_TEACHY_TV_HOST / 2]          = COLORS(NPC_TEXT_COLOR_MALE, NPC_TEXT_COLOR_MALE), // OBJ_EVENT_GFX_GYM_GUY
    [OBJ_EVENT_GFX_ITEM_BALL / 2]               = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_TOWN_MAP
    [OBJ_EVENT_GFX_POKEDEX / 2]                 = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_CUT_TREE
    [OBJ_EVENT_GFX_ROCK_SMASH_ROCK / 2]         = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_PUSHABLE_BOULDER
    [OBJ_EVENT_GFX_FOSSIL / 2]                  = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_RUBY
    [OBJ_EVENT_GFX_SAPPHIRE / 2]                = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_OLD_AMBER
    [OBJ_EVENT_GFX_GYM_SIGN / 2]                = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_SIGN
    [OBJ_EVENT_GFX_TRAINER_TIPS / 2]            = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_CLIPBOARD
    [OBJ_EVENT_GFX_METEORITE / 2]               = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_LAPRAS_DOLL
    [OBJ_EVENT_GFX_SEAGALLOP / 2]               = COLORS(NPC_TEXT_COLOR_NEUTRAL, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_SNORLAX
    [OBJ_EVENT_GFX_SPEAROW / 2]                 = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_CUBONE
    [OBJ_EVENT_GFX_POLIWRATH / 2]               = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_CLEFAIRY
    [OBJ_EVENT_GFX_PIDGEOT / 2]                 = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_JIGGLYPUFF
    [OBJ_EVENT_GFX_PIDGEY / 2]                  = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_CHANSEY
    [OBJ_EVENT_GFX_OMANYTE / 2]                 = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_KANGASKHAN
    [OBJ_EVENT_GFX_PIKACHU / 2]                 = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_PSYDUCK
    [OBJ_EVENT_GFX_NIDORAN_F / 2]               = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_NIDORAN_M
    [OBJ_EVENT_GFX_NIDORINO / 2]                = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_MEOWTH
    [OBJ_EVENT_GFX_SEEL / 2]                    = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_VOLTORB
    [OBJ_EVENT_GFX_SLOWPOKE / 2]                = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_SLOWBRO
    [OBJ_EVENT_GFX_MACHOP / 2]                  = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_WIGGLYTUFF
    [OBJ_EVENT_GFX_DODUO / 2]                   = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_FEAROW
    [OBJ_EVENT_GFX_MACHOKE / 2]                 = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_LAPRAS
    [OBJ_EVENT_GFX_ZAPDOS / 2]                  = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_MOLTRES
    [OBJ_EVENT_GFX_ARTICUNO / 2]                = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_MEWTWO
    [OBJ_EVENT_GFX_MEW / 2]                     = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_ENTEI
    [OBJ_EVENT_GFX_SUICUNE / 2]                 = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_RAIKOU
    [OBJ_EVENT_GFX_LUGIA / 2]                   = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_HO_OH
    [OBJ_EVENT_GFX_CELEBI / 2]                  = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_KABUTO
    [OBJ_EVENT_GFX_DEOXYS_D / 2]                = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_MON), // OBJ_EVENT_GFX_DEOXYS_A
    [OBJ_EVENT_GFX_DEOXYS_N / 2]                = COLORS(NPC_TEXT_COLOR_MON, NPC_TEXT_COLOR_NEUTRAL), // OBJ_EVENT_GFX_SS_ANNE
};

void DynamicPlaceholderTextUtil_Reset(void)
{
    const u8 **ptr = sStringPointers;
    u8 *fillval = NULL;
    const u8 **ptr2 = ptr + (NELEMS(sStringPointers) - 1);
    
    do
    {
        *ptr2-- = fillval;
    }
    while ((intptr_t)ptr2 >= (intptr_t)ptr);
}

void DynamicPlaceholderTextUtil_SetPlaceholderPtr(u8 idx, const u8 *ptr)
{
    if (idx < NELEMS(sStringPointers))
        sStringPointers[idx] = ptr;
}

u8 *DynamicPlaceholderTextUtil_ExpandPlaceholders(u8 *dest, const u8 *src)
{
#ifdef PORTABLE
    src = DynamicPlaceholderTextUtil_NormalizePortableSrc(src);
#endif
    while (*src != EOS)
    {
        if (*src != CHAR_DYNAMIC)
        {
            *dest++ = *src++;
        }
        else
        {
            src++;
            if (sStringPointers[*src] != NULL)
                dest = StringCopy(dest, sStringPointers[*src]);
            src++;
        }
    }
    *dest = EOS;
    return dest;
}

const u8 *DynamicPlaceholderTextUtil_GetPlaceholderPtr(u8 idx)
{
    return sStringPointers[idx];
}

u8 GetColorFromTextColorTable(u16 graphicId)
{
    u32 test = graphicId >> 1;
    u32 shift = (graphicId & 1) << 2;

    if (test >= NELEMS(sTextColorTable))
        return 3;
    else
        return (sTextColorTable[graphicId >> 1] >> shift) & 0xF;
}
