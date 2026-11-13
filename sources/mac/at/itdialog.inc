/* itdialog.inc -- 1.0.10: the toolkit's own dialogs on Mac, a dialog window on a page file and the modal run; ittoolkit.c includes it where the exe has them. Notes: Research/notes/maclibs.md */
#define kNoTitleBar        ((const char *)-1)
#define kItemListEmpty  -1
#define kOffScreenLeft  32000
#define kOffScreenTop   32000
#define kOffScreenRight 32300
#define kOffScreenBot   32400

extern int32_t IT_ViewChange();
extern unsigned char IT_ViewExists(void *windowRef);
extern unsigned char _PlatformIsThreaded(void);
extern pascal Boolean IT_ViewModalFilter(DialogPtr dialog, EventRecord *event, short *item);

int32_t IT_ViewCreateDialog(const char *title, unsigned char closeBox, unsigned char show,
                  const ITRect *bounds, int32_t spare, void *folder, const char *file,
                  const char *path, const char *anchor, ITWindowProc proc, int32_t refcon,
                  int32_t unused, WindowNode **out)
{
    int32_t err;
    Handle items;
    int32_t e;
    int32_t offset;
    short procID;
    ULong draw;
    WindowNode *window;
    Rect rect;
    ITRect client;
    Str255 name;

    err = 0;
    items = 0;
    window = 0;
    CHECK_WINDOWS(0);
    *out = 0;
    window = MemoryAllocateClear(sizeof(WindowNode));
    if (window == 0) {
        err = kOutOfMemory;
        goto done;
    }
    window->next = gIT_ViewList;
    gIT_ViewList = window;
    window->parent = gIT_CurrentView;
    rect = (*GetMainDevice())->gdRect;
    CopySys2DT_Rect(&client, &rect);
    if (bounds != 0) {
        CopyDT_2SysRect(&rect, bounds);
    } else {
        window->unbuffered = 1;
        SetRect(&rect, kOffScreenLeft, kOffScreenTop, kOffScreenRight, kOffScreenBot);
    }
    if (title == kNoTitleBar) {
        procID = dBoxProc;
        StringC2PCopy(name, "");
    } else if (title != 0 && *title != 0) {
        procID = noGrowDocProc;
        StringC2PCopy(name, title);
    } else {
        procID = closeBox ? noGrowDocProc : plainDBox;
        StringC2PCopy(name, "");
    }
    items = NewHandle(sizeof(short));
    if (items == 0) {
        err = kOutOfMemory;
        goto done;
    }
    **(short **)items = kItemListEmpty;
    if ((window->modal = NewColorDialog(0, &rect, name, false, procID, (WindowPtr)-1, closeBox, 0, items)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    items = 0;
#if TARGET_API_MAC_CARBON
    window->hwnd = GetDialogWindow(window->modal);
    window->dc = GetWindowPort(window->hwnd);
    GetWindowPortBounds(window->hwnd, &rect);
#else
    window->hwnd = window->modal;
    window->dc = window->hwnd;
    rect = ((GrafPtr)window->hwnd)->portRect;
#endif
    CopySys2DT_Rect(&window->bounds, &rect);
    window->titled = 1;
    e = IT_ViewCreate(kNoHwnd, 0, proc, refcon, unused, &window);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ViewChange(window, folder, file, path, anchor);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (window->unbuffered) {
        offset = (client.top - client.bottom) / 6;
        DT_RectCenter(&client, &window->frame, &client);
        if (title == kNoTitleBar)
            DT_RectOffset(&client, 0, offset);
        SizeWindow(window->hwnd, client.right - client.left, client.bottom - client.top, false);
        MoveWindow(window->hwnd, client.left, client.top, false);
#if TARGET_API_MAC_CARBON
        GetWindowPortBounds(window->hwnd, &rect);
#else
        rect = ((GrafPtr)window->hwnd)->portRect;
#endif
        CopySys2DT_Rect(&window->bounds, &rect);
    }
    if (window->parent == 0 && window->unbuffered) {
        if ((window->buffer = DT_SurfaceCreateFromWindow(window->hwnd)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    if (show) {
        draw = 0;
        if (window->buffer != 0 && !_PlatformIsThreaded())
            draw = 1;
        draw = draw != 0;
        if (draw) {
            e = _IT_ViewHandleDraw(window, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        ShowWindow(window->hwnd);
        if (!draw) {
            e = _IT_ViewHandleDraw(window, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            e = DT_SurfaceBlitRects(window->buffer, 1, &window->bounds);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_AddValidRect(window->image, &window->bounds);
            if (e != 0)
                err = e;
        }
    }
done:
    if (items != 0)
        DisposeHandle(items);
    if (err != 0 && window != 0) {
        if (window != 0)
            IT_ViewDispose(window);
        window = 0;
    }
    *out = window;
    return err;
}

int32_t IT_ViewCreateModal(void *folder, const char *file, const char *path, ITWindowProc proc,
                  int32_t refcon, char *result)
{
    int32_t err;
    int32_t e;
    ModalFilterUPP filter;
    WindowNode *view;
    short item;

    err = 0;
    view = 0;
    item = 0;
    CHECK_WINDOWS(0);
    if (result != 0)
        *result = 0;
    IT_SetCursor(0);
    filter = NewModalFilterUPP(IT_ViewModalFilter);
    if (filter == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = IT_ViewCreateDialog(kNoTitleBar, 0, 1, 0, 0, folder, file, path, 0, proc, refcon, 0, &view);
    if (e != 0) {
        err = e;
        goto done;
    }
    view->result = result;
    do {
        ModalDialog(filter, &item);
    } while (item == 0);
    IT_SetCursor(0);
done:
    if (view != 0 && IT_ViewExists(view))
        IT_ViewDispose(view);
    if (filter != 0)
        DisposeModalFilterUPP(filter);
    return err;
}
