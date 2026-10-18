// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* profile.c -- the engine's own 64-slot sampling profiler (win TU region 0x0046fbf0..0x0046ff00). Notes: Research/notes/profile.md */
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "burger.h"

#define kProfileSlots 64

static char profilePrinted[kProfileSlots];          /* dump bookkeeping */
static short profileCurrent;
static short profileParent[kProfileSlots];          /* -1 = none */
static char profileName[kProfileSlots][32];
static unsigned long profileTime[kProfileSlots];    /* milliseconds */
static unsigned long profileCalls[kProfileSlots];
static unsigned long long profileEnterTick[kProfileSlots];

/* print_dependents (mac _print_dependents). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fa30
void print_dependents(FILE *f, short slot, short depth)
{
    short i;
    short col;

    for (i = 0; i < depth * 2; i++)
        fprintf(f, " ");
    fprintf(f, "%s:", profileName[slot]);
    for (col = strlen(profileName[slot]) + depth * 2; col < 32; col++)
        fprintf(f, " ");
    fprintf(f, " %8i iterations  %8i ms  %8.1f ms average",
            profileCalls[slot], profileTime[slot],
            (double)profileTime[slot] / profileCalls[slot]);
    if (profileParent[slot] != -1) {
        for (i = 0; i < (depth + 1) * 2; i++)
            fprintf(f, " ");
        fprintf(f, "(%.1f%%)",
                profileTime[slot] * 100.0 / profileTime[profileParent[slot]]);
    }
    fprintf(f, "\n");
    profilePrinted[slot] = 1;
    for (i = 0; i < kProfileSlots; i++) {
        if (profilePrinted[i] == 0 && profileParent[i] == slot)
            print_dependents(f, i, depth + 1);
    }
}

/* MyProfileInit (mac _MyProfileInit). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fbf0
void MyProfileInit(void)
{
    short i;

    for (i = 0; i < kProfileSlots; i++)
        strcpy(profileName[i], "");
    MyProfileReset();
}

/* MyProfileReset. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fc10
void MyProfileReset(void)
{
    short i;

    for (i = 0; i < kProfileSlots; i++) {
        profileEnterTick[i] = 0;
        profileTime[i] = 0;
        profileParent[i] = -1;
        profileCalls[i] = 0;
    }
    profileCurrent = -1;
}

/* MyProfileSetName (mac _MyProfileSetName). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fde0
void MyProfileSetName(short slot, const char *name)
{
    if (slot < 0 || slot >= kProfileSlots)
        return;
    strncpy(profileName[slot], name, 31);
}

/* MyProfileEnter (mac _MyProfileEnter). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fe20
void MyProfileEnter(short slot)
{
    if (slot < 0 || slot >= kProfileSlots)
        return;
    profileEnterTick[slot] = TimerGetMicroseconds();
    if (profileCurrent != -1 && profileParent[slot] == -1)
        profileParent[slot] = profileCurrent;
    profileCurrent = slot;
}

/* MyProfileExit (mac _MyProfileExit). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fe80
void MyProfileExit(short slot)
{
    unsigned long long now;

    if (slot < 0 || slot >= kProfileSlots)
        return;
    now = TimerGetMicroseconds();
    profileTime[slot] += ((now - profileEnterTick[slot]) / 1000);
    profileCalls[slot]++;
    if (profileCurrent == slot) {
        if (profileParent[slot] != -1)
            profileCurrent = profileParent[slot];
        else
            profileCurrent = -1;
    }
}

/* MyProfileDump (mac _MyProfileDump). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ff00
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
            profilePrinted[i] = 0;
        for (i = 0; i < kProfileSlots; i++) {
            if (profileCalls[i] != 0 && profilePrinted[i] == 0)
                print_dependents(f, i, 0);
        }
        fclose(f);
    }
}
