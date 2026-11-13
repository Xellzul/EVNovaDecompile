/* loadobjectdata1010.inc -- LoadObjectData's body on mac 1.0.10; win and mac 1.1.1 keep theirs in loaddata.c. Notes: Research/notes/loaddata.md */
{
    short i;
    short j;
    short n;
    short m;
    short hits;
    short found;
    short resID;
    uint32_t resType;
    short count;
    short k;
    short gunTally, turretTally;
    float base;
    int32_t sum;
    short mountID[8];
    short mountCount[8];
    RGBColor rgb;
    short mountAmmo[8];
    unsigned char name[256];
    Boolean used[kMaxStellars];

    if (EVBugResourceFieldTest(0))
        return;
    SWSetPortToWindow(gSpriteWorld);

    /* ---------------- spöb: stellar objects ---------------- */
    {
        found = 0;
        count = CountResources(kSpobType);
    if (count == 0)
        crashlog("warning: no " kSpobResName " resources found");
    for (i = 0; i < kMaxStellars; i++) {
        stellarData[i].loaded = 0;
        stellarFileHand = GetResource(kSpobType, i + kFirstResourceID);
        if (stellarFileHand != NULL) {
            LockAHandle(stellarFileHand);
            ResourceNoPurge(stellarFileHand);
            found++;
            GetResInfo(stellarFileHand, &resID, &resType, name);
            TrimResourceName(name);
            strncpy((char *)stellarData[i].name, (char *)name, 31);
            stellarData[i].pos.h = ResRec(SpobRes, stellarFileHand)->xPos;
            stellarData[i].pos.v = ResRec(SpobRes, stellarFileHand)->yPos;
            stellarData[i].type = ResRec(SpobRes, stellarFileHand)->type;
            stellarData[i].flags = ResRec(SpobRes, stellarFileHand)->flags;
            stellarData[i].flags2 = ResRec(SpobRes, stellarFileHand)->flags2;
            stellarData[i].govt = ResRec(SpobRes, stellarFileHand)->govt;
            stellarData[i].minStatus = ResRec(SpobRes, stellarFileHand)->minStatus;
            stellarData[i].techLevel = ResRec(SpobRes, stellarFileHand)->techLevel;
            stellarData[i].customPict = ResRec(SpobRes, stellarFileHand)->custPicID;
            stellarData[i].customSound = ResRec(SpobRes, stellarFileHand)->custSndID;
            stellarData[i].landingFee = ResRec(SpobRes, stellarFileHand)->fee;
            stellarData[i].gravity = ResRec(SpobRes, stellarFileHand)->gravity;
            stellarData[i].weapon = ResRec(SpobRes, stellarFileHand)->weapon;
            if (stellarData[i].govt >= kFirstResourceID)
                stellarData[i].govt -= kFirstResourceID;
            else
                stellarData[i].govt = -1;
            stellarData[i].strength = ResRec(SpobRes, stellarFileHand)->strength;
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadType = ResRec(SpobRes, stellarFileHand)->deadType;
            stellarData[i].deadDays = ResRec(SpobRes, stellarFileHand)->deadTime;
            stellarData[i].deadDaysLeft = 0;
            stellarData[i].explodType = ResRec(SpobRes, stellarFileHand)->explodType;
            if (stellarData[i].explodType < 0)
                stellarData[i].explodType = -1;
            if (stellarData[i].deadType < 0 || stellarData[i].deadType >= kMasterStellarSprites)
                stellarData[i].deadType = stellarData[i].type;
            strcpy(stellarData[i].onDestroy, ResRec(SpobRes, stellarFileHand)->onDestroy);
            strcpy(stellarData[i].onRegen, ResRec(SpobRes, stellarFileHand)->onRegen);
            for (j = 0; j < 3; j++)
                stellarData[i].specialTech[j] = ResRec(SpobRes, stellarFileHand)->specialTech[j];
            for (j = 0; j < 5; j++)
                stellarData[i].specialTech[j + 3] = ResRec(SpobRes, stellarFileHand)->specialTech2[j];
            stellarData[i].loaded = 1;
            stellarData[i].prepared = 1;
            stellarData[i].dominated = 0;
            stellarData[i].tribute = ResRec(SpobRes, stellarFileHand)->tribute;
            if (stellarData[i].tribute <= 0)
                stellarData[i].tribute = stellarData[i].techLevel * 1000;
            stellarData[i].defCount = ResRec(SpobRes, stellarFileHand)->defCount;
            stellarData[i].defenseDude = ResRec(SpobRes, stellarFileHand)->defenseDude;
            stellarData[i].animDelay = ResRec(SpobRes, stellarFileHand)->animDelay;
            stellarData[i].frame0Bias = ResRec(SpobRes, stellarFileHand)->frame0Bias;
            for (j = 0; j < kSpobHyperLinks; j++) {
                stellarData[i].hyperLinks[j] = ResRec(SpobRes, stellarFileHand)->hyperLink[j];
                if (stellarData[i].hyperLinks[j] >= kFirstResourceID)
                    stellarData[i].hyperLinks[j] -= kFirstResourceID;
                else
                    stellarData[i].hyperLinks[j] = -1;
            }
            if (stellarData[i].defCount > 1000) {
                if (stellarData[i].defCount > 10000)
                    stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 1000;
                else
                    stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 100;
            } else
                stellarData[i].defWaveSize = stellarData[i].defCount;
            if (stellarData[i].defenseDude >= kFirstResourceID)
                stellarData[i].defenseDude -= kFirstResourceID;
            else
                stellarData[i].defenseDude = -1;
            if (stellarData[i].techLevel < 0)
                stellarData[i].techLevel = 0;
            strcpy(stellarData[i].onDominate, ResRec(SpobRes, stellarFileHand)->onDominate);
            strcpy(stellarData[i].onRelease, ResRec(SpobRes, stellarFileHand)->onRelease);
            if (stellarData[i].weapon >= kFirstResourceID)
                stellarData[i].weapon -= kFirstResourceID;
            else
                stellarData[i].weapon = -1;
            AdjustHandleSize(stellarFileHand, 0x45e);
            UnlockAHandle(stellarFileHand);
            ResourcePurge(stellarFileHand);
            ReleaseResource(stellarFileHand);
            if (SpobCanLand(&stellarData[i]) && !SpobIsHyperOrWorm(&stellarData[i]) &&
                crashlog_isactive()) {
                if (stellarData[i].customPict >= kFirstResourceID) {
                    if (!ResourceExists(kPictType, stellarData[i].customPict)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(stellarData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") custom landing picture not found\n");
                    }
                } else {
                    if (!ResourceExists(kPictType, stellarData[i].type + 10000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(stellarData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks landing picture for stellar type\n");
                    }
                }
                if (!ResourceExists(kDescType, i + kFirstResourceID)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") lacks landing description\n");
                }
                if (SpobHasBar(&stellarData[i]) &&
                    !ResourceExists(kDescType, i + 10000)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a bar but no bar description\n");
                }
            }
            LoadingIntroMusic();
        } else {
            stellarData[i].loaded = 0;
            stellarData[i].prepared = 0;
        }
        if (found >= count)
            break;
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
    }

    /* ---------------- sÿst: systems ---------------- */
    {
        short link, target;

        found = 0;
        count = CountResources(kSystType);
        if (count == 0)
            crashlog("warning: no " kSystResName " resources found");
        for (i = 0; i < kMaxSystems; i++) {
            systemData[i].loaded = 0;
            systemData[i].prepared = 0;
            systemData[i].govt = -32767;
            for (j = 0; j < kMaxSystemLinks; j++)
                systemData[i].links[j] = -1;
            systemFileHand = GetResource(kSystType, i + kFirstResourceID);
            if (systemFileHand != NULL) {
                LockAHandle(systemFileHand);
                ResourceNoPurge(systemFileHand);
                found++;
                GetResInfo(systemFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)systemData[i].name, (char *)name, 31);
                systemData[i].loaded = 1;
                systemData[i].prepared = 1;
                for (j = 0; j < 8; j++) {
                    systemData[i].dudeTypes[j] = ResRec(SystRes, systemFileHand)->dudeTypes[j];
                    systemData[i].dudeProbs[j] = ResRec(SystRes, systemFileHand)->probs[j];
                }
                systemData[i].avgShips = ResRec(SystRes, systemFileHand)->avgShips;
                systemData[i].govt = ResRec(SystRes, systemFileHand)->govt;
                systemData[i].message = ResRec(SystRes, systemFileHand)->message;
                systemData[i].asteroidCount = ResRec(SystRes, systemFileHand)->asteroids;
                systemData[i].interference = ResRec(SystRes, systemFileHand)->interference;
                if (systemData[i].asteroidCount > 0) {
                    systemData[i].asteroidCount *=
                        (GetCWindowPortRect((WindowRef)gWind)->right -
                                 GetCWindowPortRect((WindowRef)gWind)->left) / 640.0;
                    if (systemData[i].asteroidCount > kMaxAsteroids)
                        systemData[i].asteroidCount = kMaxAsteroids;
                } else {
                    systemData[i].asteroidCount = 0;
                }
                systemData[i].astTypes = ResRec(SystRes, systemFileHand)->astTypes;
                systemData[i].pos.h = ResRec(SystRes, systemFileHand)->xPos;
                systemData[i].pos.v = ResRec(SystRes, systemFileHand)->yPos;
                if (systemData[i].govt >= kFirstResourceID && systemData[i].govt < kFirstResourceID + kMaxGovts)
                    systemData[i].govt -= kFirstResourceID;
                else
                    systemData[i].govt = -1;
                systemData[i].rgb.red = ((ResRec(SystRes, systemFileHand)->bkgndColor >> 16) & 0xff) * 256;
                systemData[i].rgb.green = ((ResRec(SystRes, systemFileHand)->bkgndColor >> 8) & 0xff) * 256;
                systemData[i].rgb.blue = (ResRec(SystRes, systemFileHand)->bkgndColor & 0xff) * 256;
                if (gSpriteWorld->pixelDepth == 8) {
                    systemData[i].mapPixel = Color2Index(&systemData[i].rgb);
                } else {
                    systemData[i].rgb.red &= 0xfc00;
                    systemData[i].rgb.green &= 0xfc00;
                    systemData[i].rgb.blue &= 0xfc00;
                    systemData[i].mapPixel = ((((systemData[i].rgb.red + 0x801) >> 11) << 10) & 0x7c00) |
                                    ((((systemData[i].rgb.green + 0x801) >> 11) << 5) & 0x3e0) |
                                    (((systemData[i].rgb.blue + 0x801) >> 11) & 0x1f);
                }
                systemData[i].murk = ResRec(SystRes, systemFileHand)->murk;
                if (systemData[i].murk > 100)
                    systemData[i].murk = 100;
                else if (systemData[i].murk < 8 && systemData[i].murk > 0)
                    systemData[i].murk = 0;
                else if (systemData[i].murk < 0)
                    systemData[i].murk = -1;
                k = 0;                       /* valid dude prob total */
                systemData[i].fleetCount = 0;
                systemData[i].fleetProbTotal = 0;
                for (j = 0; j < 8; j++) {
                    if (systemData[i].dudeTypes[j] >= kFirstResourceID && systemData[i].dudeTypes[j] < kFirstResourceID + kMaxDudes) {
                        systemData[i].dudeTypes[j] -= kFirstResourceID;
                        k += systemData[i].dudeProbs[j];
                    } else {
                        if (systemData[i].dudeTypes[j] <= -kFirstResourceID && systemData[i].dudeTypes[j] > -(kFirstResourceID + kMaxFleets)) {
                            /* negative systemData[i].dudeTypes[j] entries reference fleets */
                            systemData[i].fleets[systemData[i].fleetCount] = abs(systemData[i].dudeTypes[j]) - kFirstResourceID;
                            systemData[i].fleetProbs[systemData[i].fleetCount] = systemData[i].dudeProbs[j];
                            systemData[i].fleetCount++;
                            systemData[i].fleetProbTotal += systemData[i].dudeProbs[j];
                        }
                        systemData[i].dudeTypes[j] = -1;
                        systemData[i].dudeProbs[j] = 0;
                    }
                }
                if (k != 100 && k > 0) {
                    for (j = 0; j < 8; j++)
                        systemData[i].dudeProbs[j] *= 100.0 / k;
                }
                for (j = 0; j < kMaxSystemLinks; j++) {
                    if (ResRec(SystRes, systemFileHand)->con[j] >= kFirstResourceID && ResRec(SystRes, systemFileHand)->con[j] < kFirstResourceID + kMaxSystems)
                        systemData[i].links[j] = ResRec(SystRes, systemFileHand)->con[j] - kFirstResourceID;
                    else
                        systemData[i].links[j] = -1;
                }
                for (j = 0; j < kNavSlots; j++) {
                    target = ResRec(SystRes, systemFileHand)->nav[j];
                    if (target >= kFirstResourceID && target < kFirstResourceID + kMaxStellars) {
                        systemData[i].navDefault[j] = target - kFirstResourceID;
                        if (!stellarData[systemData[i].navDefault[j]].prepared) {
                            crashlog_print("warning: ignoring reference to nonexistent stellar ID ");
                            crashlog_print_num(systemData[i].navDefault[j] + kFirstResourceID);
                            crashlog_print(" in system ");
                            crashlog_print_pstr(systemData[i].name);
                            crashlog_print(" (");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(")\n");
                            systemData[i].navDefault[j] = -1;
                        }
                    } else {
                        systemData[i].navDefault[j] = -1;
                    }
                }
                for (j = 0; j < 8; j++) {
                    systemData[i].persons[j] = ResRec(SystRes, systemFileHand)->person[j];
                    systemData[i].persProbs[j] = ResRec(SystRes, systemFileHand)->personProb[j];
                    if (systemData[i].persons[j] >= kFirstResourceID && systemData[i].persons[j] < kFirstResourceID + kMaxPersons - 1)
                        systemData[i].persons[j] -= kFirstResourceID;
                    else
                        systemData[i].persons[j] = -1;
                    if (systemData[i].persProbs[j] < 0)
                        systemData[i].persProbs[j] = 0;
                    if (systemData[i].persProbs[j] > 100)
                        systemData[i].persProbs[j] = 100;
                }
                systemData[i].reinforceFleet = ResRec(SystRes, systemFileHand)->reinfFleet;
                systemData[i].reinforceTime = ResRec(SystRes, systemFileHand)->reinfTime;
                systemData[i].reinforceInterval = ResRec(SystRes, systemFileHand)->reinfInterval;
                if (systemData[i].reinforceFleet >= kFirstResourceID) {
                    systemData[i].reinforceFleet -= kFirstResourceID;
                    if (systemData[i].reinforceInterval < 1)
                        systemData[i].reinforceInterval = 1;
                } else {
                    systemData[i].reinforceFleet = -1;
                }
                systemData[i].reinforceTimer = 0;
                systemData[i].reinforceDaysLeft = 0;
                strcpy(systemData[i].visibility, ResRec(SystRes, systemFileHand)->visibility);
                AdjustHandleSize(systemFileHand, 0x1ac);
                UnlockAHandle(systemFileHand);
                ResourcePurge(systemFileHand);
                ReleaseResource(systemFileHand);
                LoadingIntroMusic();
            } else {
                strcpy(systemData[i].visibility, "");
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* co-location chains: systems sharing a map position form linked groups */
        for (i = 0; i < kMaxSystems; i++) {
            systemData[i].colocGroup = -1;
            systemData[i].colocNext = -1;
        }
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded && systemData[i].colocGroup == -1) {
                n = i;
                systemData[i].colocGroup = i;
                for (j = 0; j < kMaxSystems; j++) {
                    if (systemData[j].loaded &&
                        systemData[j].pos.h == systemData[i].pos.h &&
                        systemData[j].pos.v == systemData[i].pos.v &&
                        (systemData[j].colocGroup == -1 || j > i)) {
                        systemData[n].colocNext = j;
                        systemData[j].colocGroup = i;
                        n = j;
                    }
                }
            }
        }

        /* drop links to nonexistent systems */
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].prepared) {
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = systemData[i].links[j];
                    if (systemData[i].links[j] != -1 && !systemData[systemData[i].links[j]].prepared) {
                        systemData[i].links[j] = -1;
                        crashlog_print("warning: ignoring link from ");
                        crashlog_print_pstr(systemData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") to nonexistent system ");
                        crashlog_print_num(link + kFirstResourceID);
                        crashlog_print("\n");
                    }
                }
            }
        }

        /* canonicalize links to group leaders, drop self- and duplicate links */
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].prepared) {
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = systemData[i].links[j];
                    if (link != -1) {
                        systemData[i].links[j] = FindFirstCoLocatedSystem(link);
                        if (systemData[i].links[j] == FindFirstCoLocatedSystem(i))
                            systemData[i].links[j] = -1;
                    }
                }
                for (j = 0; j < kMaxSystemLinks; j++) {
                    if (systemData[i].links[j] != -1) {
                        k = 0;
                        for (n = 0; n <= j; n++)
                            if (systemData[i].links[n] == systemData[i].links[j])
                                k++;
                        if (k > 1)
                            systemData[i].links[j] = -1;
                    }
                }
            }
        }

        /* assume implicit reciprocal links */
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].prepared) {
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = systemData[i].links[j];
                    if (link != -1) {
                        link = FindFirstCoLocatedSystem(link);
                        if (link >= 0 && link < kMaxSystems && link != i) {
                            Boolean haveBack = 0;
                            for (n = 0; n < kMaxSystemLinks; n++) {
                                if (systemData[link].links[n] != -1 &&
                                    FindFirstCoLocatedSystem(systemData[link].links[n]) ==
                                        FindFirstCoLocatedSystem(i)) {
                                    haveBack = 1;
                                    break;
                                }
                            }
                            if (!haveBack) {
                                crashlog_print("warning: assuming implicit reciprocal link from ");
                                crashlog_print_pstr(systemData[i].name);
                                crashlog_print(" (");
                                crashlog_print_num(i + kFirstResourceID);
                                crashlog_print(") to ");
                                crashlog_print_pstr(systemData[link].name);
                                crashlog_print(" (");
                                crashlog_print_num(link + kFirstResourceID);
                                crashlog_print(")\n");
                                for (n = 0; n < kMaxSystemLinks; n++) {
                                    if (systemData[link].links[n] == -1) {
                                        systemData[link].links[n] = FindFirstCoLocatedSystem(i);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /* orphan stellars: every prepared spob must appear in some navDefault */
        if (crashlog_isactive()) {
            for (i = 0; i < kMaxStellars; i++)
                used[i] = 0;
            for (i = 0; i < kMaxSystems; i++) {
                for (j = 0; j < kNavSlots; j++) {
                    if (systemData[i].navDefault[j] >= 0 && systemData[i].navDefault[j] < kMaxStellars)
                        used[systemData[i].navDefault[j]] = 1;
                }
            }
            for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared && !used[i]) {
                    stellarData[i].loaded = 0;
                    stellarData[i].prepared = 0;
                    crashlog_print("warning: stellar ");
                    if ((char)stellarData[i].name[0] == 0)
                        crashlog_print("\"\"");
                    else
                        crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") does not exist in any system\n");
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- oütf: outfit items ---------------- */
    {
        found = 0;
        count = CountResources(kOutfType);
        for (i = 0; i < kMaxOutfits; i++) {
            outfitSystemFileHand = GetResource(kOutfType, i + kFirstResourceID);
            if (outfitSystemFileHand != NULL) {
                found++;
                LockAHandle(outfitSystemFileHand);
                ResourceNoPurge(outfitSystemFileHand);
                outfitSystem[i].techLevel = ResRec(OutfRes, outfitSystemFileHand)->techLevel;
                outfitSystem[i].cost = ResRec(OutfRes, outfitSystemFileHand)->cost;
                outfitSystem[i].mass = ResRec(OutfRes, outfitSystemFileHand)->mass;
                outfitSystem[i].modType[0] = ResRec(OutfRes, outfitSystemFileHand)->modType;
                outfitSystem[i].modVal[0] = ResRec(OutfRes, outfitSystemFileHand)->modVal;
                outfitSystem[i].maxCount = ResRec(OutfRes, outfitSystemFileHand)->max;
                outfitSystem[i].itemClass = ResRec(OutfRes, outfitSystemFileHand)->itemClass;
                outfitSystem[i].dispWeight = ResRec(OutfRes, outfitSystemFileHand)->dispWeight;
                outfitSystem[i].scanMask = ResRec(OutfRes, outfitSystemFileHand)->scanMask;
                outfitSystem[i].buyRandom = ResRec(OutfRes, outfitSystemFileHand)->buyRandom;
                if (outfitSystem[i].buyRandom < 0)
                    outfitSystem[i].buyRandom = 0;
                if (outfitSystem[i].buyRandom > 100)
                    outfitSystem[i].buyRandom = 100;
                outfitSystem[i].govtRestrict = ResRec(OutfRes, outfitSystemFileHand)->require3;
                if ((outfitSystem[i].govtRestrict < ResClassBase(0)
                        || outfitSystem[i].govtRestrict >= ResClassBase(0) + kMaxGovts) &&
                    (outfitSystem[i].govtRestrict < ResClassBase(1)
                        || outfitSystem[i].govtRestrict >= ResClassBase(1) + kMaxGovts) &&
                    (outfitSystem[i].govtRestrict < ResClassBase(2)
                        || outfitSystem[i].govtRestrict >= ResClassBase(2) + kMaxGovts) &&
                    (outfitSystem[i].govtRestrict < ResClassBase(3)
                        || outfitSystem[i].govtRestrict >= ResClassBase(3) + kMaxGovts))
                    outfitSystem[i].govtRestrict = -1;
                strcpy(outfitSystem[i].availability, ResRec(OutfRes, outfitSystemFileHand)->availability);
                strcpy(outfitSystem[i].onPurchase, ResRec(OutfRes, outfitSystemFileHand)->onPurchase);
                strcpy(outfitSystem[i].onSell, ResRec(OutfRes, outfitSystemFileHand)->onSell);
                AdjustHandleSize(outfitSystemFileHand, 0x404);
                outfitSystem[i].persistent = 0;
                outfitSystem[i].flags = ResRec(OutfRes, outfitSystemFileHand)->flags;
                if (OutfPersistent(&outfitSystem[i]))
                    outfitSystem[i].persistent = 1;
                GetResInfo(outfitSystemFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)outfitSystem[i].name, (char *)name, 63);
                outfitSystem[i].modType[1] = ResRec(OutfRes, outfitSystemFileHand)->modType2;
                outfitSystem[i].modVal[1] = ResRec(OutfRes, outfitSystemFileHand)->modVal2;
                outfitSystem[i].modType[2] = ResRec(OutfRes, outfitSystemFileHand)->modType3;
                outfitSystem[i].modVal[2] = ResRec(OutfRes, outfitSystemFileHand)->modVal3;
                outfitSystem[i].modType[3] = ResRec(OutfRes, outfitSystemFileHand)->modType4;
                outfitSystem[i].modVal[3] = ResRec(OutfRes, outfitSystemFileHand)->modVal4;
                for (j = 0; j < kOutfMods; j++) {
                    if ((outfitSystem[i].modType[j] == kOutfModWeapon || outfitSystem[i].modType[j] == kOutfModAmmo ||
                         outfitSystem[i].modType[j] == kModTypeFakeID) && outfitSystem[i].modVal[j] >= kFirstResourceID)
                        outfitSystem[i].modVal[j] -= kFirstResourceID;
                }
                outfitSystem[i].contributes[0] = ResRec(OutfRes, outfitSystemFileHand)->contribute1;
                outfitSystem[i].contributes[1] = ResRec(OutfRes, outfitSystemFileHand)->contribute2;
                outfitSystem[i].requires[0] = ResRec(OutfRes, outfitSystemFileHand)->require1;
                outfitSystem[i].requires[1] = ResRec(OutfRes, outfitSystemFileHand)->require2;
                strcpy((char *)outfitShortNames[i], ResRec(OutfRes, outfitSystemFileHand)->shortName);
                strcpy((char *)lcOutfitName[i], ResRec(OutfRes, outfitSystemFileHand)->lcName);
                strcpy((char *)lcPluralOutfitName[i], ResRec(OutfRes, outfitSystemFileHand)->lcPlural);
                CtoPstr((char *)outfitShortNames[i]);
                CtoPstr((char *)lcOutfitName[i]);
                CtoPstr((char *)lcPluralOutfitName[i]);
                UnlockAHandle(outfitSystemFileHand);
                ResourcePurge(outfitSystemFileHand);
                ReleaseResource(outfitSystemFileHand);
                if (outfitSystem[i].buyRandom > 0 && crashlog_isactive()) {
                    if (!ResourceExists(kPictType, i + 6000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(outfitSystem[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks outfit picture\n");
                    }
                    if (!ResourceExists(kDescType, i + 3000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(outfitSystem[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks description\n");
                    }
                }
                LoadingIntroMusic();
            } else {
                strcpy(outfitSystem[i].availability, "");
                strcpy(outfitSystem[i].onPurchase, "");
                strcpy(outfitSystem[i].onSell, "");
                outfitSystem[i].techLevel = kOutfTechAbsent;
                outfitSystem[i].mass = 0;
                for (j = 0; j < kOutfMods; j++) {
                    outfitSystem[i].modType[j] = 0;
                    outfitSystem[i].modVal[j] = 0;
                }
            }
            if (found >= count)
                break;
        }

        /* alias outfits that share a lowercase display name */
        for (i = 0; i < kMaxOutfits; i++) {
            outfitSystem[i].sameNameIndex = i;
            if (outfitSystem[i].techLevel < kOutfTechAbsent) {
                /* no `i > 0` guard in the original: the for-entry test covers it
                 * (the exe has a single i-compare here, ours had two) */
                if (lcOutfitName[i][0] != 0) {
                    for (j = 0; j < i; j++) {
                        if (outfitSystem[j].techLevel < kOutfTechAbsent &&
                            lcOutfitName[j][0] != 0 && lcOutfitName[i][0] == lcOutfitName[j][0]) {
                            n = 0;
                            while (n < lcOutfitName[i][0] &&
                                   lcOutfitName[i][1 + n] == lcOutfitName[j][1 + n])
                                n++;
                            if (n == lcOutfitName[i][0]) {
                                outfitSystem[i].sameNameIndex = j;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- wëap: weapons ---------------- */
    {
        RGBColor shade;
        float factor;
        short g;

        found = 0;
        count = CountResources(kWeapType);
        for (i = 0; i < kMaxWeapTypes; i++)
            weaponStats[i].guidance = kResAbsent;
        for (i = 0; i < kMaxWeapTypes; i++) {
            weaponFileHand = GetResource(kWeapType, i + kFirstResourceID);
            if (weaponFileHand != NULL) {
                found++;
                LockAHandle(weaponFileHand);
                ResourceNoPurge(weaponFileHand);
                weaponStats[i].reload = ResRec(WeapRes, weaponFileHand)->reload;
                weaponStats[i].lifetime = ResRec(WeapRes, weaponFileHand)->count;
                weaponStats[i].damage[0] = ResRec(WeapRes, weaponFileHand)->massDmg[0];
                weaponStats[i].damage[1] = ResRec(WeapRes, weaponFileHand)->massDmg[1];
                weaponStats[i].guidance = ResRec(WeapRes, weaponFileHand)->guidance;
                weaponStats[i].speed100 = ResRec(WeapRes, weaponFileHand)->speed / 100.0;
                weaponStats[i].ammoType = ResRec(WeapRes, weaponFileHand)->ammoType;
                weaponStats[i].graphic = ResRec(WeapRes, weaponFileHand)->graphic;
                weaponStats[i].inaccuracy = ResRec(WeapRes, weaponFileHand)->inaccuracy;
                weaponStats[i].sound = ResRec(WeapRes, weaponFileHand)->sound;
                weaponStats[i].impact = ResRec(WeapRes, weaponFileHand)->impact;
                weaponStats[i].recoil = ResRec(WeapRes, weaponFileHand)->recoil;
                weaponStats[i].explodType = ResRec(WeapRes, weaponFileHand)->explodType;
                weaponStats[i].proxRadius = ResRec(WeapRes, weaponFileHand)->proxRadius;
                weaponStats[i].blastRadius = ResRec(WeapRes, weaponFileHand)->blastRadius;
                weaponStats[i].flags = ResRec(WeapRes, weaponFileHand)->flags;
                weaponStats[i].flags2 = ResRec(WeapRes, weaponFileHand)->flags2;
                weaponStats[i].flags3 = ResRec(WeapRes, weaponFileHand)->flags3;
                weaponStats[i].durability = ResRec(WeapRes, weaponFileHand)->durability;
                weaponStats[i].maxAmmo = ResRec(WeapRes, weaponFileHand)->maxAmmo;
                weaponStats[i].seeker = ResRec(WeapRes, weaponFileHand)->seeker;
                weaponStats[i].ionization = ResRec(WeapRes, weaponFileHand)->ionization;
                weaponStats[i].exitType = ResRec(WeapRes, weaponFileHand)->exitType;
                weaponStats[i].burstCount = ResRec(WeapRes, weaponFileHand)->burstCount;
                weaponStats[i].burstReload = ResRec(WeapRes, weaponFileHand)->burstReload;
                weaponStats[i].guidedTurn = ResRec(WeapRes, weaponFileHand)->guidedTurn * 0.1;
                if (weaponStats[i].inaccuracy == -1)
                    weaponStats[i].inaccuracy = 0;
                for (j = 0; j < 4; j++) {
                    weaponStats[i].jamVuln[j] = ResRec(WeapRes, weaponFileHand)->jamVuln[j];
                    if (weaponStats[i].jamVuln[j] < 0)
                        weaponStats[i].jamVuln[j] = 0;
                    if (weaponStats[i].jamVuln[j] > 100)
                        weaponStats[i].jamVuln[j] = 100;
                }
                if (ResRec(WeapRes, weaponFileHand)->liColor == 0) {
                    rgb.red = rgb.green = 0x200;
                    rgb.blue = 0xffff;
                } else {
                    rgb.red = ((ResRec(WeapRes, weaponFileHand)->liColor >> 16) & 0xff) * 256;
                    rgb.green = ((ResRec(WeapRes, weaponFileHand)->liColor >> 8) & 0xff) * 256;
                    rgb.blue = (ResRec(WeapRes, weaponFileHand)->liColor & 0xff) * 256;
                }
                ClampRGB(rgb);
                weaponStats[i].ionizeColor = Color2Index(&rgb);
                if (weaponStats[i].burstCount <= 0)
                    weaponStats[i].burstCount = -1;
                weaponStats[i].beamLength = ResRec(WeapRes, weaponFileHand)->beamLength;
                weaponStats[i].beamWidth = ResRec(WeapRes, weaponFileHand)->beamWidth;
                weaponStats[i].coronaFalloff = ResRec(WeapRes, weaponFileHand)->falloff;
                weaponStats[i].beamColor = ResRec(WeapRes, weaponFileHand)->beamColor;
                weaponStats[i].coronaColor = ResRec(WeapRes, weaponFileHand)->coronaColor;
                if (weaponStats[i].beamLength > 0 && weaponStats[i].coronaFalloff <= 0)
                    weaponStats[i].coronaFalloff = 16;
                weaponStats[i].lightningDensity = ResRec(WeapRes, weaponFileHand)->liCohesive;
                weaponStats[i].lightningAmplitude = ResRec(WeapRes, weaponFileHand)->liAmplitude;
                if (weaponStats[i].lightningDensity > 0) {
                    if (weaponStats[i].lightningDensity < 2)
                        weaponStats[i].lightningDensity = 2;
                    weaponStats[i].coronaFalloff = 0;
                    if (weaponStats[i].beamWidth < 1)
                        weaponStats[i].beamWidth = 1;
                    if (weaponStats[i].lightningAmplitude < 1)
                        weaponStats[i].lightningAmplitude = 1;
                }
                weaponStats[i].subCount = ResRec(WeapRes, weaponFileHand)->subCount;
                weaponStats[i].subType = ResRec(WeapRes, weaponFileHand)->subType;
                weaponStats[i].subTheta = ResRec(WeapRes, weaponFileHand)->subTheta;
                weaponStats[i].subLimit = ResRec(WeapRes, weaponFileHand)->subLimit;
                if (weaponStats[i].subTheta == -1)
                    weaponStats[i].subTheta = 0;
                if (weaponStats[i].subType >= kFirstResourceID && weaponStats[i].subType < kFirstResourceID + kMaxWeapTypes)
                    weaponStats[i].subType -= kFirstResourceID;
                else
                    weaponStats[i].subType = -1;
                if (weaponStats[i].guidance != kGuidanceBeam && weaponStats[i].guidance != kGuidanceTurretBeam
                    && weaponStats[i].guidance != kGuidancePDBeam)
                    weaponStats[i].decayShots = weaponStats[i].coronaFalloff;
                else
                    weaponStats[i].decayShots = 0;
                weaponStats[i].proxSafety = ResRec(WeapRes, weaponFileHand)->proxSafety;
                if (weaponStats[i].subCount <= 0 || weaponStats[i].subType < 0) {
                    weaponStats[i].subCount = 0;
                    weaponStats[i].subType = -1;
                }
                if (weaponStats[i].guidance == kGuidanceBeam || weaponStats[i].guidance == kGuidanceTurretBeam
                    || weaponStats[i].guidance == kGuidancePDBeam) {
                    weaponStats[i].rgbScratch.red = ((weaponStats[i].beamColor >> 16) & 0xff) * 256;
                    weaponStats[i].rgbScratch.green = ((weaponStats[i].beamColor >> 8) & 0xff) * 256;
                    weaponStats[i].rgbScratch.blue = (weaponStats[i].beamColor & 0xff) * 256;
                    ClampRGB(weaponStats[i].rgbScratch);
                    if (gSpriteWorld->pixelDepth == 16) {
                        weaponStats[i].beamColor = (weaponStats[i].beamColor >> 9) & 0x7c00 |
                                          (weaponStats[i].beamColor >> 6) & 0x3e0 |
                                          (weaponStats[i].beamColor >> 3) & 0x1f;
                        weaponStats[i].coronaColor = (weaponStats[i].coronaColor >> 9) & 0x7c00 |
                                            (weaponStats[i].coronaColor >> 6) & 0x3e0 |
                                            (weaponStats[i].coronaColor >> 3) & 0x1f;
                    } else {
                        weaponStats[i].beamColor = Color2Index(&weaponStats[i].rgbScratch);
                        rgb.red = ((weaponStats[i].coronaColor >> 16) & 0xff) << 7;
                        rgb.green = ((weaponStats[i].coronaColor >> 8) & 0xff) << 7;
                        rgb.blue = (weaponStats[i].coronaColor & 0xff) << 7;
                        ClampRGB(rgb);
                        weaponStats[i].coronaColor = Color2Index(&rgb);
                    }
                }
                weaponStats[i].hitParticles = ResRec(WeapRes, weaponFileHand)->hitParticles;
                rgb.red = ((ResRec(WeapRes, weaponFileHand)->hitPartColor >> 16) & 0xff) * 256;
                rgb.green = ((ResRec(WeapRes, weaponFileHand)->hitPartColor >> 8) & 0xff) * 256;
                rgb.blue = (ResRec(WeapRes, weaponFileHand)->hitPartColor & 0xff) * 256;
                weaponStats[i].hitPartColor = Color2Index(&rgb);
                weaponStats[i].hitPartLife = ResRec(WeapRes, weaponFileHand)->hitPartLife;
                weaponStats[i].hitPartVel = ResRec(WeapRes, weaponFileHand)->hitPartVel * 0.01;
                if (gSpriteWorld->pixelDepth == 8)
                    weaponStats[i].hitPartLife /= 2;
                GetResInfo(weaponFileHand, &resID, &resType, name);
                TrimResourceName(name);
                pstrcpy(weapName[i], name);
                weaponStats[i].smokeSet = ResRec(WeapRes, weaponFileHand)->smokeSet;
                weaponStats[i].decay = ResRec(WeapRes, weaponFileHand)->decay;
                weaponStats[i].particles = ResRec(WeapRes, weaponFileHand)->particles;
                weaponStats[i].partLifeMin = ResRec(WeapRes, weaponFileHand)->partLifeMin;
                weaponStats[i].partLifeMax = ResRec(WeapRes, weaponFileHand)->partLifeMax;
                base = ResRec(WeapRes, weaponFileHand)->partVel * 0.01;
                rgb.red = ((ResRec(WeapRes, weaponFileHand)->partColor >> 16) & 0xff) * 256;
                rgb.green = ((ResRec(WeapRes, weaponFileHand)->partColor >> 8) & 0xff) * 256;
                rgb.blue = (ResRec(WeapRes, weaponFileHand)->partColor & 0xff) * 256;
                for (j = 0; j < 8; j++) {
                    factor = (Rand(0x51) + 60) * 0.01;
                    shade.red = rgb.red * factor;
                    shade.green = rgb.green * factor;
                    shade.blue = rgb.blue * factor;
                    ClampRGB(shade);
                    weaponStats[i].partColor[j] = Color2Index(&shade);
                    weaponStats[i].partVel[j] = base * factor;
                }
                if (weaponStats[i].smokeSet >= 8)
                    weaponStats[i].smokeSet = 0;
                if (weaponStats[i].guidance == 2) {
                    weaponStats[i].guidance = kGuidanceHoming;
                    weaponStats[i].seeker = 0x30fe;
                }
                UnlockAHandle(weaponFileHand);
                AdjustHandleSize(weaponFileHand, 0x86);
                ResourcePurge(weaponFileHand);
                ReleaseResource(weaponFileHand);
                LoadingIntroMusic();
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* ammo-source sanity warnings */
        if (crashlog_isactive()) {
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (weaponStats[i].guidance != kResAbsent) {
                    short ammo = weaponStats[i].ammoType;
                    if (ammo >= 0 && ammo < kMaxWeapTypes &&
                        weaponStats[i].guidance != kGuidanceBay &&
                        (ammo != weaponStats[ammo].ammoType ||
                         weaponStats[ammo].guidance == kGuidanceBay)) {
                        crashlog_print("warning: weapon ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" probably shouldn't draw ammo from weapon ");
                        crashlog_print_num(ammo + kFirstResourceID);
                        crashlog_print("\n");
                    }
                }
            }
            for (i = 0; i < kMaxOutfits; i++) {
                if (outfitSystem[i].techLevel < kOutfTechAbsent) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModAmmo) {
                            k = outfitSystem[i].modVal[j];
                            g = weaponStats[k].guidance;
                            if (g != kResAbsent) {
                                if (g != kGuidanceBay && k != weaponStats[k].ammoType) {
                                    crashlog_print("warning: outfit item ");
                                    crashlog_print_num(i + kFirstResourceID);
                                    crashlog_print(" grants ammo to weapon ");
                                    crashlog_print_num(k + kFirstResourceID);
                                    if (weaponStats[k].ammoType >= 0 &&
                                        weaponStats[k].ammoType < kMaxWeapTypes)
                                        crashlog_print(", which doesn't use its own ammo\n");
                                    else
                                        crashlog_print(", which doesn't use ammo\n");
                                }
                            } else {
                                crashlog_print("warning: outfit item ");
                                crashlog_print_num(i + kFirstResourceID);
                                crashlog_print(" grants ammo to nonexisent weapon\n");
                            }
                        }
                    }
                }
            }
        }

        /* total range: beams use beam length, shots walk the submunition chain */
        for (i = 0; i < kMaxWeapTypes; i++) {
            weaponStats[i].totalRange = 0;
            g = weaponStats[i].guidance;
            if (weaponStats[i].guidance == kGuidanceUnguided || weaponStats[i].guidance == kGuidanceHoming || weaponStats[i].guidance == kGuidanceTurret || weaponStats[i].guidance == kGuidanceRocket || weaponStats[i].guidance == kGuidanceFrontQuad || weaponStats[i].guidance == kGuidanceRearQuad || weaponStats[i].guidance == kGuidancePointDef) {
                j = i;
                hits = 0;
                do {
                    weaponStats[i].totalRange += (double)(weaponStats[j].speed100 * weaponStats[j].lifetime);
                    hits++;
                    if (weaponStats[j].subCount > 0) {
                        short nxt = weaponStats[j].subType;
                        if (j == nxt) {
                            short lim = weaponStats[j].subLimit;
                            if (lim > 0)
                                weaponStats[i].totalRange += (double)(weaponStats[j].lifetime *
                                                    (lim * weaponStats[j].speed100));
                            j = -1;
                        } else {
                            j = nxt;
                        }
                    } else {
                        j = -1;
                    }
                } while (j != -1);
            } else if (g == kGuidanceBeam || g == kGuidanceTurretBeam || g == kGuidancePDBeam) {
                weaponStats[i].totalRange = weaponStats[i].beamLength;
            }
        }
    }

    /* ---------------- shïp: ship classes ---------------- */
    {
        short shipCount;
        int32_t mass;

        shipCount = CountResources(kShipType);
        if (shipCount == 0)
            crashlog("warning: no " kShipResName " resources found");
        for (i = 0; i < kMaxShipTypes;i++) {
            shipClassFileHand = GetResource(kShipType, i + kFirstResourceID);
            if (shipClassFileHand != NULL) {
                LockAHandle(shipClassFileHand);
                ResourceNoPurge(shipClassFileHand);
                shipClass[i].cargo = ResRec(ShipRes, shipClassFileHand)->cargo;
                shipClass[i].shield = ResRec(ShipRes, shipClassFileHand)->shield;
                shipClass[i].accel = ResRec(ShipRes, shipClassFileHand)->accel / 10000.0;
                shipClass[i].maxSpeed = ResRec(ShipRes, shipClassFileHand)->speed / 100.0;
                shipClass[i].turnRate = ResRec(ShipRes, shipClassFileHand)->turn * 0.1;
                shipClass[i].fuel = ResRec(ShipRes, shipClassFileHand)->energy;
                shipClass[i].freeMass = ResRec(ShipRes, shipClassFileHand)->freeSpace;
                shipClass[i].armor = ResRec(ShipRes, shipClassFileHand)->armor;
                shipClass[i].maxGuns = ResRec(ShipRes, shipClassFileHand)->maxGun;
                shipClass[i].maxTurrets = ResRec(ShipRes, shipClassFileHand)->maxTur;
                shipClass[i].techLevel = ResRec(ShipRes, shipClassFileHand)->techLevel;
                shipClass[i].cost = ResRec(ShipRes, shipClassFileHand)->cost;
                shipClass[i].deathDelay = ResRec(ShipRes, shipClassFileHand)->deathDelay;
                shipClass[i].mass = ResRec(ShipRes, shipClassFileHand)->mass;
                shipClass[i].shipLength = ResRec(ShipRes, shipClassFileHand)->length;
                shipClass[i].inherentAI = ResRec(ShipRes, shipClassFileHand)->inherentAI;
                shipClass[i].crew = ResRec(ShipRes, shipClassFileHand)->crew;
                shipClass[i].strength = ResRec(ShipRes, shipClassFileHand)->strength;
                shipClass[i].flags = ResRec(ShipRes, shipClassFileHand)->flags;
                shipClass[i].flags2 = ResRec(ShipRes, shipClassFileHand)->flags2;
                shipClass[i].flags3 = ResRec(ShipRes, shipClassFileHand)->flags3;
                shipClass[i].dispWeight = ResRec(ShipRes, shipClassFileHand)->dispWeight;
                shipClass[i].podCount = ResRec(ShipRes, shipClassFileHand)->podCount;
                shipClass[i].buyRandom = ResRec(ShipRes, shipClassFileHand)->buyRandom;
                shipClass[i].hireRandom = ResRec(ShipRes, shipClassFileHand)->hireRandom;
                if (InGovtClass(ResRec(ShipRes, shipClassFileHand)->inherentGovt, 0)) {
                    shipClass[i].inherentGovt = ResRec(ShipRes, shipClassFileHand)->inherentGovt - ResClassBase(0);
                    shipClass[i].inherentGovtB = shipClass[i].inherentGovt;
                } else if (InGovtClass(ResRec(ShipRes, shipClassFileHand)->inherentGovt, 1)) {
                    shipClass[i].inherentGovt = -1;
                    shipClass[i].inherentGovtB = ResRec(ShipRes, shipClassFileHand)->inherentGovt - ResClassBase(1);
                } else if (InGovtClass(ResRec(ShipRes, shipClassFileHand)->inherentGovt, 2)) {
                    shipClass[i].inherentGovt = ResRec(ShipRes, shipClassFileHand)->inherentGovt - ResClassBase(2);
                    shipClass[i].inherentGovtB = -1;
                } else {
                    shipClass[i].inherentGovt = -1;
                    shipClass[i].inherentGovtB = -1;
                }
                if (shipClass[i].buyRandom > 100)
                    shipClass[i].buyRandom = 100;
                if (shipClass[i].hireRandom > 100)
                    shipClass[i].hireRandom = 100;
                if (shipClass[i].strength < 1)
                    shipClass[i].strength = 1;
                shipClass[i].upgradeTo = ResRec(ShipRes, shipClassFileHand)->upgradeTo;
                shipClass[i].escUpgradeCost = ResRec(ShipRes, shipClassFileHand)->escUpgrdCost;
                shipClass[i].tradeIn = ResRec(ShipRes, shipClassFileHand)->escSellValue;
                if (shipClass[i].upgradeTo >= kFirstResourceID)
                    shipClass[i].upgradeTo -= kFirstResourceID;
                else
                    shipClass[i].upgradeTo = -1;
                if (shipClass[i].escUpgradeCost < 0)
                    shipClass[i].escUpgradeCost = 0;
                if (shipClass[i].tradeIn <= 0)
                    shipClass[i].tradeIn = shipClass[i].cost * 0.1;
                shipClass[i].escortType = ResRec(ShipRes, shipClassFileHand)->escortType;
                if (shipClass[i].escortType < 0 || shipClass[i].escortType > 3) {
                    if (shipClass[i].inherentAI >= 3) {
                        if (shipClass[i].mass < 50)
                            shipClass[i].escortType = 0;
                        else if (shipClass[i].mass < 200)
                            shipClass[i].escortType = 1;
                        else
                            shipClass[i].escortType = 2;
                    } else {
                        shipClass[i].escortType = 3;
                    }
                }
                if (shipClass[i].escortType == 0)
                    shipClass[i].aiSkill = 80;
                else if (shipClass[i].escortType == 1)
                    shipClass[i].aiSkill = 90;
                else
                    shipClass[i].aiSkill = 100;
                shipClass[i].explode1 = ResRec(ShipRes, shipClassFileHand)->explode[0];
                shipClass[i].explode2 = ResRec(ShipRes, shipClassFileHand)->explode[1];
                shipClass[i].shieldRech = ResRec(ShipRes, shipClassFileHand)->shieldRe;
                shipClass[i].armorRech = ResRec(ShipRes, shipClassFileHand)->armorRe;
                if (shipClass[i].shieldRech > 0.0f)
                    shipClass[i].shieldRech *= 0.001;
                else
                    shipClass[i].shieldRech = 0;
                if (shipClass[i].armorRech > 0.0f)
                    shipClass[i].armorRech *= 0.001;
                else
                    shipClass[i].armorRech = 0;
                shipClass[i].deionize = ResRec(ShipRes, shipClassFileHand)->deionize;
                shipClass[i].ionizeMax = ResRec(ShipRes, shipClassFileHand)->ionizeMax;
                if (shipClass[i].deionize > 0.0f)
                    shipClass[i].deionize *= 0.01;
                else
                    shipClass[i].deionize = 1.0f;
                if (shipClass[i].ionizeMax <= 0)
                    shipClass[i].ionizeMax = 0;
                shipClass[i].keyCarried = ResRec(ShipRes, shipClassFileHand)->keyCarried;
                if (shipClass[i].keyCarried >= kFirstResourceID)
                    shipClass[i].keyCarried -= kFirstResourceID;
                else
                    shipClass[i].keyCarried = -1;
                if (shipClass[i].strength < 1)
                    shipClass[i].strength = 1;
                if (shipClass[i].cargo < 0) {
                    shipClass[i].cargo *= -1;
                    shipClass[i].cargoNotFixed = 0;
                } else {
                    shipClass[i].cargoNotFixed = 1;
                }
                if (shipClass[i].shield < 0)
                    shipClass[i].shield *= -5;
                if (shipClass[i].crew < 0)
                    shipClass[i].crew = 0;
                for (j = 0; j < 4; j++) {
                    mountID[j] = mountID[j + 4] = -1;
                    if (ResRec(ShipRes, shipClassFileHand)->wType[j] >= kFirstResourceID && ResRec(ShipRes, shipClassFileHand)->wType[j] < kFirstResourceID + kMaxWeapTypes) {
                        mountID[j] = ResRec(ShipRes, shipClassFileHand)->wType[j];
                        mountCount[j] = ResRec(ShipRes, shipClassFileHand)->wCount[j];
                        mountAmmo[j] = ResRec(ShipRes, shipClassFileHand)->ammo[j];
                    } else {
                        mountID[j] = -1;
                    }
                    if (ResRec(ShipRes, shipClassFileHand)->wType2[j] >= kFirstResourceID && ResRec(ShipRes, shipClassFileHand)->wType2[j] < kFirstResourceID + kMaxWeapTypes) {
                        mountID[j + 4] = ResRec(ShipRes, shipClassFileHand)->wType2[j];
                        mountCount[j + 4] = ResRec(ShipRes, shipClassFileHand)->wCount2[j];
                        mountAmmo[j + 4] = ResRec(ShipRes, shipClassFileHand)->ammo2[j];
                    } else {
                        mountID[j + 4] = -1;
                    }
                }
                shipClass[i].defOutfID[0] = ResRec(ShipRes, shipClassFileHand)->defaultItems[0];
                shipClass[i].defOutfID[1] = ResRec(ShipRes, shipClassFileHand)->defaultItems[1];
                shipClass[i].defOutfID[2] = ResRec(ShipRes, shipClassFileHand)->defaultItems[2];
                shipClass[i].defOutfID[3] = ResRec(ShipRes, shipClassFileHand)->defaultItems[3];
                shipClass[i].defOutfID[4] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[0];
                shipClass[i].defOutfID[5] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[1];
                shipClass[i].defOutfID[6] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[2];
                shipClass[i].defOutfID[7] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[3];
                shipClass[i].defOutfCount[0] = ResRec(ShipRes, shipClassFileHand)->itemCount[0];
                shipClass[i].defOutfCount[1] = ResRec(ShipRes, shipClassFileHand)->itemCount[1];
                shipClass[i].defOutfCount[2] = ResRec(ShipRes, shipClassFileHand)->itemCount[2];
                shipClass[i].defOutfCount[3] = ResRec(ShipRes, shipClassFileHand)->itemCount[3];
                shipClass[i].defOutfCount[4] = ResRec(ShipRes, shipClassFileHand)->itemCount2[0];
                shipClass[i].defOutfCount[5] = ResRec(ShipRes, shipClassFileHand)->itemCount2[1];
                shipClass[i].defOutfCount[6] = ResRec(ShipRes, shipClassFileHand)->itemCount2[2];
                shipClass[i].defOutfCount[7] = ResRec(ShipRes, shipClassFileHand)->itemCount2[3];
                for (j = 0; j < kDefaultOutfitSlots; j++) {
                    if (shipClass[i].defOutfID[j] >= kFirstResourceID && shipClass[i].defOutfID[j] < kFirstResourceID + kMaxOutfits) {
                        shipClass[i].defOutfID[j] -= kFirstResourceID;
                        if (shipClass[i].defOutfCount[j] < 0)
                            shipClass[i].defOutfCount[j] = 0;
                    } else {
                        shipClass[i].defOutfID[j] = -1;
                        shipClass[i].defOutfCount[j] = 0;
                    }
                }
                shipClass[i].fuelRegen = ResRec(ShipRes, shipClassFileHand)->fuelRegen;
                shipClass[i].skillVar = ResRec(ShipRes, shipClassFileHand)->skillVar;
                if (shipClass[i].skillVar < 1)
                    shipClass[i].skillVar = 1;
                if (shipClass[i].skillVar > 50)
                    shipClass[i].skillVar = 50;
                shipClass[i].contributes[0] = ResRec(ShipRes, shipClassFileHand)->contribute1;
                shipClass[i].contributes[1] = ResRec(ShipRes, shipClassFileHand)->contribute2;
                shipClass[i].requires[0] = ResRec(ShipRes, shipClassFileHand)->require1;
                shipClass[i].requires[1] = ResRec(ShipRes, shipClassFileHand)->require2;
                sum = shipClass[i].cost * 0.25;
                gunTally = 0;
                turretTally = 0;
                for (j = 0; j < 8; j++) {
                    if (mountID[j] >= kFirstResourceID && mountID[j] < kFirstResourceID + kMaxWeapTypes) {
                        shipClass[i].weapCount[mountID[j] - kFirstResourceID] = mountCount[j];
                        shipClass[i].weapAmmo[mountID[j] - kFirstResourceID] = mountAmmo[j];
                        if (mountCount[j] > 0) {
                            for (n = 0; n < kMaxOutfits; n++) {
                                if (outfitSystem[n].modType[0] == kOutfModWeapon &&
                                    outfitSystem[n].modVal[0] == mountID[j] - kFirstResourceID &&
                                    outfitSystem[n].techLevel < kOutfTechAbsent) {
                                    mass = AdjustedItemMass(&outfitSystem[n], &shipClass[i]) * mountCount[j];
                                    shipClass[i].freeMass += mass;
                                    if (!outfitSystem[n].persistent)
                                        sum = sum +
                                            (AdjustedItemCost(&outfitSystem[n], &shipClass[i]) * mountCount[j]) * 0.5;
                                    if (outfitSystem[n].flags & kOutfFixedGun)
                                        gunTally += mountCount[j];
                                    if (outfitSystem[n].flags & kOutfTurret)
                                        turretTally += mountCount[j];
                                    break;
                                }
                            }
                        }
                        if (mountAmmo[j] > 0) {
                            if (weaponStats[mountID[j] - kFirstResourceID].guidance == kGuidanceBay)
                                k = mountID[j] - kFirstResourceID;
                            else {
                                if (weaponStats[mountID[j] - kFirstResourceID].ammoType >= 0 && weaponStats[mountID[j] - kFirstResourceID].ammoType < kMaxWeapTypes)
                                    k = weaponStats[mountID[j] - kFirstResourceID].ammoType;
                                else
                                    k = -1;
                            }
                            if (k != -1) {
                                for (n = 0; n < kMaxOutfits; n++) {
                                    if (outfitSystem[n].modType[0] == kOutfModAmmo &&
                                        outfitSystem[n].modVal[0] == k &&
                                        outfitSystem[n].techLevel < kOutfTechAbsent) {
                                    mass = AdjustedItemMass(&outfitSystem[n], &shipClass[i]) * mountAmmo[j];
                                    shipClass[i].freeMass += mass;
                                        if (!outfitSystem[n].persistent)
                                            sum = sum +
                                                (AdjustedItemCost(&outfitSystem[n], &shipClass[i]) * mountCount[j]) * 0.5;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                m = shipClass[i].maxGuns;
                k = shipClass[i].maxTurrets;
                for (j = 0; j < kDefaultOutfitSlots; j++) {
                    if (shipClass[i].defOutfID[j] != -1 && shipClass[i].defOutfCount[j] > 0 &&
                        outfitSystem[shipClass[i].defOutfID[j]].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&outfitSystem[shipClass[i].defOutfID[j]], &shipClass[i]);
                        mass *= shipClass[i].defOutfCount[j];
                        shipClass[i].freeMass += mass;
                        if (!outfitSystem[shipClass[i].defOutfID[j]].persistent)
                            sum = sum +
                                (AdjustedItemCost(&outfitSystem[shipClass[i].defOutfID[j]], &shipClass[i]) *
                                 shipClass[i].defOutfCount[j]) * 0.5;
                        if (crashlog_isactive()) {
                            for (n = 0; n < kOutfMods; n++) {
                                if (outfitSystem[shipClass[i].defOutfID[j]].modType[n] == 45)
                                    m += outfitSystem[shipClass[i].defOutfID[j]].modVal[n];
                                if (outfitSystem[shipClass[i].defOutfID[j]].modType[n] == 46)
                                    k += outfitSystem[shipClass[i].defOutfID[j]].modVal[n];
                            }
                        }
                    }
                }
                if (shipClass[i].flags & kShipSlowJump)
                    shipClass[i].mobilityFactor = 0.7f;
                else if (shipClass[i].flags & kShipSemiFastJump)
                    shipClass[i].mobilityFactor = 1.3f;
                else if (shipClass[i].flags & kShipFastJump)
                    shipClass[i].mobilityFactor = 1.6f;
                else
                    shipClass[i].mobilityFactor = 1.0f;
                shipClass[i].mobilityFactor *= 1.3;
                if (shipClass[i].mobilityFactor < 0.0f)
                    shipClass[i].mobilityFactor = 0.5f;
                strcpy(shipClass[i].availability, ResRec(ShipRes, shipClassFileHand)->availability);
                strcpy(shipClass[i].appearOn, ResRec(ShipRes, shipClassFileHand)->appearOn);
                strcpy(shipClass[i].onPurchase, ResRec(ShipRes, shipClassFileHand)->onPurchase);
                strcpy(shipClass[i].onCapture, ResRec(ShipRes, shipClassFileHand)->onCapture);
                strcpy(shipClass[i].onRetire, ResRec(ShipRes, shipClassFileHand)->onRetire);
                AdjustHandleSize(shipClassFileHand, 0x744);
                GetResInfo(shipClassFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)shipClass[i].name, (char *)name, 63);
                strcpy((char *)shipyardShortNames[i], ResRec(ShipRes, shipClassFileHand)->shortName);
                strcpy((char *)shipyardLongNames[i], ResRec(ShipRes, shipClassFileHand)->longName);
                strcpy((char *)shipCommNames[i], ResRec(ShipRes, shipClassFileHand)->commName);
                strcpy((char *)shipMovieFilenames[i], ResRec(ShipRes, shipClassFileHand)->movieName);
                strcpy((char *)shipClass[i].subtitle, ResRec(ShipRes, shipClassFileHand)->subtitle);
                CtoPstr((char *)shipyardShortNames[i]);
                CtoPstr((char *)shipyardLongNames[i]);
                CtoPstr((char *)shipCommNames[i]);
                CtoPstr((char *)shipMovieFilenames[i]);
                CtoPstr((char *)shipClass[i].subtitle);
                if (crashlog_isactive()) {
                    if (m < 0) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has a maximum gun count of ");
                        crashlog_print_num(m);
                        crashlog_print("\n");
                    }
                    if (k < 0) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has a maximum turret count of ");
                        crashlog_print_num(k);
                        crashlog_print("\n");
                    }
                    if (gunTally > m) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has ");
                        crashlog_print_num(gunTally);
                        crashlog_print(" gun-type weapon");
                        if (gunTally > 1)
                            crashlog_print("s");
                        crashlog_print(" but its maximum is ");
                        crashlog_print_num(m);
                        crashlog_print("\n");
                    }
                    if (turretTally > k) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has ");
                        crashlog_print_num(turretTally);
                        crashlog_print(" turreted weapon");
                        if (turretTally > 1)
                            crashlog_print("s");
                        crashlog_print(" but its maximum is ");
                        crashlog_print_num(k);
                        crashlog_print("\n");
                    }
                }
                if (sum > shipClass[i].cost && shipClass[i].buyRandom != 0) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(shipClass[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a trade-in value of ");
                    crashlog_print_num(sum);
                    crashlog_print(" but costs only ");
                    crashlog_print_num(shipClass[i].cost);
                    crashlog_print("\n");
                }
                UnlockAHandle(shipClassFileHand);
                ResourcePurge(shipClassFileHand);
                ReleaseResource(shipClassFileHand);
                if (crashlog_isactive()) {
                    if (shipClass[i].buyRandom != 0) {
                        if (!ResourceExists(kDescType, i + 13000)) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(shipClass[i].name);
                            crashlog_print(" (");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(") lacks purchase description\n");
                        }
                        for (m = 0; m < kMaxWeapTypes; m++) {
                            if (shipClass[i].weapCount[m] > 0) {
                                hits = 0;

                                for (j = 0; j < kMaxOutfits; j++) {
                                    for (n = 0; n < kOutfMods; n++) {
                                        if (outfitSystem[j].modType[n] == 1 &&
                                            outfitSystem[j].modVal[n] == m) {
                                            hits++;
                                            break;
                                        }
                                    }
                                    if (hits > 0)
                                        break;
                                }
                                if (hits == 0) {
                                    crashlog_print("warning: weapon ID ");
                                    crashlog_print_num(m + kFirstResourceID);
                                    crashlog_print(" on ship type ");
                                    crashlog_print_num(i + kFirstResourceID);
                                    crashlog_print(" lacks corresponding " kOutfResName " resource\n");
                                }
                            }
                        }
                    }
                    if (shipClass[i].hireRandom != 0 &&
                        !ResourceExists(kDescType, i + 14000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks hiring description\n");
                    }
                }
                LoadingIntroMusic();
            } else {
                strcpy(shipClass[i].availability, "");
                strcpy(shipClass[i].appearOn, "");
                strcpy(shipClass[i].onPurchase, "");
                strcpy(shipClass[i].onCapture, "");
                strcpy(shipClass[i].onRetire, "");
                shipClass[i].techLevel = kResAbsent;
                shipClass[i].shield = 1;
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- düde: dudes ---------------- */
    {
        found = 0;
        count = CountResources(kDudeType);
        for (i = 0; i < kMaxDudes; i++) {
            dudeFileHand = GetResource(kDudeType, i + kFirstResourceID);
            if (dudeFileHand != NULL) {
                found++;
                LockAHandle(dudeFileHand);
                ResourceNoPurge(dudeFileHand);
                dudeData[i].aiType = ResRec(DudeRes, dudeFileHand)->aiType;
                dudeData[i].govt = ResRec(DudeRes, dudeFileHand)->govt;
                dudeData[i].flags = ResRec(DudeRes, dudeFileHand)->booty;
                dudeData[i].adviceFlags = ResRec(DudeRes, dudeFileHand)->infoTypes;
                if (dudeData[i].govt >= kFirstResourceID && dudeData[i].govt < kFirstResourceID + kMaxGovts)
                    dudeData[i].govt -= kFirstResourceID;
                else
                    dudeData[i].govt = -1;
                for (j = 0; j < kDudeShipSlots; j++) {
                    dudeData[i].shipTypes[j] = ResRec(DudeRes, dudeFileHand)->shipTypes[j];
                    dudeData[i].shipProbs[j] = ResRec(DudeRes, dudeFileHand)->probs[j];
                    if (dudeData[i].shipTypes[j] >= kFirstResourceID && dudeData[i].shipTypes[j] < kFirstResourceID + kMaxShipTypes) {
                        dudeData[i].shipTypes[j] -= kFirstResourceID;
                        if (shipClass[dudeData[i].shipTypes[j]].techLevel == kResAbsent) {
                            crashlog_print("warning: " kDudeResName " resource ");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(" refers to nonexistent ship type ");
                            crashlog_print_num(dudeData[i].shipTypes[j] + kFirstResourceID);
                            crashlog_print("\n");
                            dudeData[i].shipTypes[j] = -1;
                        }
                    } else {
                        dudeData[i].shipTypes[j] = -1;
                        dudeData[i].shipProbs[j] = 0;
                    }
                }
                UnlockAHandle(dudeFileHand);
                AdjustHandleSize(dudeFileHand, 0x58);
                ResourcePurge(dudeFileHand);
                ReleaseResource(dudeFileHand);
                dudeData[i].loaded = 1;
                LoadingIntroMusic();
            } else {
                dudeData[i].loaded = 0;
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- gövt: governments ---------------- */
    {
        found = 0;
        count = CountResources(kGovtType);
        for (i = 0; i < kMaxGovts; i++) {
            govtFileHand = GetResource(kGovtType, i + kFirstResourceID);
            if (govtFileHand != NULL) {
                found++;
                LockAHandle(govtFileHand);
                ResourceNoPurge(govtFileHand);
                govtData[i].loaded = 1;
                govtData[i].flags = ResRec(GovtRes, govtFileHand)->flags;
                govtData[i].flags2 = ResRec(GovtRes, govtFileHand)->flags2;
                govtData[i].crimeTol = ResRec(GovtRes, govtFileHand)->crimeTol;
                govtData[i].scanFine = ResRec(GovtRes, govtFileHand)->scanFine;
                govtData[i].initialRec = ResRec(GovtRes, govtFileHand)->initialRec;
                govtData[i].voiceType = ResRec(GovtRes, govtFileHand)->voiceType;
                if (govtData[i].voiceType >= 0 && govtData[i].voiceType < kVoiceSets) {
                    govtData[i].voiceClass = -1;
                } else if (govtData[i].voiceType >= 1000 && govtData[i].voiceType < 1000 + kVoiceSets) {
                    govtData[i].voiceType -= 1000;
                    govtData[i].voiceClass = 1;
                } else if (govtData[i].voiceType >= 2000 && govtData[i].voiceType < 2000 + kVoiceSets) {
                    govtData[i].voiceType -= 2000;
                    govtData[i].voiceClass = 0;
                } else {
                    govtData[i].voiceType = -1;
                    govtData[i].voiceClass = -1;
                }
                for (j = 0; j < 4; j++) {
                    govtData[i].inhJam[j] = ResRec(GovtRes, govtFileHand)->inhJam[j];
                    if (govtData[i].inhJam[j] > 100)
                        govtData[i].inhJam[j] = 100;
                    if (govtData[i].inhJam[j] < 0)
                        govtData[i].inhJam[j] = 0;
                }
                if (ResRec(GovtRes, govtFileHand)->skillMult > 0)
                    govtData[i].skillMult = ResRec(GovtRes, govtFileHand)->skillMult * 0.01;
                else
                    govtData[i].skillMult = 1.0f;
                govtData[i].maxOdds = ResRec(GovtRes, govtFileHand)->maxOdds * 0.01;
                if (govtData[i].maxOdds < 0.01)
                    govtData[i].maxOdds = 0.01f;
                for (j = 0; j <= 4; j++)
                    govtData[i].penalties[j] = ResRec(GovtRes, govtFileHand)->penalty[j];
                for (j = 0; j < 4; j++) {
                    govtData[i].classes[j] = ResRec(GovtRes, govtFileHand)->classes[j];
                    govtData[i].allies[j] = ResRec(GovtRes, govtFileHand)->allies[j];
                    govtData[i].enemies[j] = ResRec(GovtRes, govtFileHand)->enemies[j];
                }
                govtData[i].scanMask = ResRec(GovtRes, govtFileHand)->scanMask;
                govtData[i].requires[0] = ResRec(GovtRes, govtFileHand)->require[0];
                govtData[i].requires[1] = ResRec(GovtRes, govtFileHand)->require[1];
                govtData[i].rgb.red = ((ResRec(GovtRes, govtFileHand)->color >> 16) & 0xff) * 256;
                govtData[i].rgb.green = ((ResRec(GovtRes, govtFileHand)->color >> 8) & 0xff) * 256;
                govtData[i].rgb.blue = (ResRec(GovtRes, govtFileHand)->color & 0xff) * 256;
                govtData[i].rgb2.red = (ResRec(GovtRes, govtFileHand)->shipColor >> 19) & 0x1f;
                govtData[i].rgb2.green = (ResRec(GovtRes, govtFileHand)->shipColor >> 11) & 0x1f;
                govtData[i].rgb2.blue = (ResRec(GovtRes, govtFileHand)->shipColor >> 3) & 0x1f;
                govtData[i].rgbBytes.red = govtData[i].rgb.red >> 8;
                govtData[i].rgbBytes.green = govtData[i].rgb.green >> 8;
                govtData[i].rgbBytes.blue = govtData[i].rgb.blue >> 8;
                govtData[i].interfaceID = ResRec(GovtRes, govtFileHand)->interfaceID;
                if (govtData[i].interfaceID < kFirstResourceID)
                    govtData[i].interfaceID = -1;
                govtData[i].newsPict = ResRec(GovtRes, govtFileHand)->newsPic;
                if (govtData[i].newsPict < kFirstResourceID)
                    govtData[i].newsPict = -1;
                GetResInfo(govtFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)govtData[i].name, (char *)name, 31);
                strcpy((char *)govtTargStrings[i], ResRec(GovtRes, govtFileHand)->targetCode);
                strcpy((char *)govtShortNames[i], ResRec(GovtRes, govtFileHand)->commName);
                strcpy((char *)govtMediumNames[i], ResRec(GovtRes, govtFileHand)->mediumName);
                CtoPstr((char *)govtTargStrings[i]);
                CtoPstr((char *)govtShortNames[i]);
                CtoPstr((char *)govtMediumNames[i]);
                UnlockAHandle(govtFileHand);
                AdjustHandleSize(govtFileHand, 0xc0);
                ResourcePurge(govtFileHand);
                ReleaseResource(govtFileHand);
                LoadingIntroMusic();
            } else {
                govtData[i].loaded = 0;
            }
            if (found >= count)
                break;
        }

        /* allies/enemies consistency check over all pairs */
        if (crashlog_isactive()) {
            for (i = 0; i < kMaxGovts; i++) {
                if (govtData[i].loaded) {
                    for (j = i + 1; j < kMaxGovts; j++) {
                        if (govtData[j].loaded &&
                            GovtEnemies(i, j) && GovtAllies(i, j)) {
                            crashlog_print("warning: govts ");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(" and ");
                            crashlog_print_num(j + kFirstResourceID);
                            crashlog_print(" are simultaneously allies and enemies\n");
                        }
                    }
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- përs: persons ---------------- */
    {
        found = 0;
        count = CountResources(kPersType);
        for (i = 0; i < kMaxPersons;i++) {
            personData[i].loaded = 0;
            personData[i].aiType = -1;
            personFileHand = GetResource(kPersType, i + kFirstResourceID);
            if (personFileHand != NULL) {
                found++;
                LockAHandle(personFileHand);
                ResourceNoPurge(personFileHand);
                personData[i].linkSyst = ResRec(PersRes, personFileHand)->linkSyst;
                personData[i].govt = ResRec(PersRes, personFileHand)->govt;
                personData[i].aiType = ResRec(PersRes, personFileHand)->aiType;
                personData[i].aggression = ResRec(PersRes, personFileHand)->aggress;
                personData[i].coward = ResRec(PersRes, personFileHand)->coward;
                personData[i].shipType = ResRec(PersRes, personFileHand)->shipType;
                if (personData[i].shipType >= kFirstResourceID && personData[i].shipType < kFirstResourceID + kMaxShipTypes) {
                    personData[i].shipType -= kFirstResourceID;
                    if (shipClass[personData[i].shipType].techLevel == kResAbsent) {
                        crashlog_print("warning: p\x91" "rs resource ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" refers to nonexistent ship type ");
                        crashlog_print_num(personData[i].shipType + kFirstResourceID);
                        crashlog_print("\n");
                        personData[i].shipType = -1;
                    }
                } else {
                    personData[i].shipType = 0;
                }
                personData[i].credits = ResRec(PersRes, personFileHand)->credits;
                personData[i].shieldMod = ResRec(PersRes, personFileHand)->shieldMod;
                personData[i].shieldMod = personData[i].shieldMod / 100.0;
                personData[i].commQuote = ResRec(PersRes, personFileHand)->commQuote;
                personData[i].hailQuote = ResRec(PersRes, personFileHand)->hailQuote;
                personData[i].linkMission = ResRec(PersRes, personFileHand)->linkMission;
                personData[i].flags = ResRec(PersRes, personFileHand)->flags;
                personData[i].flags2 = ResRec(PersRes, personFileHand)->flags2;
                personData[i].hailPict = ResRec(PersRes, personFileHand)->hailPict;
                personData[i].rgb5[0] = (ResRec(PersRes, personFileHand)->shipColor >> 19) & 0x1f;
                personData[i].rgb5[1] = (ResRec(PersRes, personFileHand)->shipColor >> 11) & 0x1f;
                personData[i].rgb5[2] = (ResRec(PersRes, personFileHand)->shipColor >> 3) & 0x1f;
                if (personData[i].linkMission >= kFirstResourceID)
                    personData[i].linkMission -= kFirstResourceID;
                else
                    personData[i].linkMission = -1;
                if (personData[i].hailPict < kFirstResourceID)
                    personData[i].hailPict = -1;
                if (personData[i].govt >= kFirstResourceID && personData[i].govt < kFirstResourceID + kMaxGovts)
                    personData[i].govt -= kFirstResourceID;
                else
                    personData[i].govt = -1;
                personData[i].grantClass = ResRec(PersRes, personFileHand)->grantClass;
                personData[i].grantCount = ResRec(PersRes, personFileHand)->grantCount;
                personData[i].grantProb = ResRec(PersRes, personFileHand)->grantProb;
                if (personData[i].grantClass <= 0) {
                    personData[i].grantClass = -1;
                    personData[i].grantCount = 0;
                    personData[i].grantProb = 0;
                } else {
                    if (personData[i].grantProb < 0)
                        personData[i].grantProb = 0;
                    if (personData[i].grantCount < 0)
                        personData[i].grantCount = 0;
                    if (personData[i].grantProb > 100)
                        personData[i].grantProb = 100;
                }
                mountID[1] = ResRec(PersRes, personFileHand)->weapType.left;
                mountID[0] = ResRec(PersRes, personFileHand)->weapType.top;
                mountID[3] = ResRec(PersRes, personFileHand)->weapType.right;
                mountID[2] = ResRec(PersRes, personFileHand)->weapType.bottom;
                mountCount[1] = ResRec(PersRes, personFileHand)->weapCount.left;
                mountCount[0] = ResRec(PersRes, personFileHand)->weapCount.top;
                mountCount[3] = ResRec(PersRes, personFileHand)->weapCount.right;
                mountCount[2] = ResRec(PersRes, personFileHand)->weapCount.bottom;
                mountAmmo[1] = ResRec(PersRes, personFileHand)->ammoLoad.left;
                mountAmmo[0] = ResRec(PersRes, personFileHand)->ammoLoad.top;
                mountAmmo[3] = ResRec(PersRes, personFileHand)->ammoLoad.right;
                mountAmmo[2] = ResRec(PersRes, personFileHand)->ammoLoad.bottom;
                for (j = 0; j < kMaxWeapTypes; j++) {
                    personData[i].weapCount[j] = 0;
                    personData[i].weapAmmo[j] = 0;
                }
                for (j = 0; j <= 3; j++) {
                    if (mountID[j] >= kFirstResourceID) {
                        personData[i].weapCount[mountID[j] - kFirstResourceID] = mountCount[j];
                        personData[i].weapAmmo[mountID[j] - kFirstResourceID] = mountAmmo[j];
                    }
                }
                strcpy(personData[i].subtitle, ResRec(PersRes, personFileHand)->subtitle);
                strcpy(personData[i].activeOn, ResRec(PersRes, personFileHand)->activateOn);
                CtoPstr(personData[i].subtitle);
                AdjustHandleSize(personFileHand, 400);
                personData[i].loaded = 1;
                GetResInfo(personFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy(personData[i].name, (char *)name, 30);
                UnlockAHandle(personFileHand);
                ResourcePurge(personFileHand);
                ReleaseResource(personFileHand);
                personData[i].prepared = 1;
                LoadingIntroMusic();
            } else {
                strcpy(personData[i].activeOn, "");
                personData[i].name[0] = 0;
                personData[i].shipType = 0;
                personData[i].govt = -1;
                personData[i].prepared = 0;
            }
            if (found >= count)
                break;
        }

        /* Captain Hector, the shareware enforcer */
        personData[kPersCaptainHector].linkSyst = -1;
        personData[kPersCaptainHector].govt = -1;
        personData[kPersCaptainHector].aiType = 3;
        personData[kPersCaptainHector].aggression = 4;
        personData[kPersCaptainHector].coward = -1;
        personData[kPersCaptainHector].credits = 0;
        personData[kPersCaptainHector].shieldMod = -1.0f;
        personData[kPersCaptainHector].commQuote = 8;
        personData[kPersCaptainHector].hailQuote = -1;
        personData[kPersCaptainHector].linkMission = -1;
        personData[kPersCaptainHector].flags = 2;
        personData[kPersCaptainHector].rgb5[0] = personData[kPersCaptainHector].rgb5[1] = personData[kPersCaptainHector].rgb5[2] = 0;
        personData[kPersCaptainHector].loaded = 1;
        k = kPersCaptainHector;
        strncpy(personData[k].name, (char *)"\pCaptain Hector", 16);
        strncpy(personData[k].subtitle, (char *)"\pShareware Enforcer", 20);

        /* pick him a ship: sufficiently shielded and agile vs the fleet average */
        sum = 0;
        k = 0;
        for (j = 0; j < kMaxShipTypes;j++) {
            if (shipClass[j].techLevel != kResAbsent && shipClass[j].turnRate > 2.0f) {
                sum += shipClass[j].shield;
                k++;
            }
        }
        if (k > 0)
            sum /= k;
        else
            sum = 300;
        personData[kPersCaptainHector].shipType = -1;
        for (j = 0; j < kMaxShipTypes;j++) {
            if (shipClass[j].techLevel != kResUnloaded && shipClass[j].inherentAI >= 2 &&
                shipClass[j].shield >= sum * 0.88 &&
                shipClass[j].turnRate > 2.0f) {
                personData[kPersCaptainHector].shipType = j;
                break;
            }
        }
        if (personData[kPersCaptainHector].shipType == -1)
            personData[kPersCaptainHector].shipType = 0;

        /* arm him: escalate with the shareware timer */
        for (j = 0; j < kMaxWeapTypes; j++)
            personData[kPersCaptainHector].weapCount[j] = 0;
        n = 0;
        k = kPersCaptainHector;
        for (j = 0; j < kMaxWeapTypes; j++) {
            if (weaponStats[j].guidance >= kGuidanceUnguided &&
                weaponStats[j].damage[0] > 0 && weaponStats[j].damage[1] > 0 &&
                weaponStats[j].subCount <= 0 &&
                (weaponStats[j].damage[0] >= sum / 40.0 ||
                 weaponStats[j].damage[1] >= sum / 40.0)) {
                personData[k].weapCount[j] = 10;
                personData[k].weapAmmo[j] = 1000;
                n++;
                if ((totalDaysPlayed < 35 && n > 2) ||
                    (totalDaysPlayed < 40 && n > 3) ||
                    (totalDaysPlayed < 45 && n > 4) ||
                    (totalDaysPlayed < 50 && n > 5) ||
                    (totalDaysPlayed < 55 && n > 6))
                    break;
            }
        }

        /* alias persons that share a name prefix */
        for (i = 0; i < kMaxPersons;i++)
            personData[i].sameNameIndex = i;
        for (i = 0; i < kMaxPersons;i++) {
            if (personData[i].name[0] > 0) {
                for (j = 0; j < i; j++) {
                    if (personData[j].name[0] > 0 &&
                        j == personData[j].sameNameIndex &&
                        strncmp(&personData[i].name[1],
                                &personData[j].name[1],
                                personData[i].name[0]) == 0) {
                        personData[i].sameNameIndex = j;
                        break;
                    }
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* systems must reference existing dudes and persons */
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].prepared) {
                for (j = 0; j < 8; j++) {
                    if (systemData[i].dudeTypes[j] != -1 && !dudeData[systemData[i].dudeTypes[j]].loaded) {
                        crashlog_print("warning: system ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                        crashlog_print_num(systemData[i].dudeTypes[j] + kFirstResourceID);
                        crashlog_print("\n");
                        systemData[i].dudeTypes[j] = -1;
                    }
                }
                for (j = 0; j < 8; j++) {
                    if (systemData[i].persons[j] != -1 && !personData[systemData[i].persons[j]].prepared) {
                        crashlog_print("warning: system ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" makes reference to nonexistent " kPersResName " resource ");
                        crashlog_print_num(systemData[i].persons[j] + kFirstResourceID);
                        crashlog_print("\n");
                        systemData[i].persons[j] = -1;
                    }
                }
            }
        }
    }

    /* ---------------- öops: news ---------------- */
    {
        found = 0;
        count = CountResources(kOopsType);
        for (i = 0; i < kMaxDisasters; i++) {
            disasterData[i].freq = -1;
            disasterData[i].countdown = -1;
            disasterData[i].duration = -1;
            disasterData[i].stellarID = -1;
            disasterData[i].stellar = -32767;
            disasterData[i].loaded = 0;
        }
        for (i = 0; i < kMaxDisasters; i++) {
            disasterFileHand = GetResource(kOopsType, i + kFirstResourceID);
            if (disasterFileHand != NULL) {
                found++;
                LockAHandle(disasterFileHand);
                ResourceNoPurge(disasterFileHand);
                disasterData[i].stellar = ResRec(OopsRes, disasterFileHand)->stellar;
                disasterData[i].commodity = ResRec(OopsRes, disasterFileHand)->commodity;
                disasterData[i].amount = ResRec(OopsRes, disasterFileHand)->priceDelta;
                disasterData[i].duration = ResRec(OopsRes, disasterFileHand)->duration;
                disasterData[i].freq = ResRec(OopsRes, disasterFileHand)->freq;
                disasterData[i].countdown = 0;
                if (disasterData[i].commodity > 5)
                    disasterData[i].commodity = 5;
                if (disasterData[i].commodity < 0)
                    disasterData[i].commodity = -1;
                strcpy(disasterData[i].text, ResRec(OopsRes, disasterFileHand)->activateOn);
                disasterData[i].loaded = 1;
                AdjustHandleSize(disasterFileHand, 0x11a);
                GetResInfo(disasterFileHand, &resID, &resType, name);
                TrimResourceName(name);
                pstrcpy(disasterData[i].name, name);
                UnlockAHandle(disasterFileHand);
                ResourcePurge(disasterFileHand);
                ReleaseResource(disasterFileHand);
                if (disasterData[i].commodity != -1
                    && abs(disasterData[i].amount) < basePrice[disasterData[i].commodity] * 0.5) {
                    crashlog_print("warning: disaster ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" (");
                    crashlog_print_pstr(disasterData[i].name);
                    crashlog_print(") has an unusually small price delta\n");
                }
                LoadingIntroMusic();
            } else {
                strcpy(disasterData[i].text, "");
            }
            if (found >= count)
                break;
        }
    }

    /* ---------------- nëbu: nebulae ---------------- */
    for (i = 0; i < kMaxNebulae; i++) {
        nebulaData[i].width = nebulaData[i].height = 0;
        nebulaData[i].explored = 0;
        nebulaFileHand = GetResource(kNebuType, i + kFirstResourceID);
        if (nebulaFileHand != NULL) {
            ResourceNoPurge(nebulaFileHand);
            nebulaData[i].xPos = ResRec(NebuRes, nebulaFileHand)->xPos;
            nebulaData[i].yPos = ResRec(NebuRes, nebulaFileHand)->yPos;
            nebulaData[i].width = ResRec(NebuRes, nebulaFileHand)->xSize;
            nebulaData[i].height = ResRec(NebuRes, nebulaFileHand)->ySize;
            strcpy(nebulaData[i].activeOn, ResRec(NebuRes, nebulaFileHand)->activeOn);
            strcpy(nebulaData[i].onExplore, ResRec(NebuRes, nebulaFileHand)->onExplore);
            ResourcePurge(nebulaFileHand);
            ReleaseResource(nebulaFileHand);
            LoadingIntroMusic();
        }
    }

    /* ---------------- bööm: explosions ---------------- */
    for (i = 0; i < kMaxExplodTypes; i++) {
        explodType[i].frameRate = 0.25f;
        explodType[i].sound = -1;
        explodType[i].graphic = i;
        boomFileHand = GetResource(kBoomType, i + kFirstResourceID);
        if (boomFileHand != NULL) {
            explodType[i].frameRate = ResRec(BoomRes, boomFileHand)->frameAdvance;
            explodType[i].sound = ResRec(BoomRes, boomFileHand)->soundIndex;
            explodType[i].graphic = ResRec(BoomRes, boomFileHand)->graphicIndex;
            explodType[i].frameRate *= 0.01;
            ReleaseResource(boomFileHand);
            LoadingIntroMusic();
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);

    /* ---------------- flët: fleets ---------------- */
    {
        found = 0;
        count = CountResources(kFletType);
        for (i = 0; i < kMaxFleets; i++) {
            fleetData[i].leadShipType = -1;
            fleetFileHand = GetResource(kFletType, i + kFirstResourceID);
            if (fleetFileHand != NULL) {
                found++;
                LockAHandle(fleetFileHand);
                ResourceNoPurge(fleetFileHand);
                fleetData[i].leadShipType = ResRec(FletRes, fleetFileHand)->leadShip;
                if (fleetData[i].leadShipType >= kFirstResourceID)
                    fleetData[i].leadShipType -= kFirstResourceID;
                else
                    fleetData[i].leadShipType = 0;
                fleetData[i].govt = ResRec(FletRes, fleetFileHand)->govt;
                fleetData[i].linkSyst = ResRec(FletRes, fleetFileHand)->linkSyst;
                fleetData[i].quoteStrList = ResRec(FletRes, fleetFileHand)->quote;
                fleetData[i].flags = ResRec(FletRes, fleetFileHand)->flags;
                if (fleetData[i].govt >= kFirstResourceID)
                    fleetData[i].govt -= kFirstResourceID;
                else
                    fleetData[i].govt = -1;
                for (j = 0; j <= 3; j++) {   /* exe's back-edge is `cmp ax,3 / jle` */
                    fleetData[i].escortTypes[j] = ResRec(FletRes, fleetFileHand)->escortShip[j];
                    if (fleetData[i].escortTypes[j] >= kFirstResourceID) {
                        fleetData[i].escortTypes[j] -= kFirstResourceID;
                        fleetData[i].escortMin[j] = ResRec(FletRes, fleetFileHand)->minCount[j];
                        fleetData[i].escortMax[j] = ResRec(FletRes, fleetFileHand)->maxCount[j];
                    } else {
                        fleetData[i].escortTypes[j] = -1;
                        fleetData[i].escortMin[j] = 0;
                        fleetData[i].escortMax[j] = 0;
                    }
                }
                strcpy(fleetData[i].activateOn, ResRec(FletRes, fleetFileHand)->activateOn);
                AdjustHandleSize(fleetFileHand, 0x132);
                UnlockAHandle(fleetFileHand);
                ResourcePurge(fleetFileHand);
                ReleaseResource(fleetFileHand);
                LoadingIntroMusic();
            } else {
                strcpy(fleetData[i].activateOn, "");
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- jünk: junk commodities ---------------- */
    {
        found = 0;
        count = CountResources(kJunkType);
        for (i = 0; i < kMaxJunks; i++) {
            for (j = 0; j < 8; j++) {
                junkData[i].boughtAt[j] = -1;
                junkData[i].soldAt[j] = -1;
            }
            junkData[i].basePrice = -1;
        }
        for (i = 0; i < kMaxJunks; i++) {
            junkData[i].held = 0;
            junkFileHand = GetResource(kJunkType, i + kFirstResourceID);
            if (junkFileHand != NULL) {
                found++;
                LockAHandle(junkFileHand);
                ResourceNoPurge(junkFileHand);
                junkData[i].basePrice = ResRec(JunkRes, junkFileHand)->basePrice;
                junkData[i].flags = ResRec(JunkRes, junkFileHand)->flags;
                junkData[i].scanMask = ResRec(JunkRes, junkFileHand)->scanMask;
                if (junkData[i].basePrice < 0) {
                    crashlog_print("warning: junk type ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" has invalid base price\n");
                    junkData[i].basePrice = 0;
                }
                for (j = 0; j < 8; j++) {
                    junkData[i].boughtAt[j] = ResRec(JunkRes, junkFileHand)->boughtAt[j];
                    junkData[i].soldAt[j] = ResRec(JunkRes, junkFileHand)->soldAt[j];
                    if (junkData[i].boughtAt[j] >= kFirstResourceID)
                        junkData[i].boughtAt[j] -= kFirstResourceID;
                    else
                        junkData[i].boughtAt[j] = -1;
                    if (junkData[i].soldAt[j] >= kFirstResourceID)
                        junkData[i].soldAt[j] -= kFirstResourceID;
                    else
                        junkData[i].soldAt[j] = -1;
                }
                GetResInfo(junkFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)junkData[i].name, (char *)name, 63);
                strcpy(junkData[i].lcName, ResRec(JunkRes, junkFileHand)->lcName);
                strcpy(junkData[i].abbrev, ResRec(JunkRes, junkFileHand)->abbrev);
                strcpy(junkData[i].buyOn, ResRec(JunkRes, junkFileHand)->buyOn);
                strcpy(junkData[i].sellOn, ResRec(JunkRes, junkFileHand)->sellOn);
                UnlockAHandle(junkFileHand);
                ResourcePurge(junkFileHand);
                ReleaseResource(junkFileHand);
                LoadingIntroMusic();
            }
            if (found >= count)
                break;
        }

        /* trade centers must trade something */
        for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared &&
                (stellarData[i].flags & (kSpobCanLand | kSpobTradeCenter)) ==
                    (kSpobCanLand | kSpobTradeCenter) &&
                !SpobIsHyperOrWorm(&stellarData[i]) &&
                (stellarData[i].flags & kSpobPriceBits) == 0) {
                k = 0;
                for (j = 0; j < kMaxJunks; j++) {
                    for (n = 0; n < 8; n++) {
                        if (i == junkData[j].boughtAt[n] || i == junkData[j].soldAt[n]) {
                            k++;
                            break;
                        }
                    }
                }
                if (k == 0) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a trade center but trades no goods\n");
                    stellarData[i].flags = stellarData[i].flags & ~kSpobTradeCenter;
                }
            }
        }
    }

    /* ---------------- crön: cron events ---------------- */
    {
        CountResources(kCronType);   /* result unused; the loop walks every slot */
        for (i = 0; i < kMaxCrons; i++) {
            cronEvent[i].active = 0;
            cronEvent[i].loaded = 0;
            cronEvent[i].random = -1;
            cronEvent[i].duration = -1;
            cronEvent[i].daysLeft = 0;
            cronEvent[i].holdoffLeft = 0;
            cronFileHand = GetResource(kCronType, i + kFirstResourceID);
            if (cronFileHand != NULL) {
                LockAHandle(cronFileHand);
                cronEvent[i].firstDay = ResRec(CronRes, cronFileHand)->firstDay;
                cronEvent[i].firstMonth = ResRec(CronRes, cronFileHand)->firstMonth;
                cronEvent[i].firstYear = ResRec(CronRes, cronFileHand)->firstYear;
                cronEvent[i].lastDay = ResRec(CronRes, cronFileHand)->lastDay;
                cronEvent[i].lastMonth = ResRec(CronRes, cronFileHand)->lastMonth;
                cronEvent[i].lastYear = ResRec(CronRes, cronFileHand)->lastYear;
                cronEvent[i].random = ResRec(CronRes, cronFileHand)->random;
                cronEvent[i].duration = ResRec(CronRes, cronFileHand)->duration;
                cronEvent[i].preHoldoff = ResRec(CronRes, cronFileHand)->preHoldoff;
                cronEvent[i].postHoldoff = ResRec(CronRes, cronFileHand)->postHoldoff;
                cronEvent[i].flags = ResRec(CronRes, cronFileHand)->indNewsStr;
                for (j = 0; j < 4; j++) {
                    cronEvent[i].govts[j] = ResRec(CronRes, cronFileHand)->newsGovt[j];
                    cronEvent[i].govtValues[j] = ResRec(CronRes, cronFileHand)->govtNewsStr[j];
                    if (cronEvent[i].govts[j] >= kFirstResourceID) {
                        cronEvent[i].govts[j] -= kFirstResourceID;
                    } else {
                        cronEvent[i].govts[j] = -1;
                        cronEvent[i].govtValues[j] = -1;
                    }
                    if (cronEvent[i].govtValues[j] < -1)
                        cronEvent[i].govtValues[j] = -1;
                }
                cronEvent[i].indNewsStr = ResRec(CronRes, cronFileHand)->flags;
                for (j = 0; j <= 1; j++) {
                    cronEvent[i].contributes[j] = ResRec(CronRes, cronFileHand)->contribute[j];
                    cronEvent[i].requires[j] = ResRec(CronRes, cronFileHand)->require[j];
                }
                strcpy(cronEvent[i].enableOn, ResRec(CronRes, cronFileHand)->enableOn);
                strcpy(cronEvent[i].onStart, ResRec(CronRes, cronFileHand)->onStart);
                strcpy(cronEvent[i].onEnd, ResRec(CronRes, cronFileHand)->onEnd);
                cronEvent[i].loaded = 1;
                UnlockAHandle(cronFileHand);
                AdjustHandleSize(cronFileHand, 0x336);
                ResourcePurge(cronFileHand);
                ReleaseResource(cronFileHand);
                LoadingIntroMusic();
            }
        }
    }

    /* ---------------- ränk: ranks ---------------- */
    {
        for (i = 0; i < kMaxRanks; i++) {
            rankData[i].govt = -1;
            rankData[i].active = 0;
            rankFileHand = GetResource(kRankType, i + kFirstResourceID);
            if (rankFileHand != NULL) {
                LockAHandle(rankFileHand);
                rankData[i].loaded = 1;
                rankData[i].govt = ResRec(RankRes, rankFileHand)->attachTo;
                if (rankData[i].govt >= kFirstResourceID)
                    rankData[i].govt -= kFirstResourceID;
                else
                    rankData[i].govt = -1;
                rankData[i].priority = ResRec(RankRes, rankFileHand)->weight;
                rankData[i].contributes[0] = ResRec(RankRes, rankFileHand)->contribute[0];
                rankData[i].contributes[1] = ResRec(RankRes, rankFileHand)->contribute[1];
                rankData[i].flags = ResRec(RankRes, rankFileHand)->priceMod;
                rankData[i].salary = ResRec(RankRes, rankFileHand)->salary;
                rankData[i].salaryCap = ResRec(RankRes, rankFileHand)->salaryCap;
                rankData[i].priceMod = ResRec(RankRes, rankFileHand)->flags;
                if (rankData[i].priceMod <= 0)
                    rankData[i].priceMod = 100;
                GetResInfo(rankFileHand, &resID, &resType, name);
                TrimResourceName(name);
                PtoCstr(name);
                strncpy(rankData[i].name, (char *)name, 127);
                strcpy(rankData[i].convName, ResRec(RankRes, rankFileHand)->name);
                strcpy(rankData[i].shortName, ResRec(RankRes, rankFileHand)->shortName);
                UnlockAHandle(rankFileHand);
                AdjustHandleSize(rankFileHand, 0x98);
                ResourcePurge(rankFileHand);
                ReleaseResource(rankFileHand);
                LoadingIntroMusic();
            } else {
                rankData[i].loaded = 0;
                strcpy(rankData[i].name, "");
                strcpy(rankData[i].convName, "");
                strcpy(rankData[i].shortName, "");
            }
        }
    }

    /* ---------------- röid: asteroids ---------------- */
    {
        for (i = 0; i < kMaxAsteroids; i++) {
            asteroidFileHand = GetResource(kRoidType, i + kFirstResourceID);
            if (asteroidFileHand != NULL) {
                LockAHandle(asteroidFileHand);
                asteroidType[i].strength = ResRec(RoidRes, asteroidFileHand)->strength;
                if (asteroidType[i].strength < 1)
                    asteroidType[i].strength = 1;
                asteroidType[i].frameRate = ResRec(RoidRes, asteroidFileHand)->spinRate * 0.01;
                asteroidType[i].yieldType = ResRec(RoidRes, asteroidFileHand)->yieldType;
                asteroidType[i].yieldQty = ResRec(RoidRes, asteroidFileHand)->yieldQty;
                if (asteroidType[i].yieldQty < 0) {
                    asteroidType[i].yieldQty = 0;
                    asteroidType[i].yieldType = -1;
                }
                if (asteroidType[i].yieldType < 0 ||
                    (asteroidType[i].yieldType > 6 && asteroidType[i].yieldType < 1000) ||
                    asteroidType[i].yieldType >= 1000 + kMaxJunks) {
                    asteroidType[i].yieldType = -1;
                    asteroidType[i].yieldQty = 0;
                }
                asteroidType[i].partCount = ResRec(RoidRes, asteroidFileHand)->partCount;
                if (asteroidType[i].partCount < 0)
                    asteroidType[i].partCount = 0;
                asteroidType[i].color = ResRec(RoidRes, asteroidFileHand)->partColor;
                rgb.red = ((asteroidType[i].color >> 16) & 0xff) * 256;
                rgb.green = ((asteroidType[i].color >> 8) & 0xff) * 256;
                rgb.blue = (asteroidType[i].color & 0xff) * 256;
                if (gSpriteWorld->pixelDepth == 8) {
                    asteroidType[i].color = Color2Index(&rgb);
                } else {
                    asteroidType[i].color = (((rgb.red >> 11) << 10) & 0x7c00) |
                                  (((rgb.green >> 11) << 5) & 0x3e0) |
                                  ((rgb.blue >> 11) & 0x1f);
                }
                asteroidType[i].fragCount = ResRec(RoidRes, asteroidFileHand)->fragCount;
                if (asteroidType[i].fragCount < 0)
                    asteroidType[i].fragCount = 0;
                for (j = 0; j < 2; j++) {
                    asteroidType[i].fragType[j] = ResRec(RoidRes, asteroidFileHand)->fragType[j];
                    if (asteroidType[i].fragType[j] >= kFirstResourceID && asteroidType[i].fragType[j] < kFirstResourceID + kMaxAsteroids)
                        asteroidType[i].fragType[j] -= kFirstResourceID;
                    else if (asteroidType[i].fragType[j] < 0 || asteroidType[i].fragType[j] >= kMaxAsteroids)
                        asteroidType[i].fragType[j] = -1;
                }
                asteroidType[i].explodType = ResRec(RoidRes, asteroidFileHand)->explodType;
                if (asteroidType[i].explodType < 0 ||
                    (asteroidType[i].explodType >= kMaxExplodTypes && asteroidType[i].explodType < 1000) ||
                    asteroidType[i].explodType >= 1000 + kMaxExplodTypes)
                    asteroidType[i].explodType = -1;
                asteroidType[i].mass = ResRec(RoidRes, asteroidFileHand)->mass;
                UnlockAHandle(asteroidFileHand);
                AdjustHandleSize(asteroidFileHand, 0x28);
                ResourcePurge(asteroidFileHand);
                ReleaseResource(asteroidFileHand);
                LoadingIntroMusic();
            }
        }
    }

    /* ---------------- cölr: interface colors ---------------- */
    colorFileHand = GetIndResource(kColrType, 1);
    if (colorFileHand != NULL) {
        LockAHandle(colorFileHand);
        for (i = 0; i < 2; i++) {
            menuTextColor[i].blue = (ResRec(ColrRes, colorFileHand)->menuColor[i] & 0xff) * 256;
            menuTextColor[i].green = ((ResRec(ColrRes, colorFileHand)->menuColor[i] >> 8) & 0xff) * 256;
            menuTextColor[i].red = ((ResRec(ColrRes, colorFileHand)->menuColor[i] >> 16) & 0xff) * 256;
        }
        for (i = 0; i < 2; i++) {
            shopGridColor[i].blue = (ResRec(ColrRes, colorFileHand)->gridColor[i] & 0xff) * 256;
            shopGridColor[i].green = ((ResRec(ColrRes, colorFileHand)->gridColor[i] >> 8) & 0xff) * 256;
            shopGridColor[i].red = ((ResRec(ColrRes, colorFileHand)->gridColor[i] >> 16) & 0xff) * 256;
        }
        for (i = 0; i < 3; i++) {
            progressColor[i].blue = (ResRec(ColrRes, colorFileHand)->progressColor[i] & 0xff) * 256;
            progressColor[i].green = ((ResRec(ColrRes, colorFileHand)->progressColor[i] >> 8) & 0xff) * 256;
            progressColor[i].red = ((ResRec(ColrRes, colorFileHand)->progressColor[i] >> 16) & 0xff) * 256;
        }
        for (i = 0; i < 3; i++) {
            buttonColor[i].blue = (ResRec(ColrRes, colorFileHand)->buttonColor[i] & 0xff) * 256;
            buttonColor[i].green = ((ResRec(ColrRes, colorFileHand)->buttonColor[i] >> 8) & 0xff) * 256;
            buttonColor[i].red = ((ResRec(ColrRes, colorFileHand)->buttonColor[i] >> 16) & 0xff) * 256;
        }
        for (i = 0; i < kMainButtons; i++) {
            menuButtonPos[i].h = ResRec(ColrRes, colorFileHand)->buttonPos[i * 2];
            menuButtonPos[i].v = ResRec(ColrRes, colorFileHand)->buttonPos[i * 2 + 1];
        }
        floatingBorderColor.blue = (ResRec(ColrRes, colorFileHand)->floatingMapColor & 0xff) * 256;
        floatingBorderColor.green = ((ResRec(ColrRes, colorFileHand)->floatingMapColor >> 8) & 0xff) * 256;
        floatingBorderColor.red = ((ResRec(ColrRes, colorFileHand)->floatingMapColor >> 16) & 0xff) * 256;
        escortMenuColor.blue = (ResRec(ColrRes, colorFileHand)->escortHiliteColor & 0xff) * 256;
        escortMenuColor.green = ((ResRec(ColrRes, colorFileHand)->escortHiliteColor >> 8) & 0xff) * 256;
        escortMenuColor.red = ((ResRec(ColrRes, colorFileHand)->escortHiliteColor >> 16) & 0xff) * 256;
        ldefColors[0][kLdefBlue] = (ResRec(ColrRes, colorFileHand)->listColor[0] & 0xff) * 256;
        ldefColors[0][kLdefGreen] = ((ResRec(ColrRes, colorFileHand)->listColor[0] >> 8) & 0xff) * 256;
        ldefColors[0][kLdefRed] = ((ResRec(ColrRes, colorFileHand)->listColor[0] >> 16) & 0xff) * 256;
        ldefColors[1][kLdefBlue] = (ResRec(ColrRes, colorFileHand)->listColor[1] & 0xff) * 256;
        ldefColors[1][kLdefGreen] = ((ResRec(ColrRes, colorFileHand)->listColor[1] >> 8) & 0xff) * 256;
        ldefColors[1][kLdefRed] = ((ResRec(ColrRes, colorFileHand)->listColor[1] >> 16) & 0xff) * 256;
        ldefColors[2][kLdefBlue] = (ResRec(ColrRes, colorFileHand)->listColor[2] & 0xff) * 256;
        ldefColors[2][kLdefGreen] = ((ResRec(ColrRes, colorFileHand)->listColor[2] >> 8) & 0xff) * 256;
        ldefColors[2][kLdefRed] = ((ResRec(ColrRes, colorFileHand)->listColor[2] >> 16) & 0xff) * 256;
        progressDefRect = ResRec(ColrRes, colorFileHand)->progressBar;
        strcpy((char *)name, ResRec(ColrRes, colorFileHand)->menuFont);
        CtoPstr((char *)name);
        GetFNum(name, &menuFontID);
        menuTextSize = ResRec(ColrRes, colorFileHand)->menuFontSize;
        strcpy((char *)name, ResRec(ColrRes, colorFileHand)->buttonFont);
        CtoPstr((char *)name);
        GetFNum(name, &buttonFontID);
        buttonFontSize = ResRec(ColrRes, colorFileHand)->buttonFontSize;
        logoDestPoint.h = ResRec(ColrRes, colorFileHand)->logoPos[0];
        logoDestPoint.v = ResRec(ColrRes, colorFileHand)->logoPos[1];
        rolloverDestPoint.h = ResRec(ColrRes, colorFileHand)->rolloverPos[0];
        rolloverDestPoint.v = ResRec(ColrRes, colorFileHand)->rolloverPos[1];
        for (i = 0; i < 3; i++) {
            slideDestPoint[i].h = ResRec(ColrRes, colorFileHand)->slidePos[i * 2];
            slideDestPoint[i].v = ResRec(ColrRes, colorFileHand)->slidePos[i * 2 + 1];
        }
        AdjustHandleSize(colorFileHand, 0xf4);
        UnlockAHandle(colorFileHand);
        ResourcePurge(colorFileHand);
        ReleaseResource(colorFileHand);
        LoadingIntroMusic();
    }

    /* ---------------- l33t: easter egg ---------------- */
    if (CountResources(kLeetType) > 0)
        leetMode = 1;
}
