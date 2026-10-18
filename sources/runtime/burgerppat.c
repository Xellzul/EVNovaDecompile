// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgerppat.c -- BurgerLib ImImageParsePPat: a Mac 'ppat' resource into an 8-bit Image_t. Notes: Research/notes/burgerppat.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kFullColorPattern   1
#define kMinPatMap          0x20
#define kMinPatData         0x54
#define kPatFlagMask        0xe000
#define kPatRowBytesMask    0x1fff

static __inline Word16 SwapShort(Word16 v)
{
    register Word16 t;

    t = v;
    asm { ror t, 8 }
    return t;
}

static __inline Word32 SwapLong(Word32 v)
{
    register Word32 t;

    t = v;
    asm { bswap t }
    return t;
}

/* ImageParsePPat (BurgerLib ImImageParsePPat): 0 on success, 1 (image freed) on failure.
   STATE: MATCH 100.0%, exe 191 / obj 191. OPEN: none */
// FUNCTION: EVNOVA 0x004fdf40
Word ImageParsePPat(Image_t *ImagePtr, const Word8 *InputPtr)
{
    Word32 PixelDataOffset;
    Word32 ColorTableOffset;
    Word RowBytes;
    Word32 PixMapOffset;
    Word8 *DestPtr;

    FastMemSet(ImagePtr, 0, sizeof(Image_t));
    {
        Word16 Version;
        const Word32 *HeaderPtr;

        HeaderPtr = (const Word32 *)InputPtr;
        Version = SwapShort(((const Word16 *)HeaderPtr)[0]);
        PixMapOffset = SwapLong(HeaderPtr[1]);
        PixelDataOffset = SwapLong(HeaderPtr[2]);
        if (Version != kFullColorPattern || PixMapOffset < kMinPatMap ||
            PixelDataOffset < kMinPatData) {
            NonFatal("Invalid ppat header\n");
            goto Abort;
        }
    }
    {
        const Word16 *PixMapHeaderPtr;

        PixMapHeaderPtr = (const Word16 *)(InputPtr + PixMapOffset);
        ImagePtr->Width = (short)SwapShort(PixMapHeaderPtr[6]) - (short)SwapShort(PixMapHeaderPtr[4]);
        ImagePtr->Height = (short)SwapShort(PixMapHeaderPtr[5]) - (short)SwapShort(PixMapHeaderPtr[3]);
        ColorTableOffset = SwapLong(*(const Word32 *)(PixMapHeaderPtr + 0x16));
        RowBytes = SwapShort(PixMapHeaderPtr[2]);
        if (PixMapHeaderPtr[7] != 0 || PixMapHeaderPtr[8] != 0 ||
            (RowBytes & kPatFlagMask) != kPixMapFlag) {
            NonFatal("Invalid / unsupported pixmapheader header\n");
            goto Abort;
        }
        if (SwapShort(PixMapHeaderPtr[0x11]) != 8) {
            NonFatal("ppats must be 8 bits per pixel!\n");
            goto Abort;
        }
    }
    ImagePtr->DataType = 8;
    ImagePtr->RowBytes = ImagePtr->Width;
    if (ImagePtr->Width == 0) {
        NonFatal("Width can't be zero.");
        goto Abort;
    }
    if (ImagePtr->Height == 0) {
        NonFatal("Height can't be zero.");
        goto Abort;
    }
    DestPtr = (Word8 *)AllocAPointer(ImagePtr->Width * ImagePtr->Height);
    if (DestPtr == 0)
        goto Abort;
    ImagePtr->ImagePtr = DestPtr;
    if (ColorTableOffset == 0) {
        NonFatal("No global palette in ppat file\n");
        goto Abort;
    }
    {
        Word TempHeight;
        const Word8 *SrcPtr;

        SrcPtr = InputPtr + PixelDataOffset;
        TempHeight = ImagePtr->Height;
        RowBytes &= kPatRowBytesMask;
        do {
            memcpy(DestPtr, SrcPtr, ImagePtr->Width);
            DestPtr += ImagePtr->Width;
            SrcPtr += RowBytes;
        } while (--TempHeight);
    }
    {
        const Word16 *ColorTableHeaderPtr;
        Word Count;
        const Word16 *ColorEntryPtr;

        ColorTableHeaderPtr = (const Word16 *)(InputPtr + ColorTableOffset);
        Count = (short)SwapShort(ColorTableHeaderPtr[3]) + 1;
        DestPtr = (Word8 *)AllocAPointerClear(kPaletteBytes);
        if (DestPtr == 0)
            goto Abort;
        ImagePtr->PalettePtr = DestPtr;
        ColorEntryPtr = ColorTableHeaderPtr + 4;
        do {
            Word Index;

            Index = (short)SwapShort(ColorEntryPtr[0]);
            if (Index < kPaletteEntries) {
                Word8 *FooPtr;

                FooPtr = DestPtr + Index * 3;
                FooPtr[0] = SwapShort(ColorEntryPtr[1]) >> 8;
                FooPtr[1] = SwapShort(ColorEntryPtr[2]) >> 8;
                FooPtr[2] = SwapShort(ColorEntryPtr[3]) >> 8;
            }
            ColorEntryPtr += 4;
        } while (--Count);
    }
    return 0;
Abort:
    ImageDestroy(ImagePtr);
    return 1;
}

#ifdef __cplusplus
}
#endif
