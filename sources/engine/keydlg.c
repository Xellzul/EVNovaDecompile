// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* keydlg.c -- the key-binding dialog's update proc (win 0x0048b860). Notes: Research/notes/keydlg.md */
/*
 * picBox must be declared before name: with a large array in the frame mwcc
 * orders stack slots by reference count, and picBox/name tie at 4, so the
 * declaration order is what breaks the tie and gives name the +0x08 slot.
 */
#include <stddef.h>

#include "macport.h"
#include "dialogs.h"
#include "game.h"
#include "player.h"
#include "render.h"
#include "gametypes.h"



/* both take an argument in this TU that their own bodies ignore */

#define kBindingCount 0x22


/* DoKeyDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b280
void DoKeyDialog(void)
{
    char done;
    short itemHit;
    char matched;
    short i;
    short j;
    short localBuf[kBindingCount];
    short defaultBuf[kBindingCount];

    keyDialog = NULL;
    keyDialog = GetNewDialog(0xfa2, UpdateKeyDialog, KeyDialogEvent);
    if (keyDialog == NULL)
        return;

    KeyPrefsToSlotTable();
    if (gWind != NULL)
        CenterWindow(FrontWindow(keyDialog), gWind);
    ShowMacWindow(FrontWindow(keyDialog));
    FrontWindow(keyDialog);
    SetDialogGWorld(keyDialog);
    RedrawDialog(keyDialog);
    keyDialogPict = GetPicture(0x8b);

    done = 0;
    UpdateKeyDialog();
    do {
        if (gQuit)
            done = 1;
        RunModalDialog(keyDialog, KeyDialogEvent, &itemHit, 0);
        if (itemHit == 1) {
            matched = 0;
            for (j = 0; j < kBindingCount; j++) {
                if (IsKeySlotInUse(gSlotTable[j], j)) {
                    gSelectedBinding = j;
                    RedrawDialog(keyDialog);
                    SysBeep(0);
                    matched = 1;
                    break;
                }
            }
            if (!matched) {
                SlotTableToKeyPrefs();
                done = 1;
                SavePrefs();
            }
        }
        if (itemHit == 2)
            done = 1;
        if (itemHit == 3) {
            KeyPrefsToSlotTable();
            for (i = 0; i < kBindingCount; i++)
                localBuf[i] = gSlotTable[i];
            SetDefaultKeys();
            KeyPrefsToSlotTable();
            for (j = 0; j < kBindingCount; j++) {
                defaultBuf[j] = gSlotTable[j];
                gSlotTable[j] = localBuf[j];
            }
            SlotTableToKeyPrefs();
            for (j = 0; j < kBindingCount; j++)
                gSlotTable[j] = defaultBuf[j];
            gSelectedBinding = 0;
            RedrawDialog(keyDialog);
        }
        if (itemHit >= 5 && itemHit <= 0x27) {
            gSelectedBinding = itemHit - 5;
            RedrawDialog(keyDialog);
        }
    } while (!done);

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    if (keyDialogPict != NULL)
        QDReleaseResource(keyDialogPict);
    DisposeDialog(keyDialog);
    keyDialog = NULL;
}

#define kFirstBindingItem   5
#define kNoNameStrIndex  0x119
#define kRowTextInset       5
#define kRowBaseline      0xf

/*
 * v and h must be signed `short`, and v must be declared first. PtInRect widens
 * them, and mwcc defers a short local's widening to the use site, so the field
 * read stays a movzx and the widening is a separate movsx hoisted out of the
 * loop.
 */
/* KeyDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b6d0
char KeyDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    short v;
    short h;
    int i;
    long code;
    short sel;

    SetDialogGWorld(keyDialog);
    if ((int)ev->what == kMouseDown) {
        SetDialogGWorld(keyDialog);
        v = ev->where.v;
        h = ev->where.h;
        *itemHit = -1;
        i = 0;
        do {
            GetDialogItem(keyDialog, (short)i + kFirstBindingItem, &itemType,
                          &item, &box);
            if (PtInRect(h, v, &box) != 0) {
                *itemHit = (short)i + kFirstBindingItem;
                return 1;
            }
            i = i + 1;
        } while ((short)i < kBindingCount);
        return 0;
    }
    code = FirstKeyDown();
    if ((short)code != -1 && gSelectedBinding != -1) {
        gSlotTable[gSelectedBinding] = code;
        sel = gSelectedBinding;
        gSelectedBinding = (gSelectedBinding + 1) % kBindingCount;
        InvalDialogItem(keyDialog, sel + kFirstBindingItem);
        InvalDialogItem(keyDialog, gSelectedBinding + kFirstBindingItem);
        while (KeyCheck(code))
            BusyIdle();
        return 1;
    }
    if ((int)ev->what != kUpdateEvt)
        return 0;
    UpdateKeyDialog();
    return 0;
}

/* UpdateKeyDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b860
void UpdateKeyDialog(void)
{
    short itemType;
    void *item;
    Rect picBox;
    char name[256];
    Rect fill;
    Rect itemBox;
    char inside;
    int i;

    SetDialogGWorld(keyDialog);
    GetDialogItem(keyDialog, 4, &itemType, &item, &picBox);
    SWSetPortToWorkArea(gSpriteWorld);
    RGBForeColor(blackColor);
    TextFont(0);
    TextSize(12);
    DrawPicHandle(keyDialogPict, &picBox);
    i = 0;
    do {
        GetDialogItem(keyDialog, (short)i + kFirstBindingItem, &itemType, &item,
                      &itemBox);
        inside = RectInRgn(&itemBox, GetCWindowVisRgn(FrontWindow(keyDialog)));
        if (inside) {
            if (gSelectedBinding == (short)i) {
                fill.top = itemBox.top;
                fill.left = itemBox.left;
                fill.bottom = itemBox.bottom;
                fill.right = itemBox.right;
                InsetRect(&fill, 1, 1);
                RGBForeColor(whiteColor);
                FrameRect(&itemBox);
                InsetRect(&fill, 1, 1);
                RGBForeColor(blackColor);
                PaintRect(&fill);
                RGBForeColor(blackColor);
                FrameRect(&itemBox);
                RGBForeColor(whiteColor);
                MoveTo(itemBox.left + kRowTextInset,
                       itemBox.top + kRowBaseline);
            } else {
                fill.top = itemBox.top;
                fill.left = itemBox.left;
                fill.bottom = itemBox.bottom;
                fill.right = itemBox.right;
                InsetRect(&fill, 1, 1);
                RGBForeColor(whiteColor);
                PaintRect(&fill);
                RGBForeColor(blackColor);
                FrameRect(&itemBox);
                RGBForeColor(blackColor);
                MoveTo(itemBox.left + kRowTextInset,
                       itemBox.top + kRowBaseline);
            }
            KeyboardScanCodeToString(name, 0x100, gSlotTable[(short)i]);
            CtoPstr(name);
            if (name[0] == 0)
                DrawIndString(kGameStrList, kNoNameStrIndex);
            else
                DrawString((unsigned char *)name);
        }
        i = i + 1;
    } while ((short)i < kBindingCount);
    RGBForeColor(blackColor);
    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(keyDialog);
    RGBForeColor(blackColor);
    CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(GetDialogPort(keyDialog)),
             &picBox, &picBox, 0, NULL);
}
