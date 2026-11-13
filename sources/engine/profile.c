// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* profile.c -- the engine's own 64-slot sampling profiler (win TU region 0x0046fbf0..0x0046ff00). Notes: Research/notes/profile.md */
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "burger.h"

#define kProfileSlots 64

#if V111 || defined(MACBUILD) /* V111 spelling */
#ifdef MACBUILD
UnsignedWide lastMicroseconds[kProfileSlots];
#else
static unsigned long long lastMicroseconds[kProfileSlots];
#endif
uint32_t iterations[kProfileSlots];
uint32_t totalMilliseconds[kProfileSlots];
char name[kProfileSlots][32];
short dependency[kProfileSlots];
short lastEntered;
Boolean done[kProfileSlots];
#else
static Boolean done[kProfileSlots];          /* dump bookkeeping */
static short lastEntered;
static short dependency[kProfileSlots];          /* -1 = none */
static char name[kProfileSlots][32];
static uint32_t totalMilliseconds[kProfileSlots];    /* milliseconds */
static uint32_t iterations[kProfileSlots];
static unsigned long long lastMicroseconds[kProfileSlots];
#endif

/* print_dependents (mac _print_dependents). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fa30
// MAC: _print_dependents i386 0x000a4e86 ppc 0x000b0b80
#ifndef MACBUILD
void print_dependents(FILE *f, short slot, short depth)
#include "../engine/printdependents.inc"
#endif

/* MyProfileInit (mac _MyProfileInit). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fbf0
// MAC: _MyProfileInit i386 0x000a514f ppc 0x000b0eb8
void MyProfileInit(void)
{
    short i;

    for (i = 0; i < kProfileSlots; i++)
        strcpy(name[i], "");
    MyProfileReset();
}

/* MyProfileReset. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fc10
// MAC: _MyProfileReset i386 0x000a4d23 ppc 0x000b0988
void MyProfileReset(void)
{
    short i;

    for (i = 0; i < kProfileSlots; i++) {
#ifdef MACBUILD
        lastMicroseconds[i].hi = 0;
        lastMicroseconds[i].lo = 0;
#else
        lastMicroseconds[i] = 0;
#endif
        totalMilliseconds[i] = 0;
        dependency[i] = -1;
        iterations[i] = 0;
    }
    lastEntered = -1;
}

/* MyProfileSetName (mac _MyProfileSetName). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fde0
// MAC: _MyProfileSetName i386 0x000a4d8a ppc 0x000b09f0
void MyProfileSetName(short slot, const char *slotName)
{
    if (slot < 0 || slot >= kProfileSlots)
        return;
    strncpy(name[slot], slotName, 31);
}

/* MyProfileEnter (mac _MyProfileEnter). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fe20
// MAC: _MyProfileEnter i386 0x000a4dbd ppc 0x000b0a14
void MyProfileEnter(short slot)
{
    if (slot < 0 || slot >= kProfileSlots)
        return;
#ifdef MACBUILD
    Microseconds(&lastMicroseconds[slot]);
#else
    lastMicroseconds[slot] = TimerGetMicroseconds();
#endif
    if (lastEntered != -1 && dependency[slot] == -1)
        dependency[slot] = lastEntered;
    lastEntered = slot;
}

/* MyProfileExit (mac _MyProfileExit). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fe80
// MAC: _MyProfileExit i386 0x000a4e11 ppc 0x000b0a94
void MyProfileExit(short slot)
{
#ifdef MACBUILD
    UnsignedWide now;
    uint32_t elapsed;
#else
    unsigned long long now;
#endif

    if (slot < 0 || slot >= kProfileSlots)
        return;
#ifdef MACBUILD
    Microseconds(&now);
    if (now.hi > lastMicroseconds[slot].hi)
        elapsed = (0xffffffff - (lastMicroseconds[slot].lo - now.lo)) / 1000;
    else
        elapsed = (now.lo - lastMicroseconds[slot].lo) / 1000;
    totalMilliseconds[slot] += elapsed;
#else
    now = TimerGetMicroseconds();
    totalMilliseconds[slot] += ((now - lastMicroseconds[slot]) / 1000);
#endif
    iterations[slot]++;
    if (lastEntered == slot) {
        if (dependency[slot] != -1)
            lastEntered = dependency[slot];
        else
            lastEntered = -1;
    }
}

#ifdef MACBUILD
void print_dependents(FILE *f, short slot, short depth)
#include "../engine/printdependents.inc"
#endif

/* MyProfileDump (mac _MyProfileDump). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ff00
// MAC: _MyProfileDump i386 0x000a5088 ppc 0x000b0dc0
void MyProfileDump(const char *path)
{
    FILE *f;
    short i;

    f = fopen(path, "r");
    if (f == 0)
        return;
    fclose(f);
    f = fopen(path, "w");
    if (f != 0) {
        fprintf(f, "Profile report:\n\n");
        for (i = 0; i < kProfileSlots; i++)
            done[i] = 0;
        for (i = 0; i < kProfileSlots; i++) {
            if (iterations[i] != 0 && done[i] == 0)
                print_dependents(f, i, 0);
        }
        fclose(f);
    }
}
