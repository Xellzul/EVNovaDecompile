// COMPILE: -opt level=4,schedule,peephole -enum min
/* itpagemac.c -- 1.0.10: the page list, an object of its own on the Mac between the toolkit and its item list. Notes: Research/notes/maclibs.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "ittoolkit.h"

#define kPageBuckets        31

ULong IT_HashName(const unsigned char *s)
{
    ULong hash;

    hash = 0;
    for (; *s != 0; s++)
        hash = hash << 2 ^ hash >> 29 ^ *s;
    return hash;
}

static const unsigned char DAT_00581bf4[4] = { 0 };   /* 0x00581bf4, unreferenced */

ITPageTable *_IT_PageListCreate(ITElement *root)
{
    long err;
    ITPage **link;
    ITElement *e;
    ITPageTable *table;
    ITPage *page;
    char *value;

    err = 0;
    value = 0;
    e = 0;
    table = 0;
    page = 0;
    table = MemoryAllocateClear(sizeof(ITPageTable));
    if (table == 0) {
        err = kOutOfMemory;
        goto done;
    }
    table->root = root;
    table->buckets = kPageBuckets;
    while ((e = _IT_ParseTreeFindNextTag(root, e, "PAGE", 1)) != 0) {
        page = MemoryAllocateClear(sizeof(ITPage));
        if (page == 0) {
            err = kOutOfMemory;
            goto done;
        }
        page->element = e;
        table->count++;
        value = _IT_SimpleFindAttribute(page->element, "NAME");
#if TARGET_API_MAC_CARBON
        if (value != 0) {
            if (page->element->wide)
                U2CStringCopySafe((unsigned char *)page->name, (const unsigned short *)value, sizeof(page->name), (ULong)-1);
            else
                StringCopySafe(page->name, value, sizeof(page->name));
        }
        value = _IT_SimpleFindAttribute(page->element, "TITLE");
        if (value != 0) {
            if (page->element->wide)
                UnicodeCopySafe((unsigned short *)page->title, (const unsigned short *)value, sizeof(page->title), (ULong)-1);
            else
                C2UStringCopySafe((unsigned short *)page->title, (const unsigned char *)value, sizeof(page->title), (ULong)-1);
        }
#else
        if (value != 0)
            StringCopySafe(page->name, value, sizeof(page->name));
        value = _IT_SimpleFindAttribute(page->element, "TITLE");
        if (value != 0)
            StringCopySafe(page->title, value, sizeof(page->title));
#endif
        for (link = &table->first; *link != 0; link = &(*link)->next)
            ;
        *link = page;
        for (link = &table->bucket[IT_HashName((unsigned char *)page->name) % table->buckets];
             *link != 0; link = &(*link)->link)
            ;
        *link = page;
    }
done:
    if (err != 0 && table != 0) {
        _IT_PageListDispose(table);
        table = 0;
    }
    return table;
}

void _IT_PageListDispose(void *tableRef)
{
    ITPage *e;
    ITPage *next;
    ITPageTable *table = (ITPageTable *)tableRef;

    for (e = table->first; e != 0; e = next) {
        next = e->next;
        MemoryDeallocate(e);
    }
    MemoryDeallocate(table);
}

ITPage *_IT_GetPageByName(ITPageTable *table, const char *name)
{
    ITPage *e;
    long err;

    err = 0;
    e = 0;
    for (e = table->bucket[IT_HashName((const unsigned char *)name) % table->buckets]; e != 0;
         e = e->link) {
        if (StringCompare(name, e->name) == 0)
            break;
    }
    return e;
}

ITPage *_IT_GetPageByIndex(ITPageTable *table, ULong index)
{
    ITPage *e;
    long err;

    err = 0;
    e = 0;
    for (e = table->first; e != 0 && index--; e = e->next)
        ;
    return e;
}
