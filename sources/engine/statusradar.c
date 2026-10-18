// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* statusradar.c -- DrawStatusRadar (win link order 0x0045d600), the fifth member of the status-painter run and the one function of it that is NOT in statusparts.c. Notes: Research/notes/statusradar.md */
#include <math.h>
#include <stddef.h>

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "gameconst.h"
#include "dialogs.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "stellar.h"
#include "prototypes.h"
#include "space.h"
#include "resources.h"
#include "ai.h"
#include "game.h"
#include "missions.h"

/* the cargo panel is statusOffRect[7], the weapon readout statusOffRect[5] */

/* wëap runtime record, stride 0xc8; only the three fields read here */



/*
 * DrawStatusTarg's own records.  resources.h has all three in full, but its
 * WeapRecord collides with the local one this file already carries for
 * DrawStatusWeap, so the three are windowed here the same way -- only the
 * fields this readout touches, padded to the real stride.
 */



/* the target readout is statusOffRect[6] */

/* shïp Flags bits this readout tests */

/* shïp Flags3 bits */


/* the navigation readout is statusOffRect[4] */

/* the two resource records this readout walks, windowed here because
   resources.h's WeapRecord collides with the local one above */

/* the stellar sprite record, reached here only through its frame pointer */

/* the radar is statusOffRect[0] */
#define kRadarRect          0
#define kRadarScale         0.03125     /* 1/32: system units to radar pixels */
#define kRadarStaticFrames  10
#define kRadarArrowDist     7000000.0   /* draw the departure arrow past this */
#define kSpobNoRadarOval    0x10        /* spob Flags: draw a box, not an oval */
/* DrawStatusRadar (mac _DrawStatusRadar). STATE: MATCH 100.0% (916 exact, 132
   reloc-equiv, 0 mismatched; exe 1048 / obj 1048), frame 0x44 EXACT, refs 23 of 23,
   relocregcheck 0 of 132, call/imm/fcmp censuses exact, poolcheck 7 of 7 literals equal,
   no string literals; byte-identical pending the next regate --update (bytediff reads the
   baseline). OPEN: nothing. */
// FUNCTION: EVNOVA 0x0045d600
void DrawStatusRadar(char highlight)
{
    Rect dstRect;
    Rect srcRect;
    FloatPoint tip;
    Rect blipRect;
    FloatPoint barb;
    RGBColor color;
    char hasDensity;
    char hasIFF;
    char visible;
    long cx;
    long cy;
    long i;
    long stellar;
    long size;
    long angle;
    long links;

    if (statusOffRect[kRadarRect].left >= statusOffRect[kRadarRect].right ||
        statusOffRect[kRadarRect].top >= statusOffRect[kRadarRect].bottom)
        return;
    if (playerIsLanded)
        highlight = 1;
    hasIFF = HasIFF(shipData);
    hasDensity = HasDensityScanner(shipData);
    SetGWorldContainer(&loadPort);
    srcRect.top = statusOffRect[kRadarRect].top;
    srcRect.left = statusOffRect[kRadarRect].left;
    srcRect.bottom = statusOffRect[kRadarRect].bottom;
    srcRect.right = statusOffRect[kRadarRect].right;
    dstRect.top = srcRect.top;
    dstRect.left = srcRect.left;
    dstRect.bottom = srcRect.bottom;
    dstRect.right = srcRect.right;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kTargPanelInset, 0);
    if (highlight == 0 && interferenceThisFrame != 0) {
        cx = Rand(kRadarStaticFrames);
        if (staticPat[cx] != NULL)
            FillCRect(&dstRect, staticPat[cx]);
    } else {
        RGBForeColor(blackColor);
        if (hasIFF != 0)
            PaintRect(&dstRect);
        else
            CopyBits(GetGWCPixMap(&statusOffPort), (BitMapPtr)GetGWCPixMap(&loadPort),
                     &srcRect, &dstRect, 0, 0);
        cx = (dstRect.left + dstRect.right) * 0.5;
        cy = (dstRect.top + dstRect.bottom) * 0.5;
        if (highlight == 0) {
            for (i = 0; i < kNavSlots; i++) {
                stellar = gSystems[shipData[0].curSystem].navDefault[i];
                if (stellar != -1) {
                    blipRect.left = blipRect.right =
                        (cx + (long)(stellarData[stellar].pos.h -
                                     shipData[0].pos.x) * kRadarScale);
                    blipRect.top = blipRect.bottom =
                        (cy + (long)(stellarData[stellar].pos.v -
                                     shipData[0].pos.y) * kRadarScale);
                    if ((stellarData[stellar].flags & kSpobNoRadarOval) != 0 ||
                        (stellarData[stellar].flags2 & kSpobNoNav) != 0) {
                        if (hasIFF != 0) {
                            ColorCodeStellar(&stellarData[stellar], &color);
                            RGBForeColor(&color);
                        } else {
                            RGBForeColor(radarStatusColor[0]);
                        }
                        InsetRect(&blipRect, -1, -1);
                        SectRect(&blipRect, &dstRect, &blipRect);
                        if (blipRect.left != 0 || blipRect.top != 0 ||
                            blipRect.left != 0 || blipRect.right != 0)
                            FrameRect(&blipRect);
                    } else {
                        if (hasIFF != 0) {
                            ColorCodeStellar(&stellarData[stellar], &color);
                            RGBForeColor(&color);
                        } else {
                            RGBForeColor(radarStatusColor[0]);
                        }
                        size = 0x20;
                        if (masterStellarSprite[stellarData[stellar].type] != NULL &&
                            masterStellarSprite[stellarData[stellar].type]->frameArray != NULL)
                            size = GetSpriteXSize(masterStellarSprite[stellarData[stellar].type]);
                        if (size < 200) {
                            if (size >= 90)
                                InsetRect(&blipRect, -2, -2);
                            else
                                InsetRect(&blipRect, -1, -1);
                        } else {
                            InsetRect(&blipRect, -3, -3);
                        }
                        SectRect(&blipRect, &dstRect, &blipRect);
                        if (blipRect.left != 0 || blipRect.top != 0 ||
                            blipRect.left != 0 || blipRect.right != 0)
                            FrameOval(&blipRect);
                    }
                }
            }
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active != 0 &&
                    shipData[0].curSystem == shipData[i].curSystem) {
                    if (ShipIsCloaked(&shipData[i]) == 0) {
                        visible = 1;
                    } else if (ShipCloakVisibleOnRadar(&shipData[i]) == 0 &&
                               shipData[0].seesCloakedRadar != 1) {
                        visible = 0;
                    } else {
                        visible = 1;
                    }
                    if (visible) {
                        blipRect.left = blipRect.right =
                            (cx + (long)(shipData[i].pos.x -
                                         shipData[0].pos.x) * kRadarScale);
                        blipRect.top = blipRect.bottom =
                            (cy + (long)(shipData[i].pos.y -
                                         shipData[0].pos.y) * kRadarScale);
                        if (shipData[0].targetShip == i) {
                            if (radarCount != 0) {
                                if (hasIFF != 0) {
                                    RGBForeColor(&rgbLightGrey);
                                } else {
                                    RGBForeColor(radarStatusColor[1]);
                                }
                            } else if (hasIFF != 0) {
                                ColorCodeShip(&shipData[i], &color);
                                RGBForeColor(&color);
                            } else {
                                RGBForeColor(radarStatusColor[0]);
                            }
                        } else if (hasIFF != 0) {
                            ColorCodeShip(&shipData[i], &color);
                            RGBForeColor(&color);
                        } else {
                            RGBForeColor(radarStatusColor[0]);
                        }
                        if (hasDensity == 0 ||
                            gShipTypes[shipData[i].type].mass < 100) {
                            if (blipRect.left >= dstRect.left &&
                                blipRect.left <= dstRect.right &&
                                blipRect.top >= dstRect.top &&
                                blipRect.top <= dstRect.bottom) {
                                MoveTo(blipRect.left, blipRect.top);
                                Line(0, 0);
                            }
                        } else {
                            InsetRect(&blipRect, -1, -1);
                            SectRect(&blipRect, &dstRect, &blipRect);
                            if (blipRect.left != 0 || blipRect.top != 0 ||
                                blipRect.left != 0 || blipRect.right != 0)
                                FrameRect(&blipRect);
                        }
                    }
                }
            }
        }
    }
    if (hasIFF != 0) {
        ColorCodeShip(shipData, &color);
        RGBForeColor(&color);
    } else {
        RGBForeColor(radarStatusColor[1]);
    }
    MoveTo(cx, cy);
    Line(0, 0);
    if (radarCount != 0 && shipData[0].navTargetKind != 3) {
        links = 0;
        for (i = 0; i < kNavSlots; i++) {
            if (gSystems[shipData[0].curSystem].navDefault[i] != -1)
                links++;
        }
        if (links > 0) {
            tip.x = tip.y = 0.0f;
            if (XYFloatSquaredDist(&shipData[0].pos, &tip) > kRadarArrowDist) {
                angle = CalcInterceptAngle(&shipData[0].pos, &tip);
                tip.x = barb.x = cx;
                tip.y = barb.y = cy;
                Accel(angle, 25.0f, &tip);
                Accel(angle, 50.0f, &barb);
                RGBForeColor(radarStatusColor[0]);
                MoveTo(tip.x, tip.y);
                LineTo(barb.x, barb.y);
                tip.x = barb.x;
                tip.y = barb.y;
                Accel((angle + 0x87), 6.0f, &tip);
                MoveTo(barb.x, barb.y);
                LineTo(tip.x, tip.y);
                tip.x = barb.x;
                tip.y = barb.y;
                Accel((angle - 0x87), 6.0f, &tip);
                MoveTo(barb.x, barb.y);
                LineTo(tip.x, tip.y);
                RGBForeColor(blackColor);
            }
        }
    }
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (preventStatusCopy == 0)
        CopyBits(GetGWCPixMap(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                 &dstRect, &dstRect, 0, 0);
}
