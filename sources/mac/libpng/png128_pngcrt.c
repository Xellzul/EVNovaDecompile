/* png128_pngcrt.c -- the C-runtime calls the exe's libpng links privately, as win's pngcrt.c. Not public libpng text; its licence is libpng's. Notes: Research/notes/maclibs.md */

#include <stddef.h>
#include <MacMemory.h>

#define CT_SPACE  1
#define CT_DIGIT  2
#define CT_UXDIG  4     /* A-F */
#define CT_UPPER  8     /* G-Z */
#define CT_LXDIG  16    /* a-f */
#define CT_LOWER  32    /* g-z */

void *pngcrt_malloc(size_t size);
void pngcrt_free(void *ptr);
void *pngcrt_memcpy(void *dest, const void *source, size_t len);
size_t pngcrt_strlen(const char *s);
char *pngcrt_strcpy(char *dest, const char *source);
int pngcrt_memcmp(const void *s1, const void *s2, size_t len);
void *pngcrt_memset(void *dest, int c, size_t len);
double pngcrt_strtod(const char *s, char **endptr);
int pngcrt_abs(int x);

void *pngcrt_malloc(size_t size)
{
    return size != 0 ? NewPtr(size) : NULL;
}

void pngcrt_free(void *ptr)
{
    if (ptr != NULL)
        DisposePtr(ptr);
}

void *pngcrt_memcpy(void *dest, const void *source, size_t len)
{
    BlockMoveData(source, dest, len);
    return dest;
}

size_t pngcrt_strlen(const char *s)
{
    size_t n = 0;

    while (*s++ != '\0')
        n++;
    return n;
}

char *pngcrt_strcpy(char *dest, const char *source)
{
    char *save = dest;

    while ((*dest++ = *source++) != '\0')
        ;
    return save;
}

int pngcrt_memcmp(const void *s1, const void *s2, size_t len)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    unsigned int c1, c2;

    for (len++; --len;)
        if ((c1 = *p1++) != (c2 = *p2++))
            return c1 - c2;
    return 0;
}

void *pngcrt_memset(void *dest, int c, size_t len)
{
    unsigned char *d = dest;

    while (len--)
        *d++ = (unsigned char)c;
    return dest;
}

static int pngcrt_ctype[256] = {
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
    CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, CT_LOWER, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/* BUG: callers pass a signed char, so bytes 0x80..0xff read the data before the table */
static int pngcrt_isspace(int c) { return (pngcrt_ctype[c] & CT_SPACE) ? 1 : 0; }
static int pngcrt_isdigit(int c) { return (pngcrt_ctype[c] & CT_DIGIT) ? 1 : 0; }

double pngcrt_strtod(const char *s, char **endptr)
{
    double result = 0.0;
    double scale = 0.0;
    int value;
    int digits;
    int negative;

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

int pngcrt_abs(int x)
{
    return x < 0 ? -x : x;
}
