/* burgerrez.h -- the records of BurgerLib's expanded .rez table (RzResourceManager), shared by
 * burgerrezopen.c. Types only, so including it exposes no TU to a prototype.
 * Notes: Research/notes/burgerrezopen.md
 */
#ifndef BURGERREZ_H
#define BURGERREZ_H

#include "burgertypes.h"

/* The expanded table: each group, then its entries. */
typedef struct RezGroup {
    ULong RezNum;
    ULong count;
} RezGroup;

typedef struct RezEntry {
    void **MemHand;
    ULong FileOffset;
    ULong length;
    ULong nameFlags;                             /* name offset | flags | load count */
    ULong fileLength;
} RezEntry;

typedef struct RezName {
    ULong HashOffset;
    ULong RezNum;
} RezName;

#endif
