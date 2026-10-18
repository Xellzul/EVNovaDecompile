// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atutf162.c -- the UTF-16 helpers, exe run 2 (run 1 is in atcharset.c). Notes: Research/notes/atutf16.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "atfold.h"

ULong _unicode_tocase(ULong c, const FoldRange *table, ULong n);

#define kHighSurrogateEnd   0xdbff
#define kSurrogateShift     0x2800
#define kDeseretLower       0x10428
#define kDeseretUpper       0x10400
#define kDeseretCount       0x27
#define kDeseretDelta       0x28
#define kAsciiCaseDelta     0x20

/* unicode_bytecount. STATE: MATCH 100%, exe 8 / obj 8 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052c0be
ULong unicode_bytecount(const unsigned short *s)
{
    ULong n;

    n = 0;
    while (*s != 0) {
        s = s + 1;
        n = n + 2;
    }
    return n;
}

/* unicode_charcount. STATE: MATCH 100%, exe 21 / obj 21, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c0d3
ULong unicode_charcount(const unsigned short *s)
{
    ULong n;
    unsigned short c;

    n = 0;
    while ((c = *s++) != 0) {
        n++;
        if ((c & kSurrogateMask) == kSurrogateHigh &&
            (*s & kSurrogateMask) == kSurrogateLow)
            s++;
    }
    return n;
}

/* unicode_bytecount_limit (the byte offset of a character index). STATE: MATCH 100%, exe 30 / obj 30, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c111
ULong unicode_bytecount_limit(const unsigned short *s, ULong index)
{
    const unsigned short *p;
    unsigned short c;

    p = s;
    while (index-- && (c = *p++) != 0) {
        if ((c & kSurrogateMask) == kSurrogateHigh &&
            (*p & kSurrogateMask) == kSurrogateLow)
            p++;
    }
    return (p - s) * sizeof(unsigned short);
}

/* unicode_charcount_limit (characters in a byte count, stopping at a NUL). STATE: MATCH 100%, exe 28 / obj 28, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c167
ULong unicode_charcount_limit(const unsigned short *s, ULong bytes)
{
    ULong n;
    unsigned short c;

    n = 0;
    while (bytes != 0 && (c = *s++) != 0) {
        bytes -= sizeof(unsigned short);
        n++;
        if ((c & kSurrogateMask) == kSurrogateHigh &&
            (*s & kSurrogateMask) == kSurrogateLow) {
            bytes -= sizeof(unsigned short);
            s++;
        }
    }
    return n;
}

/* unicode_strcpy (strcpy). STATE: MATCH 100%, exe 19 / obj 19, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052c1b5
unsigned short *unicode_strcpy(unsigned short *dst, const unsigned short *src)
{
    unsigned short *d;

    d = dst;
    while ((*d++ = *src++) != 0)
        ;
    return dst;
}

/* unicode_strncmp. STATE: MATCH 100%, exe 134 / obj 134, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052c1de
long unicode_strncmp(const unsigned short *a, ULong lenA,
                  const unsigned short *b, ULong lenB)
{
    const unsigned short *startA;
    const unsigned short *startB;
    const unsigned short *endA;
    const unsigned short *endB;
    unsigned short ca;
    unsigned short cb;
    long result;

    startA = a;
    startB = b;
    if (lenA == kNoLength && lenB == kNoLength) {
        if (a == b)
            return 0;
        while ((ca = *a) == (cb = *b)) {
            if (ca == 0)
                return 0;
            a++;
            b++;
        }
        endA = endB = 0;
    } else {
        if (lenA == kNoLength)
            lenA = unicode_bytecount(a);
        if (lenB == kNoLength)
            lenB = unicode_bytecount(b);
        if (lenA < lenB) {
            result = -1;
            endA = a + lenA;
        } else if (lenA > lenB) {
            result = 1;
            endA = a + lenB;
        } else {
            result = 0;
            endA = a + lenA;
        }
        if (a == b)
            return result;
        while (a < endA && (ca = *a) == (cb = *b)) {
            if (ca == 0)
                break;
            a++;
            b++;
        }
        if (a == endA)
            return result;
        endA = startA + lenA;
        endB = startB + lenB;
    }
    if (ca >= kSurrogateHigh && cb >= kSurrogateHigh) {
        if (ca <= kHighSurrogateEnd && a + 1 != endA &&
            (a[1] & kSurrogateMask) == kSurrogateLow)
            ;
        else if ((ca & kSurrogateMask) == kSurrogateLow && startA != a &&
                 (a[-1] & kSurrogateMask) == kSurrogateHigh)
            ;
        else
            ca -= kSurrogateShift;
        if (cb <= kHighSurrogateEnd && b + 1 != endB &&
            (b[1] & kSurrogateMask) == kSurrogateLow)
            ;
        else if ((cb & kSurrogateMask) == kSurrogateLow && startB != b &&
                 (b[-1] & kSurrogateMask) == kSurrogateHigh)
            ;
        else
            cb -= kSurrogateShift;
    }
    return (long)ca - (long)cb;
}

/* _unicode_findmatch (the unit index of the first character in `set', or with inSet 0 the first
   not in it; -1 - length when there is none). STATE: MATCH 100%, exe 147 / obj 147,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052c381
long _unicode_findmatch(const unsigned short *s, const unsigned short *set, long inSet)
{
    long setLen;
    long i;
    long j;
    ULong code;
    unsigned short c;
    unsigned short next;
    unsigned short next2;
    unsigned short next3;
    long firstPair;
    ULong other;

    for (firstPair = 0; (c = set[firstPair]) != 0 &&
                        (c & kSurrogateHalfMask) != kSurrogateHigh; firstPair++)
        ;
    for (setLen = firstPair; set[setLen] != 0; setLen++)
        ;
    i = 0;
    while ((c = s[i]) != 0) {
        i++;
        if ((c & kSurrogateHalfMask) != kSurrogateHigh) {
            if (inSet != 0) {
                for (j = 0; j < setLen; j++) {
                    if (c == set[j])
                        return i - 1;
                }
            } else {
                for (j = 0; j < setLen; j++) {
                    if (c == set[j])
                        goto nextChar;
                }
                return i - 1;
            }
            continue;
        }
        if ((c & kSurrogateLowBit) == 0 && ((next = s[i]) & kSurrogateMask) == kSurrogateLow) {
            i++;
            code = (c << 10) + next + kSurrogateBias;
        } else {
            code = c;
        }
        if (inSet != 0) {
            for (j = firstPair; j < setLen;) {
                other = set[j++];
                if ((other & kSurrogateMask) == kSurrogateHigh && j < setLen &&
                    ((next2 = set[j]) & kSurrogateMask) == kSurrogateLow) {
                    j++;
                    other = (other << 10) + next2 + kSurrogateBias;
                }
                if (code == other)
                    return i - (code <= kBmpMax ? 1 : 2);
            }
        } else {
            for (j = firstPair; j < setLen;) {
                other = set[j++];
                if ((other & kSurrogateMask) == kSurrogateHigh && j < setLen &&
                    ((next3 = set[j]) & kSurrogateMask) == kSurrogateLow) {
                    j++;
                    other = (other << 10) + next3 + kSurrogateBias;
                }
                if (code == other)
                    goto nextChar;
            }
            return i - (code <= kBmpMax ? 1 : 2);
        }
nextChar:
        ;
    }
    return -1 - i;
}

/* unicode_strpbrk (strpbrk). STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c54a
unsigned short *unicode_strpbrk(const unsigned short *s, const unsigned short *set)
{
    long i;

    i = _unicode_findmatch(s, set, 1);
    if (i >= 0)
        return (unsigned short *)s + i;
    return 0;
}

/* _unicode_tocase. STATE: MATCH 100%, exe 102 / obj 102, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052c56c
ULong _unicode_tocase(ULong c, const FoldRange *table, ULong n)
{
    ULong lo;
    ULong hi;
    ULong mid;
    ULong first;
    long found;
    unsigned char count;
    unsigned char stride;
    short delta;
    unsigned short info;

    if ((c >= 0x2500 && c <= 0xfeff) || (c >= 0x600 && c <= 0x1dff) ||
        (c >= 0x2000 && c <= 0x20ff) || (c >= 0x2200 && c <= 0x23ff))
        return c;
    lo = 0;
    hi = n - 1;
    while (found = -1, lo <= hi) {
        mid = (lo + hi) >> 1;
        first = table[mid].first;
        if (c < first) {
            hi = mid - 1;
            continue;
        }
        info = table[mid].info;
        count = info >> 8;
        if (c > first + count) {
            lo = mid + 1;
            continue;
        }
        stride = info;
        delta = table[mid].delta;
        found = mid;
        break;
    }
    if (found == -1) {
        if (table == kUpperRanges) {
            first = kDeseretLower;
            count = kDeseretCount;
            stride = 1;
            delta = -kDeseretDelta;
        } else {
            first = kDeseretUpper;
            count = kDeseretCount;
            stride = 1;
            delta = kDeseretDelta;
        }
        if (c >= first && c <= first + count)
            found = 0;
    }
    if (found != -1) {
        if (count != 0 && (c - first) % stride != 0)
            return c;
        return c + delta;
    }
    return c;
}

/* unicode_toupper. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0052c6bd
ULong unicode_toupper(ULong c)
{
    if (c <= kAsciiMax) {
        if (c >= 'a' && c <= 'z')
            c = c - kAsciiCaseDelta;
        return c;
    }
    return _unicode_tocase(c, kUpperRanges, kUpperRangeCount);
}

#define kBadCharMark        0x13

/* The UTF-8 / UTF-16 conversions of convertutf.c, retried past each illegal character: the
   character is skipped and, when writing, replaced by kBadCharMark. */

/* count_UTF8_to_UTF16 (UTF-16 bytes a UTF-8 string needs: a conversion into a null-based
   destination). STATE: MATCH 100%, exe 39 / obj 39, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052c6e5
ULong count_UTF8_to_UTF16(const unsigned char *s, ULong limit)
{
    const unsigned char *src;
    const unsigned char *srcEnd;
    unsigned short *d;
    unsigned short *end;
    long budget;

    src = s;
    srcEnd = (const unsigned char *)kNoLength;
    d = 0;
    end = (unsigned short *)kNoLength;
    budget = -1;
    if (limit != kNoLength) {
        limit -= limit % sizeof(unsigned short);
        end = d + limit / sizeof(unsigned short) - 1;
    }
    while (ConvertUTF8toUTF16(&src, srcEnd, &d, end, &budget, kStrictConversion) == kSourceIllegal) {
        src++;
        d++;
        budget--;
    }
    return (ULong)d;
}

/* count_UTF16_to_UTF8 (UTF-8 bytes a UTF-16 string needs, as count_UTF8_to_UTF16). STATE: MATCH 100%,
   exe 36 / obj 36, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052c753
ULong count_UTF16_to_UTF8(const unsigned short *s, ULong limit)
{
    const unsigned short *src;
    const unsigned short *srcEnd;
    unsigned char *d;
    unsigned char *end;
    long budget;

    src = s;
    srcEnd = (const unsigned short *)kNoLength;
    d = 0;
    end = (unsigned char *)kNoLength;
    budget = -1;
    if (limit != kNoLength) {
        limit -= limit % sizeof(unsigned char);
        end = d + limit / sizeof(unsigned char) - 1;
    }
    while (ConvertUTF16toUTF8(&src, srcEnd, &d, end, &budget, kStrictConversion) == kSourceIllegal) {
        src++;
        d++;
        budget--;
    }
    return (ULong)d;
}

/* convert_UTF8_to_UTF16 (UTF-8 to UTF-16). STATE: MATCH 100%, exe 53 / obj 53, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c7b7
unsigned short *convert_UTF8_to_UTF16(unsigned short *dst, const unsigned char *src, ULong dstBytes,
                             long budget, long terminate)
{
    const unsigned char *s;
    const unsigned char *sEnd;
    unsigned short *d;
    unsigned short *end;

    s = src;
    sEnd = (const unsigned char *)kNoLength;
    d = dst;
    end = (unsigned short *)kNoLength;
    if (dstBytes != kNoLength) {
        dstBytes -= dstBytes % sizeof(unsigned short);
        end = dst + dstBytes / sizeof(unsigned short);
        if (terminate != 0)
            end--;
    }
    while (ConvertUTF8toUTF16(&s, sEnd, &d, end, &budget, kStrictConversion) == kSourceIllegal) {
        s++;
        *d++ = kBadCharMark;
        budget--;
    }
    if (terminate != 0)
        *d = 0;
    return dst;
}

/* convert_UTF16_to_UTF8 (UTF-16 to UTF-8). STATE: MATCH 100%, exe 50 / obj 50, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c842
unsigned char *convert_UTF16_to_UTF8(unsigned char *dst, const unsigned short *src, ULong dstBytes,
                            long budget, long terminate)
{
    const unsigned short *s;
    const unsigned short *sEnd;
    unsigned char *d;
    unsigned char *end;

    s = src;
    sEnd = (const unsigned short *)kNoLength;
    d = dst;
    end = (unsigned char *)kNoLength;
    if (dstBytes != kNoLength) {
        dstBytes -= dstBytes % sizeof(unsigned char);
        end = dst + dstBytes / sizeof(unsigned char);
        if (terminate != 0)
            end--;
    }
    while (ConvertUTF16toUTF8(&s, sEnd, &d, end, &budget, kStrictConversion) == kSourceIllegal) {
        s++;
        *d++ = kBadCharMark;
        budget--;
    }
    if (terminate != 0)
        *d = 0;
    return dst;
}

/* convert_UTF32_to_UTF16 (UTF-32 to UTF-16). STATE: MATCH 100%, exe 53 / obj 53, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c8c1
unsigned short *convert_UTF32_to_UTF16(unsigned short *dst, const ULong *src, ULong dstBytes,
                             long budget, long terminate)
{
    const ULong *s;
    const ULong *sEnd;
    unsigned short *d;
    unsigned short *end;

    s = src;
    sEnd = (const ULong *)kNoLength;
    d = dst;
    end = (unsigned short *)kNoLength;
    if (dstBytes != kNoLength) {
        dstBytes -= dstBytes % sizeof(unsigned short);
        end = dst + dstBytes / sizeof(unsigned short);
        if (terminate != 0)
            end--;
    }
    while (ConvertUTF32toUTF16(&s, sEnd, &d, end, &budget, kStrictConversion) == kSourceIllegal) {
        s++;
        *d++ = kBadCharMark;
        budget--;
    }
    if (terminate != 0)
        *d = 0;
    return dst;
}
