/* endianswap.h -- the big-endian field readers the win port uses on Mac-authored resources.
   Notes: Research/notes/endianswap.md */
#ifndef ENDIANSWAP_H
#define ENDIANSWAP_H

#ifdef MACBUILD
/* the Mac Resource Manager hands its data over in host order: nothing to swap */
static __inline short SwapEndianShort(const unsigned short *p)
{
    return *p;
}

static __inline long SwapEndianLong(const unsigned long *p)
{
    return *p;
}
#else
/* BurgerLib's SwapEndian written out; `static __inline' is the only spelling that inlines an
   asm block in C */
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
#endif

#endif
