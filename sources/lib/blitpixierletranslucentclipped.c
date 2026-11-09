// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierletranslucentclipped.c -- tu-map object O13: four of the six inner translucent RLE blitters of the un-anchored BlitPixie block. Notes: Research/notes/blitpixierletranslucentclipped.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#ifdef MACBUILD
#ifndef kTokenShift
#define kTokenShift 24
#define kCountMask 0x00ffffff
#define kEndShapeToken 0
#define kLineStartToken 1
#define kDrawPixelsToken 2
#define kSkipPixelsToken 3
#define kSingleColorToken 4
typedef unsigned long TokenDataType;
#define ALIGN_PTR(p) (-((long)p) & 3)
#endif
#endif

#if !defined(MAC_RT_PICK) || MAC_RT_PICK == 1
/* BlitPixieRLETranslucentClipped (mac _BlitPixieRLETranslucentClipped). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00472d70
// MAC: _BlitPixieRLETranslucentClipped i386 0x000c18c0 ppc 0x000cd93c
#ifdef MACBUILD
#ifdef MACBUILD
void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned long token,op;
    unsigned long count;
    unsigned char *rowStart;
    long extra,spanend;
    short x,y;
    unsigned long redScale, greenScale, blueScale;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    scaleR = gSWCurrentSpriteBeingDrawn->tintRed;
    scaleG = gSWCurrentSpriteBeingDrawn->tintGreen;
    scaleB = gSWCurrentSpriteBeingDrawn->tintBlue;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;
    biasColor = (biasColor & 0xffff) | (biasColor << 16);

    overBias = 32 - gSWCurrentSpriteBeingDrawn->tintMode;

    overR = overG = overB = 0;

    redScale = (scaleR + 32 - alpha)
            ? (scaleR + 32 - alpha) : 0;
    alphaR = redScale;
    greenScale = (scaleG + 32 - alpha)
            ? (scaleG + 32 - alpha) : 0;
    alphaG = greenScale;
    blueScale = (scaleB + 32 - alpha)
            ? (scaleB + 32 - alpha) : 0;
    alphaB = blueScale;

    if (redScale > 32) {
        overR = ((redScale - 32) * 0x1f001f << 5) & 0x7c007c00;
        alphaR = 32;
    }
    if (greenScale > 32) {
        overG = ((greenScale - 32) * 0x1f001f) & 0x3e003e0;
        alphaG = 32;
    }
    if (blueScale > 32) {
        overB = (((blueScale - 32) * 0x1f001f) >> 5) & 0x1f001f;
        alphaB = 32;
    }

    y = 0;
    rowStart = dstPixel;

    do {
        token = *((TokenDataType *) rleTokens);
        rleTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        rleTokens += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieTranslucentCopy(dstPixel, rleTokens, count);

                    x += extra;
                    dstPixel += extra;
                    rleTokens += extra;
                }

                x += count;
                dstPixel += count;
                rleTokens += count;
                (char *)rleTokens += ALIGN_PTR(rleTokens);

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, *((unsigned long *) rleTokens), count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                rleTokens += sizeof(long);
                break;
            case kLineStartToken:
                if (y < srcBlitRect->top) {
                    rleTokens += count;
                } else if (y >= srcBlitRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = rowStart;
                rowStart += dstRowBytes;
                break;
            case kEndShapeToken:
                break;
            default:
                DebugStr("\punknown token");
                return;
        }
    } while (op != kEndShapeToken);
}
#else
void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned long token,op;
    unsigned char *rowStart;
    long count,extra,spanend;
    short x,y;
    unsigned long redScale, greenScale, blueScale;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    scaleR = gSWCurrentSpriteBeingDrawn->tintRed;
    scaleG = gSWCurrentSpriteBeingDrawn->tintGreen;
    scaleB = gSWCurrentSpriteBeingDrawn->tintBlue;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;
    biasColor = (biasColor & 0xffff) | (biasColor << 16);

    overBias = 32 - gSWCurrentSpriteBeingDrawn->tintMode;

    overR = overG = overB = 0;

    redScale = (scaleR + 32 - alpha)
            ? (scaleR + 32 - alpha) : 0;
    alphaR = redScale;
    greenScale = (scaleG + 32 - alpha)
            ? (scaleG + 32 - alpha) : 0;
    alphaG = greenScale;
    blueScale = (scaleB + 32 - alpha)
            ? (scaleB + 32 - alpha) : 0;
    alphaB = blueScale;

    if (redScale > 32) {
        overR = ((redScale - 32) * 0x1f001f << 5) & 0x7c007c00;
        alphaR = 32;
    }
    if (greenScale > 32) {
        overG = ((greenScale - 32) * 0x1f001f) & 0x3e003e0;
        alphaG = 32;
    }
    if (blueScale > 32) {
        overB = (((blueScale - 32) * 0x1f001f) >> 5) & 0x1f001f;
        alphaB = 32;
    }

    y = 0;
    rowStart = dstPixel;

    do {
        token = *( rleTokens);
        rleTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        rleTokens += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieTranslucentCopy(dstPixel, rleTokens, count);

                    x += extra;
                    dstPixel += extra;
                    rleTokens += extra;
                }

                x += count;
                dstPixel += count;
                rleTokens += count;
                rleTokens += ALIGN_PTR(rleTokens);

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, *( rleTokens), count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                rleTokens += sizeof(long);
                break;
            case kLineStartToken:
                if (y < srcBlitRect->top) {
                    rleTokens += count;
                } else if (y >= srcBlitRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = rowStart;
                rowStart += dstRowBytes;
                break;
            case kEndShapeToken:
                break;
            default:
                DebugStr("\punknown token");
                return;
        }
    } while (op != kEndShapeToken);
}
#endif
#else
void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    unsigned long token;
    unsigned long opcode;
    long count;
    short x;
    short rowIndex;
    long lastCol;
    long trim;
    long redScale;
    long greenScale;
    long blueScale;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    scaleR = gSWCurrentSpriteBeingDrawn->tintRed;
    scaleG = gSWCurrentSpriteBeingDrawn->tintGreen;
    scaleB = gSWCurrentSpriteBeingDrawn->tintBlue;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;
    biasColor = ((biasColor << 16) & 0xffff0000) | (biasColor & 0xffff);

    overBias = 32 - gSWCurrentSpriteBeingDrawn->tintMode;
    overR = overG =
            overB = 0;

    alphaR = redScale =
            (scaleR + 32 - alpha != 0)
            ? scaleR + 32 - alpha : 0;

    alphaG = greenScale =
            (scaleG + 32 - alpha != 0)
            ? scaleG + 32 - alpha : 0;

    alphaB = blueScale =
            (scaleB + 32 - alpha != 0)
            ? scaleB + 32 - alpha : 0;

    if ((unsigned long)redScale > 32) {
        overR =
                ((redScale - 32) * 0x1f001f << 5) & 0x7c007c00;
        alphaR = 32;
    }

    if ((unsigned long)greenScale > 32) {
        overG =
                ((greenScale - 32) * 0x1f001f) & 0x3e003e0;
        alphaG = 32;
    }

    if ((unsigned long)blueScale > 32) {
        overB =
                ((unsigned long)(blueScale - 32) * 0x1f001f >> 5) & 0x1f001f;
        alphaB = 32;
    }

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(unsigned long *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            lastCol = x + count;
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

                BlitPixieTranslucentCopy(dstPixel, rleTokens, count);

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
            lastCol = x + count;
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
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}
#endif
#endif

#if !defined(MAC_RT_PICK) || MAC_RT_PICK == 2
/* BlitPixieRLEAddOverClipped (mac _BlitPixieRLEAddOverClipped). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004730d0
// MAC: _BlitPixieRLEAddOverClipped i386 0x000c25e7 ppc 0x000ce954
#ifdef MACBUILD
void BlitPixieRLEAddOverClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned long token,op;
    unsigned long count;
    unsigned char *rowStart;
    long extra,spanend;
    short x,y;

    y = 0;
    rowStart = dstPixel;

    do {
        token = *((TokenDataType *) rleTokens);
        rleTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        rleTokens += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieAddOverCopy(dstPixel, rleTokens, count);

                    x += extra;
                    dstPixel += extra;
                    rleTokens += extra;
                }

                x += count;
                dstPixel += count;
                rleTokens += count;
                (char *)rleTokens += ALIGN_PTR(rleTokens);

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, *((unsigned long *) rleTokens), count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                rleTokens += sizeof(long);
                break;
            case kLineStartToken:
                if (y < srcBlitRect->top) {
                    rleTokens += count;
                } else if (y >= srcBlitRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = rowStart;
                rowStart += dstRowBytes;
                break;
            case kEndShapeToken:
                break;
            default:
                DebugStr("\punknown token");
                return;
        }
    } while (op != kEndShapeToken);
}
#else
void BlitPixieRLEAddOverClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    unsigned long token;
    unsigned long opcode;
    long count;
    short x;
    short rowIndex;
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
            lastCol = x + count;
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

                BlitPixieAddOverCopy(dstPixel, rleTokens, count);

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
            lastCol = x + count;
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
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}
#endif
#endif

#if !defined(MAC_RT_PICK) || MAC_RT_PICK == 3
/* BlitPixieRLETranslucentClipped8 (mac _BlitPixieRLETranslucentClipped8). STATE: MATCH 100.0%, exe 176 / obj 176 instrs, byte-identical yes
   (647 bytes). OPEN: nothing. see notes */
// FUNCTION: EVNOVA 0x00473270
// MAC: _BlitPixieRLETranslucentClipped8 i386 0x000c1ee4 ppc 0x000ce0f4
#ifdef MACBUILD
void BlitPixieRLETranslucentClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned long token,op;
    unsigned long count;
    unsigned char *rowStart;
    long extra,spanend;
    short x,y;
    long level;
    Boolean useTint;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    alpha = fix_power_of_two(alpha);
    alpha >>= 3;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;

    if (gSWCurrentSpriteBeingDrawn->tintMode >= 3) {
        useTint = 1;

        level = 32 - gSWCurrentSpriteBeingDrawn->tintMode;
        level = (level < 31) ? level : 31;

        overBias = level >> 3;
        overBias = (overBias < 4) ? overBias : 4;
    } else {
        useTint = 0;
    }

    y = 0;
    rowStart = dstPixel;

    do {
        token = *((TokenDataType *) rleTokens);
        rleTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        rleTokens += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    if (useTint)
                    BlitPixieTranslucentBiasCopy8(dstPixel, rleTokens, count);
                else
                    BlitPixieTranslucentCopy8(dstPixel, rleTokens, count);

                    x += extra;
                    dstPixel += extra;
                    rleTokens += extra;
                }

                x += count;
                dstPixel += count;
                rleTokens += count;
                (char *)rleTokens += ALIGN_PTR(rleTokens);

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, *((unsigned long *) rleTokens), count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                rleTokens += sizeof(long);
                break;
            case kLineStartToken:
                if (y < srcBlitRect->top) {
                    rleTokens += count;
                } else if (y >= srcBlitRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = rowStart;
                rowStart += dstRowBytes;
                break;
            case kEndShapeToken:
                break;
            default:
                DebugStr("\punknown token");
                return;
        }
    } while (op != kEndShapeToken);
}
#else
void BlitPixieRLETranslucentClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    unsigned long token;
    unsigned long opcode;
    long count;
    short x;
    short rowIndex;
    long lastCol;
    long trim;
    long level;
    Boolean useTint;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    alpha = fix_power_of_two(alpha);
    alpha >>= 3;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;

    if (gSWCurrentSpriteBeingDrawn->tintMode >= 3) {
        useTint = 1;

        level = 32 - gSWCurrentSpriteBeingDrawn->tintMode;
        level = (level < 31) ? level : 31;

        overBias = level >> 3;
        overBias = (overBias < 4) ? overBias : 4;
    } else {
        useTint = 0;
    }

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(unsigned long *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            lastCol = x + count;
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

                if (useTint)
                    BlitPixieTranslucentBiasCopy8(dstPixel, rleTokens, count);
                else
                    BlitPixieTranslucentCopy8(dstPixel, rleTokens, count);

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
            lastCol = x + count;
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
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}
#endif
#endif

#if !defined(MAC_RT_PICK) || MAC_RT_PICK == 4
/* BlitPixieRLEAddOverClipped8 (mac _BlitPixieRLEAddOverClipped8). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00473500
// MAC: _BlitPixieRLEAddOverClipped8 i386 0x000c22af ppc 0x000ce594
#ifdef MACBUILD
void BlitPixieRLEAddOverClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned long token,op;
    unsigned long count;
    unsigned char *rowStart;
    long extra,spanend;
    short x,y;

    y = 0;
    rowStart = dstPixel;

    do {
        token = *((TokenDataType *) rleTokens);
        rleTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        rleTokens += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieAddOverCopy8(dstPixel, rleTokens, count);

                    x += extra;
                    dstPixel += extra;
                    rleTokens += extra;
                }

                x += count;
                dstPixel += count;
                rleTokens += count;
                (char *)rleTokens += ALIGN_PTR(rleTokens);

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, *((unsigned long *) rleTokens), count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                rleTokens += sizeof(long);
                break;
            case kLineStartToken:
                if (y < srcBlitRect->top) {
                    rleTokens += count;
                } else if (y >= srcBlitRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = rowStart;
                rowStart += dstRowBytes;
                break;
            case kEndShapeToken:
                break;
            default:
                DebugStr("\punknown token");
                return;
        }
    } while (op != kEndShapeToken);
}
#else
void BlitPixieRLEAddOverClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    unsigned long token;
    unsigned long opcode;
    long count;
    short x;
    short rowIndex;
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
            lastCol = x + count;
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

                BlitPixieAddOverCopy8(dstPixel, rleTokens, count);

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
            lastCol = x + count;
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
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}
#endif
#endif
