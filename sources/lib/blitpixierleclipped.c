// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixierleclipped.c -- BlitPixieRLEClipped and the RLE byte-swapper FixupRLEPixelData (win 0x00471e90-0x00472190). Notes: Research/notes/blitpixierleclipped.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#ifdef MACBUILD
#define kTokenShift 24
#define kCountMask 0x00ffffff
#define kEndShapeToken 0
#define kLineStartToken 1
#define kDrawPixelsToken 2
#define kSkipPixelsToken 3
#define kSingleColorToken 4
typedef uint32_t TokenDataType;
#define ALIGN_PTR(p) (-((int32_t)p) & 3)
#endif

/* BlitPixieRLEClipped (mac _BlitPixieRLEClipped). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471e90
// MAC: _BlitPixieRLEClipped i386 0x000bde9c ppc 0x000c9938
#ifdef MACBUILD
void BlitPixieRLEClipped(
    unsigned char *srcTokens,
    unsigned char *dstPixel,
    int32_t dstOffset,
    Rect *dstClipRect)
{
    uint32_t token,op;
    int32_t count,extra,spanend;
    unsigned char *dstRow;
    int x,y;

    y = 0;
    dstRow = dstPixel;

    do {
        token = *((TokenDataType *) srcTokens);
        (char *)srcTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < dstClipRect->right && spanend > dstClipRect->left) {
                    if (x < dstClipRect->left) {
                        extra = dstClipRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        (char *)srcTokens += extra;
                    }

                    extra = 0;
                    if (spanend > dstClipRect->right) {
                        extra = spanend - dstClipRect->right;

                        count -= extra;
                    }

                    BlitPixieMemCopy(dstPixel, srcTokens, count);

                    x += extra;
                    dstPixel += extra;
                    (char *)srcTokens += extra;
                }

                x += count;
                dstPixel += count;
                (char *)srcTokens += count;
                (char *)srcTokens += ALIGN_PTR(srcTokens);

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < dstClipRect->right && spanend > dstClipRect->left) {
                    if (x < dstClipRect->left) {
                        extra = dstClipRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > dstClipRect->right) {
                        extra = spanend - dstClipRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, *((uint32_t *) srcTokens), count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                srcTokens += sizeof(int32_t);
                break;
            case kLineStartToken:
                if (y < dstClipRect->top) {
                    (char *)srcTokens += count;
                } else if (y >= dstClipRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = dstRow;
                dstRow += dstOffset;
                break;
            case kEndShapeToken:
                break;
            default:
                return;
        }
    } while (op != kEndShapeToken);
}

#else
void BlitPixieRLEClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    uint32_t token;
    uint32_t opcode;
    int32_t count;
    int32_t x;
    int32_t rowIndex;
    int32_t lastCol;
    int32_t trim;

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
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
            rleTokens += (-(int32_t)rleTokens) & 3;
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

                BlitPixieMemSet(dstPixel, *(uint32_t *)rleTokens, count);

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
#endif

#ifdef MACBUILD
/* BlitPixieRLEColor (mac _BlitPixieRLEColor, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRLEColor i386 0x000be0b8 ppc 0x000c9b74
void BlitPixieRLEColor(
    unsigned char *srcTokens,
    uint32_t srcColor,
    unsigned char *dstPixel,
    uint32_t dstRowBytes)
{
    uint32_t token,op,count;
    unsigned char *dstRow;

    dstRow = dstPixel;

    do {
        token = *((TokenDataType *) srcTokens);
        srcTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:
                BlitPixieMemSet(dstPixel, srcColor, count);
                dstPixel += count;
                srcTokens += count;
                PtrAddBytes(srcTokens, ALIGN_PTR(srcTokens));
                break;
            case kSkipPixelsToken:
                dstPixel += count;
                break;
            case kSingleColorToken:
                BlitPixieMemSet(dstPixel, srcColor, count);
                dstPixel += count;
                srcTokens += sizeof(int32_t);
                break;
            case kLineStartToken:
                dstPixel = dstRow;
                dstRow += dstRowBytes;
                break;
            case kEndShapeToken:
                break;
            default:
                return;
        }
    } while (op != kEndShapeToken);
}

/* BlitPixieRLEColorClipped (mac _BlitPixieRLEColorClipped, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRLEColorClipped i386 0x000be19c ppc 0x000c9c84
void BlitPixieRLEColorClipped(
    unsigned char *srcTokens,
    uint32_t srcColor,
    unsigned char *dstPixel,
    uint32_t dstOffset,
    Rect *dstClipRect)
{
    unsigned short token,op;
    int32_t count,extra,spanend;
    unsigned char *dstRow;
    int x,y;

    y = 0;
    dstRow = dstPixel;

    do {
        token = *((TokenDataType *) srcTokens);
        srcTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < dstClipRect->right && spanend > dstClipRect->left) {
                    if (x < dstClipRect->left) {
                        extra = dstClipRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        srcTokens += extra;
                    }

                    extra = 0;
                    if (spanend > dstClipRect->right) {
                        extra = spanend - dstClipRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, srcColor, count);

                    x += extra;
                    dstPixel += extra;
                    srcTokens += extra;
                }

                x += count;
                dstPixel += count;
                srcTokens += count;
                PtrAddBytes(srcTokens, ALIGN_PTR(srcTokens));

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < dstClipRect->right && spanend > dstClipRect->left) {
                    if (x < dstClipRect->left) {
                        extra = dstClipRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > dstClipRect->right) {
                        extra = spanend - dstClipRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, srcColor, count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                srcTokens += sizeof(int32_t);
                break;
            case kLineStartToken:
                if (y < dstClipRect->top) {
                    srcTokens += count;
                } else if (y >= dstClipRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = dstRow;
                dstRow += dstOffset;
                break;
            case kEndShapeToken:
                break;
            default:
                return;
        }
    } while (op != kEndShapeToken);
}
#endif

/* The 'RLE#' resource header; the layout of record is
   sources/lib/swcreaterledata.c. */

/* FixupRLEPixelData. STATE: MATCH 138/138, compiled after BlitPixieRLEClipped, its predecessor
   in the original object. OPEN: none */
// FUNCTION: EVNOVA 0x00472000
#ifndef MACBUILD
void FixupRLEPixelData(unsigned char *tokens, RLEPixelData *header)
{
    uint32_t token, opcode, count;
    uint32_t depth;
    uint32_t frames;
    unsigned short pixel;

    depth = header->depth;
    frames = header->frames;

    if (frames == 0)
        return;

    do {
        token = *(uint32_t *)tokens;
        *(uint32_t *)tokens = ((token >> 24) & 0xff) | ((token >> 8) & 0xff00) |
                ((token & 0xff00) << 8) | ((token & 0xff) << 24);

        token = *(uint32_t *)tokens;
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
                    token = (((int32_t)token >> 8) & 0xff) | ((token & 0xff) << 8);
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
                    token = *(uint32_t *)tokens;
                    *(uint32_t *)tokens = ((token >> 24) & 0xff) |
                            ((token >> 8) & 0xff00) | ((token & 0xff00) << 8) |
                            ((token & 0xff) << 24);

                    tokens += 4;
                } while (--count);
                break;
            }

            tokens += (-(int32_t)tokens) & 3;
            break;

        case 4:
            token = *(uint32_t *)tokens;
            *(uint32_t *)tokens = ((token >> 24) & 0xff) |
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
#endif
