// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerunquote.c -- BurgerLib StStringManager: trimming, extensions, the quoted-value reader, number formatting, string copies. Notes: Research/notes/burgerunquote.md */

#include "burgertypes.h"
#include "burger.h"

/* The member's one byte of .bss; no live code reads it. */
namespace Burger {
class Str {
public:
    static const char Empty[1];
};
}

const char Burger::Str::Empty[1];

#ifdef __cplusplus
extern "C" {
#endif

/* StripLeadingAndTrailingWhiteSpace (BurgerLib StStringManager): spaces and tabs off both
   ends, in place. STATE: MATCH 100%, exe 34 / obj 34, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fbec0
void StripLeadingAndTrailingWhiteSpace(char *text)
{
    char *end;
    Word c;
    char *out;

    end = text;
    c = (Word8)*text++;
    if (c == ' ' || c == '\t') {
        do {
            c = (Word8)*text++;
        } while (c == ' ' || c == '\t');
    }
    if (c != 0) {
        out = end;
        do {
            *out++ = (char)c;
            if (c != ' ' && c != '\t')
                end = out;
            c = (Word8)*text++;
        } while (c != 0);
    }
    *end = (char)c;
}

/* GetFileExtension (BurgerLib StStringManager): past the last '.', or the terminator when
   there is none. STATE: MATCH 100.0%, exe 17 / obj 17, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fbf20
char *GetFileExtension(const char *name)
{
    Word c;
    const char *p;

    c = (Word8)*name;
    if (c != 0) {
        p = name;
        name = 0;
        do {
            ++p;
            if (c == '.')
                name = p;
            c = (Word8)*p;
        } while (c != 0);
        if (name == 0)
            name = p;
    }
    return (char *)name;
}

/* ParseQuotedString. STATE: MATCH 100%, exe 51 / obj 51 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fbf50
char *ParseQuotedString(char *out, long outSize, const char *src)
{
    long room;
    unsigned long c;
    long next;

    if (*src == '"') {
        room = outSize;
        if (room != 0)
            --room;
        for (;;) {
            src = src + 1;
            c = (unsigned char)*src;
            if (c == '\t') {
                c = ' ';
            } else if (c == '"') {
                src = src + 1;
                next = (unsigned char)*src;
                if (next != '"')
                    break;
            } else if (c == 0 || c == '\n') {
                break;
            } else if (c == '\r') {
                if (*src == '\n')
                    src = src + 1;
                break;
            }
            if (room != 0) {
                *out = c;
                out = out + 1;
                room = room - 1;
            }
        }
    }
    if (outSize != 0)
        *out = 0;
    return (char *)src;
}

#define kDigitMask     0xff
#define kSawDigit      0x8000
#define kMaxDigits     10
#define kMaxHexDigits  8

static const char NibbleToAsciiUC[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static const ULong kPow10[kMaxDigits] = {
    1UL, 10UL, 100UL, 1000UL, 10000UL,
    100000UL, 1000000UL, 10000000UL, 100000000UL, 1000000000UL
};

/* NumberToAscii (BurgerLib StStringManager). STATE: MATCH 100.0%, exe 46 / obj 46,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fbfe0
char *NumberToAscii(char *dst, ULong value, ULong flags)
{
    ULong digits;
    ULong place;
    ULong digit;

    digits = flags & kDigitMask;
    if (digits != 0 && digits < kMaxDigits)
        value = value % kPow10[digits];
    else
        digits = kMaxDigits;

    --digits;
    while (digits != 0) {
        place = kPow10[digits];
        digit = '0';
        if (value >= place) {
            flags = flags | kSawDigit;
            do {
                value = value - place;
                digit = digit + 1;
            } while (value >= place);
        }
        if ((flags & kSawDigit) != 0) {
            *dst = digit;
            dst = dst + 1;
        }
        --digits;
    }

    *dst = (char)(value + '0');
    if ((flags & kNoTerminator) == 0)
        dst[1] = 0;
    return dst + 1;
}

/* NumberToAsciiHex (BurgerLib StStringManager; the 2007 archive's Burger::Str::ToAsciiHex):
   the same flags as NumberToAscii, eight digits at most. STATE: MATCH 100.0%, exe 48 / obj 48,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fc080
char *NumberToAsciiHex(char *dst, ULong value, ULong flags)
{
    ULong digits;
    ULong nibble;

    digits = flags & kDigitMask;
    if (digits != 0 && digits < kMaxHexDigits)
        value = value << ((kMaxHexDigits - digits) * 4);
    else
        digits = kMaxHexDigits;

    while (--digits) {
        nibble = value >> 28;
        if ((flags & kSawDigit) != 0 || nibble != 0) {
            flags = flags | kSawDigit;
            *dst = NibbleToAsciiUC[nibble];
            dst = dst + 1;
        }
        value = value << 4;
    }

    *dst = NibbleToAsciiUC[value >> 28];
    if ((flags & kNoTerminator) == 0)
        dst[1] = 0;
    return dst + 1;
}

extern const float XFloat_Inf;
extern const float XFloat_Nan;
extern const float XFloat_Min;
extern const float XFloat_Max;
extern const double XDouble_Inf;
extern const double XDouble_Nan;
extern const double XDouble_Min;
extern const double XDouble_Max;

static const float PowerOf10FltTable[6] = {
    1.0e1f, 1.0e2f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f
};

static const double PowerOf10DblTable[9] = {
    1.0e1, 1.0e2, 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256
};

#define kFloatDigits   9
#define kDoubleDigits  18

/* ToFloat and ToDouble (BurgerLib StStringManager's Burger::Str::ToFloat / ToDouble). Not linked. */
float ToFloat(const char *pInput, const char **pDest)
{
    float Sign;
    float Result;
    Word c;
    Word uDigits;
    Word uPoint;
    Word uSeen;
    int iExp;
    int iValue;
    int iPlaces;
    Word i;
    const float *pPower;
    float Scale;
    Word8 Buffer[kFloatDigits];

    if (pDest != 0)
        *pDest = pInput;
    c = (Word8)*pInput++;
    while (c == ' ' || c == '\t')
        c = (Word8)*pInput++;
    Sign = 1.0f;
    if (c == '+') {
        c = (Word8)*pInput++;
    } else if (c == '-') {
        Sign = -1.0f;
        c = (Word8)*pInput++;
    }
    if (c == 'N' || c == 'n') {
        if ((pInput[0] == 'A' || pInput[0] == 'a') && (pInput[1] == 'N' || pInput[1] == 'n'))
            goto IsNan;
    } else if (c == 'I' || c == 'i') {
        /* BUG: "INF" is refused, only the lower-case 'f' is tested */
        if ((pInput[0] == 'N' || pInput[0] == 'n') && pInput[1] == 'f')
            goto IsInf;
    }

    uDigits = 0;
    uPoint = 0;
    uSeen = 0;
    for (;;) {
        if (c == '.') {
            if (uPoint != 0)
                break;
            uPoint = 1;
        } else if (c - '0' < 10) {
            uSeen = 1;
            if (uPoint != 0)
                ++uPoint;
            if (uDigits != 0 || c != '0') {
                if (uDigits < kFloatDigits)
                    Buffer[uDigits] = c - '0';
                ++uDigits;
            }
        } else {
            break;
        }
        c = (Word8)*pInput++;
    }
    if (uSeen == 0)
        return 0.0f;

    iExp = 0;
    if (c == 'e' || c == 'E') {
        const char *pExp;
        Word uNegative;

        pExp = pInput + 1;
        uNegative = 0;
        c = (Word8)*pInput;
        if (c == '+') {
            c = (Word8)*pExp++;
        } else if (c == '-') {
            uNegative = 1;
            c = (Word8)*pExp++;
        }
        c = c - '0';
        if (c < 10) {
            do {
                if (iExp < 100)
                    iExp = iExp * 10 + c;
                c = (Word8)*pExp++ - '0';
            } while (c < 10);
            pInput = pExp;
            if (uNegative != 0)
                iExp = -iExp;
        }
    }
    if (pDest != 0)
        *pDest = pInput - 1;

    if (uPoint != 0)
        iExp -= uPoint - 1;
    if (uDigits > kFloatDigits) {
        iExp += uDigits - kFloatDigits;
        uDigits = kFloatDigits;
    }
    while (uDigits != 0 && Buffer[uDigits - 1] == 0) {
        ++iExp;
        --uDigits;
    }
    if (uDigits == 0)
        return 0.0f;

    iValue = 0;
    for (i = 0; i < uDigits; ++i)
        iValue = iValue * 10 + Buffer[i];
    Result = (float)iValue;

    if (iExp > 0) {
        iPlaces = iExp;
        if (iPlaces >= 38) {
            Result *= 1.0e38f;
            iPlaces -= 38;
        }
        if (iPlaces != 0) {
            Scale = 1.0f;
            pPower = PowerOf10FltTable;
            do {
                if (iPlaces & 1)
                    Scale *= *pPower;
                ++pPower;
                iPlaces >>= 1;
            } while (iPlaces != 0);
            Result *= Scale;
        }
    } else if (iExp < 0) {
        iPlaces = -iExp;
        if (iPlaces >= 38) {
            Result /= 1.0e38f;
            iPlaces -= 38;
        }
        if (iPlaces != 0) {
            Scale = 1.0f;
            pPower = PowerOf10FltTable;
            do {
                if (iPlaces & 1)
                    Scale *= *pPower;
                ++pPower;
                iPlaces >>= 1;
            } while (iPlaces != 0);
            Result /= Scale;
        }
    }

    iExp += uDigits;
    if (iExp > 39)
        Result = XFloat_Max;
    else if (iExp < -37)
        Result = XFloat_Min;
    return Result * Sign;

IsNan:
    Result = XFloat_Nan;
    if (Sign < 0.0f)
        Result = -Result;
    if (pDest != 0)
        *pDest = pInput + 2;
    return Result;

IsInf:
    Result = XFloat_Inf * Sign;
    if (pDest != 0)
        *pDest = pInput + 2;
    return Result;
}

double ToDouble(const char *pInput, const char **pDest)
{
    double Sign;
    double Result;
    Word c;
    Word uDigits;
    Word uPoint;
    Word uSeen;
    int iExp;
    int iHigh;
    int iLow;
    int iPlaces;
    Word i;
    const double *pPower;
    double Scale;
    Word8 Buffer[kDoubleDigits];

    if (pDest != 0)
        *pDest = pInput;
    c = (Word8)*pInput++;
    while (c == ' ' || c == '\t')
        c = (Word8)*pInput++;
    Sign = 1.0;
    if (c == '+') {
        c = (Word8)*pInput++;
    } else if (c == '-') {
        Sign = -1.0;
        c = (Word8)*pInput++;
    }
    if (c == 'N' || c == 'n') {
        if ((pInput[0] == 'A' || pInput[0] == 'a') && (pInput[1] == 'N' || pInput[1] == 'n')) {
            Result = XDouble_Nan;
            if (Sign < 0.0)
                Result = -Result;
            if (pDest != 0)
                *pDest = pInput + 2;
            return Result;
        }
    } else if (c == 'I' || c == 'i') {
        /* BUG: "INF" is refused, only the lower-case 'f' is tested */
        if ((pInput[0] == 'N' || pInput[0] == 'n') && pInput[1] == 'f') {
            Result = XDouble_Inf * Sign;
            if (pDest != 0)
                *pDest = pInput + 2;
            return Result;
        }
    }

    uDigits = 0;
    uPoint = 0;
    uSeen = 0;
    for (;;) {
        if (c == '.') {
            if (uPoint != 0)
                break;
            uPoint = 1;
        } else if (c - '0' < 10) {
            uSeen = 1;
            if (uPoint != 0)
                ++uPoint;
            if (uDigits != 0 || c != '0') {
                if (uDigits < kDoubleDigits)
                    Buffer[uDigits] = c - '0';
                ++uDigits;
            }
        } else {
            break;
        }
        c = (Word8)*pInput++;
    }
    if (uSeen == 0)
        return 0.0;

    iExp = 0;
    if (c == 'e' || c == 'E') {
        const char *pExp;
        Word uNegative;

        pExp = pInput + 1;
        uNegative = 0;
        c = (Word8)*pInput;
        if (c == '+') {
            c = (Word8)*pExp++;
        } else if (c == '-') {
            uNegative = 1;
            c = (Word8)*pExp++;
        }
        c = c - '0';
        if (c < 10) {
            do {
                if (iExp < 1000)
                    iExp = iExp * 10 + c;
                c = (Word8)*pExp++ - '0';
            } while (c < 10);
            pInput = pExp;
            if (uNegative != 0)
                iExp = -iExp;
        }
    }
    if (pDest != 0)
        *pDest = pInput - 1;

    if (uPoint != 0)
        iExp -= uPoint - 1;
    if (uDigits > kDoubleDigits) {
        iExp += uDigits - kDoubleDigits;
        uDigits = kDoubleDigits;
    }
    while (uDigits != 0 && Buffer[uDigits - 1] == 0) {
        ++iExp;
        --uDigits;
    }
    if (uDigits == 0)
        return 0.0;

    iHigh = 0;
    iLow = 0;
    for (i = 0; i < uDigits; ++i) {
        if (uDigits - i > kFloatDigits)
            iHigh = iHigh * 10 + Buffer[i];
        else
            iLow = iLow * 10 + Buffer[i];
    }
    Result = (double)iHigh * 1.0e9 + (double)iLow;

    if (iExp > 0) {
        iPlaces = iExp;
        if (iPlaces >= 308) {
            Result *= 1.0e308;
            iPlaces -= 308;
        }
        if (iPlaces != 0) {
            Scale = 1.0;
            pPower = PowerOf10DblTable;
            do {
                if (iPlaces & 1)
                    Scale *= *pPower;
                ++pPower;
                iPlaces >>= 1;
            } while (iPlaces != 0);
            Result *= Scale;
        }
    } else if (iExp < 0) {
        iPlaces = -iExp;
        if (iPlaces >= 308) {
            Result /= 1.0e308;
            iPlaces -= 308;
        }
        if (iPlaces != 0) {
            Scale = 1.0;
            pPower = PowerOf10DblTable;
            do {
                if (iPlaces & 1)
                    Scale *= *pPower;
                ++pPower;
                iPlaces >>= 1;
            } while (iPlaces != 0);
            Result /= Scale;
        }
    }

    iExp += uDigits;
    if (iExp > 309)
        Result = XDouble_Max;
    else if (iExp < -307)
        Result = XDouble_Min;
    return Result * Sign;
}

#define kPascalMaxLen   0xff

/* StrCopy (BurgerLib StStringManager): a C string copied into a new pointer.
   STATE: MATCH 100.0%, exe 27 / obj 27, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fc120
char *StrCopy(const char *text)
{
    Word length;
    char *copy;

    length = strlen(text) + 1;
    copy = (char *)AllocAPointer(length);
    if (copy != 0)
        memcpy(copy, text, length);
    return copy;
}

/* C string to Pascal string, capped at 255 payload bytes. */
/* CStr2PStr. STATE: MATCH 100%, exe 27 / obj 27 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fc170
void CStr2PStr(char *dst, const char *src)
{
    ULong c;
    ULong len;
    ULong next;
    char *out;

    len = 0;
    out = dst + 1;
    c = (unsigned char)*src++;
    while (c != 0) {
        next = (unsigned char)*src++;
        *out++ = c;
        if (++len >= kPascalMaxLen)
            break;
        c = next;
    }
    dst[0] = (unsigned char)len;
}

/* Pascal string to C string: the length byte leads, the terminator is ours. */
/* PStr2CStr (BurgerLib StStringManager). STATE: MATCH 100.0%, exe 17 / obj 17, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fc1c0
void PStr2CStr(char *dst, const unsigned char *src)
{
    unsigned long len;

    len = *src;
    src = src + 1;
    while (len != 0) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
        --len;
    }
    *dst = 0;
}

/* StrCopyHandle (BurgerLib StStringManager): a C string copied into a new handle.
   STATE: MATCH 100.0%, exe 38 / obj 38, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fc1f0
Handle StrCopyHandle(const char *text)
{
    Word length;
    Handle handle;

    if (text != 0) {
        length = strlen(text) + 1;
        handle = (Handle)AllocAHandle2(length, 0);
        if (handle != 0) {
            memcpy(*handle, text, length);
            return handle;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
