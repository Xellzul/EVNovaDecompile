// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atcharset.c -- the platform layer's character-set registry, the last thing `Platform_Open' initialises, and the UTF-16 helpers that read its tables. Notes: Research/notes/atcharset.md */

#include "burgerplat.h"
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "atfold.h"

struct HashTable;

typedef struct CharsetInfo {
    char name[256];                              /* +0x000 */
    ULong tag;                                   /* +0x100 */
    ULong reserved;                              /* +0x104 */
    void *table;                                 /* +0x108 */
} CharsetInfo;

#define kNameTableFlags     0x00000005           /* fold, unique */
#define kEntityTableFlags   0x02000006           /* binary, large, unique */
#define kTagMac             'MAC '
#define kTagWin             'WIN '
#define kTagIso             'ISO '
#define kTagUtf8            'UTF8'
#define kTagUtf16           'UT16'
#define kTagUtf32           'UT32'
#define kBomUtf8            0xefbbbfff
#define kBomUtf16Big        0xfeffffff
#define kBomUtf16Little     0xfffeffff
#define kBomUtf32Big        0x0000feff
#define kBomUtf32Little     0xfffe0000
#define kBomUtf32Odd        0x0000fffe
#define kBomUtf32Even       0xfeff0000
#define kKindUnknown        -1
#define kKindAscii          0
#define kKindMacText        1
#define kKindText           2
#define kKindUnicode        3
#define kWrongCharset       -7009
#define kSubstitute         0x1a
#define kCharRows           0x99
#define kHasMac             0x01
#define kHasWin             0x02
#define kHasIso             0x04
#define kWinHasMac          0x40
#define kSetBits            0xbf                 /* every flag but kWinHasMac */
#define kNonAscii           0xff80
#define kHighStart          0xa0


static CharEntry gTextMappingsMac[256];              /* 0x0086aab4 MacRoman */
static CharEntry gTextMappingsWin[256];              /* 0x00869eb4 Windows-1252 */
static CharEntry gTextMappingsIso[256];              /* 0x008692b4 ISO-8859-1 */
static unsigned char gTextMappingsFlags[256];          /* 0x008691b4 per-code flags */
static WinEntry gUnicodeMappings[256];               /* 0x00868db4 */
/* Placement only: no reader in the exe. */
static unsigned char sCharsetPad[0x1008];       /* 0x00867dac */
static ULong DAT_00867da8;                       /* 0x00867da8 */
static ULong gUnicodeMappingCount;                       /* 0x00867da4 win count */
static unsigned char gTextMappingsValid;               /* 0x00867da0 tables built */
static struct HashTable *gTextEncodingsHashRef;           /* 0x00867d9c names */
static struct HashTable *gTextMappingsHashRef;           /* 0x00867d98 entities */

/* 0x00578c4c */
static const CharEntry kCharEntries[kCharRows] = {
    { 0x0000, 0x00, 0x00, 0x00, "" },
    { 0x0026, 0x26, 0x26, 0x26, "amp" },
    { 0x003c, 0x3c, 0x3c, 0x3c, "lt" },
    { 0x003e, 0x3e, 0x3e, 0x3e, "gt" },
    { 0x0022, 0x22, 0x22, 0x22, "quot" },
    { 0x20ac, 0x00, 0x80, 0xdb, "euro" },
    { 0x201a, 0x00, 0x82, 0xe2, "sbquo" },
    { 0x0192, 0x00, 0x83, 0xc4, "fnof" },
    { 0x201e, 0x00, 0x84, 0xe3, "bdquo" },
    { 0x2026, 0x00, 0x85, 0xc9, "hellip" },
    { 0x2020, 0x00, 0x86, 0xa0, "dagger" },
    { 0x2021, 0x00, 0x87, 0xe0, "Dagger" },
    { 0x02c6, 0x00, 0x88, 0xf6, "circ" },
    { 0x2030, 0x00, 0x89, 0xe4, "permil" },
    { 0x0160, 0x00, 0x8a, 0x00, "Scaron" },
    { 0x2039, 0x00, 0x8b, 0xdc, "lsaquo" },
    { 0x0152, 0x00, 0x8c, 0xce, "OElig" },
    { 0x017d, 0x00, 0x8e, 0x00, "Zcaron" },
    { 0x2018, 0x00, 0x91, 0xd4, "lsquo" },
    { 0x2019, 0x00, 0x92, 0xd5, "rsquo" },
    { 0x201c, 0x00, 0x93, 0xd2, "ldquo" },
    { 0x201d, 0x00, 0x94, 0xd3, "rdquo" },
    { 0x2022, 0x00, 0x95, 0xa5, "bull" },
    { 0x2013, 0x00, 0x96, 0xd0, "ndash" },
    { 0x2014, 0x00, 0x97, 0xd1, "mdash" },
    { 0x02dc, 0x00, 0x98, 0xf7, "tilde" },
    { 0x2122, 0x00, 0x99, 0xaa, "trade" },
    { 0x0161, 0x00, 0x9a, 0x00, "scaron" },
    { 0x203a, 0x00, 0x9b, 0xdd, "rsaquo" },
    { 0x0153, 0x00, 0x9c, 0xcf, "oelig" },
    { 0x017e, 0x00, 0x00, 0x00, "zcaron" },
    { 0x0178, 0x00, 0x9f, 0xd9, "Yuml" },
    { 0x00a0, 0xa0, 0xa0, 0xca, "nbsp" },
    { 0x00a1, 0xa1, 0xa1, 0xc1, "iexcl" },
    { 0x00a2, 0xa2, 0xa2, 0xa2, "cent" },
    { 0x00a3, 0xa3, 0xa3, 0xa3, "pound" },
    { 0x00a4, 0xa4, 0xa4, 0x00, "curren" },
    { 0x00a5, 0xa5, 0xa5, 0xb4, "yen" },
    { 0x00a6, 0xa6, 0xa6, 0x00, "brvbar" },
    { 0x00a7, 0xa7, 0xa7, 0xa4, "sect" },
    { 0x00a8, 0xa8, 0xa8, 0xac, "uml" },
    { 0x00a9, 0xa9, 0xa9, 0xa9, "copy" },
    { 0x00aa, 0xaa, 0xaa, 0xbb, "ordf" },
    { 0x00ab, 0xab, 0xab, 0xc7, "laquo" },
    { 0x00ac, 0xac, 0xac, 0xc2, "not" },
    { 0x00ad, 0xad, 0xad, 0x00, "shy" },
    { 0x00ae, 0xae, 0xae, 0xa8, "reg" },
    { 0x00af, 0xaf, 0xaf, 0xf8, "macr" },
    { 0x00b0, 0xb0, 0xb0, 0xa1, "deg" },
    { 0x00b1, 0xb1, 0xb1, 0xb1, "plusmn" },
    { 0x00b2, 0xb2, 0xb2, 0x00, "sup2" },
    { 0x00b3, 0xb3, 0xb3, 0x00, "sup3" },
    { 0x00b4, 0xb4, 0xb4, 0xab, "acute" },
    { 0x00b5, 0xb5, 0xb5, 0xb5, "micro" },
    { 0x00b6, 0xb6, 0xb6, 0xa6, "para" },
    { 0x00b7, 0xb7, 0xb7, 0xe1, "middot" },
    { 0x00b8, 0xb8, 0xb8, 0xfc, "cedil" },
    { 0x00b9, 0xb9, 0xb9, 0x00, "sup1" },
    { 0x00ba, 0xba, 0xba, 0xbc, "ordm" },
    { 0x00bb, 0xbb, 0xbb, 0xc8, "raquo" },
    { 0x00bc, 0xbc, 0xbc, 0x00, "frac14" },
    { 0x00bd, 0xbd, 0xbd, 0x00, "frac12" },
    { 0x00be, 0xbe, 0xbe, 0x00, "frac34" },
    { 0x00bf, 0xbf, 0xbf, 0xc0, "iquest" },
    { 0x00c0, 0xc0, 0xc0, 0x00, "Agrave" },
    { 0x00c1, 0xc1, 0xc1, 0xe7, "Aacute" },
    { 0x00c2, 0xc2, 0xc2, 0xe5, "Acirc" },
    { 0x00c3, 0xc3, 0xc3, 0xcc, "Atilde" },
    { 0x00c4, 0xc4, 0xc4, 0x80, "Auml" },
    { 0x00c5, 0xc5, 0xc5, 0x81, "Aring" },
    { 0x00c6, 0xc6, 0xc6, 0xae, "AElig" },
    { 0x00c7, 0xc7, 0xc7, 0x82, "Ccedil" },
    { 0x00c8, 0xc8, 0xc8, 0xe9, "Egrave" },
    { 0x00c9, 0xc9, 0xc9, 0x83, "Eacute" },
    { 0x00ca, 0xca, 0xca, 0xe6, "Ecirc" },
    { 0x00cb, 0xcb, 0xcb, 0xe8, "Euml" },
    { 0x00cc, 0xcc, 0xcc, 0xed, "Igrave" },
    { 0x00cd, 0xcd, 0xcd, 0xea, "Iacute" },
    { 0x00ce, 0xce, 0xce, 0xeb, "Icirc" },
    { 0x00cf, 0xcf, 0xcf, 0xec, "Iuml" },
    { 0x00d0, 0xd0, 0xd0, 0x00, "ETH" },
    { 0x00d1, 0xd1, 0xd1, 0x84, "Ntilde" },
    { 0x00d2, 0xd2, 0xd2, 0xf1, "Ograve" },
    { 0x00d3, 0xd3, 0xd3, 0xee, "Oacute" },
    { 0x00d4, 0xd4, 0xd4, 0xef, "Ocirc" },
    { 0x00d5, 0xd5, 0xd5, 0xcd, "Otilde" },
    { 0x00d6, 0xd6, 0xd6, 0x85, "Ouml" },
    { 0x00d7, 0xd7, 0xd7, 0x00, "times" },
    { 0x00d8, 0xd8, 0xd8, 0xaf, "Oslash" },
    { 0x00d9, 0xd9, 0xd9, 0xf4, "Ugrave" },
    { 0x00da, 0xda, 0xda, 0xf2, "Uacute" },
    { 0x00db, 0xdb, 0xdb, 0xf3, "Ucirc" },
    { 0x00dc, 0xdc, 0xdc, 0x86, "Uuml" },
    { 0x00dd, 0xdd, 0xdd, 0x00, "Yacute" },
    { 0x00de, 0xde, 0xde, 0x00, "THORN" },
    { 0x00df, 0xdf, 0xdf, 0xa7, "szlig" },
    { 0x00e0, 0xe0, 0xe0, 0x88, "agrave" },
    { 0x00e1, 0xe1, 0xe1, 0x87, "aacute" },
    { 0x00e2, 0xe2, 0xe2, 0x89, "acirc" },
    { 0x00e3, 0xe3, 0xe3, 0x8b, "atilde" },
    { 0x00e4, 0xe4, 0xe4, 0x8a, "auml" },
    { 0x00e5, 0xe5, 0xe5, 0x8c, "aring" },
    { 0x00e6, 0xe6, 0xe6, 0xbe, "aelig" },
    { 0x00e7, 0xe7, 0xe7, 0x8d, "ccedil" },
    { 0x00e8, 0xe8, 0xe8, 0x8f, "egrave" },
    { 0x00e9, 0xe9, 0xe9, 0x8e, "eacute" },
    { 0x00ea, 0xea, 0xea, 0x90, "ecirc" },
    { 0x00eb, 0xeb, 0xeb, 0x91, "euml" },
    { 0x00ec, 0xec, 0xec, 0x93, "igrave" },
    { 0x00ed, 0xed, 0xed, 0x92, "iacute" },
    { 0x00ee, 0xee, 0xee, 0x94, "icirc" },
    { 0x00ef, 0xef, 0xef, 0x95, "iuml" },
    { 0x00f0, 0xf0, 0xf0, 0x00, "eth" },
    { 0x00f1, 0xf1, 0xf1, 0x96, "ntilde" },
    { 0x00f2, 0xf2, 0xf2, 0x98, "ograve" },
    { 0x00f3, 0xf3, 0xf3, 0x97, "oacute" },
    { 0x00f4, 0xf4, 0xf4, 0x99, "ocirc" },
    { 0x00f5, 0xf5, 0xf5, 0x9b, "otilde" },
    { 0x00f6, 0xf6, 0xf6, 0x9a, "ouml" },
    { 0x00f7, 0xf7, 0xf7, 0xd6, "divide" },
    { 0x00f8, 0xf8, 0xf8, 0xbf, "oslash" },
    { 0x00f9, 0xf9, 0xf9, 0x9d, "ugrave" },
    { 0x00fa, 0xfa, 0xfa, 0x9c, "uacute" },
    { 0x00fb, 0xfb, 0xfb, 0x9e, "ucirc" },
    { 0x00fc, 0xfc, 0xfc, 0x9f, "uuml" },
    { 0x00fd, 0xfd, 0xfd, 0x00, "yacute" },
    { 0x00fe, 0xfe, 0xfe, 0x00, "thorn" },
    { 0x00ff, 0xff, 0xff, 0xd8, "yuml" },
    { 0x2260, 0x00, 0x00, 0xad, "ne" },
    { 0x221e, 0x00, 0x00, 0xb0, "infin" },
    { 0x2264, 0x00, 0x00, 0xb2, "le" },
    { 0x2265, 0x00, 0x00, 0xb3, "ge" },
    { 0x2202, 0x00, 0x00, 0xb6, "part" },
    { 0x2211, 0x00, 0x00, 0xb7, "sum" },
    { 0x220f, 0x00, 0x00, 0xb8, "prod" },
    { 0x03c0, 0x00, 0x00, 0xb9, "pi" },
    { 0x222b, 0x00, 0x00, 0xba, "int" },
    { 0x03a9, 0x00, 0x00, 0xbd, "Omega" },
    { 0x221a, 0x00, 0x00, 0xc3, "radic" },
    { 0x2248, 0x00, 0x00, 0xc5, "asymp" },
    { 0x2206, 0x00, 0x00, 0xc6, "" },
    { 0x25ca, 0x00, 0x00, 0xd7, "loz" },
    { 0x2044, 0x00, 0x00, 0xda, "frasl" },
    { 0x2044, 0x00, 0x00, 0xde, "" },
    { 0x2044, 0x00, 0x00, 0xdf, "" },
    { 0xf8ff, 0x00, 0x00, 0xf0, "" },
    { 0x0131, 0x00, 0x00, 0xf5, "" },
    { 0x02d8, 0x00, 0x00, 0xf9, "" },
    { 0x02d9, 0x00, 0x00, 0xfa, "" },
    { 0x02da, 0x00, 0x00, 0xfb, "" },
    { 0x02dd, 0x00, 0x00, 0xfd, "" },
    { 0x02db, 0x00, 0x00, 0xfe, "" },
    { 0x02c7, 0x00, 0x00, 0xff, "" },
};

static const unsigned char DAT_00579378[4] = { 0 };   /* 0x00579378, unreferenced */
char gTextMappingSubstitute = kSubstitute;                 /* 0x0057937c */

void _EncodingPostflight(void);
long _EncodingPreflightMappings(struct HashTable *table);

/* _EncodingPreflight. STATE: MATCH 100%, exe 194 / obj 194, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005047d3
long _EncodingPreflight(void)
{
    CharsetInfo info;
    long err;
    long e;

    err = 0;
    if (gTextEncodingsHashRef == 0) {
        gTextEncodingsHashRef = HashCreate(kNameTableFlags);
        if (gTextEncodingsHashRef == 0) {
            err = kOutOfMemory;
            goto done;
        }
        info.table = gTextMappingsIso;
        info.tag = 0;
        StringCopy(info.name, "Unknown");
        e = HashInsertData(gTextEncodingsHashRef, "US_ASCII", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = HashInsertData(gTextEncodingsHashRef, "ASCII", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }

        info.table = gTextMappingsMac;
        info.tag = kTagMac;
        StringCopy(info.name, "MacRoman");
        e = HashInsertData(gTextEncodingsHashRef, "MacRoman", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }

        info.table = gTextMappingsWin;
        info.tag = kTagWin;
        StringCopy(info.name, "Windows-1252");
        e = HashInsertData(gTextEncodingsHashRef, "Windows_1252", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = HashInsertData(gTextEncodingsHashRef, "MS_ANSI", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = HashInsertData(gTextEncodingsHashRef, "CP1252", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }

        info.table = gTextMappingsIso;
        info.tag = kTagIso;
        StringCopy(info.name, "ISO-8859-1");
        e = HashInsertData(gTextEncodingsHashRef, "ISO_8859_1", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = HashInsertData(gTextEncodingsHashRef, "Latin_1", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = HashInsertData(gTextEncodingsHashRef, "Latin1", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = HashInsertData(gTextEncodingsHashRef, "CP819", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }

        info.table = 0;
        info.tag = kTagUtf8;
        StringCopy(info.name, "UTF-8");
        e = HashInsertData(gTextEncodingsHashRef, "UTF_8", &info, sizeof(info));
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (gTextMappingsHashRef == 0) {
        gTextMappingsHashRef = HashCreate(kEntityTableFlags);
        if (gTextMappingsHashRef == 0) {
            err = kOutOfMemory;
            goto done;
        }
        _EncodingPreflightMappings(gTextMappingsHashRef);
    }
    SystemSetQuitProc(_EncodingPostflight);
done:
    if (err != 0 && gTextEncodingsHashRef != 0) {
        HashDispose(gTextEncodingsHashRef);
        gTextEncodingsHashRef = 0;
    }
    if (err != 0 && gTextMappingsHashRef != 0) {
        HashDispose(gTextMappingsHashRef);
        gTextMappingsHashRef = 0;
        gTextMappingsValid = 0;
    }
    return err;
}

/* _EncodingPostflight. STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504ad9
void _EncodingPostflight(void)
{
    if (gTextEncodingsHashRef != 0) {
        HashDispose(gTextEncodingsHashRef);
        gTextEncodingsHashRef = 0;
    }
    if (gTextMappingsHashRef != 0) {
        HashDispose(gTextMappingsHashRef);
        gTextMappingsHashRef = 0;
    }
    gTextMappingSubstitute = kSubstitute;
    gTextMappingsValid = 0;
}

/* _EncodingPreflightMappings. STATE: MATCH 100%, exe 231 / obj 231, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504b20
long _EncodingPreflightMappings(struct HashTable *table)
{
    long err, e;
    CharEntry entry;
    char name[256];
    unsigned char index;
    const CharEntry *row;
    ULong i;
    ULong nWin;
    ULong rows;

    err = 0;
    MemoryClear(&entry, sizeof(entry));
    MemoryClear(gUnicodeMappings, sizeof(gUnicodeMappings));
    gUnicodeMappingCount = 0;

    entry.unicode = (short)gTextMappingSubstitute;
    entry.iso = gTextMappingSubstitute;
    entry.win = gTextMappingSubstitute;
    entry.mac = gTextMappingSubstitute;
    entry.entity[0] = 0;
    gTextMappingsIso[0] = entry;
    gTextMappingsWin[0] = entry;
    gTextMappingsMac[0] = entry;
    gTextMappingsFlags[0] = 0;
    for (i = 1; i < 0x80; i = i + 1) {
        entry.unicode = i;
        entry.iso = i;
        entry.win = i;
        entry.mac = i;
        entry.entity[0] = 0;
        gTextMappingsIso[i] = entry;
        gTextMappingsWin[i] = entry;
        gTextMappingsMac[i] = entry;
        gTextMappingsFlags[i] = 0xff;
    }
    for (i = 0x80; i < 0x100; i = i + 1) {
        entry.unicode = (short)gTextMappingSubstitute;
        entry.iso = gTextMappingSubstitute;
        entry.win = gTextMappingSubstitute;
        entry.mac = gTextMappingSubstitute;
        entry.entity[0] = 0;
        gTextMappingsIso[i] = entry;
        gTextMappingsWin[i] = entry;
        gTextMappingsMac[i] = entry;
        gTextMappingsFlags[i] = 0;
    }

    rows = kCharRows;
    for (i = nWin = 0; i < rows; i = i + 1) {
        row = &kCharEntries[i];
        entry = *row;
        if (entry.unicode == 0)
            entry.unicode = gTextMappingSubstitute;
        if (entry.iso == 0)
            entry.iso = gTextMappingSubstitute;
        if (entry.win == 0)
            entry.win = gTextMappingSubstitute;
        if (entry.mac == 0)
            entry.mac = gTextMappingSubstitute;
        if (row->iso != 0) {
            gTextMappingsIso[row->iso] = entry;
            gTextMappingsFlags[row->mac] |= kHasIso;
        }
        if (row->win != 0) {
            gTextMappingsWin[row->win] = entry;
            gTextMappingsFlags[row->mac] |= kHasWin;
        }
        if (row->mac != 0) {
            gTextMappingsMac[row->mac] = entry;
            gTextMappingsFlags[row->mac] |= kHasMac;
        }
        if ((row->unicode & kNonAscii) != 0 && nWin < 0x100) {
            index = row->win;
            if (index != 0) {
                gUnicodeMappings[nWin].unicode = row->unicode;
                gUnicodeMappings[nWin].win = index;
                gUnicodeMappingCount = gUnicodeMappingCount + 1;
                nWin = nWin + 1;
            }
        }
        if (table != 0 && row->entity[0] != 0) {
            index = i;
            StringFormatSafe(name, sizeof(name), "&%s;", row->entity);
            e = HashInsertData(table, name, &index, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    for (i = kHighStart; i < 0x100; i = i + 1) {
        if (gTextMappingsWin[i].mac != 0)
            gTextMappingsFlags[i] |= kWinHasMac;
    }
    gTextMappingsValid = 1;
done:
    return err;
}

/* _EncodingLookupByName (a character set by name, '-' read as '_'). STATE: MATCH 100%, exe 29 / obj 29, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00504e34
CharsetInfo *_EncodingLookupByName(const char *name)
{
    char key[256];
    char *p;
    long err;

    err = 0;
    p = 0;
    String2UpperCopySafe(key, name, sizeof(key));
    while ((p = StringFindDelimiter(key, "-")) != 0)
        *p = '_';
    return (CharsetInfo *)HashLookup(gTextEncodingsHashRef, key, 0);
}

/* _EncodingLookupTable (the per-code table of a tag). STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00504e91
CharEntry *_EncodingLookupTable(ULong tag)
{
    CharEntry *table;
    long err;

    err = 0;
    table = 0;
    switch (tag) {
    case 0:
        break;
    case kTagMac:
        table = gTextMappingsMac;
        break;
    case kTagIso:
        table = gTextMappingsIso;
        break;
    case kTagWin:
        break;
    }
    return table;
}

/* EncodingLookupTag (an entity name as its Windows-1252 byte, 0 when unknown). STATE:
   MATCH 100%, exe 64 / obj 64, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504ec8
char EncodingLookupTag(const char *entity)
{
    char key[256];
    unsigned char *index;
    unsigned char c;
    ULong code;
    long err;
    const CharEntry *row;

    err = 0;
    index = 0;
    row = 0;
    c = 0;
    StringCopySafe(key, entity, sizeof(key));
    index = (unsigned char *)HashLookup(gTextMappingsHashRef, key, 0);
    if (index == 0 && key[1] >= 'A' && key[1] <= 'Z')
        index = (unsigned char *)HashLookup(gTextMappingsHashRef, String2Upper(key), 0);
    if (index == 0)
        index = (unsigned char *)HashLookup(gTextMappingsHashRef, String2Lower(key), 0);
    if (index != 0) {
        row = &kCharEntries[*index];
        if (row->win != 0)
            code = row->win;
        else
            code = gTextMappingSubstitute;
        c = code;
    }
    return c;
}

/* _IndirectGetUnicodeEncoding (the Unicode form a byte-order mark or a leading '<' shows; 0 when none).
   STATE: MATCH 100%, exe 173 / obj 173, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504f8d
const char *_IndirectGetUnicodeEncoding(void **handle, ULong *tag, ULong *bom)
{
    long err;
    const char *name;
    ULong t;
    ULong mark;
    ULong n;
    union {
        ULong word;
        unsigned char bytes[4];
    } head;

    err = 0;
    name = 0;
    t = kNoLength;
    mark = 0;
    head.word = DAT_00867da8;
    if (tag != 0)
        *tag = kNoLength;
    if (bom != 0)
        *bom = 0;
    n = IndirectGetSize(handle);
    if (n >= sizeof(head))
        n = sizeof(head);
    MemoryCopy(head.bytes, *handle, n);
    if (head.bytes[0] == 0xef && head.bytes[1] == 0xbb && head.bytes[2] == 0xbf) {
        name = "UTF-8";
        t = kTagUtf8;
        mark = kBomUtf8;
    } else if (head.bytes[0] == 0xfe && head.bytes[1] == 0xff) {
        name = "UTF-16";
        t = kTagUtf16;
        mark = kBomUtf16Big;
    } else if (head.bytes[0] == 0xff && head.bytes[1] == 0xfe) {
        name = "UTF-16";
        t = kTagUtf16;
        mark = kBomUtf16Little;
    } else if (head.bytes[0] == 0 && head.bytes[1] == 0 && head.bytes[2] == 0xfe && head.bytes[3] == 0xff) {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Big;
    } else if (head.bytes[0] == 0xff && head.bytes[1] == 0xfe && head.bytes[2] == 0 && head.bytes[3] == 0) {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Little;
    } else if (head.bytes[0] == 0 && head.bytes[1] == 0 && head.bytes[2] == 0xff && head.bytes[3] == 0xfe) {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Odd;
    } else if (head.bytes[0] == 0xfe && head.bytes[1] == 0xff && head.bytes[2] == 0 && head.bytes[3] == 0) {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Even;
    } else if (head.bytes[0] == 0 && head.bytes[1] == '<' && (head.bytes[2] != 0 || head.bytes[3] != 0)) {
        name = "UTF-16";
        t = kTagUtf16;
        mark = kBomUtf16Big;
    } else if (head.bytes[0] == '<' && head.bytes[1] == 0 && (head.bytes[2] != 0 || head.bytes[3] != 0)) {
        name = "UTF-16";
        t = kTagUtf16;
        mark = kBomUtf16Little;
    } else if (head.bytes[0] == 0 && head.bytes[1] == 0 && head.bytes[2] == 0 && head.bytes[3] == '<') {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Big;
    } else if (head.bytes[0] == '<' && head.bytes[1] == 0 && head.bytes[2] == 0 && head.bytes[3] == 0) {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Little;
    } else if (head.bytes[0] == 0 && head.bytes[1] == 0 && head.bytes[2] == '<' && head.bytes[3] == 0) {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Odd;
    } else if (head.bytes[0] == 0 && head.bytes[1] == '<' && head.bytes[2] == 0 && head.bytes[3] == 0) {
        name = "UTF-32";
        t = kTagUtf32;
        mark = kBomUtf32Even;
    }
    if (tag != 0)
        *tag = t;
    if (bom != 0)
        *bom = mark;
    return name;
}

/* _IndirectGetEncoding (the character set of a text handle: a byte-order mark, an `encoding='
   declaration on the first line, else a guess from the high bytes). STATE: MATCH 100%, exe 337
   / obj 337, byte-identical yes. OPEN: none (`src' is assigned inside the copy's arguments,
   evaluated right to left: notes) */
// FUNCTION: EVNOVA 0x005051f9
ULong _IndirectGetEncoding(void **handle, char *nameOut, ULong *kindOut, ULong *bomOut)
{
    long err;
    const char *name;
    ULong tag;
    const char *guess;
    long kind;
    ULong bom;
    ULong size;
    ULong i;
    ULong start;
    char *src;
    ULong n;
    char *line;
    char *found;
    char c;
    char ch;
    unsigned char flags;
    unsigned char set;
    ULong bits;
    const unsigned char *s;
    char label[256];
    char key[256];

    err = 0;
    name = 0;
    tag = kNoLength;
    kind = kKindUnknown;
    bom = 0;
    if (nameOut != 0)
        *nameOut = 0;
    if (kindOut != 0)
        *kindOut = kKindUnknown;
    if (bomOut != 0)
        *bomOut = 0;
    size = IndirectGetSize(handle);
    if (size == 0) {
        name = "US-ASCII";
        tag = 0;
        kind = kKindAscii;
    }
    if (tag == kNoLength) {
        name = _IndirectGetUnicodeEncoding(handle, &tag, &bom);
        if (name != 0)
            kind = kKindUnicode;
    }
    if (tag == kNoLength && size >= 4) {
        line = 0;
        for (i = 0; i < size; i = i + 1) {
            ch = ((char *)*handle)[i];
            if (ch == '\r' || ch == '\n')
                break;
        }
        line = MemoryAllocateClear(i + 1);
        if (line == 0) {
            err = kOutOfMemory;
            goto done;
        }
        MemoryCopy(line, *handle, i);
        found = StringFindString(String2Upper(line), "ENCODING");
        if (found != 0) {
            start = found - line + StringGetLength((const unsigned char *)"ENCODING");
            while (line[start] == ' ' || line[start] == '\t')
                start = start + 1;
        }
        if (found != 0 && line[start++] == '=') {
            while (line[start] == ' ' || line[start] == '\t')
                start = start + 1;
            if (line[start] == '\'' || line[start] == '"')
                start = start + 1;
            /* BUG: == where >= was meant, so only '-' of the punctuation run is accepted */
            for (n = start; ((c = line[n]) >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                              || (c >= '0' && c <= '9') || (c == '-' && c <= '_'); n = n + 1)
                ;
            if (n - start < sizeof(label)) {
                n -= start;
                n++;
            } else
                n = sizeof(label);
            StringCopySafe(label, src = (char *)*handle + start, n);
            String2UpperCopySafe(key, label, sizeof(key));
            while ((found = StringFindDelimiter(key, "-")) != 0)
                *found = '_';
            name = HashLookup(gTextEncodingsHashRef, key, 0);
        }
        MemoryDeallocate(line);
        if (name != 0) {
            if (StringCompare(name, "US-ASCII") == 0) {
                tag = 0;
                kind = kKindAscii;
            } else if (StringCompare(name, "UTF-8") == 0) {
                tag = kTagUtf8;
                kind = kKindUnicode;
            } else if (StringCompare(name, "ISO-8859-1") == 0) {
                tag = kTagIso;
            } else if (StringCompare(name, "Windows-1252") == 0) {
                tag = kTagWin;
            } else if (StringCompare(name, "MacRoman") == 0) {
                tag = kTagMac;
            }
        } else {
            /* BUG: label is uninitialised when no declaration was found */
            name = label;
        }
    }
    if (kindOut != 0 && kind == kKindUnknown) {
        guess = 0;
        flags = 0xff;
        for (i = 0, s = (const unsigned char *)*handle; i < size && flags != 0;
             i = i + 1, s = s + 1) {
            if (*s == 0)
                flags = 0;
            else if ((*s & 0x80) != 0)
                flags &= gTextMappingsFlags[*s];
        }
        bits = flags;
        if ((bits & 0x80) != 0) {
            guess = "US-ASCII";
            tag = 0;
            kind = kKindAscii;
        } else {
            if ((bits & kWinHasMac) != 0)
                kind = kKindMacText;
            else
                kind = kKindText;
            if (tag == kNoLength) {
                if ((set = flags, set &= kSetBits) == (kHasWin | kHasIso)) {
                    guess = "ISO-8859-1";
                    tag = kTagIso;
                } else if ((set = flags, set &= kSetBits) == kHasWin) {
                    guess = "Windows-1252";
                    tag = kTagWin;
                } else if ((flags &= kSetBits) == kHasMac) {
                    guess = "MacRoman";
                    tag = kTagMac;
                } else {
                    guess = "Unknown";
                    tag = kNoLength;
                }
            }
        }
        if (name == 0)
            name = guess;
    }
    if (name == 0)
        name = "Unknown";
done:
    if (nameOut != 0)
        StringCopy(nameOut, name);
    if (kindOut != 0)
        *kindOut = kind;
    if (bomOut != 0)
        *bomOut = bom;
    return tag;
}

/* IndirectIsNormalizedEncoding (is the handle's text readable as Windows-1252). STATE: MATCH 100%, exe 53 /
   obj 53, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505622
unsigned char IndirectIsNormalizedEncoding(void **handle)
{
    unsigned char ok;
    ULong tag;
    ULong kind;
    ULong bom;

    ok = 0;
    tag = _IndirectGetEncoding(handle, 0, &kind, &bom);
    switch (kind) {
    case kKindAscii:
        ok = 1;
        break;
    case kKindMacText:
    case kKindText:
        ok = (tag == kTagWin || tag == kTagIso) ? true : false;
        break;
    case kKindUnicode:
        ok = (kind == kKindUnicode && bom == kBomUtf16Little) ? true : false;
        break;
    case kKindUnknown:
        break;
    }
    return ok;
}

/* IndirectNormalizeEncoding (a handle's 8-bit text re-coded to Windows-1252 in place, from `charset' or
   the detected set). STATE: MATCH 100%, exe 89 / obj 89, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005056a4
long IndirectNormalizeEncoding(void **handle, const char *charset)
{
    CharEntry *table;
    CharsetInfo *info;
    ULong tag;
    ULong size;
    ULong i;
    unsigned char *p;
    unsigned char c;
    long err;

    err = 0;
    table = 0;
    if (charset == 0 || *charset == 0) {
        tag = _IndirectGetEncoding(handle, 0, 0, 0);
        if (tag == kTagUtf8 || tag == kTagUtf16 || tag == kTagUtf32) {
            err = kWrongCharset;
            goto done;
        }
        if (tag == kNoLength) {
            err = kWrongCharset;
            goto done;
        }
        table = _EncodingLookupTable(tag);
    } else {
        info = _EncodingLookupByName(charset);
        if (info == 0) {
            err = kWrongCharset;
            goto done;
        }
        tag = info->tag;
        if (tag == kTagUtf8 || tag == kTagUtf16 || tag == kTagUtf32) {
            err = kWrongCharset;
            goto done;
        }
        if (tag != kNoLength) {
            if (tag != 0)
                table = info->table;
        } else {
            err = kWrongCharset;
            goto done;
        }
    }
    if (table != 0 && gTextMappingsValid == 0)
        _EncodingPreflightMappings(0);
    if (table != 0 && table != gTextMappingsIso) {
        p = (unsigned char *)*handle;
        size = IndirectGetSize(handle);
        for (i = 0; i < size; i++, p++) {
            c = *p;
            *p = table[c].win;
        }
    }
done:
    return err;
}

/* IndirectIsUnicode (the Unicode form's name, "" when none; true when there is one). STATE:
   MATCH 100%, exe 27 / obj 27, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505786
unsigned char IndirectIsUnicode(void **handle, char *nameOut)
{
    const char *name;

    name = _IndirectGetUnicodeEncoding(handle, 0, 0);
    if (nameOut != 0)
        StringCopySafe(nameOut, name != 0 ? name : "", 256);
    if (name != 0)
        return 1;
    return 0;
}

/* IndirectIsNormalizedUnicode (does the handle start with a little-endian UTF-16 mark). STATE: MATCH 100%,
   exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005057c7
unsigned char IndirectIsNormalizedUnicode(void **handle)
{
    const char *name;
    ULong bom;

    bom = 0;
    name = _IndirectGetUnicodeEncoding(handle, 0, &bom);
    return name != 0 && bom == kBomUtf16Little;
}

/* IndirectRemoveUnicodeTag (drop a UTF-16 byte-order mark from the handle). STATE: MATCH 100%, exe 53 /
   obj 53, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005057fa
long IndirectRemoveUnicodeTag(void **handle)
{
    const char *name;
    ULong size;
    long err, e;

    err = 0;
    IndirectGetLock(handle);
    IndirectGetSize(handle);
    name = _IndirectGetUnicodeEncoding(handle, 0, 0);
    if (name == 0 || StringCompare(name, "UTF-16") != 0) {
        err = kWrongCharset;
        goto done;
    }
    size = IndirectGetSize(handle);
    MemoryMove(*handle, (char *)*handle + 2, size - 2);
    size -= 2;
    e = IndirectSetSize(handle, size);
    if (e != 0)
        err = e;
done:
    return err;
}

/* A Unicode code point as its Windows-1252 byte, or the substitute. */
/* UnicodePoint2Char8. STATE: MATCH 100%, exe 25 / obj 25, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050586e
unsigned char UnicodePoint2Char8(ULong code)
{
    unsigned char c;
    ULong i;
    long err;
    ULong nWin;

    err = 0;
    c = gTextMappingSubstitute;
    if (code <= kAsciiMax) {
        c = (unsigned char)code;
    } else {
        nWin = gUnicodeMappingCount;
        i = 0;
        while (i < nWin && code != gUnicodeMappings[i].unicode)
            i++;
        if (i < nWin)
            c = gUnicodeMappings[i].win;
    }
    return c;
}

/* UnicodeLookupTag (an entity name as its Unicode code point, 0 when unknown). STATE:
   MATCH 100%, exe 64 / obj 64, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005058af
ULong UnicodeLookupTag(const char *entity)
{
    char key[256];
    unsigned char *index;
    ULong code;
    long err;
    const CharEntry *row;

    err = 0;
    index = 0;
    row = 0;
    code = 0;
    StringCopySafe(key, entity, sizeof(key));
    index = (unsigned char *)HashLookup(gTextMappingsHashRef, key, 0);
    if (index == 0 && key[1] >= 'A' && key[1] <= 'Z')
        index = (unsigned char *)HashLookup(gTextMappingsHashRef, String2Upper(key), 0);
    if (index == 0)
        index = (unsigned char *)HashLookup(gTextMappingsHashRef, String2Lower(key), 0);
    if (index != 0) {
        row = &kCharEntries[*index];
        if (row->unicode != 0)
            code = row->unicode;
        else
            code = gTextMappingSubstitute;
    }
    return code;
}

#define kHighUnitBias       0xd7c0
#define kLowUnitMask        0x3ff

/* 0x00579450: the literals of the UTF-16 formatter, whose code is not linked */
static const char kNoSign[] = "";
static const char kMinus[] = "-";
static const char kPlus[] = "+";
static const unsigned short kHexPrefix[] = { '0', 'x', 0 };
static const unsigned short kNullText[] = { '<', 'N', 'U', 'L', 'L', '>', 0 };
static const unsigned char kNullPascal[] = "\p<NULL>";
static const char kNullC[] = "<NULL>";
/* Placement only: no reader in the exe. */
static const unsigned char kPadWord[4] = { 0 };  /* 0x00579484 */

ULong unicode_bytecount(const unsigned short *s);
ULong unicode_charcount(const unsigned short *s);
long unicode_strncmp(const unsigned short *a, ULong lenA,
                  const unsigned short *b, ULong lenB);
ULong unicode_bytecount_limit(const unsigned short *s, ULong index);
ULong unicode_charcount_limit(const unsigned short *s, ULong bytes);
unsigned short *unicode_strcpy(unsigned short *dst, const unsigned short *src);
long _unicode_findmatch(const unsigned short *s, const unsigned short *set, long inSet);
unsigned short *unicode_strpbrk(const unsigned short *s, const unsigned short *set);
unsigned short *convert_UTF32_to_UTF16(unsigned short *dst, const ULong *src, ULong dstBytes,
                             long budget, long terminate);

/* UnicodeGetByteCount. STATE: MATCH 100%, exe 8 / obj 8, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00505975
ULong UnicodeGetByteCount(const unsigned short *s)
{
    long err;
    ULong result;

    err = 0;
    result = 0;
    result = unicode_bytecount(s);
    return result;
}

/* UnicodeGetCharCount. STATE: MATCH 100%, exe 8 / obj 8, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00505988
ULong UnicodeGetCharCount(const unsigned short *s)
{
    long err;
    ULong result;

    err = 0;
    result = 0;
    result = unicode_charcount(s);
    return result;
}

/* UnicodeGetByteCountFromChars (the byte offset of a character index). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050599b
ULong UnicodeGetByteCountFromChars(const unsigned short *s, ULong index)
{
    long err;
    ULong result;

    err = 0;
    result = 0;
    result = unicode_bytecount_limit(s, index);
    return result;
}

/* UnicodeGetCharCountFromBytes (characters in a byte count of UTF-16). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005059b3
ULong UnicodeGetCharCountFromBytes(const unsigned short *s, ULong bytes)
{
    long err;
    ULong result;

    err = 0;
    result = 0;
    result = unicode_charcount_limit(s, bytes);
    return result;
}

/* UnicodeGetCharByIndex (the code point at a character index). STATE: MATCH 100%, exe 23 / obj 23,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005059cb
ULong UnicodeGetCharByIndex(const unsigned short *s, ULong index)
{
    ULong n;
    ULong c;
    long err;

    err = 0;
    n = unicode_bytecount_limit(s, index) >> 1;
    c = s[n];
    if ((c & kSurrogateMask) == kSurrogateHigh)
        c = (c << 10) + s[++n] + kSurrogateBias;
    return c;
}

/* UnicodeDuplicate (a copy of a UTF-16 string in a new block). STATE: MATCH 100%, exe 31 /
   obj 31, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505a0a
unsigned short *UnicodeDuplicate(const unsigned short *s)
{
    unsigned short *copy;
    ULong bytes;
    long err;

    err = 0;
    copy = 0;
    bytes = UnicodeGetByteCount(s) + sizeof(unsigned short);
    copy = MemoryAllocate(bytes);
    if (copy == 0)
        err = kOutOfMemory;
    else
        MemoryCopy(copy, s, bytes);
    return copy;
}

/* UnicodeCopy (strcpy). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505a4a
unsigned short *UnicodeCopy(unsigned short *dst, const unsigned short *src)
{
    long err;

    err = 0;
    unicode_strcpy(dst, src);
    return dst;
}

/* UnicodeCopySafe (a bounded copy that never splits a surrogate pair). STATE: MATCH 100%, exe 93
   / obj 93, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505a63
unsigned short *UnicodeCopySafe(unsigned short *dst, const unsigned short *src,
                             ULong dstBytes, ULong maxChars)
{
    ULong bytes;
    ULong n;
    ULong i;
    ULong o;
    ULong temp;
    ULong c;

    i = temp = 0;
    dstBytes -= dstBytes & 1;
    bytes = UnicodeGetByteCount(src);
    n = UnicodeGetCharCount(src);
    if (bytes + sizeof(unsigned short) < dstBytes && n < maxChars) {
        UnicodeCopy(dst, src);
    } else {
        dstBytes -= 2;
        if (maxChars > n)
            maxChars = n;
        i = o = 0;
        while (dstBytes != 0 && maxChars != 0) {
            c = src[i++];
            temp = c & kSurrogateMask;
            if (temp == kSurrogateHigh)
                c = (c << 10) + src[i++] + kSurrogateBias;
            temp = c <= kBmpMax ? 1 : 2;
            temp += temp;
            if (temp > dstBytes)
                break;
            if (c <= kBmpMax) {
                dst[o++] = c;
            } else {
                dst[o] = (c >> 10) + kHighUnitBias;
                temp = o + 1;
                dst[temp] = (c & kLowUnitMask) | kSurrogateLow;
                o += 2;
            }
            dstBytes -= (c <= kBmpMax ? 1 : 2) * sizeof(unsigned short);
            maxChars--;
        }
        dst[o] = 0;
    }
    return dst;
}

/* UnicodeCompare. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00505b6f
long UnicodeCompare(const unsigned short *a, const unsigned short *b)
{
    long err;

    err = 0;
    return unicode_strncmp(a, kNoLength, b, kNoLength);
}

/* UnicodeCompareSafe (strncmp in characters). STATE: MATCH 100%, exe 33 / obj 33, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00505b88
long UnicodeCompareSafe(const unsigned short *a, const unsigned short *b, ULong max)
{
    ULong lenA;
    ULong lenB;
    long err;

    err = 0;
    lenA = UnicodeGetCharCount(a);
    lenB = UnicodeGetCharCount(b);
    if (lenA > max)
        lenA = max;
    if (lenB > max)
        lenB = max;
    return unicode_strncmp(a, lenA, b, lenB);
}

/* UnicodeFindDelimiter (strpbrk). STATE: MATCH 100%, exe 8 / obj 8, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505bc9
unsigned short *UnicodeFindDelimiter(const unsigned short *s, const unsigned short *set)
{
    long err;

    err = 0;
    return unicode_strpbrk(s, set);
}

/* One code point off a UTF-16 string; answers the unit after it. */
/* UnicodeAdvance. STATE: MATCH 100%, exe 26 / obj 26, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505bdd
const unsigned short *UnicodeAdvance(const unsigned short *s, ULong *code)
{
    ULong n;
    ULong c;
    long err;

    err = 0;
    n = 0;
    c = s[n++];
    if ((c & kSurrogateMask) == kSurrogateHigh)
        c = (c << 10) + s[n++] + kSurrogateBias;
    if (code != 0)
        *code = c;
    return s + n;
}

/* UnicodeAdvancePtr (one code point off a UTF-16 cursor; false at the NUL). STATE: MATCH 100%,
   exe 34 / obj 34, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505c1e
unsigned char UnicodeAdvancePtr(const unsigned short **cursor, ULong *code)
{
    const unsigned short *s;
    ULong n;
    ULong c;
    long err;

    err = 0;
    n = 0;
    s = *cursor;
    c = s[n++];
    if ((c & kSurrogateMask) == kSurrogateHigh)
        c = (c << 10) + s[n++] + kSurrogateBias;
    *cursor += n;
    if (code != 0)
        *code = c;
    if (c != 0)
        return 1;
    return 0;
}

/* UnicodeAdvanceCount (the unit after a count of characters, stopping at the NUL). STATE: MATCH
   100%, exe 43 / obj 43, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505c6d
const unsigned short *UnicodeAdvanceCount(const unsigned short *s, ULong count)
{
    ULong i;
    ULong c;
    long err;

    err = 0;
    i = 0;
    while (count != 0) {
        c = s[i];
        if ((c & kSurrogateHalfMask) == kSurrogateHigh) {
            if ((c & kSurrogateLowBit) == 0)
                c = (c << 10) + s[i + 1] + kSurrogateBias;
            else
                c = ((ULong)s[i - 1] << 10) + c + kSurrogateBias;
        }
        if (c == 0)
            break;
        i += c <= kBmpMax ? 1 : 2;
        count = count - 1;
    }
    return s + i;
}

/* UTF32_2UStringCopySafe (UTF-32 to UTF-16, terminated). STATE: MATCH 100%, exe 18 / obj 18,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505ce5
unsigned short *UTF32_2UStringCopySafe(unsigned short *dst, const ULong *src, ULong dstBytes,
                             long maxChars)
{
    long err;

    err = 0;
    dstBytes -= dstBytes % sizeof(unsigned short);
    convert_UTF32_to_UTF16(dst, src, dstBytes, maxChars, 1);
    return dst;
}

/* U2CStringCopySafe (UTF-16 narrowed to Windows-1252 through the registry's table). STATE:
   MATCH 100%, exe 60 / obj 60, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505d11
unsigned char *U2CStringCopySafe(unsigned char *dst, const unsigned short *src, ULong dstBytes,
                            ULong maxChars)
{
    ULong nWin;
    unsigned char *p;
    unsigned char substitute;
    ULong i;
    unsigned char b;
    long err;
    ULong j;
    ULong c;

    err = 0;
    substitute = gTextMappingSubstitute;
    p = dst;
    dstBytes = dstBytes - 1;
    i = 0;
    nWin = gUnicodeMappingCount;
    while (dstBytes != 0 && maxChars != 0) {
        b = substitute;
        c = src[i];
        i = i + 1;
        if ((c & kSurrogateMask) == kSurrogateHigh) {
            c = (c << 10) + src[i] + kSurrogateBias;
            i = i + 1;
        }
        if (c <= kAsciiMax) {
            b = (unsigned char)c;
        } else {
            j = 0;
            while (j < nWin && c != gUnicodeMappings[j].unicode)
                j++;
            if (j < nWin)
                b = gUnicodeMappings[j].win;
        }
        *p = b;
        dstBytes = dstBytes - 1;
        maxChars = maxChars - 1;
        p = p + 1;
    }
    *p = 0;
    return dst;
}

/* A Windows-1252 string widened to UTF-16 through the registry's table. */
/* C2UStringCopySafe. STATE: MATCH 100%, exe 41 / obj 41, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505dba
unsigned short *C2UStringCopySafe(unsigned short *dst, const unsigned char *src,
                             ULong dstBytes, ULong maxChars)
{
    long err;
    CharEntry *table;
    unsigned short *p;
    ULong c;

    err = 0;
    table = 0;
    p = dst;
    dstBytes -= dstBytes & 1;
    table = gTextMappingsWin;
    dstBytes -= 2;
    while (dstBytes != 0 && maxChars != 0 && (c = *src) != 0) {
        *p = table[c].unicode;
        dstBytes -= 2;
        maxChars--;
        src++;
        p++;
    }
    *p = 0;
    return dst;
}

/* Unicode2UpperCopySafe. STATE: MATCH 100%, exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00505e16
unsigned short *Unicode2UpperCopySafe(unsigned short *dst, const unsigned short *src,
                             ULong dstBytes, ULong maxChars)
{
    ULong n;
    ULong i;
    ULong o;
    ULong c;
    long err;

    err = 0;
    dstBytes -= dstBytes & 1;
    dstBytes -= 2;
    n = UnicodeGetCharCount(src);
    if (maxChars > n)
        maxChars = n;
    i = o = 0;
    while (dstBytes != 0 && maxChars != 0) {
        c = src[i++];
        if ((c & kSurrogateMask) == kSurrogateHigh)
            c = (c << 10) + src[i++] + kSurrogateBias;
        c = unicode_toupper(c);
        if ((c <= kBmpMax ? 1 : 2) * sizeof(unsigned short) > dstBytes)
            break;
        if (c <= kBmpMax) {
            dst[o++] = c;
        } else {
            dst[o] = (unsigned short)((c >> 10) + kHighUnitBias);
            dst[(long)(o + 1)] = (unsigned short)((c & kLowUnitMask) | kSurrogateLow);
            o += 2;
        }
        dstBytes -= (c <= kBmpMax ? 1 : 2) * sizeof(unsigned short);
        maxChars--;
    }
    dst[o] = 0;
    return dst;
}
