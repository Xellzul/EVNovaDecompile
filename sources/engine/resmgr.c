// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* resmgr.c -- the Mac Resource Manager shims the win port implements over BurgerLib's REZ container (win 0x004cdfa0..0x004ce480). Notes: Research/notes/resmgr.md */
#include <stddef.h>

#include "macport.h"

#include "gametypes.h"
#include "gameutil.h"
#include "burger.h"
#include "burgerplat.h"

struct ListRec *gRezFiles;
long gResourcesReleased;

/* one (type -> contiguous record run) entry per resource type in a rez file */

/* open rez files hang off gRezFiles, BurgerLib's list; each node's data is the RezFile */

/* every tenth release purges the container's cache */
#define kPurgeEvery 10

/*
 * The named `rez` local, and its position, are the whole function: without it
 * the container pointer takes a scratch register and mwcc gets by on three
 * callee-saved registers where the exe pushes four.
 */

/* nv_FindResourceRecord (mac (port layer)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cdfa0
ResRecord *nv_FindResourceRecord(unsigned long type, short id)
{
    ListNode *node;
    ResRecord *rec;
    long remaining;
    RezFile *rez;
    long n;
    RezTypeEntry *entry;

    if (gRezFiles == NULL)
        return NULL;

    for (node = gRezFiles->head; node != NULL; node = node->next) {
        rez = (RezFile *)node->data;
        remaining = rez->typeCount;
        entry = rez->types;
        while (remaining != 0) {
            if (entry->type == type) {
                n = entry->count;
                rec = entry->first;
                while (n != 0) {
                    if (rec->id == id)
                        return rec;
                    rec++;
                    n--;
                }
                break;
            }
            entry++;
            remaining--;
        }
    }
    return NULL;
}


/* index runs across the open files, so a file that has none of the type
   passes the remainder down to the next one */
/* nv_FindIndResourceRecord. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce030
ResRecord *nv_FindIndResourceRecord(unsigned long type, short index)
{
    ListNode *node;
    long remaining;
    RezTypeEntry *entry;

    if (gRezFiles == NULL)
        return NULL;

    for (node = gRezFiles->head; node != NULL; node = node->next) {
        remaining = ((RezFile *)node->data)->typeCount;
        entry = ((RezFile *)node->data)->types;
        while (remaining != 0) {
            if (entry->type == type) {
                if (index < entry->count)
                    return entry->first + index;
                index -= entry->count;
                break;
            }
            entry++;
            remaining--;
        }
    }
    return NULL;
}

/* nv_CountResourceRecords. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce0b0
long nv_CountResourceRecords(unsigned long type)
{
    ListNode *node;
    long remaining;
    RezTypeEntry *entry;
    long total;

    total = 0;
    if (gRezFiles == NULL)
        return 0;

    for (node = gRezFiles->head; node != NULL; node = node->next) {
        remaining = ((RezFile *)node->data)->typeCount;
        entry = ((RezFile *)node->data)->types;
        while (remaining != 0) {
            if (entry->type == type) {
                total += entry->count;
                break;
            }
            entry++;
            remaining--;
        }
    }
    return total;
}

/* nv_FindNamedResourceRecord. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce110
ResRecord *nv_FindNamedResourceRecord(unsigned long type, const char *name)
{
    ListNode *node;
    long remaining;
    RezTypeEntry *entry;
    long count;
    ResRecord *rec;

    if (gRezFiles == NULL)
        return NULL;

    for (node = gRezFiles->head; node != NULL; node = node->next) {
        remaining = ((RezFile *)node->data)->typeCount;
        entry = ((RezFile *)node->data)->types;
        while (remaining != 0) {
            if (entry->type == type) {
                count = entry->count;
                rec = entry->first;
                while (count != 0) {
                    if (strcmp(name, rec->name) == 0)
                        return rec;
                    rec++;
                    count--;
                }
                break;
            }
            entry++;
            remaining--;
        }
    }
    return NULL;
}

/* nv_FindResourceRecordForHandle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce1a0
ResRecord *nv_FindResourceRecordForHandle(Handle h)
{
    ListNode *node;
    RezFile *rez;
    long index;
    long remaining;
    RezTypeEntry *entry;
    long count;
    ResRecord *rec;
    int idx;

    if (gRezFiles == NULL)
        return NULL;

    for (node = gRezFiles->head; node != NULL; node = node->next) {
        rez = (RezFile *)node->data;
        if (ResourceGetIDFromHandle(rez, h, &index, NULL, 0) == 0) {
            remaining = rez->typeCount;
            entry = rez->types;
            while (remaining != 0) {
                count = entry->count;
                rec = entry->first;
                idx = index;
                while (count != 0) {
                    if (rec->index == idx)
                        return rec;
                    rec++;
                    count--;
                }
                entry++;
                remaining--;
            }
            return NULL;
        }
    }
    return NULL;
}

/* GetResource (mac (Toolbox shim)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce250
Handle GetResource(unsigned long type, short id)
{
    ResRecord *rec;
    Handle h;

    rec = nv_FindResourceRecord(type, id);
    if (rec != NULL) {
        h = ResourceLoadHandle(rec->rez, rec->index);
        if (h != NULL && ResourceJustLoaded != 0)
            FixupResourceByType(h, type);
        return h;
    }
    return NULL;
}

/* GetIndResource. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce2a0
Handle GetIndResource(unsigned long type, short index)
{
    ResRecord *rec;
    Handle h;

    if (index <= 0)
        return NULL;

    rec = nv_FindIndResourceRecord(type, (index - 1));
    if (rec != NULL) {
        h = ResourceLoadHandle(rec->rez, rec->index);
        if (h != NULL && ResourceJustLoaded != 0)
            FixupResourceByType(h, type);
        return h;
    }
    return NULL;
}

/* GetNamedResource. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce300
Handle GetNamedResource(unsigned long type, const unsigned char *name)
{
    ResRecord *rec;
    Handle h;
    char cname[256];

    PStr2CStr(cname, name);
    rec = nv_FindNamedResourceRecord(type, cname);
    if (rec != NULL) {
        h = ResourceLoadHandle(rec->rez, rec->index);
        if (h != NULL && ResourceJustLoaded != 0)
            FixupResourceByType(h, type);
        return h;
    }
    return NULL;
}

/* ReleaseResource (mac (Toolbox shim)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce370
void ReleaseResource(Handle h)
{
    ListNode *node;
    RezFile *rez;
    long index;

    if (gRezFiles == NULL)
        return;

    for (node = gRezFiles->head; node != NULL; node = node->next) {
        rez = (RezFile *)node->data;
        if (ResourceGetIDFromHandle(rez, h, &index, NULL, 0) == 0) {
            ResourceRelease(rez, index);
            gResourcesReleased++;
            if (gResourcesReleased % kPurgeEvery == 0)
                ResourcePurgeCache(rez);
            return;
        }
    }
}

/* DetachResource. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce400
void DetachResource(Handle h)
{
    ListNode *node;
    RezFile *rez;
    long index;

    if (gRezFiles == NULL)
        return;

    for (node = gRezFiles->head; node != NULL; node = node->next) {
        rez = (RezFile *)node->data;
        if (ResourceGetIDFromHandle(rez, h, &index, NULL, 0) == 0) {
            ResourceDetach(rez, index);
            return;
        }
    }
}

/* CountResources (mac (Toolbox shim)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce470
long CountResources(unsigned long type)
{
    return nv_CountResourceRecords(type);
}

/* GetResInfo (mac (Toolbox shim)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce480
void GetResInfo(Handle h, short *id, unsigned long *type, unsigned char *name)
{
    ResRecord *rec;

    rec = nv_FindResourceRecordForHandle(h);
    if (rec != NULL) {
        if (id != NULL)
            *id = rec->id;
        if (type != NULL)
            *type = rec->type;
        if (name != NULL)
            CStr2PStr((char *)name, rec->name);
        return;
    }
}
