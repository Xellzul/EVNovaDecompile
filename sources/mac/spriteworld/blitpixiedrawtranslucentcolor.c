/* blitpixiedrawtranslucentcolor.c -- BlitPixieDrawTranslucentColor, the last function of BlitPixieUtils.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"

#define SW_RED(c, k)    ((((c) & 0x7c007c00) >> 5) * (k) & 0x7c007c00)
#define SW_GREEN(c, k)  (((c) & 0x3e003e0) * (k) >> 5 & 0x3e003e0)
#define SW_BLUE(c, k)   (((c) & 0x1f001f) * (k) >> 5 & 0x1f001f)

/* BlitPixieDrawTranslucentColor (mac _BlitPixieDrawTranslucentColor, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDrawTranslucentColor i386 0x000c04a1 ppc 0x000cc1c0
void BlitPixieDrawTranslucentColor(unsigned char *dst, unsigned long color, long bytes, unsigned long alpha)
{
    long i;
    unsigned long pixel, d;

    {
        unsigned long *lp = (unsigned long *)dst;

        for (i = 0; i < (bytes >> 2); i++) {
            d = *lp;
            pixel = SW_RED(d, alpha);
            pixel |= SW_RED(color, alpha) | SW_GREEN(color, alpha) | SW_BLUE(color, alpha);
            pixel |= SW_GREEN(d, alpha);
            pixel |= SW_BLUE(d, alpha);
            *lp++ = pixel;
        }
        dst = (unsigned char *)lp;
    }

    if (bytes & 2) {
        pixel = SW_RED(color, alpha) | SW_GREEN(color, alpha) | SW_BLUE(color, alpha);
        pixel |= SW_RED(*(unsigned short *)dst, alpha);
        pixel |= SW_GREEN(*(unsigned short *)dst, alpha);
        pixel |= SW_BLUE(*(unsigned short *)dst, alpha);
        ((unsigned short *)dst)[1] = ((unsigned short *)dst)[0] = pixel;
        dst += 4;
    }

    if (bytes & 1) {
        pixel = SW_RED(color, alpha) | SW_GREEN(color, alpha) | SW_BLUE(color, alpha);
        pixel |= SW_RED(*dst, alpha);
        pixel |= SW_GREEN(*dst, alpha);
        pixel |= SW_BLUE(*dst, alpha);
        *dst = pixel;
    }
}
