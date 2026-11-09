/* atvideomac.c -- 1.0.10: AmbrosiaTools' video tool on QuickDraw's device list and the Display Manager, read off the fragment; no win text. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#if !TARGET_API_MAC_CARBON
#include "atvideomac.h"

static VTColor gVTFadeBlack;

long MT_Open(void)
{
    ULong offset;
    long e;
    long err;
    ULong i;
    ULong count;
    long response;

    err = 0;
    if (Gestalt('qdrw', &response) != 0) {
        err = kNotSupported;
        goto done;
    }
    if ((response & 1) == 0) {
        err = kNotSupported;
        goto done;
    }
    e = PlatformOpen();
    if (e != 0) {
        err = e;
        goto done;
    }
    count = VT_CountDevices();
    gVTDeviceCount = count;
    if ((gVTDevices = MemoryAllocate(count * sizeof(VTDevice))) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(gVTDevices, gVTDeviceCount * sizeof(VTDevice));
    for (i = 0, offset = 0; i < gVTDeviceCount; offset += sizeof(VTDevice), i++) {
        e = VT_DeviceOpen((VTDevice *)((char *)gVTDevices + offset), i);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    SystemSetQuitProc(DisposeGammaTools);
    VT_LockDevices();
    if (!VT_CheckScreens()) {
        err = kVideoBusy;
        goto done;
    }
    gVTOpen = 1;
done:
    if (err != 0)
        DisposeGammaTools();
    return err;
}

void DisposeGammaTools(void)
{
    ULong i;

    VT_LockDevices();
    if (gVTCaptured != 0)
        VT_Release();
    if (gVTDevices != 0) {
        for (i = 0; i < gVTDeviceCount; i++)
            VT_DeviceClose(&gVTDevices[i]);
        MemoryDeallocate(gVTDevices);
        gVTDevices = 0;
    }
    gVTDeviceCount = 0;
    if (!gVTMenuVisible)
        ShowMenuBar();
    gVTOpen = 0;
}

long MT_GetMode(ULong id, VTRequest *info)
{
    VTDevice *device;

    device = VT_DeviceFind(id);
    MemoryClear(info, sizeof(VTRequest));
    info->sizeRule = 1;
    info->width = device->modes[device->mode.index].width;
    info->height = device->modes[device->mode.index].height;
    info->depthRule = 1;
    info->depth = device->mode.depth;
    info->colorRule = 1;
    info->color = device->mode.hasColor;
    info->rateRule = 1;
    info->rate = device->modes[device->mode.index].rate;
    return 0;
}

long MT_SetMode(ULong id, VTRequest *req)
{
    ULong *wantDepth;
    ULong score;
    VTDevice *device;
    VTModeEntry *entry;
    long err;
    ULong bestScore;
    VTMode want;
    VTMode best;
    long e;

    device = VT_DeviceFind(id);
    err = 0;
    VT_LockDevices();
    if (!VT_CheckScreens()) {
        err = kVideoBusy;
        goto done;
    }
    if (req->sizeRule == 0 && req->rateRule == 0) {
        want.index = device->mode.index;
        want.depth = req->depthRule == 0 ? device->mode.depth : req->depth;
        if ((want.depth & device->modes[want.index].depths) == 0) {
            err = kVideoNoMode;
            goto done;
        }
    } else {
        best.index = 0;
        best.depth = req->depth;
        bestScore = -1;
        wantDepth = &want.depth;
        for (want.index = 0; want.index < device->modeCount; want.index++) {
            entry = device->modes + want.index;
            score = (VT_ScoreSize(entry, req) * 100 + VT_ScoreDepth(entry, req, wantDepth) * 100 + VT_ScoreRate(entry, req) * 100) / 100;
            if (score == 0)
                break;
            if (bestScore > score) {
                bestScore = score;
                best = want;
            }
        }
        if (want.index == device->modeCount) {
            if (bestScore > kNoScore) {
                err = kVideoNoMode;
                goto done;
            }
            want = best;
        }
    }
    want.hasColor = req->colorRule == 0 ? device->mode.hasColor : req->color;
    if ((e = VT_DeviceSwitch(device, &want)) != 0)
        err = e;
done:
    if ((e = VT_DMCurrentMode(device, &device->mode)) != 0)
        err = e;
    return err;
}

long GammaFadeTo(ULong id, ULong level, ULong ticks)
{
    long err;
    long e;
    const ULong *from;
    VTColor black;

    VT_DeviceFind(id);
    err = 0;
    from = (const ULong *)&gVTFadeBlack;
    ((ULong *)&black)[0] = *from++;
    ((ULong *)&black)[1] = *from++;
    ((ULong *)&black)[2] = *from;
    if (level < 100)
        e = GammaFadeToColor(id, &black, 100 - level, ticks, 1);
    else
        e = GammaFadeToColor(id, 0, 100, ticks, 1);
    if (e != 0)
        err = e;
    return err;
}

long GammaFadeToColor(ULong id, void *target, ULong percent, ULong ticks, unsigned char linear)
{
    unsigned long long start;
    VTDevice *d;
    long err;
    ULong last;
    ULong elapsed;
    ULong level;
    ULong i;

    d = VT_DeviceFind(id);
    err = 0;
    VT_LockDevices();
    if (!VT_CheckScreens()) {
        err = kVideoBusy;
        goto done;
    }
    for (i = 0; (id != 0 && i == 0) || (id == 0 && i < gVTDeviceCount); i++) {
        d = id != 0 ? d : &gVTDevices[i];
        if (!VT_DeviceHasGamma(d))
            continue;
        if ((d->fadeFrom = VT_GammaCopy(d, d->gammaWork)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((d->fadeTo = VT_GammaCopy(d, d->gammaSaved)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (target != 0)
            VT_GammaFill(d, d->fadeTo, target);
    }
    last = 0;
    start = TimerGetMicroseconds();
    do {
        elapsed = (TimerGetMicroseconds() - start) / 1000;
        if (elapsed > ticks)
            elapsed = ticks;
        if (ticks != 0)
            level = (percent * elapsed) / ticks;
        else
            level = percent;
        if (last != level) {
            for (i = 0; (id != 0 && i == 0) || (id == 0 && i < gVTDeviceCount); i++) {
                d = id != 0 ? d : &gVTDevices[i];
                if (!VT_DeviceHasGamma(d))
                    continue;
                VT_GammaBlend(d, d->gammaWork, d->fadeFrom, d->fadeTo,
                              linear ? level * 100
                                     : ((target != 0 || percent != 100) ? level * level
                                                                         : 10000 - (100 - level) * (100 - level)),
                              10000);
                VT_GammaSet(d, d->gammaWork);
            }
            last = level;
        }
    } while (elapsed < ticks);
done:
    for (i = 0; (id != 0 && i == 0) || (id == 0 && i < gVTDeviceCount); i++) {
        d = id != 0 ? d : &gVTDevices[i];
        if (!VT_DeviceHasGamma(d))
            continue;
        d->switched = (target != 0 || percent != 100) != 0;
        if (d->fadeFrom != 0) {
            VT_GammaFree(d, d->fadeFrom);
            d->fadeFrom = 0;
        }
        if (d->fadeTo != 0) {
            VT_GammaFree(d, d->fadeTo);
            d->fadeTo = 0;
        }
    }
    return err;
}

unsigned char VT_Capture(void)
{
    return gVTMenuWasShown; /* 1.0.10 pad @_VT_Release: a stripped body's first reference to gVTMenuWasShown, ahead of gVTSavedDevice's (the exe's TC order); no held build has its words */
}

long VT_Release(void)
{
    long err;
    long e;

    err = 0;
    VT_LockDevices();
    if (gVTSavedDevice != 0) {
        SetGDevice(gVTSavedDevice);
        gVTSavedDevice = 0;
    }
    if (gVTCaptured->isMain && gVTMenuWasShown) {
        e = ShowMenuBar();
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    gVTCaptured = 0;
    gVTMenuWasShown = 1;
done:
    return err;
}

long ShowMenuBar(void)
{
    long err;
    long e;

    err = 0;
    VT_LockDevices();
    if (!gVTMenuVisible) {
        e = _ShowMenuBar();
        if (e != 0) {
            err = e;
            goto done;
        }
        gVTMenuVisible = 1;
        VT_LockDevices();
        if (!VT_CheckScreens())
            err = kVideoBusy;
    }
done:
    return err;
}

long HideMenuBar(void)
{
    long err;
    long e;

    err = 0;
    VT_LockDevices();
    if (!VT_CheckScreens()) {
        err = kVideoBusy;
        goto done;
    }
    if (gVTMenuVisible) {
        e = _HideMenuBar();
        if (e != 0) {
            err = e;
            goto done;
        }
        gVTMenuVisible = 0;
        VT_LockDevices();
    }
done:
    return err;
}

#endif
