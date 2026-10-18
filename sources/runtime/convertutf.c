// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* convertutf.c -- Unicode, Inc.'s ConvertUTF.c (UTF-16 <-> UTF-8), altered: a character budget, a NUL-stop flag and an illegal-character answer. Notes: Research/notes/convertutf.md */

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"

typedef ULong UTF32;
typedef unsigned short UTF16;
typedef unsigned char UTF8;

#define kSurHighStart       0xd800
#define kSurHighEnd         0xdbff
#define kSurLowStart        0xdc00
#define kSurLowEnd          0xdfff
#define kHalfShift          10
#define kHalfBase           0x10000
#define kHalfMask           0x3ff
#define kMaxLegalUTF32      0x10ffff
#define kReplacementChar    0xfffd
#define kByteMask           0xbf
#define kByteMark           0x80

signed char trailingBytesForUTF8[256] = {                /* 0x00582050 trailing bytes for UTF-8 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

UTF32 offsetsFromUTF8[6] = {                        /* 0x00582150 offsets from UTF-8 */
    0x00000000UL, 0x00003080UL, 0x000e2080UL,
    0x03c82080UL, 0xfa082080UL, 0x82082080UL
};

UTF8 firstByteMark[7] = {                         /* 0x00582168 first byte mark */
    0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc
};

/* ConvertUTF32toUTF16 (ConvertUTF32toUTF16; a null target counts the units). STATE: MATCH 100%,
   exe 95 / obj 95, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00541330
long ConvertUTF32toUTF16(const UTF32 **sourceStart, const UTF32 *sourceEnd,
                  UTF16 **targetStart, UTF16 *targetEnd, long *budget, ULong flags)
{
    long result = kSourceExhausted;
    const UTF32 *source = *sourceStart;
    UTF16 *target = *targetStart;
    long writing = *targetStart ? true : false;
    UTF32 ch;

    while (source < sourceEnd) {
        if (*budget == 0) {
            result = kBudgetSpent;
            break;
        }
        if (writing && target >= targetEnd) {
            result = kTargetExhausted;
            break;
        }
        ch = *source;
        if ((flags & kPastNul) == 0 && ch == 0) {
            result = kConversionOK;
            break;
        }
        source++;
        if (ch <= kBmpMax) {
            if ((flags & kStrictConversion) != 0 && ch >= kSurHighStart && ch <= kSurLowEnd) {
                source--;
                result = kSourceIllegal;
                break;
            }
            if (writing)
                *target++ = (UTF16)ch;
            else
                target++;
        } else if (ch > kMaxLegalUTF32) {
            if ((flags & kStrictConversion) != 0) {
                result = kSourceIllegal;
            } else {
                if (writing)
                    *target++ = kReplacementChar;
                else
                    target++;
            }
        } else {
            if (writing) {
                if (target + 1 >= targetEnd) {
                    source--;
                    result = kTargetExhausted;
                    break;
                }
                ch -= kHalfBase;
                target[0] = (UTF16)((ch >> kHalfShift) + kSurHighStart);
                target[1] = (UTF16)((ch & kHalfMask) + kSurLowStart);
                target += 2;
            } else {
                target += 2;
            }
        }
        (*budget)--;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* ConvertUTF16toUTF8 (ConvertUTF16toUTF8; a null target counts the bytes). STATE: MATCH 100%,
   exe 142 / obj 142, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0054144b
long ConvertUTF16toUTF8(const UTF16 **sourceStart, const UTF16 *sourceEnd,
                  UTF8 **targetStart, UTF8 *targetEnd, long *budget, ULong flags)
{
    long result = kSourceExhausted;
    const UTF16 *source = *sourceStart;
    UTF8 *target = *targetStart;
    long writing = *targetStart ? true : false;
    const UTF16 *oldSource;
    UTF32 ch;
    UTF32 ch2;
    unsigned short bytesToWrite;

    while (source < sourceEnd) {
        bytesToWrite = 0;
        oldSource = source;
        if (*budget == 0) {
            result = kBudgetSpent;
            break;
        }
        ch = *source;
        if ((flags & kPastNul) == 0 && ch == 0) {
            result = kConversionOK;
            break;
        }
        source++;
        if (ch >= kSurHighStart && ch <= kSurHighEnd && source < sourceEnd) {
            ch2 = *source;
            if (ch2 >= kSurLowStart && ch2 <= kSurLowEnd) {
                ch = ((ch - kSurHighStart) << kHalfShift) + (ch2 - kSurLowStart) + kHalfBase;
                source++;
            } else if ((flags & kStrictConversion) != 0) {
                source--;
                result = kSourceIllegal;
                break;
            }
        } else if ((flags & kStrictConversion) != 0 && ch >= kSurLowStart && ch <= kSurLowEnd) {
            source--;
            result = kSourceIllegal;
            break;
        }

        if (ch < 0x80)
            bytesToWrite = 1;
        else if (ch < 0x800)
            bytesToWrite = 2;
        else if (ch < 0x10000)
            bytesToWrite = 3;
        else if (ch < 0x200000)
            bytesToWrite = 4;
        else {
            bytesToWrite = 2;
            ch = kReplacementChar;
        }

        target += bytesToWrite;
        if (writing) {
            if (target > targetEnd) {
                source = oldSource;
                target -= bytesToWrite;
                result = kTargetExhausted;
                break;
            }
            switch (bytesToWrite) {
            case 4:
                *--target = (UTF8)((ch | kByteMark) & kByteMask);
                ch >>= 6;
            case 3:
                *--target = (UTF8)((ch | kByteMark) & kByteMask);
                ch >>= 6;
            case 2:
                *--target = (UTF8)((ch | kByteMark) & kByteMask);
                ch >>= 6;
            case 1:
                *--target = (UTF8)(ch | firstByteMark[bytesToWrite]);
            }
            target += bytesToWrite;
        }
        (*budget)--;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* isLegalUTF8 (isLegalUTF8). STATE: MATCH 100%, exe 66 / obj 66, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005415f1
Word isLegalUTF8(const UTF8 *source, long length)
{
    UTF8 a;
    const UTF8 *srcptr;

    srcptr = source + length;
    switch (length) {
    default:
        return 0;
    case 4:
        if ((a = *--srcptr) < 0x80 || a > 0xbf)
            return 0;
    case 3:
        if ((a = *--srcptr) < 0x80 || a > 0xbf)
            return 0;
    case 2:
        if ((a = *--srcptr) > 0xbf)
            return 0;
        switch (*source) {
        case 0xe0:
            if (a < 0xa0)
                return 0;
            break;
        case 0xf0:
            if (a < 0x90)
                return 0;
            break;
        case 0xf4:
            if (a > 0x8f)
                return 0;
            break;
        default:
            if (a < 0x80)
                return 0;
        }
    case 1:
        if (*source >= 0x80 && *source < 0xc2)
            return 0;
    }
    if (*source > 0xf4)
        return 0;
    return 1;
}

/* ConvertUTF8toUTF16 (ConvertUTF8toUTF16; a null target counts the bytes). STATE: MATCH 100%,
   exe 146 / obj 146, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00541684
long ConvertUTF8toUTF16(const UTF8 **sourceStart, const UTF8 *sourceEnd,
                  UTF16 **targetStart, UTF16 *targetEnd, long *budget, ULong flags)
{
    long result = kSourceExhausted;
    const UTF8 *source = *sourceStart;
    UTF16 *target = *targetStart;
    long writing = *targetStart ? true : false;
    UTF32 ch;
    unsigned short extraBytesToRead;
    long length;

    while (source < sourceEnd) {
        if (*budget == 0) {
            result = kBudgetSpent;
            break;
        }
        ch = *source;
        if ((flags & kPastNul) == 0 && ch == 0) {
            result = kConversionOK;
            break;
        }
        extraBytesToRead = trailingBytesForUTF8[ch];
        if (source + extraBytesToRead >= sourceEnd) {
            result = kSourcePartial;
            break;
        }
        length = extraBytesToRead + 1;
        if (!isLegalUTF8(source, length)) {
            result = kSourceIllegal;
            break;
        }
        ch = 0;
        switch (extraBytesToRead) {
        case 3:
            ch += *source++;
            ch <<= 6;
        case 2:
            ch += *source++;
            ch <<= 6;
        case 1:
            ch += *source++;
            ch <<= 6;
        case 0:
            ch += *source++;
        }
        ch -= offsetsFromUTF8[extraBytesToRead];

        if (writing && target >= targetEnd) {
            source -= length;
            result = kTargetExhausted;
            break;
        }
        if (ch <= kBmpMax) {
            if ((flags & kStrictConversion) != 0 && ch >= kSurHighStart && ch <= kSurLowEnd) {
                source -= length;
                result = kSourceIllegal;
                break;
            }
            if (writing)
                *target++ = (UTF16)ch;
            else
                target++;
        } else if (ch > kMaxLegalUTF32) {
            if ((flags & kStrictConversion) != 0) {
                result = kSourceIllegal;
                source -= length;
                break;
            }
            if (writing)
                *target++ = kReplacementChar;
            else
                target++;
        } else {
            if (writing) {
                if (target + 1 >= targetEnd) {
                    source -= length;
                    result = kTargetExhausted;
                    break;
                }
                target[0] = (UTF16)(((ch - kHalfBase) >> kHalfShift) + kSurHighStart);
                target[1] = (UTF16)((ch & kHalfMask) + kSurLowStart);
                target += 2;
            } else {
                target += 2;
            }
        }
        (*budget)--;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}
