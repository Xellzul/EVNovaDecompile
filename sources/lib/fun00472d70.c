// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* fun00472d70.c -- tu-map object O13: four of the six inner translucent RLE blitters of the un-anchored BlitPixie block. Notes: Research/notes/fun00472d70.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRLETranslucentClipped (mac _BlitPixieRLETranslucentClipped). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00472d70
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

/* BlitPixieRLEAddOverClipped (mac _BlitPixieRLEAddOverClipped). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004730d0
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

/* BlitPixieRLETranslucentClipped8 (mac _BlitPixieRLETranslucentClipped8). STATE: MATCH 100.0%, exe 176 / obj 176 instrs, byte-identical yes
   (647 bytes). OPEN: nothing. see notes */
// FUNCTION: EVNOVA 0x00473270
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
    char useTint;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    alpha = ClampAlphaLevel(alpha);
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

/* BlitPixieRLEAddOverClipped8 (mac _BlitPixieRLEAddOverClipped8). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00473500
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
