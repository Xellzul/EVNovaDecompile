// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitcopy16.c -- the 16-bit straight-copy blitter (win 0x004b74c0). Notes: Research/notes/blitcopy16.md */
#include "macport.h"
#include "gametypes.h"

/* CopyBitsCopy16. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b74c0
void CopyBitsCopy16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    long width;
    long height;
    long srcRow;
    long dstRow;
    unsigned short *srcP;
    unsigned short *dstP;
    long n;
    unsigned long a;
    unsigned long b;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned short *)((unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left) * 2);
    srcRow = src->rowBytes - width * 2;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            if (n > 3) {
                do {
                    a = ((unsigned long *)srcP)[0];
                    b = ((unsigned long *)srcP)[1];
                    ((unsigned long *)dstP)[0] = a;
                    ((unsigned long *)dstP)[1] = b;
                    srcP += 4;
                    dstP += 4;
                    n -= 4;
                } while (n > 3);
            }
            if (n > 1) {
                *(unsigned long *)dstP = *(unsigned long *)srcP;
                srcP += 2;
                dstP += 2;
                n -= 2;
            }
            if (n != 0) {
                *dstP = *srcP;
                srcP++;
                dstP++;
            }
            srcP = (unsigned short *)((char *)srcP + srcRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}
