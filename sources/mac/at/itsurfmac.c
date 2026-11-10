/* itsurfmac.c -- 1.0.10: the draw tool's GWorld surfaces and pixel-map locking, win's itgfx.c on the Mac. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

#define kPixelsWholeWindow 0x01
#define kPixelsDirect 0x04
#define kPixelsLockBit 3
#define kPixelsTempMem 0x80000000
#define kPixelsBufferBit 0x1e
#define kDibFailed -7300
#define kRegionLimit 0x7d00
#define kSystemPaletteKey 0x8889

#define PM(p) ((PixMapHandle)(p)->dib)
#define GD(p) ((GDHandle)(p)->unk54)

extern unsigned char gDT_TempMem;
#if TARGET_API_MAC_CARBON
long gDT_SurfaceExtra = 1;
#define kDeviceSurfaceExtra 1
#else
long gDT_SurfaceExtra;
#define kDeviceSurfaceExtra 0
#endif
extern ITPalette gDT_DefaultPalette;

extern RgnHandle DT_RegionCreate(void);

void _DT_PaletteToMacCTab(CTabHandle ctab, const ITPalette *palette);
void _DT_MacCTabToPalette(ITPalette *palette, CTabHandle ctab, unsigned char alpha);
void _DT_MacCTabToInverse(unsigned char *table, CTabHandle ctab);
ITImage *DT_SurfaceCreateLikeWindow(CGrafPtr window, const ITRect *area);
long _DT_SurfaceSet(ITImage *image, GWorldPtr world, GDHandle device, CGrafPtr window, ULong flags, ULong extra);
long DT_SurfaceCopyRects(ITImage *image, ULong count, const ITRect *src, const ITRect *dst);
long _DT_ContextCreate(ITPort *port, void *window, CGrafPtr dc, unsigned char save);

PixMapHandle DT_CreateMacPixMap(const Rect *bounds, ULong depth, CTabHandle ctab, unsigned char temp)
{
    long err;
    PixMapHandle pm;
    Handle data;
    ULong rowBytes;
    OSErr oserr;

    err = 0;
    data = 0;
    rowBytes = (((bounds->right - bounds->left) * depth >> 3) + 0x1f) & 0xffe0;
    pm = (PixMapHandle)NewHandleClear(sizeof(PixMap));
    if (pm == 0) {
        MemError();
        err = kOutOfMemory;
        goto done;
    }
    if (temp) {
        data = TempNewHandle((bounds->bottom - bounds->top) * rowBytes, &oserr);
        if (data == 0) {
            err = kOutOfMemory;
            goto done;
        }
    } else {
        data = NewHandleClear((bounds->bottom - bounds->top) * rowBytes);
        if (data == 0) {
            MemError();
            err = kOutOfMemory;
            goto done;
        }
    }
    if (ctab == 0) {
        if (depth <= 8) {
            ctab = GetCTable(depth);
            if (ctab == 0) {
                err = kOutOfMemory;
                goto done;
            }
        } else {
            ctab = (CTabHandle)NewHandleClear(sizeof(ColorTable) + sizeof(ColorSpec));
            if (ctab == 0) {
                MemError();
                err = kOutOfMemory;
                goto done;
            }
            (*ctab)->ctSeed = depth == 16 ? 15 : 24;
            (*ctab)->ctSize = 1;
            (*ctab)->ctFlags = 0;
            (*ctab)->ctTable[0].rgb.red = 0xffff;
            (*ctab)->ctTable[0].rgb.green = 0xffff;
            (*ctab)->ctTable[0].rgb.blue = 0xffff;
            (*ctab)->ctTable[1].rgb.red = 0;
            (*ctab)->ctTable[1].rgb.green = 0;
            (*ctab)->ctTable[1].rgb.blue = 0;
        }
    }
    (*pm)->baseAddr = (Ptr)data;
    (*pm)->rowBytes = rowBytes | 0x8000;
    (*pm)->bounds = *bounds;
    (*pm)->pmVersion = 2;
    (*pm)->packType = 0;
    (*pm)->packSize = 0;
    (*pm)->hRes = 0x480000;
    (*pm)->vRes = 0x480000;
    (*pm)->pixelType = depth <= 8 ? 0 : RGBDirect;
    (*pm)->pixelSize = depth;
    (*pm)->cmpCount = depth <= 8 ? 1 : 3;
    (*pm)->cmpSize = depth <= 8 ? depth : depth == 16 ? 5 : 8;
#if TARGET_API_MAC_CARBON
    (*pm)->pmTable = ctab;
#else
    (*pm)->planeBytes = 0;
    (*pm)->pmTable = ctab;
    (*pm)->pmReserved = 0;
#endif
done:
    if (err != 0) {
        if (pm != 0)
            DisposeHandle((Handle)pm);
        if (data != 0)
            DisposeHandle(data);
        pm = 0;
    }
    return pm;
}

void DT_DisposeMacPixMap(PixMapHandle pm)
{
    if ((*pm)->baseAddr != 0) {
        UnlockPixels(pm);
        DisposeHandle((Handle)(*pm)->baseAddr);
    }
    DisposePixMap(pm);
}

void _DT_PaletteToMacCTab(CTabHandle ctab, const ITPalette *palette)
{
    ULong i;
    unsigned short r, g, b;

    GetHandleSize((Handle)ctab);
    (*ctab)->ctSeed = GetCTSeed();
    (*ctab)->ctSize = palette->count - 1;
    (*ctab)->ctFlags = 0;
    for (i = 0; i < palette->count; i++) {
        (*ctab)->ctTable[i].value = 0;
        r = palette->entry[i].red;
        (*ctab)->ctTable[i].rgb.red = r << 8 | r;
        g = palette->entry[i].green;
        (*ctab)->ctTable[i].rgb.green = g << 8 | g;
        b = palette->entry[i].blue;
        (*ctab)->ctTable[i].rgb.blue = b << 8 | b;
    }
}

void _DT_MacCTabToPalette(ITPalette *palette, CTabHandle ctab, unsigned char alpha)
{
    ULong i;

    palette->count = (*ctab)->ctSize + 1;
    for (i = 0; i < palette->count; i++) {
        if (alpha)
            palette->entry[i].alpha = 0xff;
        palette->entry[i].red = (*ctab)->ctTable[i].rgb.red >> 8;
        palette->entry[i].green = (*ctab)->ctTable[i].rgb.green >> 8;
        palette->entry[i].blue = (*ctab)->ctTable[i].rgb.blue >> 8;
    }
}

void _DT_MacCTabToInverse(unsigned char *table, CTabHandle ctab)
{
    ULong dist;
    ULong i;
    ULong j;
    unsigned short r;
    unsigned short g;
    unsigned short b;
    unsigned short qb;
    unsigned short qg;
    unsigned short qr;
    ULong best;
    ULong bestDist;
    ULong n;

    for (i = 0; i < kInverseTableSize; i++) {
        r = (i >> 8) & 0xf;
        r = r << 4 | r;
        g = (i >> 4) & 0xf;
        g = g << 4 | g;
        b = i & 0xf;
        b = b << 4 | b;
        j = best = 0;
        bestDist = 0xffffffff;
        n = (*ctab)->ctSize;
        for (; j <= n; j++) {
            qr = (*ctab)->ctTable[j].rgb.red >> 8;
            qg = (*ctab)->ctTable[j].rgb.green >> 8;
            qb = (*ctab)->ctTable[j].rgb.blue >> 8;
            dist = (r - qr) * (r - qr);
            dist += (g - qg) * (g - qg);
            dist += (b - qb) * (b - qb);
            if (dist < bestDist) {
                best = j;
                bestDist = dist;
            }
        }
        table[i] = best;
    }
}

unsigned char _DT_MacCTabMakeUnique(CTabHandle ctab, unsigned char index)
{
    unsigned char changed;
    unsigned char same;
    ULong attempt;
    ULong n;
    RGBColor *rgb;
    RGBColor *other;
    ULong j;

    changed = 0;
    attempt = 0;
    same = 1;
    rgb = &(*ctab)->ctTable[index].rgb;
    n = (*ctab)->ctSize;
    while (same) {
        for (j = 0, same = 0; !same && j < n; j++) {
            if (j != index) {
                other = &(*ctab)->ctTable[j].rgb;
                same = AsByte(other->red == rgb->red && other->green == rgb->green && other->blue == rgb->blue);
            }
        }
        if (same) {
            changed = 1;
            switch (attempt % 3) {
            case 0:
                rgb->red ^= attempt / 3;
                break;
            case 1:
                rgb->green ^= attempt / 3;
                break;
            case 2:
                rgb->blue ^= attempt / 3;
                break;
            }
        }
        attempt++;
    }
    if (changed)
        CTabChanged(ctab);
    return changed;
}

GDHandle DT_GetWindowDevice(CGrafPtr window)
{
    GDHandle device;
    GrafPtr old;
    Rect box;

#if TARGET_API_MAC_CARBON
    GetPort(&old);
    SetPortWindowPort((WindowRef)window);
    GetWindowPortBounds((WindowRef)window, &box);
    LocalToGlobal((Point *)&box.top);
    LocalToGlobal((Point *)&box.bottom);
    device = GetMaxDevice(&box);
    SetPort(old);
#else
    if (((WindowPeek)window)->visible && ((WindowPeek)window)->contRgn != 0 &&
        !EmptyRgn(((WindowPeek)window)->contRgn)) {
        box = (*((WindowPeek)window)->contRgn)->rgnBBox;
        device = GetMaxDevice(&box);
    } else {
        GetPort(&old);
        SetPort((GrafPtr)window);
        box = window->portRect;
        LocalToGlobal((Point *)&box.top);
        LocalToGlobal((Point *)&box.bottom);
        device = GetMaxDevice(&box);
        SetPort(old);
    }
#endif
    if (device == 0) {
        SetRect(&box, -kRegionLimit, -kRegionLimit, kRegionLimit, kRegionLimit);
        device = GetMaxDevice(&box);
    }
    return device;
}

ITImage *DT_SurfaceCreateFromDevice(GDHandle device, const ITRect *area)
{
    ITImage *image;
    ITPort *port;
    ITPixels *pixels;
    GWorldPtr world;
    long err;
    long e;
    long depth;
    Rect box;

    err = 0;
    port = 0;
    pixels = 0;
    world = 0;
    image = _DT_SurfaceCreate(&port, 0, &pixels);
    if (image == 0) {
        err = kOutOfMemory;
        goto done;
    }
    depth = (*(*device)->gdPMap)->pixelSize;
    if (area != 0)
        CopyDT_2SysRect(&box, area);
    else
        box = (*device)->gdRect;
    err = NewGWorld(&world, depth, &box, 0, device, (gDT_TempMem != 0 ? useTempMem : 0) | noNewDevice);
    if (err == 0) {
        err = QDError();
        if (err == 0 && (world == 0 || GetGWorldPixMap(world) == 0 || GetPixBaseAddr(GetGWorldPixMap(world)) == 0))
            err = memFullErr;
    }
    if (err != 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = _DT_SurfaceSet(image, world, device, 0, (area == 0 ? kPixelsWholeWindow : 0) | (gDT_TempMem != 0 ? kPixelsTempMem : 0),
                       kDeviceSurfaceExtra);
    if (e != 0) {
        err = e;
        goto done;
    }
    world = 0;
    e = _DT_ContextCreate(port, 0, (CGrafPtr)pixels->drawDC, 0);
    if (e != 0)
        err = e;
done:
    if (err != 0 && image != 0) {
        if (port->dc == 0) {
            image->port = 0;
            image->portSelf = 0;
        }
        _DT_SurfaceDispose(image, 0, 0, 0);
        port = 0;
        pixels = 0;
    }
    if (world != 0)
        DisposeGWorld(world);
    return pixels != 0 ? pixels->link.owner2 : 0;
}

ITImage *DT_SurfaceCreateFromWindow(CGrafPtr window)
{
    return DT_SurfaceCreateLikeWindow(window, 0);
}

ITImage *DT_SurfaceCreateLikeWindow(CGrafPtr window, const ITRect *area)
{
    ITImage *image;
    ITPort *port;
    ITPixels *pixels;
    GWorldPtr world;
    long err;
    long e;
    ULong extra;
    long depth;
    Rect box;
    GDHandle device;

    err = 0;
    extra = gDT_SurfaceExtra;
    port = 0;
    pixels = 0;
    world = 0;
    image = _DT_SurfaceCreate(&port, 0, &pixels);
    if (image == 0) {
        err = kOutOfMemory;
        goto done;
    }
#if TARGET_API_MAC_CARBON
    if (extra == 1 && !QDIsPortBuffered(GetWindowPort((WindowRef)window)))
        extra = 0;
#endif
    device = DT_GetWindowDevice(window);
    depth = (*(*device)->gdPMap)->pixelSize;
    depth = depth >= 8 ? depth : 0;
    if (area != 0)
        CopyDT_2SysRect(&box, area);
    else
#if TARGET_API_MAC_CARBON
        GetWindowPortBounds((WindowRef)window, &box);
    if (extra == 1) {
        world = GetWindowPort((WindowRef)window);
    } else
#else
        box = window->portRect;
#endif
    {
        err = NewGWorld(&world, depth, &box, 0, device, (gDT_TempMem != 0 ? useTempMem : 0) | noNewDevice);
        if (err == 0) {
            err = QDError();
            if (err == 0 && (world == 0 || GetGWorldPixMap(world) == 0 || GetPixBaseAddr(GetGWorldPixMap(world)) == 0))
                err = memFullErr;
        }
        if (err != 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    e = _DT_SurfaceSet(image, world, device, window, (area == 0 ? kPixelsWholeWindow : 0) | (gDT_TempMem != 0 ? kPixelsTempMem : 0), extra);
    if (e != 0) {
        err = e;
        goto done;
    }
    world = 0;
    e = _DT_ContextCreate(port, 0, (CGrafPtr)pixels->drawDC, 0);
    if (e != 0)
        err = e;
done:
    if (world != 0)
        DisposeGWorld(world);
    if (err != 0 && image != 0) {
        if (port->dc == 0) {
            image->port = 0;
            image->portSelf = 0;
        }
        _DT_SurfaceDispose(image, 0, 0, 0);
        port = 0;
        pixels = 0;
    }
    return pixels != 0 ? pixels->link.owner2 : 0;
}

long _DT_SurfaceSet(ITImage *image, GWorldPtr world, GDHandle device, CGrafPtr window, ULong flags, ULong extra)
{
    ITPixels *pixels;
    long err;
    unsigned short depth;
    Rect box;

    err = 0;
    pixels = image->pixels;
    depth = (*(*device)->gdPMap)->pixelSize;
#if TARGET_API_MAC_CARBON
    GetPortBounds(world, &box);
#else
    box = world->portRect;
#endif
    pixels->bytesPerPixel = _DT_ImageCalcFormat(depth, &pixels->formatFlag, pixels->bits,
                                                pixels->shift, pixels->mask, pixels->scale);
    pixels->depth = depth;
    pixels->flags = flags;
    if ((*device)->gdType == fixedType)
        pixels->flags |= kPixelsDirect;
    else
        pixels->flags &= ~kPixelsDirect;
    CopySys2DT_Rect(&pixels->bounds, &box);
    pixels->locks = 0;
    pixels->unk54 = (long)device;
    pixels->window = window;
#if TARGET_API_MAC_CARBON
    if (extra == 1) {
        if ((CGrafPtr)(pixels->drawDC = (long)GetWindowPort(pixels->window)) == 0) {
            err = kDibFailed;
            goto done;
        }
        if ((pixels->dib = (ITDib *)GetPortPixMap((CGrafPtr)pixels->drawDC)) == 0) {
            err = kDibFailed;
            goto done;
        }
    } else {
        pixels->drawDC = (long)world;
        if ((pixels->dib = (ITDib *)GetGWorldPixMap(world)) == 0) {
            err = kDibFailed;
            goto done;
        }
    }
    pixels->rowBytes = GetPixRowBytes(PM(pixels));
    pixels->baseAddr = 0;
    if (extra == 2)
        AtomicMaskApply(&pixels->flags, kPixelsBufferBit);
    else
        AtomicMaskClear(&pixels->flags, kPixelsBufferBit);
    if (pixels->depth > 8) {
        pixels->palette.count = 0;
    } else {
        _DT_MacCTabToPalette(&pixels->palette, (*PM(pixels))->pmTable, 1);
        _DT_MacCTabToInverse(pixels->inverse, (*PM(pixels))->pmTable);
    }
    pixels->info[0] = (*(*PM(pixels))->pmTable)->ctSeed;
    pixels->key = _DT_ImageCalcStamp(pixels);
    pixels->info[1] = extra;
done:
#else
    pixels->drawDC = (long)world;
    if ((pixels->dib = (ITDib *)GetGWorldPixMap(world)) == 0) {
        err = kDibFailed;
    } else {
        pixels->rowBytes = (*PM(pixels))->rowBytes & 0x3fff;
        pixels->baseAddr = 0;
        if (pixels->depth > 8) {
            pixels->palette.count = 0;
        } else {
            _DT_MacCTabToPalette(&pixels->palette, (*PM(pixels))->pmTable, 1);
            _DT_MacCTabToInverse(pixels->inverse, (*PM(pixels))->pmTable);
        }
        pixels->info[0] = (*(*PM(pixels))->pmTable)->ctSeed;
        pixels->key = _DT_ImageCalcStamp(pixels);
        pixels->info[1] = extra;
    }
#endif
    return err;
}

long DT_SurfaceDispose(ITImage *image)
{
    return _DT_SurfaceDispose(image, 0, 0, 0);
}

long DT_SurfaceBlitRects(ITImage *image, ULong count, const ITRect *rects)
{
    return DT_SurfaceCopyRects(image, count, rects, rects);
}

#if TARGET_API_MAC_CARBON
long DT_SurfaceCopyRects(ITImage *image, ULong count, const ITRect *src, const ITRect *dst)
{
    ITPixels *pixels;
    long err;
    long e;
    unsigned char locked;
    RgnHandle one;
    RgnHandle all;
    Point *corner;
    CGrafPtr port;
    PixMapHandle screen;
    GDHandle device;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    Rect from;
    Rect to;
    RGBColor oldFore;
    RGBColor oldBack;

    _DT_SurfaceGetContext(image);
    pixels = _DT_SurfaceGetImage(image);
    err = 0;
    locked = 0;
    one = NewRgn();
    all = one != 0 ? NewRgn() : 0;
    screen = 0;
    if (AtomicMaskTest(&pixels->flags, kPixelsBufferBit)) {
        screen = NewPixMap();
        if (screen == 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    if (count != 0) {
        port = GetWindowPort(pixels->window);
        device = GD(pixels) != 0 ? GD(pixels) : GetMainDevice();
        if (pixels->info[1] != 1) {
            e = DT_ImageLock(image->pixelsSelf, 0, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            locked = 1;
            GetGWorld(&oldWorld, &oldDevice);
            SetGWorld(port, device);
            GetForeColor(&oldFore);
            ForeColor(blackColor);
            GetBackColor(&oldBack);
            BackColor(whiteColor);
        }
        corner = (Point *)&to.bottom;
        while (count--) {
            CopyDT_2SysRect(&from, src++);
            CopyDT_2SysRect(&to, dst++);
            if (screen != 0) {
                LocalToGlobal((Point *)&to.top);
                LocalToGlobal(corner);
            } else if (all != 0) {
                RectRgn(one, &to);
                UnionRgn(one, all, all);
            }
            if (pixels->info[1] != 1)
                CopyBits((BitMap *)*PM(pixels), screen != 0 ? (BitMap *)*screen : GetPortBitMapForCopyBits(port), &from, &to, srcCopy, 0);
        }
        if (pixels->info[1] != 1) {
            RGBForeColor(&oldFore);
            RGBBackColor(&oldBack);
            SetGWorld(oldWorld, oldDevice);
        }
        if (screen == 0 && QDIsPortBuffered(port)) {
            QDGetDirtyRegion(port, one);
            UnionRgn(one, all, all);
            QDSetDirtyRegion(port, all);
            QDFlushPortBuffer(port, 0);
        }
    }
done:
    if (screen != 0)
        DisposePixMap(screen);
    if (one != 0)
        DisposeRgn(one);
    if (all != 0)
        DisposeRgn(all);
    if (locked)
        DT_ImageUnlock(image->pixelsSelf, 0);
    return err;
}
#else
long DT_SurfaceCopyRects(ITImage *image, ULong count, const ITRect *src, const ITRect *dst)
{
    ITPixels *pixels;
    long err;
    long e;
    unsigned char locked;
    CGrafPtr port;
    GDHandle device;
    CGrafPtr wmgr;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    Rect from;
    Rect to;
    RGBColor oldFore;
    RGBColor oldBack;

    _DT_SurfaceGetContext(image);
    pixels = _DT_SurfaceGetImage(image);
    err = 0;
    locked = 0;
    if (count != 0) {
        GetCWMgrPort(&wmgr);
        port = pixels->window != 0 ? (CGrafPtr)pixels->window : wmgr;
        device = GD(pixels) != 0 ? GD(pixels) : GetMainDevice();
        e = DT_ImageLock(image->pixelsSelf, 0, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        locked = 1;
        GetGWorld(&oldWorld, &oldDevice);
        SetGWorld(port, device);
        GetForeColor(&oldFore);
        ForeColor(blackColor);
        GetBackColor(&oldBack);
        BackColor(whiteColor);
        while (count--) {
            CopyDT_2SysRect(&from, src++);
            CopyDT_2SysRect(&to, dst++);
            CopyBits((BitMap *)*PM(pixels), (BitMap *)*port->portPixMap, &from, &to, srcCopy, 0);
        }
        RGBForeColor(&oldFore);
        RGBBackColor(&oldBack);
        SetGWorld(oldWorld, oldDevice);
    }
done:
    if (locked)
        DT_ImageUnlock(image->pixelsSelf, 0);
    return err;
}
#endif

ITImage *DT_ImageCreate(const ITRect *bounds, ULong format, const ITPalette *palette)
{
    ITImage *image;
    ITPixels *pixels;
    long err;
    CTabHandle ctab;
    ULong i;
    Rect clamped;

    err = 0;
    ctab = 0;
    pixels = 0;
    if (format == 8 && palette == 0)
        palette = &gDT_DefaultPalette;
    image = _DT_SurfaceCreate(0, 0, &pixels);
    if (image == 0) {
        err = kOutOfMemory;
        goto done;
    }
    pixels->bytesPerPixel = _DT_ImageCalcFormat(format, &pixels->formatFlag, pixels->bits,
                                                pixels->shift, pixels->mask, pixels->scale);
    pixels->depth = pixels->bytesPerPixel << 3;
    pixels->flags = gDT_TempMem != 0 ? kPixelsTempMem : 0;
    pixels->bounds = *bounds;
    pixels->locks = 0;
    if (format == 8) {
        ctab = (CTabHandle)NewHandle(0x808);
        if (ctab == 0) {
            MemError();
            err = kOutOfMemory;
            goto done;
        }
        _DT_PaletteToMacCTab(ctab, palette);
    }
    CopyDT_2SysRect(&clamped, bounds);
    if ((pixels->dib = (ITDib *)DT_CreateMacPixMap(&clamped, pixels->depth, ctab, gDT_TempMem)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
#if TARGET_API_MAC_CARBON
    pixels->rowBytes = GetPixRowBytes(PM(pixels));
#else
    pixels->rowBytes = (*PM(pixels))->rowBytes & 0x3fff;
#endif
    ctab = 0;
    pixels->baseAddr = 0;
    if (pixels->depth > 8) {
        pixels->palette.count = 0;
    } else {
        _DT_MacCTabToPalette(&pixels->palette, (*PM(pixels))->pmTable, 1);
        _DT_MacCTabToInverse(pixels->inverse, (*PM(pixels))->pmTable);
        if (palette != 0) {
            for (i = 0; i < pixels->palette.count; i++)
                pixels->palette.entry[i].alpha = palette->entry[i].alpha;
        }
    }
    pixels->info[0] = (*(*PM(pixels))->pmTable)->ctSeed;
    pixels->key = palette == &gDT_AlphaPalette ? kSystemPaletteKey : _DT_ImageCalcStamp(pixels);
done:
    if (ctab != 0)
        DisposeHandle((Handle)ctab);
    if (err != 0 && pixels != 0) {
        _DT_SurfaceDispose(image, 0, 0, 0);
        pixels = 0;
    }
    return pixels != 0 ? pixels->link.self : 0;
}

long DT_ImageDispose(ITImage *image)
{
    return _DT_SurfaceDispose(image, 0, 0, 1);
}

void _DT_ImageDispose(ITPixels *pixels)
{
    if ((GWorldPtr)pixels->drawDC != 0)
        DisposeGWorld((GWorldPtr)pixels->drawDC);
    else if (pixels->dib != 0)
        DT_DisposeMacPixMap(PM(pixels));
    pixels->unk54 = 0;
    pixels->window = 0;
    pixels->drawDC = 0;
    pixels->dib = 0;
    pixels->baseAddr = 0;
}

long DT_SurfaceCheck(ITImage *image, unsigned char *rebuilt)
{
    ITPixels *pixels;
    long err;
    unsigned char saved;
    unsigned char stale;
    unsigned char changed;
    ITImage *fresh;
    unsigned char swapped;
    ULong depth;
    Rect box;
    ITRect rect;
    ITPalette palette;
#if TARGET_API_MAC_CARBON
    Rect other;
    long e;
#endif

    pixels = _DT_SurfaceGetImage(image);
    saved = gDT_TempMem;
    err = 0;
    stale = 0;
    changed = 0;
    fresh = 0;
    swapped = 0;
    if (rebuilt != 0)
        *rebuilt = 0;
#if TARGET_API_MAC_CARBON
    if (pixels->info[1] == 1 && pixels->window != 0) {
        if ((CGrafPtr)(pixels->drawDC = (long)GetWindowPort(pixels->window)) == 0) {
            err = kDibFailed;
            goto done;
        }
        if ((e = LockPortBits((CGrafPtr)pixels->drawDC)) != 0) {
            err = e;
            goto done;
        }
        if ((pixels->dib = (ITDib *)GetPortPixMap((CGrafPtr)pixels->drawDC)) == 0) {
            err = kDibFailed;
            goto done;
        }
        pixels->rowBytes = GetPixRowBytes(PM(pixels));
    }
#endif
    CopyDT_2SysRect(&box, &pixels->bounds);
    if (pixels->window != 0) {
#if TARGET_API_MAC_CARBON
        if ((pixels->flags & kPixelsWholeWindow) != 0 && pixels->info[1] != 1 && !EqualRect(&box, &(*PM(pixels))->bounds))
            changed = 1;
        if ((pixels->flags & kPixelsWholeWindow) != 0 &&
            (!EqualRect(&box, GetPortBounds((CGrafPtr)pixels->drawDC, &other)) ||
             !EqualRect(&box, GetWindowPortBounds(pixels->window, &other))))
            changed = 1;
        CopySys2DT_Rect(&rect, GetWindowPortBounds(pixels->window, &other));
#else
        if ((pixels->flags & kPixelsWholeWindow) != 0 && !EqualRect(&box, &(*PM(pixels))->bounds))
            changed = 1;
        if ((pixels->flags & kPixelsWholeWindow) != 0 &&
            (!EqualRect(&box, &((GWorldPtr)pixels->drawDC)->portRect) ||
             !EqualRect(&box, &((CGrafPtr)pixels->window)->portRect)))
            changed = 1;
        CopySys2DT_Rect(&rect, &((CGrafPtr)pixels->window)->portRect);
#endif
        if (pixels->depth != (*PM(pixels))->pixelSize ||
            GD(pixels) != DT_GetWindowDevice(pixels->window) ||
            pixels->info[0] != (*(*PM(pixels))->pmTable)->ctSeed)
            stale = 1;
        depth = (*PM(pixels))->pixelSize;
    } else if (GD(pixels) != 0) {
        if ((pixels->flags & kPixelsWholeWindow) != 0 &&
            (!EqualRect(&box, &(*PM(pixels))->bounds) || !EqualRect(&box, &(*GD(pixels))->gdRect)))
            changed = 1;
        CopySys2DT_Rect(&rect, &(*GD(pixels))->gdRect);
        if (pixels->depth != (*PM(pixels))->pixelSize || pixels->info[0] != (*(*PM(pixels))->pmTable)->ctSeed)
            stale = 1;
        depth = (*PM(pixels))->pixelSize;
    } else {
        if (!EqualRect(&box, &(*PM(pixels))->bounds))
            changed = 1;
        CopySys2DT_Rect(&rect, &(*PM(pixels))->bounds);
        if (pixels->depth != (*PM(pixels))->pixelSize || pixels->info[0] != (*(*PM(pixels))->pmTable)->ctSeed)
            stale = 1;
        depth = (*PM(pixels))->pixelSize;
    }
    if (changed || stale) {
#if TARGET_API_MAC_CARBON
        if (pixels->info[1] == 1 && pixels->window != 0) {
            pixels->unk54 = (long)DT_GetWindowDevice(pixels->window);
            pixels->bytesPerPixel = _DT_ImageCalcFormat(depth, &pixels->formatFlag, pixels->bits,
                                                        pixels->shift, pixels->mask, pixels->scale);
            pixels->rowBytes = GetPixRowBytes(PM(pixels));
            pixels->bounds = rect;
            pixels->depth = depth;
            if (pixels->depth > 8) {
                pixels->palette.count = 0;
            } else {
                _DT_MacCTabToPalette(&pixels->palette, (*PM(pixels))->pmTable, 1);
                _DT_MacCTabToInverse(pixels->inverse, (*PM(pixels))->pmTable);
            }
            pixels->info[0] = (*(*PM(pixels))->pmTable)->ctSeed;
            pixels->key = _DT_ImageCalcStamp(pixels);
        } else
#endif
        if (pixels->window != 0) {
            fresh = DT_SurfaceCreateLikeWindow(pixels->window, (pixels->flags & kPixelsWholeWindow) != 0 ? 0 : &pixels->bounds);
            if (fresh == 0) {
                err = kOutOfMemory;
                goto done;
            }
        } else if (GD(pixels) != 0) {
            fresh = DT_SurfaceCreateFromDevice(GD(pixels), (pixels->flags & kPixelsWholeWindow) != 0 ? 0 : &pixels->bounds);
            if (fresh == 0) {
                err = kOutOfMemory;
                goto done;
            }
        } else {
            palette.count = 0;
            if (depth == 8)
                _DT_MacCTabToPalette(&palette, (*PM(pixels))->pmTable, 1);
            gDT_TempMem = pixels->flags & kPixelsTempMem;
            fresh = DT_ImageCreate(&rect, depth, &palette);
            if (fresh == 0) {
                err = kOutOfMemory;
                goto done;
            }
            swapped = 1;
        }
        if (fresh != 0) {
            if (swapped)
                _DT_ImageReplace(image->pixelsSelf, fresh->pixelsSelf);
            else
                _DT_SurfaceReplace(image, fresh);
        }
    }
done:
#if TARGET_API_MAC_CARBON
    if (pixels->info[1] == 1 && pixels->window != 0) {
        if ((CGrafPtr)(pixels->drawDC = (long)GetWindowPort(pixels->window)) == 0)
            err = kDibFailed;
        if ((e = UnlockPortBits((CGrafPtr)pixels->drawDC)) != 0)
            err = e;
        if ((pixels->dib = (ITDib *)GetPortPixMap((CGrafPtr)pixels->drawDC)) == 0)
            err = kDibFailed;
        CopySys2DT_Rect(&pixels->bounds, GetWindowPortBounds(pixels->window, &other));
        pixels->rowBytes = GetPixRowBytes(PM(pixels));
    }
#endif
    if (fresh != 0)
        _DT_SurfaceDispose(fresh, 0, 0, swapped);
    if (rebuilt != 0)
        *rebuilt = changed || stale;
    gDT_TempMem = saved;
    return err;
}

long _DT_ImageLock(ITImage *image, ITImage *other, unsigned char *rebuilt)
{
    ITPixels *pixels;
    ITPixels *otherPixels;
    long err;
    long e;
#if TARGET_API_MAC_CARBON
    PixMapPtr map;
#endif
    ITPort *port;
    CGrafPtr window;
    GDHandle device;
    CGrafPtr otherWindow;
    GDHandle otherDevice;
    GWorldPtr world;
    GWorldPtr otherWorld;
#if TARGET_API_MAC_CARBON
    Rect bits;
#endif
    Rect box;
    Rect otherBox;
    unsigned char changed;
#if TARGET_API_MAC_CARBON
    long f;
    char *base;
#endif

    pixels = _DT_ImageGetImage(image);
    otherPixels = _DT_ImageGetAlpha(other);
    err = 0;
    if (rebuilt != 0)
        *rebuilt = 0;
    changed = 0;
    if (pixels->locks == 0) {
        e = DT_SurfaceCheck(image, &changed);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (changed)
            *rebuilt = 1;
#if TARGET_API_MAC_CARBON
        if (pixels->info[1] == 2 && pixels->window != 0)
            AtomicMaskApply(&pixels->flags, kPixelsBufferBit);
        if (pixels->info[1] == 1 && pixels->window != 0) {
            if ((CGrafPtr)(pixels->drawDC = (long)GetWindowPort(pixels->window)) == 0) {
                err = kDibFailed;
                goto done;
            }
            if ((f = LockPortBits((CGrafPtr)pixels->drawDC)) != 0) {
                err = f;
                goto done;
            }
            if ((pixels->dib = (ITDib *)GetPortPixMap((CGrafPtr)pixels->drawDC)) == 0) {
                err = kDibFailed;
                goto done;
            }
            pixels->rowBytes = GetPixRowBytes(PM(pixels));
        } else
#endif
        if (changed || !LockPixels(PM(pixels))) {
            window = pixels->window;
            world = (GWorldPtr)pixels->drawDC;
            device = GD(pixels);
            GetPixelsState(PM(pixels));
            port = pixels->link.owner->port;
            if (window != 0)
                device = DT_GetWindowDevice(window);
            if ((pixels->flags & kPixelsWholeWindow) != 0) {
                if (window != 0)
#if TARGET_API_MAC_CARBON
                    GetWindowPortBounds((WindowRef)window, &box);
#else
                    box = window->portRect;
#endif
                else
                    box = (*device)->gdRect;
            } else {
#if TARGET_API_MAC_CARBON
                GetPortBounds((GWorldPtr)pixels->drawDC, &box);
#else
                box = ((GWorldPtr)pixels->drawDC)->portRect;
#endif
            }
            _DT_ContextDispose(port);
            if (UpdateGWorld(&world, 0, &box, 0, device, 0) & gwFlagErr) {
                err = kOutOfMemory;
                goto done;
            }
            e = _DT_SurfaceSet(pixels->link.owner, world, device, window, pixels->flags, pixels->info[1]);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = _DT_ContextCreate(port, 0, (CGrafPtr)pixels->drawDC, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (!LockPixels(PM(pixels))) {
                err = kDibFailed;
                goto done;
            }
            if (rebuilt != 0)
                *rebuilt = 1;
        }
        changed = 0;
        if (otherPixels != 0) {
            e = DT_SurfaceCheck(other, &changed);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (changed)
                *rebuilt = 1;
        }
        if (otherPixels != 0 && (changed || !LockPixels(PM(otherPixels)))) {
            otherWindow = pixels->window;
            otherWorld = (GWorldPtr)pixels->drawDC;
            otherDevice = GD(pixels);
            GetPixelsState(PM(otherPixels));
            port = otherPixels->link.owner->port;
            if (otherWindow != 0)
                otherDevice = DT_GetWindowDevice(otherWindow);
            if ((otherPixels->flags & kPixelsWholeWindow) != 0) {
                if (otherWindow != 0)
#if TARGET_API_MAC_CARBON
                    GetWindowPortBounds((WindowRef)otherWindow, &otherBox);
#else
                    otherBox = otherWindow->portRect;
#endif
                else
                    otherBox = (*otherDevice)->gdRect;
            } else {
#if TARGET_API_MAC_CARBON
                GetPortBounds((GWorldPtr)otherPixels->drawDC, &otherBox);
#else
                otherBox = ((GWorldPtr)otherPixels->drawDC)->portRect;
#endif
            }
            _DT_ContextDispose(port);
            if (UpdateGWorld(&otherWorld, 0, &otherBox, 0, otherDevice, 0) & gwFlagErr) {
                err = kOutOfMemory;
                goto done;
            }
            e = _DT_SurfaceSet(otherPixels->link.owner, otherWorld, otherDevice, otherWindow, otherPixels->flags, otherPixels->info[1]);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = _DT_ContextCreate(port, 0, (CGrafPtr)otherPixels->drawDC, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (!LockPixels(PM(otherPixels))) {
                err = kDibFailed;
                goto done;
            }
            if (rebuilt != 0)
                *rebuilt = 1;
        }
#if TARGET_API_MAC_CARBON
        if (pixels->info[1] == 1 && pixels->window != 0) {
            GetPortBounds((CGrafPtr)pixels->drawDC, &bits);
            map = *PM(pixels);
            base = GetPixBaseAddr(PM(pixels));
            pixels->baseAddr = (ULong)base + pixels->rowBytes * (bits.top - map->bounds.top) +
                               pixels->bytesPerPixel * (bits.left - map->bounds.left);
        } else
#endif
        pixels->baseAddr = (ULong)GetPixBaseAddr(PM(pixels));
        AtomicMaskClear(&pixels->flags, kPixelsLockBit);
        if (otherPixels != 0) {
            otherPixels->baseAddr = (ULong)GetPixBaseAddr(PM(otherPixels));
            AtomicMaskClear(&otherPixels->flags, kPixelsLockBit);
        }
    }
    AtomicIncrement(&pixels->locks, 1);
    if (otherPixels != 0)
        AtomicIncrement(&otherPixels->locks, 1);
done:
    return err;
}

void _DT_ImageUnlock(ITImage *image, ITImage *other)
{
    ITPixels *pixels;
    ITPixels *otherPixels;

    pixels = _DT_ImageGetImage(image);
    otherPixels = _DT_ImageGetAlpha(other);
    if (otherPixels != 0 && AtomicIncrement(&otherPixels->locks, -1) == 1) {
        otherPixels->baseAddr = 0;
        AtomicMaskClear(&otherPixels->flags, kPixelsLockBit);
        UnlockPixels(PM(otherPixels));
    }
    if (AtomicIncrement(&pixels->locks, -1) == 1) {
        pixels->baseAddr = 0;
        AtomicMaskClear(&pixels->flags, kPixelsLockBit);
#if TARGET_API_MAC_CARBON
        if (pixels->info[1] == 1 && pixels->window != 0) {
            pixels->drawDC = (long)GetWindowPort(pixels->window);
            UnlockPortBits((CGrafPtr)pixels->drawDC);
            pixels->dib = (ITDib *)GetPortPixMap((CGrafPtr)pixels->drawDC);
            pixels->rowBytes = GetPixRowBytes(PM(pixels));
        } else
#endif
        UnlockPixels(PM(pixels));
    }
}
