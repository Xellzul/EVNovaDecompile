/* atvideodevmac.c -- 1.0.10: the Monitor Tool's device records: QuickDraw's device list, a device opened, closed and switched, a mode scored, a device found. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#if !TARGET_API_MAC_CARBON
#include "atvideomac.h"

unsigned char gVTMenuVisible = 1;
unsigned char gVTMenuWasShown = 1;
static VTColor gVTSwitchBlack;
GDHandle gVTSavedDevice;
VTDevice *gVTCaptured;
ULong gVTDeviceCount;
VTDevice *gVTDevices;
unsigned char gVTOpen;

unsigned short VT_CountDevices(void)
{
    ULong count;
    GDHandle gd;

    count = 0;
    for (gd = GetDeviceList(); gd != 0; gd = GetNextDevice(gd)) {
        if (TestDeviceAttribute(gd, screenDevice) && TestDeviceAttribute(gd, screenActive))
            count++;
    }
    return count;
}

long VT_DeviceOpen(VTDevice *device, unsigned short index)
{
    long err;
    long e;
    GDHandle gd;
    ULong i;
    ULong last;
    GDHandle saved;

    err = 0;
    if (index == 0) {
        gd = GetMainDevice();
    } else {
        for (gd = GetDeviceList(); gd != 0; gd = GetNextDevice(gd)) {
            if (TestDeviceAttribute(gd, screenDevice) && TestDeviceAttribute(gd, screenActive)
                && !TestDeviceAttribute(gd, mainScreen)) {
                if (--index == 0)
                    break;
            }
        }
    }
    if (gd == 0) {
        err = kVideoNoDevice;
        goto done;
    }
    device->gd = gd;
    saved = GetGDevice();
    SetGDevice(device->gd);
    i = 0;
    last = (*(*(*device->gd)->gdPMap)->pmTable)->ctSize;
    for (; i <= last; i++) {
        ProtectEntry(i, 0);
        ReserveEntry(i, 0);
        ProtectEntry(i, 0);
    }
    SetGDevice(saved);
    device->isMain = AsByte(index == 0);
    if ((e = VT_DeviceModes(device)) != 0) {
        err = e;
        goto done;
    }
    if ((e = VT_DMCurrentMode(device, &device->saved)) != 0)
        err = e;
    if ((e = VT_DMCurrentMode(device, &device->mode)) != 0)
        err = e;
    device->ready = 1;
    if (VT_DeviceIsIndexed(device)) {
        if ((device->clutSaved = VT_ClutCopy(device)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((device->clutWork = VT_ClutCopy(device)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    if (VT_DeviceHasGamma(device)) {
        if ((device->gammaSaved = VT_GammaGet(device)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((device->gammaWork = VT_GammaCopy(device, device->gammaSaved)) == 0)
            err = kOutOfMemory;
    }
done:
    return err;
}

void VT_DeviceClose(VTDevice *device)
{
    if (device->ready) {
        if (gVTOpen)
            VT_DeviceSwitch(device, &device->saved);
        device->ready = 0;
    }
    if (device->modes != 0) {
        MemoryDeallocate(device->modes);
        device->modes = 0;
    }
    if (device->clutSaved != 0) {
        if (gVTOpen)
            VT_ClutSet(device, device->clutSaved);
        MemoryDeallocate(device->clutSaved);
        device->clutSaved = 0;
    }
    if (device->clutWork != 0) {
        MemoryDeallocate(device->clutWork);
        device->clutWork = 0;
    }
    if (device->table30 != 0) {
        MemoryDeallocate(device->table30);
        device->table30 = 0;
    }
    if (device->table34 != 0) {
        MemoryDeallocate(device->table34);
        device->table34 = 0;
    }
    if (device->gammaSaved != 0) {
        if (gVTOpen)
            VT_GammaSet(device, device->gammaSaved);
        VT_GammaFree(device, device->gammaSaved);
        device->gammaSaved = 0;
    }
    if (device->gammaWork != 0) {
        VT_GammaFree(device, device->gammaWork);
        device->gammaWork = 0;
    }
    if (device->fadeFrom != 0) {
        VT_GammaFree(device, device->fadeFrom);
        device->fadeFrom = 0;
    }
    if (device->fadeTo != 0) {
        VT_GammaFree(device, device->fadeTo);
        device->fadeTo = 0;
    }
}

long VT_DeviceSwitch(VTDevice *device, VTMode *mode)
{
    long err;
    long e;
    unsigned char faded;
    VTColor black;

    black = gVTSwitchBlack;
    err = 0;
    faded = 0;
    if (mode->index != device->mode.index || mode->depth != device->mode.depth
        || mode->hasColor != device->mode.hasColor) {
        if (!device->switched) {
            if ((e = GammaFadeToColor((ULong)device, &black, 100, kFadeTicks, 0)) != 0) {
                err = e;
                goto done;
            }
            faded = 1;
        }
        if ((e = VT_DMSetMode(device, mode)) != 0) {
            err = e;
            goto done;
        }
        if (device->clutWork != 0) {
            MemoryDeallocate(device->clutWork);
            device->clutWork = 0;
        }
        if (VT_DeviceIsIndexed(device)) {
            if ((device->clutWork = VT_ClutCopy(device)) == 0)
                err = kOutOfMemory;
        }
    }
done:
    if (faded) {
        if ((e = GammaFadeToColor((ULong)device, 0, 100, kFadeTicks, 0)) != 0)
            err = e;
    }
    if ((e = VT_DMCurrentMode(device, &device->mode)) != 0)
        err = e;
    return err;
}

ULong VT_ScoreSize(VTModeEntry *entry, VTRequest *req)
{
    ULong result;
    ULong score;

    score = kNoScore;
    result = score;
    switch (req->sizeRule) {
    case 0:
        result = 0;
        break;
    case 1:
        if (entry->interlaced) {
            result = score;
            break;
        }
        if (entry->stretched) {
            result = score;
            break;
        }
        if (entry->width == req->width && entry->height == req->height) {
            result = 0;
            break;
        }
        result = kNoScore;
        break;
    case 2:
        if (entry->interlaced) {
            result = score;
            break;
        }
        if (entry->stretched) {
            result = score;
            break;
        }
        if (entry->width == req->width && entry->height == req->height) {
            result = 0;
            break;
        }
        if (entry->width > req->width && entry->height > req->height) {
            result = kNoScore;
            break;
        }
        result = (req->width - entry->width) + (req->height - entry->height);
        break;
    case 3:
        if (entry->interlaced) {
            result = score;
            break;
        }
        if (entry->stretched) {
            result = score;
            break;
        }
        if (entry->width == req->width && entry->height == req->height) {
            result = 0;
            break;
        }
        if (entry->width < req->width && entry->height < req->height) {
            result = kNoScore;
            break;
        }
        result = (entry->width - req->width) + (entry->height - req->height);
        break;
    }
    return result;
}

ULong VT_ScoreRate(VTModeEntry *entry, VTRequest *req)
{
    ULong score;

    score = kNoScore;
    switch (req->rateRule) {
    case 0:
        score = 0;
        break;
    case 1:
        if (entry->rate == req->rate)
            score = 0;
        else
            score = kNoScore;
        break;
    case 2:
        if (entry->rate == req->rate) {
            score = 0;
            break;
        }
        if (entry->rate > req->rate) {
            score = kNoScore;
            break;
        }
        score = req->rate - entry->rate;
        break;
    case 3:
        if (entry->rate == req->rate) {
            score = 0;
            break;
        }
        if (entry->rate < req->rate) {
            score = kNoScore;
            break;
        }
        score = entry->rate - req->rate;
        break;
    }
    return score;
}

ULong VT_ScoreDepth(VTModeEntry *entry, VTRequest *req, ULong *depth)
{
    ULong score;

    *depth = req->depth;
    score = kNoScore;
    switch (req->depthRule) {
    case 0:
        score = 0;
        if (*depth < entry->depthMin)
            *depth = entry->depthMin;
        if (*depth > entry->depthMax)
            *depth = entry->depthMax;
        break;
    case 1:
        if (entry->depths & req->depth) {
            score = 0;
            break;
        }
        if (*depth < entry->depthMin)
            *depth = entry->depthMin;
        if (*depth > entry->depthMax)
            *depth = entry->depthMax;
        break;
    case 2:
        if (entry->depths & req->depth) {
            score = 0;
            break;
        }
        if (entry->depthMin > req->depth) {
            *depth = entry->depthMin;
            break;
        }
        *depth = entry->depthMin;       /* BUG: the same limit on both arms; the larger depth was meant here */
        break;
    case 3:
        if (entry->depths & req->depth) {
            score = 0;
            break;
        }
        if (entry->depthMax < req->depth) {
            *depth = entry->depthMax;
            break;
        }
        *depth = entry->depthMax;       /* BUG: the same limit on both arms; the smaller depth was meant here */
        break;
    }
    return score;
}

unsigned char VT_DeviceHasGamma(VTDevice *device)
{
    unsigned char has;

    has = 0;
    if ((*device->gd)->gdType != fixedType)
        has = 1;
    return has;
}

unsigned char VT_DeviceIsIndexed(VTDevice *device)
{
    GDPtr gd;
    unsigned char indexed;

    indexed = 0;
    gd = *device->gd;
    if ((*gd->gdPMap)->pixelSize <= 8 && gd->gdType <= fixedType)
        indexed = 1;
    return indexed;
}

VTDevice *VT_DeviceFind(ULong id)
{
    VTDevice *found;
    ULong i;

    found = 0;
    if (gVTDevices != 0) {
        for (i = 0; i < gVTDeviceCount; i++) {
            if (id == i + 1)
                break;
        }
        if (i < gVTDeviceCount)
            found = gVTDevices + i;
    }
    if (found == 0 && gVTDevices != 0) {
        for (i = 0; i < gVTDeviceCount; i++) {
            if ((VTDevice *)id == &gVTDevices[i])
                break;
        }
        if (i < gVTDeviceCount)
            found = gVTDevices + i;
    }
    if (found == 0 && gVTDevices != 0) {
        for (i = 0; i < gVTDeviceCount; i++) {
            if ((GDHandle)id == gVTDevices[i].gd)
                break;
        }
        if (i < gVTDeviceCount)
            found = gVTDevices + i;
    }
    return found;
}

#endif
