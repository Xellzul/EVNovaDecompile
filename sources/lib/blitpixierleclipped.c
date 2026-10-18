// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierleclipped.c -- BlitPixieRLEClipped and the RLE byte-swapper FixupRLEPixelData (win 0x00471e90-0x00472190). Notes: Research/notes/blitpixierleclipped.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRLEClipped (mac _BlitPixieRLEClipped). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471e90
void BlitPixieRLEClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    unsigned long token;
    unsigned long opcode;
    long count;
    long x;
    long rowIndex;
    long lastCol;
    long trim;

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(unsigned long *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            lastCol = x + count - 1;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                    rleTokens += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieMemCopy(dstPixel, rleTokens, count);

                x += trim;
                dstPixel += trim;
                rleTokens += trim;
            }
            rleTokens += count;
            x += count;
            dstPixel += count;
            rleTokens += (-(long)rleTokens) & 3;
            break;

        case 3:
            x += count;
            dstPixel += count;
            break;

        case 4:
            lastCol = x + count - 1;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieMemSet(dstPixel, *(unsigned long *)rleTokens, count);

                x += trim;
                dstPixel += trim;
            }
            x += count;
            dstPixel += count;
            rleTokens += 4;
            break;

        case 1:
            if (rowIndex < srcBlitRect->top)
                rleTokens += count;
            else if (rowIndex >= srcBlitRect->bottom) {
                opcode = 0;
                break;
            }

            dstPixel = rowStart;
            x = 0;
            rowIndex++;
            rowStart += dstRowBytes;
            break;

        case 0:
            break;

        default:
            return;
        }
    } while (opcode != 0);
}

/* The 'RLE#' resource header; the layout of record is
   sources/lib/swcreaterledata.c. */

/* FixupRLEPixelData. STATE: MATCH 138/138, compiled after BlitPixieRLEClipped, its predecessor
   in the original object. OPEN: none */
// FUNCTION: EVNOVA 0x00472000
void FixupRLEPixelData(unsigned char *tokens, RLEPixelData *header)
{
    unsigned long token, opcode, count;
    unsigned long depth;
    unsigned long frames;
    unsigned short pixel;

    depth = header->depth;
    frames = header->frames;

    if (frames == 0)
        return;

    do {
        token = *(unsigned long *)tokens;
        *(unsigned long *)tokens = ((token >> 24) & 0xff) | ((token >> 8) & 0xff00) |
                ((token & 0xff00) << 8) | ((token & 0xff) << 24);

        token = *(unsigned long *)tokens;
        opcode = token >> 24;
        tokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            switch (depth) {
            case 8:
                tokens += count;
                break;

            case 16:
                count >>= 1;
                do {
                    token = *(unsigned short *)tokens;
                    token = (((long)token >> 8) & 0xff) | ((token & 0xff) << 8);
                    pixel = token;

                    *(unsigned short *)tokens =
                            (gGammaRamp[(pixel >> 10) & 0x1f] << 10) +
                            (gGammaRamp[(pixel >> 5) & 0x1f] << 5) +
                            gGammaRamp[pixel & 0x1f];

                    tokens += 2;
                } while (--count);
                break;

            case 32:
                count >>= 2;
                do {
                    token = *(unsigned long *)tokens;
                    *(unsigned long *)tokens = ((token >> 24) & 0xff) |
                            ((token >> 8) & 0xff00) | ((token & 0xff00) << 8) |
                            ((token & 0xff) << 24);

                    tokens += 4;
                } while (--count);
                break;
            }

            tokens += (-(long)tokens) & 3;
            break;

        case 4:
            token = *(unsigned long *)tokens;
            *(unsigned long *)tokens = ((token >> 24) & 0xff) |
                    ((token >> 8) & 0xff00) | ((token & 0xff00) << 8) |
                    ((token & 0xff) << 24);

            tokens += 4;
            break;

        case 0:
        case 1:
        case 3:
            break;

        default:
            return;
        }
    } while (opcode != 0 || --frames != 0);
}
