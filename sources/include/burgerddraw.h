/* burgerddraw.h -- just enough DirectDraw to say what 0x004ecbd0 calls.
 * The vtable offsets are the ones the exe actually uses, read off its own
 * `call dword ptr [reg + N]' sites; the gaps between them are padding and are
 * NOT claims about what lives there.  DirectDraw's headers are not on this
 * machine and nothing here is copied from them -- the layout is recovered
 * from the call sites and from DDSURFACEDESC's own field offsets, which the
 * exe pins by storing 0x6c at +0 and the caps at +0x68.
 */

#ifndef BURGERDDRAW_H
#define BURGERDDRAW_H

#include "burgertypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DDPixelFormat {                   /* 32 bytes, desc +0x48 */
    ULong size;                                  /* +0x00 */
    ULong flags;                                 /* +0x04 */
    ULong fourCC;                                /* +0x08 */
    ULong rgbBitCount;                           /* +0x0c */
    ULong rBitMask;                              /* +0x10 */
    ULong gBitMask;                              /* +0x14 */
    ULong bBitMask;                              /* +0x18 */
    ULong rgbAlphaBitMask;                       /* +0x1c */
} DDPixelFormat;

typedef struct DDSurfaceDesc {                   /* 0x6c, asserted by the exe */
    ULong size;                                  /* +0x00 */
    ULong flags;                                 /* +0x04 */
    ULong height;                                /* +0x08 */
    ULong width;                                 /* +0x0c */
    long pitch;                                  /* +0x10 */
    ULong backBufferCount;                       /* +0x14 */
    ULong refreshRate;                           /* +0x18 */
    ULong alphaBitDepth;                         /* +0x1c */
    ULong reserved;                              /* +0x20 */
    void *surface;                               /* +0x24 */
    ULong colorKeys[8];                          /* +0x28 */
    DDPixelFormat pixelFormat;                   /* +0x48 */
    ULong caps;                                  /* +0x68 */
} DDSurfaceDesc;

typedef struct DDColorKey {
    ULong low;
    ULong high;
} DDColorKey;

typedef struct DDBltFx {                         /* DDBLTFX, 100 bytes */
    ULong size;
    ULong fx;
    ULong rop;
    ULong ddrop;
    ULong rotationAngle;
    ULong zBufferOpCode;
    ULong zBufferLow;
    ULong zBufferHigh;
    ULong zBufferBaseDest;
    ULong zDestConstBitDepth;
    ULong zDestConst;
    ULong zSrcConstBitDepth;
    ULong zSrcConst;
    ULong alphaEdgeBlendBitDepth;
    ULong alphaEdgeBlend;
    ULong reserved;
    ULong alphaDestConstBitDepth;
    ULong alphaDestConst;
    ULong alphaSrcConstBitDepth;
    ULong alphaSrcConst;
    ULong fillColor;                             /* +0x50 */
    DDColorKey destColorKey;
    DDColorKey srcColorKey;
} DDBltFx;

typedef struct Guid {
    ULong data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} Guid;

extern const Guid IID_IDirectDraw2;              /* 0x00571408 */
extern const Guid IID_IDirectDraw4;              /* 0x005713f8 */
extern const Guid IID_IDirectDrawGammaControl;   /* 0x005713e8 */

struct DDrawSurface;
struct DDrawClipper;
struct DDrawPalette;

/* IDirectDraw (v1): SetDisplayMode has no refresh rate and no flags. */
typedef struct DDrawVtbl {
    long (__stdcall *QueryInterface)(void *self, const Guid *iid,
                                     void *out);            /* +0x00 */
    ULong (__stdcall *AddRef)(void *self);                   /* +0x04 */
    ULong (__stdcall *Release)(void *self);                  /* +0x08 */
    char pad0C[0x10 - 0x0c];
    long (__stdcall *CreateClipper)(void *self, ULong flags,
                                    struct DDrawClipper **out,
                                    void *outer);            /* +0x10 */
    long (__stdcall *CreatePalette)(void *self, ULong caps, void *entries,
                                    struct DDrawPalette **out,
                                    void *outer);            /* +0x14 */
    long (__stdcall *CreateSurface)(void *self, DDSurfaceDesc *desc,
                                    struct DDrawSurface **out,
                                    void *outer);            /* +0x18 */
    char pad1C[0x50 - 0x1c];
    long (__stdcall *SetCooperativeLevel)(void *self, void *hWnd,
                                          ULong flags);      /* +0x50 */
    long (__stdcall *SetDisplayMode)(void *self, ULong w, ULong h,
                                     ULong bpp);             /* +0x54 */
} DDrawVtbl;

typedef struct DDraw {
    DDrawVtbl *vtbl;
} DDraw;

/* IDirectDraw2: only released here. */
typedef struct DDraw2Vtbl {
    long (__stdcall *QueryInterface)(void *self, const Guid *iid,
                                     void *out);            /* +0x00 */
    ULong (__stdcall *AddRef)(void *self);                   /* +0x04 */
    ULong (__stdcall *Release)(void *self);                  /* +0x08 */
} DDraw2Vtbl;

typedef struct DDraw2 {
    DDraw2Vtbl *vtbl;
} DDraw2;

/* IDirectDraw4: the same slot takes a refresh rate and flags. */
typedef struct DDraw4Vtbl {
    long (__stdcall *QueryInterface)(void *self, const Guid *iid,
                                     void *out);            /* +0x00 */
    ULong (__stdcall *AddRef)(void *self);                   /* +0x04 */
    ULong (__stdcall *Release)(void *self);                  /* +0x08 */
    char pad0C[0x54 - 0x0c];
    long (__stdcall *SetDisplayMode)(void *self, ULong w, ULong h,
                                     ULong bpp, ULong refresh,
                                     ULong flags);           /* +0x54 */
} DDraw4Vtbl;

typedef struct DDraw4 {
    DDraw4Vtbl *vtbl;
} DDraw4;

typedef struct DDrawSurfaceVtbl {
    long (__stdcall *QueryInterface)(void *self, const Guid *iid,
                                     void *out);            /* +0x00 */
    ULong (__stdcall *AddRef)(void *self);                   /* +0x04 */
    ULong (__stdcall *Release)(void *self);                  /* +0x08 */
    char pad0C[0x14 - 0x0c];
    long (__stdcall *Blt)(void *self, void *destRect, void *src,
                          void *srcRect, ULong flags, void *fx); /* +0x14 */
    char pad18[0x2c - 0x18];
    long (__stdcall *Flip)(void *self, struct DDrawSurface *target,
                           ULong flags);                     /* +0x2c */
    long (__stdcall *GetAttachedSurface)(void *self, ULong *caps,
                                         struct DDrawSurface **out); /* +0x30 */
    long (__stdcall *GetBltStatus)(void *self, ULong flags); /* +0x34 */
    char pad38[0x44 - 0x38];
    long (__stdcall *GetDC)(void *self, void **hdc);         /* +0x44 */
    char pad48[0x64 - 0x48];
    long (__stdcall *Lock)(void *self, void *rect, DDSurfaceDesc *desc,
                           ULong flags, void *event);        /* +0x64 */
    long (__stdcall *ReleaseDC)(void *self, void *hdc);      /* +0x68 */
    long (__stdcall *Restore)(void *self);                   /* +0x6c */
    long (__stdcall *SetClipper)(void *self,
                                 struct DDrawClipper *clipper); /* +0x70 */
    char pad74[0x7c - 0x74];
    long (__stdcall *SetPalette)(void *self,
                                 struct DDrawPalette *palette); /* +0x7c */
    long (__stdcall *Unlock)(void *self, void *ptr);          /* +0x80 */
} DDrawSurfaceVtbl;

typedef struct DDrawSurface {
    DDrawSurfaceVtbl *vtbl;
} DDrawSurface;

typedef struct DDrawClipperVtbl {
    long (__stdcall *QueryInterface)(void *self, const Guid *iid,
                                     void *out);            /* +0x00 */
    ULong (__stdcall *AddRef)(void *self);                   /* +0x04 */
    ULong (__stdcall *Release)(void *self);                  /* +0x08 */
    char pad0C[0x20 - 0x0c];
    long (__stdcall *SetHWnd)(void *self, ULong flags, void *hWnd); /* +0x20 */
} DDrawClipperVtbl;

typedef struct DDrawClipper {
    DDrawClipperVtbl *vtbl;
} DDrawClipper;

typedef struct DDrawPaletteVtbl {
    long (__stdcall *QueryInterface)(void *self, const Guid *iid,
                                     void *out);            /* +0x00 */
    ULong (__stdcall *AddRef)(void *self);                   /* +0x04 */
    ULong (__stdcall *Release)(void *self);                  /* +0x08 */
    char pad0C[0x18 - 0x0c];
    long (__stdcall *SetEntries)(void *self, ULong flags, ULong start,
                                 ULong count, void *entries); /* +0x18 */
} DDrawPaletteVtbl;

typedef struct DDrawPalette {
    DDrawPaletteVtbl *vtbl;
} DDrawPalette;

/* DDGAMMARAMP: 256 entries per channel. */
typedef struct DDGammaRamp {
    unsigned short red[256];                     /* +0x000 */
    unsigned short green[256];                   /* +0x200 */
    unsigned short blue[256];                    /* +0x400 */
} DDGammaRamp;

typedef struct DDrawGammaControlVtbl {
    long (__stdcall *QueryInterface)(void *self, const Guid *iid,
                                     void *out);            /* +0x00 */
    ULong (__stdcall *AddRef)(void *self);                   /* +0x04 */
    ULong (__stdcall *Release)(void *self);                  /* +0x08 */
    long (__stdcall *GetGammaRamp)(void *self, ULong flags,
                                   DDGammaRamp *ramp);       /* +0x0c */
    long (__stdcall *SetGammaRamp)(void *self, ULong flags,
                                   DDGammaRamp *ramp);       /* +0x10 */
} DDrawGammaControlVtbl;

typedef struct DDrawGammaControl {
    DDrawGammaControlVtbl *vtbl;
} DDrawGammaControl;

#define kSurfaceLost   0x887601c2UL   /* the code the exe retries Restore on */
#define kStillDrawing  0x8876021cUL   /* DDERR_WASSTILLDRAWING */
#define kWrongMode     0x8876024cUL   /* DDERR_WRONGMODE */

/* burgervideomode.c */
extern DDrawSurface *Win95FrontBuffer;           /* 0x008701b0 */
extern DDrawPalette *Win95WindowPalettePtr;      /* 0x008707d0 */
#define kNoInterface   0x800401f0UL   /* what it answers when DDRAW is absent */

#ifdef __cplusplus
}
#endif

#endif
