// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atplatlock2.c -- atplatlock, exe run 2. Notes: Research/notes/atplatlock.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

ULong gPlatLockOwnerThread;                              /* 0x008705d0 owning thread */
static long gAtomicCount;                        /* 0x0086b6b8 lock depth */

/* AtomicEnterCritical. STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505f03
void AtomicEnterCritical(void *token)
{
    long *state;
    long err;

    state = (long *)token;
    err = 0;
    if (gPlatLockOwnerThread != GetCurrentThreadId()) {
        EnterCriticalSection(gPlatCritSec);
        gPlatLockOwnerThread = GetCurrentThreadId();
    }
    gAtomicCount = gAtomicCount + 1;
}

/* AtomicLeaveCritical. STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505f36
void AtomicLeaveCritical(void *token)
{
    long *state;
    long err;

    state = (long *)token;
    err = 0;
    gAtomicCount = gAtomicCount - 1;
    if (gAtomicCount == 0) {
        GetCurrentThreadId();
        gPlatLockOwnerThread = 0;
        LeaveCriticalSection(gPlatCritSec);
    }
}

/* Atomic bit test, test-and-set and test-and-clear on a state word, each returning the
   bit's previous value. With InterlockedCompareExchange resolved and the word
   dword-aligned they spin on it; otherwise they take the recursive lock above. */

typedef long (__stdcall *CompareExchangeProc)(ULong *dest, ULong exchange,
                                             ULong comparand);

extern void *gPlatCmpXchgProc;                       /* atplatopen.c */

#define kWordAlignMask 3
#define kMisaligned -7001

typedef long (__stdcall *ExchangeAddProc)(long *dest, long delta);

extern void *gPlatExchangeAddProc;                       /* atplatopen.c */

/* AtomicIncrement (atomic add answering the old value: InterlockedExchangeAdd when resolved
   and the word aligned, else under the recursive lock). STATE: MATCH 100%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505f66
long AtomicIncrement(long *value, long delta)
{
    char token[16];
    long old;
    long err;
    ULong misaligned;

    err = 0;
    old = 0;
    misaligned = (ULong)value & kWordAlignMask;
    if (misaligned != 0)
        err = kMisaligned;
    if (gPlatExchangeAddProc != 0 && misaligned == 0) {
        old = ((ExchangeAddProc)gPlatExchangeAddProc)(value, delta);
    } else {
        AtomicEnterCritical(token);
        old = *value;
        *value += delta;
        AtomicLeaveCritical(token);
    }
    return old;
}

/* AtomicMaskApply. STATE: MATCH 100%, exe 63 / obj 63, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505fc2
unsigned char AtomicMaskApply(ULong *word, ULong bit)
{
    char token[16];
    unsigned char old;
    ULong mask;
    ULong cur;
    long err;
    ULong next;
    ULong result;

    err = 0;
    old = 1;
    if (gPlatCmpXchgProc != 0 && ((ULong)word & kWordAlignMask) == 0) {
        do {
            mask = 1 << bit;
            cur = *word;
            next = cur;
            next |= mask;
            result = ((CompareExchangeProc)gPlatCmpXchgProc)(word, next, cur);
        } while (result != cur);
        if ((cur & mask) != 0)
            old = 1;
        else
            old = 0;
    } else {
        bit = 1 << bit;
        if ((*word & bit) == 0) {
            AtomicEnterCritical(token);
            if ((*word & bit) != 0)
                old = 1;
            else
                old = 0;
            *word |= bit;
            AtomicLeaveCritical(token);
        }
    }
    return old;
}

/* AtomicMaskClear. STATE: MATCH 100%, exe 66 / obj 66, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00506046
unsigned char AtomicMaskClear(ULong *word, ULong bit)
{
    char token[16];
    unsigned char old;
    ULong mask;
    ULong cur;
    long err;
    ULong next;
    ULong result;

    err = 0;
    old = 0;
    if (gPlatCmpXchgProc != 0 && ((ULong)word & kWordAlignMask) == 0) {
        do {
            mask = 1 << bit;
            next = ~mask;
            cur = *word;
            next &= cur;
            result = ((CompareExchangeProc)gPlatCmpXchgProc)(word, next, cur);
        } while (result != cur);
        if ((cur & mask) != 0)
            old = 1;
        else
            old = 0;
    } else {
        bit = mask = 1 << bit;
        if ((*word & bit) != 0) {
            AtomicEnterCritical(token);
            if ((*word & bit) != 0)
                old = 1;
            else
                old = 0;
            *word &= ~mask;
            AtomicLeaveCritical(token);
        }
    }
    return old;
}

/* AtomicMaskTest. STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005060d0
unsigned char AtomicMaskTest(ULong *word, ULong bit)
{
    long err;
    unsigned char result;

    err = 0;
    result = 0;
    result = (*word & (1 << bit)) ? true : false;
    return result;
}

/* _AtomicCheckCritical. STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005060f4
unsigned char _AtomicCheckCritical(void)
{
    unsigned char owner;
    ULong id;
    long err;

    err = 0;
    owner = 0;
    id = 0;
    if (gPlatLockOwnerThread == GetCurrentThreadId())
        owner = 1;
    return owner;
}

/* _AtomicResetCritical. STATE: MATCH 100%, exe 3 / obj 3, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00506113
void _AtomicResetCritical(void)
{
    gAtomicCount = 0;
}
