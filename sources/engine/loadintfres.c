// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* loadintfres.c -- LoadIntfResource (win 0x004cda50), the status-bar-panel loader called once from EnterGameFromMainScreen. Notes: Research/notes/loadintfres.md */
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "render.h"
#include "burger.h"
#include "cefix.h"


#if V111 /* V111 spelling */
#define kIntfRec (*(IntfRec **)intfFileHand)
#define GovtInterfaceID(g) \
    id = govtData[g].interfaceID; \
    if (id < kFirstResourceID) \
        id = kFirstResourceID
#define CopyStatusRect(n, area) \
    statusOffRect[n] = (*rec)->area
#else
#define kIntfRec (*rec)
#define GovtInterfaceID(g) \
    pid = govtData[g].interfaceID; \
    if (pid >= kFirstResourceID) \
        id = pid
#ifdef MAC1010
#define CopyStatusRect(n, area) \
    statusOffRect[n] = (*rec)->area
#else
#define CopyStatusRect(n, area) \
    p = &(*rec)->area; \
    statusOffRect[n].top = p->top; \
    statusOffRect[n].left = p->left; \
    statusOffRect[n].bottom = p->bottom; \
    statusOffRect[n].right = p->right
#endif
#endif

/* LoadIntfResource (mac _LoadIntfResource). STATE: MATCH 100.0% (269 exact, 94
   reloc-equivalent), 363/363, byte-identical yes, relocregcheck 0 of 94. OPEN: none. */
// FUNCTION: EVNOVA 0x004cda50
// MAC: _LoadIntfResource i386 0x000735f2 ppc 0x0007c470
void LoadIntfResource(void)
{
    short type;
    short govt;
    short id;
    short pid;
    short picID;
    PicHandle pict;
    Rect *p;
    IntfRec **rec;
    IntfRec **rec2;
    Rect pictRect;
    char fontName[256];

    id = kFirstResourceID;
    type = shipData->type;
    if (type >= 0 && type < kMaxShipTypes) {
        govt = shipClass[type].inherentGovtB;
        if (govt != -1) {
            GovtInterfaceID(govt);
        } else {
            govt = shipClass[type].inherentGovt;
            if (govt != -1) {
                GovtInterfaceID(govt);
            }
        }
    }
    if (id != lastIntfResourceID) {
    intfFileHand = GetResource(kIntfType, id);
    if (intfFileHand != NULL) {
        LockAHandle(intfFileHand);
        rec = (IntfRec **)intfFileHand;

        statusColor[0].blue = (kIntfRec->brightText.rgb & 0xff) * 256;
        statusColor[0].green = (kIntfRec->brightText.rgb >> 8 & 0xff) * 256;
        statusColor[0].red = (kIntfRec->brightText.rgb >> 16 & 0xff) * 256;
        statusColor[1].blue = (kIntfRec->dimText.rgb & 0xff) * 256;
        statusColor[1].green = (kIntfRec->dimText.rgb >> 8 & 0xff) * 256;
        statusColor[1].red = (kIntfRec->dimText.rgb >> 16 & 0xff) * 256;
        statusColor[2].blue = (kIntfRec->brightRadar.rgb & 0xff) * 256;
        statusColor[2].green = (kIntfRec->brightRadar.rgb >> 8 & 0xff) * 256;
        statusColor[2].red = (kIntfRec->brightRadar.rgb >> 16 & 0xff) * 256;
        statusColor[3].blue = (kIntfRec->dimRadar.rgb & 0xff) * 256;
        statusColor[3].green = (kIntfRec->dimRadar.rgb >> 8 & 0xff) * 256;
        statusColor[3].red = (kIntfRec->dimRadar.rgb >> 16 & 0xff) * 256;
        statusColor[4].blue = (kIntfRec->shieldColor.rgb & 0xff) * 256;
        statusColor[4].green = (kIntfRec->shieldColor.rgb >> 8 & 0xff) * 256;
        statusColor[4].red = (kIntfRec->shieldColor.rgb >> 16 & 0xff) * 256;
        statusColor[5].blue = (kIntfRec->armorColor.rgb & 0xff) * 256;
        statusColor[5].green = (kIntfRec->armorColor.rgb >> 8 & 0xff) * 256;
        statusColor[5].red = (kIntfRec->armorColor.rgb >> 16 & 0xff) * 256;
        statusColor[6].blue = (kIntfRec->fuelFull.rgb & 0xff) * 256;
        statusColor[6].green = (kIntfRec->fuelFull.rgb >> 8 & 0xff) * 256;
        statusColor[6].red = (kIntfRec->fuelFull.rgb >> 16 & 0xff) * 256;
        statusColor[7].blue = (kIntfRec->fuelPartial.rgb & 0xff) * 256;
        statusColor[7].green = (kIntfRec->fuelPartial.rgb >> 8 & 0xff) * 256;
        statusColor[7].red = (kIntfRec->fuelPartial.rgb >> 16 & 0xff) * 256;

#if V111 /* V111 spelling */
        rec = (IntfRec **)intfFileHand;
#endif
        CopyStatusRect(0, radarArea);
        CopyStatusRect(1, shieldArea);
        CopyStatusRect(2, armorArea);
        CopyStatusRect(3, fuelArea);
        CopyStatusRect(4, navArea);
        CopyStatusRect(5, weapArea);
        CopyStatusRect(6, targArea);
        CopyStatusRect(7, cargoArea);

        strcpy(fontName, (*rec)->statusFont);
        CtoPstr(fontName);
        GetFNum((StringPtr)fontName, &statusFontID);

        rec2 = (IntfRec **)intfFileHand;
        statusFontSize[0] = (*rec2)->statFontSize[0];
        statusFontSize[1] = (*rec2)->statFontSize[1];

        picID = (*rec2)->statusBkgnd;
        if (picID < kFirstResourceID)
            picID = kFirstResourceID;

#ifdef CEFIX /* F12.84 */
        CE_ScaleStatusRects();
#endif
#ifdef MACBUILD
        HUnlock((Handle)rec2);
#else
        UnlockAHandle(intfFileHand);
#endif
#ifdef MACBUILD
        HPurge(intfFileHand);
#endif
        ReleaseResource(intfFileHand);
    } else {
        picID = kFirstResourceID;
    }

    pict = GetPicture(picID);
    if (pict != NULL) {
        SetGWorldContainer(&statusOffPort);
#ifdef MACBUILD
        ForeColor(blackColor);
#else
        RGBForeColor(&rgbBlack);
#endif
        PaintRect(&statusOffPort.bounds);
        QDGetPictureBounds(pict, &pictRect);
        OffsetRect(&pictRect, -pictRect.left, -pictRect.top);
#ifdef CEFIX /* F12.85 */
        CE_ScaleOffsetRect(&pictRect, -statusOffPort.bounds.left, -statusOffPort.bounds.top);
#else
        OffsetRect(&pictRect, -statusOffPort.bounds.left, -statusOffPort.bounds.top);
#endif
        DrawPicHandle(pict, &pictRect);
#ifdef MACBUILD
        HPurge((Handle)pict);
#endif
        QDReleaseResource(pict);
        UnsetGWorldContainer(&statusOffPort);
        RedrawStatus(1);
    }
    lastIntfResourceID = id;
    }
}

#ifdef MACBUILD
void DumpPilotLog(void)
#include "../engine/dumppilotlog.inc"

/* ColorSearchProc (mac _ColorSearchProc): mac only. */
// MAC: _ColorSearchProc i386 0x000767b4 ppc 0x0007f604
Boolean ColorSearchProc(RGBColor *rgb, int32_t *position)
{
    CTabHandle ctab = GetCTable(40);
    Rect bounds = { 0, 0, 10, 10 };
    GWorldPtr world;
    GWorldPtr savedWorld;
    GetGWorld(&savedWorld, NULL);
    (NewGWorld)(&world, 16, &bounds, ctab, NULL, kNativeEndianPixMap);
    SetGWorld(world, NULL);
    *position = Color2Index(rgb);
    SetGWorld(savedWorld, NULL);
    DisposeGWorld(world);
    DisposeCTable(ctab);
    return 1;
}
#endif
