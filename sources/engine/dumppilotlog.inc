/* body of DumpPilotLog: the head is in loaddata.c and, under MACBUILD, in loadintfres.c */
{
    FILE *log;
    DateTimeRec dtr;
    PStrChar name[255];
    short i;
    short mi;
    short found;
#ifdef MACBUILD
    char *dir;
    char *path;
    unsigned long contribute[2];
#endif

    if ((shipClass[gameIsRegisteredIndex].registered != 0 ? 1 : 0) == 0)
        return;

#if defined(MACBUILD) && V111
    if (novaSupportFolder != NULL) {
        dir = malloc(512);
        FSRefMakePath(novaSupportFolder, (UInt8 *)dir, 512);
        path = malloc(strlen(dir) + 1 + strlen("pilotlog.txt")); /* BUG: no room for the NUL */
        sprintf(path, "%s/%s", dir, "pilotlog.txt");
        free(dir);
    }
    /* BUG: path is never set when there is no support folder */
    log = fopen(path, "r");
    if (log == NULL) {
        if (path != NULL)
            free(path);
        return;
    }
    fclose(log);
    log = fopen(path, "w");
    if (path != NULL)
        free(path);
    if (log == NULL)
        return;
#else
    log = fopen("pilotlog.txt", "r");
    if (log == NULL)
        return;
    fclose(log);
    log = fopen("pilotlog.txt", "w");
    if (log == NULL)
        return;
#endif

    fprintf(log, "EV Nova pilot data dump\n\n");
    GetTime(&dtr);
    fprintf(log, "Output on %i/%i/%i at ", dtr.month, dtr.day, dtr.year);
    if (dtr.hour < 12)
        fprintf(log, "%i:%02i AM", dtr.hour, dtr.minute);
    else if (dtr.hour == 12)
        fprintf(log, "%i:%02i PM", dtr.hour, dtr.minute);
    else
        fprintf(log, "%i:%02i PM", dtr.hour - 12, dtr.minute);
    fprintf(log, "\n\n");
#if V111
    fprintf(log, "Game version:      %s", "1.1.1");
#else
    fprintf(log, "Game version:      %s", "1.0.A");
#endif
    fprintf(log, "\n\n");

    fprintf(log, "Plugins loaded:\n");
#ifdef MAC1010
    if (CountEVPlugins(gARef, "\pNova Plug-Ins", kPlugInFileType, log) == 0)
#elif defined(MACBUILD)
    if (CountEVPlugins(pluginsFolder, kPlugInFileType) == 0)
#else
    if (CountEVPlugins("Nova Plug-Ins", ".rez", log) == 0)
#endif
        fprintf(log, "  none\n");
    fprintf(log, "\n");

    pstrcpy((StringPtr)name, playerName);
    PtoCstr((StringPtr)name);
    fprintf(log, "Pilot name:        %s\n", name);
    pstrcpy((StringPtr)name, playerNickname);
    PtoCstr((StringPtr)name);
    fprintf(log, "Pilot nickname:    %s\n", name);
    fprintf(log, "Pilot gender:      %s\n", playerGender != 0 ? "male" : "female");
    fprintf(log, "Strict play:       %s\n", strictPlay != 0 ? "yes" : "no");
    fprintf(log, "Seen intro screen: %s\n", seenIntroScreen != 0 ? "yes" : "no");

    pstrcpy((StringPtr)name, shipName);
    PtoCstr((StringPtr)name);
    fprintf(log, "Ship name:         %s\n", name);

    pstrcpy((StringPtr)name, shipClass[shipData->type].name);
    PtoCstr((StringPtr)name);
    fprintf(log, "Ship type:         %s (%i)\n", name, shipData->type + 0x80);

    pstrcpy((StringPtr)name, systemData[shipData->curSystem].name);
    PtoCstr((StringPtr)name);
    fprintf(log, "Current system:    %s (%i)\n", name, shipData->curSystem + 0x80);

    fprintf(log, "Legal status:      %i (in system %i)\n",
            playerRecord[shipData->curSystem], shipData->curSystem + 0x80);

    fprintf(log, "Game date:         %i/%i/%i\n",
            gameTime.month, gameTime.day, gameTime.year);

    fprintf(log, "Fuel:              %0.2f\n", shipData->fuel);
    fprintf(log, "Cash:              %i\n", shipData->credits);
    fprintf(log, "Kills:             %i\n", playerKills);

    fprintf(log, "\nCurrent cargo:\n");
    found = 0;
    for (i = 0; i < kFirstJunkItem; i++) {
        if (shipData->cargo[i] > 0) {
            fprintf(log, "  commodity %i: %i\n", i, shipData->cargo[i]);
            found++;
        }
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            fprintf(log, "  junk type %i: %i\n", i + 0x80, junkData[i].held);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive missions:\n");
    found = 0;
    for (mi = 0; mi < kMaxActiveMissions; mi++) {
        if (currentMissionObjectives[mi].active != 0) {
            found++;
            pstrcpy((StringPtr)name, (unsigned char *)currentMissions[mi].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i):\n", name, currentMissions[mi].misnIndex + 0x80);
            fprintf(log, "    travelObjComplete:        %s\n",
                    currentMissionObjectives[mi].returnDone != 0 ? "yes" : "no");
            fprintf(log, "    shipObjComplete:          %s\n",
                    currentMissionObjectives[mi].objectivesDone != 0 ? "yes" : "no");
            fprintf(log, "    missionFailed:            %s\n",
                    currentMissionObjectives[mi].failed != 0 ? "yes" : "no");
            fprintf(log, "    flags:                    %04X\n", currentMissions[mi].flags);
            fprintf(log, "    flags2:                   %04X\n", currentMissions[mi].flags2);
            fprintf(log, "    canAbort:                 %s\n",
                    currentMissions[mi].canAbort != 0 ? "yes" : "no");
            fprintf(log, "    timeLeft:                 %i\n", currentMissions[mi].timeLimit);
            fprintf(log, "    destStellar:              %i\n",
                    currentMissions[mi].travelStellar == -1 ? -1 : currentMissions[mi].travelStellar + 0x80);
            fprintf(log, "    returnStellar:            %i\n",
                    currentMissions[mi].returnStellar == -1 ? -1 : currentMissions[mi].returnStellar + 0x80);
            fprintf(log, "    initialShipCount:         %i\n", currentMissions[mi].initialShipCount);
            fprintf(log, "    specialShipCount:         %i\n", currentMissions[mi].shipCount);
            fprintf(log, "    specialShipDude:          %i\n",
                    currentMissions[mi].shipDude == -1 ? -1 : currentMissions[mi].shipDude + 0x80);
            fprintf(log, "    specialShipGoal:          %i\n", currentMissions[mi].shipGoal);
            fprintf(log, "    specialShipBehavior:      %i\n", currentMissions[mi].shipBehav);
            fprintf(log, "    specialShipStart:         %i\n", currentMissions[mi].shipStart);
            fprintf(log, "    specialShipSyst:          %i\n", currentMissions[mi].shipSyst);
            fprintf(log, "    specialShipsKilled:       %i\n", currentMissions[mi].specialShipsKilled);
            fprintf(log, "    specialShipsBoarded:      %i\n", currentMissions[mi].specialShipsBoarded);
            fprintf(log, "    specialShipsDisabled:     %i\n", currentMissions[mi].specialShipsDisabled);
            fprintf(log, "    specialShipsJumpedIn:     %i\n", currentMissions[mi].specialShipsJumpedIn);
            fprintf(log, "    specialShipsJumpedOut:    %i\n", currentMissions[mi].specialShipsJumpedOut);
            fprintf(log, "    specialShipNameIndex:     %i\n", currentMissions[mi].destStrIndex);
            fprintf(log, "    specialShipSubtitleIndex: %i\n", currentMissions[mi].shipStrIndex);
            fprintf(log, "    specialShipPreselectType: %i\n", currentMissions[mi].shipTypeIndex);
            fprintf(log, "    auxShipCount:             %i\n", currentMissions[mi].auxShipCount);
            fprintf(log, "    auxShipsLeft:             %i\n", currentMissions[mi].auxShipsLeft);
            fprintf(log, "    auxShipDude:              %i\n",
                    currentMissions[mi].auxShipDude == -1 ? -1 : currentMissions[mi].auxShipDude + 0x80);
            fprintf(log, "    auxShipSyst:              %i\n", currentMissions[mi].auxShipSyst);
            fprintf(log, "    cargoType:                %i\n", currentMissions[mi].cargoType);
            fprintf(log, "    cargoQty:                 %i\n", currentMissions[mi].cargoQty);
            fprintf(log, "    pickupMode:               %i\n", currentMissions[mi].pickupMode);
            fprintf(log, "    dropoffMode:              %i\n", currentMissions[mi].dropoffMode);
            fprintf(log, "    cargoLoaded:              %s\n",
                    currentMissions[mi].hasCargo != 0 ? "yes" : "no");
            fprintf(log, "    scanMask:                 %04X\n", currentMissions[mi].scanMask);
            fprintf(log, "    compGovt:                 %i\n",
                    currentMissions[mi].compGovt == -1 ? -1 : currentMissions[mi].compGovt + 0x80);
            fprintf(log, "    compReward:               %i\n", currentMissions[mi].compReward);
            fprintf(log, "    datePostIncrement:        %i\n", currentMissions[mi].datePostIncrement);
            fprintf(log, "    pay:                      %i\n", currentMissions[mi].payment);
            fprintf(log, "\n");
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nMission bits currently set:\n");
    found = 0;
    for (i = 0; i < kMaxMissionBits; i++) {
        if (missionBit[i] != 0) {
            fprintf(log, "  b%i\n", i);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");
#ifdef MACBUILD

    fprintf(log, "\nContribute bits:\n");
    GetPlayerContributeBits(&contribute[0], &contribute[1]);
    fprintf(log, "  0x%08X\n", contribute[0]);
    fprintf(log, "  0x%08X\n", contribute[1]);
#endif

    fprintf(log, "\nEscorts:\n");
    found = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0) {
            found++;
            pstrcpy((StringPtr)name, shipClass[shipData[i].type].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i) - ", name, shipData[i].type + 0x80);
            if (shipData[i].aiType == 5)
                fprintf(log, "fighter");
            else if (shipData[i].specialMission != -1)
                fprintf(log, "mission escort");
            else if (shipData[i].hired != 0)
                fprintf(log, "hired");
            else
                fprintf(log, "captured");
            if (shipData[i].upgradeOnLanding != 0)
                fprintf(log, ", marked for upgrade");
            if (shipData[i].sellOnLanding != 0)
                fprintf(log, ", marked for sale");
            fprintf(log, "\n");
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nItems currently owned:\n");
    found = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            pstrcpy((StringPtr)name, outfitSystem[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %i %s (%i)\n", shipSystems[i], name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nStellars currently dominated:\n");
    found = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].prepared != 0 && stellarData[i].dominated != 0) {
            pstrcpy((StringPtr)name, stellarData[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i)\n", name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nStellars currently destroyed:\n");
    found = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].prepared != 0 && StellarIsDestroyed(&stellarData[i])) {
            pstrcpy((StringPtr)name, stellarData[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i)\n", name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive ranks:\n");
    found = 0;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].loaded != 0 && rankData[i].active != 0) {
            /* the exe passes the PASCAL string straight to %s here, with no
               pstrcpy/PtoCstr conversion (unlike every other name section) -
               read directly off the call site, not a guess */
            fprintf(log, "  %s (%i)\n", rankData[i].name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive disasters:\n");
    found = 0;
    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].stellarID != -1) {
            pstrcpy((StringPtr)name, disasterData[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i) on ", name, i + 0x80);
            pstrcpy((StringPtr)name, stellarData[disasterData[i].stellarID].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "%s (%i) - %i days left\n",
                    name, disasterData[i].stellarID + 0x80, disasterData[i].countdown);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive cron events:\n");
    found = 0;
    for (i = 0; i < kMaxCrons; i++) {
        if (cronEvent[i].loaded != 0 && cronEvent[i].active != 0) {
            fprintf(log, "  %i - durationCounter: %i\tholdoffCounter: %i\n",
                    i + 0x80, cronEvent[i].daysLeft, cronEvent[i].holdoffLeft);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\n\n- end of log -\n");
    fflush(log);
    fclose(log);
}
