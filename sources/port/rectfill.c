// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* rectfill.c -- FillRectWithColor (win 0x004b6a10): clip a Rect to the current port and fill it with an RGBColor. Notes: Research/notes/rectfill.md */
#include <stddef.h>

#include "macport.h"

CGrafPtr CurrentGrafPort;             /* 0x00870064 */

/* FillRectWithColor. STATE: MATCH, 129/129, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004b6a10
void FillRectWithColor(Rect *r, RGBColor *color)
{
    short top;
    short portTop;
    short left;
    short bottom;
    long advance;
    long rowBytes;
    long width;
    long height;
    long depth;
    unsigned char *dst;
    short portBottom;
    short right;
    short portRight;
    unsigned char rb;
    unsigned char gb;
    unsigned char bb;
    unsigned long pixel;
    long w;
    short portLeft;

    if (r == NULL)
        return;
    top = r->top;
    left = r->left;
    bottom = r->bottom;
    right = r->right;
    portTop = CurrentGrafPort->pix.bounds.top;
    if (top < portTop)
        top = portTop;
    portLeft = CurrentGrafPort->pix.bounds.left;
    if (left < portLeft)
        left = portLeft;
    portBottom = CurrentGrafPort->pix.bounds.bottom;
    if (bottom > portBottom)
        bottom = portBottom;
    portRight = CurrentGrafPort->pix.bounds.right;
    if (right > portRight)
        right = portRight;
    height = bottom - top;
    width = right - left;
    if (height <= 0 || width <= 0)
        return;
    advance = rowBytes = CurrentGrafPort->pix.rowBytes;
    depth = CurrentGrafPort->pix.pixelSize;
    dst = (unsigned char *)CurrentGrafPort->pix.baseAddr + (top - portTop) * rowBytes +
          (left - portLeft) * depth / 8;
    advance -= width * depth / 8;
    switch (depth) {
    case 8:
        break;
    case 16:
        gb = gGammaRamp[color->green >> 11];
        rb = gGammaRamp[color->red >> 11];
        bb = gGammaRamp[color->blue >> 11];
        pixel = (rb << 10) | (gb << 5) | bb;
        pixel |= pixel << 16;
        do {
            w = width;
            do {
                *(unsigned short *)dst = pixel;
                dst += 2;
            } while (--w != 0);
            dst += advance;
        } while (--height != 0);
        break;
    }
}
