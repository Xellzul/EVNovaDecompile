// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* endianswap.c -- byte-swap a resource body from a descriptor (win 0x004ce660). Notes: Research/notes/endianswap.md */

/* resource bodies are big-endian: they come off a Mac-authored rez file.
   Same shape as loaddata.c's helper - `static __inline` is the only spelling
   that inlines an asm block in C. */
static __inline short SwapEndianShort(const unsigned short *p)
{
    register unsigned short t;

    t = *p;
    asm { ror t, 8 }
    return t;
}

static __inline long SwapEndianLong(const unsigned long *p)
{
    register unsigned long t;

    t = *p;
    asm { bswap t }
    return t;
}

#define kSwapSkipByte  0    /* step one byte, swapping nothing */
#define kSwapShort     1
#define kSwapLong      2
#define kSwapSkipN     3    /* step by the following short */
#define kSwapRun       4    /* a counted run; kind and count follow */
#define kSwapEnd       5

/* SwapEndianByDescriptor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce660
void SwapEndianByDescriptor(unsigned char *data, unsigned short *desc)
{
    unsigned char *p;
    unsigned char kind;
    unsigned char count;

    p = data;
    for (;;) {
        switch (desc[0]) {
        case kSwapSkipByte:
            p += 1;
            break;
        case kSwapShort:
            *(unsigned short *)p = SwapEndianShort((unsigned short *)p);
            p += 2;
            break;
        case kSwapLong:
            *(unsigned long *)p = SwapEndianLong((unsigned long *)p);
            p += 4;
            break;
        case kSwapSkipN:
            p += desc[1];
            desc++;
            break;
        case kSwapRun:
            count = desc[2];
            kind = desc[1];
            if (count != 0) {
                switch (kind) {
                case kSwapSkipByte:
                    p += count;
                    break;
                case kSwapShort:
                    do {
                        *(unsigned short *)p = SwapEndianShort((unsigned short *)p);
                        p += 2;
                    } while (--count != 0);
                    break;
                case kSwapLong:
                    do {
                        *(unsigned long *)p = SwapEndianLong((unsigned long *)p);
                        p += 4;
                    } while (--count != 0);
                    break;
                }
            }
            desc += 2;
            break;
        case kSwapEnd:
            return;
        }
        desc++;
    }
}
