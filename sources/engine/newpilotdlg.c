// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* newpilotdlg.c -- the New Pilot confirm/abort gate (win 0x0048a7e0). Notes: Research/notes/newpilotdlg.md */
#include <stddef.h>

#include "burger.h"
#include "macport.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "gametypes.h"

EXTERN_C_BEGIN

#define kNewPilotDlogID          0xc1d
#define kNewPilotOneCharDlogID   0xc1e
#define kItemOK                      1
#define kItemCancel                  2
#define kItemStrictPlay              4
#define kItemFirstNameField          8
#define kItemGenderPopup            11
#define kItemCharPopup              13
#define kNameFieldCount              2
#define kNameFieldStride         0x100
#define kMaxPilotNameLen          0x18
#define kSelectAllNameText        0xfe
#define kTempMenuID             5000


/* DoNewPilotNameDialog (mac _DoNewPilotNameDialog). STATE: MATCH 100%, exe 413 / obj 413,
   byte-identical. The selection loop is a do/while under its own `n > 0' guard.
   Mac MATCH both slices. OPEN: none */
// FUNCTION: EVNOVA 0x0048a7e0
// MAC: _DoNewPilotNameDialog i386 0x000165cc ppc 0x0001a3ec
#ifdef MACBUILD
Boolean DoNewPilotNameDialog(Boolean *strictPlay)
{
#ifdef MAC1010
    short itemHit;
    short type;
    short resID;
    Handle h;
    ResType resType;
    ControlRef popup;
    Rect box;
    Str255 savedName;
    Str255 name;
    short i;
    short selected;
    DialogPtr dlg;
    Boolean result;
    Boolean strict;
    Boolean done;
    short n;
    short count;
    MenuHandle popupMenu;
    short itemCount;
    short len;
    ControlHandle ctl;
    Handle res;
    unsigned char *p;
    MenuHandle menu;
#else
    short type;
    short itemHit;
    short resID;
    Handle h;
    ControlRef popup;
    ResType resType;
    Rect box;
    Str255 savedName;
    Str255 name;
    short n;
    Boolean result;
    Boolean strict;
    MenuHandle menu;
    MenuHandle popupMenu;
    short itemCount;
    short i;
    short count;
    short selected;
    short len;
    Boolean done;
    ControlHandle ctl;
    DialogPtr dlg;
    Handle res;
    unsigned char *p;
#endif

    done = 0;
    strict = *strictPlay;
    n = CountResources(kCharType);
    if (n > 1) {
#ifdef MAC1010
        SetResLoad(false);
#endif
        count = 0;
        for (i = 0; i < n; i++) {
            res = GetIndResource(kCharType, (i + 1));
            if (res != NULL) {
                GetResInfo(res, &resID, &resType, savedName);
                if (savedName[1] != '.')
                    count++;
                ReleaseResource(res);
            }
        }
#ifdef MAC1010
        SetResLoad(true);
#endif
    } else {
        count = 0;
    }
    if (count > 1)
        dlg = GetNewDialog(kNewPilotDlogID, NULL, NULL);
    else
        dlg = GetNewDialog(kNewPilotOneCharDlogID, NULL, NULL);
    if (dlg == NULL)
        return 0;

    SetDialogDefaultItem(dlg, 1);
    CenterWindow(FrontWindow(dlg), gWind);
    ShowMacWindow(FrontWindow(dlg));
    SelectWindow(FrontWindow(dlg));
    SetDialogGWorld(dlg);
    DrawDialog(dlg);
    FrameDefaultButton(dlg, 1);

    GetDialogItem(dlg, kItemStrictPlay, &type, &h, &box);
    ctl = (ControlHandle)h;
    if (strict != 0)
        SetControlValue(ctl, 1);
    else
        SetControlValue(ctl, 0);

    for (i = 0; i < kNameFieldCount; i++) {
        GetDialogItem(dlg, i + kItemFirstNameField, &type, &h, &box);
        SetDialogItemText(h, newPlayerName[i]);
    }
    SelectDialogItemText(dlg, kItemFirstNameField, 0, kSelectAllNameText);
    FlushEvents(everyEvent - diskMask, 0);

    GetDefaultCharResourceName(savedName);
    selected = 1;
    if (savedName[0] != 0) {
        PtoCstr(savedName);
        menu = NewMenu(kTempMenuID, "\ptemp");
        if (menu != NULL) {
            AppendResMenu(menu, kCharType);
            itemCount = CountMenuItems(menu);
            for (i = 0; i < itemCount; i++) {
                GetMenuItemText(menu, i + 1, name);
                PtoCstr(name);
                if (strcmp((char *)name, (char *)savedName) == 0) {
                    selected = i + 1;
                    break;
                }
            }
            DisposeMenu(menu);
        }
    }
    GetDialogItem(dlg, kItemCharPopup, &type, &h, &box);
    SetControlValue((ControlHandle)h, selected);

    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == kItemOK) {
            result = 1;
            done = 1;
            for (i = 0; i < kNameFieldCount; i++) {
                GetDialogItem(dlg, i + kItemFirstNameField, &type, &h, &box);
                GetDialogItemText(h, savedName);
                PtoCstr(savedName);
                len = strlen((char *)savedName);
                if (len > kMaxPilotNameLen) {
                    SysBeep(0);
                    result = 0;
                    done = 0;
                    SelectDialogItemText(dlg, i + kItemFirstNameField, 0,
                                         kMaxPilotNameLen - 1);
                    break;
                }
            }
        }
        if (itemHit == kItemStrictPlay) {
            if (strict != 0)
                strict = 0;
            else
                strict = 1;
            GetDialogItem(dlg, kItemStrictPlay, &type, &h, &box);
            ctl = (ControlHandle)h;
            if (strict != 0)
                SetControlValue(ctl, 1);
            else
                SetControlValue(ctl, 0);
        }
        if (itemHit == kItemCancel) {
            result = 0;
            done = 1;
        }
        itemHit = -1;
    } while (done == 0);

    if (result != 0) {
        for (i = 0; i < kNameFieldCount; i++) {
            GetDialogItem(dlg, i + kItemFirstNameField, &type, &h, &box);
            GetDialogItemText(h, newPlayerName[i]);
        }
        *strictPlay = strict;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        GetDialogItem(dlg, kItemCharPopup, &type, &h, &box);
        popupMenu = *(MenuHandle *)*(*(ControlHandle)h)->contrlData;
        GetMenuItemText(popupMenu, GetControlValue((ControlHandle)h), newPilotPopupChoice[0]);
        GetDialogItem(dlg, kItemGenderPopup, &type, &h, &box);
        popupMenu = *(MenuHandle *)*(*(ControlHandle)h)->contrlData;
        GetMenuItemText(popupMenu, GetControlValue((ControlHandle)h), newPilotPopupChoice[1]);
#else
        GetDialogItemAsControl(dlg, kItemCharPopup, &popup);
        popupMenu = GetMenu(GetControlPopupMenuID(popup));
        if (popupMenu != NULL) {
            AppendResMenu(popupMenu, kCharType);
            GetMenuItemText(popupMenu, GetControlValue(popup), newPilotPopupChoice[0]);
            DeleteMenu(GetControlPopupMenuID(popup));
            DisposeMenu(popupMenu);
        } else {
            pstrcpy(newPilotPopupChoice[0], "\p");
        }
        GetDialogItemAsControl(dlg, kItemGenderPopup, &popup);
        popupMenu = GetMenu(GetControlPopupMenuID(popup));
        if (popupMenu != NULL) {
            GetMenuItemText(popupMenu, GetControlValue(popup), newPilotPopupChoice[1]);
            DeleteMenu(GetControlPopupMenuID(popup));
            DisposeMenu(popupMenu);
        } else {
            pstrcpy(newPilotPopupChoice[1], "\p");
        }
#endif
    }
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(dlg);
    SWSetPortToWindow(gSpriteWorld);
    return result;
}
#else
Boolean DoNewPilotNameDialog(char *strictPlay)
{
    short resID;
    uint32_t resType;
    Str255 name;
    int32_t selected;
    int32_t idx;
    short itemHit;
    Boolean result;
    Boolean done;
    Rect box;
    short type;
    Boolean strict;
    short n;
    Str255 savedName;
    void *h;
    ControlPtr ctl;
    DialogPtr dlg;
    unsigned char *p;
    short i;
    Handle res;
    short count;

    done = 0;
    strict = *strictPlay;
    n = CountResources(kCharType);
    if (n > 1) {
        count = 0;
        for (i = 0; i < n; i++) {
            res = GetIndResource(kCharType, (i + 1));
            if (res != NULL) {
                GetResInfo(res, &resID, &resType, savedName);
                if ((int32_t)savedName[1] != '.')
                    count++;
                ReleaseResource(res);
            }
        }
    } else {
        count = 0;
    }
    if (count > 1)
        dlg = GetNewDialog(kNewPilotDlogID, NULL, NULL);
    else
        dlg = GetNewDialog(kNewPilotOneCharDlogID, NULL, NULL);
    if (dlg == NULL)
        return 0;

    SetDialogDefaultItem(dlg, 1);
    CenterWindow(FrontWindow(dlg), gWind);
    ShowMacWindow(FrontWindow(dlg));
    FrontWindow(dlg);
    SetDialogGWorld(dlg);
    DrawDialog(dlg);
    FrameDefaultButton(dlg, 1);

    GetDialogItem(dlg, kItemStrictPlay, &type, (Handle *)&h, &box);
    ctl = (ControlPtr)h;
    if (strict != 0)
        SetControlValue(ctl, 1);
    else
        SetControlValue(ctl, 0);

    for (i = 0, p = newPlayerName[0]; i < kNameFieldCount; i++) {
        GetDialogItem(dlg, i + kItemFirstNameField, &type, (Handle *)&h, &box);
        SetDialogItemText((Handle)h, p);
        p += kNameFieldStride;
    }
    SelectDialogItemText(dlg, kItemFirstNameField, 0, kSelectAllNameText);
    FlushEvents(everyEvent - diskMask, 0);

    GetDefaultCharResourceName(savedName);
    selected = 1;
    if (savedName[0] != 0) {
        PtoCstr(savedName);
        n = CountResources(kCharType);
        if (n > 1) {
            i = idx = 0;
            if (n > 0) do {
                res = GetIndResource(kCharType, (i + 1));
                if (res != NULL) {
                    GetResInfo(res, &resID, &resType, name);
                    if ((int32_t)name[1] != '.') {
                        idx++;
                        PtoCstr(name);
                        if (strcmp((char *)name, (char *)savedName) == 0) {
                            selected = idx;
                            selected++;
                            break;
                        }
                    }
                    ReleaseResource(res);
                }
            } while (++i < n);
        }
    }
    GetDialogItem(dlg, kItemCharPopup, &type, (Handle *)&h, &box);
    ctl = (ControlPtr)h;
    SetControlValue(ctl, (short)selected);

    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == kItemOK) {
            result = 1;
            done = 1;
            for (n = 0; n < kNameFieldCount; n++) {
                /* SHORT here: the exe compares the length as 16-bit signed (cmp ax / jle);
                   burger.h's strlen macro turns this into a block-scope FastStrLen */
#ifndef MACBUILD
                extern short strlen(const char *s);      /* 0x004eedb0 */
#endif

                GetDialogItem(dlg, n + kItemFirstNameField, &type, (Handle *)&h, &box);
                GetDialogItemText((Handle)h, savedName);
                PtoCstr(savedName);
                if (strlen((char *)savedName) > kMaxPilotNameLen) {
                    SysBeep(0);
                    result = 0;
                    done = 0;
                    SelectDialogItemText(dlg, n + kItemFirstNameField, 0,
                                         kMaxPilotNameLen - 1);
                    break;
                }
            }
        }
        if (itemHit == kItemStrictPlay) {
            if (strict != 0)
                strict = 0;
            else
                strict = 1;
            GetDialogItem(dlg, kItemStrictPlay, &type, (Handle *)&h, &box);
            ctl = (ControlPtr)h;
            if (strict != 0)
                SetControlValue(ctl, 1);
            else
                SetControlValue(ctl, 0);
        }
        if (itemHit == kItemCancel) {
            result = 0;
            done = 1;
        }
        itemHit = -1;
    } while (done == 0);

    if (result != 0) {
        for (i = 0, p = newPlayerName[0]; i < kNameFieldCount; i++) {
            GetDialogItem(dlg, i + kItemFirstNameField, &type, (Handle *)&h, &box);
            GetDialogItemText((Handle)h, p);
            p += kNameFieldStride;
        }
        *strictPlay = strict;
        GetDialogItem(dlg, kItemCharPopup, NULL, (Handle *)&ctl, NULL);
        GetMenuItemText(ctl, GetControlValue(ctl), newPilotPopupChoice[0]);
        GetDialogItem(dlg, kItemGenderPopup, NULL, (Handle *)&ctl, NULL);
        GetMenuItemText(ctl, GetControlValue(ctl), newPilotPopupChoice[1]);
    }
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(dlg);
    SWSetPortToWindow(gSpriteWorld);
    return result;
}
#endif

EXTERN_C_END
