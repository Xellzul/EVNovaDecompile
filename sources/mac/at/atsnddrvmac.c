/* atsnddrvmac.c -- 1.0.10: the sound tool's driver on the Sound Manager's double buffers, where win's atsnddrv.c has DirectSound. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "burgerchan.h"

#if !TARGET_API_MAC_CARBON
#define kChannelBytesMac    0x424
#define kVolumeMax          0x100
#define kNoSoundDevice      -7400
#define kNoSampleRate       -7402   /* a rate for an old Sound Manager */
#define kNoSysVolume        -7403
#define kSoundManager3      0x03100000
#define kDefaultRate        0x56ee8ba3

extern ULong gSampleBits;
extern ULong gNumChannels;
extern int32_t gSamplesPerBuffer;
extern ULong gBytesPerBuffer;
extern ULong gSampleRate;
extern ULong gSoundManagerVersion;

SndDoubleBufferHeader gDoubleHeader;
SndChannelPtr gSoundChannel;

void _ST_DisposeSoundPump(void);
static SndChannelPtr _ST_NewChannel(void);
static SndDoubleBufferPtr _ST_NewBuffer(int32_t frames, ULong bytes);
static pascal void _ST_DoubleBack(SndChannelPtr channel, SndDoubleBufferPtr buffer);

int32_t _ST_CreateSoundPump(void)
{
    int32_t err;

    err = 0;
    MemoryClear(&gDoubleHeader, sizeof(SndDoubleBufferHeader));
    gDoubleHeader.dbhNumChannels = gNumChannels;
    gDoubleHeader.dbhSampleSize = gSampleBits;
    gDoubleHeader.dbhCompressionID = 0;
    gDoubleHeader.dbhPacketSize = 0;
    gDoubleHeader.dbhSampleRate = gSampleRate;
    if ((gDoubleHeader.dbhBufferPtr[0] = _ST_NewBuffer(gSamplesPerBuffer, gBytesPerBuffer)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if ((gDoubleHeader.dbhBufferPtr[1] = _ST_NewBuffer(gSamplesPerBuffer, gBytesPerBuffer)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if ((gDoubleHeader.dbhDoubleBack = NewRoutineDescriptor((ProcPtr)_ST_DoubleBack, 0x3c0, kPowerPCISA)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if ((gSoundChannel = _ST_NewChannel()) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if (SndNewChannel(&gSoundChannel, sampledSynth, gNumChannels == 1 ? initMono : initStereo, 0) != 0) {
        err = kOutOfMemory;
        goto done;
    }
    if (SndPlayDoubleBuffer(gSoundChannel, &gDoubleHeader) != 0)
        err = kNoSoundDevice;
done:
    if (err != 0)
        _ST_DisposeSoundPump();
    return err;
}

void _ST_DisposeSoundPump(void)
{
    if (gSoundChannel != 0) {
        SndDisposeChannel(gSoundChannel, 1);
        gSoundChannel = 0;
    }
    if (gDoubleHeader.dbhBufferPtr[0] != 0) {
        MemoryDeallocate(gDoubleHeader.dbhBufferPtr[0]);
        gDoubleHeader.dbhBufferPtr[0] = 0;
    }
    if (gDoubleHeader.dbhBufferPtr[1] != 0) {
        MemoryDeallocate(gDoubleHeader.dbhBufferPtr[1]);
        gDoubleHeader.dbhBufferPtr[1] = 0;
    }
}

int32_t _ST_GetCurrentSampleRate(ULong *rate)
{
    int32_t err;
    SndChannelPtr channel;

    err = 0;
    channel = 0;
    *rate = kDefaultRate;
    if (gSoundManagerVersion >= kSoundManager3) {
        if (SndNewChannel(&channel, sampledSynth, gNumChannels == 1 ? initMono : initStereo, 0) != 0) {
            err = kNoSampleRate;
            goto done;
        }
        if (SndGetInfo(channel, 'srat', rate) != 0)
            err = kNoSoundDevice;
    }
done:
    if (channel != 0)
        SndDisposeChannel(channel, 1);
    return err;
}

int32_t _ST_GetSystemVolume(ULong *left, ULong *right, ULong *max)
{
    int32_t err;
    int32_t volume;

    err = 0;
    *left = 0;
    *right = 0;
    if (max != 0)
        *max = 0;
    if (GetDefaultOutputVolume(&volume) != 0) {
        err = kNoSysVolume;
        goto done;
    }
    *left = volume & 0xffff;
    *right = (ULong)volume >> 16;
    if (max != 0)
        *max = kVolumeMax;
done:
    return err;
}

unsigned char _ST_SupportsSystemVolume(void)
{
    return 1;
}

int32_t _ST_SetSystemVolume(ULong left, ULong right)
{
    int32_t err;

    err = 0;
    if (left > 0xffff)
        left = kVolumeMax;
    if (right > 0xffff)
        right = kVolumeMax;
    if (SetDefaultOutputVolume(left | (right << 16)) != 0)
        err = kNoSysVolume;
    return err;
}

static SndChannelPtr _ST_NewChannel(void)
{
    SndChannelPtr channel;

    channel = MemoryAllocate(kChannelBytesMac);
    if (channel != 0)
        channel->qLength = stdQLength;
    return channel;
}

static SndDoubleBufferPtr _ST_NewBuffer(int32_t frames, ULong bytes)
{
    SndDoubleBufferPtr buffer;

    buffer = MemoryAllocate(bytes + 0x12);
    if (buffer != 0) {
        buffer->dbNumFrames = frames;
        buffer->dbUserInfo[0] = 0;
        buffer->dbUserInfo[0] = 0;
        if (gSampleBits == 0x10)
            MemoryClear(buffer->dbSoundData, bytes);
        else
            MemoryFill(buffer->dbSoundData, (char)0x80, bytes);
        buffer->dbFlags = dbBufferReady;
    }
    return buffer;
}

static pascal void _ST_DoubleBack(SndChannelPtr channel, SndDoubleBufferPtr buffer)
{
    SystemEnterCallback(0);
    _ST_MixerDispatch(buffer->dbSoundData, buffer->dbNumFrames);
    buffer->dbFlags = dbBufferReady;
    SystemLeaveCallback(0);
}
#else
/* Carbon: an AudioUnit's render callback, CoreAudio and AudioUnit found by name at run time */
#include <Components.h>
#include <CFString.h>

#define kDriverNoLoad       -7402
#define kDriverNoUnit       -7401
#define kDriverFailed       -7400
#define kDriverNoVolume     -7403
#define kNoSymbol           -7003
#define kPumpBytes          0x2000
#define kDefaultRate        44100
#define kUnknownProperty    'who?'
#define kVolumeScale        256.0
#define kVolumeMax          0x100
#define kFormatSize         0x28
#define kSetRenderCallback  0x17
#define kStreamFormat       8
#define kInputScope         1
#define kOutputScope        2
#define kOutputVolume       14

typedef struct StreamFormat {
    double sampleRate;
    ULong formatID;
    ULong formatFlags;
    ULong bytesPerPacket;
    ULong framesPerPacket;
    ULong bytesPerFrame;
    ULong channelsPerFrame;
    ULong bitsPerChannel;
    ULong reserved;
} StreamFormat;

typedef struct SoundPump {
    StreamFormat format;
    ULong used;
    ULong size;
    ULong spare[2];
    void *data;
} SoundPump;

typedef struct RenderBuffers {
    ULong count;
    ULong channels;
    ULong size;
    void *data;
} RenderBuffers;

typedef struct RenderCallback {
    void *proc;
    void *refcon;
} RenderCallback;

extern unsigned char _PlatformIsThreaded(void);
extern void *_PlatformFindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName);
extern void *_MachOForCFM(void *function);
extern void _SystemUnpatchTraps(void);
extern int32_t _SystemPatchTraps(void);
extern ULong UInt32_To_UFix32(ULong value);
extern void *MemoryAllocate(ULong size);
extern void MemoryDeallocate(void *p);
extern ULong gSampleRate;
extern void _ST_MixerDispatch(short *buffer, ULong frames);

ComponentInstance gSoundUnit;
SoundPump gPumpIn;
SoundPump gPumpOut;
unsigned char gSoundOpen;
int32_t (*gAudioUnitInitialize)(ComponentInstance unit);
int32_t (*gAudioUnitUninitialize)(ComponentInstance unit);
int32_t (*gAudioUnitGetPropertyInfo)(ComponentInstance unit, ULong id, ULong scope, ULong element, ULong *size, unsigned char *writable);
int32_t (*gAudioUnitGetProperty)(ComponentInstance unit, ULong id, ULong scope, ULong element, void *data, ULong *size);
int32_t (*gAudioUnitSetProperty)(ComponentInstance unit, ULong id, ULong scope, ULong element, const void *data, ULong size);
int32_t (*gAudioOutputUnitStart)(ComponentInstance unit);
int32_t (*gAudioOutputUnitStop)(ComponentInstance unit);
int32_t (*gAudioUnitGetParameter)(ComponentInstance unit, ULong id, ULong scope, ULong element, float *value);
int32_t (*gAudioUnitSetParameter)(ComponentInstance unit, ULong id, ULong scope, ULong element, float value, ULong offset);
int32_t (*gAudioHardwareGetProperty)(ULong id, ULong *size, void *data);
int32_t (*gAudioDeviceGetProperty)(ULong device, ULong channel, unsigned char input, ULong id, ULong *size, void *data);
int32_t (*gAudioDeviceSetProperty)(ULong device, const void *when, ULong channel, unsigned char input, ULong id, ULong size, const void *data);
int32_t (*gAudioDeviceGetPropertyInfo)(ULong device, ULong channel, unsigned char input, ULong id, ULong *size, unsigned char *writable);

void _ST_DisposeSoundPump(void);
int32_t _ST_LoadCoreAudio(void);
int32_t _ST_OpenUnit(ComponentInstance *unit);
int32_t _ST_GetUnitFormat(ComponentInstance *unit, StreamFormat *format, void *spare);
int32_t _ST_SetRenderProc(ComponentInstance *unit, void *proc, void *refcon, ULong element);
int32_t _ST_RenderProc(void *refcon, void *flags, void *when, ULong bus, ULong frames, RenderBuffers *buffers);

int32_t _ST_CreateSoundPump(void)
{
    int32_t err;
    ULong size;
    StreamFormat format;

    err = 0;
    size = 0;
    format.sampleRate = gSampleRate != 0 ? gSampleRate / 65536.0 : kDefaultRate;
    format.formatFlags = 0xc;
    format.formatID = 'lpcm';
    format.formatFlags = 0xe;
    format.bytesPerPacket = 4;
    format.framesPerPacket = 1;
    format.bytesPerFrame = 4;
    format.channelsPerFrame = 2;
    format.bitsPerChannel = 0x10;
    format.reserved = 0;
    if (_ST_LoadCoreAudio() != 0) {
        err = kDriverNoLoad;
        goto done;
    }
    if (_ST_OpenUnit(&gSoundUnit) != 0) {
        err = kDriverNoUnit;
        goto done;
    }
    if (_ST_GetUnitFormat(&gSoundUnit, &gPumpOut.format, 0) != 0) {
        err = kDriverFailed;
        goto done;
    }
    if (gAudioUnitSetProperty(gSoundUnit, kStreamFormat, kInputScope, 0, &format, kFormatSize) != 0) {
        err = kDriverFailed;
        goto done;
    }
    size = kFormatSize;
    if (gAudioUnitGetProperty(gSoundUnit, kStreamFormat, kInputScope, 0, &format, &size) != 0) {
        err = kDriverFailed;
        goto done;
    }
    gPumpOut.format = format;
    gPumpIn.data = MemoryAllocate(kPumpBytes);
    gPumpIn.size = kPumpBytes;
    gPumpIn.used = 0;
    gPumpOut.data = MemoryAllocate(kPumpBytes);
    gPumpOut.size = kPumpBytes;
    gPumpOut.used = 0;
    if (gAudioUnitInitialize(gSoundUnit) != 0) {
        err = kDriverFailed;
        goto done;
    }
    if (_ST_SetRenderProc(&gSoundUnit, _MachOForCFM(_ST_RenderProc), 0, 0) != 0) {
        err = kDriverFailed;
        goto done;
    }
    if (gAudioOutputUnitStart(gSoundUnit) != 0) {
        err = kDriverFailed;
        goto done;
    }
    if (_PlatformIsThreaded()) {
        int32_t e;

        _SystemUnpatchTraps();
        e = _SystemPatchTraps();
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    gSoundOpen = 1;
done:
    if (err != 0)
        _ST_DisposeSoundPump();
    return err;
}

void _ST_DisposeSoundPump(void)
{
    gAudioOutputUnitStop(gSoundUnit);
    gAudioUnitUninitialize(gSoundUnit);
    CloseComponent(gSoundUnit);
    MemoryDeallocate(gPumpIn.data);
    MemoryDeallocate(gPumpOut.data);
}

int32_t _ST_GetCurrentSampleRate(ULong *rate)
{
    if (gSoundOpen)
        *rate = UInt32_To_UFix32(gPumpOut.format.sampleRate);
    else
        *rate = UInt32_To_UFix32(kDefaultRate);
    return 0;
}

int32_t _ST_GetAppVolume(ULong *left, ULong *right, ULong *max)
{
    int32_t err;
    float leftVolume;
    float rightVolume;

    err = 0;
    leftVolume = 0.0f;
    rightVolume = 0.0f;
    if (gAudioUnitGetParameter(gSoundUnit, kOutputVolume, kOutputScope, 0, &leftVolume) != 0 ||
        gAudioUnitGetParameter(gSoundUnit, kOutputVolume, kOutputScope, 1, &rightVolume) != 0) {
        err = kDriverNoVolume;
        goto done;
    }
    *left = kVolumeScale * leftVolume;
    *right = kVolumeScale * rightVolume;
    if (max != 0)
        *max = kVolumeMax;
done:
    return err;
}

int32_t _ST_SetAppVolume(ULong left, ULong right)
{
    int32_t err;
    float leftVolume;
    float rightVolume;

    err = 0;
    leftVolume = left;
    rightVolume = right;
    leftVolume = leftVolume / kVolumeScale;
    rightVolume = rightVolume / kVolumeScale;
    if (gAudioUnitSetParameter(gSoundUnit, kOutputVolume, kOutputScope, 0, leftVolume, 0) != 0 ||
        gAudioUnitSetParameter(gSoundUnit, kOutputVolume, kOutputScope, 1, rightVolume, 0) != 0)
        err = kDriverNoVolume;
    return err;
}

unsigned char _ST_SupportsSystemVolume(void)
{
    int32_t err;
    int32_t e;
    unsigned char writable;
    ULong size;
    ULong device;
    ULong channels[2] = { 0 };

    err = 0;
    if (_ST_LoadCoreAudio() != 0) {
        err = kDriverNoLoad;
        goto done;
    }
    size = sizeof(device);
    if (gAudioHardwareGetProperty('dOut', &size, &device) != 0) {
        err = kDriverFailed;
        goto done;
    }
    size = sizeof(channels);
    e = gAudioDeviceGetProperty(device, 0, 0, 'dch2', &size, channels);
    if (e != 0)
        err = e;
    if (err != 0)
        BlockZero(channels, sizeof(channels));
    e = gAudioDeviceGetPropertyInfo(device, channels[0], 0, 'volm', 0, &writable);
    if (e != kUnknownProperty && e != 0) {
        err = kDriverNoVolume;
        goto done;
    }
    err = gAudioDeviceGetPropertyInfo(device, channels[1], 0, 'volm', 0, &writable);
    if (err != kUnknownProperty) {
        if (err != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    } else {
        size = sizeof(ULong);
        err = 0;
        if (gAudioDeviceGetPropertyInfo(device, 0, 0, 'volm', 0, &writable) != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    }
    if (!writable)
        err = kDriverNoVolume;
done:
    return err == 0;
}

int32_t _ST_GetSystemVolume(ULong *left, ULong *right, ULong *max)
{
    int32_t err;
    int32_t e;
    float leftVolume;
    float rightVolume;
    ULong device;
    ULong size;
    ULong channels[2] = { 0 };

    err = 0;
    leftVolume = 0.0f;
    rightVolume = 0.0f;
    size = sizeof(device);
    if (gAudioHardwareGetProperty('dOut', &size, &device) != 0) {
        err = kDriverFailed;
        goto done;
    }
    size = sizeof(channels);
    e = gAudioDeviceGetProperty(device, 0, 0, 'dch2', &size, channels);
    if (e != 0)
        err = e;
    if (err != 0)
        BlockZero(channels, sizeof(channels));
    size = sizeof(float);
    e = gAudioDeviceGetProperty(device, channels[0], 0, 'volm', &size, &leftVolume);
    if (e != kUnknownProperty) {
        if (e != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    } else {
        size = sizeof(float);
        if (gAudioDeviceGetProperty(device, 0, 0, 'volm', &size, &leftVolume) != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    }
    size = sizeof(float);
    err = gAudioDeviceGetProperty(device, channels[1], 0, 'volm', &size, &rightVolume);
    if (err != kUnknownProperty) {
        if (err != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    } else {
        size = sizeof(float);
        err = 0;
        if (gAudioDeviceGetProperty(device, 0, 0, 'volm', &size, &rightVolume) != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    }
    *left = kVolumeScale * leftVolume;
    *right = kVolumeScale * rightVolume;
    if (max != 0)
        *max = kVolumeMax;
done:
    return err;
}

int32_t _ST_SetSystemVolume(ULong left, ULong right)
{
    int32_t err;
    int32_t e;
    ULong size;
    ULong device;
    float leftVolume;
    float rightVolume;
    unsigned char writable;
    ULong channels[2] = { 0 };

    err = 0;
    if (_ST_LoadCoreAudio() != 0) {
        err = kDriverNoLoad;
        goto done;
    }
    leftVolume = left;
    rightVolume = right;
    size = sizeof(device);
    leftVolume = leftVolume / kVolumeScale;
    rightVolume = rightVolume / kVolumeScale;
    if (gAudioHardwareGetProperty('dOut', &size, &device) != 0) {
        err = kDriverFailed;
        goto done;
    }
    size = sizeof(channels);
    e = gAudioDeviceGetProperty(device, 0, 0, 'dch2', &size, channels);
    if (e != 0)
        err = e;
    if (err != 0)
        BlockZero(channels, sizeof(channels));
    e = gAudioDeviceGetPropertyInfo(device, channels[0], 0, 'volm', 0, &writable);
    if (e != kUnknownProperty) {
        if (e != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    } else {
        size = sizeof(float);
        if (gAudioDeviceGetPropertyInfo(device, 0, 0, 'volm', 0, &writable) != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    }
    if (!writable) {
        err = kDriverNoVolume;
        goto done;
    }
    e = gAudioDeviceSetProperty(device, 0, channels[0], 0, 'volm', sizeof(float), &leftVolume);
    if (e != kUnknownProperty) {
        if (e != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    } else {
        if (gAudioDeviceSetProperty(device, 0, 0, 0, 'volm', sizeof(float), &leftVolume) != 0) {
            err = kDriverNoVolume;
            goto done;
        }
    }
    err = gAudioDeviceSetProperty(device, 0, channels[1], 0, 'volm', sizeof(float), &rightVolume);
    if (err != kUnknownProperty) {
        if (err != 0)
            err = kDriverNoVolume;
    } else {
        /* BUG: a retry on the master channel that succeeds still returns kUnknownProperty */
        if (gAudioDeviceSetProperty(device, 0, 0, 0, 'volm', sizeof(float), &rightVolume) != 0)
            err = kDriverNoVolume;
    }
done:
    return err;
}

int32_t _ST_RenderProc(void *refcon, void *flags, void *when, ULong bus, ULong frames, RenderBuffers *buffers)
{
    _ST_MixerDispatch(buffers->data, frames);
    buffers->count = 1;
    buffers->channels = 2;
    return 0;
}

int32_t _ST_OpenUnit(ComponentInstance *unit)
{
    int32_t err = 0;
    ComponentDescription description;
    Component component;

    description.componentType = 'auou';
    description.componentSubType = 'def ';
    description.componentManufacturer = 'appl';
    description.componentFlags = 0;
    description.componentFlagsMask = 0;
    component = FindNextComponent(0, &description);
    if (component == 0) {
        err = kDriverNoUnit;
        goto done;
    }
    if (OpenAComponent(component, unit) != 0)
        err = kDriverNoUnit;
done:
    return err;
}

int32_t _ST_GetUnitFormat(ComponentInstance *unit, StreamFormat *format, void *spare)
{
    int32_t err = 0;
    ULong size;
    unsigned char writable;

    if (gAudioUnitGetPropertyInfo(*unit, kStreamFormat, kOutputScope, 0, &size, &writable) != 0) {
        err = kDriverFailed;
        goto done;
    }
    if (gAudioUnitGetProperty(*unit, kStreamFormat, kOutputScope, 0, format, &size) != 0) {
        err = kDriverFailed;
        goto done;
    }
    if (gAudioUnitSetProperty(*unit, kStreamFormat, kInputScope, (ULong)spare, format, size) != 0)
        err = kDriverFailed;
done:
    return err;
}

int32_t _ST_SetRenderProc(ComponentInstance *unit, void *proc, void *refcon, ULong element)
{
    int32_t err = 0;
    RenderCallback callback;

    BlockZero(&callback, sizeof(callback));
    callback.proc = proc;
    callback.refcon = refcon;
    if (gAudioUnitSetProperty(*unit, kSetRenderCallback, kInputScope, element, &callback, sizeof(callback)) != 0)
        err = kDriverFailed;
    return err;
}

#define LOAD_SYMBOL(pointer, framework, name)                                   \
    if (pointer == 0) {                                                         \
        *(void **)&pointer = _PlatformFindBundleSymbol(CFSTR(framework), CFSTR(name)); \
        if (pointer == 0) {                                                     \
            err = kNoSymbol;                                                    \
            goto done;                                                          \
        }                                                                       \
    }

int32_t _ST_LoadCoreAudio(void)
{
    int32_t err = 0;

    LOAD_SYMBOL(gAudioUnitInitialize, "AudioUnit.framework", "AudioUnitInitialize")
    LOAD_SYMBOL(gAudioUnitUninitialize, "AudioUnit.framework", "AudioUnitUninitialize")
    LOAD_SYMBOL(gAudioOutputUnitStart, "AudioUnit.framework", "AudioOutputUnitStart")
    LOAD_SYMBOL(gAudioOutputUnitStop, "AudioUnit.framework", "AudioOutputUnitStop")
    LOAD_SYMBOL(gAudioUnitGetParameter, "AudioUnit.framework", "AudioUnitGetParameter")
    LOAD_SYMBOL(gAudioUnitSetParameter, "AudioUnit.framework", "AudioUnitSetParameter")
    LOAD_SYMBOL(gAudioUnitGetPropertyInfo, "AudioUnit.framework", "AudioUnitGetPropertyInfo")
    LOAD_SYMBOL(gAudioUnitGetProperty, "AudioUnit.framework", "AudioUnitGetProperty")
    LOAD_SYMBOL(gAudioUnitSetProperty, "AudioUnit.framework", "AudioUnitSetProperty")
    LOAD_SYMBOL(gAudioHardwareGetProperty, "CoreAudio.framework", "AudioHardwareGetProperty")
    LOAD_SYMBOL(gAudioDeviceSetProperty, "CoreAudio.framework", "AudioDeviceSetProperty")
    LOAD_SYMBOL(gAudioDeviceGetProperty, "CoreAudio.framework", "AudioDeviceGetProperty")
    LOAD_SYMBOL(gAudioDeviceGetPropertyInfo, "CoreAudio.framework", "AudioDeviceGetPropertyInfo")
done:
    return err;
}
#endif
