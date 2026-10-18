// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atplatstr.c -- the platform layer's own string helpers: a strlen, a strcpy, a three-way strcmp, an upper-casing strncpy, a strpbrk, a bounded sprintf, and the formatter behind it with its two helpers (the number-to-digits writer and the padded field copy). Notes: Research/notes/atplatstr.md */

#include <stdarg.h>

#include "burgertypes.h"
#include "burger.h"

#define kFoldDelta    0x20

static unsigned char gStringUpperFold[256];          /* 0x0086643a upper fold */
static unsigned char gStringLowerFold[256];          /* 0x0086633a lower fold */
static unsigned char gStringLowerReady;               /* 0x00866339 lower table built */
static unsigned char gStringUpperReady;               /* 0x00866338 table built */

/* The engine already binds 0x004eb76d as `StringGetLength'; defined under that name. */
/* StringGetLength. STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb76d
unsigned long StringGetLength(const unsigned char *s)
{
    ULong n;
    long err;

    err = 0;
    n = -1;
    do {
        n++;
    } while (*s++ != 0);
    return n;
}

/* StringDuplicate (strdup). STATE: MATCH 100%, exe 31 / obj 31, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004eb784
char *StringDuplicate(const char *s)
{
    char *copy;
    char *start;
    long err;

    err = 0;
    copy = 0;
    copy = MemoryAllocate(StringGetLength((const unsigned char *)s) + 1);
    if (copy == 0) {
        err = kOutOfMemory;
    } else {
        start = copy;
        while ((*copy++ = *s++) != 0)
            ;
        copy = start;
    }
    return copy;
}

/* StringCopy. STATE: MATCH 100%, exe 19 / obj 19, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb7c2
char *StringCopy(char *dst, const char *src)
{
    char *start;
    long err;

    err = 0;
    start = dst;
    while ((*dst++ = *src++) != 0)
        ;
    dst = start;
    return dst;
}

/* StringCopySafe. STATE: MATCH 100%, exe 29 / obj 29, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb7e6
char *StringCopySafe(char *dst, const char *src, long size)
{
    char *start;
    long err;

    err = 0;
    start = dst;
    while (--size != 0 && (*dst++ = *src++) != 0)
        ;
    if (size == 0)
        *dst = 0;
    dst = start;
    return dst;
}

/* Appends src to the string in dst, dst holding n bytes in all. The engine
 * binds 0x004eb81d as `StringAppendSafe'. */
/* StringAppendSafe. STATE: MATCH 100%, exe 39 / obj 39, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb81d
unsigned char *StringAppendSafe(unsigned char *dst, const unsigned char *src, unsigned long n)
{
    unsigned char *start;
    long err;

    err = 0;
    start = dst;
    while (*dst++ != 0 && --n != 0)
        ;
    if (n != 0) {
        dst = dst - 1;
        while (--n != 0 && (*dst++ = *src++) != 0)
            ;
        if (n == 0)
            *dst = 0;
    }
    dst = start;
    return dst;
}

/* StringCompare. STATE: MATCH 100%, exe 26 / obj 26, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb866
long StringCompare(const char *a, const char *b)
{
    ULong ca;
    ULong cb;
    const unsigned char *s1;
    const unsigned char *s2;
    long err;

    err = 0;
    s1 = (const unsigned char *)a;
    s2 = (const unsigned char *)b;
    while ((ca = *s1++) == (cb = *s2++)) {
        if (ca == 0)
            return 0;
    }
    return ca < cb ? (signed char)-1 : (signed char)1;
}

/* StringCompareSafe (a strncmp answering -1, 0 or 1). STATE: MATCH 100%, exe 33 / obj 33,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb89c
long StringCompareSafe(const char *a, const char *b, ULong n)
{
    const unsigned char *s1;
    const unsigned char *s2;
    ULong ca;
    ULong cb;
    long err;

    err = 0;
    s1 = (const unsigned char *)a;
    s2 = (const unsigned char *)b;
    while (n-- && (ca = *s1++) == (cb = *s2++)) {
        if (ca == 0)
            return 0;
    }
    if (n == (ULong)-1)
        return 0;
    return ca < cb ? (signed char)-1 : (signed char)1;
}

char *gStringBlank = "";                         /* 0x005771bc the exhausted-token mark */

/* StringTokenize (a strtok on one delimiter, its state in *next). STATE: MATCH 100%, exe 36 /
   obj 36, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb8e2
char *StringTokenize(char *s, int delim, char **next)
{
    char c;
    char d;
    char *p;
    long err;

    err = 0;
    d = delim;
    p = *next;
    if (p != 0)
        s = p;
    *next = gStringBlank;
    if (*s == 0)
        return 0;
    p = s;
    while ((c = *p++) != 0 && c != d)
        ;
    if (c != 0) {
        p[-1] = 0;
        *next = p;
    }
    return s;
}

/* StringFindString (a strstr; an empty `sub' never matches). STATE: MATCH 100%, exe 33 /
   obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb92e
char *StringFindString(const char *s, const char *sub)
{
    long err;
    char first;
    ULong i;
    char c;

    err = 0;
    first = sub[0];
    i = 0;
    while ((c = *s) != 0) {
        if (c == first) {
            i = 1;
            while (sub[i] != 0 && s[i] == sub[i])
                i++;
            if (sub[i] == 0)
                return (char *)s;
        }
        s++;
    }
    return 0;
}

/* StringFindDelimiter. STATE: MATCH 100%, exe 20 / obj 20, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb96d
char *StringFindDelimiter(const char *s, const char *set)
{
    long err;
    const char *p;

    err = 0;
    while (*s != 0) {
        for (p = set; *p != 0; p = p + 1) {
            if (*s == *p)
                return (char *)s;
        }
        s = s + 1;
    }
    return 0;
}

#define kTopDecimal 1000000000UL

/* StringFromNumber. STATE: MATCH 100%, exe 46 / obj 46, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb994
char *StringFromNumber(char *out, ULong value)
{
    char *start;
    ULong place;
    unsigned char started;
    long err;

    err = 0;
    start = out;
    place = kTopDecimal;
    started = 0;
    while (place != 0) {
        if (started != 0 || value / place != 0 || place == 1) {
            *out++ = value / place + '0';
            value = value % place;
            started = 1;
        }
        place = place / 10;
    }
    *out = 0;
    return start;
}

/* StringToNumber (the leading decimal digits of a string). STATE: MATCH 100%, exe 20 / obj 20, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004eb9f2
ULong StringToNumber(const char *s)
{
    ULong value;
    long err;

    err = 0;
    value = 0;
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    return value;
}

/* String2UpperCopySafe. STATE: MATCH 100%, exe 52 / obj 52, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eba1b
char *String2UpperCopySafe(char *dst, const char *src, ULong n)
{
    char *start;
    ULong i;
    ULong c;
    int lower;
    long err;

    err = 0;
    if (gStringUpperReady == 0) {
        for (i = 0; i < 256; i = i + 1) {
            lower = 0;
            if (i >= kFoldLow && i <= kFoldHigh)
                lower = 1;
            if (lower != 0)
                c = i - kFoldDelta;
            else
                c = i;
            gStringUpperFold[i] = c;
        }
        gStringUpperReady = 1;
    }
    start = dst;
    while (--n != 0 && (*dst++ = gStringUpperFold[(unsigned char)*src++]) != 0)
        ;
    if (n == 0)
        *dst = 0;
    dst = start;
    return dst;
}

/* String2LowerCopySafe (the lower-casing strncpy, String2UpperCopySafe's twin). STATE: MATCH 100%, exe 52
   / obj 52, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eba96
char *String2LowerCopySafe(char *dst, const char *src, ULong n)
{
    char *start;
    ULong i;
    ULong c;
    int upper;
    long err;

    err = 0;
    if (gStringLowerReady == 0) {
        for (i = 0; i < 256; i = i + 1) {
            upper = 0;
            if (i >= 'A' && i <= 'Z')
                upper = 1;
            if (upper != 0)
                c = i + kFoldDelta;
            else
                c = i;
            gStringLowerFold[i] = (unsigned char)c;
        }
        gStringLowerReady = 1;
    }
    start = dst;
    while (--n != 0 && (*dst++ = gStringLowerFold[(unsigned char)*src++]) != 0)
        ;
    if (n == 0)
        *dst = 0;
    dst = start;
    return dst;
}

/* String2Upper (upper-case a C string in place). STATE: MATCH 100%, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004ebb11
char *String2Upper(char *s)
{
    return String2UpperCopySafe(s, s, kNoLength);
}

/* String2Lower (lower-case a C string in place). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004ebb22
char *String2Lower(char *s)
{
    return String2LowerCopySafe(s, s, kNoLength);
}

/* StringFormat (the unbounded sprintf). STATE: MATCH 100%, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004ebb33
char *StringFormat(char *out, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    return StringFormatVarg(out, kNoLength, fmt, args);
}

/* StringFormatSafe. STATE: MATCH 100%, exe 17 / obj 17 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ebb62
char *StringFormatSafe(char *out, long size, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    return StringFormatVarg(out, size, fmt, args);
}

typedef long long LongLong;


#define kFlagZero       0x01                     /* '0' */
#define kFlagLeft       0x02                     /* '-' */
#define kFlagPlus       0x04                     /* '+' */
#define kFlagSpace      0x08                     /* ' ' */
#define kFlagPascal     0x10                     /* '#' */
#define kNoWidth        0xffffffff
#define kScratchSize    256
#define kPointerDigits  8

void _StringFormatNumber(char *buf, ULongLong value, ULong base, const char *digits,
                  ULong flags, ULong width, ULong precision);
char *_StringFormatString(char *out, const char *src, ULong *size, ULong flags,
                   ULong width, ULong precision);

static char kHexUpper[] = "0123456789ABCDEF";

static char *kMonthAbbrev[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char *kMonthShort[12] = {
    "Jan", "Feb", "March", "April", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"
};

static char *kMonthName[12] = {
    "January", "February", "March", "April", "May", "June", "July", "August",
    "September", "October", "November", "December"
};

static char *kOrdinal[10] = {
    "th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"
};

static char *kMeridiem[2] = { " AM", " PM" };

static char kTimeLongFmt[] = "%.*ld:%.2ld:%.2ld%s";
static char kTimeShortFmt[] = "%.*ld:%.2ld%s";
static char kDateShortFmt[] = "%ld/%ld/%.2ld";
static char kDateLongFmt[] = "%ld/%ld/%.4ld";
static char kDateDayFirstFmt[] = "%ld%s %s %.4ld";
static char kDateMonthFirstFmt[] = "%s %ld%s, %.4ld";
static char kSpace[] = " ";

/* StringFormatVarg. STATE: MATCH 100%, exe 533 / obj 533, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ebb93
char *StringFormatVarg(char *out, ULong size, const char *format, va_list args)
{
    const unsigned char *fmt;
    long lengthMod;
    ULong precision;
    ULong width;
    char *outPtr;
    ULong flags;
    unsigned char conv;
    char buf[kScratchSize];
    LongLong sval;
    ULongLong uval;
    long i;
    unsigned char c;
    unsigned char m;
    const char *s;
    const unsigned char *p;
    void *ptr;
    ULong k;
    long err;

    err = 0;
    outPtr = out;
    fmt = (const unsigned char *)format;
    size = size - 1;
    while (size != 0 && (c = *fmt) != 0) {
        if (c != '%') {
            *outPtr = c;
            fmt = fmt + 1;
            outPtr = outPtr + 1;
            size = size - 1;
            continue;
        }
        if (fmt[1] == '%') {
            *outPtr = '%';
            outPtr = outPtr + 1;
            fmt = fmt + 2;
            size = size - 1;
            continue;
        }

        MemoryClear(buf, kScratchSize);
        flags = lengthMod = 0;
        width = precision = kNoWidth;
        i = 1;
        do {
            k = i;
            switch (fmt[k]) {
            case '0':
                flags = flags | kFlagZero;
                i = i + 1;
                break;
            case '-':
                flags = flags | kFlagLeft;
                i = i + 1;
                break;
            case '+':
                flags = flags | kFlagPlus;
                i = i + 1;
                break;
            case ' ':
                flags = flags | kFlagSpace;
                i = i + 1;
                break;
            case '#':
                flags = flags | kFlagPascal;
                i = i + 1;
                break;
            }
        } while (k != i);

        if (fmt[i] == '*') {
            width = va_arg(args, ULong);
            i = i + 1;
        } else if (fmt[i] >= '0' && fmt[i] <= '9') {
            width = 0;
            while (fmt[i] >= '0' && fmt[i] <= '9') {
                width = width * 10 + (fmt[i] - '0');
                i = i + 1;
            }
        }
        if (fmt[i] == '.') {
            i = i + 1;
            if (fmt[i] == '*') {
                precision = va_arg(args, ULong);
                i = i + 1;
            } else if (fmt[i] >= '0' && fmt[i] <= '9') {
                precision = 0;
                while (fmt[i] >= '0' && fmt[i] <= '9') {
                    precision = precision * 10 + (fmt[i] - '0');
                    i = i + 1;
                }
            } else {
                precision = 0;
            }
        }

        m = fmt[i];
        switch (m) {
        case 'h':
            lengthMod = m;
            i = i + 1;
            break;
        case 'q':
            lengthMod = m;
            i = i + 1;
            break;
        case 'L':
            lengthMod = 'q';
            i = i + 1;
            break;
        case 'l':
            if (fmt[i + 1] == 'l') {
                lengthMod = 'q';
                i = i + 2;
            } else {
                lengthMod = m;
                i = i + 1;
            }
            break;
        }

        conv = fmt[i];
        switch (conv) {
        case 'c':
            *outPtr = va_arg(args, int);
            outPtr = outPtr + 1;
            size = size - 1;
            i = i + 1;
            fmt = fmt + i;
            continue;

        case 'd':
        case 'i':
            if (lengthMod == 'h')
                sval = va_arg(args, int);
            else if (lengthMod == 'l')
                sval = va_arg(args, int);
            else if (lengthMod == 'q')
                sval = va_arg(args, LongLong);
            else
                sval = va_arg(args, int);
            if (sval < 0 ? 1 : 0) {
                buf[0] = '-';
                sval = -sval;
            } else if ((sval ? 1 : 0) && (flags & kFlagPlus) != 0) {
                buf[0] = '+';
            }
            uval = sval;
            _StringFormatNumber(buf, uval, 10, "0123456789", flags, width,
                         precision == kNoWidth ? 1 : precision);
            outPtr = _StringFormatString(outPtr, buf, &size, flags, kNoWidth, kNoWidth);
            fmt = fmt + (i + 1);
            continue;

        case 'p':
            outPtr = _StringFormatString(outPtr, "0x", &size, kFlagLeft, 2, kNoWidth);
            precision = width = kPointerDigits;
            ptr = va_arg(args, void *);
            uval = (ULong)ptr;
            _StringFormatNumber(buf, uval, 16, kHexUpper, flags, kPointerDigits,
                         precision == kNoWidth ? 1 : precision);
            outPtr = _StringFormatString(outPtr, buf, &size, flags, kNoWidth, kNoWidth);
            fmt = fmt + (i + 1);
            continue;

        case 'u':
        case 'o':
        case 'x':
        case 'X':
            if (lengthMod == 'h')
                uval = va_arg(args, ULong);
            else if (lengthMod == 'l')
                uval = va_arg(args, ULong);
            else if (lengthMod == 'q')
                uval = va_arg(args, ULongLong);
            else
                uval = va_arg(args, ULong);
            if (conv == 'u') {
                if ((uval ? 1 : 0) && (flags & kFlagPlus) != 0)
                    buf[0] = '+';
                _StringFormatNumber(buf, uval, 10, "0123456789", flags, width,
                             precision == kNoWidth ? 1 : precision);
            } else if (conv == 'o') {
                _StringFormatNumber(buf, uval, 8, "01234567", flags, width,
                             precision == kNoWidth ? 1 : precision);
            } else {
                _StringFormatNumber(buf, uval, 16,
                             conv == 'x' ? "0123456789abcdef" : kHexUpper,
                             flags, width, precision == kNoWidth ? 1 : precision);
            }
            outPtr = _StringFormatString(outPtr, buf, &size, flags, kNoWidth, kNoWidth);
            fmt = fmt + (i + 1);
            continue;

        case 's':
            if ((flags & kFlagPascal) != 0) {
                p = va_arg(args, const unsigned char *);
                if (p == 0)
                    p = (const unsigned char *)"\p<NULL>";
                for (k = 0; k < p[0]; k = k + 1)
                    buf[k] = p[k + 1];
                buf[k] = 0;
                outPtr = _StringFormatString(outPtr, buf, &size, flags, width, precision);
            } else {
                s = va_arg(args, const char *);
                if (s == 0)
                    s = "<NULL>";
                outPtr = _StringFormatString(outPtr, s, &size, flags, width, precision);
            }
            fmt = fmt + (i + 1);
            continue;

        case 'n':
            *va_arg(args, long *) = outPtr - out;
            fmt = fmt + (i + 1);
            continue;

        default:
            for (;;) {
                *outPtr = *fmt;
                fmt = fmt + 1;
                outPtr = outPtr + 1;
                size = size - 1;
                i = i - 1;
                if (size == 0)
                    break;
                if (i + 1 == 0)
                    break;
                if (*fmt == 0)
                    break;
            }
            continue;
        }
    }
    *outPtr = 0;
    return out;
}

/* _StringFormatNumber. STATE: MATCH 100%, exe 112 / obj 112, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ec234
void _StringFormatNumber(char *buf, ULongLong value, ULong base, const char *digits,
                  ULong flags, ULong width, ULong precision)
{
    ULongLong v;
    ULong n;
    long err;

    err = 0;
    StringGetLength((const unsigned char *)digits);
    while (*buf != 0)
        buf++;
    n = 1;
    v = value;
    while ((LongLong)(v - base) >= 0) {
        n++;
        v /= base;
    }
    if (n < precision)
        n = precision;
    if (width == kNoWidth)
        width = n;
    else if (n > width)
        width = n;
    if (precision == 0 && (value ? 1 : 0) == 0 && (flags & kFlagSpace) != 0) {
        buf[0] = ' ';
        buf[1] = 0;
        return;
    }
    buf[n] = 0;
    while (n--) {
        v = value % base;
        buf[--width] = digits[(ULong)v];
        value /= base;
    }
    while (width != 0)
        buf[--width] = (flags & kFlagZero) != 0 ? '0' : ' ';
}

/* _StringFormatString. STATE: MATCH 100%, exe 87 / obj 87, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ec35b
char *_StringFormatString(char *out, const char *src, ULong *size, ULong flags,
                   ULong width, ULong precision)
{
    if (width == kNoWidth)
        width = StringGetLength((const unsigned char *)src);
    if (width > *size)
        width = *size;
    if (precision > width)
        precision = width;
    if ((flags & kFlagLeft) != 0) {
        while (precision && *src) {
            precision--;
            *out++ = *src++;
            (*size)--;
            width -= width ? 1 : 0;
        }
        if (precision > width) {
            while (precision--) {
                *out++ = ' ';
                (*size)--;
            }
        } else {
            while (width--) {
                *out++ = ' ';
                (*size)--;
            }
        }
    } else {
        flags = precision;
        precision = 0;
        while (precision < flags && src[precision])
            precision++;
        while (width-- > precision) {
            *out++ = ' ';
            (*size)--;
        }
        while (precision--) {
            *out++ = *src++;
            (*size)--;
        }
    }
    return out;
}
