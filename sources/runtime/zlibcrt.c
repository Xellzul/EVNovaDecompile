// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: zlib 1.2.1 on libjpeg's line and compiler (-align 8 is object-inert on all ten zlib TUs, Research/flag-census.md); build 335 has no -proc switch, its default is generic x86; Research/runtime-compiler-ceiling.md
/* zlibcrt.c -- the four C-runtime calls the exe's zlib 1.2.1 build links privately (heap and byte copies, not MSL's); pngcrt.c's file, its unlinked half kept unmarked for the class table the exe keeps. Notes: Research/notes/zdeflate.md */

#include "zutil.h"
#include "win32.h"
#include "privcrt.h"

/* zlib_malloc. STATE: MATCH 100%, exe 11 / obj 11, byte-identical. OPEN: the original's name
   is unknown; zlib's zcalloc is the only caller. */
// FUNCTION: EVNOVA 0x0054325b
void *zlib_malloc(size_t size)
{
    return size != 0 ? HeapAlloc(GetProcessHeap(), 0, size) : NULL;
}

/* zlib_free. STATE: MATCH 100%, exe 9 / obj 9, byte-identical. OPEN: the original's name is
   unknown; zlib's zcfree is the only caller. */
// FUNCTION: EVNOVA 0x00543242
void zlib_free(void *ptr)
{
    if (ptr != NULL)
        HeapFree(GetProcessHeap(), 0, ptr);
}

/* zlib_memcpy (zlib's zmemcpy). STATE: MATCH 100%, exe 17 / obj 17, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00543220
void *zlib_memcpy(void *dest, const void *source, size_t len)
{
    unsigned char *d = dest;
    const unsigned char *s = source;

    while (len--)
        *d++ = *s++;
    return dest;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
size_t zlib_strlen(const char *s)
{
    size_t n = 0;

    while (*s++ != '\0')
        n++;
    return n;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
char *zlib_strcpy(char *dest, const char *source)
{
    char *save = dest;

    while ((*dest++ = *source++) != '\0')
        ;
    return save;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
int zlib_memcmp(const void *s1, const void *s2, size_t len)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    int c1, c2;

    for (len++; --len;)
        if ((c1 = *p1++) != (c2 = *p2++))
            return c1 - c2;
    return 0;
}

/* zlib_memset (zlib's zmemzero passes 0). STATE: MATCH 100%, exe 13 / obj 13, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00543203
void *zlib_memset(void *dest, int c, size_t len)
{
    unsigned char *d = dest;

    while (len--)
        *d++ = (unsigned char)c;
    return dest;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
static const int zlib_ctype[256] = {
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

static int zlib_isspace(int c) { return (zlib_ctype[c] & CT_SPACE) ? 1 : 0; }
static int zlib_isdigit(int c) { return (zlib_ctype[c] & CT_DIGIT) ? 1 : 0; }

/* not linked: dead-stripped in the exe; kept for the TU's data */
double zlib_strtod(const char *s, char **endptr)
{
    double result = 0.0;
    double scale = 0.0;
    int negative;
    int value;
    int digits;

    while (zlib_isspace(*s))
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
    while (zlib_isdigit(*s)) {
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
        while (zlib_isdigit(*s)) {
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
        while (zlib_isdigit(*s)) {
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

/* not linked: dead-stripped in the exe; kept for the TU's data */
int zlib_abs(int x)
{
    return x < 0 ? -x : x;
}
