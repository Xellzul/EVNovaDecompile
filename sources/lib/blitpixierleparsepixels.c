// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
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

#ifdef MACBUILD
#define kTokenShift 24
#define kSingleColorToken 4
typedef unsigned long TokenDataType;
#define ALIGN_PTR(p) ( (-((long) p)) & 3)
#define kMinimumFillRun 32
#define BLITPIXIE_USE_DRAW_SPANS 0
enum RunFlags
{
	kRunNone = 0,
	kRunSkip,
	kRunDraw,
	kRunFill
};
#endif

/* BlitPixieRLEParsePixels (mac _BlitPixieRLEParsePixels). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00472510
// MAC: _BlitPixieRLEParsePixels i386 0x000c2875 ppc 0x000cec48
#ifdef MACBUILD
OSErr BlitPixieRLEParsePixels(
	unsigned char *srcStartAddrP,
	unsigned char *maskStartAddrP,
	unsigned long rowBytes,
	unsigned short numBytesPerRow,
	unsigned short numberOfScanLines,
	unsigned short depth,
	void ***newPixCode)
{
	OSErr			err = noErr;
	unsigned long	newSize;
	Handle			tempPixCodeH;

	unsigned char	runFlag;

	unsigned char	*lineStartPtr;
	unsigned char 	*runTokenPtr;
	unsigned char	*dataPtr;

	unsigned char	*sourcePtr;
	unsigned char	*maskPtr;
	unsigned char	*sourceStartPtr;
	unsigned char	*maskStartPtr;

	unsigned long	xCounter;
	unsigned long	yCounter;

	unsigned long	thisPixel;
	unsigned long	maskPixel;
	unsigned long	lastPixel;
	unsigned long	advance;

	unsigned long	runCounter;
	unsigned long	sameCounter;

	Boolean			useDrawSpans = BLITPIXIE_USE_DRAW_SPANS;

	*newPixCode = NULL;

	if ( depth != 8 && depth != 16 && depth != 32 )
		return paramErr;
	if ( numBytesPerRow <= 0 || numberOfScanLines <= 0 )
		return paramErr;

	newSize = 2 * sizeof(TokenDataType) * (  numberOfScanLines * (long) numBytesPerRow) +
		 sizeof(TokenDataType) * numberOfScanLines + sizeof(TokenDataType);

	advance = depth >> 3;

	tempPixCodeH = NewHandle(newSize);
	if (tempPixCodeH != NULL)
	{
		HLock ( tempPixCodeH);
		dataPtr = (unsigned char *) ( *tempPixCodeH );

		sourceStartPtr =  srcStartAddrP;
		maskStartPtr =  maskStartAddrP;

		for( yCounter = 0; yCounter < numberOfScanLines; yCounter++ )
		{
			lineStartPtr = dataPtr;
			dataPtr += sizeof( TokenDataType );

			runFlag = kRunNone;
			runCounter = 0;
			sameCounter = 0;

			sourcePtr = sourceStartPtr;
			maskPtr = maskStartPtr;

			for( xCounter = 0; xCounter < numBytesPerRow; xCounter += advance)
			{
				switch ( depth )
				{
					case 8:		maskPixel = *( maskPtr);	break;
					case 16:	maskPixel = *((unsigned short *) maskPtr);	break;
					case 32:	maskPixel = *((unsigned long  *) maskPtr);	break;
				}

				if ( maskPixel != 0)
				{
					sameCounter = 0;

					switch ( runFlag )
					{
						case kRunSkip:

							runCounter += advance;
							break;
						case kRunDraw:

							*((TokenDataType *) runTokenPtr) = ( kDrawPixelsToken << kTokenShift ) + runCounter;
							PtrAddBytes(dataPtr, ALIGN_PTR(dataPtr));

							runTokenPtr = dataPtr;
							dataPtr += sizeof(TokenDataType);
							runFlag = kRunSkip;
							runCounter = advance;

							break;
						case kRunFill:

							*((TokenDataType *) runTokenPtr) = ( kSingleColorToken << kTokenShift ) + runCounter;
							runTokenPtr += sizeof(TokenDataType);
							if ( depth < 16 )	lastPixel |= lastPixel << 8;
							if ( depth < 32 )	lastPixel |= lastPixel << 16;
							*( ( unsigned long * )runTokenPtr ) = lastPixel;
							runTokenPtr += sizeof(unsigned long);

							runTokenPtr = dataPtr;
							dataPtr += sizeof(TokenDataType);
							runFlag = kRunSkip;
							runCounter = advance;
							break;
						case kRunNone:

							runTokenPtr = dataPtr;
							dataPtr += sizeof(TokenDataType);
							runFlag = kRunSkip;
							runCounter = advance;
							break;
					}
				}
				else
				{
					switch ( depth )
					{
						case 8:		thisPixel = *( sourcePtr);	break;
						case 16:	thisPixel = *((unsigned short *) sourcePtr);	break;
						case 32:	thisPixel = *((unsigned long  *) sourcePtr);	break;
					}

					if ( useDrawSpans && (thisPixel == lastPixel) && (sameCounter + advance >= kMinimumFillRun) )
					{
						switch (runFlag)
						{
							case kRunFill:

								runCounter += advance;
								break;
							case kRunDraw:
								if ( runCounter > sameCounter )
								{
									*( runTokenPtr) =
										( kDrawPixelsToken << kTokenShift ) + (runCounter - sameCounter);

									dataPtr -= sameCounter;
									PtrAddBytes(dataPtr, ALIGN_PTR(dataPtr));
								}
								else
								{
									dataPtr = runTokenPtr;
								}

								runTokenPtr = dataPtr;
								dataPtr += sizeof(TokenDataType) + sizeof(unsigned long);
								runFlag = kRunFill;
								runCounter = sameCounter + advance;
								break;
						}
					}
					else
					{
						if ( sameCounter > 0 && thisPixel == lastPixel )
							sameCounter += advance;
						else
							sameCounter = advance;

						switch ( runFlag )
						{
							case kRunDraw:

								runCounter += advance;
								break;
							case kRunFill:

								*((TokenDataType *) runTokenPtr) = ( kSingleColorToken << kTokenShift ) + runCounter;
								runTokenPtr += sizeof(TokenDataType);

								if ( depth < 16 )	lastPixel |= lastPixel << 8;
								if ( depth < 32 )	lastPixel |= lastPixel << 16;
								*( ( unsigned long * )runTokenPtr ) = lastPixel;
								runTokenPtr += sizeof(unsigned long);

								runTokenPtr = dataPtr;
								dataPtr += sizeof(TokenDataType);
								runFlag = kRunDraw;
								runCounter = advance;
								break;
						case kRunSkip:

								*((TokenDataType *) runTokenPtr) = ( kSkipPixelsToken << kTokenShift ) + runCounter;

								runTokenPtr = dataPtr;
								dataPtr += sizeof(TokenDataType);
								runFlag = kRunDraw;
								runCounter = advance;
								break;
						case kRunNone:

								runTokenPtr = dataPtr;
								dataPtr += sizeof(TokenDataType);
								runFlag = kRunDraw;
								runCounter = advance;
								break;
						}

						switch ( depth )
						{
							case 8:		*( dataPtr) = thisPixel;	dataPtr += 1; break;
							case 16:	*((unsigned short *) dataPtr) = thisPixel;	dataPtr += 2; break;
							case 32:	*((unsigned long  *) dataPtr) = thisPixel;	dataPtr += 4; break;
						}

						lastPixel = thisPixel;
					}
				}

				sourcePtr += advance;
				maskPtr += advance;
			}

			switch ( runFlag )
			{
				case kRunSkip:

					dataPtr = runTokenPtr;
					break;
				case kRunDraw:

					*( (TokenDataType *) runTokenPtr ) = ( kDrawPixelsToken << kTokenShift ) + runCounter;
					PtrAddBytes(dataPtr, ALIGN_PTR(dataPtr));
					break;
				case kRunFill:

					*( ( TokenDataType *)runTokenPtr) = ( kSingleColorToken << kTokenShift ) + runCounter;
					runTokenPtr += sizeof(TokenDataType);

					if ( depth < 16 )	lastPixel |= lastPixel << 8;
					if ( depth < 32 )	lastPixel |= lastPixel << 16;
					*( ( unsigned long *) runTokenPtr) = lastPixel;
					runTokenPtr += sizeof(unsigned long);
					break;
			}

			*( ( TokenDataType * )lineStartPtr ) = ( kLineStartToken << kTokenShift ) +
								( (long) dataPtr - sizeof(TokenDataType) - (long) lineStartPtr);

			sourceStartPtr += rowBytes;
			maskStartPtr += rowBytes;
		}

		*( ( TokenDataType * )dataPtr ) = kEndShapeToken << kTokenShift;
		dataPtr += sizeof(TokenDataType);

		newSize = dataPtr - (unsigned char *) (*tempPixCodeH );

		HUnlock(  tempPixCodeH );

		SetHandleSize(  tempPixCodeH, newSize );
		err = MemError();

		if (err == noErr)
		{
			*newPixCode = (void **) tempPixCodeH;
		}
	}
	else
	{
		err = memFullErr;
	}

	return err;
}
#else
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

    frameHandle = AllocAHandle2(
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

        size = (tokens + 4) - *frameHandle;

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
#endif
