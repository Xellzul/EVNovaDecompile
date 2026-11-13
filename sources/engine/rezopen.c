// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* rezopen.c -- open a BurgerLib REZ container, parse its "resource.map" entry into resmgr.c's RezFile/RezTypeEntry/ResRecord layout, and link it into gRezFiles (win 0x004ce4d0). Notes: Research/notes/rezopen.md */
/* This file adds the raw, still-big-endian counterparts the map's bytes are
   read as before SwapEndianShort/Long convert them in place; every struct and
   the two named globals here are resmgr.c's own. */
#include <stddef.h>

#include "macport.h"
#include "gametypes.h"
#include "gameutil.h"

/* resmgr.c's RezFile family, stride-exact (0x2c/0xc/0x110) */

/* the raw, still-big-endian "resource.map" entry bytes */
typedef struct RawTypeEntry {
    uint32_t type;     /* +0x00 */
    uint32_t offset;    /* +0x04 from the map's own base */
    uint32_t count;      /* +0x08 */
} RawTypeEntry;

#pragma options align=mac68k
typedef struct RawResRecord {
    uint32_t index;    /* +0x00 */
    uint32_t type;      /* +0x04 */
    unsigned short id;        /* +0x08 */
    char name[256];             /* +0x0a */
} RawResRecord;
#pragma options align=reset

typedef struct RawRezMap {
    uint32_t typeListOffset; /* +0x00 from this record's own base */
    uint32_t typeCount;       /* +0x04 */
} RawRezMap;

/* endianswap.c's helpers by value: the exe swaps the register it has just tested */
static __inline short SwapEndianShort(unsigned short t)
{
#ifndef MACBUILD
    asm { ror t, 8 }
#endif
    return t;
}

static __inline int32_t SwapEndianLong(uint32_t t)
{
#ifndef MACBUILD
    asm { bswap t }
#endif
    return t;
}


/* EVOpenResourceFile (mac _EVOpenResourceFile). STATE: MATCH 100.0%, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce4d0
short EVOpenResourceFile(void *path)
{
    RezFile *rez;
    void *mapData;
    uint32_t entryCount;
    RawTypeEntry *rawType;
    RezTypeEntry *type;
    RawResRecord *rawRecord;
    ResRecord *record;
    uint32_t typeCount;
    RawRezMap *map;

    if (gRezFiles == NULL)
        gRezFiles = LinkedListNew();

    rez = AllocAPointer(sizeof(RezFile));
    if (ResourceInit(rez, path, 0) != 0)
        goto fail;

    mapData = ResourceLoadByName(rez, "resource.map");
    if (mapData == NULL)
        goto closeFail;

    map = mapData;
    if (map->typeCount != 0) {
        rawType = (RawTypeEntry *)((char *)mapData + SwapEndianLong(map->typeListOffset));
        typeCount = SwapEndianLong(map->typeCount);
        rez->typeCount = typeCount;
        rez->types = AllocAPointer(rez->typeCount * sizeof(RezTypeEntry));

        type = rez->types;
        do {
            entryCount = rawType->count;
            if (entryCount != 0) {
                rawRecord = (RawResRecord *)((char *)mapData + SwapEndianLong(rawType->offset));
                entryCount = SwapEndianLong(rawType->count);
                type->type = SwapEndianLong(rawType->type);
                type->count = entryCount;
                type->first = AllocAPointer(type->count * sizeof(ResRecord));

                record = type->first;
                do {
                    record->rez = rez;
                    record->index = SwapEndianLong(rawRecord->index);
                    record->type = SwapEndianLong(rawRecord->type);
                    record->id = SwapEndianShort(rawRecord->id);
                    strcpy(record->name, rawRecord->name);
                    rawRecord++;
                    record++;
                } while (--entryCount != 0);
            }
            rawType++;
            type++;
        } while (--typeCount != 0);
    }

    LinkedListAddNewEntryBegin(gRezFiles, rez);
    return 0;

closeFail:
    ResourceDestroy(rez);
fail:
    DeallocAPointer(rez);
    return -1;
}
