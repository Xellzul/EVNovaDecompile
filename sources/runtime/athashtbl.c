// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* athashtbl.c -- the runtime block's hash table: the hash and key-compare functions, create, dispose, the registered test and the two inserts. Notes: Research/notes/athashtbl.md */

#include <stddef.h>
#include "burgerplat.h"
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "macport.h"

typedef struct HashEntry HashEntry;
typedef struct HashTable HashTable;

typedef ULong (*HashProc)(const void *key, ULong len);
typedef unsigned char (*CompareProc)(const void *a, ULong lenA,
                                     const void *b, ULong lenB);

struct HashEntry {
    HashEntry *chain;                            /* +0x00 next in bucket */
    HashEntry *prev;                             /* +0x04 insertion order */
    HashEntry *next;                             /* +0x08 */
    char *key;                                   /* +0x0c into data[] */
    ULong keyLen;                                /* +0x10 */
    ULong valueLen;                              /* +0x14 */
    unsigned char reserved[3];                   /* +0x18 */
    unsigned char tag;                           /* +0x1b */
    char data[1];                                /* +0x1c value, then key */
};

struct HashTable {
    HashTable *next;                             /* +0x00 all tables */
    ULong nBuckets;                              /* +0x04 */
    ULong flags;                                 /* +0x08 */
    HashProc hash;                               /* +0x0c */
    CompareProc compare;                         /* +0x10 */
    ULong keyClass;                              /* +0x14 */
    long cursorA;                                /* +0x18 */
    long cursorB;                                /* +0x1c */
    HashEntry *first;                            /* +0x20 */
    HashEntry *last;                             /* +0x24 */
    HashEntry *bucket[1];                        /* +0x28 */
};

#define kUpperFoldDelta 0xe0     /* + 0xe0 on a byte is - 0x20: to upper case */
#define kHashRotate   29

/* Every function here is a HashProc (key, length) or a CompareProc (a, length, b, length)
   of the table below; each views its keys through a local of the key's own type. */

/* _Hash. STATE: MATCH 100%, exe 19 / obj 19, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9c86
ULong _Hash(const void *key, ULong len)
{
    const char *p;
    ULong h;
    unsigned char c;
    long err;

    err = 0;
    p = (const char *)key;
    h = 0;
    while ((c = *p) != 0) {
        h = (h << 2) ^ (h >> kHashRotate) ^ c;
        p++;
    }
    return h;
}

/* _HashUpper. STATE: MATCH 100%, exe 24 / obj 24, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9cad
ULong _HashUpper(const void *key, ULong len)
{
    const char *p;
    ULong h;
    unsigned char c;
    long err;

    err = 0;
    p = (const char *)key;
    h = 0;
    while ((c = *p) != 0) {
        if (c >= kFoldLow && c <= kFoldHigh)
            c += kUpperFoldDelta;
        h = (h << 2) ^ (h >> kHashRotate) ^ c;
        p++;
    }
    return h;
}

/* _Match. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9ce1
unsigned char _Match(const void *a, ULong lenA, const void *b, ULong lenB)
{
    return StringCompare((const char *)a, (const char *)b) == 0;
}

/* _MatchUpper. STATE: MATCH 100%, exe 21 / obj 21, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9cf9
unsigned char _MatchUpper(const void *a, ULong lenA, const void *b, ULong lenB)
{
    const char *s1;
    const char *s2;
    unsigned char c;
    long err;

    err = 0;
    s1 = (const char *)a;
    s2 = (const char *)b;
    while ((c = *s2) != 0) {
        if (c >= kFoldLow && c <= kFoldHigh)
            c = c + kUpperFoldDelta;
        if (*s1 != (char)c)
            return 0;
        s1++;
        s2++;
    }
    return 1;
}

/* _HashUnicode. STATE: MATCH 100%, exe 36 / obj 36, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9d23
ULong _HashUnicode(const void *key, ULong len)
{
    const unsigned short *p;
    ULong h;
    ULong i;
    ULong c;
    long err;

    err = 0;
    p = (const unsigned short *)key;
    h = i = 0;
    c = 1;
    while (c != 0) {
        c = p[i];
        i++;
        if ((c & kSurrogateMask) == kSurrogateHigh) {
            c = (c << 10) + p[i] + kSurrogateBias;
            i++;
        }
        if (c != 0)
            h = ((h << 2) ^ c) ^ (h >> kHashRotate);
    }
    return h;
}

/* The other five the create at 0x004fa149 stores: the case-folded UTF-16 pair, the exact
   UTF-16 compare over 0x00505b6f, and the binary-key pair that takes explicit lengths.
   The folded UTF-16 hash folds through atutf162.c's Unicode table, not the byte fold. */

/* _HashUpperUnicode. STATE: MATCH 100%, exe 41 / obj 41, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9d76
ULong _HashUpperUnicode(const void *key, ULong len)
{
    const unsigned short *p;
    ULong h;
    ULong i;
    ULong c;
    long err;

    err = 0;
    p = (const unsigned short *)key;
    h = i = 0;
    c = 1;
    while (c != 0) {
        c = p[i];
        i++;
        if ((c & kSurrogateMask) == kSurrogateHigh) {
            c = (c << 10) + p[i] + kSurrogateBias;
            i++;
        }
        if (c != 0)
            h = (unicode_toupper(c) ^ (h << 2)) ^ (h >> kHashRotate);
    }
    return h;
}

/* _MatchUnicode. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9dd4
unsigned char _MatchUnicode(const void *a, ULong lenA, const void *b, ULong lenB)
{
    return UnicodeCompare((const unsigned short *)a, (const unsigned short *)b) == 0;
}

/* _MatchUpperUnicode. STATE: MATCH 100%, exe 55 / obj 55, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9dec
unsigned char _MatchUpperUnicode(const void *a, ULong lenA, const void *b, ULong lenB)
{
    const unsigned short *s1;
    const unsigned short *s2;
    ULong i;
    ULong j;
    ULong ca;
    ULong cb;
    long err;

    err = 0;
    s1 = (const unsigned short *)a;
    s2 = (const unsigned short *)b;
    i = j = 0;
    ca = 1;
    while (ca != 0) {
        ca = s1[i];
        i++;
        if ((ca & kSurrogateMask) == kSurrogateHigh) {
            ca = (ca << 10) + s1[i] + kSurrogateBias;
            i++;
        }
        cb = s2[j];
        j++;
        if ((cb & kSurrogateMask) == kSurrogateHigh) {
            cb = (cb << 10) + s2[j] + kSurrogateBias;
            j++;
        }
        if (ca != unicode_toupper(cb))
            return 0;
    }
    return 1;
}

/* _HashBinary. STATE: MATCH 100%, exe 20 / obj 20, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9e77
ULong _HashBinary(const void *key, ULong len)
{
    const unsigned char *s;
    ULong count;
    ULong h;
    long err;

    err = 0;
    s = (const unsigned char *)key;
    count = len;
    h = 0;
    while (count != 0) {
        h = (h << 2) ^ (h >> kHashRotate) ^ *s;
        count--;
        s++;
    }
    return h;
}

/* _MatchBinary. STATE: MATCH 100%, exe 25 / obj 25, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9ea1
unsigned char _MatchBinary(const void *a, ULong lenA, const void *b, ULong lenB)
{
    long err;
    const unsigned char *s1;
    const unsigned char *s2;
    ULong n;

    err = 0;
    s1 = (const unsigned char *)a;
    s2 = (const unsigned char *)b;
    n = lenA;
    if (lenA == lenB) {
        while (n != 0 && *s1 == *s2) {
            n--;
            s1++;
            s2++;
        }
    }
    if (n == 0)
        return 1;
    return 0;
}

#define kKeyFold        0x00000001
#define kLargeTable     0x00000002
#define kUnique         0x00000004
#define kKeyWide        0x00000008
#define kKindUTF16      0x01000000
#define kKindBinary     0x02000000
#define kKindMask       0xff000000
#define kFoldOrWide     (kKeyFold | kKeyWide)

#define kSmallBuckets   0x17
#define kLargeBuckets   0x3e5
#define kEntryOverhead  0x1f
#define kEntryHeader    0x20
#define kValueTag       '='

static HashTable *gHashList;                  /* 0x008676b8 every table */

unsigned char gHashKeyClasses[256] = {                /* 0x00578450 key classes */
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x24, 0x24, 0x30, 0x30, 0x24, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x14, 0x18, 0x30, 0x10, 0x11, 0x30, 0x30, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x18, 0x10, 0x18, 0x18, 0x18, 0x10,
    0x10, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x50, 0x50, 0x50, 0x50, 0x02,
    0x50, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x50, 0x50, 0x50, 0x18, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
};

char gHashHexDigits[] = "0123456789ABCDEF";        /* 0x00578550 */

#define kClassBlank     0x04
#define kClassSeparator 0x08
#define kClassEscape    0x20
#define kMaxAscii       0x7f
#define kEscapeMark     '%'

/* _HashDataEncode (percent-escape a text handle in place). STATE: MATCH 100%, exe 84 / obj 84,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9ed5
long _HashDataEncode(void *ref)
{
    long err;
    unsigned char **h;
    char escaped[16];
    unsigned char leading;
    ULong length;
    ULong i;
    ULong c;
    long e;

    err = 0;
    h = (unsigned char **)ref;
    leading = 1;
    length = IndirectGetSize((void **)h);
    for (i = 0; i < length; i++) {
        c = (*h)[i];
        if ((gHashKeyClasses[c] & kClassEscape) == 0)
            continue;
        if (leading != 0 && i + 1 == length && c == kEscapeMark)
            continue;
        if (c < '0' || c > '9')
            leading = 0;
        escaped[0] = kEscapeMark;
        escaped[1] = gHashHexDigits[(c >> 4) & 0xf];
        escaped[2] = gHashHexDigits[c & 0xf];
        c = 3;
        length += c - 1;
        e = IndirectSetSize(ref, length);
        if (e != 0) {
            err = e;
            break;
        }
        MemoryMove(*h + i + c, *h + i + 1, length - i - c);
        MemoryCopy(*h + i, escaped, c);
        i += c - 1;
    }
    return err;
}

/* _HashDataDecode (undo the percent escapes of a text handle in place; %00 becomes a space).
   STATE: MATCH 100%, exe 96 / obj 96, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9fb9
long _HashDataDecode(void *ref)
{
    long err;
    void **h;
    ULong length;
    ULong i;
    ULong d;
    ULong c;
    char *p;
    ULong n;
    long e;

    err = 0;
    h = (void **)ref;
    length = IndirectGetSize(h);
    for (i = 0; i + 2 < length; i++) {
        p = *h;
        if (p[i] != kEscapeMark)
            continue;
        d = p[i + 1];
        if (d >= '0' && d <= '9')
            c = (d - '0') << 4;
        else if (d >= 'a' && d <= 'f')
            c = (d - 'a' + 10) << 4;
        else if (d >= 'A' && d <= 'F')
            c = (d - 'A' + 10) << 4;
        else
            continue;
        d = p[i + 2];
        if (d >= '0' && d <= '9')
            c += d - '0';
        else if (d >= 'a' && d <= 'f')
            c += d - 'a' + 10;
        else if (d >= 'A' && d <= 'F')
            c += d - 'A' + 10;
        else
            continue;
        n = 3;
        if (c == 0)
            c = ' ';
        p[i] = c;
        MemoryMove((char *)*h + i + 1, (char *)*h + i + n, length - i - n);
        length -= n - 1;
        e = IndirectSetSize(ref, length);
        if (e != 0) {
            err = e;
            break;
        }
    }
    return err;
}

/* _HashDataUnicode (the percent escaping of a UTF-16 handle, through UTF-8). STATE: MATCH 100%,
   exe 73 / obj 73, byte-identical. OPEN: none (the UTF-8 handle is never freed, as in the
   exe) */
// FUNCTION: EVNOVA 0x004fa0ad
long _HashDataUnicode(void **h, unsigned char escape)
{
    long err;
    void **narrow;
    ULong length;
    long e;
    long e2;

    err = 0;
    narrow = 0;
    length = count_UTF16_to_UTF8(*h, IndirectGetSize(h));
    narrow = IndirectAllocate(length);
    if (narrow == 0) {
        err = kOutOfMemory;
        goto done;
    }
    convert_UTF16_to_UTF8(*narrow, *h, length, -1, 0);
    if (escape != 0) {
        e = _HashDataEncode(narrow);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else {
        e2 = _HashDataDecode(narrow);
        if (e2 != 0) {
            err = e2;
            goto done;
        }
    }
    length = count_UTF8_to_UTF16(*narrow, IndirectGetSize(narrow));
    e = IndirectSetSize(h, length);
    if (e != 0) {
        err = e;
        goto done;
    }
    convert_UTF8_to_UTF16(*h, *narrow, length, -1, 0);
done:
    return err;
}

unsigned char HashIsValidRef(void *ref);

/* HashCreate. STATE: MATCH 100%, exe 83 / obj 83, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fa149
HashTable *HashCreate(ULong flags)
{
    long err;
    HashTable *t;
    ULong n;

    err = 0;
    t = 0;
    n = 0;
    SystemCheckCallback();
    if ((flags & kKeyWide) != 0)
        flags |= kKeyFold;
    if ((flags & kLargeTable) != 0)
        n = kLargeBuckets;
    else
        n = kSmallBuckets;
    t = MemoryAllocateClear((n - 1) * sizeof(HashEntry *) + sizeof(HashTable));
    if (t == 0) {
        err = kOutOfMemory;
    } else {
        t->next = gHashList;
        gHashList = t;
        t->nBuckets = n;
        t->flags = flags;
        if ((flags & kKindBinary) != 0 && (flags & kFoldOrWide) == 0) {
            t->hash = _HashBinary;
            t->compare = _MatchBinary;
        } else if ((flags & kKindUTF16) != 0) {
            if ((flags & kKeyFold) != 0) {
                t->hash = _HashUpperUnicode;
                t->compare = _MatchUpperUnicode;
            } else {
                t->hash = _HashUnicode;
                t->compare = _MatchUnicode;
            }
        } else if ((flags & kKeyFold) != 0) {
            t->hash = _HashUpper;
            t->compare = _MatchUpper;
        } else {
            t->hash = _Hash;
            t->compare = _Match;
        }
        if ((flags & kKeyWide) != 0)
            t->keyClass = 3;
        else
            t->keyClass = 2;
    }
    if (err != 0 && t != 0) {
        HashDispose(t);
        t = 0;
    }
    return t;
}

/* HashInitialize (a table from a C string of KEY=VALUE pairs). STATE: MATCH 100%, exe 56 /
   obj 56, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fa250
HashTable *HashInitialize(ULong flags, const char *text)
{
    void **copy;
    HashTable *t;
    long err;

    err = 0;
    copy = 0;
    t = 0;
    SystemCheckCallback();
    HashIsValidRef((HashTable *)text);
    if ((flags & kKindUTF16) != 0) {
        err = kBadArgument;
        goto done;
    }
    if ((flags & kKeyWide) != 0)
        flags |= kKeyFold;
    copy = IndirectInitialize(text, StringGetLength((const unsigned char *)text));
    if (copy == 0) {
        err = kOutOfMemory;
        goto done;
    }
    t = HashLoad(flags, copy);
    if (t == 0)
        err = kOutOfMemory;
done:
    if (copy != 0)
        IndirectDeallocate(copy);
    return t;
}

/* HashInitializeUnicode (a table from a UTF-16 string of KEY=VALUE pairs). STATE: MATCH 100%, exe 58
   / obj 58, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fa2cd
HashTable *HashInitializeUnicode(ULong flags, const unsigned short *text)
{
    void **copy;
    HashTable *t;
    long err;

    err = 0;
    copy = 0;
    t = 0;
    SystemCheckCallback();
    HashIsValidRef((HashTable *)text);
    if ((flags & kKindBinary) != 0) {
        err = kBadArgument;
        goto done;
    }
    if ((flags & kKeyWide) != 0)
        flags |= kKeyFold;
    copy = IndirectInitialize(text, UnicodeGetByteCount(text));
    if (copy == 0) {
        err = kOutOfMemory;
        goto done;
    }
    t = HashLoad(flags | kKindUTF16, copy);
    if (t == 0)
        err = kOutOfMemory;
done:
    if (copy != 0)
        IndirectDeallocate(copy);
    return t;
}

#define kParseSkip              0
#define kParseKey               1
#define kParseAfterKey          2
#define kParseSeparator         3
#define kParseAfterSeparator    4
#define kParseValue             5
#define kParseQuoted            6

#define CHAR_IN(wide, c, mask) \
    ((wide) != 0 ? ((c) <= kMaxAscii && (gHashKeyClasses[c] & (mask)) != 0) \
                 : (gHashKeyClasses[c] & (mask)))
#define KEY_CHAR(wide, c, keyClass) \
    ((wide) != 0 ? ((c) > kMaxAscii || (gHashKeyClasses[c] & (keyClass)) != 0) \
                 : (gHashKeyClasses[c] & (keyClass)))
#define SEPARATOR(wide, c, flags) \
    ((wide) != 0 ? (((flags) & kKeyWide) != 0 ? ((c) <= kMaxAscii && (gHashKeyClasses[c] & kClassSeparator) != 0) \
                                              : (c) == '=') \
                 : (((flags) & kKeyWide) != 0 ? (gHashKeyClasses[c] & kClassSeparator) : (c) == '='))
#define IS_CHAR(wide, c, k) ((wide) != 0 ? (c) == (k) : (c) == (k))
#define QUOTE_CHAR(wide, c) \
    ((wide) != 0 ? (((c) == '"' || (c) == '\'') || (c) == '`') \
                 : (((c) == '"' || (c) == '\'') || (c) == '`'))
#define CHAR_SIZE(c)        ((c) <= kBmpMax ? 1 : 2)

/* HashLoad (parse a text handle of KEY=VALUE pairs into a new table; values may be
   quoted and are percent-unescaped; an unexpected character reads as a space). STATE: MATCH
   100%, exe 760 / obj 760, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fa351
HashTable *HashLoad(ULong flags, void **text)
{
    char message[0x100];
    char *key;
    ULong keyStart;
    ULong sep;
    char *data;
    void *t;
    ULong keyLen;
    ULong valueStart;
    ULong pos;
    ULong length;
    ULong quote;
    ULong keyClass;
    void **value;
    unsigned char wasLocked;
    unsigned char bad;
    ULong wide;
    HashTable *table;
    ULong valueLen;
    ULong c;
    long state;
    long err, e;

    err = 0;
    t = 0;
    table = 0;
    keyClass = 0;
    value = 0;
    data = 0;
    wasLocked = 0;
    bad = 0;
    t = HashCreate(flags);
    if (t == 0) {
        err = kOutOfMemory;
        goto done;
    }
    table = t;
    wasLocked = IndirectSetLock(text, 1);
    wide = flags & kKindUTF16;
    if (wide != 0)
        length = UnicodeGetCharCountFromBytes(*text, IndirectGetSize(text));
    else
        length = IndirectGetSize(text);
    data = *text;
    keyClass = table->keyClass;
    keyStart = keyLen = valueStart = valueLen = 0;
    pos = 0;
    state = kParseSkip;
    sep = 0;

    for (; pos <= length; pos += wide != 0 ? CHAR_SIZE(c) : 1) {
        if (wide != 0)
            c = pos < length ? UnicodeGetCharByIndex((const unsigned short *)data, pos) : 0;
        else
            c = pos < length ? (unsigned char)data[pos] : 0;
    dispatch:
        switch (state) {
        case kParseSkip:
            if (c == 0 || CHAR_IN(wide, c, kClassBlank))
                break;
            if (KEY_CHAR(wide, c, keyClass)) {
                keyStart = pos;
                state = kParseKey;
                break;
            }
            bad = 1;
            break;

        case kParseKey:
            if (KEY_CHAR(wide, c, keyClass))
                break;
            keyLen = pos - keyStart;
            if (c == 0)
                break;
            if (SEPARATOR(wide, c, flags)) {
                state = kParseSeparator;
                sep = c;
                break;
            }
            if (CHAR_IN(wide, c, kClassBlank)) {
                state = kParseAfterKey;
                sep = 0;
                break;
            }
            bad = 1;
            break;

        case kParseAfterKey:
            if (CHAR_IN(wide, c, kClassBlank))
                break;
            if (SEPARATOR(wide, c, flags)) {
                state = kParseSeparator;
                sep = c;
                break;
            }
            if (c == 0 || KEY_CHAR(wide, c, keyClass)) {
                if (wide != 0) {
                    err = _HashInsertAppend(t, key = (char *)((keyStart <<= 1) + (ULong)data), keyLen,
                                       0, 0, sep, 0, flags & kKindMask);
                    if (err == kDuplicateKey)
                        err = 0;
                } else {
                    err = _HashInsertAppend(t, data + keyStart, keyLen, 0, 0, sep, 0,
                                       flags & kKindMask);
                    if (err == kDuplicateKey)
                        err = 0;
                }
                require_noerr(err, done);
                keyLen = valueLen = 0;
                keyStart = pos;
                state = kParseKey;
                break;
            }
            bad = 1;
            break;

        case kParseSeparator:
            if (IS_CHAR(wide, c, '='))
                break;
            if (CHAR_IN(wide, c, kClassSeparator)) {
                bad = 1;
                break;
            }
            if (CHAR_IN(wide, c, kClassBlank)) {
                state = kParseAfterSeparator;
                break;
            }
            valueStart = pos;
            quote = c;
            state = QUOTE_CHAR(wide, c) ? kParseQuoted : kParseValue;
            break;

        case kParseAfterSeparator:
            if (c == 0 || CHAR_IN(wide, c, kClassBlank))
                break;
            valueStart = pos;
            quote = c;
            state = QUOTE_CHAR(wide, c) ? kParseQuoted : kParseValue;
            break;

        case kParseValue:
        case kParseQuoted:
            if (state == kParseValue && c != 0 && !CHAR_IN(wide, c, kClassBlank))
                break;
            if (state == kParseQuoted && c != 0 && c != quote)
                break;
            valueLen = pos - valueStart;
            if (wide != 0) {
                if (state == kParseQuoted) {
                    valueStart += CHAR_SIZE(c);
                    valueLen -= CHAR_SIZE(c);
                }
                if (value == 0) {
                    value = IndirectAllocate(0);
                    if (value == 0) {
                        err = kOutOfMemory;
                        goto done;
                    }
                }
                valueLen += valueLen;
                e = IndirectSetSize(value, valueLen);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                /* BUG: the value's character index is used as a byte offset */
                MemoryCopy(*value, data + valueStart, valueLen);
                e = _HashDataUnicode(value, 0);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                IndirectSetLock(value, 1);
                err = _HashInsertAppend(t, data + keyStart * 2, keyLen, *value, IndirectGetSize(value),
                                   sep, 0, flags & kKindMask);
                if (err == kDuplicateKey)
                    err = 0;
            } else {
                if (state == kParseQuoted) {
                    valueStart++;
                    valueLen--;
                }
                if (value == 0) {
                    value = IndirectAllocate(0);
                    if (value == 0) {
                        err = kOutOfMemory;
                        goto done;
                    }
                }
                e = IndirectSetSize(value, valueLen);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                MemoryCopy(*value, data + valueStart, valueLen);
                e = _HashDataDecode(value);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                IndirectSetLock(value, 1);
                err = _HashInsertAppend(t, data + keyStart, keyLen, *value, IndirectGetSize(value),
                                   sep, 0, flags & kKindMask);
                if (err == kDuplicateKey)
                    err = 0;
            }
            IndirectSetLock(value, 0);
            require_noerr(err, done);
            e = IndirectSetSize(value, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            keyLen = valueLen = 0;
            state = kParseSkip;
            break;
        }

        if (bad != 0) {
            StringFormatSafe(message, sizeof(message),
                         c >= kMaxAscii ? "Unexpected character 0x%lX at offset %ld"
                         : (c <= ' ' || c >= kMaxAscii) ? "Unexpected character 0x%2.2lX at offset %ld"
                         : "Unexpected character '%c' at offset %ld", c, pos);
            bad = 0;
            if ((wide != 0 && c != ' ') || c != ' ') {
                c = ' ';
                goto dispatch;
            }
        }
    }

    if (c == 0 && keyLen != 0) {
        if (wide != 0) {
            err = _HashInsertAppend(t, data + keyStart * 2, keyLen, 0, 0, sep, 0,
                               flags & kKindMask);
            if (err == kDuplicateKey)
                err = 0;
        } else {
            err = _HashInsertAppend(t, data + keyStart, keyLen, 0, 0, sep, 0,
                               flags & kKindMask);
            if (err == kDuplicateKey)
                err = 0;
        }
        e = err;
        if (0 != e)
            err = e;
    }

done:
    if (value != 0)
        IndirectDeallocate(value);
    if (wasLocked == 0)
        IndirectSetLock(text, 0);
    if (err != 0 && t != 0) {
        HashDispose(t);
        t = 0;
    }
    return (HashTable *)t;
}

#define kClassPunct     0x10
#define kClassBare      (kClassBlank | kClassPunct)

/* _HashSaveText (a table as KEY=VALUE text: bare, "quoted", 'quoted' or percent-escaped;
   a key with characters outside its class is left out). STATE: MATCH 100%, exe 285 / obj 285,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004faccc
void **_HashSaveText(void *ref)
{
    HashTable *t;
    long err, r;
    char *key;
    char *value;
    HashEntry *e;
    void **text;
    void **escaped;
    ULong keyClass;
    ULong index;
    ULong length;
    ULong n;

    t = (HashTable *)ref;
    err = 0;
    text = 0;
    escaped = 0;
    keyClass = 0;
    text = IndirectAllocate(0);
    if (text == 0) {
        err = kOutOfMemory;
        goto done;
    }
    keyClass = t->keyClass;
    for (index = 0; HashIndex(ref, index, &key, &value) != 0; index++) {
        e = (HashEntry *)(value - offsetof(HashEntry, data));
        for (n = 0; key[n] != 0 && (gHashKeyClasses[(unsigned char)key[n]] & keyClass) != 0; n++)
            ;
        if (n == 0 || key[n] != 0)
            continue;
        r = IndirectAppendData(text, key, n);
        if (r != 0) {
            err = r;
            goto done;
        }
        if (e->tag != 0 && e->tag != kValueTag) {
            r = IndirectAppendData(text, &e->tag, 1);
            if (r != 0) {
                err = r;
                goto done;
            }
        } else if (value[0] == 0) {
            r = IndirectAppendData(text, " ", 1);
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        for (n = 0; n < (length = e->valueLen)
                    && (gHashKeyClasses[(unsigned char)value[n]] & kClassBare) == 0; n++)
            ;
        if (n != 0 && n == length) {
            r = IndirectAppendData(text, "=", 1);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, value, n);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, " ", 1);
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        for (n = 0; n < length && value[n] >= '0' && value[n] <= '9'; n++)
            ;
        if (value[n] == kEscapeMark && n + 1 == length) {
            r = IndirectAppendData(text, "=", 1);
            if (r != 0) {
                err = r;
                goto done;
            }
            n++;
            r = IndirectAppendData(text, value, n);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, " ", 1);
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        for (n = 0; n < length && value[n] != '"'
                    && (value[n] == '\'' || (gHashKeyClasses[(unsigned char)value[n]] & kClassEscape) == 0); n++)
            ;
        if (n == length) {
            r = IndirectAppendData(text, "=\"", 2);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, value, n);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, "\" ", 2);
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        for (n = 0; n < length && value[n] != '\''
                    && (value[n] == '"' || (gHashKeyClasses[(unsigned char)value[n]] & kClassEscape) == 0); n++)
            ;
        if (n == length) {
            r = IndirectAppendData(text, "='", 2);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, value, n);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, "' ", 2);
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        if (escaped == 0) {
            escaped = IndirectAllocate(0);
            if (escaped == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        r = IndirectSetSize(escaped, e->valueLen);
        if (r != 0) {
            err = r;
            goto done;
        }
        MemoryCopy(*escaped, value, e->valueLen);
        r = _HashDataEncode(escaped);
        if (r != 0) {
            err = r;
            goto done;
        }
        r = IndirectAppendData(text, "=\"", 2);
        if (r != 0) {
            err = r;
            goto done;
        }
        r = IndirectAppend(text, escaped);
        if (r != 0) {
            err = r;
            goto done;
        }
        r = IndirectAppendData(text, "\" ", 2);
        if (r != 0) {
            err = r;
            goto done;
        }
    }
done:
    if (escaped != 0)
        IndirectDeallocate(escaped);
    if (err != 0 && text != 0) {
        IndirectDeallocate(text);
        text = 0;
    }
    return text;
}

unsigned short gHashSpaceW[] = { ' ', 0 };               /* 0x005785fc */
unsigned short gHashEqualsW[] = { '=', 0 };               /* 0x00578600 */
unsigned short gHashEqualsQuoteW[] = { '=', '"', 0 };          /* 0x00578604 */
unsigned short gHashQuoteSpaceW[] = { '"', ' ', 0 };          /* 0x0057860c */
unsigned short gHashEqualsAposW[] = { '=', '\'', 0 };         /* 0x00578614 */
unsigned short gHashAposSpaceW[] = { '\'', ' ', 0 };         /* 0x0057861c */

#define NEXT_CHAR(s, n, c) \
    { (c) = (s)[(n)++]; \
      if (((c) & kSurrogateMask) == kSurrogateHigh) \
          (c) = ((c) << 10) + (s)[(n)++] + kSurrogateBias; }

/* _HashSaveUnicode (_HashSaveText for a UTF-16 table). STATE: MATCH 100%, exe 385 / obj 385,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fb01d
void **_HashSaveUnicode(void *ref)
{
    HashTable *t;
    char *key;
    char *value;
    unsigned short tagText[0x100];
    HashEntry *e;
    void **text;
    void **escaped;
    ULong keyClass;
    ULong index;
    ULong flags;
    ULong keyLen;
    ULong length;
    ULong n;
    ULong c;
    long err, r;

    t = (HashTable *)ref;
    err = 0;
    text = 0;
    escaped = 0;
    keyClass = 0;
    text = IndirectAllocate(0);
    if (text == 0) {
        err = kOutOfMemory;
        goto done;
    }
    keyClass = t->keyClass;
    flags = t->flags;
    /* BUG: runs while HashIndexUnicode finds NO entry: a filled table writes nothing */
    for (index = 0; HashIndexUnicode(ref, index, &key, &value) == 0; index++) {
        e = (HashEntry *)(value - offsetof(HashEntry, data));
        n = 0;
        while ((n << 1) < (keyLen = e->keyLen)) {
            NEXT_CHAR((unsigned short *)key, n, c);
            if (c == 0)
                break;
            /* BUG: tests the class of the next character's low byte */
            if (c < kMaxAscii &&
                (gHashKeyClasses[((unsigned char *)key)[(n << 1)]] & keyClass) == 0)
                break;
        }
        if (n == 0 || c != 0)
            continue;
        r = IndirectAppendData(text, key, keyLen);
        if (r != 0) {
            err = r;
            goto done;
        }
        if (e->tag != 0 && e->tag != kValueTag) {
            C2UStringCopySafe(tagText, &e->tag, sizeof(tagText), 1);
            r = IndirectAppendData(text, tagText, UnicodeGetByteCount(tagText));
            if (r != 0) {
                err = r;
                goto done;
            }
        } else if (((unsigned short *)value)[0] == 0) {
            goto space;
        }

        n = 0;
        while ((n << 1) < (length = e->valueLen)) {
            NEXT_CHAR((unsigned short *)value, n, c);
            if (((flags & kKindUTF16) != 0 ? (c >= kMaxAscii || (gHashKeyClasses[c] & kClassBare) != 0)
                                           : (gHashKeyClasses[c] & kClassBare)) != 0)
                break;
        }
        if (n != 0 && length == (n << 1)) {
            r = IndirectAppendData(text, gHashEqualsW, sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, value, e->valueLen);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, gHashSpaceW, sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        n = 0;
        while ((n << 1) < length) {
            NEXT_CHAR((unsigned short *)value, n, c);
            if (c < '0' || c > '9')
                break;
        }
        /* BUG: length counts bytes, n + 1 characters */
        if (n != 0 && c == kEscapeMark && length == n + (c <= kBmpMax ? 1 : 2)) {
            r = IndirectAppendData(text, gHashEqualsW, sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, value, e->valueLen);
            if (r != 0) {
                err = r;
                goto done;
            }
    space:
            r = IndirectAppendData(text, gHashSpaceW, sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        n = 0;
        while ((n << 1) < length) {
            NEXT_CHAR((unsigned short *)value, n, c);
            if (c == '"' || c >= kMaxAscii)
                break;
            if (c != '\'' && (gHashKeyClasses[c] & kClassEscape) != 0)
                break;
        }
        if (length == (n << 1)) {
            r = IndirectAppendData(text, gHashEqualsQuoteW, 2 * sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, value, e->valueLen);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, gHashQuoteSpaceW, 2 * sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        n = 0;
        while ((n << 1) < length) {
            NEXT_CHAR((unsigned short *)value, n, c);
            if (c == '\'' || c >= kMaxAscii)
                break;
            if (c != '"' && (gHashKeyClasses[c] & kClassEscape) != 0)
                break;
        }
        if (n == length) {      /* BUG: characters compared with bytes */
            r = IndirectAppendData(text, gHashEqualsAposW, 2 * sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, value, e->valueLen);
            if (r != 0) {
                err = r;
                goto done;
            }
            r = IndirectAppendData(text, gHashAposSpaceW, 2 * sizeof(unsigned short));
            if (r != 0) {
                err = r;
                goto done;
            }
            continue;
        }

        if (escaped == 0) {
            escaped = IndirectAllocate(0);
            if (escaped == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        r = IndirectSetSize(escaped, e->valueLen);
        if (r != 0) {
            err = r;
            goto done;
        }
        MemoryCopy(*escaped, value, e->valueLen);
        r = _HashDataUnicode(escaped, 1);
        if (r != 0) {
            err = r;
            goto done;
        }
        r = IndirectAppendData(text, gHashEqualsQuoteW, 2 * sizeof(unsigned short));
        if (r != 0) {
            err = r;
            goto done;
        }
        r = IndirectAppend(text, escaped);
        if (r != 0) {
            err = r;
            goto done;
        }
        r = IndirectAppendData(text, gHashQuoteSpaceW, 2 * sizeof(unsigned short));
        if (r != 0) {
            err = r;
            goto done;
        }
    }
done:
    if (escaped != 0)
        IndirectDeallocate(escaped);
    if (err != 0 && text != 0) {
        IndirectDeallocate(text);
        text = 0;
    }
    return text;
}

/* HashSave (a table as text, in its own kind). STATE: MATCH 100%, exe 15 / obj 15,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fb509
void **HashSave(void *ref)
{
    HashTable *t;

    t = (HashTable *)ref;
    if (t != 0 && (t->flags & kKindUTF16) != 0)
        return _HashSaveUnicode(ref);
    return _HashSaveText(ref);
}

/* HashClone (a copy of a table). STATE: MATCH 100%, exe 75 / obj 75, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fb52d
HashTable *HashClone(void *ref)
{
    HashTable *from;
    long err;
    HashTable *t;
    HashEntry *e;

    from = (HashTable *)ref;
    err = 0;
    t = 0;
    e = 0;
    SystemCheckCallback();
    HashIsValidRef(ref);
    t = MemoryAllocateClear(sizeof(HashTable) + (from->nBuckets - 1) * sizeof(HashEntry *));
    if (t == 0) {
        err = kOutOfMemory;
        goto done;
    }
    t->next = gHashList;
    gHashList = t;
    t->nBuckets = from->nBuckets;
    t->flags = from->flags;
    t->hash = from->hash;
    t->compare = from->compare;
    t->keyClass = from->keyClass;
    for (e = from->first; e != 0; e = e->next) {
        err = _HashInsertAppend(t, e->key, e->keyLen, e->data, e->valueLen, e->tag, 0,
                           from->flags & kKindMask);
        if (err != 0)
            break;
    }
done:
    if (err != 0 && t != 0) {
        HashDispose(t);
        t = 0;
    }
    return t;
}

/* HashDispose. STATE: MATCH 100%, exe 44 / obj 44, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fb5e6
void HashDispose(void *ref)
{
    HashTable *t;
    long err;
    HashTable **link;
    HashTable *p;
    HashEntry *e;
    HashEntry *n;
    ULong i;

    t = (HashTable *)ref;
    err = 0;
    SystemCheckCallback();
    HashIsValidRef(ref);
    link = &gHashList;
    while ((p = *link) != 0 && p != t)
        link = &p->next;
    if (p != 0) {
        *link = t->next;
        for (i = 0; i < t->nBuckets; i++) {
            e = t->bucket[i];
            while (e != 0) {
                n = e->chain;
                MemoryDeallocate(e);
                e = n;
            }
        }
        MemoryDeallocate(t);
    }
}

/* HashIsEmpty (a table is empty). STATE: MATCH 100%, exe 16 / obj 16, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fb644
unsigned char HashIsEmpty(void *ref)
{
    HashTable *t;
    long err;

    t = (HashTable *)ref;
    err = 0;
    SystemCheckCallback();
    HashIsValidRef(ref);
    if (t->first != 0)
        return 0;
    return 1;
}

/* HashIsValidRef. STATE: MATCH 100%, exe 19 / obj 19, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fb669
unsigned char HashIsValidRef(void *ref)
{
    HashTable *t;
    long err;
    HashTable *p;

    t = (HashTable *)ref;
    err = 0;
    SystemCheckCallback();
    p = gHashList;
    while (p != 0 && p != t)
        p = p->next;
    return p != 0 ? true : false;
}

/* HashGetFlags (a table's flags). STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fb694
ULong HashGetFlags(void *ref)
{
    HashTable *t;
    long err;

    t = (HashTable *)ref;
    err = 0;
    SystemCheckCallback();
    return t->flags;
}

/* _HashInsertAppend. STATE: MATCH 100%, exe 264 / obj 264, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fb6a9
long _HashInsertAppend(void *ref, const void *key, ULong keyLen,
                  const void *value, ULong valueLen, unsigned char tag,
                  unsigned char front, ULong kind)
{
    HashTable *t;
    ULong charSize;
    long err;
    ULong valueSpace;
    ULong keySpace;
    ULong size;
    HashEntry **link;
    HashEntry *e;
    HashEntry *p;
    ULong keyClass;
    ULong i;
    ULong h;
    int wide;

    t = (HashTable *)ref;
    err = 0;
    e = 0;
    keyClass = 0;
    wide = 0;
    if (kind != 0 && kind != kKindBinary)
        wide = 1;
    if (wide != 0)
        charSize = 2;
    else
        charSize = 1;
    SystemCheckCallback();
    HashIsValidRef(ref);

    if ((t->flags & kKindMask) == kKindBinary &&
        (kind & kKindMask) != kKindBinary) {
        if ((kind & kKindMask) == 0) {
            kind = kKindBinary;
            if (keyLen == kNoLength)
                keyLen = StringGetLength((const unsigned char *)key);
            if (valueLen == kNoLength)
                valueLen = StringGetLength((const unsigned char *)value);
        } else if ((kind & kKindMask) == kKindUTF16) {
            kind = kKindBinary;
            if (keyLen == kNoLength)
                keyLen = UnicodeGetByteCount(key);
            if (valueLen == kNoLength)
                valueLen = UnicodeGetByteCount(value);
        }
    }

    if (front != 0) {
        t->cursorB = 0;
        t->cursorA = 0;
    }

    if ((t->flags & kKindMask) == 0) {
        keyClass = t->keyClass;
        for (i = 0; i < keyLen && ((const char *)key)[i] != 0 &&
                    (gHashKeyClasses[((const unsigned char *)key)[i]] & keyClass) != 0; i++)
            ;
        if (i < keyLen) {
            err = kBadArgument;
            goto done;
        }
    }

    valueSpace = (valueLen + charSize + 3) & ~3;
    size = valueSpace + kEntryOverhead;
    keySpace = keyLen;
    keySpace += charSize;
    size += keySpace;
    e = MemoryAllocate(size);
    if (e == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(e, kEntryHeader);
    e->key = e->data + valueSpace;
    MemoryCopy(e->data, value, valueLen);
    e->valueLen = valueLen;
    i = charSize;
    while (i--)
        e->data[valueLen + i] = 0;

    if ((kind & kKindMask) == kKindUTF16) {
        if ((t->flags & kKeyFold) != 0)
            Unicode2UpperCopySafe((unsigned short *)e->key, key, keySpace, kNoLength);
        else
            MemoryCopy(e->key, key, keyLen);
    } else {
        if ((t->flags & kKeyFold) != 0)
            String2UpperCopySafe(e->key, key, keyLen + 1);
        else
            MemoryCopy(e->key, key, keyLen);
    }
    e->keyLen = keyLen;
    i = charSize;
    while (i--)
        e->key[keyLen + i] = 0;
    e->tag = tag;

    h = t->hash(e->key, e->keyLen) % t->nBuckets;
    if ((t->flags & kUnique) != 0) {
        link = &t->bucket[h];
        while ((p = *link) != 0) {
            if (t->compare(e->key, e->keyLen, p->key, p->keyLen) != 0)
                break;
            link = &(*link)->chain;
        }
        if (*link != 0) {
            err = kDuplicateKey;
            goto done;
        }
    } else if (front == 0) {
        link = &t->bucket[h];
        while (*link != 0)
            link = &(*link)->chain;
    }
    if (front != 0) {
        e->chain = t->bucket[h];
        t->bucket[h] = e;
    } else {
        e->chain = *link;
        *link = e;
    }

    if (t->first != 0 && front != 0) {
        e->next = t->first;
        e->prev = 0;
        t->first->prev = e;
        t->first = e;
    } else if (t->first != 0) {
        e->next = 0;
        e->prev = t->last;
        t->last->next = e;
        t->last = e;
    } else {
        e->next = 0;
        e->prev = 0;
        t->first = e;
        t->last = e;
    }
done:
    return err;
}

/* HashAppend (set key = value at the back of the insertion order; HashInsert's twin,
   which inserts at the front). STATE: MATCH 100%, exe 40 / obj 40, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fb9e4
long HashAppend(HashTable *t, const char *key, const char *value)
{
    int hasValue;
    int tag;
    ULong valueLen;

    hasValue = 0;
    if (value != 0 && *value != 0)
        hasValue = 1;
    if (hasValue)
        tag = kValueTag;
    else
        tag = 0;
    if (value != 0)
        valueLen = StringGetLength((const unsigned char *)value);
    else
        valueLen = 0;
    return _HashInsertAppend(t, key, StringGetLength((const unsigned char *)key), value,
                        valueLen, tag, 0, 0);
}

/* HashAppendData (set a C-string key to a counted value at the back of the insertion
   order; HashInsertData's twin). STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fba37
long HashAppendData(HashTable *t, const char *key, const void *value,
                  ULong valueLen)
{
    int tag;

    if (valueLen != 0)
        tag = kValueTag;
    else
        tag = 0;
    return _HashInsertAppend(t, key, StringGetLength((const unsigned char *)key), value,
                        valueLen, tag, 0, 0);
}

/* HashInsert. STATE: MATCH 100%, exe 40 / obj 40, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fba6e
long HashInsert(HashTable *t, const char *key, const char *value)
{
    int hasValue;
    int tag;
    ULong valueLen;

    hasValue = 0;
    if (value != 0 && *value != 0)
        hasValue = 1;
    if (hasValue)
        tag = kValueTag;
    else
        tag = 0;
    if (value != 0)
        valueLen = StringGetLength((const unsigned char *)value);
    else
        valueLen = 0;
    return _HashInsertAppend(t, key, StringGetLength((const unsigned char *)key), value,
                        valueLen, tag, 1, 0);
}

/* HashInsertData. STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fbac1
long HashInsertData(HashTable *t, const char *key, const void *value,
                  ULong valueLen)
{
    int tag;

    if (valueLen != 0)
        tag = kValueTag;
    else
        tag = 0;
    return _HashInsertAppend(t, key, StringGetLength((const unsigned char *)key), value,
                        valueLen, tag, 1, 0);
}

/* HashAppendUnicodeData (set a UTF-16 key to a counted value at the back of the insertion order).
   STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fbaf8
long HashAppendUnicodeData(HashTable *t, const unsigned short *key, const void *value,
                  ULong valueLen)
{
    int tag;

    if (valueLen != 0)
        tag = kValueTag;
    else
        tag = 0;
    return _HashInsertAppend(t, key, UnicodeGetByteCount(key), value, valueLen, tag, 0, kKindUTF16);
}

/* _HashRemove (remove the entry for a key: 1 when one was removed). STATE: MATCH 100%, exe
   105 / obj 105, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fbb32
unsigned char _HashRemove(void *ref, const void *key, ULong keyLen, ULong kind)
{
    HashTable *t;
    long err;
    HashEntry **link;
    HashEntry *removed;
    HashEntry *e;

    t = (HashTable *)ref;
    err = 0;
    removed = 0;
    SystemCheckCallback();
    HashIsValidRef(ref);
    if ((t->flags & kKindMask) == kKindBinary &&
        (kind & kKindMask) != kKindBinary) {
        if ((kind & kKindMask) == 0) {
            kind = kKindBinary;
            if (keyLen == kNoLength)
                keyLen = StringGetLength((const unsigned char *)key);
        } else if ((kind & kKindMask) == kKindUTF16) {
            kind = kKindBinary;
            if (keyLen == kNoLength)
                keyLen = UnicodeGetByteCount(key);
        }
    }
    t->cursorB = 0;
    t->cursorA = 0;
    link = &t->bucket[t->hash(key, keyLen) % t->nBuckets];
    while ((e = *link) != 0) {
        if (t->compare(e->key, e->keyLen, key, keyLen) != 0)
            break;
        link = &(*link)->chain;
    }
    e = *link;
    if (e != 0) {
        removed = e;
        *link = e->chain;
        if (e->next != 0)
            e->next->prev = e->prev;
        if (e->prev != 0)
            e->prev->next = e->next;
        if (t->first == e)
            t->first = e->next;
        if (t->last == e)
            t->last = e->prev;
        MemoryDeallocate(e);
    }
    return removed != 0 ? true : false;
}

/* HashRemove (remove the entry for a C-string key). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fbc47
unsigned char HashRemove(HashTable *t, const char *key)
{
    return _HashRemove(t, key, kNoLength, 0);
}

/* _HashLookup. STATE: MATCH 100%, exe 79 / obj 79, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fbc5c
HashEntry *_HashLookup(void *ref, const void *key, ULong keyLen, void *after, ULong kind)
{
    HashTable *t;
    long err;
    HashEntry *e;

    t = (HashTable *)ref;
    err = 0;
    e = 0;
    SystemCheckCallback();
    HashIsValidRef(ref);

    if ((t->flags & kKindMask) == kKindBinary &&
        (kind & kKindMask) != kKindBinary) {
        if ((kind & kKindMask) == 0) {
            kind = kKindBinary;
            if (keyLen == kNoLength)
                keyLen = StringGetLength((const unsigned char *)key);
        } else if ((kind & kKindMask) == kKindUTF16) {
            kind = kKindBinary;
            if (keyLen == kNoLength)
                keyLen = UnicodeGetByteCount(key);
        }
    }

    if (after != 0) {
        e = (HashEntry *)((char *)after - offsetof(HashEntry, data));
        t->compare(e->key, e->keyLen, key, keyLen);
        e = e->chain;
    } else {
        e = t->bucket[t->hash(key, keyLen) % t->nBuckets];
    }
    while (e != 0) {
        if (t->compare(e->key, e->keyLen, key, keyLen) != 0)
            break;
        e = e->chain;
    }
    return e;
}

/* HashLookup (the value stored under a C-string key). STATE: MATCH, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fbd31
char *HashLookup(void *table, const char *key, void *after)
{
    HashEntry *e;

    e = _HashLookup(table, key, kNoLength, after, 0);
    return e != 0 ? e->data : 0;
}

/* HashLookupUnicode (the value stored under a UTF-16 key). STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fbd55
char *HashLookupUnicode(void *table, const unsigned short *key, void *after)
{
    HashEntry *e;

    e = _HashLookup(table, key, kNoLength, after, kKindUTF16);
    return e != 0 ? e->data : 0;
}

/* _HashIndex (the index-th entry in insertion order, resuming from the last lookup).
   STATE: MATCH 100%, exe 98 / obj 98, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fbd7c
char *_HashIndex(void *ref, ULong index, char **key, ULong *keyLen, char **value,
                   ULong *valueLen, ULong kind)
{
    HashTable *t;
    long err;
    HashEntry *e;
    ULong i;

    t = (HashTable *)ref;
    err = 0;
    e = 0;
    SystemCheckCallback();
    HashIsValidRef(ref);
    if ((t->flags & kKindMask) == kKindBinary && (kind & kKindMask) != kKindBinary) {
        if ((kind & kKindMask) == 0)
            kind = kKindBinary;
        else if ((kind & kKindMask) == kKindUTF16)
            kind = kKindBinary;
    }

    e = (HashEntry *)t->cursorB;
    if (e == 0 || (i = t->cursorA) >= index) {
        e = t->first;
        i = 0;
    }
    while (i < index && e != 0) {
        i++;
        e = e->next;
    }

    if (e != 0) {
        if (key != 0)
            *key = e->key;
        if (keyLen != 0)
            *keyLen = e->keyLen;
        if (value != 0)
            *value = e->data;
        if (valueLen != 0)
            *keyLen = e->valueLen;      /* BUG: stores through keyLen, not valueLen */
        t->cursorB = (long)e;
        t->cursorA = index;
    } else {
        if (key != 0)
            *key = 0;
        if (keyLen != 0)
            *keyLen = 0;
        if (value != 0)
            *value = 0;
        if (valueLen != 0)
            *valueLen = 0;
        t->cursorB = 0;
        t->cursorA = 0;
    }
    return e != 0 ? e->data : 0;
}

/* HashIndex (the index-th key and value). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fbe7d
char *HashIndex(HashTable *t, ULong index, char **key, char **value)
{
    return _HashIndex(t, index, key, 0, value, 0, 0);
}

/* HashIndexUnicode (the same, asking for UTF-16). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fbe9c
char *HashIndexUnicode(HashTable *t, ULong index, char **key, char **value)
{
    return _HashIndex(t, index, key, 0, value, 0, kKindUTF16);
}
