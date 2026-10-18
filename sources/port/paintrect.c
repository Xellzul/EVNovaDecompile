// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* paintrect.c -- PaintRect, InvertRect, EraseRect, FrameRect; exe 0x004b6b90..0x004b6ef0. Notes: Research/notes/qddraw.md */

#include <stddef.h>
#include "macport.h"
#include "burger.h"
#include "render.h"

/* PaintRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6b90
void PaintRect(Rect *r)
{
    if (CurrentGrafPort == NULL)
        return;
    FillRectWithColor(r, &CurrentGrafPort->foreColor);
}

/* InvertRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6bc0
void InvertRect(Rect *r)
{
    short top;
    short portLeft;
    short portBottom;
    short portRight;
    short bottom;
    short left;
    short portTop;
    short right;
    long width;
    long height;
    long rowBytes;
    long pixelSize;
    unsigned char *p;
    unsigned long bytes;
    unsigned long pixels;

    if (CurrentGrafPort == NULL || r == NULL)
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

    width = right - left;
    height = bottom - top;
    if (height <= 0 || width <= 0)
        return;

    rowBytes = CurrentGrafPort->pix.rowBytes;
    pixelSize = CurrentGrafPort->pix.pixelSize;
    p = (unsigned char *)CurrentGrafPort->pix.baseAddr +
        (top - portTop) * rowBytes +
        (left - portLeft) * pixelSize / 8;
    rowBytes -= width * pixelSize / 8;

    if (height != 0 && width != 0) {
        switch (CurrentGrafPort->pix.pixelSize) {
        case 8:
            do {
                bytes = width;
                if (bytes > 3) {
                    do {
                        *(unsigned long *)p = ~*(unsigned long *)p;
                        p += 4;
                        bytes -= 4;
                    } while (bytes > 3);
                }
                if (bytes > 1) {
                    *(unsigned short *)p = ~*(unsigned short *)p;
                    bytes -= 2;
                    p += 2;
                }
                if (bytes != 0) {
                    *p = ~*p;
                    p++;
                }
                p += rowBytes;
                height--;
            } while (height != 0);
            break;

        case 16:
            do {
                pixels = width;
                if (pixels > 1) {
                    do {
                        *(unsigned long *)p = ~*(unsigned long *)p & 0x7fff7fff;
                        p += 4;
                        pixels -= 2;
                    } while (pixels > 1);
                }
                if (pixels != 0) {
                    *(unsigned short *)p = (~*(unsigned short *)p & 0x7fff);
                    p += 2;
                }
                p += rowBytes;
                height--;
            } while (height != 0);
            break;

        default:
            break;
        }
    }
}

/* the Toolbox erase is a fill in the port's background colour */
/* EraseRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6ec0
void EraseRect(Rect *r)
{
    if (CurrentGrafPort == NULL)
        return;
    FillRectWithColor(r, &CurrentGrafPort->backColor);
}

/* FrameRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6ef0
void FrameRect(const Rect *r)
{
    long width;
    long height;
    int rowBytes;
    long pixelSize;
    short portTop;
    short portLeft;
    short portBottom;
    short portRight;
    short top;
    short left;
    short bottom;
    short right;
    unsigned char *p;
    unsigned char rb;
    unsigned char gb;
    unsigned char bb;
    unsigned long pixel;
    long n;
    RGBColor *fg;
    long colStep;
    long rowStep;

    if (CurrentGrafPort == NULL)
        return;
    if (r == NULL)
        return;

    fg = &CurrentGrafPort->foreColor;
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

    rowBytes = CurrentGrafPort->pix.rowBytes;
    pixelSize = CurrentGrafPort->pix.pixelSize;
    p = (unsigned char *)CurrentGrafPort->pix.baseAddr +
        (top - portTop) * rowBytes +
        (left - portLeft) * pixelSize / 8;
    rowBytes -= width * pixelSize / 8;

    switch (pixelSize) {
    case 8:
        break;
    case 16:
        gb = gGammaRamp[fg->green >> 11];
        rb = gGammaRamp[fg->red >> 11];
        bb = gGammaRamp[fg->blue >> 11];
        pixel = (rb << 10) | (gb << 5) | bb;
        pixel |= pixel << 16;

        /* top row */
        n = width;
        if (n > 1) {
            do {
                *(unsigned long *)p = pixel;
                p += 4;
                n -= 2;
            } while (n > 1);
        }
        if (n != 0) {
            *(unsigned short *)p = pixel;
            p += 2;
        }

        /* left/right columns of the rows between */
        height -= 2;
        p += rowBytes;
        if (height > 0) {
            colStep = (width - 1) * 2;
            rowStep = rowBytes + 2;
            do {
                *(unsigned short *)p = pixel;
                p += colStep;
                *(unsigned short *)p = pixel;
                p += rowStep;
                height--;
            } while (height != 0);
        }

        /* bottom row */
        n = width;
        if (n > 1) {
            do {
                *(unsigned long *)p = pixel;
                p += 4;
                n -= 2;
            } while (n > 1);
        }
        if (n != 0)
            *(unsigned short *)p = pixel;
        break;
    }
}
