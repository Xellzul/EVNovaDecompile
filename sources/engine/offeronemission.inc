/* body of OfferOneMission: the head is in mission.c at both of its positions */
{
#ifdef MAC1010
    Rect box;
    int32_t missionResID;
    short refuseDescID;
    Handle itemHandle;
    Point pt;
    short itemHit;
    short itemType;
    char setString[255];
    short i;
    short shift;
    short widest;
    short otherWidth;
    short savedTarget;
    short savedRoute;
    short shown;
    Boolean savedUp;
    Boolean savedDown;
    Boolean refused;
    Boolean accepted;
    Boolean done;
    unsigned char labelChar;
#define itemHeight savedRoute
#define savedScroll otherWidth
#elif defined(MACBUILD)
    Point pt;
    Rect box;
    int32_t missionResID;
    short refuseDescID;
    Handle itemHandle;
    short itemType;
    short itemHit;
    char setString[255];
    short i;
    short itemHeight;
    short shift;
    short widest;
    short otherWidth;
    short savedTarget;
    short savedRoute;
    short shown;
    short savedScroll;
    Boolean savedUp;
    Boolean savedDown;
    Boolean refused;
    Boolean accepted;
    Boolean done;
    unsigned char labelChar;
#else
    Rect box;
    short refuseDescID;
    Boolean refused;
    Boolean done;
    int32_t missionResID;
    void *itemHandle;
    short itemType;
    short itemHit;
    char setString[256];
    Rect *wr;
    short i;
    short itemHeight;
    short shift;
    short floatZero;
    short widest;
    short otherWidth;
    short savedTarget;
    short savedRoute;
    short shown;
    short savedScroll;
#ifndef V111
    unsigned char labelChar;
#endif
    Boolean accepted;
    Boolean savedUp;
    Boolean savedDown;
#endif

    refused = 0;
    accepted = 0;
    done = 0;
    if (mission < 0 || mission >= kMaxMissions)
        return 0;

    if ((missionAvailData[mission].flags & kMissionCantRefuse) != 0)
        canRefuseMission = 0;
    else
        canRefuseMission = 1;

    crashlog_print("offering mission ");
    crashlog_print_pstr(missionName[mission]);
    crashlog_print(" (");
    missionResID = mission + kFirstResourceID;
    crashlog_print_num(missionResID);
    crashlog_print(")\n");

    strncpy(briefingTextPtr, "", 3);
    LoadDescription(briefingTextPtr, mission + kMissionDescBase);

    if (descMovieName[0] != 0 && settings.noMovies == 0
        && (descFlags & kDescMovieAfter) == 0)
        PlayQTMovie(descMovieName, descFlags & kDescMovieDoubled,
                    descFlags & kDescMovieFade);

    if (screenFaded != 0) {
        UpdateAllPortDialogs();
        RefreshSAT();
        RedrawStatus(1);
        FadeScreenIn();
        LoadDescription(briefingTextPtr, mission + kMissionDescBase);
    }

    if (canRefuseMission == 0 && strlen(briefingTextPtr) == 0) {
        accepted = DoMissionAccept(mission);
        if (accepted != 0)
            return 1;
        return -1;
    }

    strcat(briefingTextPtr, "\r");

    missionDialog = NULL;
    if (descResourcePictId >= 128)
        missionDialog = GetNewDialog(kWideDlogID, SingleMissionDialogUpdate,
                                     SingleMissionFilter);
    else
        missionDialog = GetNewDialog(kNarrowDlogID, SingleMissionDialogUpdate,
                                     SingleMissionFilter);
    if (missionDialog == NULL)
        return 0;

    if (descResourcePictId >= 128) {
        missionBackgroundPic[0] = GetPicture(kWidePictID);
        missionBackgroundPic[1] = NULL;
        missionBackgroundPic[2] = NULL;
    } else {
        for (i = 0; i < kMissionPictCount; i++)
            missionBackgroundPic[i] = GetPicture(kNarrowPictBase + i);
    }

    CenterWindow(FrontWindow(missionDialog), gWind);
    MungeBriefing(1, mission);
    SetDialogGWorld(missionDialog);

    GetDialogItem(missionDialog, kItemBriefText, &itemType, (Handle *)&itemHandle, &box);
    offerTextHandle = TENew(&box, &box);
    if (offerTextHandle != NULL) {
        (*offerTextHandle)->txFont = smallTextFont;
        (*offerTextHandle)->txSize = smallTextSize;
        TextFont((*offerTextHandle)->txFont);
        TextSize((*offerTextHandle)->txSize);
        TESetLineMetrics(offerTextHandle);
        TESetText(briefingTextPtr, strlen(briefingTextPtr), offerTextHandle);

        shown = TETextHeight(offerTextHandle);
        itemHeight = box.bottom - box.top;
        if (shown < itemHeight) {
            if (descResourcePictId < 128) {
#ifdef MACBUILD
                if (shown >= kMinTextHeight)
                    shift = shown;
                else
                    shift = kMinTextHeight;
                box = *GetDialogPortRect(missionDialog);
                shift = itemHeight - shift - kTextMargin;
                box.bottom -= shift;
                SizeWindow(FrontWindow(missionDialog), box.right - box.left,
                           box.bottom - box.top, true);
                pt.h = pt.v = 0;
                LocalToGlobal(&pt);
                pt.v += shift * 0.3;
                MoveWindow(FrontWindow(missionDialog), pt.h, pt.v, true);
#else
                shown = shown < kMinTextHeight
                             ? (short)kMinTextHeight : shown;
                wr = GetDialogPortRect(missionDialog);
                box.top = wr->top;
                box.left = wr->left;
                box.bottom = wr->bottom;
                box.right = wr->right;
                shift = itemHeight - shown - kTextMargin;
                box.bottom -= shift;
                SizeDialogWindow(missionDialog, box.right - box.left,
                             box.bottom - box.top, 1);
                floatZero = 0;
                floatZero += shift * 0.3;
                OffsetDialogWindow(missionDialog, 0, floatZero, 1);
#endif

                GetDialogItem(missionDialog, kItemAccept, &itemType,
                              (Handle *)&itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemAccept, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemRefuse, &itemType,
                              (Handle *)&itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemRefuse, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemDivider, &itemType,
                              (Handle *)&itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemDivider, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemBriefText, &itemType,
                              (Handle *)&itemHandle, &box);
                box.bottom -= shift;
                SetDialogItem(missionDialog, kItemBriefText, itemType,
                              itemHandle, &box);
            }
            GetDialogItem(missionDialog, kItemBriefText, &itemType,
                          (Handle *)&itemHandle, &(*offerTextHandle)->destRect);
            GetDialogItem(missionDialog, kItemBriefText, &itemType,
                          (Handle *)&itemHandle, &(*offerTextHandle)->viewRect);
            TECalText(offerTextHandle);
        }
    }

    strcpy((char *)missionButtonLabel[0], "");
    strcpy((char *)missionButtonLabel[1], "");

    missionFileHand = NULL;
    missionFileHand = GetResource(kMisnType, mission + kFirstResourceID);
    if (missionFileHand != NULL) {
        strcpy((char *)missionButtonLabel[0], ((MisnRes *)*missionFileHand)->acceptButton);
        strcpy((char *)missionButtonLabel[1], ((MisnRes *)*missionFileHand)->refuseButton);
        for (i = 0; i <= kMissionButtonLabelCount - 1; i++) {
#ifdef V111 /* V111 spelling */
            if (tolower(missionButtonLabel[i][0]) < 'a'
                    || tolower(missionButtonLabel[i][0]) > 'z')
#else
            labelChar = missionButtonLabel[i][0];
            if (tolower(labelChar) < 'a' || tolower(labelChar) > 'z')
#endif
                strcpy((char *)missionButtonLabel[i], "");
        }
        refuseDescID = ((MisnRes *)*missionFileHand)->refuseText;
        strcpy(setString, ((MisnRes *)*missionFileHand)->onRefuse);
        ReleaseResource(missionFileHand);
    } else {
        refuseDescID = -1;
        strcpy(setString, "");
    }

    if (strlen((char *)missionButtonLabel[0]) == 0) {
        if (canRefuseMission != 0)
            GetIndString(missionButtonLabel[0], kButtonStrList,
                         kStrAcceptRefusable);
        else
            GetIndString(missionButtonLabel[0], kButtonStrList,
                         kStrAcceptOnly);
    } else {
        CtoPstr((char *)missionButtonLabel[0]);
    }
    if (strlen((char *)missionButtonLabel[1]) == 0)
        GetIndString(missionButtonLabel[1], kButtonStrList, kStrRefuse);
    else
        CtoPstr((char *)missionButtonLabel[1]);

    SetDialogGWorld(missionDialog);
    TextFont(0);
    TextSize(12);
    widest = StringWidth(missionButtonLabel[0]);
    otherWidth = StringWidth(missionButtonLabel[1]);
    if (widest > otherWidth)
        otherWidth = widest;
    otherWidth += kButtonPadding;
    if (otherWidth < kMinButtonWidth)
        otherWidth = kMinButtonWidth;

    GetDialogItem(missionDialog, kItemAccept, &itemType, (Handle *)&itemHandle, &box);
    box.right = box.left + otherWidth;
    SetDialogItem(missionDialog, kItemAccept, itemType, itemHandle, &box);

    GetDialogItem(missionDialog, kItemRefuse, &itemType, (Handle *)&itemHandle, &box);
    box.left = box.right - otherWidth;
    SetDialogItem(missionDialog, kItemRefuse, itemType, itemHandle, &box);

    GetDialogItem(missionDialog, kItemDivider, &itemType, (Handle *)&itemHandle, &box);
    box.left = box.right = (box.left + box.right) / 2;
    InsetRect(&box, -otherWidth / 2, 0);
    SetDialogItem(missionDialog, kItemDivider, itemType, itemHandle, &box);

    lastTextScrollTime = ReadTick();
    canScrollTextUp = 0;
    canScrollTextDown = 1;
    savedScroll = (*offerTextHandle)->destRect.bottom;
    TEPinScroll(0, -1, offerTextHandle);
    if ((*offerTextHandle)->destRect.bottom == savedScroll)
        canScrollTextDown = 0;
    TEPinScroll(0, 1, offerTextHandle);
    savedUp = canScrollTextUp;
    savedDown = canScrollTextDown;

    ShowWindowNoFlash(FrontWindow(missionDialog));
    SetDialogGWorld(missionDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    RedrawDialog(missionDialog);
    SingleMissionDialogUpdate();
    SelectWindow(FrontWindow(missionDialog));
    BringToFront(FrontWindow(missionDialog));

    do {
        if (gQuit != 0)
            done = 1;
        MyModalDialog(missionDialog, SingleMissionFilter, &itemHit, 0);
        if (canRefuseMission != 0) {
            if (itemHit == kItemAccept) {
                FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
                FlushEvents(5, 0);
                FlushEvents(1, 0);
                FlushEvents(2, 0);
                SetDialogVisible(missionDialog, 0);
#ifdef MACBUILD
                RefreshSAT();
#endif
                accepted = DoMissionAccept(mission);
                SingleMissionDialogUpdate();
                LoadDescription(briefingTextPtr, mission + kMissionDescBase);
                MungeBriefing(1, mission);
#ifdef MACBUILD
                RefreshSAT();
#endif
                SetDialogGWorld(missionDialog);
                done = 1;
            }
            if (itemHit == kItemRefuse) {
                if (refuseDescID != -1) {
                    SetDialogVisible(missionDialog, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, refuseDescID);
                    MungeBriefing(0, -1);
                    strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
                    ShowTextDialog(0, 0, 1);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                }
                crashlog_mission_bit_set("refusing mission", NULL, 0,
                                         mission + kFirstResourceID, setString);
                EvalMissionBitSetString(setString);
                refused = 1;
                done = 1;
            }
        } else if (itemHit == kItemAccept || itemHit == kItemRefuse) {
            FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            accepted = DoMissionAccept(mission);
            LoadDescription(briefingTextPtr, mission + kMissionDescBase);
            MungeBriefing(1, mission);
            SingleMissionDialogUpdate();
            done = 1;
        }
        if (itemHit == kBriefItemMap) {
            FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 1;
            if ((missionAvailData[mission].flags & kMissionBriefingArrow) != 0) {
                if (briefMissionInfo[mission].destStellar != -1) {
                    greenMissionSyst =
                        stellarData[briefMissionInfo[mission].destStellar].system;
                } else {
                    if (briefMissionInfo[mission].returnStellar != -1)
                        greenMissionSyst =
                            stellarData[briefMissionInfo[mission].returnStellar].system;
                }
            }
            savedTarget = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedTarget;
            shipData->navTargetKind = savedRoute;
            greenMissionSyst = -1;
            if (offeringFromShip == 0 && playerIsLanded != 0) {
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                PortDialogUpdate();
            }
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
        if (itemHit == kBriefItemPlayerInfo) {
            FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            DoPlayerInfoDialog();
            if (offeringFromShip == 0 && playerIsLanded != 0) {
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                PortDialogUpdate();
            }
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
        if (itemHit == kBriefItemMissionInfo) {
            widest = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0) {
                    if ((currentMissionObjectives[i].flags & kMissionHidden) == 0)
                        widest++;
                }
            }
            if (widest > 0) {
                FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
                FlushEvents(5, 0);
                FlushEvents(1, 0);
                FlushEvents(2, 0);
                DoMissionInfoDialog();
                LoadDescription(briefingTextPtr, mission + kMissionDescBase);
                MungeBriefing(1, mission);
                if (offeringFromShip == 0 && playerIsLanded != 0) {
                    SetDialogGWorld(portDialog);
                    RedrawDialog(portDialog);
                    PortDialogUpdate();
                }
                SetDialogGWorld(missionDialog);
                RedrawDialog(missionDialog);
            }
        }
        if (itemHit == kItemScrollUp) {
            if (canScrollTextUp != 0)
                ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle,
                                 kBriefScrollStep, 1);
        }
        if (itemHit == kItemScrollDown) {
            if (canScrollTextDown != 0)
                ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle,
                                 -kBriefScrollStep, 1);
        }
        if (canScrollTextUp != savedUp) {
#ifdef MACBUILD
            GetDialogItem(missionDialog, kItemScrollUp, &itemType, &itemHandle, &box);
            InvalWindowRect(FrontWindow(missionDialog), &box);
#else
            InvalDialogItem(missionDialog, kItemScrollUp);
#endif
            savedUp = canScrollTextUp;
        }
        if (canScrollTextDown != savedDown) {
#ifdef MACBUILD
            GetDialogItem(missionDialog, kItemScrollDown, &itemType, &itemHandle, &box);
            InvalWindowRect(FrontWindow(missionDialog), &box);
#else
            InvalDialogItem(missionDialog, kItemScrollDown);
#endif
            savedDown = canScrollTextDown;
        }
    } while (done == 0);

    if (descMovieName[0] != 0 && settings.noMovies == 0
        && (descFlags & kDescMovieAfter) != 0)
        PlayQTMovie(descMovieName, descFlags & kDescMovieDoubled,
                    descFlags & kDescMovieFade);

    for (i = 0; i < kMissionPictCount; i++) {
        if (missionBackgroundPic[i] != NULL) {
            QDReleaseResource(missionBackgroundPic[i]);
            missionBackgroundPic[i] = NULL;
        }
    }
    if (offerTextHandle != NULL)
        TEDispose(offerTextHandle);
    while (KeyCheck(keyPrefs.missions))
        BusyIdle();

    DisposeDialogNoFlash(missionDialog);
    missionDialog = NULL;
    if (screenFaded != 0) {
        if (playerIsLanded != 0)
            UpdateAllPortDialogs();
#ifdef MACBUILD
        RefreshSAT();
#endif
        RedrawStatus(1);
        FadeScreenIn();
    }
#ifdef MACBUILD
    RefreshSAT();
#endif
    FlushEvents(7, 0);
    if (refused != 0)
        return 0;
    if (accepted != 0)
        return 1;
    return -1;
}
#ifdef MAC1010
#undef itemHeight
#undef savedScroll
#endif
