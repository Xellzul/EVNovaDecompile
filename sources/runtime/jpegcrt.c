// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jpegcrt.c -- the C-runtime calls the exe's libjpeg 6b build links privately (MEMZERO, MEMCOPY, jmemnobs.c's malloc and free; not MSL's); pngcrt.c's file, its unlinked half kept unmarked for the class table the exe keeps. Notes: Research/notes/jpeg.md */

#include <stddef.h>
#include "win32.h"
#include "privcrt.h"

/* jpegcrt_malloc (jpeg_get_small, jpeg_get_large). STATE: MATCH, byte-identical (29 B).
   OPEN: the original's name is unknown. */
// FUNCTION: EVNOVA 0x0054dd9c
void *jpegcrt_malloc(size_t size)
{
    return size != 0 ? HeapAlloc(GetProcessHeap(), 0, size) : NULL;
}

/* jpegcrt_free (jpeg_free_small, jpeg_free_large). STATE: MATCH, byte-identical (25 B).
   OPEN: the original's name is unknown. */
// FUNCTION: EVNOVA 0x0054dd83
void jpegcrt_free(void *ptr)
{
    if (ptr != NULL)
        HeapFree(GetProcessHeap(), 0, ptr);
}

/* jpegcrt_memcpy (MEMCOPY). STATE: MATCH, byte-identical (34 B); the body is
   pngcrt_memcpy's. OPEN: the original's name is unknown. */
// FUNCTION: EVNOVA 0x0054dd61
void *jpegcrt_memcpy(void *dest, const void *source, size_t len)
{
    unsigned char *d = dest;
    const unsigned char *s = source;

    while (len--)
        *d++ = *s++;
    return dest;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
size_t jpegcrt_strlen(const char *s)
{
    size_t n = 0;

    while (*s++ != '\0')
        n++;
    return n;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
char *jpegcrt_strcpy(char *dest, const char *source)
{
    char *save = dest;

    while ((*dest++ = *source++) != '\0')
        ;
    return save;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
int jpegcrt_memcmp(const void *s1, const void *s2, size_t len)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    int c1, c2;

    for (len++; --len;)
        if ((c1 = *p1++) != (c2 = *p2++))
            return c1 - c2;
    return 0;
}

/* jpegcrt_memset (MEMZERO). STATE: MATCH, byte-identical (29 B); the body is
   pngcrt_memset's. OPEN: the original's name is unknown. */
// FUNCTION: EVNOVA 0x0054dd44
void *jpegcrt_memset(void *dest, int c, size_t len)
{
    unsigned char *d = dest;

    while (len--)
        *d++ = (unsigned char)c;
    return dest;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
static const int jpegcrt_ctype[256] = {
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

static int jpegcrt_isspace(int c) { return (jpegcrt_ctype[c] & CT_SPACE) ? 1 : 0; }
static int jpegcrt_isdigit(int c) { return (jpegcrt_ctype[c] & CT_DIGIT) ? 1 : 0; }

/* not linked: dead-stripped in the exe; kept for the TU's data */
double jpegcrt_strtod(const char *s, char **endptr)
{
    double result = 0.0;
    double scale = 0.0;
    int negative;
    int value;
    int digits;

    while (jpegcrt_isspace(*s))
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
    while (jpegcrt_isdigit(*s)) {
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
        while (jpegcrt_isdigit(*s)) {
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
        while (jpegcrt_isdigit(*s)) {
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
int jpegcrt_abs(int x)
{
    return x < 0 ? -x : x;
}
