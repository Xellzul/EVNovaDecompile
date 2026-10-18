// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerpict.c -- BurgerLib's ImImageParsePict: a QuickDraw PICT, PackBits rows or a QuickTime-compressed picture, decoded into an Image_t. Notes: Research/notes/burgerpict.md */

#include <string.h>

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "macport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* QuickTime's own records. The port's PixMap (macport.h) is a different layout, so a
   QuickTime GWorld is only ever touched through these. */
#pragma options align=mac68k
typedef struct QTPixMap {
    char *baseAddr;                              /* +0x00 */
    short rowBytes;                              /* +0x04 top two bits are flags */
    Rect bounds;                                 /* +0x06 */
    short pmVersion;                             /* +0x0e */
    short packType;                              /* +0x10 */
    long packSize;                               /* +0x12 */
    long hRes;                                   /* +0x16 */
    long vRes;                                   /* +0x1a */
    short pixelType;                             /* +0x1e */
    short pixelSize;                             /* +0x20 */
    short cmpCount;                              /* +0x22 */
    short cmpSize;                               /* +0x24 */
    long planeBytes;                             /* +0x26 */
    void *pmTable;                               /* +0x2a */
    long pmReserved;                             /* +0x2e */
} QTPixMap, **QTPixMapHandle;

typedef struct QTImageDescription {
    long idSize;                                 /* +0x00 */
    unsigned long cType;                         /* +0x04 */
    long resvd1;                                 /* +0x08 */
    short resvd2;                                /* +0x0c */
    short dataRefIndex;                          /* +0x0e */
    short version;                               /* +0x10 */
    short revisionLevel;                         /* +0x12 */
    long vendor;                                 /* +0x14 */
    long temporalQuality;                        /* +0x18 */
    long spatialQuality;                         /* +0x1c */
    short width;                                 /* +0x20 */
    short height;                                /* +0x22 */
    long hRes;                                   /* +0x24 */
    long vRes;                                   /* +0x28 */
    long dataSize;                               /* +0x2c */
    short frameCount;                            /* +0x30 */
    unsigned char name[32];                      /* +0x32 */
    short depth;                                 /* +0x52 */
    short clutID;                                /* +0x54 */
} QTImageDescription, **QTImageDescriptionHandle;
#pragma options align=reset

typedef struct QTGWorld *QTGWorldPtr;            /* opaque: QuickTime owns it */
typedef struct QTGDevice **QTGDHandle;

/* the PICT PixMap record after its rowBytes and bounds, host order */
typedef struct PICTPixmap_t {
    short version;                               /* +0x00 */
    short pack_type;                             /* +0x02 */
    long pack_size;                              /* +0x04 */
    long horizontal_resolution;                  /* +0x08 */
    long vertical_resolution;                    /* +0x0c */
    short pixel_type;                            /* +0x10 */
    short bits_per_pixel;                        /* +0x12 */
    short component_count;                       /* +0x14 */
    short component_size;                        /* +0x16 */
    long plane_bytes;                            /* +0x18 */
    long table;                                  /* +0x1c */
    long reserved;                               /* +0x20 */
} PICTPixmap_t;

/* QuickTime selector stubs, qtmlClient.lib. The last three are QuickTime's GetPixBaseAddr,
   GetGWorldDevice and GetGWorldPixMap, linked under address names (tools/qtorder.tsv); those
   Toolbox names belong to the port's QuickDraw code. */
extern Handle NewHandle(long size);                                  /* 0x004f1450 */
extern void DisposeHandle(Handle h);                                 /* 0x004f1490 */
extern void MacSetRect(Rect *r, short left, short top, short right, short bottom);
                                                                     /* 0x004f2a10 */
extern short DecompressImage(char *data, QTImageDescriptionHandle desc, QTPixMapHandle dst,
                             const Rect *srcRect, const Rect *dstRect, short mode,
                             void *mask);                            /* 0x004fc270 */
extern short NewImageGWorld(QTGWorldPtr *gworld, QTImageDescriptionHandle desc,
                            unsigned long flags);                    /* 0x004fc290 */
extern unsigned char LockPixels(QTPixMapHandle pm);                  /* 0x004fc2f0 */
extern void GetGWorld(QTGWorldPtr *port, QTGDHandle *device);        /* 0x004fc330 */
extern void SetGWorld(QTGWorldPtr port, QTGDHandle device);          /* 0x004fc350 */
extern char *FUN_004fc370(QTPixMapHandle pm);                        /* GetPixBaseAddr */
extern QTGDHandle FUN_004fc390(QTGWorldPtr gworld);                  /* GetGWorldDevice */
extern QTPixMapHandle FUN_004fc3b0(QTGWorldPtr gworld);              /* GetGWorldPixMap */

#define kPixMapRowFlag      0x8000      /* rowBytes: a PixMap, not a BitMap */
#define kDeviceIndexFlag    0x8000      /* ctFlags: entries are in order, ignore their values */
#define kQTRowBytesMask     0x3fff
#define kVariableLength     -1

/* PICT opcodes this decoder acts on */
#define opClip              0x0001
#define opPicVersion        0x0011
#define opBkPixPat          0x0012
#define opFillPixPat        0x0014
#define opRGBBkCol          0x001b
#define opLongText          0x0028
#define opFramePoly         0x0070
#define opInvertPoly        0x0077
#define opBitsRect          0x0090
#define opBitsRgn           0x0091
#define opPackBitsRect      0x0098
#define opPackBitsRgn       0x0099
#define opDirectBitsRect    0x009a
#define opDirectBitsRgn     0x009b
#define opLongComment       0x00a1
#define opEndPic            0x00ff
#define opHeaderOp          0x0c00
#define opCompressedQuickTime   0x8200
#define opUncompressedQuickTime 0x8201

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

#define BigShort(p)     SwapShort(*(const Word16 *)(p))
#define BigLong(p)      SwapLong(*(const Word32 *)(p))

/* the data bytes after each opcode up to 0xa1; kVariableLength marks the ones whose
   data opens with its own length word. The archive's names: codes, MacStandardPalette */
static const int codes[0xa2] = {
    0, 0, 8, 2, 2, 2, 4, 4, 2, 8, 8, 4, 4, 2, 4, 4,
    8, 1, 0, 0, 0, 2, 2, 0, 0, 0, 6, 6, 0, 6, 0, 6,
    8, 4, 6, 2, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1,
    8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0,
    12, 12, 12, 12, 12, 12, 12, 12, 4, 4, 4, 4, 4, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1,
    2, 0
};

/* a BitMap's two colours: index 0 white, everything else black */
static const Word8 MacStandardPalette[256 * 3] = {
    0xff, 0xff, 0xff
};

/* ReadPixmap (archive ?ReadPixmap). STATE: MATCH 100.0%, exe 40 / obj 40, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fca30
const Word8 *ReadPixmap(PICTPixmap_t *Output, const Word8 *Input)
{
    Output->version = BigShort(Input);
    Output->pack_type = BigShort(Input + 2);
    Output->pack_size = BigLong(Input + 4);
    Output->horizontal_resolution = BigLong(Input + 8);
    Output->vertical_resolution = BigLong(Input + 12);
    Output->pixel_type = BigShort(Input + 16);
    Output->bits_per_pixel = BigShort(Input + 18);
    Output->component_count = BigShort(Input + 20);
    Output->component_size = BigShort(Input + 22);
    Output->plane_bytes = BigLong(Input + 24);
    Output->table = BigLong(Input + 28);
    Output->reserved = BigLong(Input + 32);
    return Input + 36;
}

/* ExpandBuffer (archive ?ExpandBuffer). STATE: MATCH 100.0%, exe 109 / obj 109, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fcac0
const Word8 *ExpandBuffer(Word8 *Output, const Word8 *Input, Word32 *LengthPtr, Word Depth)
{
    Word32 Count;
    Word8 *WorkPtr;
    Word Value;

    Count = *LengthPtr;
    if (Count) {
        switch (Depth) {
        case 8:
        case 16:
        case 24:
        case 32:
            return Input;
        case 4:
            *LengthPtr = Count * 2;
            WorkPtr = Output;
            do {
                Value = *Input++;
                WorkPtr[0] = Value >> 4;
                WorkPtr[1] = Value & 0xf;
                WorkPtr += 2;
            } while (--Count);
            break;
        case 2:
            *LengthPtr = Count * 4;
            WorkPtr = Output;
            do {
                Value = *Input++;
                WorkPtr[0] = (Value >> 6) & 3;
                WorkPtr[1] = (Value >> 4) & 3;
                WorkPtr[2] = (Value >> 2) & 3;
                WorkPtr[3] = Value & 3;
                WorkPtr += 4;
            } while (--Count);
            break;
        case 1:
            *LengthPtr = Count * 8;
            WorkPtr = Output;
            do {
                Value = *Input++;
                WorkPtr[0] = (Value >> 7) & 1;
                WorkPtr[1] = (Value >> 6) & 1;
                WorkPtr[2] = (Value >> 5) & 1;
                WorkPtr[3] = (Value >> 4) & 1;
                WorkPtr[4] = (Value >> 3) & 1;
                WorkPtr[5] = (Value >> 2) & 1;
                WorkPtr[6] = (Value >> 1) & 1;
                WorkPtr[7] = Value & 1;
                WorkPtr += 8;
            } while (--Count);
            break;
        }
    }
    return Output;
}

/* DecodeImage (archive ?DecodeImage). STATE: MATCH 100.0%, exe 317 / obj 317,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fcc00
Word8 *DecodeImage(const Word8 *Input, Image_t *ImagePtr, Word32 RowBytes, Word Depth)
{
    const Word8 *SrcPtr;
    Word Repeat;
    Word8 *Output;
    Word8 *ExpandPtr;
    Word RowSize;
    Word PackedSize;
    Word BufferSize;
    Word32 Run;
    Word32 Length;
    Word8 *WorkPtr;
    Word Index;
    Word8 *PackedPtr;
    Word RunSize;
    Word32 Code;
    Word Count;
    Word8 *DestPtr;
    Word Height;
    Word32 RawLength;
    Word Remaining;
    Word Swap;

    RowBytes &= 0x7fff;
    RowSize = ImagePtr->Width;
    RunSize = 1;
    if (Depth == 16) {
        RowSize <<= 1;
        RunSize = 2;
    } else if (Depth == 32) {
        RowSize <<= 2;
    } else if (Depth == 24) {
        RowSize *= 3;
    }
    if (!RowBytes) {
        RowBytes = RowSize;
    }
    BufferSize = ImagePtr->Width << 2;
    Output = (Word8 *)AllocAPointerClear(RowSize * ImagePtr->Height);
    if (Output) {
        PackedPtr = (Word8 *)AllocAPointer(BufferSize * 2);
        if (PackedPtr) {
            ExpandPtr = PackedPtr + BufferSize;
            Height = ImagePtr->Height;
            DestPtr = Output;
            do {
                if (RowBytes < 8) {
                    RawLength = RowBytes;
                    SrcPtr = ExpandBuffer(ExpandPtr, Input, &RawLength, Depth);
                    memcpy(DestPtr, SrcPtr, RawLength);
                    Input += RowBytes;
                } else {
                    PackedSize = *Input++;
                    if (RowBytes > 250) {
                        PackedSize = (PackedSize << 8) + *Input++;
                    }
                    if (PackedSize > BufferSize) {
                        NonFatal("Buffer overrun");
                        DeallocAPointer(Output);
                        Output = NULL;
                        break;
                    }
                    if (PackedSize) {
                        WorkPtr = DestPtr;
                        memcpy(PackedPtr, Input, PackedSize);
                        Input += PackedSize;
                        Index = 0;
                        Remaining = RowSize;
                        do {
                            Code = PackedPtr[Index];
                            if (!(Code & 0x80)) {
                                Run = (Code + 1) * RunSize;
                                Length = Run;
                                SrcPtr = ExpandBuffer(ExpandPtr, PackedPtr + Index + 1, &Length, Depth);
                                if (Remaining < Length) {
                                    Length = Remaining;
                                }
                                Remaining -= Length;
                                memcpy(WorkPtr, SrcPtr, Length);
                                WorkPtr += Length;
                                Index += Run + 1;
                            } else {
                                Repeat = (Code ^ 0xff) + 2;
                                Length = RunSize;
                                SrcPtr = ExpandBuffer(ExpandPtr, PackedPtr + Index + 1, &Length, Depth);
                                do {
                                    if (Remaining < Length) {
                                        Length = Remaining;
                                    }
                                    Remaining -= Length;
                                    memcpy(WorkPtr, SrcPtr, Length);
                                    WorkPtr += Length;
                                } while (--Repeat);
                                Index += RunSize + 1;
                            }
                        } while (Index < PackedSize);
                    }
                }
                if (Depth == 24) {
                    Word Width2;
                    Word Width;
                    const Word8 *Src1;

                    memcpy(ExpandPtr, DestPtr, RowSize);
                    Src1 = ExpandPtr;
                    WorkPtr = DestPtr;
                    Width = ImagePtr->Width;
                    if (Width) {
                        Width2 = Width * 2;
                        Count = Width;
                        do {
                            WorkPtr[0] = Src1[0];
                            WorkPtr[1] = Src1[Width];
                            WorkPtr[2] = Src1[Width2];
                            WorkPtr += 3;
                            ++Src1;
                        } while (--Count);
                    }
                } else if (Depth == 32) {
                    Word Width2;
                    Word Width;
                    const Word8 *Src1;

                    memcpy(ExpandPtr, DestPtr + ImagePtr->Width, RowSize - ImagePtr->Width);   /* past the alpha plane */
                    Src1 = ExpandPtr;
                    WorkPtr = DestPtr;
                    Width = ImagePtr->Width;
                    if (Width) {
                        Width2 = Width * 2;
                        Count = Width;
                        do {
                            WorkPtr[0] = Src1[0];
                            WorkPtr[1] = Src1[Width];
                            WorkPtr[2] = Src1[Width2];
                            WorkPtr += 3;
                            ++Src1;
                        } while (--Count);
                    }
                    DestPtr -= ImagePtr->Width;
                } else if (Depth == 16) {
                    Swap = ImagePtr->Width;
                    if (Swap) {
                        WorkPtr = DestPtr;
                        do {
                            *(Word16 *)WorkPtr = SwapShort(*(Word16 *)WorkPtr);
                            WorkPtr += 2;
                        } while (--Swap);
                    }
                }
                DestPtr += RowSize;
            } while (--Height);
            DeallocAPointer(PackedPtr);
            return Output;
        }
        DeallocAPointer(PackedPtr);
    }
    return NULL;
}

/* ImageParsePict. STATE: MATCH 100.0%, exe 735 / obj 735, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fd0a0
Word ImageParsePict(Image_t *ImagePtr, const Word8 *InputPtr)
{
    PICTPixmap_t Pixmap;
    Rect Bounds;
    Word Version;
    Word Opcode;
    Word Length;
    Word RowBytes;
    Word Flags;
    Word Count;
    Word Index;
    Word i;
    Word8 *WorkPtr;
    const Word8 *DataPtr;
    QTImageDescriptionHandle Desc;
    QTImageDescription *DescPtr;
    QTGWorldPtr GWorld;
    QTGWorldPtr SavePort;
    QTGDHandle SaveDevice;
    QTPixMapHandle PixMap;
    Word Parity;
    Rect PixBounds;
    Rect SrcRect;
    Rect MatteRect;
    Word32 MatteSize;
    Word32 MaskSize;
    Word Width;
    Word Height;
    Word Kind;
    Rect DestRect;
    short Err;
    Word32 Size;

    FastMemSet(ImagePtr, 0, sizeof(Image_t));
    Parity = (Word)InputPtr & 1;        /* opcodes are word aligned to the picture's start */
    ImagePtr->Width = BigShort(InputPtr + 8) - BigShort(InputPtr + 4);
    ImagePtr->Height = BigShort(InputPtr + 6) - BigShort(InputPtr + 2);
    InputPtr += 10;                     /* picSize, picFrame */
    do {
        Opcode = *InputPtr++;
    } while (!Opcode);
    if (Opcode != opPicVersion) {
        NonFatal("0x11 not found in PICT image. Probably bad data.\n");
        goto Abort;
    }
    Version = InputPtr[0];
    if (Version == 2) {
        if (InputPtr[1] != 0xff) {
            NonFatal("0x02,0xFF not found in header. Bad version?\n");
            goto Abort;
        }
        ++InputPtr;
    } else if (Version != 1) {
        NonFatal("Bad version\n");
        goto Abort;
    }
    ++InputPtr;

    Opcode = 0;
    for (;;) {
        if (Version == 1 || ((Word)InputPtr & 1) != Parity) {
            Opcode = *InputPtr++;
        }
        if (Version == 2) {
            Opcode = BigShort(InputPtr);
            InputPtr += 2;
        }
        if (Opcode <= opLongComment) {
            switch (Opcode) {
            case opClip:
                Length = BigShort(InputPtr);
                if (Length == 10 && !(InputPtr[2] & 0x80) && !(InputPtr[4] & 0x80)) {
                    Width = BigShort(InputPtr + 8) - BigShort(InputPtr + 4);
                    Height = BigShort(InputPtr + 6) - BigShort(InputPtr + 2);
                    ImagePtr->Width = Width;
                    ImagePtr->Height = Height;
                }
                InputPtr += Length;
                break;
            case opBkPixPat:
            case 0x13:
            case opFillPixPat:
                NonFatal("Found unsupported pattern opcode 0x%02X\n", Opcode);
                goto Abort;
            case opRGBBkCol:
                InputPtr += 6;
                break;
            case opFramePoly:
            case 0x71:
            case 0x72:
            case 0x73:
            case 0x74:
            case 0x75:
            case 0x76:
            case opInvertPoly:
                InputPtr += BigShort(InputPtr);
                break;
            case opBitsRect:
            case opBitsRgn:
            case opPackBitsRect:
            case opPackBitsRgn:
            case opDirectBitsRect:
            case opDirectBitsRgn:
                if (Opcode != opDirectBitsRect && Opcode != opDirectBitsRgn) {
                    RowBytes = BigShort(InputPtr);
                    InputPtr += 2;
                } else {
                    RowBytes = BigShort(InputPtr + 4);  /* past the baseAddr */
                    InputPtr += 6;
                }
                PixBounds.top = BigShort(InputPtr);
                PixBounds.left = BigShort(InputPtr + 2);
                PixBounds.bottom = BigShort(InputPtr + 4);
                PixBounds.right = BigShort(InputPtr + 6);
                InputPtr += 8;
                if (!ImagePtr) {
                    goto Abort;
                }
                if (Opcode == opDirectBitsRect || Opcode == opDirectBitsRgn ||
                    (RowBytes & kPixMapRowFlag)) {
                    InputPtr = ReadPixmap(&Pixmap, InputPtr);
                    if (Pixmap.bits_per_pixel == 32 && Pixmap.component_count != 4) {
                        Pixmap.bits_per_pixel = 24;
                    }
                }
                if (Opcode != opDirectBitsRect && Opcode != opDirectBitsRgn) {
                    if (!ImagePtr->PalettePtr) {
                        ImagePtr->PalettePtr = (Word8 *)AllocAPointerClear(sizeof(MacStandardPalette));
                        memcpy(ImagePtr->PalettePtr, MacStandardPalette, sizeof(MacStandardPalette));
                    }
                    if (RowBytes & kPixMapRowFlag) {
                        Flags = BigShort(InputPtr + 4);
                        Count = BigShort(InputPtr + 6) + 1;
                        InputPtr += 8;                  /* ctSeed, ctFlags, ctSize */
                        i = 0;
                        Flags &= kDeviceIndexFlag;
                        do {
                            Index = BigShort(InputPtr);
                            if (Flags) {
                                Index = i;
                            }
                            if (Index < 256) {
                                WorkPtr = ImagePtr->PalettePtr + Index * 3;
                                WorkPtr[0] = InputPtr[2];
                                WorkPtr[1] = InputPtr[4];
                                WorkPtr[2] = InputPtr[6];
                            }
                            InputPtr += 8;
                        } while (++i < Count);
                    }
                }
                SrcRect.top = BigShort(InputPtr);
                SrcRect.left = BigShort(InputPtr + 2);
                SrcRect.bottom = BigShort(InputPtr + 4);
                SrcRect.right = BigShort(InputPtr + 6);
                DestRect.top = BigShort(InputPtr + 8);
                DestRect.left = BigShort(InputPtr + 10);
                DestRect.bottom = BigShort(InputPtr + 12);
                DestRect.right = BigShort(InputPtr + 14);
                InputPtr += 18;                         /* srcRect, dstRect, mode */
                if (Opcode == opBitsRgn || Opcode == opPackBitsRgn || Opcode == opDirectBitsRgn) {
                    InputPtr += (short)BigShort(InputPtr);  /* maskRgn */
                }
                if (Opcode != opDirectBitsRect && Opcode != opDirectBitsRgn &&
                    !(RowBytes & kPixMapRowFlag)) {
                    Pixmap.bits_per_pixel = 1;
                }
                ImagePtr->ImagePtr = DecodeImage(InputPtr, ImagePtr, RowBytes, Pixmap.bits_per_pixel);
                if (Pixmap.bits_per_pixel <= 8) {
                    ImagePtr->RowBytes = ImagePtr->Width;
                    ImagePtr->DataType = IMAGE8_PAL;
                    if (!ImagePtr->PalettePtr) {
                        ImagePtr->PalettePtr = (Word8 *)AllocAPointer(sizeof(MacStandardPalette));
                        memcpy(ImagePtr->PalettePtr, MacStandardPalette, sizeof(MacStandardPalette));
                    }
                } else if (Pixmap.bits_per_pixel == 16) {
                    ImagePtr->RowBytes = ImagePtr->Width * 2;
                    ImagePtr->DataType = IMAGE555;
                } else {
                    ImagePtr->RowBytes = ImagePtr->Width * 3;
                    ImagePtr->DataType = IMAGE888;
                }
                return 0;
            case opLongComment:
                Kind = BigShort(InputPtr);
                Length = BigShort(InputPtr + 2);
                InputPtr += 4;
                InputPtr += Length;
                break;
            default:
                if (codes[Opcode] == kVariableLength) {
                    InputPtr += 2;
                } else {
                    InputPtr += codes[Opcode];
                }
                break;
            }
        }

        if (Opcode == opHeaderOp) {
            InputPtr += 24;
        } else if (Opcode == opLongText) {
            InputPtr += 4;                              /* the pen location */
            InputPtr += InputPtr[0] + 1;
        } else if ((Opcode >= 0xb0 && Opcode <= 0xcf) || (Opcode >= 0x8000 && Opcode <= 0x80ff)) {
            /* reserved, no data */
        } else if (Opcode == opCompressedQuickTime) {
            Size = BigLong(InputPtr);
            InputPtr += 4;
            /* matrix at +2, the matte (size +0x26, rect +0x34), the mask (size +0x40),
               then the matte and mask data and the ImageDescription */
            DataPtr = InputPtr;
            MatteSize = BigLong(InputPtr + 0x26);
            MatteRect.top = BigShort(InputPtr + 0x34);
            MatteRect.left = BigShort(InputPtr + 0x36);
            MatteRect.bottom = BigShort(InputPtr + 0x38);
            MatteRect.right = BigShort(InputPtr + 0x3a);
            MaskSize = BigLong(InputPtr + 0x40);
            DataPtr += 0x44;
            DataPtr += MatteSize + MaskSize;
            if (BigLong(DataPtr) == sizeof(QTImageDescription)) {
                Desc = (QTImageDescriptionHandle)NewHandle(sizeof(QTImageDescription));
                HLock((Handle)Desc);
                DescPtr = *Desc;
                *DescPtr = *(const QTImageDescription *)DataPtr;
                DescPtr->idSize = SwapLong(DescPtr->idSize);
                DescPtr->cType = SwapLong(DescPtr->cType);
                DescPtr->resvd1 = SwapLong(DescPtr->resvd1);
                DescPtr->resvd2 = SwapShort(DescPtr->resvd2);
                DescPtr->dataRefIndex = SwapShort(DescPtr->dataRefIndex);
                DescPtr->version = SwapShort(DescPtr->version);
                DescPtr->revisionLevel = SwapShort(DescPtr->revisionLevel);
                DescPtr->vendor = SwapLong(DescPtr->vendor);
                DescPtr->temporalQuality = SwapLong(DescPtr->temporalQuality);
                DescPtr->spatialQuality = SwapLong(DescPtr->spatialQuality);
                DescPtr->width = SwapShort(DescPtr->width);
                DescPtr->height = SwapShort(DescPtr->height);
                DescPtr->hRes = SwapLong(DescPtr->hRes);
                DescPtr->vRes = SwapLong(DescPtr->vRes);
                DescPtr->dataSize = SwapLong(DescPtr->dataSize);
                DescPtr->frameCount = SwapShort(DescPtr->frameCount);
                DescPtr->depth = SwapShort(DescPtr->depth);
                DescPtr->clutID = SwapShort(DescPtr->clutID);
                DataPtr += (*Desc)->idSize;

                GWorld = NULL;
                Err = NewImageGWorld(&GWorld, Desc, noNewDevice);
                if (!Err) {
                    PixMap = FUN_004fc3b0(GWorld);
                    LockPixels(PixMap);
                    MacSetRect(&Bounds, 0, 0, (*Desc)->width, (*Desc)->height);
                    GetGWorld(&SavePort, &SaveDevice);
                    SetGWorld(GWorld, FUN_004fc390(GWorld));
                    Err = DecompressImage((char *)DataPtr, Desc, PixMap, &Bounds, &Bounds, srcCopy,
                                          NULL);
                    SetGWorld(SavePort, SaveDevice);
                }
                if (!Err) {
                    ImagePtr->RowBytes = (*PixMap)->rowBytes & kQTRowBytesMask;
                    Size = ImagePtr->RowBytes * ImagePtr->Height;
                    ImagePtr->ImagePtr = (Word8 *)AllocAPointer(Size);
                    memcpy(ImagePtr->ImagePtr, FUN_004fc370(PixMap), Size);
                    switch ((*PixMap)->pixelSize) {
                    case 8:
                        ImagePtr->DataType = IMAGE8_PAL;
                        break;
                    case 16:
                        ImagePtr->DataType = IMAGE555;
                        break;
                    case 24:
                        ImagePtr->DataType = IMAGE888;
                        break;
                    case 32:
                        ImagePtr->DataType = IMAGE8888;
                        break;
                    default:
                        NonFatal("Unsuppored PICT bit depth %d.\n", (*PixMap)->pixelSize);
                        break;
                    }
                    if ((*PixMap)->pixelSize == 16) {
                        int Rows;
                        int Skip;
                        Word16 *SwapPtr;

                        Rows = ImagePtr->Height;
                        Skip = ImagePtr->RowBytes - ImagePtr->Width * 2;
                        SwapPtr = (Word16 *)ImagePtr->ImagePtr;
                        do {
                            int Pixels;

                            Pixels = ImagePtr->Width;
                            do {
                                *SwapPtr = SwapShort(*SwapPtr);
                                ++SwapPtr;
                            } while (--Pixels);
                            SwapPtr = (Word16 *)((Word8 *)SwapPtr + Skip);
                        } while (--Rows);
                    }
                    if (ImagePtr->DataType == IMAGE8_PAL && !ImagePtr->PalettePtr) {
                        ImagePtr->PalettePtr = (Word8 *)AllocAPointer(sizeof(MacStandardPalette));
                        memcpy(ImagePtr->PalettePtr, MacStandardPalette, sizeof(MacStandardPalette));
                    }
                } else {
                    NonFatal("Unable to decompress QT PICT.\n");
                }
                if (GWorld) {
                    FUN_004fc310(GWorld);
                }
                if (Desc) {
                    DisposeHandle((Handle)Desc);
                }
                return 0;
            }
            InputPtr += Size;
        } else if (Opcode == opUncompressedQuickTime) {
            InputPtr += BigLong(InputPtr) + 4;
        } else if (Opcode == opEndPic || Opcode == 0xffff) {
            break;
        } else if ((Opcode >= 0xd0 && Opcode <= 0xfe) || (Opcode >= 0x8100 && Opcode <= 0xffff)) {
            InputPtr += BigShort(InputPtr);
        } else if (Opcode >= 0x100 && Opcode <= 0x7fff) {
            InputPtr += (Opcode >> 7) & 0xff;
        }
    }
Abort:
    ImageDestroy(ImagePtr);
    return 1;
}

#ifdef __cplusplus
}
#endif
