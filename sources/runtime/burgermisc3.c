// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgermisc3.c -- burgermisc, exe run 3. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LinkedListInit. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ff830
void LinkedListInit(ListRec *list)
{
    list->head = 0;
    list->tail = 0;
    list->count = 0;
}

/* LinkedListNew. STATE: MATCH 100%, exe 5 / obj 5 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ff850
void *LinkedListNew(void)
{
    return AllocAPointerClear(sizeof(ListRec));
}

/* LinkedListGetEntryData. STATE: MATCH 100%, exe 13 / obj 13 instrs. OPEN: none */
// FUNCTION: EVNOVA 0x004ff870
void *LinkedListGetEntryData(ListRec *list, ULong index)
{
    ListNode *p;

    if (list->count <= index)
        return 0;
    p = list->head;
    if (index != 0) {
        do {
            p = p->next;
        } while (--index != 0);
    }
    return p;
}

#ifdef __cplusplus
}
#endif

#define kListNodeSize   0x10

/* LinkedListAddNewEntryEnd. STATE: MATCH 100%, exe 24 / obj 24 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ff8a0
void LinkedListAddNewEntryEnd(ListRec *list, void *data)
{
    ListNode *node;
    ListNode *tail;

    node = (ListNode *)AllocAPointer(kListNodeSize);
    if (node != 0) {
        node->data = data;
        node->KillProc = 0;
        tail = list->tail;
        node->next = 0;
        node->prev = tail;
        if (tail != 0)
            tail->next = node;
        else
            list->head = node;
        list->tail = node;
        list->count = list->count + 1;
    }
}

/* LinkedListAddNewEntryBegin. STATE: MATCH 100%, exe 24 / obj 24 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ff900
void LinkedListAddNewEntryBegin(ListRec *list, void *data)
{
    ListNode *node;
    ListNode *head;

    node = (ListNode *)AllocAPointer(kListNodeSize);
    if (node != 0) {
        node->data = data;
        node->KillProc = 0;
        head = list->head;
        node->next = head;
        node->prev = 0;
        if (head != 0)
            head->prev = node;
        else
            list->tail = node;
        list->head = node;
        list->count = list->count + 1;
    }
}
