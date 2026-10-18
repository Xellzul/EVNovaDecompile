/* burgerdsound.h -- the DirectSound interfaces BurgerLib's sound driver calls
 * (atsnddrv.c). Types only, so including it exposes no TU to a prototype;
 * the wave format is win32.h's, reached here by its tag.
 */

#ifndef BURGERDSOUND_H
#define BURGERDSOUND_H

#include "burgertypes.h"
#include "burger.h"

struct WaveFormat;
struct DSound;
struct DSoundBuffer;

/* DSBUFFERDESC */
typedef struct DSBufferDesc {
    ULong size;
    ULong flags;
    ULong bufferBytes;
    ULong reserved;
    struct WaveFormat *format;
    unsigned char algorithm[16];
} DSBufferDesc;

#ifdef __cplusplus

/* In C++ the interfaces are classes, as the SDK's DECLARE_INTERFACE gives them:
   a method call loads `this' and then the vtable through it. */
struct DSound {
    virtual long __stdcall QueryInterface(const void *iid, void **out) = 0;
    virtual ULong __stdcall AddRef(void) = 0;
    virtual ULong __stdcall Release(void) = 0;
    virtual long __stdcall CreateSoundBuffer(const DSBufferDesc *desc, DSoundBuffer **out,
                                             void *outer) = 0;
    virtual long __stdcall GetCaps(void *caps) = 0;
    virtual long __stdcall DuplicateSoundBuffer(DSoundBuffer *in, DSoundBuffer **out) = 0;
    virtual long __stdcall SetCooperativeLevel(void *hwnd, ULong level) = 0;
};

struct DSoundBuffer {
    virtual long __stdcall QueryInterface(const void *iid, void **out) = 0;
    virtual ULong __stdcall AddRef(void) = 0;
    virtual ULong __stdcall Release(void) = 0;
    virtual long __stdcall GetCaps(void *caps) = 0;
    virtual long __stdcall GetCurrentPosition(ULong *play, ULong *write) = 0;
    virtual long __stdcall GetFormat(struct WaveFormat *format, ULong size, ULong *written) = 0;
    virtual long __stdcall GetVolume(long *volume) = 0;
    virtual long __stdcall GetPan(long *pan) = 0;
    virtual long __stdcall GetFrequency(ULong *freq) = 0;
    virtual long __stdcall GetStatus(ULong *status) = 0;
    virtual long __stdcall Initialize(DSound *ds, const DSBufferDesc *desc) = 0;
    virtual long __stdcall Lock(ULong offset, ULong bytes, void **p1, ULong *n1,
                                void **p2, ULong *n2, ULong flags) = 0;
    virtual long __stdcall Play(ULong reserved1, ULong priority, ULong flags) = 0;
    virtual long __stdcall SetCurrentPosition(ULong pos) = 0;
    virtual long __stdcall SetFormat(const struct WaveFormat *format) = 0;
    virtual long __stdcall SetVolume(long volume) = 0;
    virtual long __stdcall SetPan(long pan) = 0;
    virtual long __stdcall SetFrequency(ULong freq) = 0;
    virtual long __stdcall Stop(void) = 0;
    virtual long __stdcall Unlock(void *p1, ULong n1, void *p2, ULong n2) = 0;
    virtual long __stdcall Restore(void) = 0;
};

#else

typedef struct DSoundVtbl {
    long (__stdcall *QueryInterface)(struct DSound *self, const void *iid, void **out);
    ULong (__stdcall *AddRef)(struct DSound *self);
    ULong (__stdcall *Release)(struct DSound *self);
    long (__stdcall *CreateSoundBuffer)(struct DSound *self, const DSBufferDesc *desc,
                                        struct DSoundBuffer **out, void *outer);
    long (__stdcall *GetCaps)(struct DSound *self, void *caps);
    long (__stdcall *DuplicateSoundBuffer)(struct DSound *self, struct DSoundBuffer *in,
                                           struct DSoundBuffer **out);
    long (__stdcall *SetCooperativeLevel)(struct DSound *self, void *hwnd, ULong level);
} DSoundVtbl;

typedef struct DSound {
    DSoundVtbl *vtbl;
} DSound;

typedef struct DSoundBufferVtbl {
    long (__stdcall *QueryInterface)(struct DSoundBuffer *self, const void *iid, void **out);
    ULong (__stdcall *AddRef)(struct DSoundBuffer *self);
    ULong (__stdcall *Release)(struct DSoundBuffer *self);
    long (__stdcall *GetCaps)(struct DSoundBuffer *self, void *caps);
    long (__stdcall *GetCurrentPosition)(struct DSoundBuffer *self, ULong *play,
                                         ULong *write);
    long (__stdcall *GetFormat)(struct DSoundBuffer *self, struct WaveFormat *format,
                                ULong size, ULong *written);
    long (__stdcall *GetVolume)(struct DSoundBuffer *self, long *volume);
    long (__stdcall *GetPan)(struct DSoundBuffer *self, long *pan);
    long (__stdcall *GetFrequency)(struct DSoundBuffer *self, ULong *freq);
    long (__stdcall *GetStatus)(struct DSoundBuffer *self, ULong *status);
    long (__stdcall *Initialize)(struct DSoundBuffer *self, DSound *ds,
                                 const DSBufferDesc *desc);
    long (__stdcall *Lock)(struct DSoundBuffer *self, ULong offset, ULong bytes,
                           void **p1, ULong *n1, void **p2, ULong *n2, ULong flags);
    long (__stdcall *Play)(struct DSoundBuffer *self, ULong reserved1,
                           ULong priority, ULong flags);
    long (__stdcall *SetCurrentPosition)(struct DSoundBuffer *self, ULong pos);
    long (__stdcall *SetFormat)(struct DSoundBuffer *self, const struct WaveFormat *format);
    long (__stdcall *SetVolume)(struct DSoundBuffer *self, long volume);
    long (__stdcall *SetPan)(struct DSoundBuffer *self, long pan);
    long (__stdcall *SetFrequency)(struct DSoundBuffer *self, ULong freq);
    long (__stdcall *Stop)(struct DSoundBuffer *self);
    long (__stdcall *Unlock)(struct DSoundBuffer *self, void *p1, ULong n1,
                             void *p2, ULong n2);
    long (__stdcall *Restore)(struct DSoundBuffer *self);
} DSoundBufferVtbl;

typedef struct DSoundBuffer {
    DSoundBufferVtbl *vtbl;
} DSoundBuffer;

#endif

typedef long (__stdcall *DirectSoundCreateProc)(void *guid, DSound **out,
                                                void *outer);

#ifdef __cplusplus
extern "C" {
#endif

/* BurgerLib's PrivSound_t, one voice. */
typedef struct PrivSound {
    long SoundPtr;                               /* +0x00 */
    char pad04[0x10 - 0x04];
    void (*Proc)(void *data);                    /* +0x10 called when the voice stops */
    void *Data;                                  /* +0x14 */
    struct RezFile *SampleRezHeader;             /* +0x18 */
    ULong *CookiePtr;                            /* +0x1c the caller's cookie, reset on stop */
    long SampleResource;                         /* +0x20 resource number, or -1 */
    char pad24[0x38 - 0x24];
    struct DSoundBuffer *SampleHandle;           /* +0x38 */
    char pad3C[0x50 - 0x3c];
} PrivSound;

typedef long (*SoundProc)(PrivSound *sound, void *data);

extern PrivSound BurgerSamples[kMaxVoices + 1];  /* 0x0086e8c8 */
extern long SoundIterator(SoundProc proc, void *data, long cookie, long firstOnly);

#ifdef __cplusplus
}
#endif

#endif
