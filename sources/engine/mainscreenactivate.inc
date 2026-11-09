/* body of MainScreenActivate: the head is in mainscreen.c and, under MAC1010, in maininfo.c */
{
#ifndef MAC1010 /* 1.0.10 real: 0x1005b19c the body begins on `activate`, no gWind test */
    if (gWind == NULL)
        return;
#endif
    inBackground = !activate;
    if (activate) {
        InitCursor();
        cursorHidden = 0;
        ShowTheCursor();
        SWSetPortToWindow(gSpriteWorld);
        if (!settings.windowed) {
            HideTheMenuBar();
            mBarHidden = 1;
            ShowWindow(gWind);
        }
        SelectWindow(gWind);
        SWSetPortToWindow(gSpriteWorld);
        InvalWindowPortRect(gWind);
        MainScreenAnimDraw();
        if (useLegacyRenderingPipeline)
            MainScreenUpdate(1);
        if (!regNoticeShown) {
            regNoticeShown = 1;
            if (!RT3_IsRegistered()) {
#if V111
                ASWReg_ShowDialogWithOptions(true, true);
#elif defined(MAC1010)
                RT3_RunReminder(0, 0);
#endif
                if (useLegacyRenderingPipeline)
                    MainScreenUpdate(1);
            }
        }
    } else if (!settings.windowed) {
        ShowTheMenuBar();
        HideWindow(gWind);
    }
}
