// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* loadintfres.c -- LoadIntfResource (win 0x004cda50), the status-bar-panel loader called once from EnterGameFromMainScreen. Notes: Research/notes/loadintfres.md */
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "render.h"
#include "burger.h"


/* one packed 24-bit colour, 3 bytes used of 4 (matches the resource's own
   byte order -- see the file header on why b0/b1/b2 write in 2-1-0 order) */
typedef struct RawTriple {
    unsigned short w;   /* +0 word view (byte0's read width) */
    unsigned short pad; /* +2 keeps the 4-byte stride the resource actually uses */
} RawTriple;

/* the "ïntf" resource's runtime layout; only the fields this function reads
   are named, in the exact (irregular) order the exe reads them */
typedef struct IntfRec {
    RawTriple color0;        /* +0x00 -> statusColor[0] */
    RawTriple color1;        /* +0x04 -> statusColor[1] */
    Rect      rect0;         /* +0x08 -> statusOffRect[0] */
    RawTriple color2;        /* +0x10 -> statusColor[2] */
    RawTriple color3;        /* +0x14 -> statusColor[3] */
    Rect      rect1;         /* +0x18 -> statusOffRect[1] */
    RawTriple color4;        /* +0x20 -> statusColor[4] */
    Rect      rect2;         /* +0x24 -> statusOffRect[2] */
    RawTriple color5;        /* +0x2c -> statusColor[5] */
    Rect      rect3;         /* +0x30 -> statusOffRect[3] */
    RawTriple color6;        /* +0x38 -> statusColor[6] */
    RawTriple color7;        /* +0x3c -> statusColor[7] */
    Rect      rect4;         /* +0x40 -> statusOffRect[4] */
    Rect      rect5;         /* +0x48 -> statusOffRect[5] */
    Rect      rect6;         /* +0x50 -> statusOffRect[6] */
    Rect      rect7;         /* +0x58 -> statusOffRect[7] */
    char      fontName[0x40]; /* +0x60 C string, CtoPstr'd after copy-out */
    short     fontSize0;      /* +0xa0 */
    short     fontSize1;      /* +0xa2 */
    short     pictID;         /* +0xa4 clamped up to kFirstResourceID */
} IntfRec;

/* LoadIntfResource (mac _LoadIntfResource). STATE: MATCH 100.0% (269 exact, 94
   reloc-equivalent), 363/363, byte-identical yes, relocregcheck 0 of 94. OPEN: none. */
// FUNCTION: EVNOVA 0x004cda50
void LoadIntfResource(void)
{
    short type;
    short govt;
    short id;
    short pid;
    short picID;
    void *pict;
    Rect *p;
    IntfRec **rec;
    IntfRec **rec2;
    Rect pictRect;
    Str255 fontName;

    id = kFirstResourceID;
    type = shipData->type;
    if (type >= 0 && type < kMaxShipTypes) {
        govt = gShipTypes[type].inherentGovtB;
        if (govt != -1) {
            pid = govtData[govt].interfaceID;
            if (pid >= kFirstResourceID)
                id = pid;
        } else {
            govt = gShipTypes[type].inherentGovt;
            if (govt != -1) {
                pid = govtData[govt].interfaceID;
                if (pid >= kFirstResourceID)
                    id = pid;
            }
        }
    }
    if (id != lastIntfResourceID) {
    gIntfFileHand = GetResource(kIntfType, id);
    if (gIntfFileHand != NULL) {
        LockAHandle(gIntfFileHand);
        rec = (IntfRec **)gIntfFileHand;

        statusColor[0].blue = ((*rec)->color0.w & 0xff) << 8;
        statusColor[0].green = (unsigned char)(*(unsigned long *)&(*rec)->color0.w >> 8) << 8;
        statusColor[0].red = (unsigned char)(*(unsigned long *)&(*rec)->color0.w >> 16) << 8;

        statusColor[1].blue = ((*rec)->color1.w & 0xff) << 8;
        statusColor[1].green = (unsigned char)(*(unsigned long *)&(*rec)->color1.w >> 8) << 8;
        statusColor[1].red = (unsigned char)(*(unsigned long *)&(*rec)->color1.w >> 16) << 8;

        statusColor[2].blue = ((*rec)->color2.w & 0xff) << 8;
        statusColor[2].green = (unsigned char)(*(unsigned long *)&(*rec)->color2.w >> 8) << 8;
        statusColor[2].red = (unsigned char)(*(unsigned long *)&(*rec)->color2.w >> 16) << 8;

        statusColor[3].blue = ((*rec)->color3.w & 0xff) << 8;
        statusColor[3].green = (unsigned char)(*(unsigned long *)&(*rec)->color3.w >> 8) << 8;
        statusColor[3].red = (unsigned char)(*(unsigned long *)&(*rec)->color3.w >> 16) << 8;

        statusColor[4].blue = ((*rec)->color4.w & 0xff) << 8;
        statusColor[4].green = (unsigned char)(*(unsigned long *)&(*rec)->color4.w >> 8) << 8;
        statusColor[4].red = (unsigned char)(*(unsigned long *)&(*rec)->color4.w >> 16) << 8;

        statusColor[5].blue = ((*rec)->color5.w & 0xff) << 8;
        statusColor[5].green = (unsigned char)(*(unsigned long *)&(*rec)->color5.w >> 8) << 8;
        statusColor[5].red = (unsigned char)(*(unsigned long *)&(*rec)->color5.w >> 16) << 8;

        statusColor[6].blue = ((*rec)->color6.w & 0xff) << 8;
        statusColor[6].green = (unsigned char)(*(unsigned long *)&(*rec)->color6.w >> 8) << 8;
        statusColor[6].red = (unsigned char)(*(unsigned long *)&(*rec)->color6.w >> 16) << 8;

        statusColor[7].blue = ((*rec)->color7.w & 0xff) << 8;
        statusColor[7].green = (unsigned char)(*(unsigned long *)&(*rec)->color7.w >> 8) << 8;
        statusColor[7].red = (unsigned char)(*(unsigned long *)&(*rec)->color7.w >> 16) << 8;

        p = &(*rec)->rect0;
        statusOffRect[0].top = p->top;
        statusOffRect[0].left = p->left;
        statusOffRect[0].bottom = p->bottom;
        statusOffRect[0].right = p->right;

        p = &(*rec)->rect1;
        statusOffRect[1].top = p->top;
        statusOffRect[1].left = p->left;
        statusOffRect[1].bottom = p->bottom;
        statusOffRect[1].right = p->right;

        p = &(*rec)->rect2;
        statusOffRect[2].top = p->top;
        statusOffRect[2].left = p->left;
        statusOffRect[2].bottom = p->bottom;
        statusOffRect[2].right = p->right;

        p = &(*rec)->rect3;
        statusOffRect[3].top = p->top;
        statusOffRect[3].left = p->left;
        statusOffRect[3].bottom = p->bottom;
        statusOffRect[3].right = p->right;

        p = &(*rec)->rect4;
        statusOffRect[4].top = p->top;
        statusOffRect[4].left = p->left;
        statusOffRect[4].bottom = p->bottom;
        statusOffRect[4].right = p->right;

        p = &(*rec)->rect5;
        statusOffRect[5].top = p->top;
        statusOffRect[5].left = p->left;
        statusOffRect[5].bottom = p->bottom;
        statusOffRect[5].right = p->right;

        p = &(*rec)->rect6;
        statusOffRect[6].top = p->top;
        statusOffRect[6].left = p->left;
        statusOffRect[6].bottom = p->bottom;
        statusOffRect[6].right = p->right;

        p = &(*rec)->rect7;
        statusOffRect[7].top = p->top;
        statusOffRect[7].left = p->left;
        statusOffRect[7].bottom = p->bottom;
        statusOffRect[7].right = p->right;

        strcpy((char *)fontName, (*rec)->fontName);
        CtoPstr((char *)fontName);
        GetFNum(fontName, &statusFontID);

        rec2 = (IntfRec **)gIntfFileHand;
        statusFontSize[0] = (*rec2)->fontSize0;
        statusFontSize[1] = (*rec2)->fontSize1;

        picID = (*rec2)->pictID;
        if (picID < kFirstResourceID)
            picID = kFirstResourceID;

        UnlockAHandle(gIntfFileHand);
        ReleaseResource(gIntfFileHand);
    } else {
        picID = kFirstResourceID;
    }

    pict = GetPicture(picID);
    if (pict != NULL) {
        SetGWorldContainer(&statusOffPort);
        RGBForeColor(&rgbBlack);
        PaintRect(&statusOffPort.bounds);
        QDGetPictureBounds(pict, &pictRect);
        OffsetRect(&pictRect, -pictRect.left, -pictRect.top);
        OffsetRect(&pictRect, -statusOffPort.bounds.left, -statusOffPort.bounds.top);
        DrawPicHandle(pict, &pictRect);
        QDReleaseResource(pict);
        UnsetGWorldContainer(&statusOffPort);
        RedrawStatus(1);
    }
    lastIntfResourceID = id;
    }
}
