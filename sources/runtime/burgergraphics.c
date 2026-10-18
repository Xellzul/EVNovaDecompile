// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgergraphics.c -- BurgerLib GrGraphicsManager: the display-mode record, the shape copy, the 5-5-5 to 5-6-5 pass. Notes: Research/notes/burgergraphics.md */

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

long ScreenFlags;                               /* 0x0086ff9c, the mode flags */
unsigned char Win95UseBackBuffer = 1;           /* 0x00578244, .data; zero is the arm that ADDS the offsets in LockVideoPage */

#define kGreenLowMask   0x003f003f               /* blue and green's low bit, two pixels */
#define kShiftedMask    0xffc0ffc0               /* red and green moved up one, two pixels */

/* VideoGetCurrentMode (BurgerLib VideoGetCurrentMode). STATE: MATCH 100.0%, exe 13 / obj 13,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9910
void VideoGetCurrentMode(long *out)
{
    out[0] = ScreenWidth;
    out[1] = ScreenHeight;
    out[2] = VideoColorDepth;
    out[3] = ScreenFlags;
}

/* Four fields, three of which must be non-null before the call is worth
   making; a rejected record answers 1. */
/* VideoSetMode (BurgerLib VideoSetMode). STATE: MATCH 100.0%, exe 29 / obj 29,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9940
long VideoSetMode(long *rec)
{
    if (rec == 0 || rec[0] == 0 || rec[1] == 0)
        return 1;
    return SetDisplayToSize(rec[0], rec[1], rec[2], rec[3]);
}

/* DrawShapeLowLevel (BurgerLib GrGraphicsManager). STATE: MATCH 100.0%, exe 125 / obj 125,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f99a0
void DrawShapeLowLevel(Word x, Word y, Word width, Word height, Word skip, void *shape)
{
    Word8 *screen;
    Word32 offset;
    Word longs;
    Word count;

    if (height == 0 || width == 0)
        return;
    screen = (Word8 *)VideoPointer + (VideoWidth * y + x);
    offset = VideoWidth - width;
    longs = width >> 2;
    if (longs != 0) {
        width &= 3;
        if (width != 0) {
            do {
                count = longs;
                do {
                    ((Word32 *)screen)[0] = ((Word32 *)shape)[0];
                    screen += 4;
                    shape = (Word8 *)shape + 4;
                } while (--count);
                count = width;
                do {
                    screen[0] = ((Word8 *)shape)[0];
                    screen += 1;
                    shape = (Word8 *)shape + 1;
                } while (--count);
                screen += offset;
                shape = (Word8 *)shape + skip;
            } while (--height);
            return;
        }
        do {
            count = longs;
            do {
                ((Word32 *)screen)[0] = ((Word32 *)shape)[0];
                screen += 4;
                shape = (Word8 *)shape + 4;
            } while (--count);
            screen += offset;
            shape = (Word8 *)shape + skip;
        } while (--height);
        return;
    }
    skip += width;
    offset += width;
    switch (width) {
    default:
        do {
            screen[0] = ((Word8 *)shape)[0];
            shape = (Word8 *)shape + skip;
            screen += offset;
        } while (--height);
        return;
    case 2:
        do {
            ((Word16 *)screen)[0] = ((Word16 *)shape)[0];
            shape = (Word8 *)shape + skip;
            screen += offset;
        } while (--height);
        return;
    case 3:
        do {
            ((Word16 *)screen)[0] = ((Word16 *)shape)[0];
            screen[2] = ((Word8 *)shape)[2];
            shape = (Word8 *)shape + skip;
            screen += offset;
        } while (--height);
    }
}

/* Video555To565 (BurgerLib GrGraphicsManager). STATE: MATCH 100.0%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9b40
void Video555To565(void)
{
    Word count;
    Word32 *src;
    Word32 *dst;
    Word32 a;
    Word32 b;
    Word32 c;
    Word32 d;
    Word rows;
    Word32 *row;

    dst = (Word32 *)VideoPointer;
    src = (Word32 *)VideoOffscreen;
    rows = ScreenHeight;
    do {
        count = ScreenWidth >> 2;
        row = dst;
        do {
            a = src[0];
            c = a + a;
            b = src[1];
            d = b + b;
            a &= kGreenLowMask;
            b &= kGreenLowMask;
            c &= kShiftedMask;
            a += c;
            dst[0] = a;
            d &= kShiftedMask;
            b += d;
            dst[1] = b;
            src += 2;
            dst += 2;
        } while (--count);
        dst = (Word32 *)((Word8 *)row + VideoWidth);
    } while (--rows);
}

#ifdef __cplusplus
}
#endif
