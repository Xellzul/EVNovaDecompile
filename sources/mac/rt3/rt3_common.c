/* rt3_common.c -- RT3 (Ambrosia's registration toolkit), the license code arithmetic shared by the library's tools; the mac 1.1.1 librt3_nonag.a(rt3_common.o) (mac only). Notes: Research/notes/rt3mac.md */
#include "rt3_lib.h"

unsigned char gRT3_EightBitToFiveBitChars[256];
unsigned short gRT3_CodeVersion = 0;
char gRT3_FiveBitToEightBitChars[32] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

/* RT3_InitTables (mac _RT3_InitTables, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_InitTables i386 0x000d477f ppc 0x000df500
void RT3_InitTables(void)
{
    static unsigned char gInited;
    ULong c;
    ULong i;

    if (gInited == 0) {
        for (c = 0; c < 0x100; c++) {
            gRT3_EightBitToFiveBitChars[c] = kNoDigit;
            for (i = 0; i < 0x20; i++) {
                if (c == gRT3_FiveBitToEightBitChars[i])
                    gRT3_EightBitToFiveBitChars[c] = i;
            }
        }
        for (c = 'a'; c <= 'z'; c++)
            gRT3_EightBitToFiveBitChars[c] = gRT3_EightBitToFiveBitChars[c - ('a' - 'A')];
        gInited = 1;
    }
}

/* RT3_CalcTimePeriod (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_CalcTimePeriod i386 0x000d47f6 ppc 0x000df5a0
unsigned char RT3_CalcTimePeriod(ULong seconds)
{
    return seconds > 0x3a500ecf ? (seconds - 0x3a46d450) / 1209600 : 0;
}

/* RT3_FixProductName (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_FixProductName i386 0x000d481b ppc 0x000df5d4
void RT3_FixProductName(char *out, const char *src)
{
    char *start;
    ULong n;
    char c;

    start = out;
    while ((n = out - start) < kTextSize - 1 && (*out = *src++) != 0) {
        c = *out;
        if (c >= 'a' && c <= 'z') {
            *out += 'A' - 'a';
            out++;
        } else if (c >= 'A' && c <= 'Z') {
            out++;
        } else if (c >= '0' && c <= '9') {
            out++;
        } else {
            *out++ = '_';
        }
    }
    if (n == kTextSize - 1)
        *out = 0;
}

/* RT3_FixLicenseName (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_FixLicenseName i386 0x000d486d ppc 0x000df65c
void RT3_FixLicenseName(char *out, const char *src)
{
    char *start;
    ULong n;
    ULong i;
    ULong digits;
    char c;

    digits = 0;
    for (i = 0; src[i] != 0 && digits < kTextSize - 1 && !(src[i] >= 'a' && src[i] <= 'z') && !(src[i] >= 'A' && src[i] <= 'Z'); i++) {
        if (src[i] >= '0' && src[i] <= '9')
            out[digits++] = src[i];
    }
    if (gRT3_CodeVersion == 0x8003 && src[i] == 0 && digits == 6) {
        start = out + digits;
    } else {
        start = out;
        while ((n = start - out) < kTextSize - 1 && (*start = *src++) != 0) {
            c = *start;
            if (c >= 'a' && c <= 'z') {
                *start += 'A' - 'a';
                start++;
            } else if (c >= 'A' && c <= 'Z') {
                start++;
            }
        }
    }
    n = start - out;
    if (n <= kTextSize - 1)
        *start = 0;
}

#define ROTATE64(v, n) do { ULongLong t = (v); (v) = (t << (n)) | (t >> (64 - (n))); } while (0)

// MAC: _RT3_CalcMainHash i386 0x000d4931 ppc 0x000df774
ULongLong RT3_CalcMainHash(const char *product, const char *name, int32_t copies)
{
    ULongLong h;
    ULong i, j;
    char c, d;

    h = 0;
    for (i = 0; (c = *product) != 0; product++, i++) {
        ROTATE64(h, 4);
        h ^= c;
        ROTATE64(h, 1);
        h ^= (i + 7) * copies;
    }
    for (j = 0; (d = *name) != 0; name++, j++) {
        ROTATE64(h, 3);
        h ^= d;
        ROTATE64(h, 1);
        h ^= (j + 13) * copies * 3;
    }
    if (!h)
        h = 1;
    h &= 0x0fffffffffffffffULL;
    return h;
}

// MAC: _RT3_LicenseCodeToText i386 0x000d4a65 ppc 0x000df88c
void RT3_LicenseCodeToText(ULongLong code, char *text)
{
    ULong i;
    ULong n;

    if (text != 0) {
        code &= 0x0fffffffffffffffULL;
        n = 4;
        i = 0;
        do {
            *text++ = gRT3_FiveBitToEightBitChars[(code >> (55 - i * 5)) & 0x1f];
            if (--n == 0) {
                text[0] = i < 11 ? '-' : 0;
                text++;
                n = 4;
            }
            i++;
        } while (i != 12);
    }
}

// MAC: _RT3_LicenseCodeEnswizzle i386 0x000d4b00 ppc 0x000df944
ULongLong RT3_LicenseCodeEnswizzle(unsigned short a, unsigned short b, unsigned short c, unsigned char flags)
{
    ULongLong v;
    ULong sum;
    ULong k;
    ULong parity;

    v = a >> 12 & 0xf;
    v <<= 1;
    v |= flags & 1;
    v <<= 4;
    v |= b >> 8 & 0xf;
    v <<= 1;
    v |= flags >> 1 & 1;
    v <<= 4;
    v |= c >> 4 & 0xf;
    v <<= 4;
    v |= a & 0xf;
    v <<= 1;
    v |= flags >> 2 & 1;
    v <<= 4;
    v |= b >> 12 & 0xf;
    v <<= 1;
    v |= flags >> 3 & 1;
    v <<= 4;
    v |= c >> 8 & 0xf;
    v <<= 4;
    v |= a >> 4 & 0xf;
    v <<= 1;
    v |= flags >> 4 & 1;
    v <<= 4;
    v |= b & 0xf;
    v <<= 1;
    v |= flags >> 5 & 1;
    v <<= 4;
    v |= c >> 12 & 0xf;
    v <<= 4;
    v |= a >> 8 & 0xf;
    v <<= 1;
    v |= flags >> 6 & 1;
    v <<= 4;
    v |= b >> 4 & 0xf;
    v <<= 1;
    v |= flags >> 7 & 1;
    v <<= 4;
    v |= c & 0xf;
    sum = 0;
    for (k = 0; k != 57; k += 3)
        sum += v >> k & 7;
    sum &= 7;
    v <<= 3;
    v |= sum;
    parity = 1;
    {
        ULong fold = (v >> 32) ^ v;

        while (fold != 0) {
            fold &= fold - 1;
            parity = 1 - parity;
        }
    }
    if (parity != 0)
        v |= 0x0800000000000000ULL;
    return v;
}

// MAC: _RT3_LicenseCodeDeswizzle i386 0x000d4f12 ppc 0x000dfc90
Boolean RT3_LicenseCodeDeswizzle(ULongLong code, unsigned short *a, unsigned short *b, unsigned short *c, unsigned char *flags)
{
    ULong sum;
    ULong calc;
    ULong k;
    Boolean valid = false;

    if (a != 0)
        *a = 0;
    if (b != 0)
        *b = 0;
    if (c != 0)
        *c = 0;
    if (flags != 0)
        *flags = 0;
    sum = code & 7;
    code &= 0x07ffffffffffffffULL;
    code >>= 3;
    calc = 0;
    for (k = 0; k != 57; k += 3)
        calc += code >> k & 7;
    if (sum == (calc & 7))
        valid = true;
    if (valid) {
        if (c != 0)
            *c |= code & 0xf;
        code >>= 4;
        if (flags != 0)
            *flags |= (code & 1) << 7;
        code >>= 1;
        if (b != 0)
            *b |= (code & 0xf) << 4;
        code >>= 4;
        if (flags != 0)
            *flags |= (code & 1) << 6;
        code >>= 1;
        if (a != 0)
            *a |= (code & 0xf) << 8;
        code >>= 4;
        if (c != 0)
            *c |= (code & 0xf) << 12;
        code >>= 4;
        if (flags != 0)
            *flags |= (code & 1) << 5;
        code >>= 1;
        if (b != 0)
            *b |= code & 0xf;
        code >>= 4;
        if (flags != 0)
            *flags |= (code & 1) << 4;
        code >>= 1;
        if (a != 0)
            *a |= (code & 0xf) << 4;
        code >>= 4;
        if (c != 0)
            *c |= (code & 0xf) << 8;
        code >>= 4;
        if (flags != 0)
            *flags |= (code & 1) << 3;
        code >>= 1;
        if (b != 0)
            *b |= (code & 0xf) << 12;
        code >>= 4;
        if (flags != 0)
            *flags |= (code & 1) << 2;
        code >>= 1;
        if (a != 0)
            *a |= code & 0xf;
        code >>= 4;
        if (c != 0)
            *c |= (code & 0xf) << 4;
        code >>= 4;
        if (flags != 0)
            *flags |= (code & 1) << 1;
        code >>= 1;
        if (b != 0)
            *b |= (code & 0xf) << 8;
        code >>= 4;
        if (flags != 0)
            *flags |= code & 1;
        code >>= 1;
        if (a != 0)
            *a |= (code & 0xf) << 12;
    } else {
        if (a != 0)
            *a = 0x79b7;
        if (b != 0)
            *b = 0x77b9;
        if (c != 0)
            *c = 0x7033;
        if (flags != 0)
            *flags = 0;
    }
    return valid;
}

/* RT3_CalcLicenseChecksum (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_CalcLicenseChecksum i386 0x000d5212 ppc 0x000e00e8
ULong RT3_CalcLicenseChecksum(ULongLong code)
{
    ULong sum;
    ULong k;

    code &= 0x07ffffffffffffffULL;
    code >>= 3;
    sum = 0;
    for (k = 0; k != 57; k += 3)
        sum += code >> k & 7;
    return sum & 7;
}

/* RT3_ExtractLicenseChecksum (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_ExtractLicenseChecksum i386 0x000d5268 ppc 0x000e0148
ULong RT3_ExtractLicenseChecksum(ULongLong code)
{
    return (ULong)code & 7;
}

/* RT3_ExtractLicenseTimestamp (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_ExtractLicenseTimestamp i386 0x000d5273 ppc 0x000e0150
ULong RT3_ExtractLicenseTimestamp(ULongLong code)
{
    ULong r = 0;

    code >>= 3;
    code >>= 4;
    r |= (code & 1) << 7;
    code >>= 1;
    code >>= 4;
    r |= (code & 1) << 6;
    code >>= 1;
    code >>= 4;
    code >>= 4;
    r |= (code & 1) << 5;
    code >>= 1;
    code >>= 4;
    r |= (code & 1) << 4;
    code >>= 1;
    code >>= 4;
    code >>= 4;
    r |= (code & 1) << 3;
    code >>= 1;
    code >>= 4;
    r |= (code & 1) << 2;
    code >>= 1;
    code >>= 4;
    code >>= 4;
    r |= (code & 1) << 1;
    code >>= 1;
    code >>= 4;
    r |= code & 1;
    return r;
}

/* RT3_ExtractLicenseBlock1 (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_ExtractLicenseBlock1 i386 0x000d5305 ppc 0x000e02bc
ULong RT3_ExtractLicenseBlock1(ULongLong code)
{
    ULong r = 0;

    code >>= 3;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 1;
    r |= (code & 0xf) << 8;
    code >>= 4;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 1;
    r |= (code & 0xf) << 4;
    code >>= 4;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 1;
    r |= code & 0xf;
    code >>= 4;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 1;
    r |= (code & 0xf) << 12;
    return r;
}

/* RT3_ExtractLicenseBlock2 (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_ExtractLicenseBlock2 i386 0x000d5361 ppc 0x000e0418
ULong RT3_ExtractLicenseBlock2(ULongLong code)
{
    ULong r = 0;

    code >>= 3;
    code >>= 4;
    code >>= 1;
    r |= (code & 0xf) << 4;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 4;
    code >>= 1;
    r |= code & 0xf;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 4;
    code >>= 1;
    r |= (code & 0xf) << 12;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 4;
    code >>= 1;
    r |= (code & 0xf) << 8;
    return r;
}

/* RT3_ExtractLicenseBlock3 (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_ExtractLicenseBlock3 i386 0x000d53bd ppc 0x000e0554
ULong RT3_ExtractLicenseBlock3(ULongLong code)
{
    ULong r = 0;

    code >>= 3;
    r |= code & 0xf;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    r |= (code & 0xf) << 12;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    r |= (code & 0xf) << 8;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    code >>= 1;
    code >>= 4;
    r |= (code & 0xf) << 4;
    return r;
}

/* RT3_GetTimePeriod (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_GetTimePeriod i386 0x000d5412 ppc 0x000e0670
unsigned char RT3_GetTimePeriod(void)
{
    return RT3_CalcTimePeriod(TimerGetUTCSeconds());
}

/* RT3_LicenseTextToCode (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RT3_LicenseTextToCode i386 0x000d543e ppc 0x000e06c0
void RT3_LicenseTextToCode(const char *text, ULongLong *code)
{
    ULongLong value;
    ULong digits;
    unsigned char d;

    value = 0;
    RT3_InitTables();
    if (text != 0 && code != 0) {
        for (digits = 0; digits < kCodeDigits && *text != 0; text++) {
            d = gRT3_EightBitToFiveBitChars[*(const unsigned char *)text];
            if (d != kNoDigit) {
                value = value << kCodeBits | d;
                digits++;
            }
        }
    }
    *code = value;
}
