// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* sprites.c -- sprite metric accessors (win region 0x00462340-0x004624c0). Notes: Research/notes/sprites.md */
#include <stddef.h>

#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "game.h"
#include "crashlog.h"
#include "macport.h"

EXTERN_C_BEGIN

/*
 * Two rects: the frame the current cel occupies and the sprite's own bounds.
 * unk50 is the validity word every accessor checks first.
 */

#define kMaxStellarTypes 0x100

#define kDefaultSpriteSize 0x20

/* the larger of the frame rect's two dimensions */
/* GetFrameBiggestSize. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00462340
// MAC: _GetFrameBiggestSize i386 0x000026a3 ppc 0x000030b8
short GetFrameBiggestSize(SWFrameRec *frame)
{
    short width;
    short height;

    if (frame == NULL)
        return kDefaultSpriteSize;
    if (frame == NULL)
        SWAssertFail("Util.c", SW_LINE(0x143));
    width = frame->frameRect.right - frame->frameRect.left;
    height = frame->frameRect.bottom - frame->frameRect.top;
    if (width > height)
        return width;
    return height;
}

/* the current cel's width, but only while the frame index is in range */
/* GetSpriteXSize. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00462390
// MAC: _GetSpriteXSize i386 0x000026d4 ppc 0x000030f8
short GetSpriteXSize(SpriteRec *spr)
{
    if (spr == NULL)
        return kDefaultSpriteSize;
    if (spr->curFrameP != 0 && spr->curFrameIndex >= 0 && spr->curFrameIndex < spr->numFrames)
        return spr->destFrameRect.right - spr->destFrameRect.left;
    return kDefaultSpriteSize;
}

/* GetSpriteYSize. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004623d0
// MAC: _GetSpriteYSize i386 0x00002703 ppc 0x00003140
short GetSpriteYSize(SpriteRec *spr)
{
    if (spr == NULL)
        return kDefaultSpriteSize;
    if (spr->curFrameP != 0 && spr->curFrameIndex >= 0 && spr->curFrameIndex < spr->numFrames)
        return spr->destFrameRect.bottom - spr->destFrameRect.top;
    return kDefaultSpriteSize;
}

/* GetStellarSpriteXSize (mac _GetStellarSpriteXSize). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00462410
// MAC: _GetStellarSpriteXSize i386 0x0000ac91 ppc 0x0000d4c0
short GetStellarSpriteXSize(short id)
{
    short type;
    SpriteRec *spr;

    if (id >= 0 && id < kMaxStellars
            && stellarData[id].system == shipData[0].curSystem) {
        type = stellarData[id].type;
        if (type >= 0)
            if (type < kMaxStellarTypes) {
                spr = masterStellarSprite[type];
                if (spr != 0 && spr->curFrameP != 0)
                    return GetSpriteXSize(spr);
            }
    }
    /* BUG: no call parens, so this tests the function's address and is always true */
    if (crashlog_isactive) {
        crashlog_print("warning: tried to get size of nonexistent sprite for stellar ");
        crashlog_print_num(id + 0x80);
        crashlog_print("\n");
    }
    return 150;
}

/* GetMasterShipSpriteXSize (mac _GetMasterShipSpriteXSize). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004624c0
// MAC: _GetMasterShipSpriteXSize i386 0x0000ad38 ppc 0x0000d5b8
short GetMasterShipSpriteXSize(short id)
{
    short sprite;

    if (id >= 0 && id < kMaxShipTypes) {
        sprite = shipClass[id].spriteIndex;
        if (sprite >= 0 && sprite < kMaxShipTypes
                && masterShipSprite[sprite]->curFrameP != 0)
            return GetSpriteXSize(masterShipSprite[sprite]);
    }
    /* BUG: no call parens, so this tests the function's address and is always true */
    if (crashlog_isactive) {
        crashlog_print("warning: tried to get size of nonexistent sprite for ship class ");
        crashlog_print_num(id + 0x80);
        crashlog_print("\n");
    }
    return 75;
}

EXTERN_C_END
