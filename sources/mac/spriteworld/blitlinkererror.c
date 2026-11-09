/* blitlinkererror.c -- Trans8Bit and Trans16Bit, the translucent blends the fork kept out of line (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"

/* Trans8Bit (mac _Trans8Bit, mac-only). STATE: see the mac gate. */
// MAC: _Trans8Bit i386 0x000c2bf0 ppc 0x000cf024
char Trans8Bit(char src, char dst, unsigned long alpha)
{
    /* BUG: any alpha above 3 skips the blend, so (31 - alpha) >> 3 only ever selects row 3 */
    if (alpha > 3)
        return src;
    return alphaTable[(31 - alpha) >> 3][src][dst];
}

/* Trans16Bit (mac _Trans16Bit, mac-only). STATE: see the mac gate. */
// MAC: _Trans16Bit i386 0x000c2c2c ppc 0x000cf058
unsigned short Trans16Bit(unsigned long src, unsigned long dst, unsigned long alpha)
{
    unsigned long srcSplit, dstSplit, blended;

    srcSplit = SW_SPLIT15(src);
    dstSplit = SW_SPLIT15(dst);
    blended = srcSplit * alpha + dstSplit * (32 - alpha);
    return ((blended >> 5) & 0x7c1f) | ((blended >> 20) & 0x3e0);
}
