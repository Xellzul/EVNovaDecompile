// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
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
#if V111
#else
void DoKeyDialog(void)
{
    Boolean done;
    short itemHit;
    short itemType;
    Handle item;
    Rect box;
    short i;
    short j;
    Boolean matched;
    short localBuf[kBindingCount];
    short defaultBuf[kBindingCount];

    keyDialog = NULL;
    keyDialog = GetNewDialog(0xfa2, KeyDialogUpdate, KeyFilter);
    if (keyDialog == NULL)
        return;

    KeyPrefsToSlotTable();
    if (gWind != NULL)
        CenterWindow(FrontWindow(keyDialog), gWind);
    ShowMacWindow(FrontWindow(keyDialog));
#ifdef MAC1010
    SelectWindow(FrontWindow(keyDialog));
    SetDialogGWorld(keyDialog);
    RedrawDialog(keyDialog);
    keyPic = GetPicture(0x85);
#else
    FrontWindow(keyDialog);
    SetDialogGWorld(keyDialog);
    RedrawDialog(keyDialog);
    keyPic = GetPicture(0x8b);
#endif

    done = 0;
    KeyDialogUpdate();
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(keyDialog, KeyFilter, &itemHit, 0);
        if (itemHit == 1) {
            matched = 0;
            for (j = 0; j < kBindingCount; j++) {
                if (DupKeyCheck(newKeyPrefs[j], j)) {
#ifdef MAC1010
                    GetDialogItem(keyDialog, keyDialogSel + 5, &itemType, &item, &box);
                    InvalWindowRect(GetDialogWindow(keyDialog), &box);
                    keyDialogSel = j;
                    GetDialogItem(keyDialog, j + 5, &itemType, &item, &box);
                    InvalWindowRect(GetDialogWindow(keyDialog), &box);
#else
                    keyDialogSel = j;
                    RedrawDialog(keyDialog);
#endif
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
                localBuf[i] = newKeyPrefs[i];
            SetDefaultKeys();
            KeyPrefsToSlotTable();
            for (i = 0; i < kBindingCount; i++) {
                defaultBuf[i] = newKeyPrefs[i];
                newKeyPrefs[i] = localBuf[i];
            }
            SlotTableToKeyPrefs();
            for (i = 0; i < kBindingCount; i++)
                newKeyPrefs[i] = defaultBuf[i];
            keyDialogSel = 0;
            RedrawDialog(keyDialog);
        }
        if (itemHit >= 5 && itemHit <= 0x27) {
#ifdef MAC1010
            GetDialogItem(keyDialog, keyDialogSel + 5, &itemType, &item, &box);
            InvalWindowRect(GetDialogWindow(keyDialog), &box);
            keyDialogSel = itemHit - 5;
            GetDialogItem(keyDialog, keyDialogSel + 5, &itemType, &item, &box);
            InvalWindowRect(GetDialogWindow(keyDialog), &box);
#else
            keyDialogSel = itemHit - 5;
            RedrawDialog(keyDialog);
#endif
        }
    } while (!done);

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    if (keyPic != NULL)
        QDReleaseResource(keyPic);
    DisposeDialog(keyDialog);
    keyDialog = NULL;
}
#endif

#define kFirstBindingItem   5
#define kNoNameStrIndex  0x119
#define kRowTextInset       5
#define kRowBaseline      0xf
#define kKeyNameStrList     129

/*
 * v and h must be signed `short`, and v must be declared first. PtInRect widens
 * them, and mwcc defers a short local's widening to the use site, so the field
 * read stays a movzx and the widening is a separate movsx hoisted out of the
 * loop.
 */
/* KeyFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b6d0
// MAC: _KeyFilter i386 0x00017482 ppc 0x0001b030
#ifdef MACBUILD
Boolean KeyFilter(DialogPtr dialog, EventRecord *ev, short *itemHit)
{
    short itemType;
    Point where;
    Handle item;
    Rect box;
    int32_t n;
    short i;
    short code;
    short sel;

    SetPort((GrafPtr)GetDialogPort(keyDialog));
    if (ev->what == mouseDown) {
        if (DialogSelect(ev, &keyDialog, itemHit))
            return true;
        SetPort((GrafPtr)GetDialogPort(keyDialog));
        where = ev->where;
        GlobalToLocal(&where);
        *itemHit = -1;
        for (i = 0; i < kBindingCount; i++) {
            GetDialogItem(keyDialog, n = i + kFirstBindingItem, &itemType, &item, &box);
            if (PtInRectPoint(where, &box)) {
                *itemHit = n;
                return true;
            }
        }
    }
#ifndef MAC1010
    if (ev->what == keyDown || ev->what == nullEvent)
#endif
    {
        code = ScanForKeys();
        if (code != -1 && keyDialogSel != -1) {
            newKeyPrefs[keyDialogSel] = code;
            sel = keyDialogSel;
            keyDialogSel = (keyDialogSel + 1) % kBindingCount;
            GetDialogItem(keyDialog, sel + kFirstBindingItem, &itemType, &item, &box);
            InvalWindowRect(GetDialogWindow(keyDialog), &box);
            GetDialogItem(keyDialog, keyDialogSel + kFirstBindingItem, &itemType, &item, &box);
            InvalWindowRect(GetDialogWindow(keyDialog), &box);
            while (KeyCheck(code))
                BusyIdle();
            return true;
        }
    }
    if (ev->what == updateEvt) {
        BeginUpdate(GetDialogWindow(keyDialog));
        KeyDialogUpdate();
        EndUpdate(GetDialogWindow(keyDialog));
        return false;
    }
    return false;
}
#else
Boolean KeyFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    short v;
    short h;
    int i;
    int32_t code;
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
                          (Handle *)&item, &box);
            if (PtInRect(h, v, &box) != 0) {
                *itemHit = (short)i + kFirstBindingItem;
                return 1;
            }
            i = i + 1;
        } while ((short)i < kBindingCount);
        return 0;
    }
    code = FirstKeyDown();
    if ((short)code != -1 && keyDialogSel != -1) {
        newKeyPrefs[keyDialogSel] = code;
        sel = keyDialogSel;
        keyDialogSel = (keyDialogSel + 1) % kBindingCount;
        InvalDialogItem(keyDialog, sel + kFirstBindingItem);
        InvalDialogItem(keyDialog, keyDialogSel + kFirstBindingItem);
        while (KeyCheck(code))
            BusyIdle();
        return 1;
    }
    if ((int)ev->what != kUpdateEvt)
        return 0;
    KeyDialogUpdate();
    return 0;
}
#endif

/* KeyDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b860
// MAC: _KeyDialogUpdate i386 0x00017199 ppc 0x0001adf8
#ifdef MACBUILD
void KeyDialogUpdate(void)
{
    short itemType;
    Handle item;
    Rect itemBox;
    Rect fill;
    Rect picBox;
    Str255 name;
    short i;

    SetPort((GrafPtr)GetDialogPort(keyDialog));
    GetDialogItem(keyDialog, 4, &itemType, &item, &picBox);
#ifdef MAC1010
    if (gWind != NULL)
        SWSetPortToWorkArea(gSpriteWorld);
#endif
    ForeColor(blackColor);
    TextFont(0);
    TextSize(12);
    DrawPicture(keyPic, &picBox);
    for (i = 0; i < kBindingCount; i++) {
        GetDialogItem(keyDialog, i + kFirstBindingItem, &itemType, &item, &itemBox);
        if (RectInRgn(&itemBox, GetWindowVisRgn(GetDialogWindow(keyDialog)))) {
            if (keyDialogSel == i) {
                fill = itemBox;
                InsetRect(&fill, 1, 1);
                ForeColor(whiteColor);
                FrameRect(&itemBox);
                InsetRect(&fill, 1, 1);
                ForeColor(blackColor);
                PaintRect(&fill);
                ForeColor(blackColor);
                FrameRect(&itemBox);
                ForeColor(whiteColor);
                MoveTo(itemBox.left + kRowTextInset, itemBox.top + kRowBaseline);
            } else {
                fill = itemBox;
                InsetRect(&fill, 1, 1);
                ForeColor(whiteColor);
                PaintRect(&fill);
                ForeColor(blackColor);
                FrameRect(&itemBox);
                ForeColor(blackColor);
                MoveTo(itemBox.left + kRowTextInset, itemBox.top + kRowBaseline);
            }
            GetIndString(name, kKeyNameStrList, newKeyPrefs[i] + 1);
            if (name[0] == 0)
                DrawIndString(kGameStrList, kNoNameStrIndex);
            else
                DrawString(name);
        }
    }
    ForeColor(blackColor);
#ifdef MAC1010
    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
#endif
    SetPort((GrafPtr)GetDialogPort(keyDialog));
    ForeColor(blackColor);
#ifdef MAC1010
    if (gWind != NULL)
        CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                 GetPortBitMapForCopyBits(GetDialogPort(keyDialog)),
                 &picBox, &picBox, 0, NULL);
#endif
    DrawDialog(keyDialog);
}
#else
void KeyDialogUpdate(void)
{
    short itemType;
    void *item;
    Rect picBox;
    char name[256];
    Rect fill;
    Rect itemBox;
    Boolean inside;
    int i;

    SetDialogGWorld(keyDialog);
    GetDialogItem(keyDialog, 4, &itemType, (Handle *)&item, &picBox);
    SWSetPortToWorkArea(gSpriteWorld);
    ForeColor(blackColor);
    TextFont(0);
    TextSize(12);
    DrawPicHandle(keyPic, &picBox);
    i = 0;
    do {
        GetDialogItem(keyDialog, (short)i + kFirstBindingItem, &itemType, (Handle *)&item,
                      &itemBox);
        inside = RectInRgn(&itemBox, GetCWindowVisRgn(FrontWindow(keyDialog)));
        if (inside) {
            if (keyDialogSel == (short)i) {
                fill.top = itemBox.top;
                fill.left = itemBox.left;
                fill.bottom = itemBox.bottom;
                fill.right = itemBox.right;
                InsetRect(&fill, 1, 1);
                ForeColor(whiteColor);
                FrameRect(&itemBox);
                InsetRect(&fill, 1, 1);
                ForeColor(blackColor);
                PaintRect(&fill);
                ForeColor(blackColor);
                FrameRect(&itemBox);
                ForeColor(whiteColor);
                MoveTo(itemBox.left + kRowTextInset,
                       itemBox.top + kRowBaseline);
            } else {
                fill.top = itemBox.top;
                fill.left = itemBox.left;
                fill.bottom = itemBox.bottom;
                fill.right = itemBox.right;
                InsetRect(&fill, 1, 1);
                ForeColor(whiteColor);
                PaintRect(&fill);
                ForeColor(blackColor);
                FrameRect(&itemBox);
                ForeColor(blackColor);
                MoveTo(itemBox.left + kRowTextInset,
                       itemBox.top + kRowBaseline);
            }
            KeyboardScanCodeToString(name, 0x100, newKeyPrefs[(short)i]);
            CtoPstr(name);
            if (name[0] == 0)
                DrawIndString(kGameStrList, kNoNameStrIndex);
            else
                DrawString((unsigned char *)name);
        }
        i = i + 1;
    } while ((short)i < kBindingCount);
    ForeColor(blackColor);
    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(keyDialog);
    ForeColor(blackColor);
    CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(GetDialogPort(keyDialog)),
             &picBox, &picBox, 0, NULL);
}
#endif

#ifdef MACBUILD
/* AdjustMenus (mac _AdjustMenus): mac only. */
// MAC: _AdjustMenus i386 0x000176b4 ppc 0x0001b254
void AdjustMenus(void)
{
    int32_t response;
    MenuHandle fileMenu;
    MenuHandle menu;

    Gestalt(gestaltMenuMgrAttr, &response);
#if TARGET_API_MAC_CARBON
    if (response & gestaltMenuMgrAquaLayoutMask) {
        if (!gameLoaded || creatingPilotFile)
            DisableMenuCommand(NULL, kHICommandPreferences);
        else
            EnableMenuCommand(NULL, kHICommandPreferences);
    } else
#endif
    {
        fileMenu = GetMenuHandle(kFileMenu);
        if (fileMenu != NULL) {
            if (!gameLoaded || creatingPilotFile)
                DisableMenuItem(fileMenu, 1);
            else
                EnableMenuItem(fileMenu, 1);
        }
    }
    menu = GetMenuHandle(kAppleMenu);
    if (menu != NULL) {
        if (!gameLoaded || !gPause || creatingPilotFile)
            DisableMenuItem(menu, kAboutItem);
        else
            EnableMenuItem(menu, kAboutItem);
    }
}
#endif
