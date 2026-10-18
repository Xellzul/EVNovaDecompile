// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixieaddovercopy.c -- BlitPixieAddOverCopy (win 0x00473b60), inside the un-anchored BlitPixie block. Notes: Research/notes/blitpixieaddovercopy.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieAddOverCopy (mac _BlitPixieAddOverCopy). STATE: MATCH 100.0%, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473b60
void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                  long byteWidth)
{
    long i;
    unsigned char *s;
    unsigned char *d;

    /* s/d copies: decl (s,d) picks the load regs (src=esi dst=ebx), d-first
       statements pick dst-first load order. Increments stay src-first to
       match the exe's add order. */
    d = dstPixels;
    s = srcPixels;
    for (i = 0; i < (byteWidth >> 2); i++) {
        *(unsigned long *)d |= *(unsigned long *)s;
        s += 4;
        d += 4;
    }

    if (byteWidth & 2) {
        unsigned short *wordDst = (unsigned short *)d;
        *wordDst = *wordDst | *(unsigned short *)s;
    }

    /* BUG: the tails never advance s and d, so a width of 4n+3 re-ORs the short's low byte */
    if (byteWidth & 1) {
        *d = *d | *s;
    }
}
