// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: the private runtime's line, the one jpegcrt.c is built with; only the data is kept, so no instruction decides it; Research/notes/crt4.md
// LINK: after itattr; data EVNOVA 0x00581c10
/* crt4.c -- a fourth copy of the private C runtime pngcrt.c, zlibcrt.c and jpegcrt.c carry (heap, byte copies, strings, class table, strtod, abs); not linked. Notes: Research/notes/crt4.md */

#include <stddef.h>
#include "win32.h"
#include "privcrt.h"

void *crt4_malloc(size_t size)
{
    return size != 0 ? HeapAlloc(GetProcessHeap(), 0, size) : NULL;
}

void crt4_free(void *ptr)
{
    if (ptr != NULL)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void *crt4_memcpy(void *dest, const void *source, size_t len)
{
    unsigned char *d = dest;
    const unsigned char *s = source;

    while (len--)
        *d++ = *s++;
    return dest;
}

size_t crt4_strlen(const char *s)
{
    size_t n = 0;

    while (*s++ != '\0')
        n++;
    return n;
}

char *crt4_strcpy(char *dest, const char *source)
{
    char *save = dest;

    while ((*dest++ = *source++) != '\0')
        ;
    return save;
}

int crt4_memcmp(const void *s1, const void *s2, size_t len)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    int c1, c2;

    for (len++; --len;)
        if ((c1 = *p1++) != (c2 = *p2++))
            return c1 - c2;
    return 0;
}

void *crt4_memset(void *dest, int c, size_t len)
{
    unsigned char *d = dest;

    while (len--)
        *d++ = (unsigned char)c;
    return dest;
}

static const int crt4_ctype[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, CT_SPACE, CT_SPACE, CT_SPACE, CT_SPACE, CT_SPACE, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    CT_SPACE, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    CT_DIGIT, CT_DIGIT, CT_DIGIT, CT_DIGIT, CT_DIGIT, CT_DIGIT, CT_DIGIT, CT_DIGIT,
    CT_DIGIT, CT_DIGIT, 0, 0, 0, 0, 0, 0,
    0, CT_UXDIG, CT_UXDIG, CT_UXDIG, CT_UXDIG, CT_UXDIG, CT_UXDIG, CT_UPPER,
    CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER,
    CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER, CT_UPPER,
    0, 0, 0, 0, 0, 0, CT_LXDIG, CT_LXDIG,
    CT_LXDIG, CT_LXDIG, CT_LXDIG, CT_LXDIG, CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER,
    CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER,
    CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, 0, 0, 0
};

static int crt4_isspace(int c) { return (crt4_ctype[c] & CT_SPACE) ? 1 : 0; }
static int crt4_isdigit(int c) { return (crt4_ctype[c] & CT_DIGIT) ? 1 : 0; }

double crt4_strtod(const char *s, char **endptr)
{
    double result = 0.0;
    double scale = 0.0;
    int negative;
    int value;
    int digits;

    while (crt4_isspace(*s))
        s++;
    negative = 0;
    if (*s == '+')
        s++;
    else if (*s == '-') {
        s++;
        negative = 1;
    }

    value = digits = 0;
    scale = 1.0;
    while (crt4_isdigit(*s)) {
        value = value * 10 + (*s - '0');
        if (value != 0 || digits != 0)
            digits++;
        s++;
    }
    result = value;

    if (*s == '.') {
        s++;
        value = digits = 0;
        scale = 1.0;
        while (crt4_isdigit(*s)) {
            value = value * 10 + (*s - '0');
            if (value != 0 || digits != 0)
                digits++;
            s++;
        }
        while (digits--)
            scale *= 10.0;
        result += value / scale;
    }
    if (negative)
        result = -result;

    if (*s == 'e' || *s == 'E') {
        s++;
        negative = 0;
        if (*s == '+')
            s++;
        else if (*s == '-') {
            s++;
            negative = 1;
        }
        value = digits = 0;
        scale = 1.0;
        while (crt4_isdigit(*s)) {
            value = value * 10 + (*s - '0');
            if (value != 0 || digits != 0)
                digits++;
            s++;
        }
        while (value--)
            scale *= 10.0;
        if (negative)
            result /= scale;
        else
            result *= scale;
    }

    *endptr = (char *)s;
    return result;
}

int crt4_abs(int x)
{
    return x < 0 ? -x : x;
}
