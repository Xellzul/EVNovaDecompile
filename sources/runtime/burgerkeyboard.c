// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgerkeyboard.c -- BurgerLib InKeyboard: the polling-proc list, the key-down table, the queue readers. Notes: Research/notes/burgerkeyboard.md */

#include "burgertypes.h"
#include "burger.h"
#include "macport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One entry of the list KeyboardKbhit runs on every poll; the list is of handles. */
typedef struct PollProcs_t {
    struct PollProcs_t **Next;                   /* +0x00 */
    void (*Proc)(void *);                        /* +0x04 */
    void *Data;                                  /* +0x08 */
} PollProcs_t;

unsigned char KeyArray[128];                     /* 0x0086e098 */

static Word PollingRecurse;                      /* 0x00867480 */
static PollProcs_t **MasterKbHitProcList;        /* 0x00867484 */

#define kKeyCodeLimit 0x80
#define kKeyDownBit   1

/* KeyboardCallPollingProcs (BurgerLib InKeyboard). STATE: MATCH 100.0%, exe 23 / obj 23,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1800
void KeyboardCallPollingProcs(void)
{
    PollProcs_t **handle;
    PollProcs_t **next;
    PollProcs_t *entry;

    if (PollingRecurse == 0) {
        PollingRecurse = 1;
        handle = MasterKbHitProcList;
        if (handle != 0) {
            do {
                entry = *handle;
                next = entry->Next;
                entry->Proc(entry->Data);
                handle = next;
            } while (handle != 0);
        }
        PollingRecurse = 0;
    }
}

/* KeyboardAddRoutine (BurgerLib InKeyboard). STATE: MATCH 100.0%, exe 19 / obj 19,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1850
void KeyboardAddRoutine(void (*proc)(void *), void *data)
{
    PollProcs_t **handle;
    PollProcs_t *entry;

    handle = (PollProcs_t **)AllocAHandle2(sizeof(PollProcs_t), 0);
    if (handle != 0) {
        entry = *handle;
        entry->Proc = proc;
        entry->Data = data;
        entry->Next = MasterKbHitProcList;
        MasterKbHitProcList = handle;
    }
}

/* KeyboardRemoveRoutine (BurgerLib InKeyboard). STATE: MATCH 100.0%, exe 36 / obj 36,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f18a0
void KeyboardRemoveRoutine(void (*proc)(void *), void *data)
{
    PollProcs_t **handle;
    PollProcs_t **prev;
    PollProcs_t *entry;

    handle = MasterKbHitProcList;
    if (handle != 0) {
        prev = 0;
        do {
            entry = *handle;
            if (entry->Proc == proc && entry->Data == data) {
                if (prev != 0)
                    (*prev)->Next = entry->Next;
                else
                    MasterKbHitProcList = entry->Next;
                DeallocAHandle(handle);
                break;
            }
            prev = handle;
            handle = entry->Next;
        } while (handle != 0);
    }
}

/* KeyIsDown (BurgerLib KeyboardIsPressed). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004f1900
unsigned int KeyIsDown(unsigned int scanCode)
{
    if (scanCode >= kKeyCodeLimit)
        return 0;
    return KeyArray[scanCode] & kKeyDownBit;
}

/* KeyboardGet (BurgerLib InKeyboard). STATE: MATCH 100.0%, exe 7 / obj 7, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1920
int KeyboardGet(void)
{
    if (KeyboardKbhit() == 0)
        return 0;
    return KeyboardGetch();
}

/* Drain whatever is queued, then forget every key that was down. */
/* KeyboardFlush (BurgerLib KeyboardFlush). STATE: MATCH 100.0%, exe 15 / obj 15,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1940
void KeyboardFlush(void)
{
    while (KeyboardKbhit() != 0)
        KeyboardGetch();
    FastMemSet(KeyArray, 0, kKeyTableSize);
    MouseClicked = 0;
}

#ifdef __cplusplus
}
#endif
