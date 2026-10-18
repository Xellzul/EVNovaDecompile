// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierleparsepixels.c -- BlitPixieRLEParsePixels (win 0x00472510). Notes: Research/notes/blitpixierleparsepixels.md */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"

#define kParamErr       -50
#define kMemFullErr     -108

#define RLE_ROW         0x1000000
#define RLE_LITERAL     0x2000000
#define RLE_TRANSPARENT 0x3000000
#define RLE_FILL        0x4000000

/* BlitPixieRLEParsePixels (mac _BlitPixieRLEParsePixels). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00472510
short BlitPixieRLEParsePixels(unsigned char *srcPixels, unsigned char *maskPixels,
        unsigned long rowBytes, unsigned short byteWidth, unsigned short numRows,
        unsigned short depth, void ***frameData)
{
    void **frameHandle;
    long *rowHeader;
    unsigned char *srcRow;
    unsigned char *maskRow;
    unsigned char *srcPixel;
    unsigned char *maskPixel;
    unsigned long row;
    long *runToken;
    long runCount;
    long bytesPerPixel;
    long sameCount;
    long depthL;
    unsigned long lastPixel;
    unsigned long maskValue;
    unsigned long pixelValue;
    unsigned long width;
    unsigned long rows;
    unsigned char *tokens;
    unsigned long x;
    unsigned char state;
    long size;
    short err;

    *frameData = 0;

    if (depth != 8 && depth != 16 && depth != 32)
        return kParamErr;

    if (byteWidth == 0 || numRows == 0)
        return kParamErr;

    rows = numRows;
    depthL = depth;
    bytesPerPixel = depthL >> 3;

    frameHandle = (void **)AllocAHandle2(
            numRows * byteWidth * 8 + rows * 4 + 4, 0);
    if (frameHandle != 0) {
        LockAHandle((Handle)frameHandle);
        tokens = *frameHandle;

        srcRow = srcPixels;
        maskRow = maskPixels;

        row = 0;
        if (row < rows) {
            width = byteWidth;
            do {
                srcPixel = srcRow;

            rowHeader = (long *)tokens;
            tokens += 4;

            runCount = 0;
            state = 0;
            sameCount = 0;
            maskPixel = maskRow;

            for (x = 0; x < width; x += bytesPerPixel) {
                switch (depthL) {
                case 8:
                    maskValue = *maskPixel;
                    break;
                case 16:
                    maskValue = *(unsigned short *)maskPixel;
                    break;
                case 32:
                    maskValue = *(unsigned long *)maskPixel;
                    break;
                }

                if (maskValue != 0) {
                    sameCount = 0;

                    switch (state) {
                    case 1:
                        runCount += bytesPerPixel;
                        break;

                    case 2:
                        *runToken = runCount + RLE_LITERAL;
                        tokens += (-(long)tokens) & 3;
                        runToken = (long *)tokens;
                        tokens += 4;
                        state = 1;
                        runCount = bytesPerPixel;
                        break;

                    case 3:
                        *runToken = runCount + RLE_FILL;
                        if (depth < 16)
                            lastPixel |= lastPixel << 8;
                        if (depth < 32)
                            lastPixel |= lastPixel << 16;
                        runToken[1] = lastPixel;
                        runToken = (long *)tokens;
                        tokens += 4;
                        state = 1;
                        runCount = bytesPerPixel;
                        break;

                    case 0:
                        runToken = (long *)tokens;
                        tokens += 4;
                        state = 1;
                        runCount = bytesPerPixel;
                        break;
                    }
                } else {
                    switch (depthL) {
                    case 8:
                        pixelValue = *srcPixel;
                        break;
                    case 16:
                        pixelValue = *(unsigned short *)srcPixel;
                        break;
                    case 32:
                        pixelValue = *(unsigned long *)srcPixel;
                        break;
                    }

                    if (sameCount != 0 && pixelValue == lastPixel)
                        sameCount += bytesPerPixel;
                    else
                        sameCount = bytesPerPixel;

                    switch (state) {
                    case 2:
                        runCount += bytesPerPixel;
                        break;

                    case 3:
                        *runToken = runCount + RLE_FILL;
                        if (depth < 16)
                            lastPixel |= lastPixel << 8;
                        if (depth < 32)
                            lastPixel |= lastPixel << 16;
                        runToken[1] = lastPixel;
                        runToken = (long *)tokens;
                        tokens += 4;
                        state = 2;
                        runCount = bytesPerPixel;
                        break;

                    case 1:
                        *runToken = runCount + RLE_TRANSPARENT;
                        runToken = (long *)tokens;
                        tokens += 4;
                        state = 2;
                        runCount = bytesPerPixel;
                        break;

                    case 0:
                        runToken = (long *)tokens;
                        tokens += 4;
                        state = 2;
                        runCount = bytesPerPixel;
                        break;
                    }

                    switch (depthL) {
                    case 8:
                        *tokens = pixelValue;
                        tokens += 1;
                        break;
                    case 16:
                        *(unsigned short *)tokens = pixelValue;
                        tokens += 2;
                        break;
                    case 32:
                        *(unsigned long *)tokens = pixelValue;
                        tokens += 4;
                        break;
                    }

                    lastPixel = pixelValue;
                }

                srcPixel += bytesPerPixel;
                maskPixel += bytesPerPixel;
            }

            switch (state) {
            case 1:
                tokens = (unsigned char *)runToken;
                break;

            case 2:
                *runToken = runCount + RLE_LITERAL;
                tokens += (-(long)tokens) & 3;
                break;

            case 3:
                *runToken = runCount + RLE_FILL;
                if (depth < 16)
                    lastPixel |= lastPixel << 8;
                if (depth < 32)
                    lastPixel |= lastPixel << 16;
                runToken[1] = lastPixel;
                runToken += 2;
                break;
            }

            *rowHeader = (long)(tokens - 4) + RLE_ROW - (long)rowHeader;

            srcRow += rowBytes;
            maskRow += rowBytes;
            row++;
        } while (row < rows);
        }

        *(long *)tokens = 0;

        size = (tokens + 4) - (unsigned char *)*frameHandle;

        UnlockAHandle((Handle)frameHandle);
        ResizeAHandle((Handle)frameHandle, size);

        err = MemError();
        if ((int)err == noErr)
            *frameData = frameHandle;
    } else {
        err = kMemFullErr;
    }

    return err;
}
