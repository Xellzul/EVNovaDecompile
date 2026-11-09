/* atvideoplatmac.c -- 1.0.10: the Monitor Tool's Toolbox layer: the menu bar, colour and gamma tables, the Display Manager. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#if !TARGET_API_MAC_CARBON
#include "atvideomac.h"

static RGBColor gVTBlack;
static unsigned char sVTStripShown = 1;
static short sVTSavedBarHeight;
static RgnHandle sVTSavedGray;
static pascal void VT_ModeProc(void *user, DMListIndexType index, DMDisplayModeListEntryPtr info);
static RoutineDescriptor sVTModeProcRD = BUILD_ROUTINE_DESCRIPTOR(0xfc0, VT_ModeProc);
static DMDisplayModeListIteratorUPP sVTModeUPP = (DMDisplayModeListIteratorUPP)&sVTModeProcRD;
static unsigned short kVTSaveCode[12] = { 0x41fa, 0x0010, 0x209f, 0x2003, 0x41fa, 0x0008, 0x2f10, 0x4e75 };
static unsigned short kVTRestoreCode[12] = { 0x41fa, 0x0010, 0x209f, 0x2600, 0x41fa, 0x0008, 0x2f10, 0x4e75 };
static unsigned short kVTStripVisibleCode[12] = { 0x41fa, 0x0012, 0x209f, 0x7000, 0xaaf2, 0x41fa, 0x0008, 0x2f10, 0x4e75 };
static unsigned short kVTStripShowCode[13] = { 0x41fa, 0x0014, 0x209f, 0x303c, 0x0101, 0xaaf2, 0x41fa, 0x0008, 0x2f10, 0x4e75 };

long VT_CallSave(void)
{
    return CallUniversalProc((UniversalProcPtr)kVTSaveCode, 0x32);
}

void VT_CallRestore(long value)
{
    CallUniversalProc((UniversalProcPtr)kVTRestoreCode, 0x1802, value);
}

unsigned char VT_StripVisible(void)
{
    return CallUniversalProc((UniversalProcPtr)kVTStripVisibleCode, 0xd8, 0);
}

void VT_StripShow(unsigned char show)
{
    CallUniversalProc((UniversalProcPtr)kVTStripShowCode, 0x1c8, 1, show);
}

long _ShowMenuBar(void)
{
    long err;
    RgnHandle shown;
    RgnHandle bar;
    GrafPtr wmgr;
    RgnHandle gray;
    GrafPtr port;
    RgnHandle clip;
    Rect r;
    RGBColor fore;
    RGBColor black = gVTBlack;
    long response;
    EventRecord event;
    WindowPtr front;

    err = 0;
    shown = 0;
    bar = 0;
    clip = 0;
    if (GetMBarHeight() <= 0 && sVTSavedBarHeight > 0) {
        if ((shown = NewRgn()) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((bar = NewRgn()) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((clip = NewRgn()) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        gray = GetGrayRgn();
        GetPort(&port);
        GetWMgrPort(&wmgr);
        LMSetMBarHeight(sVTSavedBarHeight);
        if (sVTSavedGray != 0) {
            r = (*GetMainDevice())->gdRect;
            r.bottom = r.top + sVTSavedBarHeight;
            RectRgn(bar, &r);
            DiffRgn(gray, sVTSavedGray, shown);
            DiffRgn(shown, bar, shown);
            SetPort(wmgr);
            GetClip(clip);
            SetClip(shown);
            GetForeColor(&fore);
            RGBForeColor(&black);
            PaintRgn(shown);
            RGBForeColor(&fore);
            SetClip(clip);
            SetPort(port);
            UnionRgn(shown, bar, shown);
            CopyRgn(sVTSavedGray, gray);
            DisposeRgn(sVTSavedGray);
            sVTSavedGray = 0;
        }
        front = FrontWindow();
        if (front != 0) {
            PaintBehind(front, shown);
            CalcVisBehind(front, shown);
        }
        DrawMenuBar();
        SetPort(port);
        sVTSavedBarHeight = 0;
        if (Gestalt('sdev', &response) == 0 && (response & 1) != 0) {
            if (sVTStripShown) {
                if (!VT_StripVisible())
                    VT_StripShow(1);
            }
            sVTStripShown = 1;
        }
        EventAvail(0, &event);
        EventAvail(0, &event);
        EventAvail(0, &event);
        EventAvail(0, &event);
    }
done:
    if (shown != 0)
        DisposeRgn(shown);
    if (bar != 0)
        DisposeRgn(bar);
    if (clip != 0)
        DisposeRgn(clip);
    return err;
}

long _HideMenuBar(void)
{
    long err;
    short height;
    RgnHandle hidden;
    RgnHandle saved;
    RgnHandle gray;
    GDHandle gd;
    GrafPtr port;
    long response;
    EventRecord event;
    WindowPtr front;

    err = 0;
    hidden = 0;
    saved = 0;
    height = GetMBarHeight();
    if (height > 0) {
        if ((saved = NewRgn()) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((hidden = NewRgn()) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((sVTSavedGray = NewRgn()) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        gray = GetGrayRgn();
        GetPort(&port);
        CopyRgn(gray, saved);
        if (Gestalt('sdev', &response) == 0 && (response & 1) != 0) {
            if ((sVTStripShown = VT_StripVisible()) != 0)
                VT_StripShow(0);
        }
        DiffRgn(gray, saved, hidden);
        CopyRgn(gray, sVTSavedGray);
        sVTSavedBarHeight = height;
        LMSetMBarHeight(0);
        for (gd = GetDeviceList(); gd != 0; gd = GetNextDevice(gd)) {
            if (TestDeviceAttribute(gd, screenDevice) && TestDeviceAttribute(gd, screenActive)) {
                RectRgn(saved, &(*gd)->gdRect);
                DiffRgn(saved, gray, saved);
                UnionRgn(saved, hidden, hidden);
            }
        }
        UnionRgn(hidden, gray, gray);
        front = FrontWindow();
        if (front != 0) {
            PaintBehind(front, hidden);
            CalcVisBehind(front, hidden);
        }
        SetPort(port);
        EventAvail(0, &event);
        EventAvail(0, &event);
        EventAvail(0, &event);
        EventAvail(0, &event);
    }
done:
    if (saved != 0)
        DisposeRgn(saved);
    if (hidden != 0)
        DisposeRgn(hidden);
    return err;
}

void VT_LockDevices(void)
{
    GDHandle gd;

    for (gd = GetDeviceList(); gd != 0; gd = GetNextDevice(gd)) {
        if (TestDeviceAttribute(gd, screenDevice) && TestDeviceAttribute(gd, screenActive)) {
            if ((HGetState((Handle)gd) & 0x80) == 0)
                HLock((Handle)gd);
        }
    }
}

#define LMPortList (*(short ***)0x0d66)

unsigned char VT_CheckScreens(void)
{
    long offset;
    short **list;
    ULong i;
    CGrafPtr port;
    CGrafPtr wmgr;
    Ptr base;
    Rect r;

    list = LMPortList;
    _AtomicEnter();
    GetCWMgrPort(&wmgr);
    base = GetPixBaseAddr(wmgr->portPixMap);
    for (i = 0, offset = 0; i < **list; offset += 4, i++) {
        port = *(CGrafPtr *)((char *)*list + offset + 2);
        if (port->device != 0)
            break;
        if (port->portVersion & 0x8000) {
            if (port->portPixMap == 0)
                break;
            if (*port->portPixMap == 0)
                break;
            if (HandleZone((Handle)port->portPixMap) == 0)
                break;
            if (MemError() != 0)
                break;
            if ((ULong)GetPixBaseAddr(port->portPixMap) > (ULong)base)
                break;
        } else {
            if (((GrafPtr)port)->portBits.baseAddr == 0)
                break;
            if ((ULong)((GrafPtr)port)->portBits.baseAddr > (ULong)base)
                break;
        }
        r = port->portRect;
        if (r.left > r.right)
            break;
        if (r.top > r.bottom)
            break;
        if (port->visRgn == 0)
            break;
        if (*port->visRgn == 0)
            break;
        if (HandleZone((Handle)port->visRgn) == 0)
            break;
        if (MemError() != 0)
            break;
        if (port->clipRgn == 0)
            break;
        if (*port->clipRgn == 0)
            break;
        if (HandleZone((Handle)port->clipRgn) == 0)
            break;
        if (MemError() != 0)
            break;
    }
    _AtomicLeave();
    return 1;                           /* BUG: the walk's break is dropped, a bad port still reads as good */
}

VTClut *VT_ClutCopy(VTDevice *device)
{
    VTClut *clut;
    ULong count;
    CTabHandle table;
    ULong i;

    clut = 0;
    if ((*(*device->gd)->gdPMap)->pixelSize <= 8) {
        count = 1 << (*(*device->gd)->gdPMap)->pixelSize;
        if ((clut = MemoryAllocate((count - 1) * sizeof(VTColor) + sizeof(VTClut))) != 0) {
            clut->count = count;
            table = (*(*device->gd)->gdPMap)->pmTable;
            for (i = 0; i < count; i++) {
                clut->entries[i].red = (*table)->ctTable[i].rgb.red << 16;
                clut->entries[i].green = (*table)->ctTable[i].rgb.green << 16;
                clut->entries[i].blue = (*table)->ctTable[i].rgb.blue << 16;
            }
        }
    }
    return clut;
}

long VT_ClutSet(VTDevice *device, VTClut *clut)
{
    GDHandle saved;
    CTabHandle table;
    ULong count;
    ULong i;
    SInt8 state;

    saved = GetGDevice();
    SetGDevice(device->gd);
    table = (*(*device->gd)->gdPMap)->pmTable;
    count = (*table)->ctSize + 1;
    if (clut != 0) {
        for (i = 0; i < count; i++) {
            (*table)->ctTable[i].value = i;
            (*table)->ctTable[i].rgb.red = clut->entries[i].red >> 16;
            (*table)->ctTable[i].rgb.green = clut->entries[i].green >> 16;
            (*table)->ctTable[i].rgb.blue = clut->entries[i].blue >> 16;
        }
    } else {
        RestoreDeviceClut(device->gd);
        goto done;
    }
    state = HGetState((Handle)table);
    HLock((Handle)table);
    SetEntries(-1, count - 1, (*table)->ctTable);
    HSetState((Handle)table, state);
    MakeITable(0, 0, 0);
    (*table)->ctSeed = GetCTSeed();
done:
    if (saved != 0)
        SetGDevice(saved);
    return 0;
}

static inline void VTCopy(void *dst, const void *src, ULong n)
{
    if (n != 0)
        BlockMoveData(src, dst, n);
}

GammaTblPtr VT_GammaGet(VTDevice *device)
{
    GammaTblPtr copy;
    long size;
    ULong data;
    long head;
    GammaTblPtr table;
    CntrlParam pb;

    copy = 0;
    table = 0;
    MemoryClear(&pb, sizeof(CntrlParam));
    pb.csCode = cscGetGamma;
    pb.ioCRefNum = (*device->gd)->gdRefNum;
    *(GammaTblPtr **)pb.csParam = &table;
    if (PBStatusSync((ParmBlkPtr)&pb) != 0)
        goto done;
    head = table->gFormulaSize + kGammaHead;
    data = table->gDataCnt * ((table->gDataWidth + 7) / 8);
    size = head + table->gChanCnt * data;
    if ((copy = MemoryAllocate(head + data * 3)) == 0)
        goto done;
    VTCopy(copy, table, size);
    if (table->gChanCnt == 1 && copy->gChanCnt != 3) {
        copy->gChanCnt = 3;
        VTCopy((char *)copy + head + data, (char *)table + head, data);
        VTCopy((char *)copy + head + data * 2, (char *)table + head, data);
    }
done:
    return copy;
}

long VT_GammaSet(VTDevice *device, GammaTblPtr table)
{
    long err;
    char state;
    GDHandle saved;
    CTabHandle clut;
    CntrlParam pb;

    err = 0;
    MemoryClear(&pb, sizeof(CntrlParam));
    pb.csCode = cscSetGamma;
    pb.ioCRefNum = (*device->gd)->gdRefNum;
    *(GammaTblPtr **)pb.csParam = &table;
    if (PBControlSync((ParmBlkPtr)&pb) != 0) {
        err = kVideoError;
        goto done;
    }
    saved = GetGDevice();
    SetGDevice(device->gd);
    clut = (*(*device->gd)->gdPMap)->pmTable;
    state = HGetState((Handle)clut);
    HLock((Handle)clut);
    SetEntries(0, (*clut)->ctSize, (*clut)->ctTable);
    HSetState((Handle)clut, state);
    SetGDevice(saved);
done:
    return err;
}

GammaTblPtr VT_GammaCopy(VTDevice *device, void *ref)
{
    GammaTblPtr table;
    GammaTblPtr copy;
    ULong size;

    table = (GammaTblPtr)ref;
    size = kGammaHead + table->gFormulaSize + ((table->gDataWidth + 7) / 8) * (table->gChanCnt * table->gDataCnt);
    if ((copy = MemoryAllocate(size)) != 0)
        VTCopy(copy, table, size);
    return copy;
}

void VT_GammaFree(VTDevice *device, GammaTblPtr table)
{
    MemoryDeallocate(table);
}

void VT_GammaBlend(VTDevice *device, GammaTblPtr out, GammaTblPtr from, GammaTblPtr to, ULong num, ULong den)
{
    ULong most;
    ULong width;
    long head;
    unsigned char *a;
    unsigned char *b;
    unsigned char *o;
    ULong chan;
    ULong i;
    long delta;

    width = out->gFormulaSize;
    head = width + kGammaHead;
    width = out->gDataWidth;
    most = (1 << width) - 1;
    most++;
    a = (unsigned char *)from + head;
    b = (unsigned char *)to + head;
    o = (unsigned char *)out + head;
    if (den > most) {
        num = (most * num) / den;
        den = most;
    }
    if (width <= 8) {
        for (chan = 0; chan < out->gChanCnt; chan++) {
            for (i = 0; i < out->gDataCnt; i++) {
                delta = *b++ - *a;
                *o++ = *a++ + (num * delta) / den;
            }
        }
    } else if (width <= 16) {
        for (chan = 0; chan < out->gChanCnt; chan++) {
            for (i = 0; i < out->gDataCnt; i++) {
                delta = *((unsigned short *)b)++ - *(unsigned short *)a;
                *((unsigned short *)o)++ = *((unsigned short *)a)++ + (num * delta) / den;
            }
        }
    } else {
        for (chan = 0; chan < out->gChanCnt; chan++) {
            for (i = 0; i < out->gDataCnt; i++) {
                delta = *((ULong *)b)++ - *(ULong *)a;
                *((ULong *)o)++ = *((ULong *)a)++ + (num * delta) / den;
            }
        }
    }
}

void VT_GammaFill(VTDevice *device, GammaTblPtr table, VTMode *color)
{
    unsigned char *o;
    ULong *p;
    ULong chan;
    ULong i;
    ULong level[3];
    ULong width;
    ULong head;

    head = table->gFormulaSize + kGammaHead;
    o = (unsigned char *)table + head;
    width = table->gDataWidth;
    level[0] = color->index;
    level[1] = color->depth;
    level[2] = *(ULong *)&color->hasColor;
    if (width <= 8) {
        for (p = level, chan = 0; chan < table->gChanCnt; p++, chan++) {
            for (i = 0; i < table->gDataCnt; i++)
                *o++ = *p >> (32 - table->gDataWidth);
        }
    } else if (width <= 16) {
        for (p = level, chan = 0; chan < table->gChanCnt; p++, chan++) {
            for (i = 0; i < table->gDataCnt; i++) {
                *(unsigned short *)o = *p >> (32 - table->gDataWidth);
                o += 2;
            }
        }
    } else {
        for (p = level, chan = 0; chan < table->gChanCnt; p++, chan++) {
            for (i = 0; i < table->gDataCnt; i++) {
                *(ULong *)o = *p >> (32 - table->gDataWidth);
                o += 4;
            }
        }
    }
}

#define kDepthStep (1 + 1)

static inline void VTStrCat(unsigned char *dst, const unsigned char *src)
{
    unsigned char *p;
    const unsigned char *q;

    p = dst - 1;
    q = src - 1;
    while (*++p != 0)
        ;
    p--;
    while ((*++p = *++q) != 0)
        ;
}

static inline long VTStrLen(const char *s)
{
    long n;
    const unsigned char *p;

    p = (const unsigned char *)s - 1;
    n = -1;
    do {
        n++;
    } while (*++p != 0);
    return n;
}

long VT_DeviceModes(VTDevice *device)
{
    long err;
    DMListType list;
    DMListIndexType count;
    ULong i;
    VTModeEntry *entry;
    ULong depth;
    long e;
    VDResolutionInfoRec resolution;
    CntrlParam pb;

    err = 0;
    list = 0;
    if (VT_HasDisplayManager()) {
        if (DMGetDisplayIDByGDevice(device->gd, &device->displayID, 0) != 0) {
            err = kVideoError;
            goto done;
        }
        if (DMNewDisplayModeList(device->displayID, 0, 0, &count, &list) != 0) {
            err = kVideoError;
            goto done;
        }
        if ((device->modes = MemoryAllocate(count * sizeof(VTModeEntry))) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        MemoryClear(device->modes, count * sizeof(VTModeEntry));
        device->modeCount = count;
        for (i = 0; i < count; i++) {
            if (DMGetIndexedDisplayModeFromList(list, i, 0, sVTModeUPP, device) != 0) {
                err = kVideoError;
                goto done;
            }
        }
    } else {
        if ((device->modes = MemoryAllocate(sizeof(VTModeEntry))) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        MemoryClear(device->modes, sizeof(VTModeEntry));
        device->modeCount = 1;
        entry = device->modes;
        entry->width = (*device->gd)->gdRect.right - (*device->gd)->gdRect.left;
        entry->height = (*device->gd)->gdRect.bottom - (*device->gd)->gdRect.top;
        entry->rate = 0;
        entry->displayMode = *(long *)&(*device->gd)->gdMode;
        entry->depthMin = 32;
        entry->depthMax = 1;
        for (depth = 1; depth <= 32; depth *= kDepthStep) {
            if (HasDepth(device->gd, depth, 0, 0) != 0) {
                entry->depths |= depth;
                if (entry->depthMin > depth)
                    entry->depthMin = depth;
                if (entry->depthMax < depth)
                    entry->depthMax = depth;
            }
        }
        MemoryClear(&pb, sizeof(CntrlParam));
        pb.csCode = cscGetNextResolution;
        pb.ioCRefNum = (*device->gd)->gdRefNum;
        *(VDResolutionInfoRec **)pb.csParam = &resolution;
        MemoryClear(&resolution, sizeof(VDResolutionInfoRec));
        resolution.csPreviousDisplayModeID = 0;
        e = PBStatusSync((ParmBlkPtr)&pb);
        if (e != 0)
            err = e;
        entry->rate = err == 0 ? Fix2Long(resolution.csRefreshRate) : 60;
        StringFromNumber(entry->name, entry->width);
        VTStrCat((unsigned char *)entry->name, (const unsigned char *)" x ");
        StringFromNumber(entry->name + VTStrLen(entry->name), entry->height);
        if (err == 0) {
            VTStrCat((unsigned char *)entry->name, (const unsigned char *)", ");
            StringFromNumber(entry->name + VTStrLen(entry->name), entry->rate);
        }
        err = 0;
    }
done:
    if (list != 0)
        DMDisposeList(list);
    if (err != 0 && device->modes != 0) {
        MemoryDeallocate(device->modes);
        device->modes = 0;
        device->modeCount = 0;
    }
    return err;
}

long VT_DMCurrentMode(VTDevice *device, VTMode *mode)
{
    ULong count;
    long err;
    ULong i;
    VDSwitchInfoRec info;

    err = 0;
    MemoryClear(mode, sizeof(VTMode));
    mode->index = 0;
    mode->depth = (*(*device->gd)->gdPMap)->pixelSize;
    mode->hasColor = TestDeviceAttribute(device->gd, gdDevType) != 0;
    if (VT_HasDisplayManager()) {
        if (DMGetDisplayMode(device->gd, &info) != 0) {
            err = kVideoError;
            goto done;
        }
        count = device->modeCount;
        for (i = 0; i < count; i++) {
            if (info.csData == ((VTModeEntry *)device->modes)[i].displayMode)
                break;
        }
        if (i == count) {
            err = kVideoError;
            goto done;
        }
        mode->index = i;
    }
done:
    return err;
}

long VT_DMSetMode(VTDevice *device, VTMode *mode)
{
    Handle token;
    ULong depth;
    long err;
    ULong color;
    volatile long saved;

    err = 0;
    token = 0;
    if (VT_HasDisplayManager() && (gVTDeviceCount == 1 || VT_DisplayManagerVersion() >= 0x20000UL)) {
        if (DMBeginConfigureDisplays(&token) != 0) {
            err = kVideoError;
            goto done;
        }
        if (device->mode.index != mode->index) {
            depth = mode->depth;
            err = DMSetDisplayMode(device->gd, device->modes[mode->index].displayMode, &depth, (unsigned long)&device->modes[mode->index].info, token);
            if (err == kDMDriverNotDisplayMgrAwareErr)
                err = 0;
            if (err != 0) {
                err = kVideoError;
                goto done;
            }
        } else {
            depth = (*(*device->gd)->gdPMap)->pixelSize;
        }
    } else {
        depth = (*(*device->gd)->gdPMap)->pixelSize;
    }
    color = TestDeviceAttribute(device->gd, gdDevType) ? 1 : 0;
    if (mode->depth != depth || mode->hasColor != color) {
        if (SetDepth(device->gd, mode->depth, mode->hasColor != color, mode->hasColor ? 1 : 0) != 0)
            err = kVideoError;
    }
done:
    if (token != 0) {
        saved = VT_CallSave();
        DMEndConfigureDisplays(token);
        VT_CallRestore(saved);
        token = 0;
    }
    return err;
}

static unsigned char VT_HasDisplayManager(void)
{
    unsigned char has;
    long response;

    has = 0;
    if (Gestalt('dply', &response) == 0 && (response & 1) != 0 && (void *)DMGetDisplayMode != 0)
        has = 1;
    return has;
}

static long VT_DisplayManagerVersion(void)
{
    long response;

    if (Gestalt('dplv', &response) != 0)
        response = 0;
    return response;
}

static inline void VTP2C(unsigned char *dst, const unsigned char *src)
{
    long n;
    unsigned char *p;

    n = *src + 1;
    p = dst - 1;
    while (--n != 0)
        *++p = *++src;
    p[1] = 0;
}

static pascal void VT_ModeProc(void *user, DMListIndexType index, DMDisplayModeListEntryPtr info)
{
    VTDevice *device;
    VTModeEntry *entry;
    ULong interlaced;
    ULong i;
    ULong count;
    VPBlockPtr block;
    char cname[256];

    device = user;
    interlaced = (info->displayModeTimingInfo->csTimingFlags >> 1) & 1;
    entry = device->modes + index;
    VTP2C((unsigned char *)cname, info->displayModeName);
    StringCopySafe(entry->name, cname, sizeof(entry->name));
    entry->width = info->displayModeResolutionInfo->csHorizontalPixels;
    entry->height = info->displayModeResolutionInfo->csVerticalLines;
    entry->rate = Fix2Long(info->displayModeResolutionInfo->csRefreshRate);
    entry->depthMin = 32;
    entry->depthMax = 1;
    entry->stretched = AsByte(interlaced == 0);
    entry->info = *info->displayModeSwitchInfo;
    entry->displayMode = info->displayModeSwitchInfo->csData;
    entry->depthMin = 32;
    entry->depthMax = 1;
    block = info->displayModeDepthBlockInfo->depthVPBlock->depthVPBlock;
    count = info->displayModeDepthBlockInfo->depthBlockCount;
    for (i = 0; i < count; i++, block++) {
        entry->depths |= block->vpPixelSize;
        if (entry->depthMin > block->vpPixelSize)
            entry->depthMin = block->vpPixelSize;
        if (entry->depthMax < block->vpPixelSize)
            entry->depthMax = block->vpPixelSize;
    }
}

#endif
