// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itmarkup.c -- the interface tool's markup tree (0x0052489d..0x00526eac): elements linked by next / first child and numbered in document order; the character classes and the URL / HTML escaper. Notes: Research/notes/itmarkup.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "ittoolkit.h"

/* text nodes carry tags -3..-1 and their text where an element keeps its attributes */

#define kTagText2           -2
#define kTagText3           -1
#define kTableWide          0x01000000

extern unsigned short *UnicodeCopySafe(unsigned short *dst, const unsigned short *src,
                                    ULong dstBytes, ULong maxChars);    /* atcharset.c */
extern ULong UnicodeGetByteCountFromChars(const unsigned short *s, ULong index);        /* atcharset.c */

/* the parser's state (0x24 bytes, a local of 0x00524bc7) */
typedef struct ITParser {
    ITElement *root;
    ITElement *open;                /* the elements still open, innermost first */
    void **text;
    ULong unk0c;
    const char *base;
    const char *end;
    struct HashTable *raw;          /* tag names whose content is not parsed */
    struct HashTable *empty;        /* tag names that take no end tag */
    unsigned char wide;
} ITParser;

/* one parse error as appended to the error handle */
typedef struct ITParseError {
    long unk00;
    long unk04;
    long unk08;
    ULong offset;
    ULong length;
    ULong line;
    ULong lineStart;
    ULong lineLength;
    char file[256];
    char message[256];
} ITParseError;

typedef struct ITString {
    char s[256];
} ITString;

#define kClassOpener        0x08    /* `<(', `<[', `<{', `<#' ... open a special tag */
#define kUnknownEncoding    -7009

/* character classes of a code point; UTF-16 text treats every non-ASCII one as a name char */
#define IS_SPACE(wide, c)   ((wide) ? ((c) <= 0x7f && (kCharClass[c] & kClassSpace)) \
                                    : (kCharClass[c] & kClassSpace))
#define IS_NAME(wide, c)    ((wide) ? ((c) > 0x7f || (kCharClass[c] & kClassName)) \
                                    : (kCharClass[c] & kClassName))
#define EITHER(wide, e)     ((wide) ? (e) : (e))

#define kClassQuote     0x20
#define kClassUrl       0x40
#define kClassHtml      0x80
#define kAsciiLimit     0x7f
#define kFirstWide      0xff
#define kEscapeSize     0x100

/* 0x0057bef0 */
const unsigned char kCharClass[256] = {
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0x46, 0x46, 0xd0, 0xd0, 0x46, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0x16, 0x10, 0x30, 0x18, 0x18, 0x40, 0x90, 0x70, 0x18, 0x10, 0x18, 0x18, 0x10, 0x10, 0x10, 0x10,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x10, 0x10, 0x90, 0x1c, 0x90, 0x18,
    0x18, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x18, 0x10, 0x10, 0x10, 0x01,
    0x10, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x18, 0x10, 0x10, 0x18, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0,
    0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0, 0xd0
};
extern unsigned char IndirectIsNormalizedUnicode(void **handle);                        /* atcharset.c */
extern long IndirectRemoveUnicodeTag(void **handle);                                 /* atcharset.c */
extern unsigned short *UnicodeDuplicate(const unsigned short *s);            /* atcharset.c */
extern long UnicodeCompareSafe(const unsigned short *a, const unsigned short *b, ULong max);
extern const unsigned short *UnicodeAdvanceCount(const unsigned short *s, ULong count);
long _PT_ParseTreeLoadTag(ITParser *parser, void **errors, const char *start, const char *end,
                  char opener, unsigned char *raw);
const char *_PT_ParseFindDelim(ITParser *parser, const char *start, const char *end, char close);
unsigned char IT_SkipSlash(ITParser *parser, const char **cursor, const char *end);
long _PT_ParseTreeLoadText(ITParser *parser, void **errors, const char *start, const char *end);
long IT_ParseCommentNode(ITParser *parser, void **errors, const char *start, const char *end);
long IT_ParseSpecialNode(ITParser *parser, void **errors, const char *start, const char *end);
long _PT_ParseHashAppend(struct HashTable *table, const void *key, ULong keyLength, const void *value,
                  ULong valueLength);

extern ULong UnicodeGetCharCount(const unsigned short *s);                      /* atcharset.c */
extern unsigned char UnicodeAdvancePtr(const unsigned short **cursor, ULong *code);  /* atcharset.c */
long _PT_ParseTreeLoad(ITParser *parser, ITElement *node, void **errors, unsigned char open,
                  unsigned char close);
long PT_ParseDecode(void **text, unsigned char wide, unsigned char escapes);
long _PT_ParseError(void **text, void **errors, long unk00, long unk04, long unk08, ULong offset,
                  ULong length, const char *file, const char *format, const void *argument,
                  unsigned char wide);
ITElement *PT_ParseTreeCreate(long tag, const char *text, ULong length);
ITElement *PT_ParseTreeCreateUnicode(long tag, const unsigned short *text, ULong count);

ULong _PT_ParseTreeRenumber(ITElement *e, ULong n);

/* _PT_ParseTreeInsert (a new element or text node linked into a tree: text tags -3..-1 copy the
   text, element tags 1..2 copy the name and get an attribute table; UTF-16 when `wide';
   NULL when out of memory). STATE: MATCH 100.0%, exe 145 / obj 145, byte-identical yes. OPEN:
   the seventh argument is never read */
// FUNCTION: EVNOVA 0x0052489d
ITElement *_PT_ParseTreeInsert(ITElement *root, ITElement *where, unsigned char before, long tag,
                        const void *text, ULong length, ULong unused, unsigned char wide)
{
    long err;
    ITElement *node;
    ULong size;
    const unsigned short *wtext;
    const char *ctext;

    err = 0;
    wtext = text;
    ctext = text;
    node = 0;
    node = MemoryAllocateClear(sizeof(ITElement));
    if (node == 0) {
        err = kOutOfMemory;
        goto done;
    }
    node->wide = wide;
    node->tag = tag;
    switch (node->tag) {
    case kTagText1:
        if (wide) {
            size = length + 2;
            if ((node->attributes = MemoryAllocate(size)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            UnicodeCopySafe((unsigned short *)node->attributes, text, size, -1);
        } else {
            size = length + 1;
            if ((node->attributes = MemoryAllocate(size)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            StringCopySafe((char *)node->attributes, text, size);
        }
        break;
    case kTagText2:
    case kTagText3:
        if (wide) {
            size = length + 2;
            if ((node->attributes = MemoryAllocate(size)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            UnicodeCopySafe((unsigned short *)node->attributes, text, size, -1);
        } else {
            size = length + 1;
            if ((node->attributes = MemoryAllocate(size)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            StringCopySafe((char *)node->attributes, text, size);
        }
        break;
    case 0:
        break;
    case kTagElement:
    case kTagEnd:
        if (wide) {
            size = length + 2;
            if ((node->name = MemoryAllocate(size)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            Unicode2UpperCopySafe((unsigned short *)node->name, text, size, -1);
        } else {
            size = length + 1;
            if ((node->name = MemoryAllocate(size)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            String2UpperCopySafe(node->name, text, size);
        }
        if ((node->attributes = HashCreate((wide ? kTableWide : 0) | 1)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        break;
    }
    PT_ParseTreeInsertNode(root, where, node, before);
done:
    if (err != 0 && node != 0) {
        PT_ParseTreeDispose(node);
        node = 0;
    }
    return node;
}

/* PT_ParseTreeCreate (a new detached node from a C string). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00524a2b
ITElement *PT_ParseTreeCreate(long tag, const char *text, ULong length)
{
    return _PT_ParseTreeInsert(0, 0, 1, tag, text, length, length, 0);
}

/* PT_ParseTreeCreateUnicode (a new detached node from UTF-16 text, `count' characters). STATE:
   MATCH 100%, exe 20 / obj 20, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00524a4a
ITElement *PT_ParseTreeCreateUnicode(long tag, const unsigned short *text, ULong count)
{
    return _PT_ParseTreeInsert(0, 0, 1, tag, text, UnicodeGetByteCountFromChars(text, count), count, 1);
}

/* PT_ParseTreeInsertNode (link `node' into a tree: with no `where', as the root's successor, or as the new
   root before it when `before'; else at the end of where's children, or, when `before', ahead of
   them; the tree renumbered). STATE: MATCH 100%, exe 45 / obj 45, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00524a75
ITElement *PT_ParseTreeInsertNode(ITElement *root, ITElement *where, ITElement *node,
                        unsigned char before)
{
    ITElement **link;

    if (root == 0) {
        root = node;
    } else if (where == 0) {
        if (before) {
            node->next = root;
            root = node;
        } else {
            root->next = node;
        }
    } else if (before) {
        for (link = &node->next; *link != 0; link = &(*link)->next)
            ;
        *link = where->child;
        where->child = node;
    } else {
        for (link = &where->child; *link != 0; link = &(*link)->next)
            ;
        *link = node;
    }
    _PT_ParseTreeRenumber(root, 1);
    return node;
}

/* PT_ParseTreeGetParent (the parent of `node', found by the document-order numbers). STATE: MATCH 100%,
   exe 29 / obj 29, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00524ad5
ITElement *PT_ParseTreeGetParent(ITElement *root, ITElement *node)
{
    ITElement *parent;
    ITElement *e;
    ITElement *next;
    long err;

    err = 0;
    e = 0;
    parent = 0;
    for (e = root; e != 0 && e != node; ) {
        next = e->next;
        if (next == 0 || next->order > node->order) {
            parent = e;
            e = e->child;
        } else {
            e = next;
        }
    }
    if (e == 0)
        parent = 0;
    return parent;
}

/* PT_ParseTreeGetNext (the element after `node' in document order). STATE: MATCH 100%, exe 24 / obj 24,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00524b11
ITElement *PT_ParseTreeGetNext(ITElement *root, ITElement *node)
{
    ITElement *e;
    long err;

    err = 0;
    e = 0;
    for (e = root; e != 0 && e->order != node->order + 1;) {
        if (e->next == 0 || e->next->order > node->order + 1)
            e = e->child;
        else
            e = e->next;
    }
    return e;
}

/* PT_ParseTreeRemove (unlink `node' from a tree, with its children when `subtree', else leaving them
   in its place; disposed when asked; the tree renumbered and its root answered). STATE: MATCH
   100%, exe 56 / obj 56, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00524b45
ITElement *PT_ParseTreeRemove(ITElement *root, ITElement *node, unsigned char dispose,
                        unsigned char subtree)
{
    ITElement *parent;
    ITElement **link;
    long err;

    err = 0;
    parent = PT_ParseTreeGetParent(root, node);
    link = parent != 0 ? &parent->child : &root;
    while (*link != 0 && *link != node)
        link = &(*link)->next;
    if (*link == node) {
        if (subtree) {
            *link = node->next;
            node->next = 0;
        } else {
            *link = node->child;
            while (*link != 0)
                link = &(*link)->next;
            *link = node->next;
            node->next = 0;
            node->child = 0;
        }
        if (dispose)
            PT_ParseTreeDispose(node);
    }
    _PT_ParseTreeRenumber(root, 1);
    return root;
}

/* PT_ParseTreeLoad (parse a markup text into a tree: a UTF-16 text is parsed from a converted
   copy, NULs become spaces; text, comment and tag nodes placed through 0x0052624b; the
   content of a `raw' tag kept as one node up to its end tag; elements left open reported and
   closed; an empty text gives a lone text node). STATE: MATCH 100%, exe 710 / obj 710,
   byte-identical yes (2446 bytes). OPEN: none; notes/itmarkup.md */
// FUNCTION: EVNOVA 0x00524bc7
long PT_ParseTreeLoad(void **text, const char *rawTags, const char *emptyTags, void **errors,
                  ITElement **out)
{
    long err;
    long r;
    const char *base;
    unsigned char converted;
    unsigned char wide;
    const char *q;
    char name[256];
    unsigned short wideName[256];
    const char *p;
    const char *mark;
    const char *pos;
    const char *end;
    const unsigned short *u;
    ITElement *node;
    unsigned char isTag;
    unsigned char isNamed;
    unsigned char isOpen;
    ULong size;
    ULong i;
    ULong n;
    ULong last;
    ULong limit;
    const unsigned short *next;
    ITElement *e;
    ITElement *tree;
    char c;
    unsigned char raw;
    ITParser parser;
    ULong ch;
    const char *cur;
    ULong code;

    err = 0;
    converted = 0;
    raw = 0;
    node = 0;
    if (errors != 0)
        IndirectGetSize(errors);
    IndirectGetLock(text);
    *out = tree = 0;
    MemoryClear(&parser, sizeof(parser));
    parser.text = text;
    parser.raw = 0;
    parser.empty = 0;
    if (IndirectIsUnicode(text, 0) && !IndirectIsNormalizedUnicode(text)) {
        err = kUnknownEncoding;
        goto done;
    }
    parser.wide = IndirectIsUnicode(text, 0);
    wide = parser.wide;
    if (wide) {
        converted = 1;
        text = IndirectDuplicate(text);
        if (text == 0) {
            err = kOutOfMemory;
            goto done;
        }
        r = IndirectRemoveUnicodeTag(text);
        if (r != 0) {
            err = r;
            goto done;
        }
    }
    if (wide) {
        if (rawTags != 0 && *rawTags != 0) {
            parser.raw = HashInitializeUnicode(1, C2UStringCopySafe(wideName, (const unsigned char *)rawTags,
                                                      sizeof(wideName), -1));
            if (parser.raw == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        if (emptyTags != 0 && *emptyTags != 0) {
            parser.empty = HashInitializeUnicode(1, C2UStringCopySafe(wideName,
                                                        (const unsigned char *)emptyTags,
                                                        sizeof(wideName), -1));
            if (parser.empty == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
    } else {
        if (rawTags != 0 && *rawTags != 0) {
            parser.raw = HashInitialize(1, rawTags);
            if (parser.raw == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        if (emptyTags != 0 && *emptyTags != 0) {
            parser.empty = HashInitialize(1, emptyTags);
            if (parser.empty == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
    }
    if (wide) {
        ULong step;

        next = *text;
        u = *text;
        size = IndirectGetSize(text);
        for (i = 0; i < size; i += step) {
            next = UnicodeAdvance(u = next, &ch);
            step = (const char *)next - (const char *)u;
            if (ch == 0) {
                r = IndirectSpliceData(text, i, step, L" ", 2);
                if (r != 0) {
                    err = r;
                    goto done;
                }
                step = 2;
                next = (const unsigned short *)((char *)*text + i + 2);
            }
        }
    } else {
        char *s;
        ULong j;
        ULong len;

        s = *text;
        len = IndirectGetSize(text);
        for (j = 0; j < len; j++)
            if (s[j] == 0)
                s[j] = ' ';
    }
    IndirectSetLock(text, 1);
    parser.base = *text;
    base = parser.base;
    parser.end = parser.base + IndirectGetSize(text);
    end = parser.end;
    wide = parser.wide;
    for (pos = base; pos < end; pos = q) {
        q = _PT_ParseFindDelim(&parser, pos, end, '<');
        if (pos == base && q != pos) {
            r = _PT_ParseTreeLoadText(&parser, errors, pos, q);
            if (r != 0) {
                err = r;
                goto done;
            }
        } else if (q - pos != 0) {
            r = _PT_ParseTreeLoadText(&parser, errors, pos, q);
            if (r != 0) {
                err = r;
                goto done;
            }
        }
        pos = q;
        if (pos == end)
            break;
        if ((wide && end - pos >= UnicodeGetByteCount(L"<!--")
             && UnicodeCompareSafe((const unsigned short *)pos, L"<!--", 4) == 0)
            || (!wide && (ULong)(end - pos) >= 5 && StringCompareSafe(pos, "<!--", 4) == 0)) {
            if (wide) {
                cur = pos;
                code = 0;
                cur += UnicodeGetByteCount(L"<!--") / 2 * 2;
                while (cur < end) {
                    last = code;
                    UnicodeAdvancePtr((const unsigned short **)&cur, &code);
                    if (code == '-' && last == '-')
                        break;
                }
                while (cur < end) {
                    UnicodeAdvancePtr((const unsigned short **)&cur, &code);
                    if (code == '>')
                        break;
                }
                /* BUG: q is left past the `>', and the `q += 2' below steps it again */
                q = cur;
            } else {
                p = pos;
                for (p += StringGetLength((const unsigned char *)"<!--"); p < end - 1; p++)
                    if (p[0] == '-' && p[1] == '-')
                        break;
                for (; p < end; p++)
                    if (*p == '>')
                        break;
                q = p;
            }
            r = IT_ParseSpecialNode(&parser, errors, pos, q);
            if (r != 0) {
                err = r;
                goto done;
            }
            if (q < end)
                q += wide ? 2 : 1;
            goto advanced;
        } else {
            c = 0;
            if (wide)
                q = (const char *)UnicodeAdvance((const unsigned short *)pos, 0);
            else
                q = pos + 1;
            q = _PT_ParseFindDelim(&parser, q, end, '>');
            if (q < end) {
                if (wide)
                    c = UnicodePoint2Char8(UnicodeGetCharByIndex((const unsigned short *)pos, 1));
                else
                    c = pos[1];
            }
            if (kCharClass[(unsigned char)c] & kClassOpener) {
                r = _PT_ParseTreeLoadTag(&parser, errors, pos, q, c, &raw);
                if (r != 0) {
                    err = r;
                    goto done;
                }
                raw = 0;
            } else {
                r = _PT_ParseTreeLoadTag(&parser, errors, pos, q, 0, &raw);
                if (r != 0) {
                    err = r;
                    goto done;
                }
            }
            if (q < end)
                q += wide ? 2 : 1;
            goto advanced;
        }
advanced:
        pos = q;
        if (pos == end)
            break;
        if (raw) {
            if (parser.open->tag > 0)
                isOpen = 1;
            else
                isOpen = 0;
            while (raw && q < end) {
                /* BUG: one byte past q, not one character: a `<' at q is missed, and a
                   UTF-16 text is read off its character boundary */
                q = _PT_ParseFindDelim(&parser, p = q + 1, end, '<');
                mark = q;
                if (q == end)
                    break;
                if (wide)
                    UnicodeAdvancePtr((const unsigned short **)&q, 0);
                else
                    q++;
                if (!IT_SkipSlash(&parser, &q, end))
                    continue;
                if (q == end)
                    break;
                if (wide) {
                    n = end - q + 1;
                    if (n > sizeof(wideName))
                        n = sizeof(wideName);
                    limit = n;
                    Unicode2UpperCopySafe(wideName, (const unsigned short *)q, n,
                                 UnicodeGetCharCount((unsigned short *)parser.open->name));
                    r = UnicodeCompare((unsigned short *)parser.open->name, wideName);
                } else {
                    limit = end - q + 1;
                    if (limit > sizeof(name))
                        limit = sizeof(name);
                    n = limit;
                    if (limit > StringGetLength((unsigned char *)parser.open->name) + 1)
                        n = StringGetLength((unsigned char *)parser.open->name) + 1;
                    String2UpperCopySafe(name, q, n);
                    r = StringCompare(parser.open->name, name);
                }
                if (r == 0) {
                    q += n;
                    q = mark;
                    raw = 0;
                }
            }
            raw = 0;
            r = IT_ParseCommentNode(&parser, errors, pos, q);
            if (r != 0) {
                err = r;
                goto done;
            }
        }
    }
    while ((e = parser.open) != 0) {
        if (e->tag > 0)
            isTag = 1;
        else
            isTag = 0;
        r = (e->tag > 2 && e->tag < 0x7f);
        if (r)
            isNamed = 1;
        else
            isNamed = 0;
        node = e;
        parser.open = e->next;
        e->next = 0;
        r = _PT_ParseError(text, errors, 0, 1, 1, e->sourceOffset,
                         e->wide ? UnicodeGetByteCount((unsigned short *)e->name)
                                 : StringGetLength((unsigned char *)e->name),
                         0, "Unbalanced or badly nested tag <%s>", e->name, e->wide);
        if (r != 0) {
            err = r;
            goto done;
        }
        _PT_ParseTreeLoad(&parser, e, errors, 0, 0);
        node = 0;
    }
    if (parser.root == 0) {
        parser.root = MemoryAllocateClear(sizeof(ITElement));
        if (parser.root == 0) {
            err = kOutOfMemory;
            goto done;
        }
        parser.root->tag = kTagText3;
        parser.root->wide = wide;
        if (wide) {
            if ((parser.root->attributes = (struct HashTable *)UnicodeDuplicate(L"")) == 0) {
                err = kOutOfMemory;
                goto done;
            }
        } else {
            if ((parser.root->attributes = (struct HashTable *)StringDuplicate("")) == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
    }
    _PT_ParseTreeRenumber(parser.root, 1);
    *out = parser.root;
    parser.root = 0;
done:
    if (text != 0)
        IndirectSetLock(text, 0);
    if (converted && text != 0)
        IndirectDeallocate(text);
    if (parser.root != 0)
        PT_ParseTreeDispose(parser.root);
    if (parser.open != 0)
        PT_ParseTreeDispose(parser.open);
    if (parser.raw != 0)
        HashDispose(parser.raw);
    if (parser.empty != 0)
        HashDispose(parser.empty);
    return err;
}

/* PT_ParseTreeDispose (dispose a list of elements and everything under them). STATE: MATCH 100%, exe 53
   / obj 53, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00525555
void PT_ParseTreeDispose(ITElement *e)
{
    ITElement **link;
    long err;
    ITElement *cur;

    err = 0;
    link = &e;
    while (*link != 0) {
        cur = *link;
        *link = cur->next;
        if (cur->child != 0)
            PT_ParseTreeDispose(cur->child);
        if (cur->name != 0)
            MemoryDeallocate(cur->name);
        switch (cur->tag) {
        case kTagText1:
        case kTagText2:
        case kTagText3:
            if (cur->attributes != 0)
                MemoryDeallocate(cur->attributes);
            break;
        case 0:
            break;
        case kTagElement:
        case kTagEnd:
            if (cur->attributes != 0)
                HashDispose(cur->attributes);
            break;
        }
        MemoryDeallocate(cur);
    }
}

/* _PT_ParseTreeRenumber (number a list of elements and their descendants in document order from
   `n'; the next number). STATE: MATCH 100%, exe 21 / obj 21, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005255cd
ULong _PT_ParseTreeRenumber(ITElement *e, ULong n)
{
    long err;

    err = 0;
    for (; e != 0; e = e->next) {
        e->order = n++;
        if (e->child != 0)
            n = _PT_ParseTreeRenumber(e->child, n);
    }
    return n;
}

/* _PT_ParseTreeLoadTag (parse one tag between `start' (its `<') and `end' (its `>'): the element
   and its attributes, values unquoted and %-decoded; `opener' a special tag's second
   character, which becomes its tag; `raw' set for a tag whose content is not markup).
   STATE: MATCH 100%, exe 779 / obj 779, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005255f8
long _PT_ParseTreeLoadTag(ITParser *parser, void **errors, const char *start, const char *end,
                  char opener, unsigned char *raw)
{
    long err;
    long e;
    unsigned char wide;
    const char *p;
    void **value;
    unsigned char endTag;
    unsigned char empty;
    unsigned char pending;
    unsigned char unexpected;
    char closer;
    ITElement *node;
    const char *here;
    const char *last;
    const char *name;
    const char *valueStart;
    ULong nameLength;
    ULong valueLength;
    ULong n;
    long state;
    ULong quote;
    ULong ch;
    ULong code;

    err = 0;
    wide = parser->wide;
    value = 0;
    node = 0;
    empty = 0;
    endTag = 0;
    pending = 0;
    unexpected = 0;
    closer = 0;
    *raw = 0;
    switch (opener) {
    case '(':
        closer = ')';
        break;
    case '[':
        closer = ']';
        break;
    case '{':
        closer = '}';
        break;
    default:
        closer = opener;
        break;
    }
    if (wide) {
        UnicodeAdvancePtr((const unsigned short **)&start, 0);
        if (start >= end)
            goto done;
        if (opener != 0) {
            UnicodeAdvancePtr((const unsigned short **)&start, 0);
            if (start >= end)
                goto done;
            n = UnicodeGetCharCountFromBytes((const unsigned short *)start, end - start);
            if (n != 0) {
                last = (const char *)UnicodeAdvanceCount((const unsigned short *)start, n - 1);
                UnicodeAdvance((const unsigned short *)last, &code);
                if (closer == (char)UnicodePoint2Char8(code))
                    end = last;
            }
            if (start >= end)
                goto done;
        }
    } else {
        start++;
        if (start >= end)
            goto done;
        if (opener != 0) {
            start++;
            if (start >= end)
                goto done;
            if (end[-1] == closer)
                end--;
            if (start >= end)
                goto done;
        }
    }
    here = p = start;
    state = 0;
    for (; here <= end; here = p) {
        if (wide)
            p = (const char *)UnicodeAdvance((const unsigned short *)here, &ch);
        else
            ch = *(const unsigned char *)p++;
        if (here == end)
            ch = 0;
        switch (state) {
        case 0:                         /* before the tag name */
            if (ch == 0 || IS_SPACE(wide, ch))
                break;
            if (EITHER(wide, ch == '/')) {
                endTag = 1;
            } else if (IS_NAME(wide, ch)) {
                state = 1;
                name = here;
                nameLength = 1;
            } else if (!unexpected) {
                pending = unexpected = 1;
            }
            break;
        case 1:                         /* in the tag name */
            if (IS_NAME(wide, ch)) {
                nameLength++;
                break;
            }
            if (wide)
                node = PT_ParseTreeCreateUnicode(kTagEnd, (const unsigned short *)name, nameLength);
            else
                node = PT_ParseTreeCreate(kTagEnd, name, nameLength);
            if (node == 0)
                goto nomem;
            node->sourceOffset = start - parser->base;
            name = 0;
            if (EITHER(wide, ch == '/'))
                empty = 1;
            if (endTag) {
                p = end + 1;
                break;
            }
            state = 2;
            break;
        case 2:                         /* between attributes */
            if (ch == 0 || IS_SPACE(wide, ch))
                break;
            if (EITHER(wide, ch == '/')) {
                empty = 1;
            } else if (IS_NAME(wide, ch)) {
                empty = 0;
                state = 3;
                name = here;
                nameLength = 1;
            } else if (!unexpected) {
                pending = unexpected = 1;
            }
            break;
        case 3:                         /* in an attribute name */
            if (IS_NAME(wide, ch)) {
                nameLength++;
                break;
            }
            if (EITHER(wide, ch == '/'))
                empty = 1;
            state = EITHER(wide, ch == '=') ? 5 : 4;
            break;
        case 4:                         /* after an attribute name */
            if (ch == 0 || IS_SPACE(wide, ch))
                break;
            if (EITHER(wide, ch == '=')) {
                state = 5;
            } else if (IS_NAME(wide, ch)) {
                err = _PT_ParseHashAppend(node->attributes, name, nameLength, 0, 0);
                if (err == kDuplicateKey) {
                    e = _PT_ParseError(parser->text, errors, kDuplicateKey, 1, 1,
                                     start - parser->base, end - start, 0,
                                     "Ignoring duplicate parameter in tag <%s>", node->name,
                                     node->wide);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    err = 0;
                }
                e = err;
                if (e != 0)
                    goto done;
                state = 3;
                name = here;
                nameLength = 1;
            } else if (!unexpected) {
                pending = unexpected = 1;
            }
            break;
        case 5:                         /* after `=' */
            if (ch == 0 || IS_SPACE(wide, ch))
                break;
            empty = 0;
            if (EITHER(wide, ch == '"' || ch == '\'' || ch == '`')) {
                state = 7;
                quote = ch;
                valueStart = p;
                valueLength = 0;
            } else {
                state = 6;
                quote = 0;
                valueStart = here;
                valueLength = 1;
            }
            break;
        case 6:                         /* in an unquoted value */
        case 7:                         /* in a quoted value */
            if (state == 6 && ch != 0 && !EITHER(wide, ch == '/') && !IS_SPACE(wide, ch)) {
                valueLength++;
                break;
            }
            if (state == 7 && ch != quote) {
                valueLength++;
                break;
            }
            if (value == 0) {
                value = IndirectAllocate(0);
                if (value == 0) {
nomem:
                    err = kOutOfMemory;
                    goto done;
                }
            }
            e = IndirectReplaceData(value, valueStart, here - valueStart);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = PT_ParseDecode(value, wide, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(value, 1);
            err = _PT_ParseHashAppend(node->attributes, name, nameLength, *value,
                               IndirectGetSize(value));
            IndirectSetLock(value, 0);
            if (err == kDuplicateKey) {
                e = _PT_ParseError(parser->text, errors, kDuplicateKey, 1, 1,
                                 start - parser->base, end - start, 0,
                                 "Ignoring duplicate parameter in tag <%s>", node->name,
                                 node->wide);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                err = 0;
            }
            e = err;
            if (e != 0)
                goto done;
            e = IndirectSetSize(value, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            name = 0;
            if (ch == '/')
                empty = 1;
            state = 2;
            break;
        }
        if (pending) {
            e = _PT_ParseError(parser->text, errors, 0, 1, 1, start - parser->base,
                             end - start, 0, "Unexpected character in tag", 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            pending = 0;
        }
    }
    if (node == 0)
        goto done;
    if (name != 0) {
        err = _PT_ParseHashAppend(node->attributes, name, nameLength, 0, 0);
        if (err == kDuplicateKey) {
            e = _PT_ParseError(parser->text, errors, kDuplicateKey, 1, 1, start - parser->base,
                             end - start, 0, "Ignoring duplicate parameter in tag <%s>",
                             node->name, node->wide);
            if (e != 0) {
                err = e;
                goto done;
            }
            err = 0;
        }
        e = err;
        if (e != 0)
            goto done;
    }
    if (wide) {
        if (parser->empty != 0
            && HashLookupUnicode(parser->empty, (unsigned short *)node->name, 0) != 0)
            empty = 1;
        if (!empty && !endTag && !unexpected && parser->raw != 0
            && HashLookupUnicode(parser->raw, (unsigned short *)node->name, 0) != 0)
            *raw = 1;
    } else {
        if (parser->empty != 0 && HashLookup(parser->empty, node->name, 0) != 0)
            empty = 1;
        if (!empty && !endTag && !unexpected && parser->raw != 0
            && HashLookup(parser->raw, node->name, 0) != 0)
            *raw = 1;
    }
    if (opener != 0)
        node->tag = opener;
    else if (empty)
        node->tag = kTagElement;
    if (empty)
        endTag = 0;
    _PT_ParseTreeLoad(parser, node, errors, !endTag && !empty, endTag);
    node = 0;
done:
    if (node != 0)
        PT_ParseTreeDispose(node);
    if (value != 0)
        IndirectDeallocate(value);
    return err;
}

/* _PT_ParseTreeLoadText (a text node (tag -1) for the text between `start' and `end', its entities decoded
   first (0x00526abd), its source offset kept, inserted by 0x0052624b). STATE: MATCH 100%, exe 95
   / obj 95, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00526076
long _PT_ParseTreeLoadText(ITParser *parser, void **errors, const char *start, const char *end)
{
    long err;
    long e;
    void **text;
    ITElement *node;

    err = 0;
    node = 0;
    text = 0;
    text = IndirectInitialize(start, end - start);
    if (text == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = PT_ParseDecode(text, parser->wide, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    IndirectSetLock(text, 1);
    if (parser->wide) {
        node = PT_ParseTreeCreateUnicode(kTagText3, *text, UnicodeGetCharCountFromBytes(*text, IndirectGetSize(text)));
        if (node == 0) {
            err = kOutOfMemory;
            goto done;
        }
    } else {
        node = PT_ParseTreeCreate(kTagText3, *text, IndirectGetSize(text));
        if (node == 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    node->sourceOffset = start - parser->base;
    _PT_ParseTreeLoad(parser, node, errors, 0, 0);
    node = 0;
done:
    if (node != 0)
        PT_ParseTreeDispose(node);
    if (text != 0)
        IndirectDeallocate(text);
    return err;
}

/* IT_ParseCommentNode (a comment node (tag -2) for the text between `start' and `end', its source
   offset kept, inserted by 0x0052624b). STATE: MATCH 100%, exe 56 / obj 56, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052614f
long IT_ParseCommentNode(ITParser *parser, void **errors, const char *start, const char *end)
{
    long err;
    ITElement *node;

    err = 0;
    node = 0;
    if (parser->wide)
        node = PT_ParseTreeCreateUnicode(kTagText2, (const unsigned short *)start,
                            UnicodeGetCharCountFromBytes((const unsigned short *)start, end - start));
    else
        node = PT_ParseTreeCreate(kTagText2, start, end - start);
    if (node == 0) {
        err = kOutOfMemory;
    } else {
        node->sourceOffset = start - parser->base;
        _PT_ParseTreeLoad(parser, node, errors, 0, 0);
        node = 0;
    }
    if (node != 0)
        PT_ParseTreeDispose(node);
    return err;
}

/* IT_ParseSpecialNode (the same for a tag -3 node). STATE: MATCH 100%, exe 56 / obj 56,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005261cd
long IT_ParseSpecialNode(ITParser *parser, void **errors, const char *start, const char *end)
{
    long err;
    ITElement *node;

    err = 0;
    node = 0;
    if (parser->wide)
        node = PT_ParseTreeCreateUnicode(kTagText1, (const unsigned short *)start,
                            UnicodeGetCharCountFromBytes((const unsigned short *)start, end - start));
    else
        node = PT_ParseTreeCreate(kTagText1, start, end - start);
    if (node == 0) {
        err = kOutOfMemory;
    } else {
        node->sourceOffset = start - parser->base;
        _PT_ParseTreeLoad(parser, node, errors, 0, 0);
        node = 0;
    }
    if (node != 0)
        PT_ParseTreeDispose(node);
    return err;
}

/* _PT_ParseTreeLoad (place a parsed node: an open tag is pushed on the open list; an end tag
   closes its innermost open namesake, every element opened inside it reported as badly
   nested and closed first, and is disposed, or is reported and kept when nothing matches;
   anything else goes under the innermost open element or at the top level). STATE: MATCH
   100%, exe 213 / obj 213, byte-identical yes. OPEN: none (one walker `e` for both list
   walks, two labels for the shared tail: 335 cross-jumps the two identical stores otherwise) */
// FUNCTION: EVNOVA 0x0052624b
long _PT_ParseTreeLoad(ITParser *parser, ITElement *node, void **errors, unsigned char open,
                  unsigned char close)
{
    long err;
    ITElement *e;
    long result;

    err = 0;
    e = 0;
    e = 0;
    if (close) {
        if (node->wide) {
            if (node->name != 0)
                UnicodeGetCharCount((unsigned short *)node->name);
        } else if (node->name != 0) {
            StringGetLength((unsigned char *)node->name);
        }
        for (e = parser->open; e != 0; e = e->next) {
            if (e->wide) {
                if (e->name != 0)
                    UnicodeGetCharCount((unsigned short *)e->name);
            } else if (e->name != 0) {
                StringGetLength((unsigned char *)e->name);
            }
            if (node->wide) {
                if (UnicodeCompare((unsigned short *)node->name, (unsigned short *)e->name) == 0)
                    break;
            } else if (StringCompare(node->name, e->name) == 0) {
                break;
            }
        }
        if (e != 0) {
            while (node->wide ? UnicodeCompare((unsigned short *)parser->open->name,
                                             (unsigned short *)node->name)
                              : StringCompare(parser->open->name, node->name)) {
                e = parser->open;
                parser->open = e->next;
                e->next = 0;
                result = _PT_ParseError(parser->text, errors, 0, 1, 1, e->sourceOffset,
                                      e->wide ? UnicodeGetByteCount((unsigned short *)e->name)
                                              : StringGetLength((unsigned char *)e->name),
                                      0, "Unbalanced or badly nested tag <%s>", e->name, e->wide);
                if (result != 0) {
                    err = result;
                    return err;
                }
                _PT_ParseTreeLoad(parser, e, errors, 0, 0);
                e = 0;
            }
            PT_ParseTreeDispose(node);
            node = 0;
            e = parser->open;
            parser->open = e->next;
            e->next = 0;
            _PT_ParseTreeLoad(parser, e, errors, 0, 0);
            e = 0;
            return 0;
        }
        result = _PT_ParseError(parser->text, errors, 0, 1, 1, node->sourceOffset,
                              node->wide ? UnicodeGetByteCount((unsigned short *)node->name)
                                         : StringGetLength((unsigned char *)node->name),
                              0, "Unbalanced or badly nested tag </%s>", node->name, node->wide);
        if (result != 0) {
            err = result;
            return err;
        }
        _PT_ParseTreeLoad(parser, node, errors, 0, 0);
        goto closed;
    }
    if (open) {
        node->next = parser->open;
        parser->open = node;
        goto done;
    }
    if (parser->open != 0) {
        for (e = (ITElement *)&parser->open->child; e->next != 0; e = e->next)
            ;
        e->next = node;
        goto done;
    }
    if (parser->root != 0) {
        for (e = parser->root; e->next != 0; e = e->next)
            ;
        e->next = node;
closed:
done:
        node = 0;
        return 0;
    }
    parser->root = node;
    node = 0;
    return 0;
}

/* _PT_ParseFindDelim (the end of a tag: the first `close' outside quotes, or inside quotes that saw
   nothing odd). STATE: MATCH 100%, exe 157 / obj 157, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00526458
const char *_PT_ParseFindDelim(ITParser *parser, const char *start, const char *end, char close)
{
    long err;
    unsigned char wide;
    unsigned char odd;
    char quote;
    char c;
    const char *p;
    const char *here;
    const char *last;
    const char *q;
    ULong count;
    ULong code;

    err = 0;
    wide = parser->wide;
    odd = 0;
    quote = 0;
    p = here = start;
    while (p < end) {
        last = here;
        here = p;
        if (wide) {
            UnicodeAdvancePtr((const unsigned short **)&p, &code);
            c = UnicodePoint2Char8(code);
        } else {
            c = *p++;
        }
        if (c == quote) {
            quote = 0;
            odd = 0;
            continue;
        }
        switch (c) {
        case '"':
        case '\'':
        case '`':
            if (quote != 0)
                goto retest;
            /* BUG: counts the quote's partners to the end of the line, not of the tag */
            count = 0;
            quote = c;
            odd = 0;
            if (wide) {
                q = p;
                while (q < end) {
                    UnicodeAdvancePtr((const unsigned short **)&q, &code);
                    if (code == '\r' || code == '\n')
                        break;
                    if (c == (char)UnicodePoint2Char8(code))
                        count++;
                }
            } else {
                for (q = p; q < end && *q != '\r' && *q != '\n'; q++)
                    if (*q == c)
                        count++;
            }
            if (count & 1)
                odd = 1;
            break;
retest:
            if (quote != 0)
                odd = 1;
            break;
        case '#': case '$': case '&': case '(': case '*': case '+': case ',': case ';':
        case '=': case '@': case '[': case '{': case '|':
            odd = 1;
            break;
        case '<':
            if (c == close)
                return here;
            /* BUG: ends the tag before the character read last, not at the `<' */
            if (quote == 0)
                return last;
            break;
        case '>':
            if (c == close && quote == 0)
                return here;
            if (c == close && quote != 0 && !odd)
                return here;
            break;
        }
    }
    return p;
}

/* IT_SkipSlash (skip white space, an optional `/', white space again; whether there was a
   `/'). STATE: MATCH 100%, exe 85 / obj 85, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00526646
unsigned char IT_SkipSlash(ITParser *parser, const char **cursor, const char *end)
{
    unsigned char slash;
    unsigned char wide;
    const char *p;
    ULong n;
    ULong ch;
    ULong code;
    long err;

    err = 0;
    slash = 0;
    wide = parser->wide;
    for (p = *cursor; p < end; p += n) {
        n = _PT_ParseAdvance((const unsigned char *)p, &ch, wide ? &code : 0);
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
            break;
    }
    if (p < end) {
        n = _PT_ParseAdvance((const unsigned char *)p, &ch, wide ? &code : 0);
        if (ch == '/') {
            p += n;
            slash = 1;
        }
    }
    for (; p < end; p += n) {
        n = _PT_ParseAdvance((const unsigned char *)p, &ch, wide ? &code : 0);
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
            break;
    }
    *cursor = p;
    return slash;
}

/* _PT_ParseHashAppend (set an attribute from a counted name: copied NUL-terminated, as UTF-16 for a
   wide table). STATE: MATCH 100%, exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00526725
long _PT_ParseHashAppend(struct HashTable *table, const void *key, ULong keyLength, const void *value,
                  ULong valueLength)
{
    long err;
    long e;
    char *name;
    unsigned short *wideName;
    ULong size;

    err = 0;
    name = 0;
    wideName = 0;
    if (HashGetFlags(table) & kTableWide) {
        size = UnicodeGetByteCountFromChars(key, keyLength) + 2;
        wideName = MemoryAllocate(size);
        if (wideName == 0)
            goto nomem;
        UnicodeCopySafe(wideName, key, size, keyLength);
        e = HashAppendUnicodeData(table, wideName, value, valueLength);
        if (e != 0)
            err = e;
    } else {
        size = keyLength + 1;
        name = MemoryAllocate(size);
        if (name == 0) {
nomem:
            err = kOutOfMemory;
            goto done;
        }
        StringCopySafe(name, key, size);
        e = HashAppendData(table, name, value, valueLength);
        if (e != 0)
            err = e;
    }
done:
    if (name != 0)
        MemoryDeallocate(name);
    if (wideName != 0)
        MemoryDeallocate(wideName);
    return err;
}

/* 0x0057c0a4: the literals of a markup function whose code is not linked */
static const char kTagOpen[] = "<";
static const char kTagSpace[] = " ";
static const char kTagEquals[] = "=";
static const char kTagClose[] = ">";
static const char kTagSlash[] = "/";

/* One character off the text, UTF-16 when a code point is asked for; answers
   the bytes it used. */
/* _PT_ParseAdvance. STATE: MATCH 100%, exe 32 / obj 32, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005267ef
ULong _PT_ParseAdvance(const unsigned char *s, ULong *ch, ULong *code)
{
    ULong len;

    if (code != 0) {
        len = (ULong)UnicodeAdvance((const unsigned short *)s, code);
        *ch = UnicodePoint2Char8(*code);
        len -= (ULong)s;
        return len;
    }
    *ch = *s;
    return 1;
}

static const char kHexDigits[] = "0123456789ABCDEF";

/* Escapes a handle's text in place, for a URL or for HTML. */
/* PT_ParseEncode. STATE: MATCH 100%, exe 178 / obj 178, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052682e
long PT_ParseEncode(void **h, unsigned char wide, unsigned char url,
                  unsigned char spaces, unsigned char quotes)
{
    long err = 0;
    ULong mask;
    unsigned char digits;
    ULong size;
    ULong pos;
    ULong used;
    ULong ch;
    char text[kEscapeSize];
    unsigned short wideText[kEscapeSize];
    ULong code;
    long e;
    ULong length;
    const unsigned char *start;
    const unsigned char *p;
    const void *out;

    digits = 1;
    size = IndirectGetSize(h);
    if (url)
        mask = kClassUrl;
    else
        mask = kClassHtml;
    if (spaces)
        mask |= kClassSpace;
    if (quotes)
        mask |= kClassQuote;

    for (pos = 0; pos < size; pos += used) {
        start = (const unsigned char *)*h;
        p = start + pos;
        used = _PT_ParseAdvance(p, &ch, wide ? &code : 0);
        if (ch < kAsciiLimit && (kCharClass[ch] & mask) == 0)
            continue;

        if (url) {
            if (digits && ch == '%' && pos + used == size)
                continue;
            if (ch < '0' || ch > '9')
                digits = 0;
            if (wide && code >= kFirstWide) {
                StringFormatSafe(text, kEscapeSize, "%%u%4.4lX", code);
            } else {
                text[0] = '%';
                text[1] = kHexDigits[(ch >> 4) & 0xf];
                text[2] = kHexDigits[ch & 0xf];
                text[3] = 0;
            }
        } else if (wide && code >= kFirstWide) {
            StringFormatSafe(text, kEscapeSize, "&#u%4.4lX;", code);
        } else {
            switch (ch) {
            case '&':
                StringCopy(text, "&amp;");
                break;
            case '<':
                StringCopy(text, "&lt;");
                break;
            case '>':
                StringCopy(text, "&gt;");
                break;
            case 0xa0:
                StringCopy(text, "&nbsp;");
                break;
            case 0xa9:
                StringCopy(text, "&copy;");
                break;
            case 0xae:
                StringCopy(text, "&reg;");
                break;
            default:
                StringFormatSafe(text, kEscapeSize, "&#%lu;", ch);
                break;
            }
        }

        if (wide) {
            C2UStringCopySafe(wideText, (const unsigned char *)text, sizeof(wideText),
                         kNoLength);
            out = wideText;
            length = UnicodeGetByteCount(wideText);
        } else {
            out = text;
            length = StringGetLength((const unsigned char *)text);
        }
        e = IndirectSpliceData(h, pos, used, out, length);
        if (e != 0) {
            err = e;
            break;
        }
        used = length;
    }
    return err;
}

extern char EncodingLookupTag(const char *entity);                            /* atcharset.c */

extern ULong UnicodeLookupTag(const char *entity);                           /* atcharset.c */

/* Placement only: no reader in the exe. */
static unsigned char sMarkupPad[24];           /* 0x0086bdc0 */


/* PT_ParseDecode (decode a text in place: `&name;' entities, or with `escapes' `%xx' and
   `%uxxxx'; the numeric forms `&#nn;' / `&#unnnn;' and the % escapes never replace anything,
   their digit counter is never advanced, as in the exe). STATE: MATCH 100%, exe 304 / obj 304,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00526abd
long PT_ParseDecode(void **text, unsigned char wide, unsigned char escapes)
{
    long err;
    const void *entity;
    long e;
    ULong size;
    ULong pos;
    ULong n;
    ULong len;
    ULong count;
    ULong digits;
    ULong value;
    const void *data;
    ULong dataLength;
    const char *p;
    const char *base;
    ULong ch;
    ULong code;
    unsigned short utf16[256];
    char name[259];
    unsigned char byte;

    err = 0;
    size = IndirectGetSize(text);
    for (pos = 0; pos < size; pos += len) {
        base = *text;
        p = base + pos;
        len = n = _PT_ParseAdvance((const unsigned char *)p, &ch, wide ? &code : 0);
        if (escapes && ch == '%') {
            digits = 2;
            n = _PT_ParseAdvance((const unsigned char *)p + n, &ch, wide ? &code : 0);
            if (ch == 'u') {
                digits = 4;
                len += n;
            }
            /* BUG: count is never advanced (here and below), so no numeric form ever replaces */
            count = value = 0;
            for (; count < digits && pos + len < size; len += n) {
                n = _PT_ParseAdvance((const unsigned char *)p + len, &ch, wide ? &code : 0);
                if (ch >= '0' && ch <= '9')
                    value = value << 4 | (ch - '0');
                else if (ch >= 'a' && ch <= 'f')
                    value = value << 4 | (ch - 'a' + 10);
                else if (ch >= 'A' && ch <= 'F')
                    value = value << 4 | (ch - 'A' + 10);
                else
                    break;
            }
            if (count < digits)
                continue;
        } else if (!escapes && ch == '&') {
            n = _PT_ParseAdvance((const unsigned char *)p + n, &ch, wide ? &code : 0);
            if (ch == '#') {
                digits = 2;
                len += n;
                n = _PT_ParseAdvance((const unsigned char *)p + len, &ch, wide ? &code : 0);
                if (ch == 'u') {
                    digits = 4;
                    len += n;
                }
                count = value = 0;
                for (; count < digits && pos + len < size; len += n) {
                    n = _PT_ParseAdvance((const unsigned char *)p + len, &ch, wide ? &code : 0);
                    if (ch < '0' || ch > '9')
                        break;
                    value = value * 10 | (ch - '0');
                }
                if (count < digits)
                    continue;
                n = _PT_ParseAdvance((const unsigned char *)(p += len), &ch, wide ? &code : 0);
                if (ch != ';')
                    continue;
                len += n;
            } else {
                n = 0;
                count = 0;
                for (; count < 7 && ch != '&' && ch != ';' && pos + len + n < size; count++)
                    n += _PT_ParseAdvance((const unsigned char *)p + len + n, &ch, wide ? &code : 0);
                if (ch != ';')
                    continue;
                if (wide) {
                    count++;
                    entity = p;
                    U2CStringCopySafe((unsigned char *)name, (const unsigned short *)entity, 0x100, count);
                    value = UnicodeLookupTag(name);
                } else {
                    entity = p;
                    StringCopySafe(name, entity, count + 2);
                    value = EncodingLookupTag(name);
                }
                if (value == 0)
                    continue;
                len += n;
            }
        } else {
            continue;
        }
        if (wide) {
            code = value;
            UTF32_2UStringCopySafe(utf16, &code, sizeof(utf16), 1);
            data = utf16;
            dataLength = UnicodeGetByteCount(utf16);
        } else {
            byte = value;
            data = &byte;
            dataLength = 1;
        }
        e = IndirectSpliceData(text, pos, len, data, dataLength);
        if (e != 0) {
            err = e;
            break;
        }
        len = dataLength;
    }
    return err;
}

/* _PT_ParseError (append a parse error: its line number, line start and length found in the text,
   the file name and the message FORMAT). STATE: MATCH 100%, exe 213 / obj 213, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x00526eac
long _PT_ParseError(void **text, void **errors, long unk00, long unk04, long unk08, ULong offset,
                  ULong length, const char *file, const char *format, const void *argument,
                  unsigned char wide)
{
    static ITString noArgument;
    static ITString noMessage;
    long err;
    long e;
    long result;
    ULong line;
    ULong lineStart;
    ULong lineLength;
    ULong pos;
    const char *s;
    const unsigned short *u;
    unsigned char cr;
    unsigned char lf;
    unsigned char lastCR;
    unsigned char lastLF;
    unsigned char isCR;
    unsigned char isLF;
    ITParseError record;
    ITString message;
    ITString narrow;
    ULong ch;

    err = 0;
    message = noMessage;
    narrow = noArgument;
    lineStart = -1;
    lineLength = -1;
    line = -1;
    if (text != 0 && offset != -1 && length != -1) {
        IndirectGetSize(text);
        IndirectGetSize(text);
        if (wide) {
            u = *text;
            lastCR = lastLF = 0;
            lineStart = lineLength = 0;
            line = 1;
            pos = 0;
            while (pos < offset + length) {
                u = UnicodeAdvance(u, &ch);
                pos = (const char *)u - (const char *)*text;
                if (ch == '\r')
                    isCR = 1;
                else
                    isCR = 0;
                cr = isCR;
                if (ch == '\n')
                    isLF = 1;
                else
                    isLF = 0;
                lf = isLF;
                if (pos < offset) {
                    if (cr || (lf && !lastCR))
                        line++;
                    if ((lastCR && !lf) || lastLF)
                        lineStart = pos;
                } else if (pos == offset) {
                    if (cr || lf)
                        break;
                    lineLength = pos - lineStart;
                /* BUG: compares with length where offset + length is meant */
                } else if (pos < length) {
                    if (cr || lf)
                        break;
                    lineLength++;
                }
                lastCR = cr;
                lastLF = lf;
            }
        } else {
            s = *text;
            for (lineStart = offset; lineStart != 0; lineStart--)
                if (s[lineStart] == '\r' || s[lineStart] == '\n')
                    break;
            for (lineLength = lineStart; s[lineLength] != 0; lineLength++)
                if (s[lineLength] == '\r' || s[lineLength] == '\n')
                    break;
            lineLength -= lineStart;
            pos = 0;
            line = 1;
            for (; pos <= lineStart; pos++)
                if (s[pos] == '\n' || (s[pos] == '\r' && s[pos + 1] != '\n'))
                    line++;
        }
    }
    if (format != 0) {
        if (wide && argument != 0)
            argument = U2CStringCopySafe((unsigned char *)narrow.s, argument, sizeof(narrow.s), -1);
        StringFormatSafe(message.s, sizeof(message.s), format, argument);
    }
    if (errors != 0) {
        MemoryClear(&record, sizeof(record));
        record.unk00 = unk00;
        record.unk04 = unk04;
        record.unk08 = unk08;
        record.offset = offset;
        record.length = length;
        record.lineStart = lineStart;
        record.lineLength = lineLength;
        record.line = line;
        if (file != 0)
            StringCopySafe(record.file, file, sizeof(record.file));
        /* BUG: copies the format, and the message formatted above is dropped */
        if (format != 0)
            StringCopySafe(record.message, format, sizeof(record.message));
        e = IndirectAppendData(errors, &record, sizeof(record));
        if (e != 0)
            err = e;
    }
    result = err;
    return result;
}
