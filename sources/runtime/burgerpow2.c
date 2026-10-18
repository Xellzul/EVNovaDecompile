// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: BurgerLib's P band line (burgermace.c's COMPILE-WHY); naked asm, object-identical under it but for the C++ -defaultlib .drectve the band's other objects carry
/* burgerpow2.c -- BurgerLib's PowerOf2 and FastStrLen (the game's strlen), hand assembly in the library itself (the 2007 archive's ClMicrosoftVC5.obj is the same streams, register-passed). Notes: Research/notes/burgerpow2.md */

#include "burgertypes.h"
#include "burger.h"

/* PowerOf2 (BurgerLib): the smallest power of two >= Input, 0x80000000 past 2^30. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004eed80
NAKED Word32 PowerOf2(Word32 Input)
{
    __asm {
        mov     eax, dword ptr [esp + 4]
        cmp     eax, 0x40000000
        ja      big
        cmp     eax, 2
        jbe     done
        dec     eax
        bsr     ecx, eax
        mov     eax, 2
        shl     eax, cl
    done:
        ret
        align   16
    big:
        mov     eax, 0x80000000
        ret
    }
}

/* FastStrLen (BurgerLib; Burger.h maps strlen onto it): word-at-a-time length. STATE: MATCH, exe 45 / obj 45, byte-identical yes (149 bytes). OPEN: none */
// FUNCTION: EVNOVA 0x004eedb0
NAKED Word FastStrLen(const char *Input)
{
    __asm {
        mov     eax, dword ptr [esp + 4]
        push    ebx
        mov     ebx, eax
        and     eax, 3
        neg     eax
        je      scan
        mov     edx, 0xffffffff
        lea     ecx, [eax * 8 + 0x20]
        shr     edx, cl
        mov     ecx, dword ptr [ebx + eax]
        add     eax, 4
        or      ecx, edx
        mov     edx, ecx
        xor     ecx, 0xffffffff
        sub     edx, 0x1010101
        and     ecx, 0x80808080
        test    ecx, edx
        jne     found
        align   16
    scan:
        mov     edx, dword ptr [ebx + eax]
        mov     ecx, dword ptr [ebx + eax]
        add     eax, 4
        xor     ecx, 0xffffffff
        sub     edx, 0x1010101
        and     ecx, 0x80808080
        test    ecx, edx
        je      scan
    found:
        add     edx, 0x1010101
        test    edx, 0xff
        je      len4
        test    edx, 0xff00
        je      len3
        test    edx, 0xff0000
        je      len2
        dec     eax
        pop     ebx
        ret
        align   16
    len2:
        sub     eax, 2
        pop     ebx
        ret
    len3:
        sub     eax, 3
        pop     ebx
        ret
        align   16
    len4:
        sub     eax, 4
        pop     ebx
        ret
    }
}
