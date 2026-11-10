/* evcarbonhack1010.c -- EVCarbonHack.o of the mac 1.0.10 fragments: on Classic it also defines the accessors the shared text calls. Notes: Research/notes/evcarbonhack.md */

Rect tempPortRect;
RgnHandle carbonTempRgn;

void CarbonHackInit(void)
{
    carbonTempRgn = NewRgn();
}

void CarbonHackCleanup(void)
{
    if (carbonTempRgn != NULL)
        DisposeRgn(carbonTempRgn);
}

#if TARGET_API_MAC_CARBON

Rect *GetWindowPortRect(WindowRef window)
{
    GetWindowBounds(window, kWindowContentRgn, &tempPortRect);
    OffsetRect(&tempPortRect, -tempPortRect.left, -tempPortRect.top);
    return &tempPortRect;
}

Rect *GetCWindowPortRect(WindowRef window)
{
    GetWindowBounds(window, kWindowContentRgn, &tempPortRect);
    OffsetRect(&tempPortRect, -tempPortRect.left, -tempPortRect.top);
    return &tempPortRect;
}

Rect *GetCPortPortRect(CGrafPtr port)
{
    GetPortBounds(port, &tempPortRect);
    return &tempPortRect;
}

CGrafPtr GetWindowGrafPort(WindowRef window)
{
    return GetWindowPort(window);
}

CGrafPtr GetCWindowGrafPort(WindowRef window)
{
    return GetWindowPort(window);
}

Rect *GetDialogPortRect(DialogRef dialog)
{
    GetWindowBounds(GetDialogWindow(dialog), kWindowContentRgn, &tempPortRect);
    OffsetRect(&tempPortRect, -tempPortRect.left, -tempPortRect.top);
    return &tempPortRect;
}

RgnHandle GetCWindowVisRgn(WindowRef window)
{
    return GetPortVisibleRegion(GetWindowPort(window), carbonTempRgn);
}

RgnHandle GetWindowVisRgn(WindowRef window)
{
    return GetPortVisibleRegion(GetWindowPort(window), carbonTempRgn);
}

RgnHandle GetPortVisRgn(CGrafPtr port)
{
    return GetPortVisibleRegion(port, carbonTempRgn);
}

Rect *GetRgnBBox(RgnHandle rgn)
{
    GetRegionBounds(rgn, &tempPortRect);
    return &tempPortRect;
}

PixMapHandle GetCWindowPixMap(WindowRef window)
{
    return GetPortPixMap(GetWindowPort(window));
}

void SetRetainedWindow(WindowRef window, Boolean retained)
{
}

#else

Rect *GetWindowPortRect(WindowRef window)
{
    tempPortRect = window->portRect;
    return &tempPortRect;
}

Rect *GetCWindowPortRect(WindowRef window)
{
    tempPortRect = window->portRect;
    return &tempPortRect;
}

CGrafPtr GetWindowGrafPort(WindowRef window)
{
    return (CGrafPtr)window;
}

CGrafPtr GetCWindowGrafPort(WindowRef window)
{
    return (CGrafPtr)window;
}

pascal PixMapHandle GetPortPixMap(CGrafPtr port)
{
    return port->portPixMap;
}

PixMapHandle GetCWindowPixMap(WindowRef window)
{
    return ((CGrafPtr)window)->portPixMap;
}

pascal CGrafPtr GetDialogPort(DialogRef dialog)
{
    return (CGrafPtr)dialog;
}

Rect *GetDialogPortRect(DialogRef dialog)
{
    tempPortRect = dialog->portRect;
    return &tempPortRect;
}

RgnHandle GetCWindowVisRgn(WindowRef window)
{
    return window->visRgn;
}

RgnHandle GetWindowVisRgn(WindowRef window)
{
    return window->visRgn;
}

RgnHandle GetPortVisRgn(CGrafPtr port)
{
    return port->visRgn;
}

pascal const BitMap *GetPortBitMapForCopyBits(CGrafPtr port)
{
    return &((GrafPtr)port)->portBits;
}

Rect *GetRgnBBox(RgnHandle rgn)
{
    tempPortRect = (*rgn)->rgnBBox;
    return &tempPortRect;
}

Rect *GetCPortPortRect(CGrafPtr port)
{
    tempPortRect = port->portRect;
    return &tempPortRect;
}

pascal OSStatus InvalWindowRect(WindowRef window, const Rect *bounds)
{
    GrafPtr savePort;

    GetPort(&savePort);
    SetPort(window);
    InvalRect(bounds);
    SetPort(savePort);
    return noErr;
}

void SetRetainedWindow(WindowRef window, Boolean retained)
{
}

#endif
