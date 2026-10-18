// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
// COMPILE-WHY: ModalDialog alone is built under `#pragma opt_lifetimes off'; not
// `-opt ...,nolifetimes' because file-wide it breaks PopUpListSelect and nv_NewDialog.
/* dialogmgr.c -- the port's Dialog, Control and List Manager (win 0x004cea90..0x004d2950, in exe order). Notes: Research/notes/dialogmgr.md */
#include "burgerplat.h"
#include "win32.h"
#include <stddef.h>
#include <string.h>   /* strcpy 0x004d6010, strlen 0x004eedb0 */
#include <stdio.h>

#include "macport.h"
#include "gametypes.h"
#include "game.h"
#include "burger.h"
#include "render.h"
#include "sound.h"
#include "dialogs.h"
#include "prototypes.h"

/* A control points back at the dialog that owns it so that changing a value
   can mark the dialog dirty. */

#define kEditTextItem 0x10

/* the dialog manager's globals */
DialogPtr gCurDialog;
long gDialogCount;
DialogPtr gDialogStack[16];
long gBlinkTick;
long gLastModifiers;
unsigned long gClickTime;
void *gClickDialog;
void *gClickItem;
unsigned char DAT_00864f9c[4];
long gRezOpenPending;
long paintBlackFlag;
void *gKeyboardHook;
long gUseKeyboardHook;
OSVERSIONINFOA gOSVersionInfo;

/* the port has exactly one window, so there is nothing to search */

/* the Toolbox GetMouse answers in port-local coordinates, so the port's
   raw global reading has the current dialog's top left taken off it */

/* a dialog record starts with the GWorldContainer it draws into */

/* clamps into the control's own range, then marks its dialog for redraw */

/* the GetWindowPortRect of the port's own dialog record */

/* Three Toolbox entry points that collapsed to the same store: the item and
   the rect saying WHAT to invalidate are discarded and the dialog is redrawn. */

/* an item with no text of its own answers the empty string */

/* only the three editable item kinds carry text */

/*
 * Resize a dialog and the offscreen world behind it (win 0x004d18b0). The DIB
 * section is rebuilt at the new size, so the old bitmap is deleted first and
 * the port's baseAddr is re-pointed by CreateDIBSection.
 */

/*
 * Replaces an item's text, reallocating to fit. An editable item also has its
 * selection reset to the whole string, which is what reuses minValue/maxValue.
 * The dialog is only marked dirty when text was actually installed.
 */

/* the pascal-string entry point SetControlTitle dispatches to */

/* and the pascal-string reader that mirrors it */

/* set the text back without disturbing the caret or the selection */

/*
 * A list item's data block: the MENU's title, then fixed-width C-string
 * entries, each with its flag byte at [0x100]. Indices are 1-based.
 */
#define kListItem 0x80

typedef struct DlgListEntry {
    char text[0x100];
    char flag;
} DlgListEntry;

typedef struct DlgListRec {
    char title[0x100];          /* +0x000 */
    long count;                 /* +0x100 */
    DlgListEntry item[256];     /* +0x104 */
} DlgListRec;

/*
 * The scroll state behind a list; a separate record from the dialog, sharing
 * only the convention of being reached through a handle.
 */

/* The two "no such item" stores load their zero byte from a pooled empty string, so
   they are an inlined strcpy: this file takes <string.h> and intrinsics. */

/* scroll by `delta`, then clamp to the last full page and to zero */

/* The bevel the port draws round a control: an optional fill, then a highlight
   down the top and left edges and a shadow up the bottom and right. Each edge is
   drawn as its own move/line pair rather than a polyline, so the corner pixel is
   painted twice and both diagonals start from the same inset corner. */

/* The vertical run's `if (n == 9) n = 0;' can never fire and is in the original. The
   corner goes through the botRight Point view of the Rect: spelled r->bottom/r->right
   it value-numbers into the loop bounds. */

/* scroll the selected row into view, doing nothing when the whole list already
   fits or when the row is already on screen */

/* the box a run of wrapped text will occupy, sat on its baseline */

/*
 * The item list is a plain 0x28-stride array hanging off the dialog, and
 * item numbers are 1-based as the Toolbox has them. Every Dialog Manager
 * entry point that takes an item number funnels through this.
 */
#define kDialogItemSize 0x28

/* An absent item answers zero in each out-parameter except the handle, which
   gets the null record itself. */

/*
 * The Toolbox signature carries the type and handle too, but the port stores
 * only the box — the other two are set when the item is built. Marking the
 * owning dialog for update is what makes a moved item repaint.
 */

/* The three resource-backed item kinds load their resource here; the three longs ahead
   of the box are the control's value and its range. The length is unsigned and the
   guard casts it signed, so a negative count means "no data". */
#define kResourceControlItem 7
#define kIconItem         0x20
#define kPictureItem      0x40

#define kHelpItem         1
#define kButtonItem       4
#define kCheckBoxItem     5
#define kRadioButtonItem  6
#define kStaticTextItem   8

long gCaretPhase = 1;                             /* 0x00576130 (name invented) */

/* 0x0056f118 / 11e / 124 / 12a (names invented) */
static const RGBColor bevelGray1 = { 50000, 50000, 50000 };
static const RGBColor bevelGray2 = { 35000, 35000, 35000 };
static const RGBColor bevelGray3 = { 25000, 25000, 25000 };
static const RGBColor bevelGray4 = { 15000, 15000, 15000 };

extern "C" {

static __inline short SwapEndianShort(const unsigned short *p)
{
    register unsigned short t;

    t = *p;
    asm { ror t, 8 }
    return t;
}

static __inline long SwapEndianLong(const unsigned long *p)
{
    register unsigned long t;

    t = *p;
    asm { bswap t }
    return t;
}

/* A CNTL resource, big-endian as it comes off the rez file. Only min, procID
   and the title survive; bounds/value/max are swapped and dropped. */
#pragma options align=packed
typedef struct CntlTemplate {
    Rect bounds;                     /* +0x00, discarded */
    short value;                     /* +0x08, discarded */
    char visible;                    /* +0x0a, discarded */
    char filler;                     /* +0x0b, discarded */
    short max;                       /* +0x0c, discarded */
    short min;                       /* +0x0e */
    short procID;                    /* +0x10 */
    long refCon;                     /* +0x12 */
    unsigned char title[1];          /* +0x16, Str255 */
} CntlTemplate;
#pragma options align=reset

#define kCNTLType 0x434e544c

#define kResourceSliderProc  1008
#define kResourceRangeProc   1012

#define kMENUType 0x4d454e55

/*
 * Put the selection on an editable item's text (win 0x004cfca0). The range is
 * clamped to the text that is actually there, an item with no text gets an
 * empty selection, and the dialog remembers which item owns the caret.
 */

/*
 * Redraw the dialog stack (win 0x004d1810). Screen updates are suspended for
 * the duration and the previous suspend state restored, the game's own screen
 * is refreshed underneath first, and `all` says whether the frontmost dialog
 * is included or left for its own update proc.
 */

/*
 * Blit the dialogs' offscreen worlds to the screen (win 0x004d16f0). `all`
 * walks the whole stack; otherwise only the current dialog goes up. A dialog
 * with visible clear has nothing on screen yet and is skipped either way.
 */

/* a control's storage is the item array itself, so there is nothing to free */
static __inline void DisposeControl(void *ctl)
{
}

/* Dialog Manager item types, as far as the port distinguishes them */
#define kResCtrlItem 7                  /* ctrlItem + resCtrl */
#define kPicItem    64

#define kDLOGType 0x444c4f47
#define kDITLType 0x4449544c

typedef char (*DialogEventProc)(DialogPtr dlg, EventRecord *ev,
                                short *itemHit);

/* keys come back from KeyboardGet2 as BurgerLib ASCII codes */
#define ASCII_BACKSPACE  0x08
#define ASCII_TAB        0x09
#define ASCII_DOWNARROW  0x0a
#define ASCII_UPARROW    0x0b
#define ASCII_RETURN     0x0d
#define ASCII_ESCAPE     0x1b
#define ASCII_RIGHTARROW 0x15
#define ASCII_LEFTARROW  0x96
#define ASCII_DELETE     0x97

/*
 * Build a dialog from its DLOG and DITL (win 0x004cf760) and make it current.
 * The DLOG parser answers the DITL's id. The window rect starts as a copy of
 * the screen bounds moved to the origin, because the dialog draws into its own
 * offscreen world and is blitted into place later.
 */

/*
 * Register the window class and create the app's single window (win
 * 0x004d2950), called once from WinMain with the instance handle. Also
 * decides whether to poll or hook the keyboard: Win95/98/Me (dwPlatformId
 * 1, VER_PLATFORM_WIN32_WINDOWS) leave the screen saver disabled instead,
 * everything else installs a low-level keyboard hook once.
 */

#define CS_DBLCLKS  0x0008
#define CS_OWNDC    0x0020

#define kMainWindowStyle (WS_POPUP | WS_VISIBLE | WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX)

#define WS_EX_APPWINDOW 0x00040000

#define VER_PLATFORM_WIN32_WINDOWS 1

#define SPI_SCREENSAVERRUNNING 0x61

#define WH_KEYBOARD_LL 0xd

/*
 * The rest of the window plumbing: the list scrollbar pair the dialog list
 * accessors above reach through nv_NewDialog records, the WinKey-eating
 * low-level keyboard hook, the main window proc CreateGameWindow registers, and
 * the atexit handler winmain.c installs by pointer.
 */

typedef struct WinPoint {
    long x;
    long y;
} WinPoint;

typedef struct MINMAXINFO {
    WinPoint ptReserved;        /* +0x00 */
    WinPoint ptMaxSize;         /* +0x08 */
    WinPoint ptMaxPosition;     /* +0x10 */
    WinPoint ptMinTrackSize;    /* +0x18 */
    WinPoint ptMaxTrackSize;    /* +0x20 */
} MINMAXINFO;

#define HC_ACTION   0

#define GWL_EXSTYLE (-20)
#define GWL_STYLE   (-16)

                                                         /* IAT 0x0058f8f8 */
                                                         /* IAT 0x0058f8e4 */
                                                         /* IAT 0x0058f8e8 */
                                                         /* IAT 0x0058f8ec */

/*
 * A click in one of the lists behind an nv_NewDialog record: either drives
 * the scrollbar (arrows scroll one row, the shaft pages by a screenful,
 * against the same 15-wide geometry LUpdate draws) or picks the row
 * under the point. Both paths timestamp the click for the callers that
 * detect double clicks.
 */

/*
 * WH_KEYBOARD_LL hook: eat the two Windows keys so a fullscreen game does
 * not lose focus. CreateGameWindow installs it on NT-family systems only.
 */

/* nv_GetDialogItemRecord. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cea90
void *nv_GetDialogItemRecord(DialogPtr dlg, long item)
{
    void *rec;

    if (item < 1 || item > dlg->itemCount)
        rec = NULL;
    else
        rec = (char *)dlg->items + (item - 1) * kDialogItemSize;
    return rec;
}

/* LoadMenuListItem. STATE: MATCH 149/149, byte-identical, relocregcheck 0 of 13. OPEN: none */
// FUNCTION: EVNOVA 0x004ceac0
void LoadMenuListItem(ControlRecord *rec, long menuID, unsigned char *promptTitle,
                  long resType)
{
    DlgListRec *list;
    Handle h;
    unsigned char *scratch;
    long i, idx;
    long count;
    Handle rh;
    unsigned char *p;
    char *dest;
    unsigned char entry[256];
    unsigned char name[256];
    short id;
    unsigned long type;
    unsigned char title[256];
    long slot;
    char *line;
    unsigned char flag;

    h = GetResource(kMENUType, menuID);
    if (h == NULL)
        return;
    LockAHandle(h);
    p = (unsigned char *)*h;
    rec->kind = kListItem;
    rec->data = AllocAPointer(sizeof(DlgListRec));
    rec->value = 1;
    list = (DlgListRec *)rec->data;
    SwapEndianShort((unsigned short *)p);
    SwapEndianShort((unsigned short *)(p + 6));
    SwapEndianLong((unsigned long *)(p + 0xa));
    title[0] = p[0xe];
    p += 0xf;
    memcpy(&title[1], p, title[0]);
    p += title[0];
    PStr2CStr(list->title, title);
    list->count = 0;
    scratch = &entry[1];
    while (*p != 0 && list->count < 256) {
        idx = list->count++;
        dest = list->item[idx].text;
        entry[0] = *p;
        p += 1;
        memcpy(scratch, p, entry[0]);
        p += entry[0];
        PStr2CStr(dest, entry);
        flag = p[1];
        p += 4;
        dest[0x100] = flag;
    }
    if (resType != 0) {
        count = CountResources(resType);
        i = 0;
        while (i < count && list->count < 256) {
            rh = GetIndResource(resType, (i + 1));
            if (rh != NULL) {
                GetResInfo(rh, &id, &type, name);
                if ((int)name[1] != '.') {
                    slot = list->count++;
                    line = list->item[slot].text;
                    PStr2CStr(line, name);
                    line[0x100] = 0;
                }
                ReleaseResource(rh);
            }
            i += 1;
        }
    }
    ReleaseResource(h);
}

/* LoadControlResource. STATE: MATCH. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004cecf0
void LoadControlResource(ControlRecord *rec, unsigned long resID)
{
    long min;
    unsigned long refCon;
    short procID;
    unsigned char title[256];
    CntlTemplate *t;
    Handle h;

    h = GetResource(kCNTLType, resID);
    if (h == NULL)
        return;
    LockAHandle((Handle)h);
    t = (CntlTemplate *)*h;
    SwapEndianShort((unsigned short *)&t->bounds.top);
    SwapEndianShort((unsigned short *)&t->bounds.left);
    SwapEndianShort((unsigned short *)&t->bounds.bottom);
    SwapEndianShort((unsigned short *)&t->bounds.right);
    SwapEndianShort((unsigned short *)&t->value);
    SwapEndianShort((unsigned short *)&t->max);
    min = SwapEndianShort((unsigned short *)&t->min);
    procID = SwapEndianShort((unsigned short *)&t->procID);
    refCon = SwapEndianLong((unsigned long *)&t->refCon);
    title[0] = t->title[0];
    memcpy(&title[1], &t->title[1], title[0]);
    ReleaseResource(h);
    switch (procID) {
    case kResourceSliderProc:
        LoadMenuListItem(rec, min, title, 0);
        break;
    case kResourceRangeProc:
        LoadMenuListItem(rec, min, title, refCon);
        break;
    default:
        break;
    }
}

/* nv_AppendDialogItem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cede0
ControlRecord *nv_AppendDialogItem(long kind, long value, long minValue,
                                   long maxValue, const Rect *box,
                                   const void *data, unsigned long dataLen,
                                   DialogPtr dlg)
{
    long index;
    ControlRecord *rec;

    index = dlg->itemCount;
    dlg->itemCount = dlg->itemCount + 1;
    rec = (ControlRecord *)((char *)dlg->items + index * kDialogItemSize);
    rec->kind = kind;
    rec->itemNo = index + 1;
    rec->value = value;
    rec->minValue = minValue;
    rec->maxValue = maxValue;
    rec->box.top = box->top;
    rec->box.left = box->left;
    rec->box.bottom = box->bottom;
    rec->box.right = box->right;
    rec->data = NULL;
    rec->owner = dlg;
    rec->enabled = 1;
    switch (kind) {
    case kResourceControlItem:
        LoadControlResource(rec, value);
        break;
    case kIconItem:
        rec->data = GetCIcon(value);
        if (rec->data != NULL)
            goto done;
        break;
    case kPictureItem:
        rec->data = GetPicture(value);
        if (rec->data != NULL)
            goto done;
        break;
    default:
        if ((long)dataLen > 0) {
            rec->data = AllocAPointer(dataLen);
            memcpy(rec->data, data, dataLen);
        }
    }
done:
    return rec;
}

/* nv_SetDialogTemplateBounds. STATE: MATCH, byte-identical. OPEN: none
   Second live use of itemsID forces early widening (notes). */
// FUNCTION: EVNOVA 0x004ceec0
long nv_SetDialogTemplateBounds(DialogPtr dlg, const unsigned short *tmpl)
{
    long itemsID;
    long halfWidth;
    long halfHeight;
    long centerX;
    long centerY;

    dlg->bounds.top = SwapEndianShort(&tmpl[0]);
    dlg->bounds.left = SwapEndianShort(&tmpl[1]);
    dlg->bounds.bottom = SwapEndianShort(&tmpl[2]);
    dlg->bounds.right = SwapEndianShort(&tmpl[3]);
    itemsID = SwapEndianShort(&tmpl[9]);
    halfWidth = (dlg->bounds.right - dlg->bounds.left) >> 1;
    halfHeight = (dlg->bounds.bottom - dlg->bounds.top) >> 1;
    centerX = gVideoWidth >> 1;
    centerY = gVideoHeight >> 1;
    dlg->bounds.left = centerX - halfWidth;
    dlg->bounds.right = centerX + halfWidth;
    dlg->bounds.top = centerY - halfHeight;
    dlg->bounds.bottom = centerY + halfHeight;
    return itemsID ? itemsID : itemsID;
}

/* ParseDITL. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cef50
void ParseDITL(DialogPtr dlg, unsigned char *p)
{
    long lastIndex;
    long i;
    long type;
    unsigned char rawType;
    long value;
    long dataLen;
    long len0;
    Rect box;
    unsigned char text[256];
    ControlRecord *item;

    lastIndex = SwapEndianShort((unsigned short *)p);
    p += 2;
    if (lastIndex >= 0)
        dlg->items = AllocAPointer((lastIndex + 1) * kDialogItemSize);
    else
        dlg->items = NULL;

    for (i = 1; i <= lastIndex + 1; i++) {
        box.top = SwapEndianShort((unsigned short *)(p + 4));
        box.left = SwapEndianShort((unsigned short *)(p + 6));
        box.bottom = SwapEndianShort((unsigned short *)(p + 8));
        box.right = SwapEndianShort((unsigned short *)(p + 10));
        rawType = p[12];
        p += 13;
        type = rawType & 0x7f;

        if (type == kButtonItem || type == kCheckBoxItem ||
            type == kRadioButtonItem || type == kStaticTextItem ||
            type == kEditTextItem) {
            len0 = p[0];
            PStr2CStr((char *)text, p);
            dataLen = strlen((char *)text) + 1;
            p += (unsigned char)len0 + 1;
        } else if (type == kResourceControlItem || type == kIconItem ||
                   type == kPictureItem) {
            value = SwapEndianShort((unsigned short *)(p + 1));
            p += 3;
        } else {
            p += 1;
        }

        switch (type) {
        case 0:
            item = nv_AppendDialogItem(0, 0, 0, 1, &box, NULL, 0, dlg);
            break;
        case kHelpItem:
            break;
        case kButtonItem:
            item = nv_AppendDialogItem(kButtonItem, 0, 0, 1, &box, text, dataLen, dlg);
            break;
        case kCheckBoxItem:
        case kRadioButtonItem:
            item = nv_AppendDialogItem(kCheckBoxItem, 0, 0, 1, &box, text, dataLen, dlg);
            break;
        case kResourceControlItem:
            item = nv_AppendDialogItem(kResourceControlItem, value, 0, 1, &box, NULL, 0, dlg);
            break;
        case kStaticTextItem:
            item = nv_AppendDialogItem(kStaticTextItem, 0, 0, 1, &box, text, dataLen, dlg);
            break;
        case kEditTextItem:
            item = nv_AppendDialogItem(kEditTextItem, 0, 0, 0, &box, text, dataLen, dlg);
            break;
        case kIconItem:
            item = nv_AppendDialogItem(kIconItem, value, 0, 1, &box, NULL, 0, dlg);
            break;
        case kPictureItem:
            item = nv_AppendDialogItem(kPictureItem, value, 0, 1, &box, NULL, 0, dlg);
            break;
        }

        if (dlg->editItem == 0) {
            if (item->kind != kPictureItem && item->kind != kIconItem &&
                item->kind != 0 && item->kind != kStaticTextItem &&
                item->kind != 1)
                dlg->editItem = item->itemNo;
        }
        if (((long)p & 1) != 0)
            p += 1;
    }
}

/* PopUpListSelect. STATE: MATCH, exe 397 / obj 397, byte-identical yes. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004cf1c0
long PopUpListSelect(DialogPtr dlg, ControlRecord *rec)
{
    GWorldContainer gw;
    Rect box;
    Rect hilite;
    Point pt;
    Rect *portRect;
    long result;
    long scroll;
    long maxScroll;
    CGrafPtr savedPort;
    DlgListRec *list;
    long w;
    long i;
    short lastV;
    short lastH;
    short h;
    long v;
    long x;

    list = (DlgListRec *)rec->data;
    result = 0;
    scroll = 0;
    maxScroll = 0;
    box.top = rec->box.top;
    box.left = rec->box.left;
    box.bottom = rec->box.bottom;
    box.right = rec->box.right;
    box.left += 100;
    GetGlobalMouse(&pt);
    if (PtInRect(pt.h - dlg->bounds.left, pt.v - dlg->bounds.top, &box)) {
        portRect = GetCWindowPortRect(gWind);
        CreateGWorldContainer(&gw, 0, 0, &box);
        SetGWorldContainer(&gw);
        for (i = 0; i < list->count; i++) {
            w = MeasureCString(list->item[i].text) + 3;
            if (w > box.right - box.left + 8)
                box.right = box.left + w + 8;
        }
        UnsetGWorldContainer(&gw);
        DisposeGWorldContainer(&gw);
        if ((box.right - box.left) % 2 != 0)
            box.right += 1;
        box.bottom = box.top + list->count * 20;
        OffsetRect(&box, dlg->bounds.left, dlg->bounds.top);
        if (list->count * 20 > portRect->bottom - portRect->top) {
            box.top = portRect->top;
            box.bottom = portRect->bottom;
            maxScroll = list->count * 20 - (portRect->bottom - portRect->top);
        } else if (box.bottom > portRect->bottom) {
            OffsetRect(&box, 0, portRect->bottom - box.bottom);
        } else if (box.top < portRect->top) {
            OffsetRect(&box, 0, portRect->top - box.top);
        }
        CreateGWorldContainer(&gw, 0, 0, &box);
        SetGWorldContainer(&gw);
        EraseRect(&box);
        UnsetGWorldContainer(&gw);
        lastH = lastV = -1;
        do {
            GetGlobalMouse(&pt);
            h = pt.h;
            x = h;
            v = pt.v;
            if (maxScroll != 0 && scroll > 0 && pt.v < 20) {
                scroll -= 20;
                if (scroll < 0)
                    scroll = 0;
                lastH = lastV = -1;
            } else if (maxScroll != 0 && scroll < maxScroll &&
                       pt.v > portRect->bottom - portRect->top - 20) {
                scroll += 20;
                if (scroll > maxScroll)
                    scroll = maxScroll;
                lastH = lastV = -1;
            }
            if (h != lastH || pt.v != lastV) {
                if (PtInRect(x, v, &box))
                    result = (v - (box.top - scroll)) / 20 + 1;
                else
                    result = 0;
                GetPort(&savedPort);
                SetPort(gw.world);
                DrawBevel(&box, &bevelGray2, &bevelGray1,
                             &bevelGray4);
                RGBForeColor(&rgbBlack);
                for (i = 0; i < list->count; i++) {
                    w = MeasureCString(list->item[i].text) + 4;
                    MoveTo(box.right - w,
                           box.top - scroll + i * 20 +
                               (CurrentGrafPort->txSize + 20) / 2 - 1);
                    DrawCString((char *)&list->item[i]);
                    if (i == result - 1) {
                        hilite.left = box.left + 2;
                        hilite.right = box.right - 2;
                        hilite.top = box.top - scroll + i * 20 + 2;
                        hilite.bottom = hilite.top + 0x12;
                        InvertRect(&hilite);
                    }
                }
                DrawBevel(&box, NULL, &bevelGray1, &bevelGray4);
                CopyBits(GetGWCPixMap(&gw),
                         GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                         &box, &box, 0, NULL);
                SetPort(savedPort);
                ScreenEndScene();
                lastV = pt.v;
                lastH = pt.h;
            }
            HandleIntroMusic(0);
            KeyboardKbhit();
        } while (MouseReadButtons() != 0);
        DisposeGWorldContainer(&gw);
        dlg->needsUpdate = 1;
        RedrawDialogs(1);
        ScreenEndScene();
    }
    return result;
}

/* GetNewDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cf760
DialogPtr GetNewDialog(short dlogID, void *updateProc, void *eventProc)
{
    DialogPtr dlg;
    Handle dlogRes;
    Handle ditlRes;
    short ditlID;
    dlg = (DialogPtr)AllocAPointerClear(sizeof(DialogRecord));
    dlg->itemCount = 0;
    dlg->editItem = 0;
    dlg->defaultItem = 0;
    dlg->cancelItem = 0;
    dlg->updateProc = updateProc;
    dlg->eventProc = eventProc;
    dlg->needsUpdate = 1;
    dlg->visible = 1;
    dlogRes = GetResource(kDLOGType, dlogID);
    LockAHandle((Handle)dlogRes);
    ditlID = nv_SetDialogTemplateBounds(dlg,
                                        (const unsigned short *)*dlogRes);
    ditlRes = GetResource(kDITLType, ditlID);
    LockAHandle((Handle)ditlRes);
    ParseDITL(dlg, (unsigned char *)*ditlRes);
    ReleaseResource((Handle)ditlRes);
    ReleaseResource((Handle)dlogRes);
    dlg->portRect.top = dlg->bounds.top;
    dlg->portRect.left = dlg->bounds.left;
    dlg->portRect.bottom = dlg->bounds.bottom;
    dlg->portRect.right = dlg->bounds.right;
    OffsetRect(&dlg->portRect, -dlg->bounds.left, -dlg->bounds.top);
    CreateGWorldContainer((GWorldContainer *)dlg, 0, 0, &dlg->portRect);
    SetGWorldContainer((GWorldContainer *)dlg);
    EraseRect(&dlg->portRect);
    UnsetGWorldContainer((GWorldContainer *)dlg);
    gDialogStack[gDialogCount] = dlg;
    gDialogCount += 1;
    gCurDialog = dlg;
    MouseClicked = 0;
    return dlg;
}

/* DisposeDialog. STATE: MATCH, 71/71, displacement at the stack pop checked by hand (+0);
   byte check pending the next regate. OPEN: none */
// FUNCTION: EVNOVA 0x004cf890
void DisposeDialog(DialogPtr dlg)
{
    ControlPtr item;
    long i;

    if (dlg->items != NULL) {
        for (i = 0; i < dlg->itemCount; i++) {
            item = &((ControlPtr)dlg->items)[i];
            if (item->data != NULL) {
                switch (item->kind) {
                case kResCtrlItem:
                    if (item->data != NULL)
                        DisposeControl(item->data);
                    break;
                case kIconItem:
                    DisposeCIcon((Image_t *)item->data);
                    break;
                case kPicItem:
                    QDReleaseResource((PicHandle)item->data);
                    break;
                default:
                    if (item->data != NULL)
                        DeallocAPointer(item->data);
                    break;
                }
            }
        }
    }
    DisposeGWorldContainer((GWorldContainer *)dlg);
    if (dlg->items != NULL)
        DeallocAPointer(dlg->items);
    DeallocAPointer(dlg);
    gDialogCount -= 1;
    gDialogStack[gDialogCount] = NULL;
    if (gDialogCount > 0) {
        i = gDialogCount - 1;
        gCurDialog = gDialogStack[i];
    } else {
        gCurDialog = NULL;
    }
    RedrawDialogs(1);
    ScreenEndScene();
}

/* GetDialogItem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cf960
void GetDialogItem(DialogPtr dlg, long item, short *type, void **handle,
                   Rect *box)
{
    ControlRecord *rec;

    rec = (ControlRecord *)nv_GetDialogItemRecord(dlg, item);
    if (type != NULL)
        *type = rec != NULL ? rec->kind : 0;
    if (handle != NULL)
        *handle = rec;
    if (box != NULL) {
        box->top = rec != NULL ? rec->box.top : 0;
        box->left = rec != NULL ? rec->box.left : 0;
        box->bottom = rec != NULL ? rec->box.bottom : 0;
        box->right = rec != NULL ? rec->box.right : 0;
    }
}

/* SetDialogItem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cf9f0
void SetDialogItem(DialogPtr dlg, long item, short type, void *handle,
                   const Rect *box)
{
    ControlRecord *rec;

    rec = (ControlRecord *)nv_GetDialogItemRecord(dlg, item);
    if (rec == NULL)
        return;
    if (box != NULL) {
        rec->box.top = box->top;
        rec->box.left = box->left;
        rec->box.bottom = box->bottom;
        rec->box.right = box->right;
        rec->owner->needsUpdate = 1;
    }
}

/* SetDialogDefaultItem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfa40
void SetDialogDefaultItem(DialogPtr dlg, long value)
{
    dlg->defaultItem = value;
    dlg->needsUpdate = 1;
}

/* GetMenuItemCText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfa60
void GetMenuItemCText(ControlPtr ctl, int index, char *text)
{
    DlgListRec *list;

    if (ctl->kind != kListItem) {
        strcpy(text, "");
    } else {
        list = (DlgListRec *)ctl->data;
        if (list->count <= 0 || index < 1 || index > list->count)
            strcpy(text, "");
        else
            strcpy(text, list->item[index - 1].text);
    }
}

/* GetMenuItemText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfac0
void GetMenuItemText(ControlPtr ctl, int index, unsigned char *text)
{
    char buf[256];

    GetMenuItemCText(ctl, index, buf);
    text[0] = strlen(buf);
    memcpy(text + 1, buf, text[0]);
}

/* SetControlText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfb10
void SetControlText(ControlPtr ctl, const char *text)
{
    char changed;
    long len;

    changed = 0;
    if (ctl->data != NULL) {
        DeallocAPointer(ctl->data);
        ctl->data = NULL;
    }
    if (text != NULL) {
        len = strlen(text);
        ctl->data = AllocAPointer(len + 1);
        strcpy((char *)ctl->data, text);
        if (ctl->kind == kEditTextItem) {
            ctl->minValue = 0;
            ctl->maxValue = strlen((char *)ctl->data);
        }
        changed = 1;
    }
    if (changed)
        ctl->owner->needsUpdate = 1;
}

/* SetDialogItemText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfb90
void SetDialogItemText(ControlPtr ctl, const unsigned char *text)
{
    char buf[256];

    memcpy(buf, text + 1, text[0]);
    buf[text[0]] = 0;
    SetControlText(ctl, buf);
}

/* GetControlText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfbe0
void GetControlText(ControlPtr ctl, char *text)
{
    char *src;

    src = (char *)ctl->data;
    if (src != NULL)
        strcpy(text, src);
    else
        *text = 0;
}

/* GetDialogItemText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfc10
void GetDialogItemText(ControlPtr ctl, unsigned char *text)
{
    char buf[256];

    GetControlText(ctl, buf);
    text[0] = strlen(buf);
    memcpy(text + 1, buf, text[0]);
}

/* InvalDialogItem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfc60
void InvalDialogItem(DialogPtr dlg, long item)
{
    dlg->needsUpdate = 1;
}

/* SetControlTitle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfc70
void SetControlTitle(ControlPtr ctl, const unsigned char *text)
{
    switch (ctl->kind) {
    case 4:
    case 5:
    case 6:
        SetDialogItemText(ctl, text);
    }
}

/* SelectDialogItemText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfca0
void SelectDialogItemText(DialogPtr dlg, long item, long start, long end)
{
    ControlRecord *rec;
    long len;

    rec = (ControlRecord *)nv_GetDialogItemRecord(dlg, item);
    if (rec != NULL) {
        rec->minValue = start;
        rec->maxValue = end;
        if (rec->data != NULL) {
            len = strlen((char *)rec->data);
            if (rec->minValue < 0)
                rec->minValue = 0;
            if (rec->maxValue > len)
                rec->maxValue = len;
            if (rec->minValue > rec->maxValue)
                rec->minValue = rec->maxValue;
        } else {
            rec->minValue = 0;
            rec->maxValue = 0;
        }
        dlg->editItem = rec->itemNo;
        dlg->needsUpdate = 1;
    }
}

/* GetDialogPortRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfd30
Rect *GetDialogPortRect(DialogPtr dlg)
{
    return &dlg->portRect;
}

/* SetControlValue. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfd40
void SetControlValue(ControlPtr ctl, long value)
{
    long lo;
    long hi;

    if (ctl != NULL) {
        lo = ctl->minValue;
        hi = ctl->maxValue;
        if (value < lo)
            value = lo;
        else if (value > hi)
            value = hi;
        ctl->value = value;
        ctl->owner->needsUpdate = 1;
    }
}

/* GetControlValue. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfd80
long GetControlValue(ControlPtr ctl)
{
    long value;

    value = 0;
    if (ctl != NULL)
        value = ctl->value;
    return value;
}

/* DisableControl. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfd90
void DisableControl(ControlPtr ctl)
{
    if (ctl != NULL) {
        ctl->enabled = 0;
        ctl->owner->needsUpdate = 1;
    }
}

/* RedrawDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfdb0
void RedrawDialog(DialogPtr dlg)
{
    dlg->needsUpdate = 1;
}

/* InvalDialogRect (the InvalWindowRect twin). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cfdc0
void InvalDialogRect(DialogPtr dlg, Rect *box)
{
    dlg->needsUpdate = 1;
}

#pragma opt_lifetimes off
/* ModalDialog. STATE: MATCH 100.0%, exe 546 / obj 546, byte-identical yes.
   OPEN: none; the two-role `saved'/`list', the round trips and the pun are the only
   spellings measured to match (notes, lanes RC and CL3). */
// FUNCTION: EVNOVA 0x004cfdd0
void ModalDialog(void *filterProc, short *itemHitPtr)
{
    extern int PtInRect(long h, long v, Rect *r);
    char handled;
    long mods;
    unsigned long saved;    /* the tick rate, then the eventProc's itemHit save: one register */
    long temp;
    long doubleClick;
    long clicked;
    long localH;
    long keyChar;
    short itemHit;
    long tick;
    Point pt;
    ControlRecord *esc;
    unsigned long list;     /* the double-click threshold, then the DOWN arm's DlgListRec */
    long val;
    char res;
    ControlRecord *next;
    DialogPtr dlg;
    long keyItem;
    ControlRecord *rec;
    short lastV;
    short boxTop;
    long key;
    short boxLeft;
    long shift;
    long item;
    long i;
    short lastH;
    short boxBottom;
    long cur;
    ControlRecord *editRec;
    long found;
    long j;
    long w;
    long n;
    long clickKind;
    long pos;
    long located;
    long editKind;
    EventRecord event;
    Point pt2;

    handled = 0;
    doubleClick = 0;
    dlg = gCurDialog;
    mods = GetKeyModifiers();
    HandleIntroMusic(0);
    do {
        key = KeyboardGet2();
    } while (key != 0 && (key & 0x8000) != 0);
    if (key != 0) {
        keyChar = key;
        keyChar &= 0xff;
    }
    MouseReadButtons();
    if (MouseClicked != 0) {
        MouseClicked = 0;
        clicked = 1;
    } else {
        clicked = 0;
    }
    tick = ReadTick();
    if (tick > gBlinkTick) {
        gBlinkTick = tick + 15;
        gCaretPhase = (gCaretPhase == 0);
        if (gCurDialog != NULL)
            gCurDialog->needsUpdate = 1;
    }
    itemHit = 0;
    if (clicked != 0) {
        GetGlobalMouse(&pt2);
        localH = pt2.h - dlg->bounds.left;
        clicked = pt2.v - dlg->bounds.top;
        for (j = 0; j < dlg->itemCount; j++) {
            rec = &((ControlRecord *)dlg->items)[j];
            if (PtInRect(localH, clicked, &rec->box)) {
                itemHit = rec->itemNo;
                if (rec->kind != kPicItem && rec->kind != kIconItem &&
                    rec->kind != 0 && rec->kind != 8 && rec->kind != 1) {
                    dlg->editItem = rec->itemNo;
                    dlg->needsUpdate = 1;
                }
                break;
            }
        }
        saved = TimerGetTickRate();
        list = saved / 2;
        list = (long)list;  /* code-free signedness round trip: the exe's EBX colouring */
        if (TimerGetTicks() - gClickTime < list) {
            if (gClickItem == rec && gClickDialog == dlg)
                doubleClick = 1;
        }
        gClickTime = TimerGetTicks();
        gClickItem = rec;
        gClickDialog = dlg;
        if (dlg->eventProc != NULL) {
            event.where.h = localH;
            event.where.v = clicked;
            event.modifiers = mods;
            saved = *(unsigned long *)&itemHit;   /* the exe's dword load; `= itemHit' is movsx */
            event.what = kMouseDown;
            res = ((DialogEventProc)dlg->eventProc)(dlg, &event, &itemHit);
            if (res != 0)
                dlg->needsUpdate = 1;
            else
                itemHit = saved;
        } else {
            res = 0;
        }
        if (itemHit != 0 && res == 0) {
            switch (rec->kind) {
            case kEditTextItem:
                clickKind = 3;
                break;
            case kListItem:
                clickKind = 2;
                break;
            case 4:
            case 5:
            case 6:
                clickKind = 1;
                break;
            default:
                clickKind = 0;
                break;
            }
            if (clickKind == 1) {
                keyChar = doubleClick = rec->value;
                lastV = pt2.v;
                lastH = pt2.h;
                switch (rec->kind) {
                case 4:
                    temp = 1;
                    break;
                case 5:
                case 6:
                    temp = (doubleClick == 0);
                    break;
                }
                rec->value = temp;
                DrawDialog(dlg);
                do {
                    GetGlobalMouse(&pt2);
                    if (pt2.h != lastH || pt2.v != lastV) {
                        if (PtInRect(pt2.h - dlg->bounds.left,
                                     pt2.v - dlg->bounds.top, &rec->box))
                            rec->value = temp;
                        else
                            rec->value = keyChar;
                        DrawDialog(dlg);
                        lastV = pt2.v;
                        lastH = pt2.h;
                    }
                    KeyboardKbhit();
                } while (MouseReadButtons() != 0);
                if (rec->value != doubleClick) {
                    switch (rec->kind) {
                    case 4:
                        rec->value = 0;
                    }
                } else {
                    itemHit = 0;
                }
            } else if (clickKind == 2) {
                val = PopUpListSelect(dlg, rec);
                if (val != 0)
                    rec->value = val;
                else
                    itemHit = 0;
                dlg->needsUpdate = 1;
            } else if (clickKind == 3) {
                if (rec->data != NULL) {
                    keyChar = localH - (rec->box.left + 2);
                    localH = strlen((char *)rec->data);
                    located = 0;
                    temp = (long)rec->data;
                    if (doubleClick != 0) {
                        rec->minValue = 0;
                        rec->maxValue = localH;
                    } else {
                        for (pos = 0; pos < localH; pos++) {
                            w = TextWidth((char *)temp, pos + 1);
                            if (keyChar < w) {
                                located = 1;
                                rec->maxValue = pos;
                                rec->minValue = pos;
                                break;
                            }
                        }
                        if (located == 0) {
                            rec->maxValue = localH;
                            rec->minValue = rec->maxValue;
                        }
                    }
                }
            }
            if (itemHit != 0) {
                dlg->editItem = rec->itemNo;
                dlg->needsUpdate = 1;
            }
        }
    } else {
        if (key != 0) {
            if (dlg->eventProc != NULL) {
                GetGlobalMouse(&pt);
                event.where.h = pt.h - dlg->bounds.left;
                event.where.v = pt.v - dlg->bounds.top;
                event.what = kKeyDown;
                event.message = key;
                event.modifiers = mods;
                handled = ((DialogEventProc)dlg->eventProc)(dlg, &event,
                                                            &itemHit);
                if (handled != 0)
                    dlg->needsUpdate = 1;
                else
                    itemHit = 0;
            }
            if (handled == 0 && keyChar != 0) {
                switch (keyChar) {
                case ASCII_TAB:
                    shift = KeyIsDown(kScanLShift) || KeyIsDown(kScanRShift);
                    item = dlg->editItem;
                    clicked = item;
                    if (item == 0)
                        break;
                    for (;;) {
                        if (shift == 0) {
                            if (item <= dlg->itemCount)
                                dlg->editItem += 1;
                            else
                                dlg->editItem = 1;
                        } else {
                            if (item > 1)
                                dlg->editItem -= 1;
                            else
                                dlg->editItem = dlg->itemCount;
                        }
                        item = dlg->editItem;
                        if (item == clicked)
                            break;
                        next = &((ControlRecord *)dlg->items)[item - 1];
                        switch (next->kind) {
                        case 0:
                        case 1:
                        case 8:
                        case kIconItem:
                        case kPicItem:
                            continue;
                        }
                        boxTop = next->box.top;
                        boxLeft = next->box.left;
                        boxBottom = next->box.bottom;
                        if (next->box.right < 0)
                            continue;
                        if (boxLeft > dlg->portRect.right)
                            continue;
                        if (boxBottom < 0)
                            continue;
                        if (boxTop > dlg->portRect.bottom)
                            continue;
                        break;
                    }
                    dlg->needsUpdate = 1;
                    break;
                case ASCII_RETURN:
                    keyItem = dlg->defaultItem;
                    if (keyItem == 0)
                        break;
                    rec = (ControlRecord *)nv_GetDialogItemRecord(dlg, keyItem);
                    itemHit = rec->itemNo;
                    dlg->needsUpdate = 1;
                    break;
                case ASCII_ESCAPE:
                    tick = dlg->cancelItem;
                    if (tick == 0)
                        break;
                    esc = (ControlRecord *)nv_GetDialogItemRecord(dlg, tick);
                    itemHit = esc->itemNo;
                    dlg->needsUpdate = 1;
                    break;
                default:
                    cur = dlg->editItem;
                    if (cur == 0)
                        break;
                    editRec = (ControlRecord *)nv_GetDialogItemRecord(dlg, cur);
                    editKind = editRec->kind;
                    if (editKind == kEditTextItem) {
                        EditTextKey(editRec, keyChar, mods);
                        itemHit = editRec->itemNo;
                        dlg->needsUpdate = 1;
                    } else if (keyChar == 0x20) {
                        switch (editKind) {
                        case 4:
                            itemHit = editRec->itemNo;
                            break;
                        case 5:
                        case 6:
                            editRec->value = (editRec->value == 0);
                            break;
                        }
                        dlg->needsUpdate = 1;
                    } else if (keyChar == ASCII_UPARROW) {
                        if (editKind == kListItem && editRec->value > 1) {
                            editRec->value -= 1;
                            dlg->needsUpdate = 1;
                        }
                    } else if (keyChar == ASCII_DOWNARROW) {
                        if (editKind == kListItem) {
                            list = (unsigned long)editRec->data;
                            list = (long)list;  /* the same round trip */
                            if (editRec->value < ((DlgListRec *)list)->count) {
                                editRec->value += 1;
                                dlg->needsUpdate = 1;
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            if (dlg->eventProc != NULL) {
                GetGlobalMouse(&pt);
                event.what = kNullEvent;
                event.where.h = pt.h - dlg->bounds.left;
                event.where.v = pt.v - dlg->bounds.top;
                event.modifiers = mods;
                res = ((DialogEventProc)dlg->eventProc)(dlg, &event,
                                                        &itemHit);
                if (res != 0)
                    dlg->needsUpdate = 1;
                else
                    itemHit = 0;
            }
        }
    }
    *itemHitPtr = itemHit;
    gLastModifiers = mods;
    found = 0;
    for (i = 0; i < gDialogCount; i++)
        found = found || gDialogStack[i]->needsUpdate;
    if (found != 0) {
        gDisableUpdatesMode = 1;
        for (n = 0; n < gDialogCount; n++)
            DrawDialog(gDialogStack[n]);
        gDisableUpdatesMode = 0;
        ScreenEndScene();
    }
}

#pragma opt_lifetimes reset

/* SetControlTextKeepSelection. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d0610
void SetControlTextKeepSelection(ControlPtr ctl, const char *text)
{
    long selStart;
    long selEnd;
    long caret;

    selStart = ctl->minValue;
    selEnd = ctl->maxValue;
    caret = ctl->value;
    SetControlText(ctl, text);
    ctl->minValue = selStart;
    ctl->maxValue = selEnd;
    ctl->value = caret;
}

/* EditTextKey. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d0640
void EditTextKey(ControlRecord *rec, long key, long mods)
{
    char buf[256];
    char prefix[256];
    char prefix3[256];
    char prefix2[256];
    char dest2[256];
    char dest[256];
    char dest3[256];
    long len;
    long pos;

    GetControlText((ControlPtr)rec, buf);
    len = strlen(buf);
    switch (key) {
    case ASCII_UPARROW:
        rec->minValue = 0;
        if ((mods & shiftKey) == 0)
            rec->maxValue = rec->minValue;
        break;
    case ASCII_LEFTARROW:
        if (rec->minValue > 0)
            rec->minValue -= 1;
        if ((mods & shiftKey) == 0)
            rec->maxValue = rec->minValue;
        break;
    case ASCII_DOWNARROW:
        rec->maxValue = len;
        if ((mods & shiftKey) == 0)
            rec->minValue = rec->maxValue;
        break;
    case ASCII_RIGHTARROW:
        if (rec->maxValue < len)
            rec->maxValue += 1;
        if ((mods & shiftKey) == 0)
            rec->minValue = rec->maxValue;
        break;
    case ASCII_BACKSPACE:
        if (rec->maxValue != rec->minValue) {
            memmove(buf + rec->minValue, buf + rec->maxValue,
                    len - rec->maxValue + 1);
            SetControlTextKeepSelection((ControlPtr)rec, buf);
            rec->maxValue = rec->minValue;
        } else if (rec->minValue > 0) {
            if (rec->minValue > len)
                rec->minValue = len;
            if (rec->minValue == len) {
                len -= 1;
                buf[len] = 0;
                SetControlTextKeepSelection((ControlPtr)rec, buf);
            } else {
                strcpy(prefix, buf);
                pos = rec->minValue - 1;
                prefix[pos] = 0;
                sprintf(dest, "%s%s", prefix, buf + rec->minValue);
                SetControlTextKeepSelection((ControlPtr)rec, dest);
            }
            rec->minValue -= 1;
            rec->maxValue = rec->minValue;
        }
        break;
    case ASCII_DELETE:
        if (rec->maxValue != rec->minValue) {
            memmove(buf + rec->minValue, buf + rec->maxValue,
                    len - rec->maxValue + 1);
            SetControlTextKeepSelection((ControlPtr)rec, buf);
            rec->maxValue = rec->minValue;
        } else if (rec->minValue < len) {
            if (rec->minValue < 0)
                rec->minValue = 0;
            if (rec->minValue == len - 1) {
                len -= 1;
                buf[len] = 0;
                SetControlTextKeepSelection((ControlPtr)rec, buf);
            } else {
                strcpy(prefix2, buf);
                prefix2[rec->minValue] = 0;
                sprintf(dest2, "%s%s", prefix2, buf + (rec->minValue + 1));
                SetControlTextKeepSelection((ControlPtr)rec, dest2);
            }
            rec->maxValue = rec->minValue;
        }
        break;
    default:
        if (!isprint(key))
            break;
        if (len >= 0xff)
            break;
        if (rec->maxValue != rec->minValue) {
            memmove(buf + rec->minValue, buf + rec->maxValue,
                    len - rec->maxValue + 1);
            rec->maxValue = rec->minValue;
        }
        if (rec->minValue == 0) {
            sprintf(dest3, "%c%s", key, buf);
            SetControlTextKeepSelection((ControlPtr)rec, dest3);
        } else if (rec->minValue <= len) {
            strcpy(prefix3, buf);
            prefix3[rec->minValue] = 0;
            sprintf(dest3, "%s%c%s", prefix3, key, buf + rec->minValue);
            SetControlTextKeepSelection((ControlPtr)rec, dest3);
        } else {
            sprintf(dest3, "%s%c", buf, key);
            SetControlTextKeepSelection((ControlPtr)rec, dest3);
        }
        rec->minValue += 1;
        rec->maxValue = rec->minValue;
        break;
    }
}

/* DrawBevel. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d0a50
void DrawBevel(Rect *r, const RGBColor *fill, const RGBColor *light,
                  const RGBColor *dark)
{
    if (fill != NULL) {
        RGBForeColor(fill);
        PaintRect(r);
    }
    RGBForeColor(light);
    MoveTo(r->left + 1, r->top + 1);
    LineTo(r->right - 1, r->top + 1);
    MoveTo(r->left + 1, r->top + 1);
    LineTo(r->left + 1, r->bottom - 1);
    RGBForeColor(dark);
    MoveTo(r->right - 2, r->bottom - 2);
    LineTo(r->left + 1, r->bottom - 1);
    MoveTo(r->right - 2, r->bottom - 2);
    LineTo(r->right - 1, r->top + 1);
}

/* MeasureTextBox. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d0b70
void MeasureTextBox(const char *text, int left, int bottom, Rect *box)
{
    long width;
    long height;

    width = MeasureCString(text);
    height = MeasureWrappedText(text, strlen(text), width);
    box->top = (bottom + 3) - height;
    box->left = left;
    box->bottom = bottom + 3;
    box->right = left + width;
}

/* DrawDottedFrame. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d0bd0
void DrawDottedFrame(Rect *r)
{
    int at;
    unsigned int n;

    at = r->left;
    n = 0;
    if (at < r->right) {
        do {
            if ((n & 1) != 0) {
                MoveTo(at, r->top);
                LineTo(at + 1, r->top);
                MoveTo(at, r->bottom);
                LineTo(at + 1, r->bottom);
            }
            at = at + 1;
            n = n + 1;
        } while (at < r->right);
    }
    at = r->top;
    n = 0;
    if (at < r->bottom) {
        do {
            if ((n & 1) == 0) {
                if (n == 9)
                    n = 0;
            } else {
                MoveTo(r->left, at);
                LineTo(r->left, at + 1);
                MoveTo(r->right, at);
                LineTo(r->right, at + 1);
            }
            at = at + 1;
            n = n + 1;
        } while (at < r->bottom);
    }
    MoveTo(((Point *) r)[1].h - 1, ((Point *) r)[1].v - 1);
    LineTo(((Point *) r)[1].h, ((Point *) r)[1].v);
}

/* DrawDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d0d00
void DrawDialog(DialogPtr dlg)
{
    long x;
    long i;
    long y;
    char *shown;
    long caretX;
    void (*updateProc)(void);
    long rightBound;
    long width;
    long rowWidth;
    long titleWidth;
    short maxW;
    ControlRecord *item;
    DlgListRec *list;
    long row;
    long count;
    long rowY;
    char *entry;
    long prefix;
    long titleX;
    long titleY;
    CGrafPtr savedPort;
    Rect sel;
    Rect focus;
    Rect bevel;
    Rect focus2;

    if (dlg->visible != 0) {
        if (gWind == NULL)
            SetupWind();
        GetPort(&savedPort);
        SetPort(dlg->port);
        dlg->needsUpdate = 0;
        RGBForeColor(&rgbBlack);
        BackColor(&rgbWhite);
        EraseRect(&dlg->portRect);
        FrameRect(&dlg->portRect);

        for (i = 0; i < dlg->itemCount; i++) {
            item = (ControlRecord *)((char *)dlg->items + i * kDialogItemSize);
            Rect box = item->box;
            if (item->enabled == 0)
                continue;
            TextFace(1);
            TextSize(0xc);
            TextFont(0);
            switch (item->kind) {
            case kListItem: {
                Rect label;
                Rect value;

                list = (DlgListRec *)item->data;
                label.top = box.top;
                label.left = box.left;
                label.bottom = box.bottom;
                label.right = box.right;
                value.top = box.top;
                value.left = box.left;
                value.bottom = box.bottom;
                value.right = box.right;
                label.left += 100;
                value.left += 5;
                value.right = label.left;
                EraseRect(&value);
                RGBForeColor(&rgbBlack);
                MoveTo(value.left, value.top + (CurrentGrafPort->txSize + 20) / 2 - 1);
                DrawCString(list->title);

                if (item->minValue == 0) {
                    if (list->count <= 0) {
                        shown = list->title;
                    } else {
                        shown = list->item[item->value - 1].text;
                    }
                    DrawBevel(&label, &bevelGray2, &bevelGray1, &bevelGray4);
                    RGBForeColor(&rgbBlack);
                    width = MeasureCString(shown) + 3;
                    MoveTo(label.right - width,
                           label.top + (CurrentGrafPort->txSize + 20) / 2 - 1);
                    DrawCString(shown);
                    if (dlg->editItem == item->itemNo) {
                        focus.top = label.top;
                        focus.left = label.left;
                        focus.bottom = label.bottom;
                        focus.right = label.right;
                        InsetRect(&focus, 3, 3);
                        focus.right -= 1;
                        focus.bottom -= 1;
                        DrawDottedFrame(&focus);
                    }
                } else {
                    count = list->count;
                    label.bottom = label.top + (count * 20);
                    EraseRect(&label);
                    DrawBevel(&label, &bevelGray2, &bevelGray1, &bevelGray4);
                    RGBForeColor(&rgbBlack);
                    for (row = 0; row < list->count; row++) {
                        entry = list->item[row].text;
                        rowWidth = MeasureCString(entry) + 3;
                        rowY = row * 20;
                        MoveTo(label.right - rowWidth,
                               label.top + rowY + (CurrentGrafPort->txSize + 20) / 2 - 1);
                        DrawCString(list->item[row].text);
                        if (row == item->value - 1) {
                            Rect hilite;

                            hilite.left = label.left + 2;
                            hilite.right = label.right - 2;
                            hilite.top = label.top + rowY + 2;
                            hilite.bottom = hilite.top + 0x12;
                            InvertRect(&hilite);
                        }
                    }
                }
                break;
            }
            case kCheckBoxItem:
            case kRadioButtonItem: {
                bevel.top = box.top;
                bevel.left = box.left;
                bevel.bottom = box.bottom;
                bevel.right = box.right;
                bevel.right = bevel.left + 17;
                bevel.bottom = bevel.top + 17;
                if (item->value != 0) {
                    DrawBevel(&bevel, &bevelGray2, &bevelGray4, &bevelGray1);
                    RGBForeColor(&rgbWhite);
                    MoveTo(bevel.left + 3, bevel.top + 3);
                    LineTo(bevel.right - 3, bevel.bottom - 3);
                    MoveTo(bevel.right - 3, bevel.top + 3);
                    LineTo(bevel.left + 3, bevel.bottom - 3);
                } else {
                    DrawBevel(&bevel, &bevelGray2, &bevelGray1, &bevelGray4);
                }
                RGBForeColor(&rgbBlack);
                FrameRect(&bevel);
                if (item->data != NULL) {
                    x = bevel.left + 20;
                    y = (box.top + box.bottom) / 2 + 5;
                    MoveTo(x, y);
                    DrawCString((char *)item->data);
                    if (item->itemNo == dlg->editItem) {
                        Rect label;

                        MeasureTextBox((char *)item->data, x, y, &label);
                        DrawDottedFrame(&label);
                    }
                }
                break;
            }
            case kButtonItem: {
                if (item->value != 0) {
                    DrawBevel(&box, &bevelGray3, &bevelGray4, &bevelGray1);
                    RGBForeColor(&rgbBlack);
                    FrameRect(&box);
                    RGBForeColor(&rgbWhite);
                } else {
                    DrawBevel(&box, &bevelGray2, &bevelGray1, &bevelGray4);
                    RGBForeColor(&rgbBlack);
                    FrameRect(&box);
                }
                if (item->data != NULL) {
                    titleWidth = MeasureCString((char *)item->data);
                    titleY = (box.bottom + box.top + CurrentGrafPort->txSize) / 2 - 1;
                    titleX = (box.right + box.left) / 2 - titleWidth / 2;
                    MoveTo(titleX, titleY);
                    DrawCString((char *)item->data);
                    if (item->itemNo == dlg->editItem) {
                        focus2.top = box.top;
                        focus2.left = box.left;
                        focus2.bottom = box.bottom;
                        focus2.right = box.right;
                        InsetRect(&focus2, 3, 3);
                        focus2.right -= 1;
                        focus2.bottom -= 1;
                        DrawDottedFrame(&focus2);
                    }
                }
                RGBForeColor(&rgbBlack);
                break;
            }
            case kResourceControlItem:
                break;
            case kStaticTextItem:
                if (item->data != NULL) {
                    TETextBox(item->data, strlen((char *)item->data), &box, 0);
                    if (item->itemNo == dlg->editItem)
                        DrawDottedFrame(&box);
                }
                break;
            case kEditTextItem: {
                bevel.top = box.top;
                bevel.left = box.left;
                bevel.bottom = box.bottom;
                bevel.right = box.right;
                InsetRect(&bevel, -1, -1);
                DrawBevel(&bevel, &rgbWhite, &bevelGray1, &rgbWhite);
                RGBForeColor(&rgbBlack);
                FrameRect(&box);
                if (item->data != NULL) {
                    Rect inner;

                    MoveTo(box.left + 3, box.bottom - 3);
                    inner.left = box.left + 3;
                    inner.right = box.right - 3;
                    inner.top = box.top + 3;
                    inner.bottom = box.bottom;
                    DrawTextRun(item->data, strlen((char *)item->data), &inner, 0, 0);
                }
                if (dlg->editItem == item->itemNo) {
                    if (item->minValue < item->maxValue) {
                        sel.top = box.top + 1;
                        sel.left = box.left + 1;
                        sel.bottom = box.bottom - 1;
                        sel.right = box.right - 1;
                        if (item->maxValue < strlen((char *)item->data)) {
                            maxW = TextWidth((char *)item->data, item->maxValue);
                            rightBound = sel.left + maxW;
                            if (sel.right > rightBound)
                                sel.right = rightBound;
                        }
                        if (item->minValue > 0)
                            sel.left += TextWidth((char *)item->data, item->minValue);
                        InvertRect(&sel);
                    } else if (gCaretPhase != 0) {
                        caretX = box.left + 2;
                        prefix = item->minValue > 0
                            ? TextWidth((char *)item->data, item->minValue) : 0;
                        caretX += prefix;
                        if (caretX < box.right - 3) {
                            MoveTo(caretX, box.bottom - 3);
                            LineTo(caretX, box.top + 3);
                        }
                    }
                }
                break;
            }
            case kIconItem:
                nv_BlitImageOr(&box, (Image_t *)item->data);
                break;
            case kPictureItem:
                DrawPicHandle((PicHandle)item->data, &box);
                break;
            }
        }

        updateProc = (void (*)(void))dlg->updateProc;
        if (updateProc != NULL)
            updateProc();

        CopyBits(GetGWCPixMap((GWorldContainer *)dlg),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                 (Rect *)&dlg->portRect, &dlg->bounds, 0, 0);
        SetPort(savedPort);
        ScreenEndScene();
    }
}

/* nv_UpdateDialogWindow. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d16f0
void nv_UpdateDialogWindow(int all)
{
    DialogPtr dlg;
    int i;
    DialogPtr cur;

    if (all != 0) {
        for (i = 0; i < gDialogCount; i++) {
            dlg = gDialogStack[i];
            if (dlg->visible != 0)
                CopyBits(GetGWCPixMap((GWorldContainer *)dlg),
                         GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                         (Rect *)&dlg->portRect, &dlg->bounds, 0, 0);
        }
    } else {
        cur = gCurDialog;
        if (cur->visible != 0)
            CopyBits(GetGWCPixMap((GWorldContainer *)cur),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                     (Rect *)&cur->portRect, &cur->bounds, 0, 0);
    }
    ScreenEndScene();
}

/* SetDialogGWorld. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d17a0
void SetDialogGWorld(DialogPtr dlg)
{
    SetGWorldContainer((GWorldContainer *)dlg);
}

/* FrontWindow. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d17b0
CWindowPtr FrontWindow(void *window)
{
    return gWind;
}

/* GetDialogPort. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d17c0
CGrafPtr GetDialogPort(DialogPtr dlg)
{
    CGrafPtr port;

    if (dlg == NULL)
        port = NULL;
    else
        port = dlg->port;
    return port;
}

/* GetMouse. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d17e0
void GetMouse(Point *pt)
{
    GetGlobalMouse(pt);
    pt->h -= gCurDialog->bounds.left;
    pt->v -= gCurDialog->bounds.top;
}

/* RedrawDialogs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1810
void RedrawDialogs(int all)
{
    int i;
    int n;
    int saved;

    saved = gDisableUpdatesMode;
    gDisableUpdatesMode = 1;
    if (gPause != 0)
        MainScreenUpdate(1);
    else if (gSpriteWorld != NULL)
        RefreshSAT();
    n = gDialogCount;
    if (n > 0) {
        if (all == 0)
            n -= 1;
        for (i = 0; i < n; i++)
            DrawDialog(gDialogStack[i]);
    }
    gDisableUpdatesMode = saved;
}

/* SetDialogVisible. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1890
void SetDialogVisible(DialogPtr dlg, unsigned char value)
{
    dlg->visible = value;
    RedrawDialogs(1);
    ScreenEndScene();
}

/* SizeDialogWindow. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d18b0
void SizeDialogWindow(DialogPtr dlg, short width, short height, char update)
{
    CGrafPtr gw;

    if (dlg->portRect.right == width && dlg->portRect.bottom == height)
        return;
    dlg->bounds.right = dlg->bounds.left + width;
    dlg->bounds.bottom = dlg->bounds.top + height;
    dlg->portRect.top = dlg->bounds.top;
    dlg->portRect.left = dlg->bounds.left;
    dlg->portRect.bottom = dlg->bounds.bottom;
    dlg->portRect.right = dlg->bounds.right;
    OffsetRect(&dlg->portRect, -dlg->bounds.left, -dlg->bounds.top);
    gw = dlg->port;
    gw->pix.width = width;
    gw->pix.height = height;
    {
        BitmapInfo bmi = {0};

        bmi.bmiHeader.biSize = kDIBHeaderSize;
        bmi.bmiHeader.biWidth = gw->pix.width;
        bmi.bmiHeader.biHeight = -gw->pix.height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = gw->pix.pixelSize;
        bmi.bmiHeader.biCompression = 0;
        bmi.bmiHeader.biSizeImage = 0;
        DeleteObject(gw->bitmap);
        gw->bitmap = CreateDIBSection(gw->hdc, &bmi, 0, &gw->pix.baseAddr, NULL, 0);
    }
    gw->pix.rowBytes = gw->pix.pixelSize / 8 * gw->pix.width;
    dlg->port->pix.bounds.right = dlg->port->pix.bounds.left + width;
    dlg->port->pix.bounds.bottom = dlg->port->pix.bounds.top + height;
    dlg->port->pix.width = width;
    dlg->port->pix.height = height;
    dlg->gwBounds.right = dlg->gwBounds.left + width;
    dlg->gwBounds.bottom = dlg->gwBounds.top + height;
    if (update)
        dlg->needsUpdate = 1;
}

/* OffsetDialogWindow. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1a30
void OffsetDialogWindow(DialogPtr dlg, short dh, short dv, char redraw)
{
    OffsetRect(&dlg->bounds, dh, dv);
    dlg->needsUpdate = 1;
}

/* nv_NewDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1a60
ScrollRec **nv_NewDialog(const Rect *rView, const Rect *dataBounds,
                         Point cellSize, ListDrawProcPtr drawProc, CGrafPtr port,
                         unsigned char hasScrollBar)
{
    ScrollRec **list;
    Rect *r;
    Point *p;

    list = (ScrollRec **)AllocAHandle2(sizeof(ScrollRec), 0);
    memset(*list, 0, sizeof(ScrollRec));
    (*list)->port = port;
    r = &(*list)->rView;
    r->top = rView->top;
    r->left = rView->left;
    r->bottom = rView->bottom;
    r->right = rView->right;
    r = &(*list)->dataBounds;
    r->top = dataBounds->top;
    r->left = dataBounds->left;
    r->bottom = dataBounds->bottom;
    r->right = dataBounds->right;
    /* the exe writes the cell size through a Point pointer: any other
       spelling costs 3.9 points */
    p = &(*list)->cellSize;
    p->v = cellSize.v;
    p->h = cellSize.h;
    (*list)->drawProc = drawProc;
    (*list)->hasScrollBar = hasScrollBar;
    (*list)->drawIt = 0;
    (*list)->cells = (char **)AllocAHandle2(1, 0);
    (*list)->cellArray = (short *)AllocAPointerClear(((*list)->dataBounds.bottom + 1) * 2);
    (*list)->maxIndex = 0;
    return list;
}

/* LDispose. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1b40
void LDispose(ScrollRec **list)
{
    if ((*list)->cells != NULL)
        DeallocAHandle((*list)->cells);
    if ((*list)->cellArray != NULL)
        DeallocAPointer((*list)->cellArray);
    DeallocAHandle(list);
}

/* LSetDrawingMode. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1b70
void LSetDrawingMode(char value, ScrollRec **list)
{
    (*list)->drawIt = value;
}

/* LSetCell (mac _LSetCell, a Carbon import). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1b80
void LSetCell(void *text, long length, Point cell, ScrollRec **rec)
{
    long lineNo;
    short start;
    long delta;
    long k;
    long next;

    start = (*rec)->cellArray[lineNo = cell.v];
    next = cell.v + 1;
    delta = length - ((*rec)->cellArray[next] - start);
    if (delta == 0) {
        memcpy(*(*rec)->cells + start, text, length);
        return;
    }
    (*rec)->maxIndex += delta;
    if (delta > 0)
        (*rec)->cells = ResizeAHandle((*rec)->cells, (*rec)->maxIndex);
    if (cell.v != (*rec)->dataBounds.bottom - 1) {
        char *tail;

        tail = *(*rec)->cells + (*rec)->cellArray[cell.v + 1];
        memmove(tail + delta, tail,
                (*rec)->maxIndex - delta - (*rec)->cellArray[cell.v + 1]);
    }
    if (delta < 0)
        (*rec)->cells = ResizeAHandle((*rec)->cells, (*rec)->maxIndex);
    memcpy(*(*rec)->cells + (*rec)->cellArray[lineNo], text, length);
    for (k = next; k <= (*rec)->dataBounds.bottom; k++)
        (*rec)->cellArray[k] += delta;
}

/* LSetSelect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1cd0
void LSetSelect(int track, Point pt, ScrollRec **list)
{
    Point *hp;

    if (track == 0) {
        (*list)->lastClick.h = (*list)->lastClick.v = -1;
    } else {
        hp = &(*list)->lastClick;
        hp->v = pt.v;
        hp->h = pt.h;
    }
    LUpdate(list);
}

/* LGetSelect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1d10
char LGetSelect(long unused, Point *pt, ScrollRec **list)
{
    int same;

    same = 0;
    if (pt->h == (*list)->lastClick.h && pt->v == (*list)->lastClick.v)
        same = 1;
    return same;
}

/* LAutoScroll. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1d40
void LAutoScroll(ScrollRec **h)
{
    long page;
    long sel;
    ScrollRec *sc;
    long scroll;

    sc = *h;
    page = (sc->rView.bottom - sc->rView.top) / sc->cellSize.v;
    sel = sc->lastClick.v;
    if (page >= sc->dataBounds.bottom)
        return;
    if (sel == -1)
        return;
    scroll = sc->scroll;
    if (sel >= scroll && sel < scroll + page)
        return;
    LScroll(0, (sel - scroll), h);
}

/* LClick. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1db0
char LClick(Point where, short modifiers, ScrollRec **h)
{
    Rect r;
    long t;
    long line;
    long thumb;

    SetRect(&r, 0, 0, 15, (*h)->rView.bottom - (*h)->rView.top);
    OffsetRect(&r, (*h)->rView.right, (*h)->rView.top);
    if ((*h)->hasScrollBar == 0 || PtInRect(where.h, where.v, &r) == 0) {
        line = (where.v - (*h)->rView.top) / (*h)->cellSize.v + (*h)->scroll;
        (*h)->lastClick.h = 0;
        (*h)->lastClick.v = line;
    } else {
        t = ((*h)->rView.bottom - (*h)->rView.top) / (*h)->cellSize.v;
        if (where.v < r.top + 15) {
            (*h)->scroll -= 1;
        } else if (where.v > r.bottom - 15) {
            (*h)->scroll += 1;
        } else {
            thumb = ((*h)->scroll
                    * (((*h)->rView.bottom - (*h)->rView.top - 45)
                       / (float)((*h)->dataBounds.bottom - t))
                    + 15.0f + (*h)->rView.top);
            if (where.v < thumb)
                (*h)->scroll -= t;
            else if (where.v > thumb + 15)
                (*h)->scroll += t;
        }
        if ((*h)->scroll > (*h)->dataBounds.bottom - t)
            (*h)->scroll = (*h)->dataBounds.bottom - t;
        if ((*h)->scroll < 0)
            (*h)->scroll = 0;
    }
    (*h)->clikTime = ReadTick();
    return 0;
}

/* GetListCellRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1f50
void GetListCellRect(Rect *box, Point cell, ScrollRec **h)
{
    SetRect(box, 0, (*h)->cellSize.v * cell.v, (*h)->cellSize.h, (*h)->cellSize.v * (cell.v + 1));
    OffsetRect(box, 0, (*h)->cellSize.v * (*h)->scroll);
    OffsetRect(box, (*h)->rView.left, (*h)->rView.top);
}

/* LScroll. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d1fb0
void LScroll(long unused, short delta, ScrollRec **h)
{
    ScrollRec *sc;
    long limit;

    if ((*h)->hasScrollBar == 0)
        return;
    (*h)->scroll += delta;
    limit = (*h)->dataBounds.bottom - ((*h)->rView.bottom - (*h)->rView.top) / (*h)->cellSize.v;
    if ((*h)->scroll > limit)
        (*h)->scroll = limit;
    sc = *h;
    if (sc->scroll < 0)
        sc->scroll = 0;
}

/* LUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d2010
void LUpdate(void *list)
{
    Rect *vr;
    long i;
    short penV;
    long t;
    Point *pen;
    short penH;
    ScrollRec **h;
    long last;
    long line;
    ScrollRec *lp;
    long j;
    long cellH;
    short start;
    Point cell;
    CGrafPtr port;
    Rect r;
    Rect r2;
    short len;
    int nextRow;
    float scale;

    h = (ScrollRec **)list;
    if ((*h)->drawIt == 0 || (*h)->drawProc == NULL)
        return;
    cellH = (unsigned short)(*h)->cellSize.v;
    t = ((*h)->rView.bottom - (*h)->rView.top) / (short)cellH;
    line = (*h)->scroll;
    last = line + t;
    if (last > (*h)->dataBounds.bottom)
        last = (*h)->dataBounds.bottom;
    SetRect(&r, 0, 0, (*h)->cellSize.h, (short)cellH);
    OffsetRect(&r, (*h)->rView.left, (*h)->rView.top);
    while (line < last) {
        nextRow = line + 1;
        cell.h = 0;
        cell.v = line;
        lp = *h;
        start = lp->cellArray[line];
        len = -start + lp->cellArray[nextRow];
        lp->drawProc(1, lp->lastClick.v == line, &r, &cell, start, len, h);
        line++;
        OffsetRect(&r, 0, (*h)->cellSize.v);
    }
    if ((*h)->hasScrollBar != 0) {
        vr = &(*h)->rView;
        CopyRect(&r, vr);
        r.left = r.right;
        r.right = r.left + 15;
        GetPort(&port);
        pen = &port->pnSize;
        penV = pen->v;
        penH = pen->h;
        PenSize(1, 1);
        RGBForeColor(&redRGB);
        FrameRect(&r);
        CopyRect(&r2, &r);
        r2.top += 15;
        r2.bottom -= 15;
        FrameRect(&r2);
        scale = (r2.bottom - r2.top - 15) / (float)((*h)->dataBounds.bottom - t);
        r2.bottom = r2.top + 15;
        InsetRect(&r2, 1, 1);
        OffsetRect(&r2, 0, ((*h)->scroll * scale));
        FrameRect(&r2);
        for (i = 0, j = 0; i < 4; i++, j += 2) {
            MoveTo(r2.left + 3, r2.top + 3 + j);
            Line(7, 0);
        }
        CopyRect(&r2, &r);
        r2.bottom = r2.top + 15;
        InsetRect(&r2, 1, 1);
        MoveTo(r2.left, r2.bottom);
        LineTo((r2.left + r2.right) / 2, r2.top);
        LineTo(r2.right, r2.bottom);
        CopyRect(&r2, &r);
        r2.top = r2.bottom - 15;
        InsetRect(&r2, 1, 1);
        MoveTo(r2.left, r2.top);
        LineTo((r2.left + r2.right) / 2, r2.bottom);
        LineTo(r2.right, r2.top);
        PenSize(penH, penV);
    }
}

/* KeyboardHookProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d23f0
long __stdcall KeyboardHookProc(int code, unsigned long wParam, long lParam)
{
    long *key;

    key = (long *)lParam;
    if (code == HC_ACTION) {
        if (*key == VK_LWIN)
            return 1;
        if (*key == VK_RWIN)
            return 1;
    }
    return CallNextHookEx(gKeyboardHook, code, wParam, lParam);
}

/* WinGameWndProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d2440
long __stdcall WinGameWndProc(void *hwnd, unsigned long msg,
                            unsigned long wParam, long lParam)
{
    PAINTSTRUCT ps;
    WinRect wr;
    WinRect br;
    WinRect lr;
    WinRect mr;
    void *dc;
    long savedSuspend;
    MINMAXINFO *mmi;

    switch (msg) {
    case WM_ACTIVATEAPP:
        Win95ApplsActive = wParam;
        KeyboardFlush();
        if (Win95ApplsActive != 0 && gWind != NULL)
            gNeedUpdateFlag = 1;
        if (Win95ApplsActive != 0)
            RT3_Idle();
        if (Win95ApplsActive != 0)
            ResumePlay(0);
        else
            SwitchedOut(0);
        break;
    case WM_GETMINMAXINFO:
        if (gVideoWidth == 0)
            break;
        mr.bottom = gVideoHeight;
        mr.top = 0;
        mr.left = 0;
        mr.right = gVideoWidth;
        AdjustWindowRectEx(&mr, GetWindowLongA(Win95MainWindow, GWL_STYLE),
                           GetMenu(Win95MainWindow) != NULL,
                           GetWindowLongA(Win95MainWindow, GWL_EXSTYLE));
        mmi = (MINMAXINFO *)lParam;
        mmi->ptMaxSize.x = gVideoWidth;
        mmi->ptMaxSize.y = gVideoHeight;
        mmi->ptMaxTrackSize.x = mr.right - mr.left;
        mmi->ptMaxTrackSize.y = mr.bottom - mr.top;
        mmi->ptMinTrackSize.x = mmi->ptMaxTrackSize.x;
        mmi->ptMinTrackSize.y = mmi->ptMaxTrackSize.y;
        return 0;
    case WM_MOVE:
    case WM_SIZE:
        if (IsIconic(hwnd) != 0)
            Win95ApplsActive = 0;
        if (Win95MainWindow != NULL) {
            GetWindowRect(Win95MainWindow, &wr);
            MoveWindow(Win95MainWindow, wr.left, wr.top, wr.right - wr.left,
                       wr.bottom - wr.top, 1);
        }
        break;
    case WM_SETCURSOR:
        return 1;
    case WM_QUERYNEWPALETTE:
        PaletteSetPtr(0, 0x100, gGamePalette);
        break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        if ((unsigned char)wParam == VK_MENU)
            return 0;
        break;
    case WM_SYSCHAR:
        return 1;
    case WM_PAINT:
        dc = BeginPaint(hwnd, &ps);
        if (paintBlackFlag != 0) {
            GetWindowRect(hwnd, &br);
            FillRect(dc, &br, GetStockObject(BLACK_BRUSH));
            gNeedUpdateFlag = 1;
        } else if (gWind != NULL) {
            SWSetPortToWindow(gSpriteWorld);
            if (creatingPilotFile == 0) {
                savedSuspend = gDisableUpdatesMode;
                gDisableUpdatesMode = 1;
                if (gameInProgress != 0 && gameLoaded != 0) {
                    if (onPlanet != 0 || inBackground != 0) {
                        RedrawStatus(1);
                        RefreshSAT();
                        RedrawComm(1);
                    }
                } else {
                    if (loadPort.world != NULL) {
                        CopyBits(GetGWCPixMap(&loadPort),
                                 GetPortBitMapForCopyBits(
                                     GetCWindowGrafPort(gWind)),
                                 GetCWindowPortRect(gWind),
                                 GetCWindowPortRect(gWind), 0, 0);
                    } else {
                        GetWindowRect(hwnd, &lr);
                        FillRect(dc, &lr, GetStockObject(BLACK_BRUSH));
                    }
                }
                nv_UpdateDialogWindow(1);
                gDisableUpdatesMode = savedSuspend;
                ScreenEndScene();
            }
            SWSetPortToWindow(gSpriteWorld);
        }
        EndPaint(hwnd, &ps);
        return 1;
    case WM_CHAR:
        return 1;
    case WM_CLOSE:
        if (gameLoaded != 0 && screenFaded == 0)
            closeRequested = 1;
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_QUIT:
        exit(0);
    }
    HandleIntroMusic(0);
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ShutdownWinGame. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d28b0
void ShutdownWinGame(void)
{
    int wasRunning;

    JoystickDestroy();
    KeyboardDestroy();
    MouseDestroy();
    KillSoundPlayer();
    DigitalMusicShutdown();
    if (VideoWidth != 0) {
        LockVideoPage();
        DrawARect16(0, 0, gVideoWidth, gVideoHeight, 0);
        UpdateAndPageFlip();
        ReleaseVideo();
    }
    if (gUseKeyboardHook != 0) {
        if (gKeyboardHook != NULL) {
            UnhookWindowsHookEx(gKeyboardHook);
            gKeyboardHook = NULL;
        }
    } else {
        SystemParametersInfoA(SPI_SCREENSAVERRUNNING, 0, &wasRunning, 0);
    }
}

/* CreateGameWindow. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d2950
void CreateGameWindow(void *hInstance, long nCmdShow)
{
    WNDCLASSA wc;
    int classAtom;
    int screenSaverWasRunning;

    FastMemSet(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = WinGameWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "WinGameClass";
    classAtom = RegisterClassA(&wc);
    if (classAtom == 0)
        Fatal("Can't register my game class");
    Win95MainWindow = CreateWindowExA(WS_EX_APPWINDOW, "WinGameClass", "EV Nova",
                                  kMainWindowStyle, 0, 0, 0x280, 0x1e0,
                                  NULL, NULL, hInstance, NULL);
    if (Win95MainWindow == NULL)
        Fatal("Can't allocate my window");
    UpdateWindow(Win95MainWindow);
    SetFocus(Win95MainWindow);
    gOSVersionInfo.dwOSVersionInfoSize = sizeof(gOSVersionInfo);
    GetVersionExA(&gOSVersionInfo);
    if (gOSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        gUseKeyboardHook = 0;
    else
        gUseKeyboardHook = 1;
    if (gUseKeyboardHook == 0)
        SystemParametersInfoA(SPI_SCREENSAVERRUNNING, 1, &screenSaverWasRunning, 0);
    else if (gKeyboardHook == NULL)
        gKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, 0);
}

}
