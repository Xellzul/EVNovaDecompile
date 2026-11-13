// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itattr.c -- the interface tool's markup element helpers (0x0054016a..0x00540b7e): attribute values, number and rectangle lists, the name table. Notes: Research/notes/itattr.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

extern unsigned char UnicodeAdvancePtr(const unsigned short **p, ULong *c);

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
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    unsigned short wideName[256];
#endif
    char wanted[256];
    int32_t err;
    unsigned char a;

    err = 0;
    String2UpperCopySafe(wanted, name, sizeof(wanted));
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    if (root->wide)
        C2UStringCopySafe(wideName, (const unsigned char *)wanted, sizeof(wideName), (ULong)-1);
#endif
    if (skip && start != 0)
        start = _IT_ParseTreeFindLastNode(root, start);
    if (start == 0)
        start = root;
    else
        start = PT_ParseTreeGetNext(root, start);
    for (; start != 0; start = PT_ParseTreeGetNext(root, start)) {
#if defined(MAC1010) && TARGET_API_MAC_CARBON
        if (start->name != 0 && AsByte(start->tag > 0) && !AsByte(start->tag > 2 && start->tag < 0x7f)) {
            if (root->wide && UnicodeCompare((unsigned short *)start->name, wideName) == 0)
                break;
            if (!root->wide && StringCompare(start->name, wanted) == 0)
                break;
        }
#else
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
#endif
    }
    return start;
}

/* _IT_ParseTreeFindParentTag (`element' or its nearest enclosing element named `tag'; NULL when none). STATE:
   MATCH 100%, exe 67 / obj 67, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054021f
ITElement *_IT_ParseTreeFindParentTag(ITElement *root, ITElement *element, const char *tag)
{
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    unsigned short wideName[256];
#endif
    char tagName[256];
    int32_t err;
    unsigned char a;
    unsigned char b;
    unsigned char c;

    err = 0;
    String2UpperCopySafe(tagName, tag, sizeof(tagName));
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    if (root->wide)
        C2UStringCopySafe(wideName, (const unsigned char *)tagName, sizeof(wideName), (ULong)-1);
    do {
        if (element->tag > 0 && !AsByte(element->tag > 2 && element->tag < 0x7f) && element->name != 0) {
            if (root->wide && UnicodeCompare((unsigned short *)element->name, wideName) == 0)
                break;
            if (!root->wide && StringCompare(element->name, tagName) == 0)
                break;
        }
    } while ((element = PT_ParseTreeGetParent(root, element)) != 0);
    return element;
#else
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
#endif
}

/* _IT_ParseTreeFindLastNode (the last element of a subtree in document order). STATE: MATCH 100%, exe 35 / obj
   35, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005402c4
ITElement *_IT_ParseTreeFindLastNode(void *doc, ITElement *element)
{
    int32_t err;
    unsigned char named;
    ITElement *e;

    err = 0;
    if (element->child != 0) {
        element = element->child;
        while ((e = element->next) != 0 || element->child != 0) {
            if (element->tag > 0)
                named = true;
            else
                named = false;
            if (element->tag > 0)
                named = true;
            else
                named = false;
            element = e != 0 ? e : element->child;
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
    int32_t err;

    err = 0;
    found = 0;
    if (text == 0) {
        found = e;
    } else {
        for (; e != 0 && found == 0; e = e->next) {
            if ((char *)e->attributes == text)
                found = e;
            else if (e->child != 0)
                found = _IT_ParseTreeFindThisData(e->child, text);
        }
    }
    return found;
}

/* _IT_ParseTreeFindNextData (the next text node after `node' under `root' whose text is not empty). STATE:
   MATCH 100%, exe 28 / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054034a
ITElement *_IT_ParseTreeFindNextData(ITElement *root, ITElement *node)
{
    int32_t err;
    ULong len;

    err = 0;
    len = 0;
    while ((node = PT_ParseTreeGetNext(root, node)) != 0) {
        if (node->tag == kTagText && node->attributes != 0 &&
#if defined(MAC1010) && TARGET_API_MAC_CARBON
            (len = node->wide ? UnicodeGetCharCount((unsigned short *)node->attributes) : StringGetLength((unsigned char *)node->attributes)) != 0)
#else
            (len = StringGetLength((unsigned char *)node->attributes)) != 0)
#endif
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
    int32_t err;
    unsigned char a;
    unsigned char b;
    unsigned char c;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    unsigned short wideTag[256];
    unsigned short wideKey[256];
#endif
    char tagName[256];
    char keyName[256];

    err = 0;
    value = 0;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    if (tag != 0) {
        String2UpperCopySafe(tagName, tag, sizeof(tagName));
        if (root->wide)
            C2UStringCopySafe(wideTag, (const unsigned char *)tagName, sizeof(wideTag), (ULong)-1);
    }
    StringCopySafe(keyName, key, sizeof(keyName));
    if (root->wide)
        C2UStringCopySafe(wideKey, (const unsigned char *)keyName, sizeof(wideKey), (ULong)-1);
    do {
        if (element->tag > 0 && !AsByte(element->tag > 2 && element->tag < 0x7f) && element->name != 0) {
            if (tag != 0) {
                if (root->wide && UnicodeCompare((unsigned short *)element->name, wideTag) != 0)
                    goto next;
                if (!root->wide && StringCompare(element->name, tagName) != 0)
                    goto next;
            }
            if (root->wide)
                value = HashLookupUnicode(element->attributes, wideKey, 0);
            else
                value = HashLookup(element->attributes, keyName, 0);
        }
next:
        ;
    } while (inherit && value == 0 && (element = PT_ParseTreeGetParent(root, element)) != 0);
    return value;
#else
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
#endif
}

/* _IT_SimpleFindAttribute (an element's attribute value by name, NULL when absent). STATE: MATCH 100%,
   exe 28 / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540482
char *_IT_SimpleFindAttribute(ITElement *element, const char *key)
#if defined(MAC1010) && TARGET_API_MAC_CARBON
{
    unsigned short wideName[256];
    char name[256];

    StringCopySafe(name, key, sizeof(name));
    if (element->wide)
        C2UStringCopySafe(wideName, (const unsigned char *)name, sizeof(wideName), (ULong)-1);
    if (element->wide)
        return HashLookupUnicode(element->attributes, wideName, 0);
    return HashLookup(element->attributes, name, 0);
}
#else
{
    char name[256];
    int32_t err;
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
#endif

/* _IT_ParseAttributeGetNumber (a signed decimal number, up to a comma). STATE: MATCH 100%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005404d9
int32_t _IT_ParseAttributeGetNumber(unsigned char wide, const char *s)
#if defined(MAC1010) && TARGET_API_MAC_CARBON
{
    unsigned char negative;
    int32_t value;

    if (wide) {
        const unsigned short *p;
        ULong c;

        p = (const unsigned short *)s;
        value = 0;
        negative = 0;
        while (UnicodeAdvancePtr(&p, &c) && c != ',') {
            if (c >= '0' && c <= '9')
                value = value * 10 + (c - '0');
            else if (value == 0 && c == '-')
                negative = 1;
            else
                break;
        }
    } else {
        value = 0;
        negative = 0;
        while (*s != 0 && *s != ',') {
            if (*s >= '0' && *s <= '9')
                value = value * 10 + (*s - '0');
            else if (value == 0 && *s == '-')
                negative = 1;
            else
                break;
            s++;
        }
    }
    return negative ? -value : value;
}
#elif defined(MAC1010)
{
    unsigned char negative;
    int32_t value;

    value = 0;
    negative = 0;
    while (*s != 0 && *s != ',') {
        if (*s >= '0' && *s <= '9')
            value = value * 10 + (*s - '0');
        else if (value == 0 && *s == '-')
            negative = 1;
        else
            break;
        s++;
    }
    return negative ? -value : value;
}
#else
{
    int32_t value;
    unsigned char negative;
    char c;
    int32_t err = 0;
    int32_t tmp = 0;
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
#endif

static char kNumberFormat[] = "%ld";
#if defined(MAC1010) && TARGET_API_MAC_CARBON
#define kRightW L"RIGHT"
#define kLeftW L"LEFT"
#define kFalseW L"FALSE"
#define kNoneW L"NONE"
#endif

/* _IT_ParseAttributeGetAlign (an ALIGN value: 1 RIGHT, -1 LEFT, 0 anything else). STATE: MATCH 100%, exe 35 / obj 35, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00540528
int32_t _IT_ParseAttributeGetAlign(unsigned char wide, const char *s)
{
    int32_t align;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    unsigned short wideValue[256];
#endif
    char value[256];
    int32_t err;

    err = 0;
    align = 0;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    if (wide) {
        Unicode2UpperCopySafe(wideValue, (const unsigned short *)s, sizeof(wideValue), (ULong)-1);
        if (UnicodeCompare(wideValue, kRightW) == 0)
            align = kAlignRight;
        else if (UnicodeCompare(wideValue, kLeftW) == 0)
            align = -1;
    } else {
#endif
    String2UpperCopySafe(value, s, sizeof(value));
    if (StringCompare(value, "RIGHT") == 0)
        align = kAlignRight;
    else if (StringCompare(value, "LEFT") == 0)
        align = kAlignLeft;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    }
#endif
    return align;
}

static char kAlignCenter[] = "CENTER";

#ifdef MAC1010
static
#endif
ITColor gIT_DefaultColor = { 0xff, 0xff, 0xff, 0xff };  /* 0x00581b74 the default colour */

/* _IT_ParseAttributeGetColor (a colour attribute: `#rgb' or `#rrggbb', any number of leading '#', opaque;
   anything else is the default, opaque white). STATE: MATCH 100%, exe 169 / obj 169, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054058a
ITColor _IT_ParseAttributeGetColor(unsigned char wide, const char *s)
{
    ITColor color;
    const char *p;
    ULong length;
    ULong i;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    const unsigned short *wp;
#endif
    ULong c;
    int32_t err;

    err = 0;
    color = gIT_DefaultColor;
    p = s;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    wp = (const unsigned short *)s;
    if (wide) {
        while (UnicodeAdvancePtr(&wp, &c) && c != '#')
            ;
        length = UnicodeGetCharCount(wp);
    } else {
#endif
    while ((c = *p) == '#')
        p++;
    length = StringGetLength((const unsigned char *)p);
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    }
#endif
    if (length == 3 || length == 6) {
        color.alpha = 0xff;
        color.red = 0;
        color.green = 0;
        color.blue = 0;
        for (i = 0; i < length; i += 3) {
#if defined(MAC1010) && TARGET_API_MAC_CARBON
            if (wide)
                UnicodeAdvancePtr(&wp, &c);
            else
#endif
            c = *p++;
            color.red = color.red << 4 | HEX_DIGIT(c);
        }
        for (i = 0; i < length; i += 3) {
#if defined(MAC1010) && TARGET_API_MAC_CARBON
            if (wide)
                UnicodeAdvancePtr(&wp, &c);
            else
#endif
            c = *p++;
            color.green = color.green << 4 | HEX_DIGIT(c);
        }
        for (i = 0; i < length; i += 3) {
#if defined(MAC1010) && TARGET_API_MAC_CARBON
            if (wide)
                UnicodeAdvancePtr(&wp, &c);
            else
#endif
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
#ifdef MAC1010
void _IT_ParseAttributeGetRect(void *rect, unsigned char wide, char *s)
#else
ITRect *_IT_ParseAttributeGetRect(void *rect, unsigned char wide, const char *s)
#endif
{
    ITRect *r = rect;
    int32_t err = 0;
#ifdef MAC1010
    char *p = s;
#else
    const char *p = s;
#endif
    ITRect parsed = {0, 0, 0, 0};
    unsigned char negative;
    int32_t value;
    int32_t *field;
    ULong i;
    char c;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    const unsigned short *wp;
    ULong wc;
#endif

#if defined(MAC1010) && TARGET_API_MAC_CARBON
    wp = (const unsigned short *)s;
#endif
    field = &parsed.left;
    for (i = 0; i < 4; i++) {
#if defined(MAC1010) && TARGET_API_MAC_CARBON
        if (wide) {
            value = 0;
            negative = 0;
            while (UnicodeAdvancePtr(&wp, &wc) && wc != ',') {
                if (wc >= '0' && wc <= '9')
                    value = value * 10 + (wc - '0');
                else if (value == 0 && wc == '-')
                    negative = 1;
            }
        } else {
#endif
        value = 0;
        negative = 0;
        while ((c = *p++) != 0 && c != ',') {
            if (c >= '0' && c <= '9')
                value = value * 10 + (c - '0');
            else if (value == 0 && c == '-')
                negative = 1;
        }
#if defined(MAC1010) && TARGET_API_MAC_CARBON
        }
#endif
        *field++ = negative ? -value : value;
    }
    *r = parsed;
#ifndef MAC1010
    return rect;
#endif
}

/* _IT_ParseAttributeGetFlag (a boolean attribute: anything but FALSE or NONE, in any case, is true). STATE:
   MATCH 100%, exe 33 / obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005407b7
unsigned char _IT_ParseAttributeGetFlag(unsigned char wide, const char *s)
{
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    unsigned short wideValue[256];
#endif
    char value[256];
    int32_t err;
    int32_t result;

    err = 0;
    result = 1;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    if (wide) {
        Unicode2UpperCopySafe(wideValue, (const unsigned short *)s, sizeof(wideValue), (ULong)-1);
        if (UnicodeCompare(wideValue, kFalseW) == 0 || UnicodeCompare(wideValue, kNoneW) == 0)
            result = 0;
    } else {
#endif
    String2UpperCopySafe(value, s, sizeof(value));
    if (StringCompare(value, "FALSE") == 0 || StringCompare(value, "NONE") == 0)
        result = 0;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    }
#endif
    return result;
}

/* _IT_ParseTreeGetFontAttributes (the text style in force at `element': FACE and SIZE inherited, style bits from
   the enclosing B / STRONG, I / EM, U, TT, PRE and A HREF elements, FGCOLOR; each output only
   when asked for). STATE: MATCH 100%, exe 156 / obj 156, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540814
void _IT_ParseTreeGetFontAttributes(ITElement *root, ITElement *element, char **face, int32_t *size, ULong *style,
                  ITColor *color, char **href)
{
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    static unsigned char faceText[256];
    static unsigned char hrefText[256];
#endif
    char *value;
    int32_t err;

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
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    if (face != 0) {
        value = _IT_ParseTreeFindAttribute(root, element, 0, "FACE", 1);
        if (root->wide && value != 0)
            *face = (char *)U2CStringCopySafe(faceText, value, sizeof(faceText), -1);
        else
            *face = value;
    }
    if (size != 0) {
        value = _IT_ParseTreeFindAttribute(root, element, 0, "SIZE", 1);
        if (value != 0)
            *size = _IT_ParseAttributeGetNumber(root->wide, value);
    }
#else
    if (face != 0)
        *face = _IT_ParseTreeFindAttribute(root, element, 0, "FACE", 1);
    if (size != 0) {
        value = _IT_ParseTreeFindAttribute(root, element, 0, "SIZE", 1);
        if (value != 0)
            *size = _IT_ParseAttributeGetNumber(0, value);
    }
#endif
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
#if defined(MAC1010) && TARGET_API_MAC_CARBON
            *color = _IT_ParseAttributeGetColor(root->wide, value);
#else
            *color = _IT_ParseAttributeGetColor(0, value);
#endif
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
#if defined(MAC1010) && TARGET_API_MAC_CARBON
                if (root->wide)
                    *href = (char *)U2CStringCopySafe(hrefText, value, sizeof(hrefText), -1);
                else
#endif
                *href = value;
        }
    }
}

#ifndef MAC1010
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
    int32_t err;
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
int32_t _IT_PageListDispose(void *tableRef)
{
    ITPageTable *table = tableRef;
    int32_t err;
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
    int32_t err;

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
    int32_t err;

    err = 0;
    e = 0;
    for (e = table->first; e != 0 && index--; e = e->next)
        ;
    return e;
}
#endif
