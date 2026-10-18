// COMPILE: -opt level=2,speed -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: the sound tool is its speed part (16-byte aligned, where 335's level 2 is packed) and calls memset at constant sizes, so no ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atsnddrv.c -- the sound output driver: DirectSound with a looping secondary buffer, or waveOut with two ping-ponged headers, fed by one mixer thread; volume and rate helpers. Notes: Research/notes/atsnddrv.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "ambrosia.h"
#include "burgerdsound.h"

extern void _ST_MixerDispatch(short *buffer, ULong frames);   /* atsndmem.c */

extern void *gDirectSoundCreate;                       /* atsoundup.c, DirectSoundCreate */
extern ULong gBytesPerBuffer;                    /* atsoundup.c, buffer bytes */
extern long gSamplesPerBuffer;                   /* atsoundup.c, buffer frames */
extern ULong gDurationBuffer;                    /* atsoundup.c, buffer ms */
extern ULong gSampleBits;                        /* atsoundup.c, bits */
extern ULong gNumChannels;                       /* atsoundup.c, channels */

DSound *gDirectSound;                            /* 0x008700ec */
DSoundBuffer *gSndDSPrimaryBuffer;                      /* 0x0086ffa4 primary */
DSoundBuffer *gMixBuffer;                      /* 0x00870ddc secondary */
void *gMixerThread;                              /* 0x00870194 mixer thread */
void *gWaveOut;                              /* 0x0087014c waveOut */
void *gMixerEvent;                              /* 0x0087006c buffer-done event */
WaveHdr gWaveBlocks[2];                         /* 0x0086d8c0 */

#define kWaveMapper         0xffffffffUL
#define kWaveFormatPcm      1
#define kWaveFormat1S16     0x008
#define kWaveFormat2S16     0x080
#define kWaveFormat4S16     0x800
#define kWaveCapsLRVolume   0x0008
#define kWhdrDone           0x01
#define kWhdrPrepared       0x02
#define kCallbackEvent      0x00050000UL
#define kDSSclPriority      2
#define kDSBPrimary         0x00000001UL
#define kDSBCtrlVolume      0x00000080UL
#define kDSBStickyFocus     0x00004000UL
#define kDSBPosition2       0x00010000UL
#define kDSBPlayLooping     1
#define kBufferDescV1       0x14
#define kDSVolumeRange      10000
#define kWaveVolumeMax      0xffff
#define kRate44k            0xac440000UL        /* 44100.0, 16.16 */
#define kRate22k            0x56220000UL
#define kRate11k            0x2b110000UL

ULong __stdcall _ST_PumpThread(void *param);

/* _ST_CreateSoundPump. STATE: MATCH 100%, exe 251 / obj 251, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00507300
long _ST_CreateSoundPump(void)
{
    long err, e;
    ULong threadId;
    WaveFormat format;
    DSBufferDesc desc;

    err = 0;
    threadId = 0;
    if (gDirectSoundCreate != 0) {
        err = ((DirectSoundCreateProc)gDirectSoundCreate)(0, &gDirectSound, 0);
        if (err != 0) {
            gDirectSound = 0;
            err = 0;
        }
    }

    memset(&format, 0, sizeof(format));
    format.formatTag = kWaveFormatPcm;
    format.channels = gNumChannels;
    format.samplesPerSec = UFix32_To_UInt32(gSampleRate);
    format.bitsPerSample = gSampleBits;
    format.blockAlign = format.channels * format.bitsPerSample / 8;
    format.avgBytesPerSec = format.blockAlign * format.samplesPerSec;
    format.cbSize = 0;

    if (gDirectSound != 0) {
        void *window;

        window = _PlatformGetWindow();
        if (window == 0) {
            err = kNoDefault;
            goto done;
        }
        e = gDirectSound->vtbl->SetCooperativeLevel(gDirectSound, window, kDSSclPriority);
        if (e != 0) {
            err = e;
            err = kNoDefault;
            goto done;
        }

        memset(&desc, 0, sizeof(desc));
        desc.size = kBufferDescV1;
        desc.flags = kDSBPrimary | kDSBCtrlVolume | kDSBStickyFocus;
        desc.bufferBytes = 0;
        desc.format = 0;
        e = gDirectSound->vtbl->CreateSoundBuffer(gDirectSound, &desc, &gSndDSPrimaryBuffer, 0);
        if (e != 0) {
            err = e;
            err = kNoDefault;
            goto done;
        }
        e = gSndDSPrimaryBuffer->vtbl->SetFormat(gSndDSPrimaryBuffer, &format);
        if (e != 0) {
            err = e;
            err = kNoDefault;
            goto done;
        }

        memset(&desc, 0, sizeof(desc));
        desc.size = kBufferDescV1;
        desc.flags = kDSBPosition2 | kDSBStickyFocus;
        desc.bufferBytes = gBytesPerBuffer;
        desc.format = &format;
        e = gDirectSound->vtbl->CreateSoundBuffer(gDirectSound, &desc, &gMixBuffer, 0);
        if (e != 0) {
            err = e;
            err = kNoDefault;
            goto done;
        }

        gMixerThread = CreateThread(0, kThreadStack, _ST_PumpThread, 0, 0, &threadId);
        if (gMixerThread == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        if (SetThreadPriority(gMixerThread, THREAD_PRIORITY_TIME_CRITICAL) == 0) {
            err = GetLastError();
            err = kOSFailure;
            goto done;
        }
        e = gMixBuffer->vtbl->Play(gMixBuffer, 0, 0, kDSBPlayLooping);
        if (e != 0) {
            err = e;
            err = kNoDefault;
            goto done;
        }
    } else {
        memset(gWaveBlocks, 0, sizeof(gWaveBlocks));
        gMixerEvent = CreateEventA(0, 0, 0, 0);
        if (gMixerEvent == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        e = waveOutOpen(&gWaveOut, kWaveMapper, &format, (ULong)gMixerEvent, 0,
                        kCallbackEvent);
        if (e != 0) {
            err = e;
            goto done;
        }

        gWaveBlocks[0].bufferLength = gBytesPerBuffer;
        gWaveBlocks[0].data = MemoryAllocate(gBytesPerBuffer);
        if (gWaveBlocks[0].data == 0) {
            err = kOutOfMemory;
            goto done;
        }
        gWaveBlocks[1].bufferLength = gBytesPerBuffer;
        gWaveBlocks[1].data = MemoryAllocate(gBytesPerBuffer);
        if (gWaveBlocks[1].data == 0) {
            err = kOutOfMemory;
            goto done;
        }

        gMixerThread = CreateThread(0, kThreadStack, _ST_PumpThread, 0, 0, &threadId);
        if (gMixerThread == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        if (SetThreadPriority(gMixerThread, THREAD_PRIORITY_TIME_CRITICAL) == 0) {
            err = GetLastError();
            err = kOSFailure;
            goto done;
        }
        SetEvent(gMixerEvent);
        SetEvent(gMixerEvent);
    }
done:
    return err;
}

/* _ST_DisposeSoundPump. STATE: MATCH 100%, exe 91 / obj 91, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005076a0
void _ST_DisposeSoundPump(void)
{
    if (gDirectSound != 0) {
        if (gMixBuffer != 0)
            gMixBuffer->vtbl->Stop(gMixBuffer);
        if (gMixerThread != 0)
            SuspendThread(gMixerThread);
        if (gMixBuffer != 0) {
            gMixBuffer->vtbl->Release(gMixBuffer);
            gMixBuffer = 0;
        }
        if (gSndDSPrimaryBuffer != 0) {
            gSndDSPrimaryBuffer->vtbl->Release(gSndDSPrimaryBuffer);
            gSndDSPrimaryBuffer = 0;
        }
        gDirectSound->vtbl->Release(gDirectSound);
        gDirectSound = 0;
    }

    if (gWaveOut != 0) {
        waveOutReset(gWaveOut);
        if (gWaveBlocks[0].flags & kWhdrPrepared)
            waveOutUnprepareHeader(gWaveOut, &gWaveBlocks[0], sizeof(WaveHdr));
        if (gWaveBlocks[0].data != 0) {
            MemoryDeallocate(gWaveBlocks[0].data);
            gWaveBlocks[0].data = 0;
        }
        if (gWaveBlocks[1].flags & kWhdrPrepared)
            waveOutUnprepareHeader(gWaveOut, &gWaveBlocks[1], sizeof(WaveHdr));
        if (gWaveBlocks[1].data != 0) {
            MemoryDeallocate(gWaveBlocks[1].data);
            gWaveBlocks[1].data = 0;
        }
        waveOutClose(gWaveOut);
        gWaveOut = 0;
    }

    if (gMixerThread != 0) {
        TerminateThread(gMixerThread, 0);
        gMixerThread = 0;
    }
    if (gMixerEvent != 0) {
        CloseHandle(gMixerEvent);
        gMixerEvent = 0;
    }
}

/* The best rate the default device plays in 16-bit stereo, 16.16. 44 kHz
   only through DirectSound. */
/* _ST_GetCurrentSampleRate. STATE: MATCH 100%, exe 38 / obj 38, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00507820
long _ST_GetCurrentSampleRate(ULong *rate)
{
    WaveOutCaps caps;
    long err, e;

    err = 0;
    e = waveOutGetDevCapsA(kWaveMapper, &caps, sizeof(caps));
    if (e != 0) {
        err = e;
        err = kNoDefault;
    } else if ((caps.formats & kWaveFormat4S16) != 0 && gDirectSoundCreate != 0)
        *rate = kRate44k;
    else if ((caps.formats & kWaveFormat2S16) != 0)
        *rate = kRate22k;
    else if ((caps.formats & kWaveFormat1S16) != 0)
        *rate = kRate11k;
    else
        err = kNoDevice;
    return err;
}

/* _ST_GetWaveVolume. STATE: MATCH 100%, exe 96 / obj 96, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005078a0
long _ST_GetWaveVolume(ULong *left, ULong *right, ULong *max)
{
    long err, e;
    WaveOutCaps caps;
    ULong volume;

    err = 0;
    *left = 0;
    *right = 0;
    if (max != 0)
        *max = 0;

    if (gDirectSound != 0) {
        e = gSndDSPrimaryBuffer->vtbl->GetVolume(gSndDSPrimaryBuffer, (long *)&volume);
        if (e != 0) {
            err = e;
            err = kVolumeFailed;
        } else {
            *right = volume + kDSVolumeRange;
            *left = *right;
            if (max != 0)
                *max = kDSVolumeRange;
        }
    } else if (gWaveOut != 0) {
        memset(&caps, 0, sizeof(caps));
        e = waveOutGetDevCapsA((ULong)gWaveOut, &caps, sizeof(caps));
        if (e != 0) {
            err = e;
            err = kNoDefault;
        } else if ((e = waveOutGetVolume(gWaveOut, &volume)) != 0) {
            err = e;
            err = kVolumeFailed;
        } else if ((caps.support & kWaveCapsLRVolume) != 0) {
            *left = volume & kWaveVolumeMax;
            *right = (volume >> 16) & kWaveVolumeMax;
            if (max != 0)
                *max = kWaveVolumeMax;
        } else {
            *left = volume & kWaveVolumeMax;
            *right = volume & kWaveVolumeMax;
            if (max != 0)
                *max = kWaveVolumeMax;
        }
    }
    return err;
}

/* _ST_SetWaveVolume. STATE: MATCH 100%, exe 77 / obj 77, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005079f0
long _ST_SetWaveVolume(ULong left, ULong right)
{
    long err, e;
    WaveOutCaps caps;

    err = 0;
    if (gDirectSound != 0) {
        if (left > kDSVolumeRange)
            left = kDSVolumeRange;
        if (right > kDSVolumeRange)
            right = kDSVolumeRange;
        e = gSndDSPrimaryBuffer->vtbl->SetVolume(gSndDSPrimaryBuffer, (left + right) / 2 - kDSVolumeRange);
        if (e != 0) {
            err = e;
            err = kVolumeFailed;
        }
    } else if (gWaveOut != 0) {
        if (left > kWaveVolumeMax)
            left = kWaveVolumeMax;
        if (right > kWaveVolumeMax)
            right = kWaveVolumeMax;
        memset(&caps, 0, sizeof(caps));
        e = waveOutGetDevCapsA((ULong)gWaveOut, &caps, sizeof(caps));
        if (e != 0) {
            err = e;
            err = kNoDefault;
        } else {
            if ((caps.support & kWaveCapsLRVolume) != 0)
                e = right << 16 | left;
            else
                e = (left + right) / 2;
            e = waveOutSetVolume(gWaveOut, e);
            if (e != 0) {
                err = e;
                err = kVolumeFailed;
            }
        }
    }
    return err;
}

/* _ST_SupportsSystemVolume. STATE: MATCH 100%, exe 2 / obj 2 instrs. OPEN: none */
// FUNCTION: EVNOVA 0x00507ae0
unsigned char _ST_SupportsSystemVolume(void)
{
    return 1;
}

/* _ST_GetSystemVolume. STATE: MATCH 100%, exe 9 / obj 9 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00507af0
long _ST_GetSystemVolume(ULong *left, ULong *right, ULong *max)
{
    return _ST_GetWaveVolume(left, right, max);
}

/* _ST_SetSystemVolume. STATE: MATCH 100%, exe 8 / obj 8 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00507b10
void _ST_SetSystemVolume(ULong left, ULong right)
{
    _ST_SetWaveVolume(left, right);
}

/* The mixer thread: refill whatever the device has played, forever. */
/* _ST_PumpThread. STATE: MATCH 100%, exe 164 / obj 164, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00507b30
ULong __stdcall _ST_PumpThread(void *param)
{
    ULong play;
    ULong write;
    void *p1;
    void *p2;
    ULong n1;
    ULong n2;
    ULong length;
    ULong shift;
    ULong cursor;
    long which;
    WaveHdr *hdr;
    char *data;

    which = 0;
    shift = 0;
    cursor = 0;
    if (gNumChannels == 2)
        shift++;
    if (gSampleBits == 16)
        shift++;

    if (gDirectSound != 0) {
        for (;;) {
            Sleep(gDurationBuffer / 2);
            if (gMixBuffer->vtbl->GetCurrentPosition(gMixBuffer, &play, &write) != 0)
                play = 0;
            if (cursor < play)
                length = play - cursor;
            else
                length = gBytesPerBuffer - (cursor - play);
            while (gMixBuffer->vtbl->Lock(gMixBuffer, cursor, length, &p1, &n1,
                                            &p2, &n2, 0) != 0) {
                gMixBuffer->vtbl->Restore(gMixBuffer);
                gMixBuffer->vtbl->Play(gMixBuffer, 0, 0, kDSBPlayLooping);
            }
            if (p1 != 0 && n1 != 0)
                _ST_MixerDispatch(p1, n1 >> shift);
            if (p2 != 0 && n2 != 0)
                _ST_MixerDispatch(p2, n2 >> shift);
            gMixBuffer->vtbl->Unlock(gMixBuffer, p1, n1, p2, n2);
            cursor += length;
            if (cursor >= gBytesPerBuffer)
                cursor -= gBytesPerBuffer;
        }
    }

    for (;;) {
        hdr = &gWaveBlocks[which];
        which = 1 - which;
        if ((hdr->flags & kWhdrDone) == 0)
            WaitForSingleObject(gMixerEvent, INFINITE);
        data = hdr->data;
        if (hdr->flags & kWhdrPrepared)
            waveOutUnprepareHeader(gWaveOut, hdr, sizeof(WaveHdr));
        _ST_MixerDispatch((short *)data, gSamplesPerBuffer);
        memset(hdr, 0, sizeof(WaveHdr));
        hdr->data = data;
        hdr->bufferLength = gBytesPerBuffer;
        waveOutPrepareHeader(gWaveOut, hdr, sizeof(WaveHdr));
        waveOutWrite(gWaveOut, hdr, sizeof(WaveHdr));
    }
}
