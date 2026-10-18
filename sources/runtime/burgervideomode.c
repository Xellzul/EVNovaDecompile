// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgervideomode.c -- BurgerLib GrWin95Code: the display mode, the page lock and flip, the release, the filled rectangle, the gamma save. Notes: Research/notes/burgervideomode.md */

#include <string.h>

#include "burgerddraw.h"
#include "burgerchan.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void *gMsimgTransparentBlit;                      /* atpalinit.c     */

Word ScreenClipRight;                            /* 0x0086ffe4 */
Word ScreenClipBottom;                           /* 0x008701c0 */
Word ScreenClipLeft;                             /* 0x00870d8c */
Word ScreenClipTop;                              /* 0x0087091c */
char *VideoPointer;                              /* 0x00870114 */
char *Win95VideoPointer;                         /* 0x008706f4 */
long Win95DestScreenX;                          /* 0x008701a4 */
long Win95DestScreenY;                           /* 0x008701a8 */
long VideoPageLocked;                            /* 0x008700c4 */
DDrawSurface *Win95WorkBuffer;                   /* 0x0086ffa8 */
void *Win95LockedMemory;                         /* 0x008705e0 */
void *Direct3DViewPortPtr;

static DDBltFx BlitTable = { sizeof(DDBltFx) };

/* GrWin95Code's .bss, 0x00866540..0x00867170 in the member's order. */
static void *DirectDrawInstance;                 /* 0x00866540 DDRAW.DLL */
static void *DirectDrawCreatePtr;                /* 0x00866544 */
static void *DirectDrawEnumerateAPtr;            /* 0x00866548 */
static void *DirectDrawEnumerateExAPtr;          /* 0x0086654c */
static long OldWindowXYValid;                    /* 0x00866550 */
static long OldWindowX;                          /* 0x00866554 */
static long OldWindowY;                          /* 0x00866558 */
static DDrawGammaControl *lpDDGammaControl;      /* 0x0086655c */
static DDGammaRamp DDGammaOld;                   /* 0x00866560 */
static DDGammaRamp DDGammaCurrent;               /* 0x00866b60 */
static unsigned char GammaDC;                    /* 0x00867160 */
static unsigned char GammaInit;                  /* 0x00867161 gamma saved */
static unsigned char GammaTry;                   /* 0x00867162 gamma wanted */
static unsigned char *Direct3DExecInstEndPtr;    /* 0x00867164 */
static void *OpenGLContext;                      /* 0x00867168 */
static void *OpenGLDeviceContext;                /* 0x0086716c */

long VideoFullScreen;                            /* 0x00870054 */
ULong VideoTrueScreenWidth;                      /* 0x00870168 */
ULong VideoTrueScreenHeight;                     /* 0x008707bc */
DDraw *DirectDrawPtr;                           /* 0x008707c0 */
DDraw4 *DirectDraw4Ptr;                          /* 0x00870960 */
DDraw2 *DirectDraw2Ptr;                          /* 0x0087095c */
DDrawSurface *Win95FrontBuffer;                  /* 0x008701b0 */
DDrawSurface *Win95BackBuffer;                   /* 0x008700c8 */
DDrawClipper *Win95WindowClipperPtr;             /* 0x008707b0 */
DDrawPalette *Win95WindowPalettePtr;             /* 0x008707d0 */
void *BurgerHPalette;                            /* 0x00870db4 GDI palette */
void *BurgerHBitMap;                             /* 0x008706f8 DIB section */
char *VideoOffscreen;                            /* 0x00870dbc */
long VideoHardWidth;                             /* 0x00870de8 */
Word VideoColorDepth;
long VideoTrueScreenDepth;                       /* 0x008700e0 */
long VideoRedShift;                              /* 0x008707ac */
long VideoRedMask;                               /* 0x008700fc */
long VideoGreenBits;                             /* 0x00870978 */
long VideoGreenMask;                             /* 0x00870dc0 */
long VideoBlueShift;                             /* 0x00870dd8 */
long VideoBlueMask;                              /* 0x0087092c */
long VideoBlueBits;                              /* 0x008707d4 */
long VideoGreenShift;                            /* 0x00870000 */
long VideoRedBits;                               /* 0x008700e8 */
unsigned char CurrentPalette[0x300];             /* 0x0086fab0 the palette bytes */

#include "burgerplat.h"

/* BITMAPINFO with the whole 8-bit palette inline: the same record
   burgerplat.h declares, at the length THIS caller needs. */
typedef struct BitmapInfoPal8 {
    BitmapInfoHeader bmiHeader;              /* +0x000 */
    unsigned char bmiColors[0x400];          /* +0x028 256 BGRA entries */
} BitmapInfoPal8;

void *BurgerHBitMapPtr;                              /* the DIB's bits */


#define kFullscreen      1                      /* SETDISPLAYFULLSCREEN */
#define kOffscreen       2                      /* SETDISPLAYOFFSCREEN */
#define kDoubleOk        4                      /* SETDISPLAYDOUBLEOK */
#define kDirect3D        0x10                   /* SETDISPLAYD3D */
#define kNoRestyle       0x20                   /* SETDISPLAYNOWINDOWTWIDDLE */
#define kStyleIndex      -16
#define kExStyleIndex    -20
#define kStylePopup      0x80000000             /* WS_POPUP */
#define kStyleWindowed   0x00c60000             /* WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX */
#define kWorkArea        0x30

#define kEnumAllDevices  7                      /* attached, detached, non-display */
#define kEnumStop        0                      /* DDENUMRET_CANCEL */
#define kEnumContinue    1                      /* DDENUMRET_OK */
#define kCapsBackBuffer  4                      /* DDSCAPS_BACKBUFFER */
#define kCapsOffscreen   0x40                   /* DDSCAPS_OFFSCREENPLAIN */
#define kCaps3DDevice    0x2000                 /* DDSCAPS_3DDEVICE */
#define kPaletteCaps     4                      /* DDPCAPS_8BIT */
#define kPaletteVersion  0x300
#define kBltDone         2                      /* DDGBS_ISBLTDONE */
#define kLockWait        1                      /* DDLOCK_WAIT */
#define kFlipWait        1                      /* DDFLIP_WAIT */
#define kSrcCopy         0xcc0020               /* SRCCOPY */

/* DDRAW.DLL is loaded by the first entry point that needs it; the call sites each carry an
   inlined copy. Not linked. */
void GetDDRAWDLL(void)
{
    if (DirectDrawInstance == 0) {
        DirectDrawInstance = LoadLibraryA("DDRAW.DLL");
        if (DirectDrawInstance != 0) {
            DirectDrawCreatePtr = GetProcAddress(DirectDrawInstance, "DirectDrawCreate");
            DirectDrawEnumerateAPtr = GetProcAddress(DirectDrawInstance, "DirectDrawEnumerate");
            DirectDrawEnumerateExAPtr = GetProcAddress(DirectDrawInstance, "DirectDrawEnumerateExA");
        }
    }
}

/* LOGPALETTE: the entries are what DirectDraw's CreatePalette takes, the
   whole record what GDI's does. */
typedef struct LogPalette {
    unsigned short version;                      /* +0x000 */
    unsigned short count;                        /* +0x002 */
    unsigned char entries[kPaletteEntries][4];   /* +0x004 R,G,B,flags */
} LogPalette;

/* The device search DirectDrawEnumerateExA is handed: counts down to the
   wanted device and keeps a copy of its GUID. */
typedef struct DeviceSearch {
    ULong remaining;                             /* +0x00 */
    Guid *guid;                                  /* +0x04 */
    Guid storage;                                /* +0x08 */
} DeviceSearch;

/* FindDeviceCallback (BurgerLib FindDeviceCallback). STATE: MATCH 100%, exe 27 / obj 27,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ec420
int __stdcall FindDeviceCallback(Guid *guid, char *description, char *driverName,
                           void *context, void *monitor)
{
    DeviceSearch *search;

    search = (DeviceSearch *)context;
    if (--search->remaining == 0) {
        if (guid != 0) {
            search->guid = &search->storage;
            memcpy(&search->storage, guid, sizeof(Guid));
        } else {
            search->guid = 0;
        }
        return kEnumStop;
    }
    return kEnumContinue;
}

/* LockVideoPage (BurgerLib GrWin95Code). STATE: MATCH 100.0%, exe 113 / obj 113,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ec480
void LockVideoPage(void)
{
    DDrawSurface *surface;
    long result;
    DDSurfaceDesc desc;
    ULong offset;

    if (VideoPageLocked == 0) {
        if (VideoOffscreen != 0) {
            VideoPointer = VideoOffscreen;
            VideoWidth = VideoHardWidth;
            Win95VideoPointer = 0;
            return;
        }
        if (Win95UseBackBuffer != 0)
            surface = Win95BackBuffer;
        else
            surface = Win95FrontBuffer;
        while (surface->vtbl->GetBltStatus(surface, kBltDone) == kStillDrawing)
            ;
        desc.size = sizeof(DDSurfaceDesc);
        desc.flags = 0;
        result = surface->vtbl->Lock(surface, 0, &desc, kLockWait, 0);
        if (result == 0) {
Locked:
            VideoPageLocked = 1;
            VideoPointer = (char *)desc.surface;
            Win95LockedMemory = VideoPointer;
            Win95WorkBuffer = surface;
            Win95VideoPointer = VideoPointer;
            VideoWidth = desc.pitch;
            if (Win95UseBackBuffer != 0)
                return;
            offset = ((VideoColorDepth + 7) >> 3) * Win95DestScreenX;
            VideoPointer += Win95DestScreenY * VideoWidth + offset;
            Win95VideoPointer = VideoPointer;
        } else {
            if (result == kSurfaceLost) {
                result = surface->vtbl->Restore(surface);
                if (result == 0) {
                    result = surface->vtbl->Lock(surface, 0, &desc, kLockWait, 0);
                    if (result == 0)
                        goto Locked;
                } else if (result == kWrongMode) {
                    result = Win95FrontBuffer->vtbl->Restore(Win95FrontBuffer);
                    if (result == 0) {
                        result = surface->vtbl->Lock(surface, 0, &desc, kLockWait, 0);
                        if (result == 0)
                            goto Locked;
                    }
                }
            }
            VideoPointer = 0;
            NonFatal("Can't lock the buffer bits!\n");
            return;
        }
    }
}

/* UnlockVideoPage (BurgerLib GrWin95Code; DrawARect16 inlines it). STATE: MATCH 100.0%,
   exe 18 / obj 18, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ec670
void UnlockVideoPage(void)
{
    if (VideoPageLocked != 0) {
        if (Win95VideoPointer != 0) {
            Win95WorkBuffer->vtbl->Unlock(Win95WorkBuffer, Win95LockedMemory);
            Win95VideoPointer = 0;
        }
        VideoPageLocked = 0;
    }
}

/* UpdateAndNoPageFlip (BurgerLib GrWin95Code). STATE: MATCH 100%, exe 129 / obj 129,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ec6c0
void UpdateAndNoPageFlip(void)
{
    char *offscreen;
    ULong bytes;
    void *memDC;
    void *hdc;

    UnlockVideoPage();
    if (Win95UseBackBuffer != 0) {
        if (VideoFullScreen != 0) {
            if (VideoOffscreen != 0) {
                offscreen = VideoOffscreen;
                VideoOffscreen = 0;
                LockVideoPage();
                if (VideoPointer != 0) {
                    bytes = (VideoColorDepth + 7) >> 3;
                    if (VideoColorDepth == 15 && VideoTrueScreenDepth == 16) {
                        VideoOffscreen = offscreen;
                        Video555To565();
                        VideoOffscreen = 0;
                    } else {
                        DrawShapeLowLevel(0, 0, VideoTrueScreenWidth * bytes, VideoTrueScreenHeight, 0,
                                          offscreen);
                    }
                    UnlockVideoPage();
                }
                VideoOffscreen = offscreen;
            }
            Win95FrontBuffer->vtbl->Flip(Win95FrontBuffer, 0, kFlipWait);
        } else {
            hdc = GetDC(Win95MainWindow);
            memDC = CreateCompatibleDC(hdc);
            if (BurgerHPalette != 0)
                SelectPalette(hdc, BurgerHPalette, 0);
            RealizePalette(hdc);
            SelectObject(memDC, BurgerHBitMap);
            BitBlt(hdc, 0, 0, ScreenWidth, ScreenHeight, memDC, 0, 0, kSrcCopy);
            DeleteDC(memDC);
            ReleaseDC(Win95MainWindow, hdc);
        }
        Win95UseBackBuffer = 0;
        Win95WorkBuffer = Win95FrontBuffer;
        Win95VideoPointer = VideoPointer;
    }
}

/* UpdateAndPageFlip (BurgerLib GrWin95Code). STATE: MATCH 100.0%, exe 7 / obj 7,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ec960
void UpdateAndPageFlip(void)
{
    UpdateAndNoPageFlip();
    Win95UseBackBuffer = 1;
    Win95WorkBuffer = Win95BackBuffer;
    Win95VideoPointer = VideoPointer;
}

/* ReleaseVideo (BurgerLib GrWin95Code). STATE: MATCH 100.0%, exe 121 / obj 121,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ec990
void ReleaseVideo(void)
{
    unsigned char wanted;

    UnlockVideoPage();
    wanted = GammaTry;
    VideoOSGammaDestroy();
    GammaTry = wanted;
    if (Win95WindowClipperPtr != 0) {
        Win95WindowClipperPtr->vtbl->Release(Win95WindowClipperPtr);
        Win95WindowClipperPtr = 0;
    }
    if (Win95BackBuffer != 0) {
        Win95BackBuffer->vtbl->Release(Win95BackBuffer);
        Win95BackBuffer = 0;
    }
    if (Win95WindowPalettePtr != 0) {
        Win95WindowPalettePtr->vtbl->Release(Win95WindowPalettePtr);
        Win95WindowPalettePtr = 0;
    }
    if (Win95FrontBuffer != 0) {
        Win95FrontBuffer->vtbl->Release(Win95FrontBuffer);
        Win95FrontBuffer = 0;
    }
    if (DirectDraw4Ptr != 0) {
        DirectDraw4Ptr->vtbl->Release(DirectDraw4Ptr);
        DirectDraw4Ptr = 0;
    }
    if (DirectDraw2Ptr != 0) {
        DirectDraw2Ptr->vtbl->Release(DirectDraw2Ptr);
        DirectDraw2Ptr = 0;
    }
    if (DirectDrawPtr != 0) {
        DirectDrawPtr->vtbl->Release(DirectDrawPtr);
        DirectDrawPtr = 0;
    }
    if (VideoOffscreen != 0 && VideoOffscreen != BurgerHBitMapPtr) {
        DeallocAPointer(VideoOffscreen);
        VideoOffscreen = 0;
    }
    if (BurgerHBitMap != 0) {
        DeleteObject(BurgerHBitMap);
        BurgerHBitMap = 0;
        BurgerHBitMapPtr = 0;
    }
    if (BurgerHPalette != 0) {
        DeleteObject(BurgerHPalette);
        BurgerHPalette = 0;
    }
    VideoOffscreen = 0;
    ScreenWidth = 0;
    ScreenHeight = 0;
    if (DirectDrawInstance != 0) {
        FreeLibrary(DirectDrawInstance);
        DirectDrawInstance = 0;
    }
}

/* Win95VideoGetGuid (BurgerLib GrWin95Code): the exe has no body of its own; SetDisplayToSize
   inlines it. */
static inline Word Win95VideoGetGuid(Guid **OutPtr, Guid *Output, Word DevNum)
{
    DeviceSearch search;
    long result;

    if (!DevNum)
        DevNum = 1;
    search.remaining = DevNum;
    GetDDRAWDLL();
    result = (DirectDrawEnumerateExAPtr == 0)
        ? kNoInterface
        : ((long (__stdcall *)(void *, void *, ULong))DirectDrawEnumerateExAPtr)(
              FindDeviceCallback, &search, kEnumAllDevices);
    if (result == 0 && search.remaining == 0) {
        if (search.guid != 0) {
            *OutPtr = Output;
            memcpy(Output, search.guid, sizeof(Guid));
        } else {
            FastMemSet(Output, 0, sizeof(Guid));
            *OutPtr = 0;
        }
        return 0;
    }
    return 1;
}

/* SetDisplayToSize (BurgerLib SetDisplayToSize). STATE: MATCH 100%, exe 1199 / obj 1199,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ecbd0
long SetDisplayToSize(ULong width, ULong height, ULong depth, ULong flags)
{
    unsigned char *entryPtr;
    const unsigned char *src;
    DDSurfaceDesc desc;
    ULong gammaWanted;
    long rect[4];
    ULong caps;
    long workArea[4];
    LogPalette palette;
    ULong style;
    ULong exStyle;
    ULong refresh;
    Word device;
    ULong wantFifteen;
    DDSurfaceDesc probeDesc;
    long result;
    DDrawSurface *probe;
    void *probeDC;
    void *hdc;
    void *memDC;
    ULong pixel;
    BitmapInfoPal8 info;
    ULong windowRect[4];
    Guid deviceGuid;
    Guid *deviceGuidPtr;
    ULong fullScreen;
    unsigned short *pixels;
    Word i;
    unsigned char *colors;

    gammaWanted = GammaInit;
    if (gammaWanted != 0)
        GammaTry = 1;

    refresh = depth >> 16;
    depth = (unsigned short)depth;
    wantFifteen = 0;
    if (depth == 24)
        depth = 32;
    if (depth == 15) {
        if ((flags & kOffscreen) != 0)
            wantFifteen = 1;
        depth = 16;
    }

    if (VideoFullScreen == 0) {
        GetWindowRect(Win95MainWindow, (WinRect *)rect);
        OldWindowX = rect[0];
        OldWindowY = rect[1];
        OldWindowXYValid = 1;
    }

    ReleaseVideo();
    ScreenWidth = width;
    ScreenHeight = height;
    flags &= ~kDoubleOk;
    ScreenFlags = flags;
    fullScreen = flags & kFullscreen;
    VideoFullScreen = fullScreen;
    ScreenClipTop = 0;
    ScreenClipLeft = 0;
    ScreenClipRight = width;
    ScreenClipBottom = height;
    VideoTrueScreenWidth = width;
    VideoTrueScreenHeight = height;

    if ((flags & kNoRestyle) == 0) {
        if (fullScreen == 0) {
            style = (GetWindowLongA(Win95MainWindow, kStyleIndex) & ~kStylePopup)
                  | kStyleWindowed;
            SetWindowLongA(Win95MainWindow, kStyleIndex, style);
            windowRect[1] = windowRect[0] = 0;
            windowRect[2] = width;
            windowRect[3] = height;
            exStyle = GetWindowLongA(Win95MainWindow, kExStyleIndex);
            AdjustWindowRectEx((WinRect *)windowRect, style,
                               GetMenu(Win95MainWindow) != 0, exStyle);
            SetWindowPos(Win95MainWindow, (void *)-2, 0, 0,
                         windowRect[2] - windowRect[0],
                         windowRect[3] - windowRect[1], 0x12);
            SystemParametersInfoA(kWorkArea, 0, workArea, 0);
            if (OldWindowXYValid == 0) {
                GetWindowRect(Win95MainWindow, (WinRect *)windowRect);
            } else {
                windowRect[0] = OldWindowX;
                windowRect[1] = OldWindowY;
            }
            if ((long)windowRect[0] < workArea[0])
                windowRect[0] = workArea[0];
            if ((long)windowRect[1] < workArea[1])
                windowRect[1] = workArea[1];
            SetWindowPos(Win95MainWindow, 0, windowRect[0], windowRect[1],
                         0, 0, 0x15);
        } else {
            style = GetWindowLongA(Win95MainWindow, kStyleIndex);
            SetWindowLongA(Win95MainWindow, kStyleIndex,
                           (style | kStylePopup) & ~kStyleWindowed);
        }
    }

    device = flags >> 16;
    if (device != 0) {
        if (Win95VideoGetGuid(&deviceGuidPtr, &deviceGuid, device) == 0) {
            GetDDRAWDLL();
            result = (DirectDrawCreatePtr == 0)
                ? kNoInterface
                : ((long (__stdcall *)(void *, DDraw **, void *))DirectDrawCreatePtr)(
                      deviceGuidPtr, &DirectDrawPtr, 0);
            if (result != 0) {
                NonFatal(
                    "Couldn't create the direct draw instance for device #%d\n",
                    device);
                return 1;
            }
        } else {
            NonFatal("Device number #%d is too high\n", device);
            return 1;
        }
    } else {
        GetDDRAWDLL();
        result = (DirectDrawCreatePtr == 0)
            ? kNoInterface
            : ((long (__stdcall *)(void *, DDraw **, void *))DirectDrawCreatePtr)(
                  0, &DirectDrawPtr, 0);
        if (result != 0) {
            NonFatal("Couldn't create the direct draw buffer\n");
            return 1;
        }
    }

    /* IDirectDraw2 first, and only with it IDirectDraw4, the one whose
       SetDisplayMode takes a refresh rate. */
    if (DirectDrawPtr->vtbl->QueryInterface(DirectDrawPtr, &IID_IDirectDraw2,
                                           &DirectDraw2Ptr) == 0) {
        if (DirectDrawPtr->vtbl->QueryInterface(DirectDrawPtr,
                                               &IID_IDirectDraw4,
                                               &DirectDraw4Ptr) != 0)
            DirectDraw4Ptr = 0;
    } else {
        DirectDraw2Ptr = 0;
    }

    if (DirectDrawPtr->vtbl->SetCooperativeLevel(
            DirectDrawPtr, Win95MainWindow,
            (fullScreen != 0) ? 0x51 : 8) != 0) {
        NonFatal("Couldn't set direct draw cooperative level\n");
        return 1;
    }

    FastMemSet(&desc, 0, sizeof(desc));
    desc.size = sizeof(desc);

    if (fullScreen != 0) {
        if (DirectDraw4Ptr == 0) {
            if (DirectDrawPtr->vtbl->SetDisplayMode(DirectDrawPtr, width,
                                                   height, depth) != 0) {
                ReleaseVideo();
                NonFatal("Couldn't set display mode %dx%dx%d\n",
                             width, height, depth);
                return 1;
            }
        } else {
            if (DirectDraw4Ptr->vtbl->SetDisplayMode(DirectDraw4Ptr, width,
                                                   height, depth,
                                                   refresh, 0) != 0) {
                ReleaseVideo();
                NonFatal(
                    "Couldn't set display mode %dx%dx%d at %d hertz\n",
                    width, height, depth, refresh);
                return 1;
            }
        }
        desc.backBufferCount = 2;
        desc.flags = 0x21;
        desc.caps = 0x218;
    } else {
        desc.flags = 1;
        desc.caps = 0x200;
    }

    if ((flags & kDirect3D) != 0)
        desc.caps = desc.caps | kCaps3DDevice;

    if (DirectDrawPtr->vtbl->CreateSurface(DirectDrawPtr, &desc,
                                          &Win95FrontBuffer, 0) != 0) {
        ReleaseVideo();
        NonFatal("Couldn't create the frontbuffer surface\n");
        return 1;
    }

    if (fullScreen != 0) {
        FastMemSet(&caps, 0, sizeof(caps));
        caps = kCapsBackBuffer;
        if ((flags & kDirect3D) != 0)
            caps = caps | kCaps3DDevice;
        if (Win95FrontBuffer->vtbl->GetAttachedSurface(Win95FrontBuffer, &caps,
                                                   &Win95BackBuffer) != 0) {
            ReleaseVideo();
            NonFatal("Couldn't get attached surface on the front buffer\n");
            return 1;
        }
    } else {
        FastMemSet(&desc, 0, sizeof(desc));
        desc.size = sizeof(desc);
        desc.flags = 7;
        desc.caps = kCapsOffscreen;
        if ((flags & kDirect3D) != 0)
            desc.caps = desc.caps | kCaps3DDevice;
        desc.width = width;
        desc.height = height;
        if (DirectDrawPtr->vtbl->CreateSurface(DirectDrawPtr, &desc,
                                              &Win95BackBuffer, 0) != 0) {
            ReleaseVideo();
            NonFatal("Couldn't create the backbuffer for window\n");
            return 1;
        }
        if (DirectDrawPtr->vtbl->CreateClipper(DirectDrawPtr, 0,
                                              &Win95WindowClipperPtr, 0) != 0) {
            ReleaseVideo();
            NonFatal("Couldn't create clipper\n");
            return 1;
        }
        if (Win95WindowClipperPtr->vtbl->SetHWnd(Win95WindowClipperPtr, 0,
                                        Win95MainWindow) != 0) {
            ReleaseVideo();
            NonFatal("Couldn't set clipper hwnd\n");
            return 1;
        }
        if (Win95FrontBuffer->vtbl->SetClipper(Win95FrontBuffer,
                                           Win95WindowClipperPtr) != 0) {
            ReleaseVideo();
            NonFatal("Couldn't set clipper for frontbuffer\n");
            return 1;
        }

        /* BITMAPINFO carries the palette as RGBQUAD, which is B,G,R --
           the reverse of the R,G,B triples held at 0x0086fab0, so the copy
           is a reversal and not a memcpy. */
        i = 0;
        colors = info.bmiColors;
        do {
            colors[2] = CurrentPalette[i];
            colors[1] = CurrentPalette[i + 1];
            colors[0] = CurrentPalette[i + 2];
            colors[3] = 0;
            colors += 4;
            i += 3;
        } while (i < kPaletteEntries * 3);
        VideoHardWidth = (depth >> 3) * width;
        info.bmiHeader.biSize = sizeof(BitmapInfoHeader);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = depth;
        info.bmiHeader.biCompression = 0;
        info.bmiHeader.biSizeImage = height * VideoHardWidth;
        info.bmiHeader.biXPelsPerMeter = 0;
        info.bmiHeader.biYPelsPerMeter = 0;
        info.bmiHeader.biClrUsed = 0;
        info.bmiHeader.biClrImportant = 0;

        hdc = GetDC(Win95MainWindow);
        BurgerHBitMap = CreateDIBSection(hdc, &info, 0, &BurgerHBitMapPtr, 0, 0);
        VideoOffscreen = (char *)BurgerHBitMapPtr;

        if (depth == 16) {
            memDC = CreateCompatibleDC(hdc);
            SelectObject(memDC, BurgerHBitMap);
            *(unsigned short *)VideoOffscreen = 0x7fff;
            pixel = GetPixel(memDC, 0, 0);
            DeleteDC(memDC);
            if ((pixel & 0xffffff) == 0xffffff)
                depth = 15;
        }
        ReleaseDC(Win95MainWindow, hdc);

    }

    if (depth <= 8) {
        entryPtr = palette.entries[0];
        CurrentPalette[0] = 0;
        CurrentPalette[1] = 0;
        CurrentPalette[2] = 0;
        CurrentPalette[0x2fd] = 0xff;
        CurrentPalette[0x2fe] = 0xff;
        CurrentPalette[0x2ff] = 0xff;
        colors = entryPtr;
        src = CurrentPalette;
        i = 0;
        do {
            colors[0] = src[0];
            colors[1] = src[1];
            colors[2] = src[2];
            colors[3] = 0;
            src += 3;
            colors += 4;
        } while (++i < kPaletteEntries);
        if (DirectDrawPtr->vtbl->CreatePalette(DirectDrawPtr, kPaletteCaps,
                                              entryPtr,
                                              &Win95WindowPalettePtr, 0) != 0) {
            ReleaseVideo();
            NonFatal("Couldn't create the palette\n");
            return 1;
        }
        /* Restore and retry ONCE on a lost surface; never a loop. */
        if (Win95FrontBuffer->vtbl->SetPalette(Win95FrontBuffer,
                                               Win95WindowPalettePtr)
            == kSurfaceLost) {
            Win95FrontBuffer->vtbl->Restore(Win95FrontBuffer);
            Win95FrontBuffer->vtbl->SetPalette(Win95FrontBuffer, Win95WindowPalettePtr);
        }
        if (fullScreen == 0) {
            palette.version = kPaletteVersion;
            palette.count = kPaletteEntries;
            BurgerHPalette = CreatePalette(&palette);
        }
    }

    FastMemSet(&desc, 0, sizeof(desc));
    desc.size = sizeof(desc);

    result = Win95BackBuffer->vtbl->Lock(Win95BackBuffer, 0, &desc, 1, 0);
    if (result == kSurfaceLost) {
        Win95BackBuffer->vtbl->Restore(Win95BackBuffer);
        result = Win95BackBuffer->vtbl->Lock(Win95BackBuffer, 0, &desc, 1, 0);
    }
    if (result != 0) {
        ReleaseVideo();
        NonFatal("I could not lock the backbuffer!\n");
        return 1;
    }

    Win95BackBuffer->vtbl->Unlock(Win95BackBuffer, 0);

    if (VideoOffscreen == 0 && (flags & kOffscreen) != 0) {
        VideoHardWidth = ((depth + 7) >> 3) * width;
        VideoOffscreen = (char *)AllocAPointer(VideoHardWidth * height);
    }

    if (depth == 16 && BurgerHBitMap == 0) {
        probe = Win95BackBuffer;
        if (probe->vtbl->GetDC(probe, &probeDC) == 0 ||
            (probe = Win95FrontBuffer, probe->vtbl->GetDC(probe, &probeDC) == 0)) {
            SetPixel(probeDC, 0, 0, 0xff00);
            probe->vtbl->ReleaseDC(probe, probeDC);
            probeDesc.size = sizeof(probeDesc);
            if (probe->vtbl->Lock(probe, 0, &probeDesc, 1, 0) == 0) {
                pixels = (unsigned short *)probeDesc.surface;
                if ((*pixels & 0x7e0) != 0x7e0)
                    depth = 15;
                probe->vtbl->Unlock(probe, 0);
            }
            probe->vtbl->ReleaseDC(probe, probeDC);
        }
    }

    MouseSetRange(ScreenWidth, ScreenHeight);
    VideoColorDepth = depth;
    VideoTrueScreenDepth = depth;

    if (depth == 16 && wantFifteen == 0) {
        VideoRedShift = 11;
        VideoRedMask = 0xf800;
        VideoGreenBits = 6;
        VideoGreenMask = 0x7e0;
    } else {
        if (depth == 16 && wantFifteen != 0)
            VideoColorDepth = 15;
        VideoRedShift = 10;
        VideoRedMask = 0x7c00;
        VideoGreenBits = 5;
        VideoGreenMask = 0x3e0;
    }
    VideoBlueShift = 0;
    VideoBlueMask = 0x1f;
    VideoBlueBits = 5;
    VideoGreenShift = 5;
    VideoRedBits = 5;

    LockVideoPage();
    UnlockVideoPage();

    if (gammaWanted != 0 || GammaTry != 0)
        VideoOSGammaInit();
    GammaTry = 0;
    return 0;
}

/* IsDirectInputPresent (BurgerLib GrWin95Code). Not linked. */
Word IsDirectInputPresent(void)
{
    void *dll;
    Word present;

    present = 0;
    dll = LoadLibraryA("DINPUT.DLL");
    if (dll != 0) {
        if (GetProcAddress(dll, "DirectInputCreateA") != 0)
            present = 1;
        FreeLibrary(dll);
    }
    return present;
}

#define kBytesPerPixel 2
#define kBltFlags      0x600

/* DrawARect16 (BurgerLib DrawARect16). STATE: MATCH 100.0%, exe 155 / obj 155,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ee300
void DrawARect16(int x, int y, Word Width, Word Height, Word Color)
{
    char *Screenad;

    if (x >= (int)ScreenClipRight || y >= (int)ScreenClipBottom)
        goto Exit;
    Width += x;
    Height += y;
    if ((int)Width <= (int)ScreenClipLeft || (int)Height <= (int)ScreenClipTop)
        goto Exit;
    if (x < (int)ScreenClipLeft)
        goto ClipLeft;
    if (y < (int)ScreenClipTop)
        goto ClipTop;
ClipRightTest:
    if (Width > ScreenClipRight)
        goto ClipRight;
ClipBottomTest:
    if (Height > ScreenClipBottom)
        goto ClipBottom;
Clipped:
    Width -= x;
    Height -= y;
    if (VideoPointer == Win95VideoPointer) {
        WinRect MyRect;

        if (Win95UseBackBuffer == 0) {
            x += Win95DestScreenX;
            y += Win95DestScreenY;
        }
        MyRect.left = x;
        MyRect.right = x + Width;
        MyRect.top = y;
        MyRect.bottom = y + Height;
        BlitTable.fillColor = Color;
        if (VideoPageLocked != 0) {
            UnlockVideoPage();
            Win95WorkBuffer->vtbl->Blt(Win95WorkBuffer, &MyRect, 0, 0, kBltFlags, &BlitTable);
            LockVideoPage();
            return;
        }
        Win95WorkBuffer->vtbl->Blt(Win95WorkBuffer, &MyRect, 0, 0, kBltFlags, &BlitTable);
        return;
    }
    x *= kBytesPerPixel;
    Width *= kBytesPerPixel;
    Screenad = VideoPointer + (VideoWidth * y + x);
    x = VideoWidth;
    if (Width == x)
        goto OneFill;
    do {
        FastMemSet16(Screenad, Color, Width);
        Screenad += x;
    } while (--Height);
Exit:
    return;
OneFill:
    FastMemSet16(Screenad, Color, Width * Height);
    return;

ClipLeft:
    x = ScreenClipLeft;
    if (y >= (int)ScreenClipTop)
        goto ClipRightTest;
ClipTop:
    y = ScreenClipTop;
    if (Width <= ScreenClipRight)
        goto ClipBottomTest;
ClipRight:
    Width = ScreenClipRight;
    if (Height <= ScreenClipBottom)
        goto Clipped;
ClipBottom:
    Height = ScreenClipBottom;
    goto Clipped;
}

/* VideoOSGammaInit (BurgerLib GrWin95Code). STATE: MATCH 100.0%, exe 72 / obj 72,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ee5a0
void VideoOSGammaInit(void)
{
    void *hdc;

    if (GammaInit == 0) {
        if (VideoFullScreen != 0 && OpenGLContext == 0 && Win95FrontBuffer != 0 &&
            Win95FrontBuffer->vtbl->QueryInterface(Win95FrontBuffer,
                                                   &IID_IDirectDrawGammaControl,
                                                   &lpDDGammaControl) == 0) {
            lpDDGammaControl->vtbl->GetGammaRamp(lpDDGammaControl, 0, &DDGammaOld);
            memcpy(&DDGammaCurrent, &DDGammaOld, sizeof(DDGammaRamp));
            GammaInit = 1;
            return;
        }
        if (Win95MainWindow != 0) {
            hdc = GetDC(Win95MainWindow);
            if (hdc != 0) {
                GetDeviceGammaRamp(hdc, &DDGammaOld);
                memcpy(&DDGammaCurrent, &DDGammaOld, sizeof(DDGammaRamp));
                GammaDC = 1;
            }
            GammaInit = 1;
        }
    }
}

/* VideoOSGammaDestroy (BurgerLib GrWin95Code). STATE: MATCH 100.0%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ee6f0
void VideoOSGammaDestroy(void)
{
    void *hdc;

    GammaTry = 0;
    if (GammaInit != 0) {
        if (lpDDGammaControl != 0) {
            lpDDGammaControl->vtbl->SetGammaRamp(lpDDGammaControl, 0, &DDGammaOld);
            lpDDGammaControl->vtbl->Release(lpDDGammaControl);
            lpDDGammaControl = 0;
        }
        if (GammaDC != 0) {
            hdc = GetDC(Win95MainWindow);
            if (hdc != 0)
                SetDeviceGammaRamp(hdc, &DDGammaOld);
            GammaDC = 0;
        }
        GammaInit = 0;
    }
}

typedef struct GammaTable_t {
    float Red[256];
    float Green[256];
    float Blue[256];
} GammaTable_t;

#define kGammaMax 0xffff

static inline unsigned short GammaToWord(float f)
{
    int v;

    v = (int)(f * (float)kGammaMax);
    if (v < 0)
        v = 0;
    if (v >= 0x10000)
        v = kGammaMax;
    return v;
}

/* VideoOSGammaSet and VideoOSGammaGet (BurgerLib GrWin95Code). Not linked. */
void VideoOSGammaSet(const GammaTable_t *TablePtr)
{
    void *hdc;
    Word i;

    if (GammaInit != 0 && TablePtr != 0) {
        i = 0;
        do {
            DDGammaCurrent.red[i] = GammaToWord(TablePtr->Red[i]);
            DDGammaCurrent.green[i] = GammaToWord(TablePtr->Green[i]);
            DDGammaCurrent.blue[i] = GammaToWord(TablePtr->Blue[i]);
        } while (++i < 256);
        if (lpDDGammaControl != 0)
            lpDDGammaControl->vtbl->SetGammaRamp(lpDDGammaControl, 0, &DDGammaCurrent);
        if (GammaDC != 0) {
            hdc = GetDC(Win95MainWindow);
            if (hdc != 0)
                SetDeviceGammaRamp(hdc, &DDGammaCurrent);
        }
    }
}

void VideoOSGammaGet(GammaTable_t *TablePtr)
{
    Word i;

    if (TablePtr != 0) {
        if (GammaInit != 0) {
            i = 0;
            do {
                TablePtr->Red[i] = DDGammaCurrent.red[i] * (1.0f / kGammaMax);
                TablePtr->Green[i] = DDGammaCurrent.green[i] * (1.0f / kGammaMax);
                TablePtr->Blue[i] = DDGammaCurrent.blue[i] * (1.0f / kGammaMax);
            } while (++i < 256);
        } else {
            FastMemSet(TablePtr, 0, sizeof(GammaTable_t));
        }
    }
}

#ifdef __cplusplus
}
#endif
