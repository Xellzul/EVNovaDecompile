// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerprefs.c -- the preferences file: open, close, write, lookup. Notes: Research/notes/burgerprefs.md */

#include <stdio.h>

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The INI parser's arities reconcile against the vendor archive:
   @PrefFileNewFromMemory@8, @PrefFileNewFromFile@4 = OpenPrefsFile,
   @PrefFileSectionGetString@20 = PrefsGetString, and so on down the surface. */

extern char *strchr(const char *s, int c);       /* MSL's, called */

/* The file is a circular list of lines; the file record itself (type 0) is the sentinel. */
typedef struct PrefsNode {
    long Type;                                   /* +0x00 */
    struct PrefsNode *Next;                      /* +0x04 */
    struct PrefsNode *Prev;                      /* +0x08 */
    char *ValueText;                             /* +0x0c, type 2 only */
} PrefsNode;

#define kNodeFile    0
#define kNodeGroup   1
#define kNodeKey     2
#define kNodeText    3
#define kGroupName   0x0c                        /* inline, types 1 and 3 */
#define kKeyName     0x10                        /* inline, type 2 */
#define kLineHeader  0x0c                        /* the record without its text */
#define kTextLineSize 0x10                       /* a section or verbatim line, text[4] */
#define kKeyLineSize 0x14                        /* an entry, value pointer and key[4] */
#define kLineSize    0x400

#define kOpenBracket  '['
#define kCloseBracket ']'
#define kNewline      '\n'
#define kEquals       '='
#define kSemicolon    ';'
#define kSlash        '/'

/* BurgerLib's PrefFileNew: an empty file, its record the list's sentinel. */
static PrefsNode *PrefFileNew(void)
{
    PrefsNode *file;

    file = (PrefsNode *)AllocAPointer(kLineHeader);
    if (file != 0) {
        file->Type = kNodeFile;
        file->Next = file;
        file->Prev = file;
    }
    return file;
}

/* BurgerLib's LineEntryAddComment: a verbatim line after last. */
static void LineEntryAddComment(PrefsNode *last, const char *text)
{
    PrefsNode *node;
    Word length;

    length = strlen(text);
    node = (PrefsNode *)AllocAPointer(length + kTextLineSize);
    if (node != 0) {
        memcpy((char *)node + kGroupName, text, length + 1);
        node->Type = kNodeText;
        node->Next = last->Next;
        node->Prev = last;
        node->Next->Prev = node;
        node->Prev->Next = node;
    }
}

extern PrefsNode *PrefFileAddSection(PrefsNode *file, const char *name);
extern void PrefFileSectionAddEntry(PrefsNode *group, const char *key, const char *value);

/* PrefFileNewFromMemory (BurgerLib PgPrefFile): parse an INI image line by line into
   sections, "key = value" entries and verbatim lines. STATE: MATCH 100%, exe 149 / obj 149,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ffa00
void *PrefFileNewFromMemory(const char *data, unsigned long length)
{
    char line[kLineSize];
    ULong used;
    PrefsNode *file;
    char *mark;
    Word c;

    file = PrefFileNew();
    if (file != 0) {
        if (length != 0) do {
            used = length;
            ExtractAString2(data, &used, kExtractStrDelimitLF, line, sizeof(line));
            length -= used;
            data += used;
            c = (Word8)line[0];
            if (c == kOpenBracket) {
                mark = strchr(line, kCloseBracket);
                if (mark != 0) {
                    *mark = 0;
                    PrefFileAddSection(file, line + 1);
                    continue;
                }
            }
            if (c != kEquals && c != kSemicolon && (c != kSlash || (Word8)line[1] != kSlash)) {
                mark = strchr(line, kEquals);
                if (mark != 0) {
                    *mark = 0;
                    StripLeadingAndTrailingWhiteSpace(line);
                    StripLeadingAndTrailingWhiteSpace(mark + 1);
                    if (line[0] != 0)
                        PrefFileSectionAddEntry(file->Prev, line, mark + 1);
                    continue;
                }
            }
            LineEntryAddComment(file->Prev, line);
        } while (length != 0);
        return file;
    }
    return 0;
}

/* OpenPrefsFile. STATE: MATCH 100%, exe 33 / obj 33 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ffc50
void *OpenPrefsFile(const char *path)
{
    void *text;
    unsigned long size;
    void *file;

    text = LoadAFile(path, &size);
    if (text == 0)
        return 0;
    file = PrefFileNewFromMemory((const char *)text, size);
    DeallocAPointer(text);
    return file;
}

/* ClosePrefsFile. STATE: MATCH 100%, exe 36 / obj 36 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004ffcc0
void ClosePrefsFile(PrefsNode *file)
{
    PrefsNode *node;
    PrefsNode *next;

    if (file != 0) {
        node = file->Next;
        while (node != file) {
            switch (node->Type) {
            case kNodeKey:
                DeallocAPointer(node->ValueText);
                break;
            }
            next = node->Next;
            DeallocAPointer(node);
            node = next;
        }
        DeallocAPointer(file);
    }
}

/* WritePrefsFile. STATE: MATCH 100%, exe 104 / obj 104 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004ffd30
long WritePrefsFile(PrefsNode *file, const char *path)
{
    TextBuilder text;
    PrefsNode *node;
    FILE *out;
    long result;                                /* never set on failure, as in the exe */

    StreamHandleInitPut(&text);

    node = file->Next;
    while (node != file) {
        switch (node->Type) {
        case kNodeText:
        default:
            StreamHandlePutStringNoZero(&text, ((char *)node + kGroupName));
            break;
        case kNodeGroup:
            StreamHandlePutByte(&text, kOpenBracket);
            StreamHandlePutStringNoZero(&text, ((char *)node + kGroupName));
            StreamHandlePutByte(&text, kCloseBracket);
            break;
        case kNodeKey:
            StreamHandlePutStringNoZero(&text, ((char *)node + kKeyName));
            StreamHandlePutStringNoZero(&text, " = ");
            StreamHandlePutStringNoZero(&text, node->ValueText);
            break;
        }
        StreamHandlePutByte(&text, kNewline);
        node = node->Next;
    }

    StreamHandleEndSave(&text);
    if (text.failed == 0) {
        out = (FILE *)FOpenExpandedPath(path, "w");
        if (out != 0) {
            fwrite(LockAHandle(text.handle), 1, text.extent, out);
            fclose(out);
            result = 0;
        }
    }
    StreamHandleDestroy(&text);
    return result;
}

/* PrefsFindGroup. STATE: MATCH 100%, exe 30 / obj 30 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ffeb0
PrefsNode *PrefsFindGroup(PrefsNode *file, const char *name)
{
    PrefsNode *node;

    node = file->Next;
    while (node != file) {
        if (node->Type == kNodeGroup
            && stricmp(((char *)node + kGroupName), name) == 0)
            return node;
        node = node->Next;
    }
    return 0;
}

/* PrefFileAddSection (BurgerLib PgPrefFile): a "[name]" line at the end of the file.
   STATE: MATCH 100.0%, exe 40 / obj 40, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fff10
PrefsNode *PrefFileAddSection(PrefsNode *file, const char *name)
{
    PrefsNode *node;
    Word length;

    length = strlen(name);
    node = (PrefsNode *)AllocAPointer(length + kTextLineSize);
    if (node != 0) {
        memcpy((char *)node + kGroupName, name, length + 1);
        node->Type = kNodeGroup;
        node->Next = file;
        node->Prev = file->Prev;
        node->Next->Prev = node;
        node->Prev->Next = node;
    }
    return node;
}

/* BurgerLib's PrefFileSectionFindEntry; the exe has no copy of its own, only this one inlined. */
static PrefsNode *PrefsFindEntry(PrefsNode *group, const char *key)
{
    PrefsNode *node;

    node = group->Next;
    while (node->Type > kNodeGroup) {
        if (node->Type == kNodeKey) {
            if (stricmp((char *)node + kKeyName, key) == 0)
                return node;
        }
        node = node->Next;
    }
    return 0;
}

/* PrefFileSectionGetFloat and ...GetDouble (BurgerLib PgPrefFile): a number read from a key,
   clamped to a range unless an end is zero. Not linked. */
float PrefFileSectionGetFloat(PrefsNode *group, const char *key, float dflt, float min,
                              float max)
{
    PrefsNode *node;
    char *value;
    float result;

    node = PrefsFindEntry(group, key);
    value = node != 0 ? node->ValueText : 0;
    if (value == 0 || sscanf(value, "%f", &result) != 1)
        return dflt;
    if (!min || !max)
        return result;
    if (result < min)
        return min;
    if (result > max)
        return max;
    return result;
}

double PrefFileSectionGetDouble(PrefsNode *group, const char *key, double dflt, double min,
                                double max)
{
    PrefsNode *node;
    char *value;
    double result;

    node = PrefsFindEntry(group, key);
    value = node != 0 ? node->ValueText : 0;
    if (value == 0 || sscanf(value, "%lf", &result) != 1)
        return dflt;
    if (!min || !max)
        return result;
    if (result < min)
        return min;
    if (result > max)
        return max;
    return result;
}

/* The value is unquoted into the caller's buffer first; only if that came
   back unchanged AND the value is non-empty does the value replace the
   default, and the copy below runs on whichever survived. */
/* PrefsGetString. STATE: MATCH 100%, exe 80 / obj 80 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fff90
void PrefsGetString(PrefsNode *group, const char *key, const char *dflt,
                    char *out, long outSize)
{
    PrefsNode *node;
    char *value;
    ULong len;

    node = PrefsFindEntry(group, key);
    value = node != 0 ? node->ValueText : 0;
    if (value != 0) {
        if (ParseQuotedString(out, outSize, value) != value)
            return;
        if (*value != 0)
            dflt = value;
    }
    if (outSize != 0) {
        if (dflt != 0) {
            len = strlen(dflt);
            if (len >= (ULong)outSize)
                len = outSize - 1;
            memcpy(out, dflt, len);
        } else {
            len = 0;
        }
        out[len] = 0;
    }
}

/* PrefFileSectionAddEntry (BurgerLib PgPrefFile): "key = value" as the last line of the
   section group starts. STATE: MATCH 100.0%, exe 50 / obj 50, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00500080
void PrefFileSectionAddEntry(PrefsNode *group, const char *key, const char *value)
{
    PrefsNode *node;
    PrefsNode *next;
    Word length;

    length = strlen(key);
    node = (PrefsNode *)AllocAPointer(length + kKeyLineSize);
    if (node != 0) {
        memcpy((char *)node + kKeyName, key, length + 1);
        node->ValueText = StrCopy(value);
        node->Type = kNodeKey;
        next = group->Next;
        while (next->Type > kNodeGroup)
            next = next->Next;
        node->Next = next;
        node->Prev = next->Prev;
        node->Next->Prev = node;
        node->Prev->Next = node;
    }
}

/* PrefFileSectionPutFloat (BurgerLib PgPrefFile): a key set to a number. Not linked. */
void PrefFileSectionPutFloat(PrefsNode *group, const char *key, float value)
{
    char text[64];
    PrefsNode *node;

    sprintf(text, "%g", (double)value);
    node = PrefsFindEntry(group, key);
    if (node != 0) {
        DeallocAPointer(node->ValueText);
        node->ValueText = StrCopy(text);
    } else {
        PrefFileSectionAddEntry(group, key, text);
    }
}

#ifdef __cplusplus
}
#endif
