/* ititemTU.c -- 1.0.10: the toolkit's item list and the interface tool's item calls are one object; the exe calls _IT_ItemCallback bare from the second. Notes: Research/notes/maclibs.md */
#include "../../runtime/ittoolkit3.c"
#include "../../runtime/interfacetool.c"

#if TARGET_API_MAC_CARBON
static unsigned short kEnglishW[] = { 'E', 'n', 'g', 'l', 'i', 's', 'h', 0 };

extern int32_t IT_IndexCount(ITElement *index, const char *language);
extern int32_t IT_IndexFind(ITElement *index, const char *language, ULong n, char *a, char *b, char *c, unsigned short *d);
extern int32_t IT_IndexLookup(ITElement *index, const char *language, const char *path, char *a, char *b, char *c, unsigned short *d, int32_t *f);

ULong IT_ItemCallbackLanguageCount(void)
{
    WindowNode *w;
    ULong n;

    w = gIT_CurrentView;
    n = 1;
    if (w->tracking != 0)
        n = IT_IndexCount((ITElement *)w->tracking, w->path);
    return n;
}

int32_t IT_ItemCallbackLanguageCurrent(char *code, unsigned short *name)
{
    WindowNode *w;
    int32_t index;

    w = gIT_CurrentView;
    index = 0;
    if (w->tracking != 0) {
        IT_IndexLookup((ITElement *)w->tracking, w->path, w->anchor, 0, code, 0, name, &index);
    } else {
        if (code != 0)
            *code = 0;
        if (name != 0)
            UnicodeCopySafe(name, kEnglishW, 0x200, -1);
    }
    return index;
}

unsigned char IT_ItemCallbackLanguageCode(ULong index, char *code)
{
    WindowNode *w;
    unsigned char found;

    found = 0;
    *code = 0;
    w = gIT_CurrentView;
    if (w->tracking != 0) {
        if (IT_IndexFind((ITElement *)w->tracking, w->path, index, 0, code, 0, 0) == 0)
            found = 1;
    } else if (index == 0) {
        found = 1;
    }
    return found;
}

unsigned char IT_ItemCallbackLanguageName(ULong index, unsigned short *name)
{
    WindowNode *w;
    unsigned char found;

    found = 0;
    *name = 0;
    w = gIT_CurrentView;
    if (w->tracking != 0) {
        if (IT_IndexFind((ITElement *)w->tracking, w->path, index, 0, 0, 0, name) == 0)
            found = 1;
    } else if (index == 0) {
        UnicodeCopySafe(name, kEnglishW, 0x200, -1);
        found = 1;
    }
    return found;
}
#endif
