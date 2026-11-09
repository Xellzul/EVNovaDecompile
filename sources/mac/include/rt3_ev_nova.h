/* rt3_ev_nova.h -- the registration library's EV Nova extras: the scramble helpers EnterGameFromMainScreen
   and InitEVNova fold into their license checks. The mac 1.1.1 Main Screen.o and Escape Velocity.o inline
   every one (no symbol); the win port kept them as functions in commdlg.c, gameview.c and ai.c under the
   same names. Notes: Research/notes/mainscreen.md, Research/notes/initevnova.md */
#ifndef RT3_EV_NOVA_H
#define RT3_EV_NOVA_H

extern char *RT3_CheckLicenseName(const char *licenseName);

static inline char RegScramA3(unsigned int x)
{
    return IsPlatformOpen() * x;
}

#ifndef RT3_HELPERS3_IN_UNIT
static inline void RegScramB3(unsigned char x)
{
    static unsigned char table[256];

    table[x]++;
}
#endif

static inline short RegScramC3(unsigned char x)
{
    int n = x;

    return ((n >> (8 - n)) | (n << n)) & 0xff;
}

static inline void *RegScramF3(void)
{
    char buf[4];

    return buf;
}

static inline char RegScramD3(char *p)
{
    char c = *p;

    TimerGetSeconds();
    return c;
}

static inline char RegScramE3(void)
{
    return 0;
}

/* the same helpers under their Escape Velocity.o names (RegScramA2..E2, called by InitEVNova's license check) */
extern unsigned long RT3_ExtractLicenseBlock2(unsigned long long lic);

#ifndef RT3_HELPERS_IN_UNIT
static inline char RegScramA2(short x)
{
    return IsPlatformOpen() * x;
}

static inline void RegScramB2(unsigned char x)
{
    static unsigned char table[256];

    table[x]++;
}

static inline short RegScramC2(unsigned char x)
{
    int n = x;

    return ((n >> (8 - n)) | (n << n)) & 0xff;
}

static inline void *RegScramF2(void)
{
    char buf[4];

    return buf;
}

static inline char RegScramD2(char *p)
{
    unsigned char c = *p;

    TimerGetSeconds();
    return c;
}

static inline char RegScramE2(void)
{
    return 0;
}
#endif

#endif
