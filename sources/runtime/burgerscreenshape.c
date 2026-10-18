// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
// LINK: after itmarkup; data EVNOVA 0x0057ca90
/* burgerscreenshape.c -- BurgerLib SSScreenShape: the filtering mode, the 2D coordinate switch and the scaled sub-rectangle draw; not linked. Notes: Research/notes/burgerscreenshape.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgergl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LBRect {
    int left;
    int top;
    int right;
    int bottom;
} LBRect;

typedef struct ScreenShape_t {
    void *Data1;
    void *Data2;
    void *Data3;
    void *ActionProc;
    void *DrawProc;
    Word Flags;
    Word Width, Height;
    int XOffset, YOffset;
} ScreenShape_t;

struct Direct3DTexture_t;

enum { kApiOpenGL = 1, kApiDirect3D = 2 };
enum { kFilterOff, kFilterBilinear };

#define kShapeTypeMask   0xf
#define kShapeSoft       0
#define kShapeDirect3D   1
#define kShapeOpenGL     2
#define kShapeMasked     0x10
#define kShapePurgeable  0x40
#define kRecipTableSize  1024

#define kD3DStateRender  0x20808
#define kD3DTextureMag   0x11
#define kD3DTextureMin   0x12
#define kD3DFilterPoint  1
#define kD3DFilterLinear 2

/* SSScreenShape's COMMON data, in the member's order. */
Word ScreenAPI;
Word ScreenTranslucencyMode;
Word ScreenFilteringMode;
Word ScreenShadingMode;
Word ScreenDepthTestMode;
Word ScreenDepthWriteMode;
Word ScreenPerspectiveMode;
Word ScreenWireFrameMode;
Word ScreenBlendMode;
Word ScreenUsing2DCoords;
Word32 ScreenCurrentTexture;
void *ScreenInit;
void *ScreenSolidRect;

/* The member's 8 bytes of .bss, in its order. */
extern "C++" {
static Word ScreenShapeBytesPerPixel;
static Word8 *ScreenITable;
}

Word8 *Direct3DExecInstPtr;
extern float FloatRecipTable[kRecipTableSize];

extern void Direct3DCheckExecBuffer(Word InstCount, Word VertexCount);
extern Word ScreenShapeLoad(ScreenShape_t *Input);
extern void Direct3DTextureDraw2DSubRect(struct Direct3DTexture_t *Input, int x, int y, Word Width,
                                         Word Height, const float *UVPtr);
extern void DrawShapeLowLevelScaledClipped(const LBRect *Dest, Word Width, Word Height, Word Skip,
                                           void *Src);
extern void DrawShapeLowLevelScaledClipped16(const LBRect *Dest, Word Width, Word Height, Word Skip,
                                             void *Src);
extern void DrawMShapeLowLevelScaledClipped(const LBRect *Dest, Word Width, Word Height, Word Skip,
                                            void *Src);
extern void DrawMShapeLowLevelScaledClipped16(const LBRect *Dest, Word Width, Word Height,
                                              Word Skip, void *Src);

#define FloatReciprocal(x) ((x) < kRecipTableSize ? FloatRecipTable[x] : 1.0f / (x))

void ScreenForceFilteringMode(Word Input)
{
    Word32 *Ptr;

    ScreenFilteringMode = Input;
    switch (ScreenAPI) {
    case kApiDirect3D:
        Direct3DCheckExecBuffer(0x14, 0);
        Ptr = (Word32 *)Direct3DExecInstPtr;
        *Ptr++ = kD3DStateRender;
        switch (Input) {
        case kFilterOff:
            Ptr[0] = kD3DTextureMag;
            Ptr[1] = kD3DFilterPoint;
            Ptr[2] = kD3DTextureMin;
            Ptr[3] = kD3DFilterPoint;
            Ptr += 4;
            break;
        case kFilterBilinear:
            Ptr[0] = kD3DTextureMag;
            Ptr[1] = kD3DFilterLinear;
            Ptr[2] = kD3DTextureMin;
            Ptr[3] = kD3DFilterLinear;
            Ptr += 4;
            break;
        }
        Direct3DExecInstPtr = (Word8 *)Ptr;
        /* BUG: falls into the OpenGL arm */
    case kApiOpenGL:
        switch (Input) {
        case kFilterOff:
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
        case kFilterBilinear:
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
        }
    }
}

void ScreenForceUse2DCoords(Word Input)
{
    switch (ScreenAPI) {
    case kApiOpenGL:
        if (Input) {
            if (!ScreenUsing2DCoords) {
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadIdentity();
                glOrtho(0.0, ScreenWidth, ScreenHeight, 0.0, -1.0, 1.0);
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();
            }
        } else {
            if (ScreenUsing2DCoords) {
                glMatrixMode(GL_PROJECTION);
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                glPopMatrix();
            }
        }
    }
    ScreenUsing2DCoords = Input;
}

void ScreenShapeDrawScaledSubRect(ScreenShape_t *Input, const LBRect *DestRect, const LBRect *SrcRect)
{
    float UV[4];
    float XScale, YScale;
    void **Handle;
    char *SrcPtr;
    Word Bpp;
    Word Stride;
    Word Width;
    Word Height;
    Word Skip;

    if (ScreenShapeLoad(Input))
        return;
    switch (Input->Flags & kShapeTypeMask) {
    case kShapeSoft:
        Handle = (void **)Input->Data1;
        LockVideoPage();
        Bpp = (VideoColorDepth + 7) >> 3;
        Stride = Input->Width * Bpp;
        SrcPtr = (char *)*Handle + Bpp * SrcRect->left + Stride * SrcRect->top;
        Width = SrcRect->right - SrcRect->left;
        Height = SrcRect->bottom - SrcRect->top;
        Skip = Stride - Width * Bpp;
        if (Input->Flags & kShapeMasked) {
            if (VideoColorDepth == 8)
                DrawMShapeLowLevelScaledClipped(DestRect, Width, Height, Skip, SrcPtr);
            else
                DrawMShapeLowLevelScaledClipped16(DestRect, Width, Height, Skip, SrcPtr);
        } else {
            if (VideoColorDepth == 8)
                DrawShapeLowLevelScaledClipped(DestRect, Width, Height, Skip, SrcPtr);
            else
                DrawShapeLowLevelScaledClipped16(DestRect, Width, Height, Skip, SrcPtr);
        }
        if (Input->Flags & kShapePurgeable)
            SetHandlePurgeFlag(Handle, 1);
        break;
    case kShapeDirect3D:
        XScale = FloatReciprocal(Input->Width);
        YScale = FloatReciprocal(Input->Height);
        UV[0] = SrcRect->left * XScale;
        UV[2] = SrcRect->right * XScale;
        UV[1] = SrcRect->top * YScale;
        UV[3] = SrcRect->bottom * YScale;
        Direct3DTextureDraw2DSubRect((struct Direct3DTexture_t *)Input, DestRect->left, DestRect->top,
                                     DestRect->right - DestRect->left, DestRect->bottom - DestRect->top,
                                     UV);
        break;
    case kShapeOpenGL:
        XScale = FloatReciprocal(Input->Width);
        YScale = FloatReciprocal(Input->Height);
        UV[0] = SrcRect->left * XScale;
        UV[2] = SrcRect->right * XScale;
        UV[1] = SrcRect->top * YScale;
        UV[3] = SrcRect->bottom * YScale;
        OpenGLTextureDraw2DSubRect((Word)Input->Data1, DestRect->left, DestRect->top,
                                   DestRect->right - DestRect->left, DestRect->bottom - DestRect->top,
                                   UV);
        break;
    }
}

#ifdef __cplusplus
}
#endif
