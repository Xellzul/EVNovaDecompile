/* atomicmac.c -- 1.0.10: AmbrosiaTools' atomics: lwarx and stwcx. on an aligned word, 68k code that masks interrupts otherwise. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"

#define kProcDisable    0x32
#define kProcEnable     0x1802
#define kDurationForever    0x7fffffff

extern void *gPlatformMutex;
extern int32_t (*gCallUniversalProc)(UniversalProcPtr proc, ULong info, ...);
extern unsigned char _PlatformIsThreaded(void);
extern int32_t _MutexEnter(struct TaskMutex *mutex, int32_t timeout);
extern int32_t _MutexLeave(struct TaskMutex *mutex);
extern int32_t _MutexDepth(struct TaskMutex *mutex, ULong *depth);

extern void _AtomicEnter(void);
extern void _AtomicLeave(void);

static ULong gAtomicDepth;

static int32_t _AtomicExchange(int32_t *word, int32_t compare, int32_t value);
static int32_t _AtomicAdd(int32_t *word, int32_t delta);
static int32_t _AtomicOr(ULong *word, ULong bit);
static int32_t _AtomicAnd(ULong *word, ULong bit);

void AtomicEnterCritical(void *token)
{
    if (gAtomicDepth == 0) {
        int32_t code[3] = { 0x40c0007c, 0x07004e75, 0 };

#if TARGET_API_MAC_CARBON
        if (_PlatformIsThreaded())
            _MutexEnter(gPlatformMutex, kDurationForever);
        else
            *(int32_t *)token = gCallUniversalProc((UniversalProcPtr)code, kProcDisable);
#else
        *(int32_t *)token = CallUniversalProc((UniversalProcPtr)code, kProcDisable);
#endif
    }
    gAtomicDepth++;
}

void AtomicLeaveCritical(void *token)
{
    if (--gAtomicDepth == 0) {
        unsigned short code[3] = { 0x46c0, 0x4e75, 0 };

#if TARGET_API_MAC_CARBON
        if (_PlatformIsThreaded())
            _MutexLeave(gPlatformMutex);
        else
            gCallUniversalProc((UniversalProcPtr)code, kProcEnable, *(int32_t *)token);
#else
        CallUniversalProc((UniversalProcPtr)code, kProcEnable, *(int32_t *)token);
#endif
    }
}

int32_t AtomicCompareSwap(void *word, int32_t compare, int32_t value)
{
    int32_t old;
    int32_t state[4];

    if (((ULong)word & 3) == 0) {
        _AtomicEnter();
        old = _AtomicExchange(word, compare, value);
        _AtomicLeave();
    } else {
        AtomicEnterCritical(state);
        old = *(int32_t *)word;
        *(int32_t *)word = value;
        AtomicLeaveCritical(state);
    }
    return old;
}

int32_t AtomicIncrement(int32_t *value, int32_t delta)
{
    int32_t old;
    int32_t state[4];

    if (((ULong)value & 3) == 0) {
        _AtomicEnter();
        old = _AtomicAdd(value, delta);
        _AtomicLeave();
    } else {
        AtomicEnterCritical(state);
        old = *value;
        *value = old + delta;
        AtomicLeaveCritical(state);
    }
    return old;
}

int32_t AtomicMaskApply(ULong *word, ULong bit)
{
    int32_t mask;
    int32_t state[4];
    int32_t result;

    mask = bit;
    result = 1;
    if (((ULong)word & 3) == 0) {
        _AtomicEnter();
        result = _AtomicOr(word, mask);
        _AtomicLeave();
    } else if ((*word & (mask = 1UL << mask)) == 0) {
        AtomicEnterCritical(state);
        result = (*word & mask) != 0;
        *word |= mask;
        AtomicLeaveCritical(state);
    }
    return result;
}

int32_t AtomicMaskClear(ULong *word, ULong bit)
{
    ULong old;
    int32_t mask;
    int32_t state[4];
    int32_t result;

    mask = bit;
    result = 0;
    if (((ULong)word & 3) == 0) {
        _AtomicEnter();
        result = _AtomicAnd(word, mask);
        _AtomicLeave();
    } else if ((*word & (mask = 1UL << mask)) != 0) {
        AtomicEnterCritical(state);
        old = *word;
        result = (old & mask) != 0;
        *word = old & ~mask;
        AtomicLeaveCritical(state);
    }
    return result;
}

unsigned char AtomicMaskTest(ULong *word, ULong bit)
{
    int32_t err;
    unsigned char result;

    err = 0;
    result = 0;
    result = (*word & (1 << bit)) ? true : false;
    return result;
}

int32_t _AtomicCheckCritical(void)
{
    int32_t result;
#if TARGET_API_MAC_CARBON
    ULong depth;

    depth = 0;
    result = 0;
    if (gAtomicDepth != 0) {
        if (_PlatformIsThreaded()) {
            _MutexDepth(gPlatformMutex, &depth);
            if (depth != 0)
                result = 1;
        } else {
            result = 1;
        }
    }
#else
    result = 0;
    if (gAtomicDepth != 0)
        result = 1;
#endif
    return result;
}

void _AtomicResetCritical(void)
{
    if (gAtomicDepth != 0) {
        unsigned short code[3] = { 0x46c0, 0x4e75, 0 };

#if TARGET_API_MAC_CARBON
        if (!_PlatformIsThreaded())
            gCallUniversalProc((UniversalProcPtr)code, kProcEnable, 0);
#else
        CallUniversalProc((UniversalProcPtr)code, kProcEnable, 0);
#endif
    }
    gAtomicDepth = 0;
}

static asm int32_t _AtomicExchange(int32_t *word, int32_t compare, int32_t value)
{
    nofralloc
    li      r0, 0
@retry:
    lwarx   r6, r0, r3
    cmpw    r6, r4
    bne     @done
    stwcx.  r5, r0, r3
    bne     @retry
    sync
@done:
    mr      r3, r6
    blr
}

static asm int32_t _AtomicAdd(int32_t *word, int32_t delta)
{
    nofralloc
    li      r0, 0
@retry:
    lwarx   r5, r0, r3
    add     r6, r5, r4
    stwcx.  r6, r0, r3
    bne     @retry
    sync
    mr      r3, r5
    blr
}

static asm int32_t _AtomicOr(ULong *word, ULong bit)
{
    nofralloc
    li      r0, 0
    li      r5, 1
    slw     r5, r5, r4
@retry:
    lwarx   r6, r0, r3
    or      r7, r6, r5
    stwcx.  r7, r0, r3
    bne     @retry
    sync
    and.    r3, r6, r5
    beqlr
    li      r3, 1
    blr
}

static asm int32_t _AtomicAnd(ULong *word, ULong bit)
{
    nofralloc
    li      r0, 0
    li      r5, 1
    slw     r5, r5, r4
@retry:
    lwarx   r6, r0, r3
    andc    r7, r6, r5
    stwcx.  r7, r0, r3
    bne     @retry
    sync
    and.    r3, r6, r5
    beqlr
    li      r3, 1
    blr
}
