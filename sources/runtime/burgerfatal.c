// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerfatal.c -- BurgerLib's error reporters (DbDebugManager) and the key-name table. Notes: Research/notes/burgerfatal.md */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

char ErrorMsg[512];                              /* 0x0086e420 */
Word BombFlag;                                   /* 0x008701bc */

typedef struct KeyName {
    const char *name;
    long code;
} KeyName;

/* 0x005776dc */
static KeyName kKeyNames[] = {
    { "Escape",     0x01 },
    { "Esc",        0x01 },
    { "0",          0x0b },
    { "1",          0x02 },
    { "2",          0x03 },
    { "3",          0x04 },
    { "4",          0x05 },
    { "5",          0x06 },
    { "6",          0x07 },
    { "7",          0x08 },
    { "8",          0x09 },
    { "9",          0x0a },
    { "A",          0x1e },
    { "B",          0x30 },
    { "C",          0x2e },
    { "D",          0x20 },
    { "E",          0x12 },
    { "F",          0x21 },
    { "G",          0x22 },
    { "H",          0x23 },
    { "I",          0x17 },
    { "J",          0x24 },
    { "K",          0x25 },
    { "L",          0x26 },
    { "M",          0x32 },
    { "N",          0x31 },
    { "O",          0x18 },
    { "P",          0x19 },
    { "Q",          0x10 },
    { "R",          0x13 },
    { "S",          0x1f },
    { "T",          0x14 },
    { "U",          0x16 },
    { "V",          0x2f },
    { "W",          0x11 },
    { "X",          0x2d },
    { "Y",          0x15 },
    { "Z",          0x2c },
    { "F1",         0x3b },
    { "F2",         0x3c },
    { "F3",         0x3d },
    { "F4",         0x3e },
    { "F5",         0x3f },
    { "F6",         0x40 },
    { "F7",         0x41 },
    { "F8",         0x42 },
    { "F9",         0x43 },
    { "F10",        0x44 },
    { "F11",        0x57 },
    { "F12",        0x58 },
    { "-",          0x0c },
    { "=",          0x0d },
    { "BakSpc",     0x0e },
    { "Tab",        0x0f },
    { "[",          0x1b },
    { "]",          0x1a },
    { ";",          0x27 },
    { "'",          0x28 },
    { "`",          0x29 },
    { "\\",         0x2b },
    { ",",          0x33 },
    { ".",          0x34 },
    { "/",          0x35 },
    { "Enter",      0x1c },
    { "LCtrl",      0x1d },
    { "RCtrl",      0x6b },
    { "LShift",     0x2a },
    { "RShift",     0x36 },
    { "LAlt",       0x38 },
    { "RAlt",       0x6f },
    { "Space",      0x39 },
    { "CapLck",     0x3a },
    { "NumLck",     0x45 },
    { "ScrLck",     0x46 },
    { "Pause",      0x6e },
    { "Up",         0x61 },
    { "Down",       0x66 },
    { "Left",       0x63 },
    { "Right",      0x64 },
    { "Insert",     0x68 },
    { "Delete",     0x69 },
    { "Home",       0x60 },
    { "End",        0x65 },
    { "PgUp",       0x62 },
    { "PgDn",       0x67 },
    { "PrtScn",     0x6d },
    { "KPad0",      0x52 },
    { "KPad1",      0x4f },
    { "KPad2",      0x50 },
    { "KPad3",      0x51 },
    { "KPad4",      0x4b },
    { "KPad5",      0x4c },
    { "KPad6",      0x4d },
    { "KPad7",      0x47 },
    { "KPad8",      0x48 },
    { "KPad9",      0x49 },
    { "KPad*",      0x37 },
    { "KPad-",      0x4a },
    { "KPad+",      0x4e },
    { "KPad.",      0x53 },
    { "KPad/",      0x6c },
    { "KPdEnt",     0x6a },
    { "KPadEnt",    0x6a },
    { "KPadEnter",  0x6a },
    { 0, -1 }
};

/* KeyboardScanCodeToString (BurgerLib KeyboardScanCodeToString). STATE: MATCH 100%, exe 45 / obj 45
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1990
void KeyboardScanCodeToString(char *out, ULong size, ULong code)
{
    const KeyName *entry;

    if (size != 0) {
        entry = kKeyNames;
        do {
            if (entry->code == code) {
                code = strlen(entry->name);
                if (code >= size)
                    code = size - 1;
                memcpy(out, entry->name, code);
                out[code] = 0;
                return;
            }
            ++entry;
        } while (entry->name != 0);
        out[0] = 0;
    }
}

#ifdef __cplusplus
}
#endif
