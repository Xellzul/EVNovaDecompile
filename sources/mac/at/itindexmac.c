/* itindexmac.c -- 1.0.10 Carbon: index.iml, a window's table of interface files by language. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "ittoolkit.h"

#if TARGET_API_MAC_CARBON

#define IsTag(e) ((e)->tag == 1 || (e)->tag == 2)
#define kInfoFolder 2
#define kInfoAlias 4
#define kChunkMax 0x32000
#define kFileNotFound -7203

extern int32_t FT_DirGetEntry(int32_t folder, ULong index, FTFileInfo *info, unsigned char flags);
extern int32_t IndirectNormalizeUnicode(void **handle, const char *charset);
#define IndirectNormalizeEncoding IndirectNormalizeUnicode

extern ITElement *PT_ParseTreeGetNext(ITElement *root, ITElement *element);
extern ITElement *PT_ParseTreeGetParent(ITElement *root, ITElement *element);

#define kPopupW L"POPUP"
#define kLanguageW L"LANGUAGE"
#define kInterfaceW L"INTERFACE"
#define kStringTableW L"STRINGTABLE"
#define kNameW L"NAME"
#define kIsoW L"ISO_639_1"
#define kHrefW L"HREF"
#define kSrcW L"SRC"
#define kSourceW L"SOURCE"
#define kTitleW L"TITLE"
#define kIndexFile "index.iml"
#define kFourSpacesW L"    "
#define kCarriageW L"\r"
#define kSpaceW L" "
#define kEmptyW L""
#define kPW L"P"
#define kNewlineW L"\n"
#define kBlankLineW L"\n\n"
#define kBrW L"BR"
#define kLiW L"LI"
#define kBulletW L"\n\xa5 "
#define kDtW L"DT"
#define kDdW L"DD"
#define kStringW L"STRING"
#define kNoNameA ""
#define kStringTableA "STRINGTABLE"
#define kStringA "STRING"
#define kNameA "NAME"

extern unsigned short *C2UStringAppendSafe(unsigned short *dst, const char *src, ULong size, ULong count);
extern unsigned short *UnicodeAppendSafe(unsigned short *dst, const unsigned short *src, ULong size, ULong count);
extern unsigned char HashRemoveUnicode(struct HashTable *t, const unsigned short *key);
extern int32_t HashAppendUnicode(struct HashTable *t, const unsigned short *key, const unsigned short *value);


unsigned char IT_IndexIsPlain(ITElement *root)
{
    ITElement *e;
    const unsigned short *popup = kPopupW;
    const unsigned short *language = kLanguageW;

    for (e = root; e != 0; e = PT_ParseTreeGetNext(root, e)) {
        if (IsTag(e)) {
            if (UnicodeCompare((unsigned short *)e->name, popup) != 0) {
                if (UnicodeCompare((unsigned short *)e->name, language) == 0)
                    break;
                if (HashLookupUnicode(e->attributes, language, 0) != 0)
                    break;
            }
        }
    }
    return AsByte(e == 0);
}

void IT_IndexUpperNames(ITElement *root)
{
    ITElement *e;
    unsigned short *v;

    for (e = root; e != 0; e = PT_ParseTreeGetNext(root, e)) {
        if (IsTag(e) && (UnicodeCompare((unsigned short *)e->name, kLanguageW) == 0 ||
                         UnicodeCompare((unsigned short *)e->name, kInterfaceW) == 0 ||
                         UnicodeCompare((unsigned short *)e->name, kStringTableW) == 0)) {
            if ((v = (unsigned short *)HashLookupUnicode(e->attributes, kNameW, 0)) != 0)
                Unicode2UpperCopySafe(v, v, -1, -1);
            if ((v = (unsigned short *)HashLookupUnicode(e->attributes, kLanguageW, 0)) != 0)
                Unicode2UpperCopySafe(v, v, -1, -1);
            if ((v = (unsigned short *)HashLookupUnicode(e->attributes, kIsoW, 0)) != 0)
                Unicode2UpperCopySafe(v, v, -1, -1);
        }
    }
}

int32_t IT_IndexMakeLinks(ITElement *root, const char *prefix)
{
    ITElement *e;
    int32_t err;
    int32_t r;
    unsigned short *attr;
    unsigned short *v;
    unsigned short buffer[256];

    err = 0;
    for (e = root; e != 0; e = PT_ParseTreeGetNext(root, e)) {
        if (!IsTag(e))
            continue;
        attr = kHrefW;
        if ((v = (unsigned short *)HashLookupUnicode(e->attributes, attr, 0)) != 0)
            goto have;
        attr = kSrcW;
        if ((v = (unsigned short *)HashLookupUnicode(e->attributes, attr, 0)) != 0)
            goto have;
        attr = kSourceW;
        if ((v = (unsigned short *)HashLookupUnicode(e->attributes, attr, 0)) != 0) {
have:
            if (*prefix != 0) {
                C2UStringCopySafe(buffer, (const unsigned char *)prefix, sizeof(buffer), (ULong)-1);
                C2UStringAppendSafe(buffer, "/", sizeof(buffer), -1);
            } else {
                buffer[0] = 0;
            }
            if (UnicodePoint2Char8(*v) == '#')
                C2UStringAppendSafe(buffer, kIndexFile, sizeof(buffer), -1);
            UnicodeAppendSafe(buffer, v, sizeof(buffer), -1);
            while (HashRemoveUnicode(e->attributes, attr))
                ;
            r = HashAppendUnicode(e->attributes, attr, buffer);
            if (r != 0) {
                err = r;
                break;
            }
        } else if (UnicodeCompare((unsigned short *)e->name, kInterfaceW) == 0 ||
                   UnicodeCompare((unsigned short *)e->name, kStringTableW) == 0) {
            if (*prefix != 0) {
                C2UStringCopySafe(buffer, (const unsigned char *)prefix, sizeof(buffer), (ULong)-1);
                C2UStringAppendSafe(buffer, "/", sizeof(buffer), -1);
            } else {
                buffer[0] = 0;
            }
            C2UStringAppendSafe(buffer, kIndexFile, sizeof(buffer), -1);
            v = (unsigned short *)HashLookupUnicode(e->attributes, kNameW, 0);
            if (v != 0) {
                C2UStringAppendSafe(buffer, "#", sizeof(buffer), -1);
                UnicodeAppendSafe(buffer, v, sizeof(buffer), -1);
            }
            r = HashAppendUnicode(e->attributes, kHrefW, buffer);
            if (r != 0) {
                err = r;
                break;
            }
        }
    }
    return err;
}

ITElement *IT_IndexGetLanguage(ITElement *root, ITElement *e)
{
    if (HashLookupUnicode(e->attributes, kLanguageW, 0) == 0) {
        do {
            if (e->tag > 0 && !AsByte(e->tag > 2 && e->tag < 0x7f) && e->name != 0 &&
                UnicodeCompare((unsigned short *)e->name, kLanguageW) == 0 &&
                HashLookupUnicode(e->attributes, kNameW, 0) != 0)
                break;
        } while ((e = PT_ParseTreeGetParent(root, e)) != 0);
    }
    return e;
}

int32_t IT_IndexCountTag(ITElement *root, const char *tag, const char *language)
{
    ITElement *e;
    unsigned char found;
    int32_t count;
    unsigned short *v;
    unsigned short wideTag[256];
    unsigned short wideLanguage[256];

    count = 0;
    C2UStringCopySafe(wideTag, (const unsigned char *)tag, sizeof(wideTag), (ULong)-1);
    C2UStringCopySafe(wideLanguage, (const unsigned char *)language, sizeof(wideLanguage), (ULong)-1);
    Unicode2UpperCopySafe(wideLanguage, wideLanguage, -1, -1);
    for (e = root; e != 0; ) {
        found = 0;
        if (IsTag(e) && UnicodeCompare((unsigned short *)e->name, wideTag) == 0) {
            v = (unsigned short *)HashLookupUnicode(e->attributes, kNameW, 0);
            if (v == 0)
                v = kEmptyW;
            if (wideLanguage[0] == 0 || UnicodeCompare(v, wideLanguage) == 0) {
                found = 1;
                count++;
            }
        }
        e = found ? e->next : PT_ParseTreeGetNext(root, e);
    }
    return count;
}

int32_t IT_IndexFindNth(ITElement *root, const char *tag, const char *language, ULong n, char *a, char *b, char *c, unsigned short *d)
{
    int32_t err;
    ITElement *e;
    unsigned char found;
    unsigned short *v;
    struct HashTable *attrs;
    struct HashTable *other;
    ITElement *parent;
    unsigned short wideTag[256];
    unsigned short wideLanguage[256];

    err = 0;
    if (a != 0)
        *a = 0;
    if (b != 0)
        *b = 0;
    if (c != 0)
        *c = 0;
    if (d != 0)
        *d = 0;
    C2UStringCopySafe(wideTag, (const unsigned char *)tag, sizeof(wideTag), (ULong)-1);
    C2UStringCopySafe(wideLanguage, (const unsigned char *)language, sizeof(wideLanguage), (ULong)-1);
    Unicode2UpperCopySafe(wideLanguage, wideLanguage, -1, -1);
    for (e = root; e != 0; ) {
        found = 0;
        if (IsTag(e) && UnicodeCompare((unsigned short *)e->name, wideTag) == 0) {
            v = (unsigned short *)HashLookupUnicode(e->attributes, kNameW, 0);
            if (v == 0)
                v = kEmptyW;
            if (wideLanguage[0] == 0 || UnicodeCompare(v, wideLanguage) == 0) {
                if (n == 0)
                    break;
                found = 1;
                n--;
            }
        }
        e = found ? e->next : PT_ParseTreeGetNext(root, e);
    }
    if (e != 0) {
        HashGetFlags(e->attributes);
        parent = IT_IndexGetLanguage(root, e);
        attrs = e->attributes;
        other = parent != 0 ? parent->attributes : attrs;
        if (a != 0) {
            v = (unsigned short *)HashLookupUnicode(attrs, kHrefW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(other, kHrefW, 0)) != 0)
                U2CStringCopySafe((unsigned char *)a, v, 0x100, (ULong)-1);
        }
        if (b != 0) {
            v = (unsigned short *)HashLookupUnicode(attrs, kLanguageW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(other, kNameW, 0)) != 0)
                U2CStringCopySafe((unsigned char *)b, v, 0x100, (ULong)-1);
        }
        if (c != 0) {
            v = (unsigned short *)HashLookupUnicode(attrs, kIsoW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(other, kIsoW, 0)) != 0)
                U2CStringCopySafe((unsigned char *)c, v, 0x100, (ULong)-1);
        }
        if (d != 0) {
            v = (unsigned short *)HashLookupUnicode(other, kTitleW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(attrs, kTitleW, 0)) != 0)
                UnicodeCopySafe(d, v, 0x100, -1);
        }
    } else if (n != 0) {
        err = kNotFound;
    } else {
        err = kNotFound;
    }
    return err;
}

int32_t IT_IndexFindPath(ITElement *root, const char *tag, const char *language, const char *path, char *a, char *b, char *c, unsigned short *d, ITElement **result, int32_t *number)
{
    int32_t err;
    int32_t index;
    ITElement *e;
    const char *p;
    const char *comma;
    unsigned short *v;
    struct HashTable *attrs;
    struct HashTable *other;
    ITElement *parent;
    ULong length;
    char one[256];
    unsigned short wideTag[256];
    unsigned short wideLanguage[256];
    unsigned short widePath[256];

    err = 0;
    index = -1;
    if (a != 0)
        *a = 0;
    if (b != 0)
        *b = 0;
    if (c != 0)
        *c = 0;
    if (d != 0)
        *d = 0;
    if (result != 0)
        *result = 0;
    if (number != 0)
        *number = -1;
    for (p = path; p != 0 && *p == ' '; p++)
        ;
    while (p != 0 && (comma = StringFindDelimiter(p, ",")) != 0) {
        StringCopySafe(one, p, (length = comma - p + 1) > 0x100 ? 0x100 : length);
        for (p = comma + 1; *p == ' '; p++)
            ;
        err = IT_IndexFindPath(root, tag, language, one, a, b, c, d, result, number);
        if (err == 0)
            goto done;
    }
    C2UStringCopySafe(wideTag, (const unsigned char *)tag, sizeof(wideTag), (ULong)-1);
    C2UStringCopySafe(wideLanguage, (const unsigned char *)language, sizeof(wideLanguage), (ULong)-1);
    Unicode2UpperCopySafe(wideLanguage, wideLanguage, -1, -1);
    C2UStringCopySafe(widePath, (const unsigned char *)p, sizeof(widePath), (ULong)-1);
    Unicode2UpperCopySafe(widePath, widePath, -1, -1);
    for (e = root; e != 0; parent = PT_ParseTreeGetNext(root, e), e = parent) {    /* 1.0.10 pad: a second copy of the next element, the exe's colours of root, a and b */
        if (!IsTag(e) || UnicodeCompare((unsigned short *)e->name, wideTag) != 0)
            continue;
        index++;
        v = (unsigned short *)HashLookupUnicode(e->attributes, kNameW, 0);
        if (v == 0)
            v = kEmptyW;
        if (wideLanguage[0] != 0 && UnicodeCompare(v, wideLanguage) != 0)
            continue;
        if ((v = (unsigned short *)HashLookupUnicode(e->attributes, kIsoW, 0)) != 0 &&
            (widePath[0] == 0 || UnicodeCompare(v, widePath) == 0))
            break;
        if ((v = (unsigned short *)HashLookupUnicode(e->attributes, kLanguageW, 0)) != 0) {
            if (widePath[0] != 0 && UnicodeCompare(v, widePath) != 0)
                continue;
            break;
        }
        if ((parent = IT_IndexGetLanguage(root, e)) == 0)
            continue;
        if ((v = (unsigned short *)HashLookupUnicode(parent->attributes, kIsoW, 0)) != 0 &&
            (widePath[0] == 0 || UnicodeCompare(v, widePath) == 0))
            break;
        if ((v = (unsigned short *)HashLookupUnicode(parent->attributes, kNameW, 0)) != 0 &&
            (widePath[0] == 0 || UnicodeCompare(v, widePath) == 0))
            break;
    }
    if (e != 0) {
        HashGetFlags(e->attributes);
        parent = IT_IndexGetLanguage(root, e);
        attrs = e->attributes;
        other = parent != 0 ? parent->attributes : attrs;
        if (a != 0) {
            v = (unsigned short *)HashLookupUnicode(attrs, kHrefW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(other, kHrefW, 0)) != 0)
                U2CStringCopySafe((unsigned char *)a, v, 0x100, (ULong)-1);
        }
        if (b != 0) {
            v = (unsigned short *)HashLookupUnicode(attrs, kLanguageW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(other, kNameW, 0)) != 0)
                U2CStringCopySafe((unsigned char *)b, v, 0x100, (ULong)-1);
        }
        if (c != 0) {
            v = (unsigned short *)HashLookupUnicode(attrs, kIsoW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(other, kIsoW, 0)) != 0)
                U2CStringCopySafe((unsigned char *)c, v, 0x100, (ULong)-1);
        }
        if (d != 0) {
            v = (unsigned short *)HashLookupUnicode(other, kTitleW, 0);
            if (v != 0 || (v = (unsigned short *)HashLookupUnicode(attrs, kTitleW, 0)) != 0)
                UnicodeCopySafe(d, v, 0x100, -1);
        }
        if (result != 0)
            *result = e;
        if (number != 0)
            *number = index;
    } else {
        err = kNotFound;
    }
done:
    return err;
}

int32_t IT_IndexLoad(int32_t where, const char *file, ITElement **index, ITElement **root)
{
    int32_t err;
    int32_t e;
    struct FolderSpec **folder;
    ITElement *tree;
    ITElement *sub;
    void **data;
    ULong i;
    FTFileInfo info;
    FTFileInfo inner;

    err = 0;
    folder = 0;
    tree = 0;
    sub = 0;
    data = 0;
    *index = 0;
    if (root != 0)
        *root = 0;
    e = FT_FileGetFlags(where, file, &info, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (!info.flags[kInfoFolder]) {
        e = FT_FileLoad(where, file, kChunkMax, 1, &data);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = IndirectNormalizeEncoding(data, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = PT_ParseTreeLoad(data, "SCRIPT", "LI BR P", 0, &tree);
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectDeallocate(data);
        data = 0;
    } else {
        e = FT_PathCreate((struct FolderSpec **)where, file, &folder);
        if (e != 0) {
            err = e;
            goto done;
        }
        err = FT_FileGetFlags((int32_t)folder, kIndexFile, &inner, 1);
        if (err == 0 && !inner.flags[kInfoFolder]) {
            if (IT_IndexLoad((int32_t)folder, kIndexFile, &tree, root) == 0 && tree != 0)
                IT_IndexMakeLinks(tree, "");
        } else if (!info.flags[kInfoAlias]) {
            for (i = 0; FT_DirGetEntry((int32_t)folder, i, &info, 1) == 0; i++) {
                if (info.flags[kInfoFolder] && IT_IndexLoad(where, info.name, &sub, 0) == 0 && sub != 0) {
                    IT_IndexMakeLinks(sub, info.name);
                    if (tree == 0) {
                        tree = sub;
                        sub = 0;
                    } else {
                        if (PT_ParseTreeInsertNode(tree, tree, sub, 0) == 0) {
                            err = kOutOfMemory;
                            goto done;
                        }
                        sub = 0;
                    }
                }
            }
        }
    }
    if (tree == 0) {
        if (root == 0 || *root == 0) {
            err = kFileNotFound;
            goto done;
        }
    }
    if (tree != 0) {
        if (tree->wide && IT_IndexIsPlain(tree) == 0) {
            IT_IndexUpperNames(tree);
            *index = tree;
            tree = 0;
        } else if (root != 0) {
            *root = tree;
            tree = 0;
        } else {
            err = kNotFound;
        }
    }
done:
    if (folder != 0)
        FT_PathDispose(folder);
    if (tree != 0)
        PT_ParseTreeDispose(tree);
    if (sub != 0)
        PT_ParseTreeDispose(sub);
    if (data != 0)
        IndirectDeallocate(data);
    return err;
}

void IT_IndexDispose(ITElement *index)
{
    PT_ParseTreeDispose(index);
}

int32_t IT_IndexCount(ITElement *index, const char *language)
{
    return IT_IndexCountTag(index, "INTERFACE", language);
}

int32_t IT_IndexFind(ITElement *index, const char *language, ULong n, char *a, char *b, char *c, unsigned short *d)
{
    return IT_IndexFindNth(index, "INTERFACE", language, n, a, b, c, d);
}

int32_t IT_IndexLookup(ITElement *index, const char *language, const char *path, char *a, char *b, char *c, unsigned short *d, int32_t *f)
{
    return IT_IndexFindPath(index, "INTERFACE", language, path, a, b, c, d, 0, f);
}

int32_t IT_TextAppendNormal(void **handle, const unsigned short *text, unsigned char squeeze, unsigned char keepEdge)
{
    int32_t err;
    int32_t e;
    const unsigned short *from;
    const unsigned short *start;
    unsigned char space;
    ULong size;
    ULong n;
    ULong c;

    err = 0;
    do {
        start = text;
        from = start;
        space = 0;
        UnicodeAdvancePtr(&text, &c);
        size = 2;
        if (c == 0)
            break;
        switch (c) {
        case 9:
            if (squeeze)
                space = 1;
            else
                from = kFourSpacesW;
            break;
        case 13:
            if (UnicodeGetCharByIndex(text, 0) == 10)
                continue;
            /* fall through */
        case 10:
            if (squeeze)
                space = 1;
            else
                from = kCarriageW;
            break;
        case 0x20:
            if (squeeze)
                space = 1;
            break;
        }
        if (space) {
            n = IndirectGetSize(handle);
            if (n != 0) {
                c = UnicodeGetCharByIndex(*handle, UnicodeGetCharCountFromBytes(*handle, n) - 1);
                if (c == 0x20)
                    continue;
            } else if (keepEdge) {
                continue;
            }
            from = kSpaceW;
        }
        if (from != start)
            size = UnicodeGetByteCount(from);
        e = IndirectAppendData(handle, from, size);
        if (e != 0) {
            err = e;
            goto done;
        }
    } while (c != 0);
    e = IndirectAppendData(handle, kEmptyW, 2);
    if (e != 0)
        err = e;
done:
    return err;
}

#define kNodeSpecial    -3
#define kNodeText       -1
#define kNodeRoot       0
#define kNodeOpen       1
#define kNodeEmpty      2

static void **IT_TextFromNodes(ITElement *root)
{
    int32_t err;
    int32_t e;
    void **text;
    void **piece;
    ITElement *node;
    const unsigned short *from;
    ULong size;
    ULong c;
    unsigned char raw;
    unsigned char edge;

    err = 0;
    piece = 0;
    raw = 0;
    edge = 1;
    if ((text = IndirectAllocate(0)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if ((piece = IndirectAllocate(0)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    for (node = root; node != 0; node = PT_ParseTreeGetNext(root, node)) {
        switch (node->tag) {
        case kNodeSpecial:
            from = 0;
            raw = 0;
            break;
        case kNodeText:
            from = (const unsigned short *)node->attributes;
            raw = 0;
            break;
        case kNodeRoot:
            from = 0;
            raw = 0;
            break;
        case kNodeOpen:
        case kNodeEmpty:
            if (UnicodeCompare((unsigned short *)node->name, kPW) == 0) {
                from = kBlankLineW;
                raw = 1;
            } else if (UnicodeCompare((unsigned short *)node->name, kBrW) == 0) {
                from = kNewlineW;
                raw = 1;
            } else if (UnicodeCompare((unsigned short *)node->name, kLiW) == 0) {
                from = kBulletW;
                raw = 1;
            } else if (UnicodeCompare((unsigned short *)node->name, kDtW) == 0) {
                from = kNewlineW;
                raw = 1;
            } else if (UnicodeCompare((unsigned short *)node->name, kDdW) == 0) {
                from = kNewlineW;
                raw = 1;
            } else {
                from = 0;
            }
            break;
        default:
            from = 0;
            raw = 0;
            break;
        }
        if (from != 0) {
            IndirectSetLock(piece, 0);
            IndirectSetSize(piece, 0);
            e = IT_TextAppendNormal(piece, from, AsByte(raw == 0), edge);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(piece, 1);
            from = *piece;
        }
        if (from != 0 && (size = UnicodeGetByteCount(from)) != 0) {
            e = IndirectAppendData(text, from, size);
            if (e != 0) {
                err = e;
                goto done;
            }
            c = UnicodeGetCharByIndex(from, size - 1);
            edge = AsByte(c == 9 || c == 10 || c == 13 || c == 0x20);
        }
    }
    e = IndirectAppendData(text, kEmptyW, 2);
    if (e != 0)
        err = e;
done:
    if (piece != 0)
        IndirectDeallocate(piece);
    if (err != 0 && text != 0) {
        IndirectDeallocate(text);
        text = 0;
    }
    return text;
}

int32_t IT_StringTableLoad(ITElement *root, unsigned char keepWide, struct HashTable **out)
{
    void **text;
    char *narrow;
    struct HashTable *table;
    ITElement *e1;
    ITElement *node;
    ITElement *top;
    const void *name;
    int32_t err;
    int32_t e;
    unsigned char handled;
    char key[256];

    err = 0;
    narrow = 0;
    text = 0;
    *out = 0;
    table = HashCreate((!root->wide || keepWide) ? 0 : 0x01000000);
    if (table == 0) {
        err = kOutOfMemory;
        goto done;
    }
    for (e1 = root; e1 != 0; e1 = PT_ParseTreeGetNext(root, e1)) {
        if (IsTag(e1)) {
            if (!root->wide && StringCompare(e1->name, kStringTableA) == 0)
                break;
            if (root->wide && UnicodeCompare((unsigned short *)e1->name, kStringTableW) == 0)
                break;
        }
    }
    top = e1 != 0 ? e1->child : 0;
    for (node = top; node != 0; node = handled ? node->next : PT_ParseTreeGetNext(top, node)) {
        handled = 0;
        if (!IsTag(node))
            continue;
        if (!top->wide && StringCompare(node->name, kStringA) != 0)
            continue;
        if (top->wide && UnicodeCompare((unsigned short *)node->name, kStringW) != 0)
            continue;
        if (!top->wide) {
            name = HashLookup(node->attributes, kNameA, 0);
            if (name == 0)
                name = kNoNameA;
        } else {
            name = HashLookupUnicode(node->attributes, kNameW, 0);
            if (name == 0)
                name = kEmptyW;
        }
        text = IT_TextFromNodes(node->child);
        if (text == 0) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(text, 1);
        if (!top->wide) {
            e = HashAppend(table, name, *text);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else if (!keepWide) {
            e = HashAppendUnicode(table, name, *text);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            narrow = MemoryAllocate(UnicodeGetCharCount(*text) + 1);
            if (narrow == 0) {
                err = kOutOfMemory;
                goto done;
            }
            U2CStringCopySafe((unsigned char *)key, name, sizeof(key), -1);
            U2CStringCopySafe((unsigned char *)narrow, *text, -1, -1);
            e = HashAppend(table, key, narrow);
            if (e != 0) {
                err = e;
                goto done;
            }
            MemoryDeallocate(narrow);
            narrow = 0;
        }
        IndirectDeallocate(text);
        text = 0;
        handled = 1;
    }
    *out = table;
    table = 0;
done:
    if (narrow != 0)
        MemoryDeallocate(narrow);
    if (text != 0)
        IndirectDeallocate(text);
    if (table != 0)
        HashDispose(table);
    return err;
}

#endif
