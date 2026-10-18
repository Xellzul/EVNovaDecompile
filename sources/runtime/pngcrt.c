// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libpng 1.2.7's one line, compiler and headers (zlib's compiler, built with debug information on, against the 2.4-era MSL). -g moves jump threading and register choice; the 2.4-era math_x87.h makes fabs an asm inline; build 335 has no -proc generic, its default is generic x86; Research/runtime-compiler-ceiling.md
/* pngcrt.c -- the C-runtime calls the exe's libpng 1.2.7 build links privately (heap, byte copies, strings, abs; not MSL's). Notes: Research/notes/png.md */

#include "png.h"
#include "win32.h"
#include "privcrt.h"

/* pngcrt_malloc (pngmem.c's malloc). STATE: MATCH, byte-identical (29 B).
   OPEN: the original's name is unknown. */
// FUNCTION: EVNOVA 0x0054f7d2
void *pngcrt_malloc(size_t size)
{
    return size != 0 ? HeapAlloc(GetProcessHeap(), 0, size) : NULL;
}

/* pngcrt_free (pngmem.c's free). STATE: MATCH, byte-identical (25 B).
   OPEN: the original's name is unknown. */
// FUNCTION: EVNOVA 0x0054f7b9
void pngcrt_free(void *ptr)
{
    if (ptr != NULL)
        HeapFree(GetProcessHeap(), 0, ptr);
}

/* pngcrt_memcpy (png_memcpy). STATE: MATCH, byte-identical (34 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f797
void *pngcrt_memcpy(void *dest, const void *source, size_t len)
{
    unsigned char *d = dest;
    const unsigned char *s = source;

    while (len--)
        *d++ = *s++;
    return dest;
}

/* pngcrt_strlen (png_strlen). STATE: MATCH, byte-identical (18 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f785
size_t pngcrt_strlen(const char *s)
{
    size_t n = 0;

    while (*s++ != '\0')
        n++;
    return n;
}

/* pngcrt_strcpy (png_strcpy). STATE: MATCH, byte-identical (30 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f767
char *pngcrt_strcpy(char *dest, const char *source)
{
    char *save = dest;

    while ((*dest++ = *source++) != '\0')
        ;
    return save;
}

/* pngcrt_memcmp (png_memcmp). STATE: MATCH, byte-identical (47 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f738
int pngcrt_memcmp(const void *s1, const void *s2, size_t len)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    int c1, c2;

    for (len++; --len;)
        if ((c1 = *p1++) != (c2 = *p2++))
            return c1 - c2;
    return 0;
}

/* pngcrt_memset (png_memset). STATE: MATCH, byte-identical (29 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f71b
void *pngcrt_memset(void *dest, int c, size_t len)
{
    unsigned char *d = dest;

    while (len--)
        *d++ = (unsigned char)c;
    return dest;
}

/* BUG: the letter rows are short: X-Z are not upper case, and the lower-case run starts at '^'
   (hex digits '^'..'c', letters 'd'..'t') */
static const int pngcrt_ctype[256] = {
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

/* BUG: callers pass a signed char, so bytes 0x80..0xff read the data before the table */
static int pngcrt_isspace(int c) { return (pngcrt_ctype[c] & CT_SPACE) ? 1 : 0; }
static int pngcrt_isdigit(int c) { return (pngcrt_ctype[c] & CT_DIGIT) ? 1 : 0; }

/* pngcrt_strtod (png_handle_sCAL's strtod). STATE: MATCH, byte-identical (389 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f596
double pngcrt_strtod(const char *s, char **endptr)
{
    double result = 0.0;
    double scale = 0.0;
    int negative;
    int value;
    int digits;

    while (pngcrt_isspace(*s))
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
    while (pngcrt_isdigit(*s)) {
        value = value * 10 + (*s - '0');
        if (value != 0 || digits != 0)
            digits++;
        s++;
    }
    result = value;

    /* BUG: leading zeros are not counted, so "0.05" reads 0.5 */
    if (*s == '.') {
        s++;
        value = digits = 0;
        scale = 1.0;
        while (pngcrt_isdigit(*s)) {
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
        while (pngcrt_isdigit(*s)) {
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

/* pngcrt_abs (png_set_dither's PNG_COLOR_DIST). STATE: MATCH, byte-identical (11 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f58b
int pngcrt_abs(int x)
{
    return x < 0 ? -x : x;
}
