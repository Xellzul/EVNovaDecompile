/* itpopupmac.c -- 1.0.10: the interface tool's POPUP control on the Control Manager, both fragments. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "ittoolkit.h"

#if !TARGET_API_MAC_CARBON
#pragma options align=mac68k
typedef struct ITPopup {
    void **title;                                /* +0x00 */
    unsigned char language;                      /* +0x04 the LANGUAGE popup: never shown */
    unsigned char pad05;
    ITRect bounds;                               /* +0x06 */
    int32_t selected;                               /* +0x16 0-based */
    unsigned char active;                        /* +0x1a */
    unsigned char visible;                       /* +0x1b */
    unsigned char enabled;                       /* +0x1c */
    unsigned char pad1d;
    ControlHandle control;                       /* +0x1e */
    MenuHandle menu;                             /* +0x22 */
} ITPopup;
#pragma options align=reset

#define kFirstMenuID        -30000
#define kPopupMenuProc      1008
#define kPressHilite        10
#define kPressMillis        120
#define kHiliteInactive     0xff
#define kHiliteDisabled     0xfe

extern CQDProcs gIT_StdCProcs;

int32_t IT_PopupHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    Point pt;
    int32_t flag;
    void *block;
    GrafPtr owner;
    unsigned char refresh;
    CGrafPtr savePort;
    ITPopup *popup;
    GDHandle saveDevice;
    Rect rect;
    int32_t value;
    char cstr[256];
    Str255 title;
    Str255 text;
    char copy[256];
    Str255 item;
    int32_t size;
    ULong index;
    short id;
    ITElement *node;
    char *name;
    int32_t err;
    unsigned char separator;
    CQDProcsPtr procs;
    int32_t e;

    err = 0;
    refresh = 0;
    popup = *data;
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
    GetGWorld(&savePort, &saveDevice);
    if (dc != 0)
        SetGWorld(dc, GetMainDevice());
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITPopup));
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock(data, 1);
        block = *data;
        popup = block;
        MemoryClear(block, sizeof(ITPopup));
        if ((popup->title = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (IT_ItemCallbackGetParam("LANGUAGE", 0) != 0)
            popup->language = 1;
        popup->enabled = 1;
        popup->visible = 1;
        popup->active = 1;
        if (popup->language)
            popup->visible = 0;
        break;
    case kMsgOpen:
        size = IndirectGetSize(popup->title) + 1 < 256 ? IndirectGetSize(popup->title) + 1 : 256;
        StringCopySafe(cstr, *popup->title, size);
        StringC2PCopy(title, cstr);
        CopyDT_2SysRect(&rect, &popup->bounds);
        id = kFirstMenuID;
        while (GetMenuHandle(id) != 0)
            id++;
        if ((popup->menu = NewMenu(id, "\p")) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        InsertMenu(popup->menu, hierMenu);
        if ((popup->control = NewControl(hwnd, &rect, title, popup->visible, 0, id, 0, kPopupMenuProc, 0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        popup->selected = 0;
        refresh = 1;
        node = 0;
        while (element != 0 && (node = _IT_ParseTreeFindNextTag(element, node, "ITEM", AsByte(node != 0))) != 0) {
            name = _IT_SimpleFindAttribute(node, "TITLE");
            flag = 0;
            if (name == 0 || *name == 0 || StringCompare(name, "-") == 0)
                flag = 1;
            separator = flag;
            AppendMenu(popup->menu, separator ? "\p-" : "\p ");
            if (!separator)
                SetMenuItemText(popup->menu, CountMItems(popup->menu), StringC2PCopy(title, name));
            if (!separator && _IT_SimpleFindAttribute(node, "DEFAULT") != 0)
                popup->selected = CountMItems(popup->menu) - 1;
        }
        if (CountMItems(popup->menu) == 0)
            AppendMenu(popup->menu, "\pnone");
        SetControlMaximum(popup->control, CountMItems(popup->menu));
        SetControlMinimum(popup->control, 1);
        SetControlValue(popup->control, popup->selected + 1);
        break;
    case kMsgClose:
        if (popup->control != 0)
            DisposeControl(popup->control);
        popup->control = 0;
        popup->menu = 0;
        break;
    case kMsgDispose:
        if (popup->title != 0) {
            IndirectDeallocate(popup->title);
            popup->title = 0;
        }
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgHitTest:
        if (popup->control == (ControlHandle)a)
            *(unsigned char *)b = 1;
        break;
    case kMsgClick:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (TestControl(popup->control, pt) != 0)
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
        owner = (*popup->control)->contrlOwner;
        (*popup->control)->contrlOwner = dc;
        procs = ((CGrafPtr)dc)->grafProcs;
        ((CGrafPtr)dc)->grafProcs = &gIT_StdCProcs;
        Draw1Control(popup->control);
        ((CGrafPtr)dc)->grafProcs = procs;
        (*popup->control)->contrlOwner = owner;
        break;
    case kMsgWhat:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (TrackControl(popup->control, pt, (ControlActionUPP)-1) != 0)
            *(unsigned char *)b = 1;
        break;
    case kMsgPress:
        HiliteControl(popup->control, kPressHilite);
        SystemWait(kPressMillis);
        if (!popup->active) {
            separator = kHiliteInactive;
        } else {
            flag = 0;
            if (!popup->enabled || !IT_ItemCallbackIsViewEnabled())
                flag = 1;
            separator = 0;
            if (flag != 0)
                separator = kHiliteDisabled;
        }
        HiliteControl(popup->control, separator);
        break;
    case kMsgMouse:
        *(unsigned short *)b = 0;
        break;
    case kMsgActivate:
    case kMsgDeactivate:
        popup->active = AsByte(msg == kMsgActivate);
        if (popup->control != 0)
            refresh = 1;
        break;
    case kMsgGetBounds:
        *(ITRect *)b = popup->bounds;
        break;
    case kMsgBounds:
        popup->bounds = *(ITRect *)a;
        CopyDT_2SysRect(&rect, &popup->bounds);
        if (popup->control != 0)
            (*popup->control)->contrlRect = rect;
        break;
    case kMsgGetTitle:
        e = IndirectReplace(b, popup->title);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kMsgTitle:
        e = IndirectReplace(popup->title, (void **)a);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (popup->control != 0) {
            size = IndirectGetSize(popup->title) + 1 < 256 ? IndirectGetSize(popup->title) + 1 : 256;
            StringCopySafe(cstr, *popup->title, size);
            StringC2PCopy((*popup->control)->contrlTitle, cstr);
        }
        break;
    case kMsgGetText:
        e = IT_PopupHandler(hwnd, dc, element, data, kMsgGetValue, 0, &value);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (popup->control != 0) {
            GetMenuItemText(popup->menu, value, text);
            e = IndirectReplaceData(b, &text[1], text[0]);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            e = IndirectSetSize(b, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        break;
    case kMsgSetText:
        if (popup->control != 0) {
            index = IndirectGetSize((void **)a);
            if (index > 255)
                index = 255;
            MemoryCopy(copy, *(void **)a, index);
            copy[index] = 0;
            for (index = GetControlMinimum(popup->control); index <= GetControlMaximum(popup->control); index++) {
                GetMenuItemText(popup->menu, index, item);
                StringP2C(item);
                if (StringCompare(copy, (char *)item) == 0)
                    break;
            }
            if (index > GetControlMaximum(popup->control)) {
                err = kNotFound;
                goto done;
            }
            (*popup->control)->contrlValue = index;
        }
        break;
    case kMsgGetValue:
        *(int32_t *)b = popup->selected;
        if (popup->control != 0)
            *(int32_t *)b = GetControlValue(popup->control);
        break;
    case kMsgSetValue:
        popup->selected = a;
        if (popup->control != 0)
            (*popup->control)->contrlValue = popup->selected != 0;
        break;
    case kMsgVisible:
        *(unsigned char *)b = popup->visible;
        break;
    case kMsgShow:
        popup->visible = AsByte(a != 0);
        if (popup->language)
            popup->visible = 0;
        if (popup->control != 0)
            (*popup->control)->contrlVis = popup->visible != 0 ? 0xff : 0;
        break;
    case kMsgEnabled:
        *(unsigned char *)b = popup->enabled;
        break;
    case kMsgEnable:
        popup->enabled = AsByte(a != 0);
        if (popup->control != 0)
            refresh = 1;
        break;
    case kMsgGetFore:
    case kMsgFore:
    case kMsgGetBack:
    case kMsgBack:
        break;
    }
    if (refresh) {
        if (!popup->active)
            (*popup->control)->contrlHilite = kHiliteInactive;
        else if (!popup->enabled || !IT_ItemCallbackIsViewEnabled())
            (*popup->control)->contrlHilite = kHiliteDisabled;
        else
            (*popup->control)->contrlHilite = 0;
    }
done:
    SetGWorld(savePort, saveDevice);
    return err;
}
#else /* TARGET_API_MAC_CARBON */
#pragma options align=mac68k
typedef struct ITPopup {
    void **title;                                /* +0x00 */
    unsigned char language;                      /* +0x04 the LANGUAGE popup: lists the window's languages */
    unsigned char pad05;
    ITRect bounds;                               /* +0x06 */
    int32_t selected;                               /* +0x16 0-based */
    unsigned char active;                        /* +0x1a */
    unsigned char visible;                       /* +0x1b */
    unsigned char enabled;                       /* +0x1c */
    unsigned char pad1d;
    ControlHandle control;                       /* +0x1e */
    MenuHandle menu;                             /* +0x22 */
} ITPopup;
#pragma options align=reset

#define kFirstMenuID        -30000
#define kPopupMenuProc      1008
#define kPressHilite        10
#define kPressMillis        120
#define kHiliteInactive     0xff
#define kHiliteDisabled     0xfe
#define kBeepTicks          7

extern CQDProcs gIT_StdCProcs;
extern CFStringRef IndirectCopyCFString(void **handle);
extern CFStringRef UnicodeCreateCFString(const unsigned short *s, ULong count);
extern ULong IT_ItemCallbackLanguageCount(void);
extern int32_t IT_ItemCallbackLanguageCurrent(char *code, unsigned short *name);
extern unsigned char IT_ItemCallbackLanguageCode(ULong index, char *code);
extern unsigned char IT_ItemCallbackLanguageName(ULong index, unsigned short *name);

#define kDashW L"-"

int32_t IT_PopupHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    unsigned char separator;
    char cstr[256];
    Str255 title;
    Str255 text;
    char copy[256];
    Str255 item;
    Rect rect;
    Point pt;
    CGrafPtr savePort;
    GDHandle saveDevice;
    int32_t value;
    unsigned char visible;
    int32_t flag;
    unsigned char refresh;
    ITPopup *popup;
    ULong size;
    ULong index;
    short id;
    int32_t i;
    void *block;
    ITElement *node;
    char *name;
    int32_t err;
    unsigned short wide[256];
    CFStringRef string;
    CQDProcsPtr procs;
    int32_t e;

    err = 0;
    refresh = 0;
    string = 0;
    popup = *data;
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
    GetGWorld(&savePort, &saveDevice);
    if (dc != 0)
        SetGWorld(dc, GetMainDevice());
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITPopup));
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock(data, 1);
        block = *data;
        popup = block;
        MemoryClear(block, sizeof(ITPopup));
        if ((popup->title = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (IT_ItemCallbackGetParam("LANGUAGE", 0) != 0)
            popup->language = 1;
        popup->enabled = 1;
        popup->visible = 1;
        popup->active = 1;
        if (popup->language && IT_ItemCallbackLanguageCount() <= 1)
            popup->visible = 0;
        break;
    case kMsgOpen:
        title[0] = 0;
        if (!IndirectIsUnicode(popup->title, 0)) {
            size = IndirectGetSize(popup->title) + 1 < 256 ? IndirectGetSize(popup->title) + 1 : 256;
            StringCopySafe(cstr, *popup->title, size);
            StringC2PCopy(title, cstr);
        }
        CopyDT_2SysRect(&rect, &popup->bounds);
        id = kFirstMenuID;
        while (GetMenuHandle(id) != 0)
            id++;
        if ((popup->menu = NewMenu(id, "\p")) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        InsertMenu(popup->menu, hierMenu);
        if ((popup->control = NewControl(hwnd, &rect, title, popup->visible, 0, id, 0, kPopupMenuProc, 0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (IndirectIsUnicode(popup->title, 0)) {
            if ((string = IndirectCopyCFString(popup->title)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            e = SetControlTitleWithCFString(popup->control, string);
            if (e != 0)
                err = e;
            CFRelease(string);
            string = 0;
        }
        popup->selected = 0;
        refresh = 1;
        if (popup->language) {
            for (i = 0; IT_ItemCallbackLanguageName(i, wide); i++) {
                AppendMenu(popup->menu, "\p ");
                if ((string = UnicodeCreateCFString(wide, -1)) == 0) {
                    err = kOutOfMemory;
                    goto done;
                }
                e = SetMenuItemTextWithCFString(popup->menu, (i + 1) & 0xffff, string);
                if (e != 0)
                    err = e;
                CFRelease(string);
                string = 0;
            }
            popup->selected = IT_ItemCallbackLanguageCurrent(0, 0);
        } else {
            node = 0;
            while (element != 0 && (node = _IT_ParseTreeFindNextTag(element, node, "ITEM", AsByte(node != 0))) != 0) {
                name = _IT_SimpleFindAttribute(node, "TITLE");
                if (node->wide) {
                    flag = 0;
                    if (name == 0 || *(unsigned short *)name == 0 || UnicodeCompare((unsigned short *)name, kDashW) == 0)
                        flag = 1;
                    separator = flag;
                } else {
                    flag = 0;
                    if (name == 0 || *name == 0 || StringCompare(name, "-") == 0)
                        flag = 1;
                    separator = flag;
                }
                AppendMenu(popup->menu, separator ? "\p-" : "\p ");
                if (!separator) {
                    if (node->wide) {
                        if ((string = UnicodeCreateCFString((unsigned short *)name, -1)) == 0) {
                            err = kOutOfMemory;
                            goto done;
                        }
                        e = SetMenuItemTextWithCFString(popup->menu, (unsigned short)CountMenuItems(popup->menu), string);
                        if (e != 0)
                            err = e;
                        CFRelease(string);
                        string = 0;
                    } else {
                        SetMenuItemText(popup->menu, CountMenuItems(popup->menu), StringC2PCopy(title, name));
                    }
                }
                if (!separator && _IT_SimpleFindAttribute(node, "DEFAULT") != 0)
                    popup->selected = CountMenuItems(popup->menu) - 1;
            }
        }
        if (CountMenuItems(popup->menu) == 0)
            AppendMenu(popup->menu, "\pnone");
        SetControlMaximum(popup->control, CountMenuItems(popup->menu));
        SetControlMinimum(popup->control, 1);
        SetControlValue(popup->control, popup->selected + 1);
        break;
    case kMsgClose:
        if (popup->control != 0)
            DisposeControl(popup->control);
        popup->control = 0;
        popup->menu = 0;
        break;
    case kMsgDispose:
        if (popup->title != 0) {
            IndirectDeallocate(popup->title);
            popup->title = 0;
        }
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgHitTest:
        if (popup->control == (ControlHandle)a)
            *(unsigned char *)b = 1;
        break;
    case kMsgClick:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (TestControl(popup->control, pt) != 0)
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
        procs = GetPortGrafProcs(dc);
        SetPortGrafProcs(dc, &gIT_StdCProcs);
        DrawControlInCurrentPort(popup->control);
        SetPortGrafProcs(dc, procs);
        break;
    case kMsgWhat:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (TrackControl(popup->control, pt, (ControlActionUPP)-1) != 0) {
            if (popup->language) {
                if (IT_ItemCallbackLanguageCode(GetControlValue(popup->control) - 1, ((ITClick *)b)->anchor)) {
                    ((ITClick *)b)->followed = 1;
                    ((ITClick *)b)->hasAnchor = 1;
                } else {
                    SysBeep(kBeepTicks);
                }
            } else {
                *(unsigned char *)b = 1;
            }
        }
        break;
    case kMsgPress:
        HiliteControl(popup->control, kPressHilite);
        SystemWait(kPressMillis);
        if (!popup->active) {
            separator = kHiliteInactive;
        } else {
            flag = 0;
            if (!popup->enabled || !IT_ItemCallbackIsViewEnabled())
                flag = 1;
            separator = 0;
            if (flag != 0)
                separator = kHiliteDisabled;
        }
        HiliteControl(popup->control, separator);
        break;
    case kMsgMouse:
        *(unsigned short *)b = 0;
        break;
    case kMsgActivate:
    case kMsgDeactivate:
        popup->active = AsByte(msg == kMsgActivate);
        if (popup->control != 0)
            refresh = 1;
        break;
    case kMsgGetBounds:
        *(ITRect *)b = popup->bounds;
        break;
    case kMsgBounds:
        popup->bounds = *(ITRect *)a;
        CopyDT_2SysRect(&rect, &popup->bounds);
        if (popup->control != 0) {
            visible = IsControlVisible(popup->control);
            if (visible)
                SetControlVisibility(popup->control, 0, 0);
            SetControlBounds(popup->control, &rect);
            if (visible)
                SetControlVisibility(popup->control, 1, 0);
        }
        break;
    case kMsgGetTitle:
        e = IndirectReplace(b, popup->title);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kMsgTitle:
        e = IndirectReplace(popup->title, (void **)a);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (popup->control != 0) {
            visible = IsControlVisible(popup->control);
            if (visible)
                SetControlVisibility(popup->control, 0, 0);
            if (IndirectIsUnicode(popup->title, 0)) {
                if ((string = IndirectCopyCFString(popup->title)) == 0) {
                    err = kOutOfMemory;
                    goto done;
                }
                e = SetControlTitleWithCFString(popup->control, string);
                if (e != 0)
                    err = e;
                CFRelease(string);
                string = 0;
            } else {
                size = IndirectGetSize(popup->title) + 1 < 256 ? IndirectGetSize(popup->title) + 1 : 256;
                StringCopySafe(cstr, *popup->title, size);
                SetControlTitle(popup->control, StringC2PCopy(title, cstr));
            }
            if (visible)
                SetControlVisibility(popup->control, 1, 0);
        }
        break;
    case kMsgGetText:
        e = IT_PopupHandler(hwnd, dc, element, data, kMsgGetValue, 0, &value);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (popup->control != 0) {
            GetMenuItemText(popup->menu, value, text);
            e = IndirectReplaceData(b, &text[1], text[0]);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            e = IndirectSetSize(b, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        break;
    case kMsgSetText:
        if (popup->control != 0) {
            size = IndirectGetSize((void **)a);
            if (size > 255)
                size = 255;
            MemoryCopy(copy, *(void **)a, size);
            copy[size] = 0;
            for (index = GetControlMinimum(popup->control); index <= GetControlMaximum(popup->control); index++) {
                GetMenuItemText(popup->menu, index, item);
                StringP2C(item);
                if (StringCompare(copy, (char *)item) == 0)
                    break;
            }
            if (index > GetControlMaximum(popup->control)) {
                err = kNotFound;
                goto done;
            }
            visible = IsControlVisible(popup->control);
            if (visible)
                SetControlVisibility(popup->control, 0, 0);
            SetControlValue(popup->control, index);
            if (visible)
                SetControlVisibility(popup->control, 1, 0);
        }
        break;
    case kMsgGetValue:
        *(int32_t *)b = popup->selected;
        if (popup->control != 0)
            *(int32_t *)b = GetControlValue(popup->control);
        break;
    case kMsgSetValue:
        popup->selected = a;
        if (popup->control != 0) {
            visible = IsControlVisible(popup->control);
            if (visible)
                SetControlVisibility(popup->control, 0, 0);
            SetControlValue(popup->control, popup->selected != 0);
            if (visible)
                SetControlVisibility(popup->control, 1, 0);
        }
        break;
    case kMsgVisible:
        *(unsigned char *)b = popup->visible;
        break;
    case kMsgShow:
        popup->visible = AsByte(a != 0);
        if (popup->control != 0)
            SetControlVisibility(popup->control, popup->visible, 0);
        break;
    case kMsgEnabled:
        *(unsigned char *)b = popup->enabled;
        break;
    case kMsgEnable:
        popup->enabled = AsByte(a != 0);
        if (popup->control != 0)
            refresh = 1;
        break;
    case kMsgGetFore:
    case kMsgFore:
    case kMsgGetBack:
    case kMsgBack:
        break;
    }
    if (refresh) {
        if (!popup->active) {
            visible = IsControlVisible(popup->control);
            HiliteControl(popup->control, kHiliteInactive);
            if (visible)
                SetControlVisibility(popup->control, 1, 0);
        } else if (!popup->enabled || !IT_ItemCallbackIsViewEnabled()) {
            visible = IsControlVisible(popup->control);
            HiliteControl(popup->control, kHiliteDisabled);
            if (visible)
                SetControlVisibility(popup->control, 1, 0);
        } else {
            visible = IsControlVisible(popup->control);
            HiliteControl(popup->control, 0);
            if (visible)
                SetControlVisibility(popup->control, 1, 0);
        }
    }
done:
    if (string != 0)
        CFRelease(string);
    SetGWorld(savePort, saveDevice);
    return err;
}
#endif
