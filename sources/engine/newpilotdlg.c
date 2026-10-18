// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* newpilotdlg.c -- the New Pilot confirm/abort gate (win 0x0048a7e0). Notes: Research/notes/newpilotdlg.md */
#include <stddef.h>

#include "burger.h"
#include "macport.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "gametypes.h"

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


/* DoNewPilotNameDialog (mac _DoNewPilotNameDialog). STATE: MATCH 100%, exe 413 / obj 413,
   byte-identical. The selection loop is a do/while under its own `n > 0' guard.
   OPEN: none */
// FUNCTION: EVNOVA 0x0048a7e0
char DoNewPilotNameDialog(char *strictPlay)
{
    short resID;
    unsigned long resType;
    Str255 name;
    long selected;
    long idx;
    short itemHit;
    char result;
    char done;
    Rect box;
    short type;
    char strict;
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
                if ((long)savedName[1] != '.')
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

    GetDialogItem(dlg, kItemStrictPlay, &type, &h, &box);
    ctl = h;
    if (strict != 0)
        SetControlValue(ctl, 1);
    else
        SetControlValue(ctl, 0);

    for (i = 0, p = newPlayerName[0]; i < kNameFieldCount; i++) {
        GetDialogItem(dlg, i + kItemFirstNameField, &type, &h, &box);
        SetDialogItemText(h, p);
        p += kNameFieldStride;
    }
    SelectDialogItemText(dlg, kItemFirstNameField, 0, kSelectAllNameText);
    FlushEvents(everyEvent - diskMask, 0);

    GetDefaultCharacterName(savedName);
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
                    if ((long)name[1] != '.') {
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
    GetDialogItem(dlg, kItemCharPopup, &type, &h, &box);
    ctl = h;
    SetControlValue(ctl, (short)selected);

    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == kItemOK) {
            result = 1;
            done = 1;
            for (n = 0; n < kNameFieldCount; n++) {
                /* SHORT here: the exe compares the length as 16-bit signed (cmp ax / jle);
                   burger.h's strlen macro turns this into a block-scope FastStrLen */
                extern short strlen(const char *s);      /* 0x004eedb0 */

                GetDialogItem(dlg, n + kItemFirstNameField, &type, &h, &box);
                GetDialogItemText(h, savedName);
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
            GetDialogItem(dlg, kItemStrictPlay, &type, &h, &box);
            ctl = h;
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
            GetDialogItem(dlg, i + kItemFirstNameField, &type, &h, &box);
            GetDialogItemText(h, p);
            p += kNameFieldStride;
        }
        *strictPlay = strict;
        GetDialogItem(dlg, kItemCharPopup, NULL, (void **)&ctl, NULL);
        GetMenuItemText(ctl, GetControlValue(ctl), newPilotPopupChoice[0]);
        GetDialogItem(dlg, kItemGenderPopup, NULL, (void **)&ctl, NULL);
        GetMenuItemText(ctl, GetControlValue(ctl), newPilotPopupChoice[1]);
    }
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(dlg);
    SWSetPortToWindow(gSpriteWorld);
    return result;
}
