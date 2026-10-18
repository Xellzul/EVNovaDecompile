// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* rezfixup.c -- post-load fixup for one resource body (win 0x004ce700, neighbour of rezopen.c). Notes: Research/notes/rezfixup.md */
#include "macport.h"
#include "resources.h"
#include "gameutil.h"
#include "burger.h"

/* The 25 endian-swap descriptor streams endianswap.c's SwapEndianByDescriptor walks --
   see Research/data-global-initializers.md rows 13-37. */
unsigned short addr_00575eb0[18] = {   /* misn: covers 0x7b2 == minSize */
    4, 1, 14,
    2,
    4, 1, 30,
    3, 1530,
    2,
    2,
    1,
    3, 320,
    4, 1, 9,
    5
};
unsigned short addr_00575ed4[4] = {   /* covers 0xc == minSize */
    4, 1, 6,
    5
};
unsigned short addr_00575edc[4] = {   /* covers 0xc0 == minSize */
    4, 1, 96,
    5
};
unsigned short addr_00575ee4[22] = {   /* spob: covers 0x45e == minSize */
    4, 1, 3,
    2,
    4, 1, 22,
    3, 510,
    2,
    1,
    1,
    2,
    4, 1, 3,
    3, 510,
    4, 1, 13,
    5
};
unsigned short addr_00575f10[12] = {   /* syst: covers 0x1ac == minSize */
    4, 1, 71,
    2,
    1,
    1,
    3, 256,
    4, 1, 11,
    5
};
unsigned short addr_00575f28[16] = {   /* outf: covers 0x404 == minSize */
    4, 1, 7,
    2,
    4, 1, 6,
    4, 2, 4,
    3, 958,
    4, 1, 12,
    5
};
unsigned short addr_00575f48[21] = {   /* weap: covers 0x86 == minSize */
    4, 1, 22,
    2,
    1,
    1,
    1,
    2,
    2,
    4, 1, 10,
    2,
    4, 1, 14,
    2,
    4, 1, 8,
    5
};
unsigned short addr_00575f72[34] = {   /* ship: covers 0x744 == minSize */
    4, 1, 24,
    2,
    4, 1, 24,
    2,
    2,
    3, 766,
    4, 1, 11,
    2,
    2,
    4, 1, 36,
    3, 766,
    4, 1, 12,
    3, 64,
    1,
    1,
    2,
    2,
    4, 1, 9,
    5
};
unsigned short addr_00575fb6[4] = {   /* dede: covers 0x58 == minSize */
    4, 1, 44,
    5
};
unsigned short addr_00575fbe[19] = {   /* govt: covers 0xc0 == minSize */
    4, 1, 26,
    3, 32,
    4, 2, 2,
    4, 1, 4,
    3, 64,
    2,
    2,
    4, 1, 10,
    5
};
unsigned short addr_00575fe4[19] = {   /* pers: covers 0x190 == minSize */
    4, 1, 18,
    2,
    4, 1, 6,
    3, 256,
    1,
    1,
    1,
    3, 64,
    2,
    4, 1, 9,
    5
};
unsigned short addr_0057600a[9] = {   /* covers 0x11a == minSize */
    4, 1, 5,
    3, 256,
    4, 1, 8,
    5
};
unsigned short addr_0057601c[6] = {   /* nebu: covers 0x206 == minSize */
    4, 1, 4,
    3, 510,
    5
};
unsigned short addr_00576028[4] = {   /* char: covers 0x6 == minSize */
    4, 1, 3,
    5
};
unsigned short addr_00576030[9] = {   /* flet: covers 0x132 == minSize */
    4, 1, 15,
    3, 256,
    4, 1, 10,
    5
};
unsigned short addr_00576042[6] = {   /* junk: covers 0x2a4 == minSize */
    4, 1, 19,
    3, 638,
    5
};
unsigned short addr_0057604e[12] = {   /* covers 0x336 == minSize */
    4, 1, 12,
    3, 766,
    4, 2, 4,
    4, 1, 8,
    5
};
unsigned short addr_00576066[10] = {   /* covers 0x98 == minSize */
    1,
    1,
    1,
    4, 2, 4,
    1,
    3, 128,
    5
};
unsigned short addr_0057607a[8] = {   /* roid: covers 0x28 == minSize */
    4, 1, 5,
    2,
    4, 1, 13,
    5
};
unsigned short addr_0057608a[4] = {   /* covers 0x40 == minSize */
    4, 1, 32,
    5
};
unsigned short addr_00576092[29] = {   /* covers 0xa6 == minSize */
    2,
    2,
    4, 1, 4,
    2,
    2,
    4, 1, 4,
    2,
    4, 1, 4,
    2,
    4, 1, 4,
    2,
    2,
    4, 1, 16,
    3, 64,
    4, 1, 3,
    5
};
unsigned short addr_005760cc[4] = {   /* rle8/rleD: covers 0x10 == minSize */
    4, 1, 8,
    5
};
unsigned short addr_005760d4[27] = {   /* colr: covers 0xf4 == minSize */
    4, 2, 3,
    3, 64,
    1,
    4, 2, 4,
    4, 1, 4,
    4, 2, 3,
    4, 1, 12,
    4, 2, 5,
    3, 64,
    4, 1, 11,
    5
};
unsigned short addr_0057610a[17] = {   /* cron: covers 0x16a == minSize */
    2,
    4, 1, 23,
    3, 256,
    4, 1, 4,
    3, 16,
    3, 16,
    4, 1, 8,
    5
};
unsigned short addr_0057612c[2] = {   /* covers 0x4 == minSize */
    2,
    5
};

/* FixupResourceByType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce700
void FixupResourceByType(Handle h, unsigned long type)
{
    unsigned short *desc;
    unsigned long minSize = 0;

    switch (type) {
    case kMisnType: desc = addr_00575eb0; minSize = kMisnResSize; break;
    case kSpinType: desc = addr_00575ed4; minSize = 0xc;   break;
    case kShanType: desc = addr_00575edc; minSize = 0xc0;  break;
    case kSpobType: desc = addr_00575ee4; minSize = 0x45e; break;
    case kSystType: desc = addr_00575f10; minSize = 0x1ac; break;
    case kOutfType: desc = addr_00575f28; minSize = 0x404; break;
    case kWeapType: desc = addr_00575f48; minSize = 0x86;  break;
    case kShipType: desc = addr_00575f72; minSize = 0x744; break;
    case kDudeType: desc = addr_00575fb6; minSize = 0x58;  break;
    case kGovtType: desc = addr_00575fbe; minSize = 0xc0;  break;
    case kPersType: desc = addr_00575fe4; minSize = 0x190; break;
    case kOopsType: desc = addr_0057600a; minSize = 0x11a; break;
    case kNebuType: desc = addr_0057601c; minSize = 0x206; break;
    case kBoomType: desc = addr_00576028; minSize = 6;     break;
    case kFletType: desc = addr_00576030; minSize = 0x132; break;
    case kJunkType: desc = addr_00576042; minSize = 0x2a4; break;
    case kCronType: desc = addr_0057604e; minSize = 0x336; break;
    case kRankType: desc = addr_00576066; minSize = 0x98;  break;
    case kRoidType: desc = addr_0057607a; minSize = 0x28;  break;
    case kEBugType: desc = addr_0057608a; minSize = 0x40;  break;
    case kIntfType: desc = addr_00576092; minSize = 0xa6;  break;
    case kRLE8ResType:
    case kRLEDResType:
        desc = addr_005760cc;
        break;
    case kColrType: desc = addr_005760d4; minSize = 0xf4;  break;
    case kCharType: desc = addr_0057610a; minSize = 0x16a; break;
    case 0x63739f6d: desc = addr_0057612c; minSize = 4;     break;
    default:
        return;
    }

    if (GetAHandleSize(h) < minSize)
        h = ResizeAHandle(h, minSize);
    LockAHandle(h);
    SwapEndianByDescriptor((unsigned char *)*h, desc);

    if (type == 0x726c9138 || type == 0x726c9144)
        FixupRLEPixelData((unsigned char *)*h + 0x10, (RLEPixelData *)*h);
    UnlockAHandle(h);
}
