/* rt3commonmac.c -- 1.0.10: RT3's code words and hashes, win's rt3.c text. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"

#define kTextSize   0x100
#define kCodeDigits 12
#define kCodeBits   5
#define kNoDigit    0xff
#define kNibble     0xf

static unsigned char gRT3_EightBitToFiveBitChars[256];
static unsigned char gInited;

static char kCodeDigitSet[32] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

static void RT3_InitTables(void)
{
    const char *p;
    unsigned char *t;
    ULong c;
    ULong i;

    if (gInited == 0) {
        for (c = 0, t = gRT3_EightBitToFiveBitChars; c < 0x100; c++, t++) {
            *t = kNoDigit;
            p = kCodeDigitSet;
            for (i = 0; i < 0x20; p++, i++) {
                if (c == *p)
                    *t = i;
            }
        }
        for (c = 'a'; c <= 'z'; c++)
            gRT3_EightBitToFiveBitChars[c] = gRT3_EightBitToFiveBitChars[c - ('a' - 'A')];
        gInited = 1;
    }
}

/* Copies a name as upper-case letters only, at most 255 of them. */
void RT3_FixLicenseName(char *out, const char *src)
{
    ULong n;
    char *start;
    char c;

    start = out;
    while ((n = out - start) < kTextSize - 1 && (*out = *src++) != 0) {
        c = *out;
        if (c >= 'a' && c <= 'z') {
            *out += 'A' - 'a';
            out++;
        } else if (c >= 'A' && c <= 'Z') {
            out++;
        }
    }
    if (n == kTextSize - 1)
        *out = 0;
}

ULongLong RT3_CalcMainHash(const char *product, const char *name, int32_t copies)
{
    ULongLong h;
    ULong i;

    h = 0;
    for (i = 0; *product != 0; product++, i++) {
        h = h >> 60 | h << 4;
        h ^= *product;
        h = h >> 63 | h << 1;
        h ^= (copies * (i + 7));
    }
    for (i = 0; *name != 0; name++, i++) {
        h = h >> 61 | h << 3;
        h ^= *name;
        h = h >> 63 | h << 1;
        h ^= (copies * (i + 13) * 3);
    }
    if (!h)
        h = 1;
    return h & 0x0fffffffffffffffULL;
}

void RT3_LicenseTextToCode(const char *text, ULongLong *code)
{
    ULongLong value;
    unsigned char d;
    ULong digits;

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

/* DRHash64's twin: four nibbles, bits 13, 27, 41 and 55 the low ends, in another order. */
int32_t RT3_ExtractLicenseBlock1(ULongLong v)
{
    ULong r;

    r = 0;
    r |= ((ULong)(v >> 13) & kNibble) << 8;
    r |= ((ULong)(v >> 27) & kNibble) << 4;
    r |= (ULong)(v >> 41) & kNibble;
    r |= ((ULong)(v >> 55) & kNibble) << 12;
    return r;
}

/* Four nibbles of a license code, as a 16-bit check value: win's DRHash64. */
ULong RT3_ExtractLicenseBlock2(ULongLong lic)
{
    ULong r;

    r = 0;
    r |= ((ULong)(lic >> 8) & kNibble) << 4;
    r |= (ULong)(lic >> 22) & kNibble;
    r |= ((ULong)(lic >> 36) & kNibble) << 12;
    r |= ((ULong)(lic >> 50) & kNibble) << 8;
    return r;
}
