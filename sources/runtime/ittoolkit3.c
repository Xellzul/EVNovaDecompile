// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* ittoolkit3.c -- the interface tool's control lists, from 0x0051db65: a window's controls built from its page, the key, click, paint and focus walkers over a list, a control's messages, links and scripts, the inserted elements. Notes: Research/notes/ittoolkit.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"
#include "ittoolkit.h"

static ITRect DAT_0086bb20[2];                   /* 0x0086bb20, unreferenced */
static ITRect gIT_SetInsertTextZeroRect;                      /* 0x0086bb10 _IT_ParamModify's empty clip */

ITControl *_IT_GetPreviousItem(ITControlList *list, ITControl *control);
extern unsigned short *UnicodeFindDelimiter(const unsigned short *s, const unsigned short *set);
extern ITElement *PT_ParseTreeRemove(ITElement *root, ITElement *node, unsigned char dispose,
                               unsigned char subtree);                   /* itmarkup.c */
long _IT_ItemCreate(WindowNode *window, ITControlList *list, ITElement *element,
                  unsigned char front, unsigned char owns);
long _IT_ParamCreate(WindowNode *window, ITControlList *list, ITElement *element);
long _IT_ParamModify(WindowNode *window, ULong index, const char *value, long unused);
extern ITControlProc IT_ItemHandlerLookup(WindowNode *owner, const char *name);  /* interfacetool.c */
void _IT_ItemDispose(WindowNode *window, ITControlList *list, ITControl *control);
long _IT_ParamDispose(WindowNode *window, ITControlList *list, ITInsert *insert);

#define kMsgState           0x03
#define kMsg4c              0x4c
#define kMsg4e              0x4e
#define kMsgActive          0x48
#define kMsg13              0x13
#define kEventFollow        9

/* _IT_ItemListCreate (build a window's controls from a page's elements: PARAMETER / PARAM elements
   recorded, each one's NAME looked up in the window's then the global text variables and a value
   inserted through 0x0051ed23; every element whose name is a registered control type becomes a
   control (its subtree skipped); the focusable list rebuilt; NULL, the list disposed, on
   failure). STATE: MATCH 100%, exe 198 / obj 198, byte-identical yes (510 bytes). OPEN: none;
   `name' / `value' block-scoped in the insert walk, one walker for both element walks, and the
   entry's dead zeros (err an initialiser; list zeroed twice, el once); notes/ittoolkit.md */
// FUNCTION: EVNOVA 0x0051db65
ITControlList *_IT_ItemListCreate(WindowNode *window, ITElement *first)
{
    long err = 0;
    long e;
    long index;
    ITControlList *list;
    ITElement *el;
    ITElement *next;
    ITInsert *ins;
    WindowNode *root;

    e = 0;
    ins = 0;
    list = 0;
    el = 0;
    next = 0;
    list = 0;
    list = MemoryAllocateClear(sizeof(ITControlList));
    if (list == 0) {
        err = kOutOfMemory;
        goto done;
    }
    list->root = first;
    window->controls = list;
    for (el = first; el != 0; el = PT_ParseTreeGetNext(first, el)) {
        if (el->name != 0 && AsByte(el->tag > 0)
            && (StringCompare("PARAMETER", el->name) == 0 || StringCompare("PARAM", el->name) == 0)) {
            e = _IT_ParamCreate(window, list, el);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    index = 0;
    for (ins = list->inserts; ins != 0; ins = ins->next, index++) {
        char *name;
        char *value;

        name = 0;
        value = 0;
        name = HashLookup(ins->element->attributes, "NAME", 0);
        if (name != 0 && window->variables != 0)
            value = HashLookup(window->variables, name, 0);
        if (name != 0 && value == 0 && gIT_DefaultHash != 0)
            value = HashLookup(gIT_DefaultHash, name, 0);
        if (value != 0) {
            e = _IT_ParamModify(window, index, value, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    for (el = first; el != 0; el = PT_ParseTreeGetNext(first, el)) {
        if (el->name != 0 && AsByte(el->tag > 0) && IT_ItemHandlerLookup(window, el->name) != 0) {
            e = _IT_ItemCreate(window, list, el, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            next = _IT_ParseTreeFindLastNode(first, el);
            if (next == 0)
                next = el;
            el = next;
        }
    }
    for (root = window; root->parent != 0; root = root->parent)
        ;
    e = _IT_ViewQueryFocus(root, 0);
    if (e != 0)
        err = e;
done:
    if (err != 0 && list != 0) {
        _IT_ItemListDispose(window, list);
        list = 0;
    }
    return list;
}

/* _IT_ItemListDispose (dispose a window's control list: the focus moved out of the window, the tracked
   item dropped when one of these controls holds it, every control and every inserted element
   removed). STATE: MATCH 100%, exe 69 / obj 69, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051dd63
void _IT_ItemListDispose(WindowNode *window, ITControlList *list)
{
    WindowNode *root;
    unsigned char hit;
    long err;
    long e;

    err = 0;
    e = _IT_ViewMoveFocus(window, 1, window, 0, 0);
    if (e != 0)
        err = e;
    root = 0;
    hit = 0;
    for (root = window; root->parent != 0; root = root->parent)
        ;
    if (root->tracking != 0) {
        _IT_ViewHandleTest(window, window->tracking, &hit);
        if (hit)
            root->tracking = 0;
    }
    while (list->first != 0)
        _IT_ItemDispose(window, list, list->first);
    while (list->inserts != 0)
        _IT_ParamDispose(window, list, list->inserts);
    MemoryDeallocate(list);
    list = 0;
    window->controls = 0;
}

/* _IT_GetItemIndex (a control's index in a list, -1 when absent). STATE: MATCH 100%, exe 19 / obj 19,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051de05
long _IT_GetItemIndex(ITControlList *list, ITControl *control)
{
    long index;
    ITControl *c;
    long err;

    err = 0;
    c = 0;
    index = -1;
    c = list->first;
    index = 0;
    while (c != 0 && c != control) {
        c = c->next;
        index++;
    }
    if (c == 0)
        index = -1;
    return index;
}

/* _IT_GetPreviousItem (the control before `control' in a list). STATE: MATCH 100%, exe 19 / obj 19,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051de30
ITControl *_IT_GetPreviousItem(ITControlList *list, ITControl *control)
{
    ITControl *prev;
    long err;

    err = 0;
    prev = 0;
    if (control != list->first) {
        prev = list->first;
        while (prev != 0 && prev->next != control)
            prev = prev->next;
    }
    return prev;
}

/* _IT_GetObjectItem (the window and control holding a focusable item, looking inside FRAME controls'
   child windows). STATE: MATCH 100%, exe 68 / obj 68, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051de58
long _IT_GetObjectItem(WindowNode *window, ITControlList *list, long item, WindowNode **ownerOut,
                  ITControl **controlOut)
{
    long err;
    long e;
    unsigned char hit;
    WindowNode *child;
    ITControl *c;

    err = 0;
    hit = 0;
    child = 0;
    c = 0;
    *ownerOut = 0;
    *controlOut = 0;
    for (c = list->first; c != 0; c = c->next) {
        e = _IT_ItemCallback(window, c, kMsgHitTest, item, &hit);
        if (e != 0) {
            err = e;
            break;
        }
        if (hit) {
            if (c->proc == IT_FrameHandler) {
                e = _IT_ItemCallback(window, c, kMsgChild, 0, &child);
                if (e != 0) {
                    err = e;
                    break;
                }
                /* BUG: the nested window's hit-test error is dropped (exe 0x0051dee1) */
                _IT_GetObjectItem(child, child->controls, item, ownerOut, controlOut);
            } else {
                *ownerOut = window;
                *controlOut = c;
            }
            break;
        }
    }
    return err;
}

/* _IT_ItemListQueryFocus (collect the focusable items of a window's controls into a handle, each control's
   own list rebuilt first). STATE: MATCH 100%, exe 58 / obj 58, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051df05
long _IT_ItemListQueryFocus(WindowNode *window, ITControlList *list, void **items)
{
    long err;
    long e;
    ITControl *c;

    err = 0;
    c = 0;
    IndirectGetSize(items);
    _IT_ViewValidateFocus(window, 0);
    if (list != 0) {
        for (c = list->first; c != 0; c = c->next) {
            e = IndirectSetSize(c->items, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = _IT_ItemCallback(window, c, kMsgItems, 0, c->items);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = IndirectAppend(items, c->items);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    IndirectGetSize(items);
done:
    return err;
}

/* _IT_ItemListHandleIdle (message 0x13 to every control of a list, up to the first error). STATE: MATCH
   100%, exe 26 / obj 26, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051df83
long _IT_ItemListHandleIdle(WindowNode *window, ITControlList *list)
{
    long err;
    long e;
    ITControl *c;

    err = 0;
    c = 0;
    for (c = list->first; c != 0; c = c->next) {
        e = _IT_ItemCallback(window, c, kMsg13, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
done:
    return err;
}

/* _IT_ItemListHandleKey (a key to a window's controls: Tab moves the focus (Shift backwards); without Alt,
   Enter / keypad Enter mean `+' and Escape `-', and the focus holder sees the key first (0x16);
   with Alt, a letter, digit or `.' (as `-'); unless auto-repeated, the first control whose KEYS
   holds that character is activated (0x17); FRAME controls pass it on (0x16)). STATE: MATCH
   100%, exe 206 / obj 206, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051dfba
long _IT_ItemListHandleKey(WindowNode *window, ITControlList *list, const ITKeyEvent *key,
                  ITClick *click)
{
    long err;
    long e;
    char accel;
    unsigned char mnemonic;
    ITControl *c;
    char *keys;
    unsigned short wide[256];
    char narrow[256];

    err = 0;
    c = 0;
    MemoryClear(click, sizeof(ITClick));
    if (key->key == kKeyTab) {
        click->followed = 1;
        e = _IT_ViewMoveFocus(window, (key->modifiers & kModShift) ? (unsigned char)0 : (unsigned char)1,
                         0, 0, 0);
        if (e != 0)
            err = e;
        goto done;
    }
    mnemonic = 0;
    accel = key->ch;
    keys = 0;
    if (key->modifiers & kModAlt) {
        if (!(accel >= '0' && accel <= '9') && !(accel >= 'A' && accel <= 'Z')) {
            if (accel >= 'a' && accel <= 'z')
                accel += 'A' - 'a';
            else if (accel == '.')
                accel = '-';
            else
                accel = 0;
        }
        mnemonic = 1;
    } else {
        if (key->key == kKeyReturn || key->key == kKeyEnter)
            accel = '+';
        else if (key->key == kKeyEscape)
            accel = '-';
        else
            accel = 0;
    }
    if (!mnemonic && window->focus->control != 0) {
        e = _IT_ItemCallback(window, window->focus->control, kMsgKey, (ULong)key, click);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (click->followed && click->window == 0) {
            click->window = window->focus->window;
            click->control = window->focus->control;
        }
    }
    if (key->modifiers & kModRepeat)
        accel = 0;
    if (accel != 0 && !click->followed) {
        for (c = list->first; c != 0 && !click->followed; c = c->next) {
            keys = _IT_SimpleFindAttribute(c->element, "KEYS");
            if (keys == 0)
                continue;
            if (c->element->wide) {
                C2UStringCopySafe(wide, (unsigned char *)&accel, sizeof(wide), 1);
                if (UnicodeFindDelimiter((unsigned short *)keys, wide) != 0) {
                    click->followed = 1;
                    break;
                }
            } else {
                StringCopySafe(narrow, &accel, 2);
                if (StringFindDelimiter(keys, narrow) != 0) {
                    click->followed = 1;
                    break;
                }
            }
        }
        if (click->followed) {
            e = _IT_ItemCallback(window, c, kMsgPress, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            click->window = window;
            click->control = c;
        }
    }
    if (accel != 0 && !click->followed) {
        for (c = list->first; c != 0 && !click->followed; c = c->next) {
            if (c->proc == IT_FrameHandler) {
                e = _IT_ItemCallback(window, c, kMsgKey, (ULong)key, click);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
        }
    }
done:
    return err;
}

/* _IT_ItemListHandleEdit (an edit command to the focused control (message event + 0x20); a link it reports
   is recorded with the focus's window and control). STATE: MATCH 100%, exe 54 / obj 54, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e21d
long _IT_ItemListHandleEdit(WindowNode *window, ITControlList *list, short event, ITClick *click)
{
    long err;
    long e;
    ITControl *control;
    long msg;
    unsigned char b;

    err = 0;
    b = 0;
    control = 0;
    msg = 0;
    MemoryClear(click, sizeof(ITClick));
    if (window->focus->control != 0) {
        msg = event + kMsgEdit;
        e = _IT_ItemCallback(window->focus->window, window->focus->control, msg, 0, click);
        if (e != 0) {
            err = e;
        } else if (click->followed && click->window == 0) {
            click->window = window->focus->window;
            click->control = window->focus->control;
        }
    }
    return err;
}

/* _IT_ItemListHandleClick (a click to the last active control under it that claims it (0x11), which then
   reports what the click does (0x15): a link, recorded with its window and control). STATE: MATCH
   100%, exe 77 / obj 77, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e2a3
long _IT_ItemListHandleClick(WindowNode *window, ITControlList *list, ITInputEvent *event,
                  ITClick *click)
{
    long err;
    long e;
    unsigned char hit;
    ITControl *c;

    err = 0;
    hit = 0;
    c = 0;
    MemoryClear(click, sizeof(ITClick));
    for (c = _IT_GetPreviousItem(list, c); c != 0; c = _IT_GetPreviousItem(list, c)) {
        if (c->active && DT_IsPointInRect(&event->where, &c->bounds)) {
            e = _IT_ItemCallback(window, c, kMsgClick, (ULong)event, &hit);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (hit)
                break;
        }
    }
    if (hit) {
        e = _IT_ItemCallback(window, c, kMsgWhat, (ULong)event, click);
        if (e != 0) {
            err = e;
        } else if (click->followed && click->window == 0) {
            click->window = window;
            click->control = c;
        }
    }
done:
    return err;
}

/* _IT_ItemListHandleTest (the controls of a list asked in turn whether they claim an item, up to the first
   that does). STATE: MATCH 100%, exe 33 / obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e359
long _IT_ItemListHandleTest(WindowNode *window, ITControlList *list, ULong item, unsigned char *hit)
{
    long err;
    long e;
    ITControl *c;

    err = 0;
    c = 0;
    *hit = 0;
    for (c = list->first; c != 0; c = c->next) {
        e = _IT_ItemCallback(window, c, kMsgHitTest, item, hit);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (*hit)
            break;
    }
done:
    return err;
}

/* _IT_ItemListHandleDraw (paint the controls of a list that meet `area', or all of them). STATE: MATCH
   100%, exe 45 / obj 45, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e3a0
long _IT_ItemListHandleDraw(WindowNode *window, ITControlList *list, const ITRect *area)
{
    long err;
    long e;
    ITControl *c;
    ITRect common;

    err = 0;
    c = 0;
    for (c = list->first; c != 0; c = c->next) {
        if (area == 0 || !DT_IsRectEmpty(DT_RectIntersect(&common, area, &c->bounds))) {
            e = _IT_ItemCallback(window, c, kMsgPaint, (ULong)area, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
done:
    return err;
}

/* _IT_ItemListHandleActivate (tell the controls of a list that the window activates (0x30) or deactivates
   (0x31)). STATE: MATCH 100%, exe 31 / obj 31, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e403
long _IT_ItemListHandleActivate(WindowNode *window, ITControlList *list, unsigned char active)
{
    long err;
    long e;
    ITControl *c;

    err = 0;
    c = 0;
    for (c = list->first; c != 0; c = c->next) {
        e = _IT_ItemCallback(window, c, active ? kMsgActivate : kMsgDeactivate, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
done:
    return err;
}

/* _IT_ItemListHandleCursor (a mouse event to the last active control of a list whose bounds hold it). STATE:
   MATCH 100%, exe 49 / obj 49, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e44c
long _IT_ItemListHandleCursor(WindowNode *window, ITControlList *list, ITInputEvent *event,
                  unsigned short *modifiers)
{
    long err;
    long e;
    unsigned char found;
    ITControl *c;

    err = 0;
    found = 0;
    c = 0;
    for (c = _IT_GetPreviousItem(list, c); c != 0; c = _IT_GetPreviousItem(list, c)) {
        if (c->active && DT_IsPointInRect(&event->where, &c->bounds)) {
            e = _IT_ItemCallback(window, c, kMsgMouse, (ULong)event, modifiers);
            if (e != 0)
                err = e;
            break;
        }
    }
    return err;
}

/* _IT_ItemCreate (a control for an element: KEYS upper-cased in place, RECT parsed and moved by the
   window's origin, its handles, its type's procedure, linked at the end of the list or at the
   front, then messages 0 (create), 0x41 (bounds), 0x43 (TITLE, or ""), 0x30 (activate), 1 and
   0x32 (list items); removed again on failure). STATE: MATCH 100%, exe 203 / obj 203, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e4b1
long _IT_ItemCreate(WindowNode *window, ITControlList *list, ITElement *element,
                  unsigned char front, unsigned char owns)
{
    long err;
    long e;
    unsigned char ok;
    ITControl *c;
    ITControl **link;
    char *value;
    void *title;
    ITRect r;

    err = 0;
    ok = 0;
    value = 0;
    title = 0;
    c = 0;
    c = MemoryAllocateClear(sizeof(ITControl));
    if (c == 0)
        goto nomem;
    c->element = element;
    c->ownsElement = owns;
    c->active = 1;
    value = _IT_SimpleFindAttribute(c->element, "KEYS");
    if (value != 0)
        String2Upper(value);
    value = _IT_SimpleFindAttribute(c->element, "RECT");
    if (value != 0) {
        _IT_ParseAttributeGetRect(&r, 0, value);
        c->bounds = r;
        DT_RectOffset(&c->bounds, window->bounds.left, window->bounds.top);
    }
    if ((c->items = IndirectAllocate(0)) == 0)
        goto nomem;
    if ((c->data = IndirectAllocate(0)) == 0)
        goto nomem;
    if ((c->proc = IT_ItemHandlerLookup(window, c->element->name)) == 0)
        err = 0;
    if (front) {
        link = &list->first;
        c->next = *link;
    } else {
        for (link = &list->first; *link != 0; link = &(*link)->next)
            ;
    }
    c->list = list;
    *link = c;
    value = _IT_SimpleFindAttribute(c->element, "TITLE");
    if (value == 0)
        value = "";
    title = IndirectInitialize(value, StringGetLength((unsigned char *)value));
    if (title == 0) {
nomem:
        err = kOutOfMemory;
        goto done;
    }
    if ((e = _IT_ItemCallback(window, c, kMsgCreate, 0, 0)) != 0
        || (e = _IT_ItemCallback(window, c, kMsgBounds, (ULong)&c->bounds, 0)) != 0
        || (e = _IT_ItemCallback(window, c, kMsgTitle, (ULong)title, 0)) != 0
        || (e = _IT_ItemCallback(window, c, kMsgActivate, 0, 0)) != 0
        || (e = _IT_ItemCallback(window, c, kMsgOpen, 0, 0)) != 0) {
        err = e;
        goto done;
    }
    e = _IT_ItemCallback(window, c, kMsgItems, 0, c->items);
    if (e != 0)
        err = e;
done:
    if (title != 0)
        IndirectDeallocate((void **)title);
    if (err != 0 && c != 0) {
        _IT_ItemDispose(window, list, c);
        c = 0;
    }
    return err;
}

/* _IT_ItemDispose (remove a control: the focus moved off it, messages 2 and 3, its handles freed,
   its element unlinked from the document when it owns it, the root's focusable list rebuilt).
   STATE: MATCH 100%, exe 103 / obj 103, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e6cc
void _IT_ItemDispose(WindowNode *window, ITControlList *list, ITControl *control)
{
    long err;
    long e;
    ITControl **link;
    WindowNode *root;

    err = 0;
    root = 0;
    if (control == window->focus->control) {
        e = _IT_ViewMoveFocus(window, 1, 0, window->focus->control, 0);
        if (e != 0)
            err = e;
    }
    e = _IT_ItemCallback(window, control, kMsgClose, 0, 0);
    if (e != 0)
        err = e;
    e = _IT_ItemCallback(window, control, kMsgState, 0, 0);
    if (e != 0)
        err = e;
    if (control->items != 0)
        IndirectDeallocate(control->items);
    if (control->data != 0)
        IndirectDeallocate((void **)control->data);
    if (control->element != 0 && control->ownsElement)
        PT_ParseTreeRemove(window->source.root, control->element, 1, 1);
    control->element = 0;
    for (link = &list->first; *link != 0; link = &(*link)->next) {
        if (*link == control) {
            *link = control->next;
            control->next = 0;
            break;
        }
    }
    MemoryDeallocate(control);
    control = 0;
    for (root = window; root->parent != 0; root = root->parent)
        ;
    e = _IT_ViewQueryFocus(root, 0);
    if (e != 0)
        err = e;
}

/* _IT_ItemExecute (follow what a control leads to, after the callback allows it (event 9): a SCRIPT
   (QUIT closes the dialog, its NAME copied to the root's result; BEEP beeps), else the link or
   the control's HREF: a URL or mailto: handed to the shell, `#anchor' a page of this file,
   `file#anchor' a page of another file loaded from the page folder). STATE: MATCH 100%, exe
   199 / obj 199, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e7b8
long _IT_ItemExecute(WindowNode *window, ITControl *control, const char *link)
{
    long err;
    long e;
    char *file;
    char *anchor;
    struct FolderSpec **folder;
    char *text;
    void *target;
    char command[256];
    long spare;

    err = 0;
    text = 0;
    file = 0;
    anchor = 0;
    folder = 0;
    spare = 0;
    e = _IT_ViewCallback(window, 0, control, kEventFollow);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (link != 0 && *link == 0)
        link = 0;
    if (link == 0) {
        text = _IT_SimpleFindAttribute(control->element, "SCRIPT");
        if (text != 0) {
            String2UpperCopySafe(command, text, sizeof(command));
            text = StringFindDelimiter(command, " ()");
            if (text != 0)
                *text = 0;
            if (StringCompare(command, "QUIT") == 0) {
                while (window->parent != 0)
                    window = window->parent;
                if (window->result != 0 && !control->element->wide) {
                    text = HashLookup(control->element->attributes, "NAME", 0);
                    if (text != 0)
                        StringCopySafe(window->result, text, 0x100);
                }
                IT_ViewDispose(window);
            } else if (StringCompare(command, "BEEP") == 0) {
                MessageBeep(0);
            }
            goto done;
        }
    }
    if ((text = (char *)link) == 0) {
        text = _IT_SimpleFindAttribute(control->element, "HREF");
        if (text == 0)
            goto done;
    }
    target = window;
    if (StringFindString(text, "://") != 0 || StringFindString(text, "mailto:") != 0) {
        e = IT_OpenBrowser(text);
        if (e != 0)
            err = e;
    } else {
        e = _IT_ViewDissectLink(window, text, &file, &anchor);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = FT_PathCreate(window->source.folder, 0, &folder);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (file != 0) {
            e = IT_ViewChange(window, folder, file, window->path, anchor);
            if (e != 0)
                err = e;
        } else {
            e = IT_ViewGoToPage(target, anchor);
            if (e != 0)
                err = e;
        }
    }
done:
    if (file != 0)
        MemoryDeallocate(file);
    if (anchor != 0)
        MemoryDeallocate(anchor);
    if (folder != 0)
        FT_PathDispose(folder);
    return err;
}

/* _IT_ItemCallback (send a message to a control's procedure, on a DC with the window's colours (the
   root's buffer for painting); an inactive control ignores 0x10..0x32; after anything but 3,
   0x10, 0x11 and 0x48 the control is asked whether it is active). STATE: MATCH 100%, exe 125 /
   obj 125, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051e9f8
long _IT_ItemCallback(WindowNode *window, ITControl *control, ULong msg, ULong a, void *b)
{
    long err;
    void *hwnd;
    long saved;
    void *dc;
    WindowNode *root;
    long e;
    WindowNode *savedWindow;
    ITControl *savedControl;

    err = 0;
    hwnd = 0;
    dc = 0;
    root = window;
    saved = 0;
    if (msg >= kMsgHitTest && msg <= kMsgItems && control->active == 0)
        goto done;
    hwnd = window->hwnd;
    for (root = window; root->parent != 0; root = root->parent)
        ;
    if (root->buffer != 0 && msg == kMsgPaint) {
        dc = DT_PeekAtSysContext(root->buffer->portSelf);
    } else {
        dc = GetDC(hwnd);
        if (dc == 0 || (saved = SaveDC(dc)) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        SetTextColor(dc, DT_2SysColor(&window->fore));
        SetBkColor(dc, DT_2SysColor(&window->back));
    }
    if (control->proc != 0) {
        savedWindow = gIT_CurrentView;
        savedControl = gIT_CurrentItem;
        gIT_CurrentView = window;
        gIT_CurrentItem = control;
        err = control->proc(hwnd, dc, control->element, control->data, msg, a, b);
        if (msg != kMsgHitTest && msg != kMsgState && msg != kMsgClick && msg != kMsgActive)
            err = control->proc(hwnd, dc, control->element, control->data, kMsgActive, 0,
                                &control->active);
        gIT_CurrentView = savedWindow;
        gIT_CurrentItem = savedControl;
    }
    e = err;
    if (err != 0)
        err = e;
done:
    if (saved != 0)
        RestoreDC(dc, saved);
    if (dc != 0)
        ReleaseDC(hwnd, dc);
    return err;
}

/* _IT_GetItemColor (offer an item to the controls of a list; the first that claims it gets 0x4c with
   `a' and then 0x4e with `b'). STATE: MATCH 100%, exe 59 / obj 59, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051eb7c
long _IT_GetItemColor(WindowNode *window, ITControlList *list, ULong item, void *a, void *b,
                  unsigned char *hit)
{
    long err;
    long e;
    unsigned char found;
    ITControl *c;

    err = 0;
    found = 0;
    c = 0;
    *hit = 0;
    for (c = list->first; c != 0; c = c->next) {
        e = _IT_ItemCallback(window, c, kMsgHitTest, item, hit);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (*hit)
            break;
    }
    if (*hit) {
        e = _IT_ItemCallback(window, c, kMsg4c, item, a);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _IT_ItemCallback(window, c, kMsg4e, item, b);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _IT_ParamCreate (record an element inserted into a window's document, at the end of the list's
   inserts). STATE: MATCH 100%, exe 40 / obj 40, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ec01
long _IT_ParamCreate(WindowNode *window, ITControlList *list, ITElement *element)
{
    long err;
    ITInsert *insert;
    ITInsert **link;
    long e;

    err = 0;
    e = 0;
    insert = 0;
    insert = MemoryAllocateClear(sizeof(ITInsert));
    if (insert == 0) {
        err = kOutOfMemory;
    } else {
        insert->element = element;
        for (link = &list->inserts; *link != 0; link = &(*link)->next)
            ;
        insert->list = list;
        *link = insert;
    }
    if (err != 0 && insert != 0) {
        _IT_ParamDispose(window, list, insert);
        insert = 0;
    }
    return err;
}

/* _IT_ParamDispose (remove an inserted element: the window's inserts lying inside it removed first,
   then it is unlinked from the document and disposed, and the record freed). STATE: MATCH 100%,
   exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ec5c
long _IT_ParamDispose(WindowNode *window, ITControlList *list, ITInsert *insert)
{
    long err;
    ITElement *after;
    ITInsert *i;
    ITElement *limit;
    ITInsert **link;
    ULong order;

    err = 0;
    link = 0;
    after = 0;
    if (insert->element != 0) {
        after = insert->element->next;
        if (after == 0)
            after = PT_ParseTreeGetNext(window->source.root, insert->element);
        limit = after;
        i = window->controls->inserts;
        while (i != 0) {
            order = i->element->order;
            if (order > insert->element->order && (after == 0 || order < after->order)) {
                _IT_ParamDispose(window, window->controls, i);
                i = 0;
            }
            if (i != 0)
                i = i->next;
            else
                i = window->controls->inserts;
        }
        PT_ParseTreeRemove(window->source.root, insert->element, 1, 1);
    }
    insert->element = 0;
    for (link = &list->inserts; *link != 0; link = &(*link)->next) {
        if (*link == insert) {
            *link = insert->next;
            insert->next = 0;
            break;
        }
    }
    MemoryDeallocate(insert);
    insert = 0;
    return 0;
}

static inline unsigned char IsElement(ITElement *e)
{
    if (e->tag > 0)
        return 1;
    return 0;
}

/* _IT_ParamModify (set the text of the page's insert at `index': the inserts recorded inside its
   element dropped, its children disposed, `value' parsed as markup and its PARAMETER / PARAM
   elements recorded, the tree linked in as the element's first children; drawing clipped to
   nothing meanwhile, the window refreshed). STATE: MATCH 100%, exe 222 / obj 222, byte-identical
   yes (647 bytes). OPEN: none; `link' / `child' are dead locals the exe's entry proves (names
   invented), the fourth argument is never read; notes/ittoolkit.md */
// FUNCTION: EVNOVA 0x0051ed23
long _IT_ParamModify(WindowNode *window, ULong index, const char *value, long unused)
{
    long err;
    ITInsert **link;
    ITElement *child;
    long e;
    ITInsert *insert;
    ITInsert *p;
    ITElement *after;
    ITElement *limit;
    ITElement *tree;
    ITElement *element;
    void **text;
    void *saved;
    ITRect clip;
    ULong order;

    err = 0;
    tree = 0;
    link = 0;
    child = 0;
    child = 0;
    limit = 0;
    insert = 0;
    text = 0;
    saved = 0;
    clip = gIT_SetInsertTextZeroRect;
    for (insert = window->controls->inserts; insert != 0 && index--; insert = insert->next)
        ;
    if (insert == 0) {
        err = kNotFound;
        goto done;
    }
    saved = DT_GetClipRegion(window->image);
    if (saved == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_SetClipRect(window->image, &clip);
    if (e != 0) {
        err = e;
        goto done;
    }
    after = insert->element->next;
    if (after == 0)
        after = PT_ParseTreeGetNext(window->source.root, insert->element);
    limit = after;
    p = window->controls->inserts;
    while (p != 0) {
        order = p->element->order;
        if (order > insert->element->order
            && (after == 0 || order < after->order)) {
            _IT_ParamDispose(window, window->controls, p);
            p = 0;
        }
        if (p != 0)
            p = p->next;
        else
            p = window->controls->inserts;
    }
    while (insert->element->child != 0)
        PT_ParseTreeRemove(window->source.root, insert->element->child, 1, 1);
    if (value != 0) {
        text = IndirectInitialize(value, StringGetLength((unsigned char *)value));
        if (text == 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = PT_ParseTreeLoad(text, "SCRIPT", "LI BR P", 0, &tree);
        if (e != 0) {
            err = e;
            goto done;
        }
        for (element = tree->child; element != 0; element = PT_ParseTreeGetNext(tree, element)) {
            if (element->name != 0 && IsElement(element)
                && (StringCompare("PARAMETER", element->name) == 0
                    || StringCompare("PARAM", element->name) == 0)) {
                e = _IT_ParamCreate(window, window->controls, element);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
        }
        PT_ParseTreeInsertNode(window->source.root, insert->element, tree, 1);
        tree = 0;
    }
    e = DT_SetClipRegion(window->image, saved);
    if (e != 0) {
        err = e;
        goto done;
    }
    DT_RegionDispose(saved);
    saved = 0;
    e = _IT_ViewValidate(window);
    if (e != 0)
        err = e;
done:
    if (tree != 0)
        PT_ParseTreeDispose(tree);
    if (text != 0) {
        IndirectDeallocate(text);
        text = 0;
    }
    if (saved != 0) {
        DT_SetClipRegion(window->image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

/* IT_ViewGetItemCount (how many controls a window has). STATE: MATCH 100%, exe 14 / obj 14, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051efaa
long IT_ViewGetItemCount(void *windowRef)
{
    WindowNode *window = (WindowNode *)windowRef;
    long n;
    ITControl *c;
    long err;

    err = 0;
    c = 0;
    n = 0;
    for (c = window->controls->first; c != 0; c = c->next)
        n++;
    return n;
}

/* IT_ViewGetNamedItem (the index of the control whose NAME attribute is `name', -1 when none). STATE:
   MATCH 100%, exe 66 / obj 66, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051efcb
long IT_ViewGetNamedItem(WindowNode *window, const char *name)
{
    long index;
    ITControl *c;
    char *value;
    char wanted[256];
    char have[256];
    WindowNode *w;
    long err;
    unsigned char isTag;

    w = window;
    err = 0;
    c = 0;
    index = 0;
    value = 0;
    String2UpperCopySafe(wanted, name, sizeof(wanted));
    for (c = window->controls->first; c != 0; c = c->next) {
        if (c->element->tag > 0)
            isTag = true;
        else
            isTag = false;
        value = HashLookup(c->element->attributes, "NAME", 0);
        if (value != 0 && *value != 0) {
            String2UpperCopySafe(have, value, sizeof(have));
            if (StringCompare(wanted, have) == 0)
                break;
        }
        index++;
    }
    if (c == 0)
        index = -1;
    return index;
}
