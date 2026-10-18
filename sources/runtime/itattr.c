// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itattr.c -- the interface tool's markup element helpers (0x0054016a..0x00540b7e): attribute values, number and rectangle lists, the name table. Notes: Research/notes/itattr.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

#define kPageBuckets        31
#define HEX_DIGIT(c)        ((c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
                             (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : \
                             (c) >= '0' && (c) <= '9' ? (c) - '0' : 0)

/* _IT_ParseTreeFindNextTag (the next element named `name' after `start' in document order, past its subtree
   when `skip'; from the root when no start; tags 3..0x7e never match). STATE: MATCH 100%, exe 75
   / obj 75, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054016a
ITElement *_IT_ParseTreeFindNextTag(ITElement *root, ITElement *start, const char *name, unsigned char skip)
{
    char wanted[256];
    long err;
    unsigned char a;

    err = 0;
    String2UpperCopySafe(wanted, name, sizeof(wanted));
    if (skip && start != 0)
        start = _IT_ParseTreeFindLastNode(root, start);
    if (start == 0)
        start = root;
    else
        start = PT_ParseTreeGetNext(root, start);
    for (; start != 0; start = PT_ParseTreeGetNext(root, start)) {
        if (start->name != 0) {
            if (start->tag > 0)
                a = 1;
            else
                a = 0;
            if (a) {
                if (!AsByte(start->tag > 2 && start->tag < 0x7f)
                    && StringCompare(start->name, wanted) == 0)
                    break;
            }
        }
    }
    return start;
}

/* _IT_ParseTreeFindParentTag (`element' or its nearest enclosing element named `tag'; NULL when none). STATE:
   MATCH 100%, exe 67 / obj 67, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054021f
ITElement *_IT_ParseTreeFindParentTag(ITElement *root, ITElement *element, const char *tag)
{
    char tagName[256];
    long err;
    unsigned char a;
    unsigned char b;
    unsigned char c;

    err = 0;
    String2UpperCopySafe(tagName, tag, sizeof(tagName));
    do {
        if (element->tag > 0)
            a = 1;
        else
            a = 0;
        if (element->tag > 0)
            b = 1;
        else
            b = 0;
        if (element->tag > 0)
            c = 1;
        else
            c = 0;
        if (c) {
            if (!AsByte(element->tag > 2 && element->tag < 0x7f)) {
                if (element->name != 0 && StringCompare(element->name, tagName) == 0)
                    break;
            }
        }
        element = PT_ParseTreeGetParent(root, element);
    } while (element != 0);
    return element;
}

/* _IT_ParseTreeFindLastNode (the last element of a subtree in document order). STATE: MATCH 100%, exe 35 / obj
   35, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005402c4
ITElement *_IT_ParseTreeFindLastNode(void *doc, ITElement *element)
{
    long err;
    unsigned char named;
    ITElement *e;

    err = 0;
    e = element->child;
    if (e != 0) {
        while ((e = (element = e)->next) != 0 || element->child != 0) {
            if (element->tag > 0)
                named = true;
            else
                named = false;
            if (element->tag > 0)
                named = true;
            else
                named = false;
            if (e == 0)
                e = element->child;
        }
    }
    return element;
}

/* _IT_ParseTreeFindThisData (the node among `e', its successors and their subtrees whose text is `text';
   `e' itself when no text is given). STATE: MATCH 100%, exe 31 / obj 31, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x0054030c
ITElement *_IT_ParseTreeFindThisData(ITElement *e, const char *text)
{
    ITElement *found;
    long err;

    err = 0;
    found = 0;
    if (text == 0)
        return e;
    for (; e != 0 && found == 0; e = e->next) {
        if ((char *)e->attributes == text)
            found = e;
        else if (e->child != 0)
            found = _IT_ParseTreeFindThisData(e->child, text);
    }
    return found;
}

/* _IT_ParseTreeFindNextData (the next text node after `node' under `root' whose text is not empty). STATE:
   MATCH 100%, exe 28 / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054034a
ITElement *_IT_ParseTreeFindNextData(ITElement *root, ITElement *node)
{
    long err;
    long len;

    err = 0;
    len = 0;
    while ((node = PT_ParseTreeGetNext(root, node)) != 0) {
        if (node->tag == kTagText && node->attributes != 0 &&
            (len = StringGetLength((unsigned char *)node->attributes)) != 0)
            break;
    }
    return node;
}

/* _IT_ParseTreeFindAttribute (an attribute of `element', or, when `inherit', of the nearest enclosing element
   that has it; only elements named `tag' when one is given; tags 3..0x7e never answer). STATE:
   MATCH 100%, exe 96 / obj 96, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540387
char *_IT_ParseTreeFindAttribute(ITElement *root, ITElement *element, const char *tag, const char *key,
                   unsigned char inherit)
{
    char *value;
    long err;
    unsigned char a;
    unsigned char b;
    unsigned char c;
    char tagName[256];
    char keyName[256];

    err = 0;
    value = 0;
    if (tag != 0)
        String2UpperCopySafe(tagName, tag, sizeof(tagName));
    StringCopySafe(keyName, key, sizeof(keyName));
    do {
        if (element->tag > 0)
            a = 1;
        else
            a = 0;
        if (element->tag > 0)
            b = 1;
        else
            b = 0;
        if (element->tag > 0)
            c = 1;
        else
            c = 0;
        if (c) {
            if (!AsByte(element->tag > 2 && element->tag < 0x7f)) {
                if (element->name != 0 && (tag == 0 || StringCompare(element->name, tagName) == 0))
                    value = HashLookup(element->attributes, keyName, 0);
            }
        }
    } while (inherit && value == 0 && (element = PT_ParseTreeGetParent(root, element)) != 0);
    return value;
}

/* _IT_SimpleFindAttribute (an element's attribute value by name, NULL when absent). STATE: MATCH 100%,
   exe 28 / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540482
char *_IT_SimpleFindAttribute(ITElement *element, const char *key)
{
    char name[256];
    long err;
    char *result;
    unsigned char tagged;

    err = 0;
    result = 0;
    StringCopySafe(name, key, sizeof(name));
    if (element->tag > 0)
        tagged = true;
    else
        tagged = false;
    result = HashLookup(element->attributes, name, 0);
    return result;
}

/* _IT_ParseAttributeGetNumber (a signed decimal number, up to a comma). STATE: MATCH 100%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005404d9
long _IT_ParseAttributeGetNumber(unsigned char wide, const char *s)
{
    long value;
    unsigned char negative;
    char c;
    long err = 0;
    long tmp = 0;
    const char *p;

    p = s;
    value = 0;
    negative = 0;
    while ((c = *p) != 0 && c != ',') {
        if (c >= '0' && c <= '9')
            value = value * 10 + (c - '0');
        else if (value == 0 && c == '-')
            negative = 1;
        else
            break;
        p++;
    }
    tmp = negative ? -value : value;
    return tmp;
}

static char kNumberFormat[] = "%ld";

/* _IT_ParseAttributeGetAlign (an ALIGN value: 1 RIGHT, -1 LEFT, 0 anything else). STATE: MATCH 100%, exe 35 / obj 35, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00540528
long _IT_ParseAttributeGetAlign(unsigned char wide, const char *s)
{
    long align;
    char value[256];
    long err;

    err = 0;
    align = 0;
    String2UpperCopySafe(value, s, sizeof(value));
    if (StringCompare(value, "RIGHT") == 0)
        align = kAlignRight;
    else if (StringCompare(value, "LEFT") == 0)
        align = kAlignLeft;
    return align;
}

static char kAlignCenter[] = "CENTER";

ITColor gIT_DefaultColor = { 0xff, 0xff, 0xff, 0xff };  /* 0x00581b74 the default colour */

/* _IT_ParseAttributeGetColor (a colour attribute: `#rgb' or `#rrggbb', any number of leading '#', opaque;
   anything else is the default, opaque white). STATE: MATCH 100%, exe 169 / obj 169, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054058a
ITColor _IT_ParseAttributeGetColor(unsigned char wide, const char *s)
{
    ITColor color;
    ULong length;
    ULong i;
    ULong c;
    long err;
    const char *p;

    err = 0;
    color = gIT_DefaultColor;
    p = s;
    while ((c = *p) == '#')
        p++;
    length = StringGetLength((const unsigned char *)p);
    if (length == 3 || length == 6) {
        color.alpha = 0xff;
        color.red = 0;
        color.green = 0;
        color.blue = 0;
        for (i = 0; i < length; i += 3) {
            c = *p++;
            color.red = color.red << 4 | HEX_DIGIT(c);
        }
        for (i = 0; i < length; i += 3) {
            c = *p++;
            color.green = color.green << 4 | HEX_DIGIT(c);
        }
        for (i = 0; i < length; i += 3) {
            c = *p++;
            color.blue = color.blue << 4 | HEX_DIGIT(c);
        }
        if (length == 3) {
            color.red |= color.red << 4;
            color.green |= color.green << 4;
            color.blue |= color.blue << 4;
        }
    }
    return color;
}

static char kColorFormat[] = "#%2.2lX#%2.2lX#%2.2lX";
static char kRectFormat[] = "%ld,%ld,%ld,%ld";

/* _IT_ParseAttributeGetRect (four comma-separated signed numbers as a rectangle; each field's scan steps past
   its terminator, a NUL too, as the exe's does). STATE: MATCH 100%, exe 67 / obj 67, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054072a
ITRect *_IT_ParseAttributeGetRect(void *rect, unsigned char wide, const char *s)
{
    static ITRect zero;
    ITRect *r;
    ITRect parsed;
    long *field;
    ULong i;
    long value;
    unsigned char negative;
    char c;
    long err;
    const char *p;

    r = rect;
    err = 0;
    p = s;
    parsed = zero;
    field = &parsed.left;
    for (i = 0; i < 4; i++) {
        value = 0;
        negative = 0;
        while ((c = *p++) != 0 && c != ',') {
            if (c >= '0' && c <= '9')
                value = value * 10 + (c - '0');
            else if (value == 0 && c == '-')
                negative = 1;
        }
        *field++ = negative ? -value : value;
    }
    *r = parsed;
    return rect;
}

/* _IT_ParseAttributeGetFlag (a boolean attribute: anything but FALSE or NONE, in any case, is true). STATE:
   MATCH 100%, exe 33 / obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005407b7
unsigned char _IT_ParseAttributeGetFlag(unsigned char wide, const char *s)
{
    char value[256];
    long err;
    long result;

    err = 0;
    result = 1;
    String2UpperCopySafe(value, s, sizeof(value));
    if (StringCompare(value, "FALSE") == 0 || StringCompare(value, "NONE") == 0)
        result = 0;
    return result;
}

/* _IT_ParseTreeGetFontAttributes (the text style in force at `element': FACE and SIZE inherited, style bits from
   the enclosing B / STRONG, I / EM, U, TT, PRE and A HREF elements, FGCOLOR; each output only
   when asked for). STATE: MATCH 100%, exe 156 / obj 156, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540814
void _IT_ParseTreeGetFontAttributes(ITElement *root, ITElement *element, char **face, long *size, ULong *style,
                  ITColor *color, char **href)
{
    char *value;
    long err;

    err = 0;
    value = 0;
    if (face != 0)
        *face = 0;
    if (size != 0)
        *size = 0;
    if (style != 0)
        *style = 0;
    if (href != 0)
        *href = 0;
    if (face != 0)
        *face = _IT_ParseTreeFindAttribute(root, element, 0, "FACE", 1);
    if (size != 0) {
        value = _IT_ParseTreeFindAttribute(root, element, 0, "SIZE", 1);
        if (value != 0)
            *size = _IT_ParseAttributeGetNumber(0, value);
    }
    if (style != 0) {
        if (_IT_ParseTreeFindParentTag(root, element, "B") != 0 || _IT_ParseTreeFindParentTag(root, element, "STRONG") != 0)
            *style |= kStyleBold;
        if (_IT_ParseTreeFindParentTag(root, element, "I") != 0 || _IT_ParseTreeFindParentTag(root, element, "EM") != 0)
            *style |= kStyleItalic;
        if (_IT_ParseTreeFindParentTag(root, element, "U") != 0)
            *style |= kStyleUnderline;
        if (_IT_ParseTreeFindParentTag(root, element, "TT") != 0)
            *style |= kStyleTeletype;
        if (_IT_ParseTreeFindParentTag(root, element, "PRE") != 0)
            *style |= kStylePre;
    }
    if (color != 0) {
        value = _IT_ParseTreeFindAttribute(root, element, 0, "FGCOLOR", 1);
        if (value != 0) {
            *color = _IT_ParseAttributeGetColor(0, value);
            if (style != 0)
                *style |= kStyleColor;
        }
    }
    if (href != 0 || style != 0) {
        value = _IT_ParseTreeFindAttribute(root, element, "A", "HREF", 1);
        if (value != 0) {
            if (style != 0)
                *style |= kStyleLink;
            if (href != 0)
                *href = value;
        }
    }
}

/* IT_HashName (a name's hash). STATE: MATCH 100%, exe 17 / obj 17, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x005409d9
ULong IT_HashName(const unsigned char *s)
{
    ULong hash;

    hash = 0;
    for (; *s != 0; s++)
        hash = hash << 2 ^ hash >> 29 ^ *s;
    return hash;
}

static const unsigned char DAT_00581bf4[4] = { 0 };   /* 0x00581bf4, unreferenced */

/* _IT_PageListCreate (the page table of a document: every PAGE element with its NAME and TITLE, in
   document order and hashed by name over 31 buckets; NULL when out of memory). STATE: MATCH
   100%, exe 104 / obj 104, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005409fc
ITPageTable *_IT_PageListCreate(ITElement *root)
{
    long err;
    ITPageTable *table;
    ITPage **link;
    ITElement *e;
    ITPage *page;
    char *value;

    err = 0;
    value = 0;
    e = 0;
    table = 0;
    page = 0;
    table = MemoryAllocateClear(sizeof(ITPageTable));
    if (table == 0)
        goto nomem;
    table->root = root;
    table->buckets = kPageBuckets;
    while ((e = _IT_ParseTreeFindNextTag(root, e, "PAGE", 1)) != 0) {
        page = MemoryAllocateClear(sizeof(ITPage));
        if (page == 0) {
nomem:
            err = kOutOfMemory;
            goto done;
        }
        page->element = e;
        table->count++;
        value = _IT_SimpleFindAttribute(page->element, "NAME");
        if (value != 0)
            StringCopySafe(page->name, value, sizeof(page->name));
        value = _IT_SimpleFindAttribute(page->element, "TITLE");
        if (value != 0)
            StringCopySafe(page->title, value, sizeof(page->title));
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

/* _IT_PageListDispose (dispose a name table and its entries). STATE: MATCH 100%, exe 22 / obj 22,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540b10
long _IT_PageListDispose(void *tableRef)
{
    ITPageTable *table = (ITPageTable *)tableRef;
    long err;
    ITPage *e;
    ITPage *next;

    err = 0;
    for (e = table->first; e != 0; e = next) {
        next = e->next;
        MemoryDeallocate(e);
    }
    MemoryDeallocate(table);
    table = 0;
    return err;
}

/* _IT_GetPageByName (a name table's entry for a name, NULL when absent). STATE: MATCH 100%, exe 32 / obj 32, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00540b3c
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

/* _IT_GetPageByIndex (a name table's entry by position). STATE: MATCH 100%, exe 18 / obj 18,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540b7e
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
