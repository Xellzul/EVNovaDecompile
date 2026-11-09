/* evcarbonhack.c -- the classic-Toolbox accessors over Carbon, the mac 1.1.1 EVCarbonHack.o (mac only; the win port's own versions are in port/blitcopy24.c and port/dialogmgr.c). Notes: Research/notes/evcarbonhack.md */

RgnHandle carbonTempRgn;
RgnHandle carbonDirtyRgn;
RgnHandle carbonEmptyRgn;
Rect tempPortRect;

/* CarbonHackInit (mac _CarbonHackInit, mac-only). STATE: see the mac gate. */
// MAC: _CarbonHackInit i386 0x000a42cf ppc 0x000afd4c
void CarbonHackInit(void)
{
    carbonTempRgn = NewRgn();
}

/* CarbonHackCleanup (mac _CarbonHackCleanup, mac-only). STATE: see the mac gate. */
// MAC: _CarbonHackCleanup i386 0x000a42e4 ppc 0x000afd78
void CarbonHackCleanup(void)
{
    if (carbonTempRgn != NULL)
        DisposeRgn(carbonTempRgn);
}

/* GetWindowPortRect (mac _GetWindowPortRect, mac-only). STATE: see the mac gate. */
// MAC: _GetWindowPortRect i386 0x000a42ff ppc 0x000afd90
Rect *GetWindowPortRect(WindowRef window)
{
    GetWindowBounds(window, kWindowContentRgn, &tempPortRect);
    OffsetRect(&tempPortRect, -tempPortRect.left, -tempPortRect.top);
    return &tempPortRect;
}

/* GetCWindowPortRect (mac _GetCWindowPortRect, mac-only). STATE: see the mac gate. */
// MAC: _GetCWindowPortRect i386 0x000a4348 ppc 0x000afdec
Rect *GetCWindowPortRect(WindowRef window)
{
    return GetWindowPortRect(window);
}

/* GetPortPortRect (mac _GetPortPortRect, mac-only). STATE: see the mac gate. */
// MAC: _GetPortPortRect i386 0x000a4351 ppc 0x000afdf0
Rect *GetPortPortRect(CGrafPtr port)
{
    GetPortBounds(port, &tempPortRect);
    return &tempPortRect;
}

/* GetWindowGrafPort (mac _GetWindowGrafPort, mac-only). STATE: see the mac gate. */
// MAC: _GetWindowGrafPort i386 0x000a4375 ppc 0x000afe28
CGrafPtr GetWindowGrafPort(WindowRef window)
{
    return GetWindowPort(window);
}

/* GetWindowPixMap (mac _GetWindowPixMap, mac-only). STATE: see the mac gate. */
// MAC: _GetWindowPixMap i386 0x000a437e ppc 0x000afe2c
PixMapHandle GetWindowPixMap(WindowRef window)
{
    return GetPortPixMap(GetWindowPort(window));
}

/* GetDialogPortRect (mac _GetDialogPortRect, mac-only). STATE: see the mac gate. */
// MAC: _GetDialogPortRect i386 0x000a4398 ppc 0x000afe54
Rect *GetDialogPortRect(DialogRef dialog)
{
    GetWindowBounds(GetDialogWindow(dialog), kWindowContentRgn, &tempPortRect);
    OffsetRect(&tempPortRect, -tempPortRect.left, -tempPortRect.top);
    return &tempPortRect;
}

/* GetCWindowVisRgn (mac _GetCWindowVisRgn, mac-only). STATE: see the mac gate. */
// MAC: _GetCWindowVisRgn i386 0x000a43e9 ppc 0x000afeb4
RgnHandle GetCWindowVisRgn(WindowRef window)
{
    return GetPortVisibleRegion(GetWindowPort(window), carbonTempRgn);
}

/* GetWindowVisRgn (mac _GetWindowVisRgn, mac-only). STATE: see the mac gate. */
// MAC: _GetWindowVisRgn i386 0x000a4414 ppc 0x000afee8
RgnHandle GetWindowVisRgn(WindowRef window)
{
    return GetPortVisibleRegion(GetWindowPort(window), carbonTempRgn);
}

/* GetPortVisRgn (mac _GetPortVisRgn, mac-only). STATE: see the mac gate. */
// MAC: _GetPortVisRgn i386 0x000a443f ppc 0x000aff1c
RgnHandle GetPortVisRgn(CGrafPtr port)
{
    return GetPortVisibleRegion(port, carbonTempRgn);
}

/* GetDialogCWindow (mac _GetDialogCWindow, mac-only). STATE: see the mac gate. */
// MAC: _GetDialogCWindow i386 0x000a445d ppc 0x000aff2c
WindowRef GetDialogCWindow(DialogRef dialog)
{
    return GetDialogWindow(dialog);
}

/* GetRgnBBox (mac _GetRgnBBox, mac-only). STATE: see the mac gate. */
// MAC: _GetRgnBBox i386 0x000a4466 ppc 0x000aff30
Rect *GetRgnBBox(RgnHandle rgn)
{
    GetRegionBounds(rgn, &tempPortRect);
    return &tempPortRect;
}

/* ScreenBitsBounds (mac _ScreenBitsBounds, mac-only). STATE: see the mac gate. */
// MAC: _ScreenBitsBounds i386 0x000a448a ppc 0x000aff68
Rect *ScreenBitsBounds(void)
{
    BitMap screenBits;

    GetQDGlobalsScreenBits(&screenBits);
    tempPortRect = screenBits.bounds;
    return &tempPortRect;
}

/* SetRetainedWindow (mac _SetRetainedWindow, mac-only). STATE: see the mac gate. */
// MAC: _SetRetainedWindow i386 0x000a44ad ppc 0x000affa4
void SetRetainedWindow(WindowRef window, Boolean retained)
{
}

/* GetCPortVisRgn (mac _GetCPortVisRgn, mac-only). STATE: see the mac gate. */
// MAC: _GetCPortVisRgn i386 0x000a44b2 ppc 0x000affa8
RgnHandle GetCPortVisRgn(CGrafPtr port)
{
    return GetPortVisibleRegion(port, carbonTempRgn);
}

/* GetCPortPortRect (mac _GetCPortPortRect, mac-only). STATE: see the mac gate. */
// MAC: _GetCPortPortRect i386 0x000a44d0 ppc 0x000affb8
Rect *GetCPortPortRect(CGrafPtr port)
{
    GetPortBounds(port, &tempPortRect);
    return &tempPortRect;
}

/* GetCWindowGrafPort (mac _GetCWindowGrafPort, mac-only). STATE: see the mac gate. */
// MAC: _GetCWindowGrafPort i386 0x000a44f4 ppc 0x000afff0
CGrafPtr GetCWindowGrafPort(WindowRef window)
{
    return GetWindowPort(window);
}

/* GetCWindowPixMap (mac _GetCWindowPixMap, mac-only). STATE: see the mac gate. */
// MAC: _GetCWindowPixMap i386 0x000a44fd ppc 0x000afff4
PixMapHandle GetCWindowPixMap(WindowRef window)
{
    return GetPortPixMap(GetWindowPort(window));
}
