// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* fontcache.c -- the port's font family/size cache (win 0x004bc600-0x004bc75d). Notes: Research/notes/fontcache.md */
#include <stddef.h>

#include "macport.h"
#include "dialogs.h"

#include "burgerplat.h"
#include "burger.h"
#include "win32.h"

/* one realised GDI font */
typedef struct FontEntry {
    short size;                 /* +0x00 */
    short style;                /* +0x02 */
    void *hfont;                /* +0x04 */
} FontEntry;

typedef struct FontFamily {
    char name[0x100];           /* +0x000 */
    ListRec fonts;              /* +0x100 */
} FontFamily;

short gChicagoFont;
short gTimesFont;
ListRec gFontFamilies;
void *gMacRomanToCP1252;

/* the Mac-to-Windows substitution table the port ships */
typedef struct FontMap {
    const char *macName;        /* +0x00 */
    const char *faceName;       /* +0x04 */
    int charSet;                /* +0x08 */
    int boldWeight;             /* +0x0c */
    int normalWeight;           /* +0x10 */
    int pitchAndFamily;         /* +0x14 */
    char useDeviceCaps;         /* +0x18 */
} FontMap;

#define kFontMapCount 7
#define OUT_TT_PRECIS 5

#define iconv_open libiconv_open                          /* as GNU iconv.h does */

static FontEntry *MakeFontEntry(FontFamily *family, int size, int style);

static FontFamily *AddFontFamily(const char *name);

/* InitFonts. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc3e0
void InitFonts(void)
{
    short scratch;

    LinkedListInit(&gFontFamilies);
    GetFNum("\pChicago", &gChicagoFont);
    GetFNum("\pTimes", &gTimesFont);
    GetFNum("\pNewYork", &scratch);
    GetFNum("\pGeneva", &scratch);
    gMacRomanToCP1252 = iconv_open("CP1252", "MacRoman");
}

FontMap gFontMap[kFontMapCount] = {                      /* 0x00575d80 (name invented) */
    { "Times",     "Times New Roman", 0, FW_BOLD, FW_NORMAL, 0, 0 },
    { "Helvetica", "Arial",           0, FW_BOLD, FW_NORMAL, 0, 0 },
    { "Geneva",    "Arial",           0, FW_BOLD, FW_NORMAL, 0, 0 },
    { "Courier",   "Courier New",     0, FW_BOLD, FW_NORMAL, 0, 0 },
    { "Chicago",   "Arial",           0, FW_BOLD, FW_NORMAL, 0, 0 },
    { "Charcoal",  "Arial",           0, FW_BOLD, FW_NORMAL, 0, 0 },
    { "New York",  "Georgia",         0, FW_BOLD, FW_NORMAL, 0, 0 }
};

/* MakeFontEntry. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc450
static FontEntry *MakeFontEntry(FontFamily *family, int size, int style)
{
    const char *faceName;
    int charSet;
    char useDeviceCaps;
    int pitchAndFamily;
    FontEntry *entry;
    int i;
    int weight;
    int height;
    int italic;
    void *hdc;

    useDeviceCaps = 0;
    for (i = 0; i < kFontMapCount; i++) {
        if (stricmp(family->name, gFontMap[i].macName) == 0) {
            if ((style & kFontBold) != 0)
                weight = gFontMap[i].boldWeight;
            else
                weight = gFontMap[i].normalWeight;
            charSet = gFontMap[i].charSet;
            pitchAndFamily = gFontMap[i].pitchAndFamily;
            faceName = gFontMap[i].faceName;
            useDeviceCaps = gFontMap[i].useDeviceCaps;
            break;
        }
    }
    if (i == kFontMapCount) {
        if ((style & kFontBold) != 0)
            weight = FW_BOLD;
        else
            weight = FW_NORMAL;
        charSet = 0;
        pitchAndFamily = 0;
        faceName = family->name;
    }
    if (useDeviceCaps != 0) {
        hdc = GetDC(Win95MainWindow);
        height = -MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(Win95MainWindow, hdc);
    } else {
        height = -size;
    }
    entry = (FontEntry *)AllocAPointerClear(sizeof(FontEntry));
    entry->size = size;
    entry->style = style;
    if ((style & kFontItalic) != 0)
        italic = 1;
    else
        italic = 0;
    entry->hfont = CreateFontA(height, 0, 0, 0, weight, italic, 0, 0, charSet,
                               OUT_TT_PRECIS, 0, 0, pitchAndFamily, faceName);
    LinkedListAddNewEntryEnd(&family->fonts, entry);
    return entry;
}

/* AddFontFamily. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc5c0
static FontFamily *AddFontFamily(const char *name)
{
    FontFamily *family;

    family = (FontFamily *)AllocAPointer(sizeof(FontFamily));
    strcpy(family->name, name);
    LinkedListInit(&family->fonts);
    LinkedListAddNewEntryEnd(&gFontFamilies, family);
    return family;
}

/* FindFontEntry. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc600
FontEntry *FindFontEntry(FontFamily *family, short size, short style)
{
    ListNode *node;
    FontEntry *entry;

    node = family->fonts.head;
    while (node != NULL) {
        entry = (FontEntry *)node->data;
        if (entry->size == size && entry->style == style)
            return entry;
        node = node->next;
    }
    return NULL;
}

/* GetFontFamily. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc640
FontFamily *GetFontFamily(short familyID)
{
    ListNode *node;
    FontFamily *family;

    node = (ListNode *)LinkedListGetEntryData(&gFontFamilies, familyID);
    if (node == NULL)
        family = NULL;
    else
        family = (FontFamily *)node->data;
    return family;
}

/* GetFont. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc670
void *GetFont(short familyID, short size, short style)
{
    FontFamily *family;
    FontEntry *entry;

    family = GetFontFamily(familyID);
    if (family == NULL)
        return NULL;
    entry = FindFontEntry(family, size, style);
    if (entry == NULL)
        entry = MakeFontEntry(family, size, style);
    if (entry == NULL)
        return NULL;
    return entry->hfont;
}

/* GetFNum. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc6e0
void GetFNum(const unsigned char *name, short *familyID)
{
    ListNode *node;
    int index;
    char cname[256];

    index = 0;
    PStr2CStr(cname, name);
    node = gFontFamilies.head;
    while (node != NULL) {
        if (strcmp((char *)node->data, cname) == 0) {
            *familyID = index;
            return;
        }
        node = node->next;
        index = index + 1;
    }
    if (AddFontFamily(cname) != NULL)
        *familyID = (gFontFamilies.count - 1);
}
