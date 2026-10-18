// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitcopy8.c -- the two 8-bit-source CopyBits blitters (win 0x004b7230 and 0x004b73b0). Notes: Research/notes/blitcopy8.md */
#include "gametypes.h"
#include "macport.h"

/* the same blitter-family record the 15-bit pair uses: a bounds Rect at
   +0x00, the pixel base at +0x0c, a 3-byte-entry colour table at +0x10 (this
   is its only reader), and the row stride at +0x20 */

/* CopyBitsCopy8. STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x004b7230
void CopyBitsCopy8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    long width;
    long height;
    long srcRow;
    long dstRow;
    unsigned char *dstP;
    unsigned long rgb;
    unsigned long rgb2;
    unsigned char *srcP;
    unsigned char *table;
    long n;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left);
    srcRow = src->rowBytes - width;
    table = src->ctab;
    dstP = (unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2;
    dstRow = dst->rowBytes - width * 2;
    if (width > 0 && height > 0) {
        do {
            n = width;
            if (n > 1) {
                do {
                /* rgb (srcP[0]) first: the index-load order assigns the
                   whole function's colouring, prologue included. */
                rgb = *(unsigned long *)(table + srcP[0] * 3);
                rgb2 = *(unsigned long *)(table + srcP[1] * 3);
                *(unsigned long *)dstP =
                    ((rgb << 7 & 0x7c00) + (rgb >> 6 & 0x3e0) +
                     (rgb >> 0x13 & 0x1f)) +
                    (((rgb2 << 7 & 0x7c00) + (rgb2 >> 6 & 0x3e0) +
                      (rgb2 >> 0x13 & 0x1f)) << 16);
                srcP += 2;
                dstP += 4;
                } while ((n -= 2) > 1);
            }
            if (n != 0) {
                rgb = *(unsigned long *)(table + *srcP * 3);
                *(unsigned short *)dstP = ((rgb << 7 & 0x7c00) +
                    (rgb >> 6 & 0x3e0) + (rgb >> 0x13 & 0x1f));
                srcP++;
                dstP += 2;
            }
            srcP += srcRow;
            dstP += dstRow;
            height--;
        } while (height != 0);
    }
}

/* CopyBitsOr8. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b73b0
void CopyBitsOr8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    unsigned char *srcP;
    long height;
    long srcRow;
    long dstRow;
    unsigned short *dstP;
    unsigned long rgb;
    unsigned char *table;
    long width;
    unsigned int idx;
    long n;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left);
    srcRow = src->rowBytes - width;
    table = src->ctab;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                idx = *srcP;
                if (idx != 0) {
                    rgb = *(unsigned long *)(table + idx * 3);
                    *dstP = ((rgb << 7 & 0x7c00) + (rgb >> 6 & 0x3e0) +
                                              (rgb >> 0x13 & 0x1f));
                }
                srcP++;
                dstP++;
                n--;
            } while (n != 0);
            srcP += srcRow;
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}
