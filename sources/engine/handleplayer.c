// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* handleplayer.c -- the player-ship object (tu-map O7, win 0x0044aa70..0x0045c880). Notes: Research/notes/handleplayer.md */
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "burgermath.h"
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "spriteworld.h"
#include "burger.h"
#include "crashlog.h"

char warpSoundDone;
char playerTurnedThisFrame;
char dontMovePlayerAfterLanding;
char abOn;
short newbieFlag;
float sabotageCounter;
unsigned char DAT_007cab24[4];
char autoDockFlag;
char dockTryFlag;
char boardTryFlag;
char DAT_007cab2b;
FloatPoint wrapDelta;
char lastJumpDistOk;
char ackCommFlag;
char navModeSelectFlag;
char navTargetSelectFlag;
char navDefaultSelectFlag;
char helpKeyFlag;
char cloakFlag;
char fighterCommandFlag;
char weapModeSelectFlag;
char weapTargetSelectFlag;
char weapDefaultSelectFlag;
char playerInfoFlag;
char mapTryFlag;
char jumpTryFlag;
char weapSelectFlag;
char spawnShipFlag;
char weapCloseTargFlag;
char missionInfoFlag;
char commTryFlag;
char mapZoomFlag;
char framerateFlag;
char escortMenuFlag;
char escortGroupFlag[5];
char escortCommandFlag[4];
char pauseKeyFlag;
short gSelectedQuickKey;
short lastEscortCount;
unsigned char DAT_007cab58[16];

extern "C" {

/* the sprite record the engine hangs off ShipObj.unk0 and stellarSprite[];
   gameloop.c's CursorSpriteRec is the same object -- only the three fields
   this TU hit-tests are transcribed */

/* SpriteWorldRec +0x1c is the third SWFrameRec in the arrival blit, so the
   macro casts it. */
#define SWWindowFrame(sw)   ((sw)->windowFrameP)
#define SWWindowRect(sw)    (&(sw)->windRect)

#define kOutfModType47      0x2f
#define kOutfModType50      0x32
#define kShipFlagUntargetable 0x0004
#define kWeapFlagAmmo       0x0800
#define kSpobNoJumpMask     0x3000
#define kMisnHidden         0x0400
#define kJunkGrows          0x0001
#define kJunkPerishes       0x0002

static __inline void CopyFloatPoint(FloatPoint *d, const FloatPoint *s)
{
    d->x = s->x;
    d->y = s->y;
}


                                                            /* 0x004b8ca0 */
                                                            /* 0x0046aad0 */
                                                            /* 0x004192d0 */
                                                            /* 0x0046eeb0 */
                                                            /* 0x0041ad50 */
                                                            /* 0x0046ef00 */
                                                            /* 0x00465d90 */
                                                            /* 0x00413990 */
                                                            /* 0x004254b0 */
                                                            /* 0x00475f70 */
                                                            /* 0x00475830 */
                                                            /* 0x00470220 */

/* HandlePlayer (mac _HandlePlayer). STATE: MATCH 100%, exe 9548 / obj 9548, frame 0x29c,
   byte-identical yes (relocregcheck 0). OPEN: none. */
// FUNCTION: EVNOVA 0x0044aa70
void HandlePlayer(ShipObj *player)
{
    char jumping;
    char turned;
    Str255 msg;
    short i;
    short j;
    short n;
    short count;
    short fuelCap;
    short turnStep;
    float turnLim;
    float heading;
    float angle;
    float step;
    Str255 name;
    float dist;
    float dmg;
    char dying;
    char disabled;
    char ok;
    long aligned;
    char targetUncloaked;
    char targetCloaked;
    char linked[16];
    Point pt;
    Rect box;
    Rect hit;
    Rect *r;
    Rect *portRect;
    FloatPoint stelPos;
    FloatPoint pos;
    FloatPoint dest;
    FloatPoint origin;
    char canJump;
    char routeDone;
    short lost;
    short dir;
    double factor;
    double rollStep;
    short k;
    char found;
    char escortHit;
    short groupCount[4];
    char flag;
    char destructing;
    short rank;
    double span;

    keyComboBlocked = 0;
    if (windowedPref == 0 && noHyperGammaPref == 0 && ignoreKeys == 0) {
        if (player->jumpTimer > 0.0f) {
            if (hyperGamma < 0.0)
                hyperGamma = 0;
            if (hyperGamma > 100.0)
                hyperGamma = 100.0f;
            OnTheFlyWhiteFade(hyperGamma);
        } else if (hyperGamma >= 0.0) {
            hyperGamma = -1.0f;
            FadeGammaTo(100, 0);
        }
    }
    if (cheatsActive == 0)
        gameFrozen = 0;

    if (player->tractorShip != -1) {
        if (player->tractorShip != player->index) {
            if (!shipData[player->tractorShip].active
                    || IsDisabled(&shipData[player->tractorShip])
                    || IsDying(&shipData[player->tractorShip])) {
                player->tractorShip = -1;
                player->tractorTick = 0;
            }
        }
        step = ShipAccelRate(shipData) * 0.25;
        if (player->tractorShip == player->index) {
            pos.x = 0.0f;
            pos.y = 0.0f;
        } else {
            pos.x = shipData[player->tractorShip].vel.x;
            pos.y = shipData[player->tractorShip].vel.y;
        }
        if (pos.x + step < player->vel.x)
            player->vel.x = player->vel.x - step;
        if (pos.x - step > player->vel.x)
            player->vel.x = player->vel.x + step;
        if (pos.y + step < player->vel.y)
            player->vel.y = player->vel.y - step;
        if (pos.y - step > player->vel.y)
            player->vel.y = player->vel.y + step;
        if (FloatAbs(player->vel.x - pos.x) < step)
            player->vel.x = pos.x;
        if (FloatAbs(player->vel.y - pos.y) < step)
            player->vel.y = pos.y;
        if (ReadTick() >= player->tractorTick + 0x1e)
            player->tractorShip = -1;
    }

    heading = player->heading;
    playerTurnedThisFrame = 0;
    if (player->jumpTimer <= 0.0f)
        WrapPlayerPosition();
    if (*(short *)player->unk6e == 1 && player->targetShip != -1) {
        if (!shipData[player->targetShip].active
                || IsDying(&shipData[player->targetShip])
                || AIShipIsEmergingFromHypergate(&shipData[player->targetShip])
                || (ShipIsCloaked(&shipData[player->targetShip])
                    && shipData[player->targetShip].parentShip != 0
                    && !ShipCanTargetCloakedShips(player))) {
            player->targetShip = -1;
            statusRedraw[6] = 1;
        }
    }

    if (!player->active) {
        player->targetShip = -1;
        player->targetStellar = -1;
        if (player->deathTimer > -240.0f) {
            player->deathTimer = player->deathTimer - 1.0f;
        } else {
            if (playerSabotaged != 0) {
                for (i = 0; i < kMaxOutfits; i++) {
                    if (shipSystems[i] > 0) {
                        for (j = 0; j < kOutfMods; j++) {
                            if (gOutfits[i].modType[j] == kOutfModType47
                                    && gOutfits[i].modVal[j] > 0) {
                                HideCursorSprite();
                                RefreshSAT();
                                ShowTheCursor();
                                LoadDescription(gTextShowString,
                                                gOutfits[i].modVal[j]);
                                ShowTextDialog(0, 0, 1);
                                SetAllKeyCmdFlags();
                                break;
                            }
                        }
                    }
                }
            }
            gPause = 1;
        }
        if (QuickKeyCheck(keyPrefs.cheatRevive)) {
            ResetPlayer(1);
            player->targetShip = -1;
            player->selectedWeapon = -1;
            RedrawStatus(1);
            RedrawComm(0);
        }
        return;
    }

    disabled = IsDisabled(player);
    if (disabled && player->jumpTimer > 0.0f) {
        span = (ReadTick() - player->jumpTick)
               * gShipTypes[player->type].mobilityFactor / (WarpSoundDuration() * 0.01);
        step = span;
        if (gShipTypes[player->type].mobilityFactor != 0)
            step = step - 35.0 / gShipTypes[player->type].mobilityFactor;
        if (step > 0.0) {
            if (ShipMaxSpeed(player) < step)
                step = ShipMaxSpeed(player);
            player->vel.x = 0;
            player->vel.y = 0;
            Accel(player->heading, step, &player->vel);
        }
        player->jumpTimer = -1.0f;
        GetIndString(msg, kGameStrList, 0x23);
        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
        if (ST_IsSoundPlaying(warpUpSnd[0])) {
            ST_HaltSound(warpUpSnd[0]);
            warpSoundDone = 0;
        }
        if (ST_IsSoundPlaying(warpUpSnd[1])) {
            ST_HaltSound(warpUpSnd[1]);
            warpSoundDone = 0;
        }
        PlaySound(beepSnd[1], 0x32, gSoundVolume, gSoundVolume);
    }

    dying = IsDying(player);
    if (disabled) {
        player->vel.x = player->vel.x * 0.995;
        player->vel.y = player->vel.y * 0.995;
    } else {
        player->boarded = 0;
    }
    if (playerRepairInhibit >= 0.0)
        playerRepairInhibit = playerRepairInhibit - gTimeScale;
    if (disabled && playerRepairInhibit < 0.0 && ShipCanSelfRepair(player)) {
        if (gShipTypes[player->type].flags & kShipFlagHalfArmor)
            player->armor = ShipArmorCapacity(player) * 0.1 + 1.0;
        else
            player->armor = ShipArmorCapacity(player) * 0.3333 + 1.0;
        GetIndString(msg, kGameStrList, 0x25);
        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    }
    if (!disabled)
        HandleShipPointDefense(player);

    turned = 0;
    if (frameCounter % 60 == 0) {
        oldPlayerThreatened = playerThreatened;
        playerThreatened = IsPlayerThreatened();
        if (playerThreatened && !oldPlayerThreatened
                && player->escapePods <= 0) {
            PlaySound(redAlertSnd, 5, gSoundVolume, gSoundVolume);
            if (volumePref <= 1)
                alertSpritePending = 0x1e;
        }
    }

    if (player->escapePods > 0) {
        AdjustedAccel(player->heading,
                      ShipAccelRate(player) * gTimeScale,
                      &player->vel, ShipMaxSpeed(player));
        player->pos.x = player->vel.x * gTimeScale + player->pos.x;
        player->pos.y = player->vel.y * gTimeScale + player->pos.y;
        player->escapePods -= 1;
        if (player->escapePods == 0) {
            FadeScreenOut();
            ClearStarfield();
            ClearParticles(0);
            SWHideAllSprites(gSpriteWorld);
            SWSetPortToWindow(gSpriteWorld);
            RGBForeColor(blackColor);
            PaintRect(GetCWindowPortRect(gWind));
            FadeScreenIn();
            ShowTheCursor();
            SWSetPortToWindow(gSpriteWorld);
            RGBForeColor(blackColor);
            PaintRect(GetCWindowPortRect(gWind));
            ShowTheCursor();
            LoadDescription(gTextShowString, 0x36af);
            ShowTextDialog(0, 0, 1);
            SWSetPortToWindow(gSpriteWorld);
            RGBForeColor(blackColor);
            PaintRect(GetCWindowPortRect(gWind));
            FadeScreenOut();
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active) {
                    AbortMission(i, 1);
                    currentMissionObjectives[i].active = 0;
                }
            }
            n = player->curSystem;
            ResetPlayer(0);
            EvalMissionBitSetString(gShipTypes[shipData->type].onPurchase);
            DeleteAllComputerShips(1, 1);
            j = EscapeStellarSearch(n);
            if (j != -1)
                player->curSystem = stellarData[j].system;
            else
                player->curSystem = 0;
            mapScroll.x = gSystems[player->curSystem].xPos;
            mapScroll.y = gSystems[player->curSystem].yPos;
            player->weap[0].count = 0;
            player->weap[0].ammo = 0;
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (gShipTypes[player->type].weapAmmo[i] > 0) {
                    if (gWeapons[i].ammoType >= 0
                            && gWeapons[i].ammoType < kMaxWeapTypes
                            && gWeapons[i].guidance != kGuidanceBay)
                        player->weap[gWeapons[i].ammoType].ammo =
                            gShipTypes[player->type].weapAmmo[i];
                    else
                        player->weap[i].ammo =
                            gShipTypes[player->type].weapAmmo[i];
                }
                player->weap[i].count = gShipTypes[player->type].weapCount[i];
                player->weap[i].reloadTimer = 0;
                player->weap[i].burstCount = 0;
            }
            AutoSetExploration(player->curSystem, 0, 1);
            RandomizeStars();
            player->fuel = ShipFuelCapacity(player);
            player->shield = ShipShieldCapacity(player);
            player->armor = ShipShieldCapacity(player);
            SetupShipsInSystem(player->curSystem, 1);
            n = Rand(0x1e) + 15;
            for (i = 0; i < n; i++)
                IncrementGameTime();
            HandleStellarSystemVisibility();
            pstrcpy(shipName, gShipTypes->name);
            PtoCstr(shipName);
            strcat((char *)shipName, " ");
            for (i = 0; i <= 3; i++) {
                NumToString(Rand(9) + 1, msg);
                PtoCstr(msg);
                strcat((char *)shipName, (char *)msg);
            }
            CtoPstr((char *)shipName);
            RGBForeColor(blackColor);
            PaintRect(GetCWindowPortRect(gWind));
            SetSystemBackgroundColor();
            PaintRect(&SWWindowFrame(gSpriteWorld)->frameRect);
            RGBForeColor(blackColor);
            RefreshSAT();
            RedrawStatus(1);
            RedrawComm(0);
            statusRedraw[6] = statusRedraw[0] = statusRedraw[3] =
                statusRedraw[4] = statusRedraw[1] = 1;
            for (i = 0; i < kMaxSystems; i++) {
                if (gSystems[i].govt >= 0)
                    playerLegalStatus[i] =
                        govtData[gSystems[i].govt].initialRec;
                else
                    playerLegalStatus[i] = 0;
            }
            if (strictPlay) {
                found = 0;
                for (j = 0; j < kNavSlots; j++) {
                    if (gSystems[player->curSystem].navDefault[j] != -1) {
                        SavePilotFile(
                            gSystems[player->curSystem].navDefault[j]);
                        found = 1;
                        break;
                    }
                }
                if (!found)
                    SavePilotFile(0);
            }
            FadeScreenIn();
        }
        return;
    }

    if (playerSabotaged != 0) {
        if (sabotageCounter < 0.0f) {
            sabotageCounter = Rand(100);
        } else if (!dying) {
            sabotageCounter = sabotageCounter + gTimeScale;
            if (sabotageCounter > 300.0f) {
                PlaySound(explodSnd[0], 0x64, gSoundVolume, gSoundVolume);
                if (playerSabotaged == 1) {
                    player->armor = -1.0f;
                    player->shield = -1.0f;
                    player->jumpTimer = -1.0f;
                    msg[0] = 0;
                    for (i = 0; i < kMaxOutfits; i++) {
                        if (shipSystems[i] > 0) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (gOutfits[i].modType[j] == kOutfModType47) {
                                    if (shipSystems[i] > 1
                                            && lcPluralOutfitName[i][0]) {
                                        strcpy((char *)msg, "");
                                        AppendIndString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcPluralOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        AppendIndString((char *)msg, kGameStrList, 0x29);
                                        CtoPstr((char *)msg);
                                    } else if (shipSystems[i] == 1
                                            && lcOutfitName[i][0]) {
                                        strcpy((char *)msg, "");
                                        AppendIndString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        AppendIndString((char *)msg, kGameStrList, 0x28);
                                        CtoPstr((char *)msg);
                                    }
                                    break;
                                }
                            }
                        }
                        if (msg[0])
                            break;
                    }
                    if (ST_IsSoundPlaying(warpUpSnd[0])) {
                        ST_HaltSound(warpUpSnd[0]);
                        warpSoundDone = 0;
                    }
                    if (ST_IsSoundPlaying(warpUpSnd[1])) {
                        ST_HaltSound(warpUpSnd[1]);
                        warpSoundDone = 0;
                    }
                    if (!msg[0]) {
                        if (leetMode)
                            pstrcpy(msg, "\pSomeone set up us the bomb!");
                        else
                            GetIndString(msg, kGameStrList, 0x26);
                    }
                    DisplayComm(msg, 0x168, 0, 0xc, &rgbWhite, 0, 0);
                } else {
                    for (i = 0; i < kMaxOutfits; i++) {
                        count = shipSystems[i];
                        if (count > 0) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (gOutfits[i].modType[j] == kOutfModType50) {
                                    if (count > 1) {
                                        strcpy((char *)msg, "");
                                        AppendIndString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcPluralOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        AppendIndString((char *)msg, kGameStrList, 0x29);
                                        CtoPstr((char *)msg);
                                    } else {
                                        strcpy((char *)msg, "");
                                        AppendIndString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        AppendIndString((char *)msg, kGameStrList, 0x28);
                                        CtoPstr((char *)msg);
                                    }
                                    DisplayComm(msg, 0x190, 0, 0xc, &rgbWhite, 0, 0);
                                    if (gOutfits[i].modVal[j] >= 0x80 && gOutfits[i].modVal[j] < 0xc0)
                                        CreateExplosion(
                                            player->pos,
                                            gOutfits[i].modVal[j] - 0x80, 0, 1);
                                    shipSystems[i] = 0;
                                    dmg = Rand((ShipArmorCapacity(player) * 0.5 + 1.0))
                                          + ShipArmorCapacity(player);
                                    DamageShip(player, &player->pos, 0, dmg, 0, -1, 0, 0, 1, 1, 1, 0);
                                    break;
                                }
                            }
                        }
                    }
                    statusRedraw[1] = 1;
                    ResetPlayerPrecalcedValues();
                }
            }
        }
    } else {
        sabotageCounter = 0;
    }

    if (QuickKeyCheck(keyPrefs.clearTarget) && (KeyCheck(0x38) || KeyCheck(0x6f))) {
        keyPressedThisFrame = 1;
        if (!navModeSelectFlag) {
            navModeSelectFlag = 1;
            if (player->targetShip != -1) {
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                player->targetShip = -1;
                statusRedraw[6] = 1;
            }
        }
    }
    if ((QuickKeyCheck(keyPrefs.clearTarget) && !KeyCheck(0x38) && !KeyCheck(0x6f))
            || QuickKeyCheck(keyPrefs.hyperspace)) {
        if (player->jumpTimer <= 0.0f) {
            keyPressedThisFrame = 1;
            if (!navModeSelectFlag) {
                navModeSelectFlag = 1;
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                statusRedraw[0] = 1;
                dockTarget = -1;
                dockPermission = -1;
                if (QuickKeyCheck(keyPrefs.clearTarget)) {
                    player->navTargetKind = -1;
                    player->targetStellar = -1;
                    statusRedraw[0] = 1;
                }
                if (QuickKeyCheck(keyPrefs.hyperspace)
                        && player->navTargetKind != kDepartureRoutePicked) {
                    player->navTargetKind = kDepartureRoutePicked;
                    player->targetStellar = -1;
                    SelectHyperLinkToSelection();
                }
            }
            if (QuickKeyCheck(keyPrefs.hyperspace))
                StartHyperJump();
        } else {
            navModeSelectFlag = 0;
        }
    } else {
        navModeSelectFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.nextSystem)) {
        keyPressedThisFrame = 1;
        if (!navTargetSelectFlag && player->jumpTimer <= 0.0f) {
            navTargetSelectFlag = 1;
            statusRedraw[0] = 1;
            n = 0;
            for (i = 0; i < kNavSlots; i++) {
                linked[i] = 0;
                if (gSystems[player->curSystem].links[i] != -1
                        && FindActiveCoLocatedSystem(
                               gSystems[player->curSystem].links[i]) != -1) {
                    n++;
                    linked[i] = 1;
                }
            }
            if (n > 0) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (player->navTargetKind != kDepartureRoutePicked)
                    player->targetStellar = -1;
                player->navTargetKind = kDepartureRoutePicked;
                do {
                    player->targetStellar += 1;
                    if (player->targetStellar >= kNavSlots)
                        player->targetStellar = 0;
                    if (player->targetStellar < 0)
                        player->targetStellar = kNavSlots - 1;
                } while (!linked[player->targetStellar]);
            } else {
                player->targetStellar = -1;
            }
        }
        if (player->navTargetKind == kDepartureRoutePicked)
            StartHyperJump();
    } else {
        navTargetSelectFlag = 0;
    }

    gShipTypes[gameIsRegisteredIndex].registered =
        (*(long *)((char *)shipData + kLicenseCopies) != 0) ? (unsigned char)1 : (unsigned char)0;

    if (player->jumpTimer <= 0.0f) {
        j = -1;
        if (QuickKeyCheck(2))
            j = 0;
        if (QuickKeyCheck(3))
            j = 1;
        if (QuickKeyCheck(4))
            j = 2;
        if (QuickKeyCheck(5))
            j = 3;
        for (i = 0; i <= 3; i++) {
            if (QuickKeyCheck(keyPrefs.navPreset[i]))
                j = i;
        }
        if (QuickKeyCheck(0x3f)
                || ((QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                    && QuickKeyCheck(6)))
            j = 0x10;
        if (j != -1) {
            keyPressedThisFrame = 1;
            if (!navDefaultSelectFlag) {
                navDefaultSelectFlag = 1;
                if (j == 0x10)
                    n = FindNearestStellar(shipData);
                else
                    n = gSystems[player->curSystem].navDefault[j];
                if (n != -1
                        && player->curSystem == stellarData[n].system) {
                    PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                    if (player->navTargetKind != 2 || player->targetStellar != n) {
                        navTargetExpand = 256.0f;
                        player->targetStellar = n;
                        statusRedraw[0] = 1;
                        player->navTargetKind = 2;
                        dockTarget = -1;
                        dockPermission = -1;
                    }
                }
            }
        } else {
            navDefaultSelectFlag = 0;
        }
    }

    if (QuickKeyCheck(keyPrefs.targetNext)) {
        keyPressedThisFrame = 1;
        if (!weapTargetSelectFlag) {
            weapTargetSelectFlag = 1;
            statusRedraw[6] = 1;
            if (*(short *)player->unk6e == 1) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
                    n = FindPrevTarget(player->targetShip, player->curSystem);
                else
                    n = FindNextTarget(player->targetShip, player->curSystem);
                if (n == player->targetShip || n == player->index) {
                    player->targetShip = -1;
                } else {
                    player->targetShip = n;
                    if (SWIsSpriteInRect((SpritePtr)shipData[n].sprite,
                                         SWWindowRect(gSpriteWorld)))
                        weapTargetExpand = 256.0f;
                    else
                        weapTargetExpand = 0;
                }
            }
        }
    } else if (Button()) {
        if (!weapTargetSelectFlag) {
            weapTargetSelectFlag = 1;
            n = player->targetShip;
            SWSetPortToWindow(gSpriteWorld);
            GetGlobalMouse(&pt);
            portRect = GetCWindowPortRect(gWind);
            box.top = portRect->top;
            box.left = portRect->left;
            box.bottom = portRect->bottom;
            box.right = portRect->right;
            box.right -= 0xc2;
            if (escortMenuOnscreen > 0
                    && PtInRect(pt.h, pt.v, &escortMenuRect)) {
                for (i = 0; i < 5; i++) {
                    hit.left = escortMenuRect.left + 4;
                    hit.right = escortMenuRect.right - 4;
                    hit.top = escortMenuRect.top + 0x1e + i * 0x12;
                    hit.bottom = escortMenuRect.top + 0x2b + i * 0x12;
                    if (PtInRect(pt.h, pt.v, &hit)) {
                        if (gSelectedQuickKey != i - 1) {
                            gSelectedQuickKey = i - 1;
                            ResetMapZoom();
                            lastEscortMenuTime = ReadTick();
                        }
                        break;
                    }
                }
            } else if (showFloatingMap > 0 && player->jumpTimer <= 0.0f
                    && PtInRect(pt.h, pt.v, &floatingMapRect)) {
                hit.left = hit.right =
                    (floatingMapRect.left + floatingMapRect.right) / 2;
                hit.top = hit.bottom =
                    (floatingMapRect.top + floatingMapRect.bottom) / 2;
                InsetRect(&hit, -8, -8);
                if (PtInRect(pt.h, pt.v, &hit)) {
                    player->navTargetKind = kDepartureRoutePicked;
                    player->targetStellar = -1;
                    statusRedraw[0] = 1;
                    StartHyperJump();
                    PlaySound(beepSnd[2], 1, gSoundVolume,
                              gSoundVolume);
                } else {
                    for (j = 0; j < kNavSlots; j++) {
                        if (gSystems[player->curSystem].links[j] != -1
                                && FindActiveCoLocatedSystem(
                                       gSystems[player->curSystem].links[j])
                                   != -1) {
                            hit.left = hit.right = 
                                ((gSystems[gSystems[player->curSystem].links[j]].xPos
                                  - gSystems[player->curSystem].xPos)
                                 / floatingMapZoom
                                 + (floatingMapRect.left
                                    + floatingMapRect.right) / 2);
                            hit.top = hit.bottom = 
                                ((gSystems[gSystems[player->curSystem].links[j]].yPos
                                  - gSystems[player->curSystem].yPos)
                                 / floatingMapZoom
                                 + (floatingMapRect.top
                                    + floatingMapRect.bottom) / 2);
                            if (floatingMapZoom <= 0.5)
                                InsetRect(&hit, -12, -12);
                            else
                                InsetRect(&hit, -8, -8);
                            if (PtInRect(pt.h, pt.v, &hit)) {
                                player->targetStellar = j;
                                player->navTargetKind = kDepartureRoutePicked;
                                statusRedraw[0] = 1;
                                StartHyperJump();
                                PlaySound(beepSnd[2], 1, gSoundVolume,
                                          gSoundVolume);
                                break;
                            }
                        }
                    }
                }
            } else if (PtInRect(pt.h, pt.v, &box)) {
                targetUncloaked = ShipCanTargetUntargetableShips(shipData);
                targetCloaked = ShipCanTargetCloakedShips(shipData);
                if (player->active && player->sprite != NULL) {
                    r = &((SpriteRec *)player->sprite)->destFrameRect;
                    box.top = r->top;
                    box.left = r->left;
                    box.bottom = r->bottom;
                    box.right = r->right;
                    if (box.right - box.left < 0x30)
                        InsetRect(&box, -0x10, -0x10);
                    if (PtInRect(pt.h, pt.v, &box))
                        player->targetShip = -1;
                }
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active
                            && !IsDying(&shipData[i])
                            && (!ShipIsCloaked(&shipData[i]) || targetCloaked
                                || shipData[i].parentShip == 0)
                            && player->curSystem == shipData[i].curSystem
                            && (targetUncloaked
                                || !(gShipTypes[shipData[i].type].flags2
                                     & kShipFlagUntargetable))
                            && shipData[i].sprite != NULL) {
                        r = &((SpriteRec *)shipData[i].sprite)->destFrameRect;
                        box.top = r->top;
                        box.left = r->left;
                        box.bottom = r->bottom;
                        box.right = r->right;
                        if (box.right - box.left < 0x30)
                            InsetRect(&box, -0x10, -0x10);
                        if (PtInRect(pt.h, pt.v, &box)) {
                            player->targetShip = i;
                            break;
                        }
                    }
                }
                if (player->targetShip != n) {
                    if (*(short *)player->unk6e == 1) {
                        statusRedraw[6] = 1;
                        if (player->targetShip != -1)
                            PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                        else
                            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                        if (player->targetShip != -1) {
                            if (SWIsSpriteInRect((SpritePtr)shipData[player->targetShip].sprite,
                                    SWWindowRect(gSpriteWorld)))
                                weapTargetExpand = 256.0f;
                            else
                                weapTargetExpand = 0;
                        }
                    }
                } else if (player->jumpTimer <= 0.0f) {
                    for (i = 0; i < kNavSlots; i++) {
                        if (stellarSprite[i] != NULL && stellarSprite[i]->isVisible) {
                            j = stellarSprite[i]->userData;
                            if (stellarData[j].loaded) {
                                r = &stellarSprite[i]->destFrameRect;
                                box.top = r->top;
                                box.left = r->left;
                                box.bottom = r->bottom;
                                box.right = r->right;
                                if (box.right - box.left < 0x30)
                                    InsetRect(&box, -0x10, -0x10);
                                if (PtInRect(pt.h, pt.v, &box)) {
                                    if (player->targetStellar != j
                                            || player->navTargetKind != 2) {
                                        PlaySound(beepSnd[0], 1, gSoundVolume,
                                                  gSoundVolume);
                                        player->targetStellar = j;
                                        navTargetExpand = 256.0f;
                                        statusRedraw[0] = 1;
                                        player->navTargetKind = 2;
                                        dockTarget = -1;
                                        dockPermission = -1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        weapTargetSelectFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.map)) {
        keyPressedThisFrame = 1;
        if (!mapTryFlag && player->jumpTimer <= 0.0f && shipData->deathTimer <= 0.0f
                && fadePending[0] == 0 && fadePending[1] == 0
                && frameCounter >= 0) {
            PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
            HideCursorSprite();
            RefreshSAT();
            ShowTheCursor();
            hypergateMapMode = 0;
            DoSystemMap();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            SWSetPortToWindow(gSpriteWorld);
            RedrawStatus(1);
            RedrawComm(0);
            SetAllKeyCmdFlags();
        } else {
            mapTryFlag = 1;
        }
    } else {
        mapTryFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.land) && player->deathTimer <= 0.0f
            && player->escapePods <= 0 && player->jumpTimer <= 0.0f) {
        keyPressedThisFrame = 1;
        if (player->navTargetKind != 2 || player->targetStellar == -1) {
            n = FindNearestStellar(player);
            if (n != -1) {
                statusRedraw[0] = 1;
                player->navTargetKind = 2;
                player->targetStellar = n;
                navTargetExpand = 256.0f;
            }
        }
        if (!dockTryFlag && player->navTargetKind == 2
                && player->targetStellar != -1) {
            dockTryFlag = 1;
            HandlePlayerDockRequest();
        }
    } else {
        dockTryFlag = 0;
    }
    if (dockTarget != -1 && !dockTryFlag)
        HandlePlayerDockApproach();

    if (QuickKeyCheck(keyPrefs.board)) {
        keyPressedThisFrame = 1;
        HandlePlayerBoardAttempt();
    } else {
        boardTryFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.targetNearest)) {
        keyPressedThisFrame = 1;
        if (!weapCloseTargFlag) {
            if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                n = FindNearestShipToPlayer();
            else
                n = FindNearestThreatToPlayer();
            weapCloseTargFlag = 1;
            if (n != -1 && *(short *)player->unk6e == 1
                    && n != player->targetShip) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                statusRedraw[6] = 1;
                *(short *)player->unk6e = 1;
                player->targetShip = n;
                if (SWIsSpriteInRect((SpritePtr)shipData[n].sprite,
                                     SWWindowRect(gSpriteWorld)))
                    weapTargetExpand = 256.0f;
                else
                    weapTargetExpand = 0;
            } else if (n == -1) {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        weapCloseTargFlag = 0;
    }

    if (player->jumpTimer <= 0.0f && QuickKeyCheck(keyPrefs.firePrimary)
            && player->aiTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (player->weap[i].count > 0
                    && !(gWeapons[i].flags & kWeapFlagTurret))
                FirePlayerWeapon(i);
        }
    }
    if (player->jumpTimer <= 0.0f && QuickKeyCheck(keyPrefs.fireSecondary)
            && player->selectedWeapon != -1 && player->aiTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        FirePlayerWeapon(player->selectedWeapon);
    }
    if (player->selectedWeapon != -1
            && (gWeapons[player->selectedWeapon].flags & kWeapFlagAmmo)
            && !WeaponHasAmmo(player, player->selectedWeapon)) {
        player->selectedWeapon = -1;
        statusRedraw[3] = 1;
    }

    if (QuickKeyCheck(keyPrefs.selectSecondary)) {
        keyPressedThisFrame = 1;
        if (!weapSelectFlag) {
            weapSelectFlag = 1;
            n = 0;
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (player->weap[i].count > 0
                        && (gWeapons[i].flags & kWeapFlagTurret)
                        && (gWeapons[i].guidance < kGuidancePointDef
                            || gWeapons[i].guidance == kGuidanceBay)) {
                    if (gWeapons[i].flags2 & kWeapFlagAmmo) {
                        if (WeaponHasAmmo(player, i))
                            n++;
                    } else {
                        n++;
                    }
                }
            }
            if (n > 0) {
                statusRedraw[3] = 1;
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                    j = -1;
                else
                    j = 1;
                ok = 0;
                do {
                    player->selectedWeapon += j;
                    while (player->selectedWeapon >= kMaxWeapTypes)
                        player->selectedWeapon -= kMaxWeapTypes;
                    while (player->selectedWeapon < 0)
                        player->selectedWeapon += kMaxWeapTypes;
                    if (player->selectedWeapon >= 0 && player->selectedWeapon < kMaxWeapTypes) {
                        if (player->weap[player->selectedWeapon].count > 0
                                && (gWeapons[player->selectedWeapon].flags
                                    & kWeapFlagTurret)
                                && (gWeapons[player->selectedWeapon].guidance < kGuidancePointDef
                                    || gWeapons[player->selectedWeapon].guidance
                                       == kGuidanceBay)) {
                            if (!(gWeapons[player->selectedWeapon].flags2 & kWeapFlagAmmo))
                                ok = 1;
                            else if (WeaponHasAmmo(player, player->selectedWeapon))
                                ok = 1;
                        }
                    } else {
                        ok = 0;
                    }
                } while (!ok);
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        weapSelectFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.deselectSecondary) && player->selectedWeapon != -1) {
        keyPressedThisFrame = 1;
        player->selectedWeapon = -1;
        statusRedraw[3] = 1;
        PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
    }

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (player->weap[i].count > 0) {
            if (player->weap[i].reloadTimer > 0.0f)
                player->weap[i].reloadTimer -= gTimeScale;
            else
                player->weap[i].reloadTimer = 0;
            if ((gWeapons[i].seeker & kWeapSeekNoFireIonized)
                    && ShipIonizationFactor(player) > 0.0f)
                player->weap[i].reloadTimer = 1.0f;
        }
    }

    if (QuickKeyCheck(keyPrefs.faceTarget) && player->jumpTimer <= 0.0f) {
        keyPressedThisFrame = 1;
        if (player->targetShip != -1) {
            if (player->targetStellar == -1) {
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos,
                        &shipData[player->targetShip].pos);
            } else if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt)) {
                stelPos = XYShortToFloat(
                    stellarData[player->targetStellar].pos);
                origin.x = stelPos.x;
                origin.y = stelPos.y;
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos, &origin);
            } else {
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos,
                        &shipData[player->targetShip].pos);
            }
            turned = 1;
        } else if (player->targetStellar != -1) {
            stelPos = XYShortToFloat(
                stellarData[player->targetStellar].pos);
            origin.x = stelPos.x;
            origin.y = stelPos.y;
            player->desiredHeading = CalcInterceptAngle(&player->pos, &origin);
            turned = 1;
        }
    }
    if (player->navTargetKind == kDepartureRoutePicked) {
        dockTarget = -1;
        dockPermission = -1;
        if (player->targetStellar != -1 && player->jumpTimer > 0.0f
                && (ShipCanExpiditeJumps(player)
                    || (abs((short)player->vel.x) < 2
                        && abs((short)player->vel.y) < 2))) {
            pos.x = gSystems[player->curSystem].xPos;
            pos.y = gSystems[player->curSystem].yPos;
            j = gSystems[player->curSystem].links[player->targetStellar];
            dest.x = gSystems[j].xPos;
            dest.y = gSystems[j].yPos;
            player->desiredHeading = CalcInterceptAngle(&pos, &dest);
            turned = 1;
        }
    }

    jumping = 0;
    if ((QuickKeyCheck(keyPrefs.jump) && !IsDisabled(player))
            || player->jumpTimer > 0.0f) {
        keyPressedThisFrame = 1;
        if (player->navTargetKind == kDepartureRoutePicked
                && player->targetStellar != -1
                && -999.0f != player->jumpTimer
                && player->fuel >= 100.0f) {
            canJump = 1;
            for (i = 0; i < kNavSlots; i++) {
                if (gSystems[player->curSystem].navDefault[i] != -1
                        && !(stellarData[gSystems[player->curSystem].navDefault[i]].flags2
                             & kSpobNoJumpMask)) {
                    origin.y = 0;
                    origin.x = origin.y;
                    turnLim = ShipHyperSafeDist(player);
                    if (FloatAbs(XYFloatSquaredDist(&origin,
                                &player->pos)) <= turnLim)
                        canJump = 0;
                }
            }
            if (player->tractorShip != -1 && player->tractorShip != player->index)
                canJump = 0;
            if (canJump || player->jumpTimer > 0.0f) {
                if (commMsg[0] > 0) {
                    GetIndString(msg, kGameStrList, 0x2a);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x24);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1e);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1f);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                }
                if (canJump && player->jumpTimer <= 0.0f) {
                    count = 0;
                    n = -1;
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active
                                && shipData[i].parentShip == 0
                                && shipData[i].aiType == 5
                                && !AIShipHasFuelForJump(&shipData[i])
                                && !(gShipTypes[shipData[i].type].flags2
                                     & kShipFlagNoVoice)) {
                            count = count + 1;
                            n = shipData[i].type;
                        }
                    }
                    if (count > 0) {
                        pstrcpy(msg, gShipTypes[n].name);
                        PtoCstr(msg);
                        strcat((char *)msg, ":  ");
                        if (count == 1)
                            AppendIndString((char *)msg, kGameStrList,
                                            Rand(2) + 0xa0);
                        else
                            AppendIndString((char *)msg, kGameStrList,
                                            Rand(2) + 0xa2);
                        CtoPstr((char *)msg);
                        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
                        PlaySound(beepSnd[4], 0x32, gSoundVolume, gSoundVolume);
                    }
                }
                if (ShipCanExpiditeJumps(player)
                        || (abs((short)player->vel.x) < 2
                            && abs((short)player->vel.y) < 2)) {
                    jumping = 1;
                    if ((gShipTypes[player->type].shanFlags & kShipRecBanks)
                            && !(gShipTypes[player->type].shanFlags & 0x80)
                            && player->foldSet > 0)
                        player->foldDir = -1;
                    if (player->jumpTimer <= 1.0f) {
                        player->jumpTimer = 2.0f;
                        player->jumpTick = ReadTick();
                        statusRedraw[0] = 1;
                        newbieFlag = 0x7fff;
                        if (doubleTime) {
                            if (!ST_IsSoundPlaying(warpUpSnd[1])) {
                                warpUpSoundSpec.handle = (void **)warpUpSnd[1];
                                SetSoundRate(
                                    gShipTypes[player->type].mobilityFactor);
                                ST_PlaySoundParam(&warpUpSoundSpec);
                                warpSoundDone = 0;
                            }
                        } else if (!ST_IsSoundPlaying(warpUpSnd[0])) {
                            warpUpSoundSpec.handle = (void **)warpUpSnd[0];
                            SetSoundRate(
                                gShipTypes[player->type].mobilityFactor);
                            ST_PlaySoundParam(&warpUpSoundSpec);
                            warpSoundDone = 0;
                        }
                    }
                } else {
                    player->jumpTick = ReadTick();
                    player->jumpTimer = 1.0f;
                    if (ShipIsInertialess(player)) {
                        player->accelMag = player->accelMag
                            - ShipAccelRate(player) * gTimeScale;
                        if (player->accelMag < 0.0f)
                            player->accelMag = 0;
                    } else {
                        pos.y = 0;
                        pos.x = pos.y;
                        dest.x = player->vel.x * 100.0;
                        dest.y = player->vel.y * 100.0;
                        player->desiredHeading =
                            CalcInterceptAngle(&dest, &pos);
                        turned = 1;
                        aligned = abs(AngularDifference(
                                 player->desiredHeading,
                                 player->heading))
                                  < (ShipTurnRate(player) + 1.0f > 20.0f
                                     ? ShipTurnRate(player) + 1.0f : 20.0f);
                        if (aligned) {
                            Accel(player->heading, ShipAccelRate(player),
                                  &player->vel);
                            if (player->engineGlow < 0x18)
                                player->engineGlow += 3;
                        }
                        player->vel.x = player->vel.x * 0.992;
                        player->vel.y = player->vel.y * 0.992;
                    }
                }
            } else {
                if (jumpTryFlag) {
                    player->jumpTimer = 0;
                } else {
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                    if (player->tractorShip != -1
                            && player->tractorShip != player->index)
                        GetIndString(msg, kGameStrList, 0x24);
                    else
                        GetIndString(msg, kGameStrList, 0x2a);
                    DisplayComm(msg, 0x168, 0, 0xc, &rgbWhite, 0, 0);
                }
            }
        } else if (!jumpTryFlag) {
            if (newbieFlag <= 2 && !disabled && newbieString[7][0] != 0) {
                if (player->navTargetKind != kDepartureRoutePicked
                        || player->targetStellar == -1) {
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                    DisplayComm(newbieString[7], 0x168, 0, 0xc, &rgbWhite, 0, 0);
                }
            } else if (player->navTargetKind == kDepartureRoutePicked
                    && player->targetStellar != -1
                    && -999.0f != player->jumpTimer
                    && player->fuel < 100.0f) {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                DisplayComm(fuelString[7], 0xf0, 0, 0xc, &rgbWhite, 0, 0);
            }
        }
        jumpTryFlag = 1;
    } else {
        jumpTryFlag = 0;
    }

    if ((jumping || player->jumpTimer > 1.0f) && !disabled) {
        MakeEscortsHyperWithParent(player);
        player->jumpTimer = player->jumpTimer + gTimeScale;
        if (!ShipCanExpiditeJumps(player)) {
            player->vel.x = player->vel.x * 0.98;
            player->vel.y = player->vel.y * 0.98;
        }
        if (player->jumpTimer > 30.0f) {
            if (!ST_IsSoundPlaying(warpUpSnd[0])
                    && !ST_IsSoundPlaying(warpUpSnd[1]))
                warpSoundDone = 1;
        }
        if (player->jumpTimer >= 30.0f && warpSoundDone) {
            warpSoundDone = 0;
            if (!ST_IsSoundPlaying(warpOutSnd))
                PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
            player->pos.y = 0;
            player->pos.x = player->pos.y;
            pos.x = gSystems[gSystems[player->curSystem]
                         .links[player->targetStellar]].xPos;
            pos.y = gSystems[gSystems[player->curSystem]
                         .links[player->targetStellar]].yPos;
            dest.x = gSystems[player->curSystem].xPos;
            dest.y = gSystems[player->curSystem].yPos;
            Accel(((CalcInterceptAngle(&dest, &pos) + 180) % 360),
                  1350.0f, &player->pos);
            player->vel.y = 0;
            player->vel.x = player->vel.y;
            if (!ShipIsInertialess(player))
                Accel(player->heading, ShipMaxSpeed(player),
                      &player->vel);
            else
                player->accelMag = ShipMaxSpeed(player);
            n = gSystems[player->curSystem].links[player->targetStellar];
            player->prevSystem = player->curSystem;
            routeDone = 0;
            if (hyperRoute[1] != -1
                    && FindActiveCoLocatedSystem(n)
                       == FindActiveCoLocatedSystem(hyperRoute[1])
                    && (count = ShipJumpsPerJump(player)) > 1) {
                for (i = 1; i < count; i++) {
                    n = FindActiveCoLocatedSystem(hyperRoute[1]);
                    if (n == -1)
                        break;
                    player->curSystem = n;
                    ExploreNebulaeFromSystem(n);
                    CheckJumpRouteArrival();
                    if (hyperRoute[1] == -1)
                        break;
                }
                routeDone = 1;
            }
            player->curSystem = n;
            ExploreNebulaeFromSystem(player->curSystem);
            player->navTargetKind = -1;
            player->targetStellar = -1;
            player->targetShip = -1;
            player->fuel = player->fuel - 100.0f;
            SWSetPortToWindow(gSpriteWorld);
            RGBForeColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            RGBForeColor(blackColor);
            ScreenEndScene();
            SWSetPortToWorkArea(gSpriteWorld);
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, SWWindowFrame(gSpriteWorld),
                         &gSpriteWorld->workFrameP->frameRect,
                         &SWWindowFrame(gSpriteWorld)->frameRect);
            PropagateMissionBitEffects();
            if (gSystems[shipData->curSystem].murk >= 0)
                RandomizeStars();
            else
                ClearStarfield();
            ScrollStarfield(0.0f, 0.0f);
            EraseParticlesOffscreen(gSpriteWorld);
            ClearParticles(1);
            statusRedraw[0] = 1;
            statusRedraw[6] = 1;
            statusRedraw[2] = 1;
            statusRedraw[3] = 1;
            statusRedraw[4] = 1;
            dockTarget = -1;
            dockPermission = -1;
            lastMissionStellar = -1;
            bribeRandom = -1;
            adviceRandom = Rand(0x800);
            weapLawFlag = 0;
            smuggleFlag = 0;
            newbieFlag = 0x7fff;
            antiEscortDelay = Rand(0x1e) + 0x1e;
            SWSetSpriteVisible(alertSprite, 0);
            alertSpritePending = 0;
            for (i = 0; i < 1000; i++)
                missionRandom[i] = Rand(100) + 1;
            mapScroll.x = gSystems[player->curSystem].xPos;
            mapScroll.y = gSystems[player->curSystem].yPos;
            count = ShipHyperTransitTime(player);
            lost = 0;
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active && shipData[i].parentShip == 0
                        && !IsDisabled(&shipData[i])) {
                    if (shipData[i].aiType == 5
                            && !AIShipHasFuelForJump(&shipData[i])) {
                        shipData[i].active = 0;
                        lost++;
                    }
                    if (shipData[i].active) {
                        rank = ShipHyperTransitTime(&shipData[i]);
                        if (rank > count)
                            count = rank;
                    }
                }
            }
            for (i = 0; i < count; i++)
                IncrementGameTime();
            HandleGlobalPriceFlux();
            HandleLocalPriceFlux();
            MissionHandlePlayerEnteredNewSystem();
            j = FindFirstCoLocatedSystem(player->curSystem);
            if (gSystems[j].exploreLevel < 1)
                gSystems[j].exploreLevel = 1;
            if (gSystems[player->curSystem].exploreLevel < 1)
                gSystems[player->curSystem].exploreLevel = 1;
            AutoSetExploration(player->curSystem, 0, 1);
            player->jumpTimer = -999.0f;
            SetupShipsInSystem(player->curSystem, 0);
            SpawnBountyHunter();
            HandleStellarSystemVisibility();
            player->jumpTimer = 0;
            crashlog_print("entering system ");
            crashlog_print_num(j + 128);
            crashlog_print(" (");
            crashlog_print_pstr(gSystems[j].name);
            crashlog_print(")\n");
            if (gSystems[j].message != -1) {
                DisplayMessage(gSystems[j].message);
            } else {
                n = Rand(3);
                strcpy((char *)msg, "");
                AppendIndString((char *)msg, kGameStrList, n + 0x2b);
                strcat((char *)msg, " ");
                pstrcpy(name, gSystems[j].name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                AppendIndString((char *)msg, kGameStrList, 0x30);
                strcat((char *)msg, " ");
                DateToLongString(gameTime, (char *)name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, ".");
                n = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (gSystems[player->curSystem].navDefault[i] != -1)
                        n++;
                }
                if (n == 0) {
                    strcat((char *)msg, " ");
                    AppendIndString((char *)msg, kGameStrList, 0x31);
                }
                if (lost > 0) {
                    strcat((char *)msg, "  (");
                    StrcatOrdinalNumber(lost, (char *)msg, 1);
                    strcat((char *)msg, " ");
                    if (lost == 1)
                        AppendIndString((char *)msg, kGameStrList, 0xa4);
                    else
                        AppendIndString((char *)msg, kGameStrList, 0xa5);
                    strcat((char *)msg, ")");
                }
                CtoPstr((char *)msg);
                DisplayComm(msg, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
            }
            if (hyperRoute[1] != -1 && !routeDone) {
                j = FindActiveCoLocatedSystem(hyperRoute[1]);
                if (j != FindActiveCoLocatedSystem(shipData->curSystem)) {
                    for (i = 0; i < 32; i++)
                        hyperRoute[i] = -1;
                }
            }
            CheckJumpRouteArrival();
            SelectHyperLinkToSelection();
            DrawMapInFloatingFrame();
            killShots = 1;
            killBoxes = 1;
            killExplods = 1;
            killSmoke = 1;
            killAsteroids = 1;
            missionSoundID = -1;
            escortMenuOnscreen = 0;
            CreateAsteroids();
            ResetPlayerPrecalcedValues();
            if (gShipTypes[player->type].shanFlags & 0x80) {
                player->foldSet =
                    gShipTypes[player->type].baseSets - 1;
                player->foldDir = 0;
            } else {
                player->foldSet = 0;
                player->foldDir = 1;
            }
            player->frameTimer = 0;
            player->shieldGlow = 0;
            player->weapGlow = 0;
            SWHideAllSprites(gSpriteWorld);
            HandleStellarSystemVisibility();
            PrepareAllSprites();
            SWSetPortToWorkArea(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->backFrameP->frameRect);
            SWSetPortToWorkArea(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            SWSetPortToWindow(gSpriteWorld);
            RefreshSAT();
            RedrawComm(1);
            DoEscortPayment(count);
            return;
        }
        if (player->jumpTimer > 30.0f) {
            if ((ReadTick() - player->jumpTick)
                    > WarpSoundDuration()
                      / gShipTypes[player->type].mobilityFactor) {
                if (ST_IsSoundPlaying(warpUpSnd[0]))
                    ST_HaltSound(warpUpSnd[0]);
                if (ST_IsSoundPlaying(warpUpSnd[1]))
                    ST_HaltSound(warpUpSnd[1]);
                if (!warpSoundDone) {
                    warpSoundDone = 1;
                    PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
                }
            }
        }
    } else if (-999.0f == player->jumpTimer) {
        player->jumpTimer = 0;
    }

    if (QuickKeyCheck(keyPrefs.reverse) && player->jumpTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        if (ShipIsInertialess(player)) {
            player->accelMag = player->accelMag
                               - ShipAccelRate(player) * gTimeScale;
            if (player->accelMag < 0.0)
                player->accelMag = 0;
        } else if (FloatAbs(player->vel.x) >= 0.05 || FloatAbs(player->vel.y) >= 0.05) {
            pos.y = 0;
            pos.x = pos.y;
            dest.x = player->vel.x * 100.0;
            dest.y = player->vel.y * 100.0;
            player->desiredHeading =
                (CalcInterceptAngle(&pos, &dest) + 180) % 360;
            turned = 1;
        }
    }

    turnStep = ShipTurnRate(player);
    dir = 0;
    if (turned && abs(AngularDifference(player->desiredHeading, player->heading)) > turnStep * gTimeScale
            && !disabled) {
        angle = player->desiredHeading - player->heading;
        while (angle >= 360.0f)
            angle = angle - 360.0f;
        while (angle < 0.0f)
            angle = angle + 360.0f;
        if (angle > 180.0f) {
            player->heading -= turnStep * gTimeScale;
            dir = -1;
        } else {
            player->heading += turnStep * gTimeScale;
            dir = 1;
        }
    } else if (player->jumpTimer <= 0.0f && !disabled && !turned
            && (QuickKeyCheck(keyPrefs.turnLeft) || QuickKeyCheck(keyPrefs.turnRight))) {
        keyPressedThisFrame = 1;
        if (QuickKeyCheck(keyPrefs.turnLeft) && !QuickKeyCheck(keyPrefs.turnRight)) {
            player->heading = player->heading - turnStep * gTimeScale;
            dir = -1;
        } else if (QuickKeyCheck(keyPrefs.turnRight)
                && !QuickKeyCheck(keyPrefs.turnLeft)) {
            player->heading = player->heading + turnStep * gTimeScale;
            dir = 1;
        }
    }
    if (player->heading >= 360.0)
        player->heading -= 360.0;
    if (player->heading < 0.0)
        player->heading += 360.0;

    abOn = 0;
    if (QuickKeyCheck(keyPrefs.afterburner) && !disabled) {
        keyPressedThisFrame = 1;
        if (HasAfterburner(player) && player->jumpTimer <= 0.0f
                && player->fuel > 0.0f && player->aiTimer <= 0.0f
                && ShipAfterburnerFuelUsage(player) <= player->fuel)
            abOn = 1;
    }

    if (player->aiTimer <= 0.0f && QuickKeyCheck(keyPrefs.accelerate)
            && player->jumpTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        if (ShipIsInertialess(player)) {
            player->accelMag = ShipAccelRate(player) * gTimeScale
                               + player->accelMag;
            if (ShipMaxSpeed(player) < player->accelMag)
                player->accelMag = ShipMaxSpeed(player);
        } else if (abOn && !gravitySourcesPresent) {
            AdjustedAccel(player->heading,
                          ShipAccelRate(player) * gTimeScale,
                          &player->vel,
                          ShipMaxSpeed(player) * 1.8);
        } else {
            AdjustedAccel(player->heading,
                          ShipAccelRate(player) * gTimeScale,
                          &player->vel, ShipMaxSpeed(player));
        }
        if (player->engineGlow < 0x18)
            player->engineGlow += 1;
    } else if (player->engineGlow > 0) {
        player->engineGlow -= 1;
    }

    player->bankDir = 0;
    if (gShipTypes[player->type].shanFlags & kShipRecRollable) {
        if (dir != 0) {
            if (dir > 0 && player->frameTimer < 16.0f)
                player->frameTimer += gTimeScale;
            if (dir < 0 && player->frameTimer > -16.0f)
                player->frameTimer -= gTimeScale;
        } else {
            rollStep = gTimeScale * 2.0;
            if (player->frameTimer >= rollStep)
                player->frameTimer -= rollStep;
            else if (-gTimeScale * 2.0 >= player->frameTimer)
                player->frameTimer += rollStep;
            else
                player->frameTimer = 0;
        }
        if ((dir > 0 || turned) && player->frameTimer > 6.0f)
            player->bankDir = 1;
        else if ((dir < 0 || turned) && player->frameTimer < -6.0f)
            player->bankDir = -1;
        else
            player->bankDir = 0;
        if (player->bankDir != 0
                && (gShipTypes[player->type].shanFlags & kShipRecBanks)
                && player->engineGlow < 0x18)
            player->engineGlow += 2;
    }

    if (player->aiTimer > 0.0f || player->jumpTimer > 0.0f) {
        if ((gShipTypes[player->type].shanFlags & kShipRecRollable)
                && player->jumpTimer > 30.0f)
            player->frameTimer = 0;
        if (player->engineGlow > 0)
            player->engineGlow -= 1;
    }

    if (ShipShieldCapacity(player) > player->shield && !dying && !disabled) {
        player->shield = ShipShieldRechargeRate(player) * gTimeScale + player->shield;
        statusRedraw[1] = 1;
    }
    if (ShipArmorCapacity(player) > player->armor && !dying
            && !IsDisabled(player) && playerRepairInhibit < 0.0) {
        player->armor = ShipArmorRechargeRate(player) * gTimeScale + player->armor;
        statusRedraw[1] = 1;
    }

    if (player->jumpTimer > 1.0f && !disabled) {
        pos.x = gSystems[player->curSystem].xPos;
        pos.y = gSystems[player->curSystem].yPos;
        n = gSystems[player->curSystem].links[player->targetStellar];
        dest.x = gSystems[n].xPos;
        dest.y = gSystems[n].yPos;
        n = CalcInterceptAngle(&pos, &dest);
        aligned = abs(AngularDifference(player->heading, n))
                  <= (ShipTurnRate(player) > 30.0f ? ShipTurnRate(player) : 30.0f);
        if (aligned) {
            span = (ReadTick() - player->jumpTick)
                   * gShipTypes[player->type].mobilityFactor / (WarpSoundDuration() * 0.01);
            step = span;
            if (gShipTypes[player->type].mobilityFactor != 0) {
                factor = doubleTime ? 1.5 : 1.0;
                step = step - factor * 35.0
                       / gShipTypes[player->type].mobilityFactor;
            }
            if (step > 0.0) {
                if (noHyperGammaPref == 0)
                    hyperGamma = step * 0.3 - 15.0;
                if (step > 50.0f)
                    step = 50.0f;
                Accel(player->heading, step * gTimeScale,
                      &player->pos);
                if (player->engineGlow < 0x20) {
                    player->engineGlow += 4;
                    if (player->engineGlow > 0x20)
                        player->engineGlow = 0x20;
                }
            }
        }
    }

    if (ShipIsInertialess(player)) {
        if (player->accelMag > playerMaxVel.x)
            player->accelMag = playerMaxVel.x;
        if (disabled) {
            player->accelMag = player->accelMag * 0.985;
            if (player->engineGlow > 0)
                player->engineGlow -= 1;
        } else {
            n = (player->accelMag * 32.0f * 0.75
                                  / ShipMaxSpeed(player));
            if (n > 0x18)
                n = 0x18;
            if (player->engineGlow < n)
                player->engineGlow += 1;
            else
                player->engineGlow -= 1;
        }
        AdjustInertialessShipVelocity(player);
    }

    if (player->vel.x > playerMaxVel.x)
        player->vel.x = playerMaxVel.x;
    if (player->vel.x < -playerMaxVel.x)
        player->vel.x = -playerMaxVel.x;
    if (player->vel.y > playerMaxVel.y)
        player->vel.y = playerMaxVel.y;
    if (player->vel.y < -playerMaxVel.y)
        player->vel.y = -playerMaxVel.y;
    player->pos.x = player->vel.x * gTimeScale + player->pos.x;
    player->pos.y = player->vel.y * gTimeScale + player->pos.y;

    if (player->navTargetKind == kDepartureRoutePicked
            && player->targetStellar != -1) {
        canJump = 1;
        for (i = 0; i < kNavSlots; i++) {
            if (gSystems[player->curSystem].navDefault[i] != -1
                    && !(stellarData[gSystems[player->curSystem].navDefault[i]].flags2
                         & kSpobNoJumpMask)) {
                origin.y = 0;
                origin.x = origin.y;
                turnLim = ShipHyperSafeDist(player);
                if (FloatAbs(XYFloatSquaredDist(&origin,
                            &player->pos)) <= turnLim)
                    canJump = 0;
            }
        }
        if (canJump) {
            if (!lastJumpDistOk && player->jumpTimer <= 0.0f
                    && player->fuel >= 100.0) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                statusRedraw[0] = 1;
                if (commMsg[0] > 0) {
                    GetIndString(msg, kGameStrList, 0x2a);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x24);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1e);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1f);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                }
            }
            lastJumpDistOk = 1;
        } else {
            lastJumpDistOk = 0;
        }
    }

    if (player->aiTimer > 0.0f)
        player->aiTimer = player->aiTimer - 1.0f;
    if (player->deathTimer > 0.0f)
        player->deathTimer = player->deathTimer - 1.0f;

    if (player->ionization > 0.0f) {
        player->ionization = player->ionization - ShipDeionizationRate(player) * gTimeScale;
        step = ShipIonizationFactor(player);
        if (step > 0.7)
            step = 0.7f;
        step = ShipMaxSpeed(player) * (1.0f - step);
        if (player->vel.x > step)
            player->vel.x -= gTimeScale * 0.025;
        if (-step > player->vel.x)
            player->vel.x += gTimeScale * 0.025;
        if (player->vel.y > step)
            player->vel.y -= gTimeScale * 0.025;
        if (-step > player->vel.y)
            player->vel.y += gTimeScale * 0.025;
    } else {
        player->ionization = 0;
    }

    if (ShipFuelGenRate(player, &step)) {
        n = player->fuel;
        player->fuel += step * gTimeScale;
        if ((short)player->fuel != n)
            statusRedraw[4] = 1;
    }
    fuelCap = ShipFuelCapacity(player);
    if (fuelCap < player->fuel)
        player->fuel = fuelCap;
    if (player->fuel < 0.0f)
        player->fuel = 0;

    if (QuickKeyCheck(0x72) && cheatsActive) {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt)) {
            for (i = 0; i < kMaxSystems; i++)
                gSystems[i].exploreLevel = 0;
            for (i = 0; i < kMaxNebulae; i++)
                nebulaData[i].explored = 0;
        } else {
            for (i = 0; i < kMaxSystems; i++)
                gSystems[i].exploreLevel = 2;
            for (i = 0; i < kMaxNebulae; i++)
                nebulaData[i].explored = 1;
        }
        HandleStellarSystemVisibility();
        while (KeyCheck(0x72))
            BusyIdle();
    }

    if (QuickKeyCheck(keyPrefs.cheatAmmo) && cheatsActive) {
        PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (gShipTypes[player->type].weapAmmo[i] > 0) {
                if (gWeapons[i].ammoType >= 0
                        && gWeapons[i].ammoType < kMaxWeapTypes
                        && gWeapons[i].guidance != kGuidanceBay)
                    player->weap[gWeapons[i].ammoType].ammo =
                        gShipTypes[player->type].weapAmmo[i];
                else
                    player->weap[i].ammo =
                        gShipTypes[player->type].weapAmmo[i];
            }
        }
        if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
            player->fuel = 0;
        else
            player->fuel = ShipFuelCapacity(player);
        player->shield = ShipShieldCapacity(player);
        player->armor = ShipShieldCapacity(player);
        player->ionization = 0;
        player->ionColor = 0;
        player->deathTimer = -1.0f;
        shipSystems[106] = 1;
        shipSystems[69] = 1;
        shipSystems[60] = 1;
        shipSystems[143] = 1;
        shipSystems[57] = 1;
        ActivateRank((char *)&rankData[19]);
        SystemInfoToShipStats();
        statusRedraw[4] = 1;
        statusRedraw[1] = 1;
        statusRedraw[3] = 1;
        ResetPlayerPrecalcedValues();
    }

    if (cheatsActive && QuickKeyCheck(0x6c) && player->targetShip != -1
            && shipData[player->targetShip].active) {
        if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
            AIMakeShipLeave(&shipData[player->targetShip]);
        else
            AIMakeShipAttackPlayer(&shipData[player->targetShip]);
    }

    escortHit = 0;
    if (QuickKeyCheck(keyPrefs.dismiss)) {
        keyPressedThisFrame = 1;
        if (!ackCommFlag) {
            ackCommFlag = 1;
            if (commCount > 0)
                DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
            if (showFloatingMap > 0)
                showFloatingMap = 0;
            if (escortMenuOnscreen > 0)
                escortHit = 1;
        }
    } else {
        ackCommFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.escortMenu)) {
        if (!escortMenuFlag) {
            escortMenuFlag = 1;
            if (escortMenuOnscreen > 0) {
                escortHit = 1;
            } else {
                flag = 0;
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active && shipData[i].parentShip == 0) {
                        flag = 1;
                        break;
                    }
                }
                if (flag) {
                    PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                    gSelectedQuickKey = -1;
                    GetIndString(msg, kGameStrList, 0x33);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm("\p", 1, 0, 0xc, &rgbWhite, 0, 0);
                    ResetMapZoom();
                    lastEscortMenuTime = ReadTick();
                } else {
                    GetIndString(msg, kGameStrList, 0x33);
                    DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                }
            }
        }
    } else {
        escortMenuFlag = 0;
    }

    if (escortMenuOnscreen > 0) {
        count = 0;
        for (j = 1; j < kMaxShips; j++) {
            if (shipData[j].active && shipData[j].parentShip == 0
                    && !IsDisabled(&shipData[j]) && !IsDying(&shipData[j]))
                count++;
        }
        if (count != lastEscortCount) {
            lastEscortCount = count;
            if (count == 0) {
                lastEscortMenuTime = 0;
                gSelectedQuickKey = -1;
            }
            DrawEscortMenuInFrame();
        }
        if (ReadTick() > lastEscortMenuTime + 0x1e0) {
            escortMenuOnscreen -= 1;
            if (escortMenuOnscreen == 0)
                escortHit = 1;
        }
        if (escortHit) {
            escortMenuOnscreen = 0;
            if (QuickKeyCheck(keyPrefs.escortMenu))
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        } else {
            for (k = 0; k < 5; k++) {
                if (RawKeyCheck(keyPrefs.quickKey[k])) {
                    if (!escortGroupFlag[k]) {
                        escortGroupFlag[k] = 1;
                        if (k > 0) {
                            found = 0;
                            for (j = 1; j < kMaxShips; j++) {
                                if (shipData[j].active
                                        && shipData[j].parentShip == 0
                                        && gShipTypes[shipData[j].type].escortType
                                           == k - 1)
                                    found = 1;
                            }
                        } else {
                            found = 1;
                        }
                        if (found) {
                            gSelectedQuickKey = k - 1;
                            ResetMapZoom();
                            lastEscortMenuTime = ReadTick();
                            PlaySound(beepSnd[2], 1, gSoundVolume,
                                      gSoundVolume);
                        }
                        lastEscortMenuTime = ReadTick();
                    }
                } else {
                    escortGroupFlag[k] = 0;
                }
            }
        }
    } else {
        lastEscortCount = -1;
    }

    for (i = 0; i < 4; i++) {
        if (QuickKeyCheck(keyPrefs.escortOrder[i])) {
            if (!escortCommandFlag[i]) {
                escortCommandFlag[i] = 1;
                if (i == 0) {
                    n = kOrderAttack;
                } else if (i == 1) {
                    n = kOrderDefend;
                } else if (i == 2) {
                    n = kOrderHold;
                } else if (i == 3) {
                    if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                        n = kOrderRegroup;
                    else
                        n = kOrderFormation;
                }
                if (gSelectedQuickKey != -1 && escortMenuOnscreen > 0) {
                    playerEscortOrders[gSelectedQuickKey] = n;
                } else {
                    playerEscortOrders[0] = n;
                    playerEscortOrders[1] = n;
                    playerEscortOrders[2] = n;
                    playerEscortOrders[3] = n;
                }
                if (escortMenuOnscreen > 0) {
                    if (IssueNewEscortCommand(gSelectedQuickKey, n, 0)) {
                        ResetMapZoom();
                        lastEscortMenuTime = ReadTick();
                    }
                } else {
                    IssueNewEscortCommand(-1, n, 0);
                }
            }
        } else {
            escortCommandFlag[i] = 0;
        }
    }

    for (i = 0; i < 4; i++) {
        if (playerEscortOrders[i] == kOrderRegroup) {
            found = 0;
            for (j = 1; j < kMaxShips; j++) {
                if (shipData[j].active && shipData[j].parentShip == 0
                        && gShipTypes[shipData[j].type].escortType == i
                        && shipData[j].aiType == 5
                        && shipData[j].escortMayAttack) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                playerEscortOrders[i] = kOrderFormation;
                if (escortMenuOnscreen > 0)
                    ResetMapZoom();
            }
        }
    }

    groupCount[0] = 0;
    groupCount[1] = 0;
    groupCount[2] = 0;
    groupCount[3] = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active && shipData[i].parentShip == 0
                && shipData[i].escortMayAttack
                && gShipTypes[shipData[i].type].escortType >= 0
                && gShipTypes[shipData[i].type].escortType < 4)
            groupCount[gShipTypes[shipData[i].type].escortType] += 1;
    }
    for (i = 0; i < 4; i++) {
        if (groupCount[i] == 0 && (long)playerEscortOrders[i] != kOrderFormation) {
            playerEscortOrders[i] = kOrderFormation;
            if (escortMenuOnscreen > 0)
                ResetMapZoom();
        }
    }

    if (QuickKeyCheck(keyPrefs.toggleFPS)) {
        if (!framerateFlag) {
            framerateFlag = 1;
            showFPS = !showFPS;
            if (showFPS) {
                lastFPSUpdateTime = 0;
                fpsCounter = 0;
                commCount = 0;
            } else {
                commCount = 1;
            }
        }
    } else {
        framerateFlag = 0;
    }

    if (cheatsActive) {
        if (QuickKeyCheck(0x53)) {
            if (ST_IsSoundPlaying(warpUpSnd[0])) {
                ST_HaltSound(warpUpSnd[0]);
                WarpUpCallback(1, 0, 0);
            } else if (ST_IsSoundPlaying(warpUpSnd[1])) {
                ST_HaltSound(warpUpSnd[1]);
                WarpUpCallback(1, 0, 0);
            }
        }

        if (QuickKeyCheck(keyPrefs.cheatSpawn) || QuickKeyCheck(0x40)) {
            if (!spawnShipFlag) {
                spawnShipFlag = 1;
                if (!QuickKeyCheck(kScanLAlt) && !QuickKeyCheck(kScanRAlt)) {
                    if (QuickKeyCheck(kScanLCtrl) || QuickKeyCheck(kScanRCtrl))
                        player->targetShip = SpawnPerson(player->curSystem, 1, 0x201);
                    else
                        player->targetShip = HyperShipSpawn(player->curSystem);
                } else if (QuickKeyCheck(kScanLCtrl) || QuickKeyCheck(kScanRCtrl)) {
                    HyperSpawnFleet(player->curSystem, 6, -1, 1);
                } else if (QuickKeyCheck(0x30)) {
                    HyperSpawnFleet(player->curSystem, 0x15, -1, 1);
                } else {
                    HyperSpawnFleet(player->curSystem, 1, -1, 1);
                }
                statusRedraw[6] = 1;
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            }
        } else {
            spawnShipFlag = 0;
        }

        if ((QuickKeyCheck(keyPrefs.cheatKill) || QuickKeyCheck(0x41)
                || QuickKeyCheck(0x45)) && player->targetShip != -1) {
            shipData[player->targetShip].active = 0;
            statusRedraw[6] = 1;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }

        if (QuickKeyCheck(keyPrefs.cheatArmor) || QuickKeyCheck(0x42)) {
            if (player->targetShip != -1) {
                if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt)) {
                    if (IsDisabled(&shipData[player->targetShip])) {
                        shipData[player->targetShip].armor =
                            gShipTypes[shipData[player->targetShip].type].armor;
                        shipData[player->targetShip].shield = 0;
                    }
                    shipData[player->targetShip].aiState = kAIStateHiJack;
                    shipData[player->targetShip].aiSubstate = kSubLoJack;
                    shipData[player->targetShip].targetShip = -1;
                    shipData[player->targetShip].targetStellar = -1;
                    shipData[player->targetShip].govt = -1;
                    shipData[player->targetShip].parentShip = 0;
                    shipData[player->targetShip].aiType = 6;
                    shipData[player->targetShip].sellOnLanding = 0;
                    shipData[player->targetShip].upgradeOnLanding = 0;
                    shipData[player->targetShip].fuelRegenState = -1;
                    if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                        shipData[player->targetShip].hired = 1;
                    else
                        shipData[player->targetShip].hired = 0;
                    shipData[player->targetShip].armor =
                        ShipArmorCapacity(&shipData[player->targetShip]);
                    AISetup(&shipData[player->targetShip]);
                    AIShipCapturedByPlayer(&shipData[player->targetShip]);
                } else {
                    if (shipData[player->targetShip].parentShip == 0) {
                        if (shipData[player->targetShip].aiType == 6
                                && gShipTypes[shipData[player->targetShip].type]
                                   .inherentAI <= 2) {
                            DestroyPartialFleetCargo(player->targetShip);
                            statusRedraw[2] = 1;
                        }
                        if (shipData[player->targetShip].aiType == 5)
                            shipData[player->targetShip].formerEscort = 0;
                        else if (shipData[player->targetShip].specialMission == -1)
                            shipData[player->targetShip].formerEscort = 1;
                        else
                            shipData[player->targetShip].formerEscort = -1;
                        shipData[player->targetShip].parentShip = -1;
                        shipData[player->targetShip].aiTimer = 500.0f;
                    }
                    DamageShip(&shipData[player->targetShip], NULL, 0, 0x7d00, 0x7d00,
                               -1, 0, 0, 1, 0, 1, 0);
                }
                statusRedraw[6] = 1;
            } else {
                while (!IsDisabled(player) && !IsDying(player))
                    player->armor = player->armor - 1.0f;
                player->armor = 1.0f;
                playerRepairInhibit = 300.0f;
                statusRedraw[1] = 1;
            }
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }

        if (QuickKeyCheck(keyPrefs.cheatDamage) || QuickKeyCheck(0x43)) {
            if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt)) {
                player->armor = 1.0f;
                statusRedraw[1] = 1;
            } else if (player->targetShip != -1
                    && shipData[player->targetShip].armor > 0.0f) {
                if ((QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
                        && !ShipIsDerelict(&shipData[player->targetShip])) {
                    SlapWithPenalty(shipData->curSystem,
                                    shipData[player->targetShip].govt, 3,
                                    shipData[player->targetShip].specialMission);
                    AddToPlayerKills(
                        gShipTypes[shipData[player->targetShip].type].strength);
                }
                if (shipData[player->targetShip].aiType == 6
                        && gShipTypes[shipData[player->targetShip].type]
                           .inherentAI <= 2) {
                    DestroyPartialFleetCargo(player->targetShip);
                    statusRedraw[2] = 1;
                    DrawStatusCargo();
                }
                shipData[player->targetShip].armor = -1.0f;
                statusRedraw[6] = 1;
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            }
        }

        if (QuickKeyCheck(keyPrefs.cheatShipNext) || QuickKeyCheck(keyPrefs.cheatShipPrev)) {
            if (QuickKeyCheck(keyPrefs.cheatShipNext)) {
                do {
                    player->type += 1;
                    if (player->type >= kShipTypeSpecial)
                        player->type = 0;
                } while (gShipTypes[player->type].mass < 0);
            } else {
                do {
                    player->type -= 1;
                    if (player->type < 0)
                        player->type = kShipTypeSpecial - 1;
                } while (gShipTypes[player->type].mass < 0);
            }
            while (KeyCheck(keyPrefs.cheatShipNext) || KeyCheck(keyPrefs.cheatShipPrev))
                ;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            player->shield = ShipShieldCapacity(player);
            player->armor = ShipArmorCapacity(player);
            player->fuel = gShipTypes[player->type].fuel;
            player->foldSet =
                gShipTypes[player->type].baseSets - 1;
            player->foldDir = 0;
            player->frameTimer = 0;
            statusRedraw[4] = statusRedraw[1] = statusRedraw[3] =
                statusRedraw[2] = 1;
            for (i = 0; i < kMaxOutfits; i++)
                shipSystems[i] = 0;
            for (i = 0; i < kMaxWeapTypes; i++) {
                player->weap[i].count = 0;
                player->weap[i].ammo = 0;
            }
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (gShipTypes[player->type].weapCount[i] > 0)
                    player->weap[i].count =
                        gShipTypes[player->type].weapCount[i];
                if (gShipTypes[player->type].weapAmmo[i] > 0) {
                    if (gWeapons[i].ammoType >= 0
                            && gWeapons[i].ammoType < kMaxWeapTypes
                            && gWeapons[i].guidance != kGuidanceBay)
                        player->weap[gWeapons[i].ammoType].ammo =
                            gShipTypes[player->type].weapAmmo[i];
                    else
                        player->weap[i].ammo =
                            gShipTypes[player->type].weapAmmo[i];
                }
            }
            for (i = 0; i < 8; i++) {
                if (gShipTypes[player->type].defOutfCount[i] > 0)
                    shipSystems[gShipTypes[player->type].defOutfID[i]] +=
                        gShipTypes[player->type].defOutfCount[i];
            }
            ShipStatsToSystemInfo();
            LoadIntfResource();
            player->selectedWeapon = -1;
            ResetPlayerPrecalcedValues();
            player->armor = ShipArmorCapacity(player);
            player->shield = ShipShieldCapacity(player);
        }

        if ((QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
                && QuickKeyCheck(keyPrefs.cheatCredits)) {
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            if (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt)) {
                if (player->credits >= 10000)
                    player->credits -= 10000;
                else
                    player->credits = 0;
            } else {
                player->credits += 50000;
            }
            statusRedraw[2] = 1;
        }

        if (QuickKeyCheck(0x6d)) {
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            cheatsActive = 0;
        }
    }

    if (player->credits < 0) {
        player->credits = 0x5fffffff;
        statusRedraw[2] = 1;
    }
    if (player->credits > 0x5fffffff) {
        player->credits = 0x5fffffff;
        statusRedraw[2] = 1;
    }
    if (player->hasFormation)
        AICalcFormationPositions(player, 0);
    if (player->deathTimer > 1.0f) {
        if (!ST_IsSoundPlaying(klaxxonSnd))
            PlaySound(klaxxonSnd, 0xf, gSoundVolume, gSoundVolume);
    }

    if (disabled || dying) {
        flag = 0;
        if (dying) {
            if (gShipTypes[player->type].deathDelay * 0.5 >= player->deathTimer
                    || player->deathTimer <= 30.0f) {
                if (HasAutoEject(player))
                    flag = 1;
            }
        }
        if (((QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                && QuickKeyCheck(keyPrefs.eject)) || flag) {
            keyPressedThisFrame = 1;
            if (HasEscapePod(player)) {
                n = GenericRandomShipSpawn(player->curSystem, 0);
                if (n != -1) {
                shipData[n].aiType = -1;
                CopyFloatPoint(&shipData[n].pos, &player->pos);
                CopyFloatPoint(&shipData[n].vel, &player->vel);
                shipData[n].heading = player->heading;
                shipData[n].type = player->type;
                shipData[n].deathTimer = player->deathTimer;
                shipData[n].ionization = 0;
                shipData[n].ionColor = 0;
                shipData[n].lightLevel = 0;
                shipData[n].blinkCount = 0;
                shipData[n].altSet = 0;
                shipData[n].animSet = 0;
                shipData[n].bankDir = 0;
                shipData[n].animTimer = 0;
                shipData[n].shield = player->shield;
                shipData[n].armor = player->armor;
                shipData[n].boarded = 1;
                SWCloneFramesFromMaster((SpritePtr)shipData[n].sprite,
                                        masterShipSprite[shipData[n].type]);
                SWSetCurrentFrameIndex((SpritePtr)shipData[n].sprite, 0);
                player->deathTimer = -1.0f;
                player->targetShip = -1;
                player->targetStellar = -1;
                player->selectedWeapon = -1;
                statusRedraw[4] = statusRedraw[0] = statusRedraw[6] =
                    statusRedraw[2] = statusRedraw[3] = 1;
                crashlog_mission_bit_set("retiring",
                    (char *)gShipTypes[shipData->type].name, 1,
                    shipData->type + 128,
                    gShipTypes[shipData->type].onRetire);
                EvalMissionBitSetString(gShipTypes[shipData->type].onRetire);
                if (!HasEscapeShip(player)) {
                    player->type = kShipTypeSpecial;
                    player->shield = gShipTypes[player->type].shield;
                    player->armor = gShipTypes[player->type].armor;
                    player->escapePods = 0x15e;
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active && shipData[i].targetShip == 0)
                            shipData[i].targetShip = -1;
                    }
                } else {
                    player->type = EscapeShipType(player);
                    player->shield = ((Rand(0x1e) + 0x32)
                        * gShipTypes[player->type].shield) * 0.01;
                    player->armor = ((Rand(0x1e) + 0x32)
                        * gShipTypes[player->type].armor) * 0.01;
                    player->fuel = ((Rand(0x1e) + 0x32)
                        * gShipTypes[player->type].fuel) * 0.01;
                    player->escapePods = -1;
                    strcpy((char *)msg, "");
                    AppendIndString((char *)msg, kGameStrList, 0x34);
                    strcat((char *)msg, " ");
                    pstrcpy(name, gShipLongNames[player->type]);
                    PtoCstr(name);
                    strcat((char *)msg, (char *)name);
                    strcat((char *)msg, ".");
                    CtoPstr((char *)msg);
                    DisplayComm(msg, 0x200, 0, 0xc, &rgbWhite, 0, 0);
                }
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active) {
                        if (IsThreatToPlayer(&shipData[i])) {
                            if (shipData[i].targetShip == 0)
                                shipData[i].targetShip = n;
                            if (shipData[i].targetStellar == 0)
                                shipData[i].targetStellar = n;
                        } else if (shipData[i].parentShip == 0
                                && player->type == kShipTypeSpecial) {
                            shipData[i].parentShip = -1;
                        }
                    }
                }
                SWCloneFramesFromMaster((SpritePtr)player->sprite,
                                        masterShipSprite[player->type]);
                SWSetCurrentFrameIndex((SpritePtr)player->sprite, 0);
                player->vel.x = player->vel.y;
                Accel(player->heading, ShipMaxSpeed(player),
                      &player->vel);
                for (i = 0; i < kMaxOutfits; i++) {
                    if (!gOutfits[i].persistent)
                        shipSystems[i] = 0;
                }
                for (i = 0; i < kMaxWeapTypes; i++) {
                    player->weap[i].count = 0;
                    player->weap[i].ammo = 0;
                }
                for (i = 0; i < kMaxWeapTypes; i++) {
                    player->weap[i].count =
                        gShipTypes[player->type].weapCount[i];
                    if (gShipTypes[player->type].weapAmmo[i] > 0) {
                        if (gWeapons[i].ammoType >= 0
                                && gWeapons[i].ammoType < kMaxWeapTypes
                                && gWeapons[i].guidance != kGuidanceBay)
                            player->weap[gWeapons[i].ammoType].ammo =
                                gShipTypes[player->type].weapAmmo[i];
                        else
                            player->weap[i].ammo =
                                gShipTypes[player->type].weapAmmo[i];
                    }
                    player->weap[i].reloadTimer = 0;
                    player->weap[i].burstCount = 0;
                }
                ShipStatsToSystemInfo();
                for (i = 0; i < 8; i++) {
                    if (gShipTypes[player->type].defOutfCount[i] > 0)
                        shipSystems[
                            gShipTypes[player->type].defOutfID[i]] +=
                            gShipTypes[player->type].defOutfCount[i];
                }
                PlaySound(escapePodSnd, 0x32, gSoundVolume, gSoundVolume);
                if (ST_IsSoundPlaying(warpUpSnd[0]))
                    ST_HaltSound(warpUpSnd[0]);
                if (ST_IsSoundPlaying(warpUpSnd[1]))
                    ST_HaltSound(warpUpSnd[1]);
                player->jumpTimer = 0;
                player->deathTimer = -1.0f;
                }
            }
        }
    }

    if (abOn) {
        if (!gravitySourcesPresent) {
            playerMaxVel.x = ShipMaxSpeed(player) * 1.8;
            playerMaxVel.y = ShipMaxSpeed(player) * 1.8;
        }
        if (ShipIsInertialess(player)) {
            player->accelMag = ShipAccelRate(player) * 2.75
                               + player->accelMag;
            if (ShipMaxSpeed(player) * 1.8 < player->accelMag)
                player->accelMag = ShipMaxSpeed(player) * 1.8;
        } else {
            pos.x = pos.y = 0;
            dest.x = dest.y = 0;
            Accel(player->heading, ShipAccelRate(player) * 2.75, &pos);
            Accel(player->heading, ShipMaxSpeed(player) * 1.8, &dest);
            if ((pos.y > 0.0f && player->vel.y < dest.y)
                    || (pos.y <= 0.0f && player->vel.y > dest.y))
                player->vel.y = player->vel.y + pos.y;
            else
                player->vel.y = player->vel.y * 0.99;
            if ((pos.x > 0.0f && player->vel.x < dest.x)
                    || (pos.x <= 0.0f && player->vel.x > dest.x))
                player->vel.x = player->vel.x + pos.x;
            else
                player->vel.x = player->vel.x * 0.99;
        }
        player->fuel = player->fuel - ShipAfterburnerFuelUsage(player) * gTimeScale;
        if (player->fuel < 0.0)
            player->fuel = 0;
        if (frameCounter % 4 == 0)
            statusRedraw[4] = 1;
        if (player->engineGlow < 0x20) {
            player->engineGlow += 1;
            if (!QuickKeyCheck(keyPrefs.accelerate))
                player->engineGlow += 1;
        }
        if (player->engineGlow > 0x20)
            player->engineGlow -= 1;
    } else {
        step = ShipMaxSpeed(player);
        turnLim = ShipAccelRate(player) * 0.4;
        if (playerMaxVel.x > step)
            playerMaxVel.x = playerMaxVel.x - turnLim;
        if (playerMaxVel.y > step)
            playerMaxVel.y = playerMaxVel.y - turnLim;
        if (playerMaxVel.x < step)
            playerMaxVel.x = step;
        if (playerMaxVel.y < step)
            playerMaxVel.y = step;
        if (player->engineGlow > 0x18)
            player->engineGlow -= 1;
    }

    if ((QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
            && QuickKeyCheck(keyPrefs.jettison)) {
        keyPressedThisFrame = 1;
        if (QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
            JettisonCargo(0);
        else
            JettisonCargo(1);
        statusRedraw[2] = 1;
        ResetPlayerPrecalcedValues();
    }

    if ((QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
            && QuickKeyCheck(keyPrefs.selfDestruct) && destructCount < 0.0f
            && !dying) {
        keyPressedThisFrame = 1;
        PlaySound(beepSnd[3], 0xf, gSoundVolume, gSoundVolume);
        strcpy((char *)msg, "");
        AppendIndString((char *)msg, kGameStrList, 0x181);
        strcat((char *)msg, " ");
        AppendIndString((char *)msg, kGameStrList, 0x182);
        CtoPstr((char *)msg);
        DisplayComm(msg, 0x28, 0, 0xc, &rgbWhite, 0, 0);
        destructCount = 150.0f;
    }

    if (destructCount > 0.0f) {
        if ((QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                && QuickKeyCheck(keyPrefs.selfDestruct)) {
            destructing = 1;
            if (destructCount >= 1.0)
                destructCount = destructCount - gTimeScale;
            if ((short)destructCount % 30 == 0 && destructCount <= 120.0f) {
                extern char *strcpy(char *, const char *);
                strcpy((char *)msg, "");
                AppendIndString((char *)msg, kGameStrList, 0x181);
                strcat((char *)msg, " ");
                AppendIndString((char *)msg, kGameStrList, 0x18b);
                strcat((char *)msg, " ");
                NumToString(destructCount / 30.0f, name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                AppendIndString((char *)msg, kGameStrList, 0x185);
                strcat((char *)msg, ".");
                CtoPstr((char *)msg);
                PlaySound(beepSnd[2], 0xf, gSoundVolume,
                          gSoundVolume);
                DisplayComm(msg, 0x46, 0, 0xc, &rgbWhite, 0, 0);
            }
        } else {
            extern char *strcpy(char *, const char *);
            PlaySound(beepSnd[1], 0xf, gSoundVolume, gSoundVolume);
            strcpy((char *)msg, "");
            AppendIndString((char *)msg, kGameStrList, 0x181);
            strcat((char *)msg, " ");
            AppendIndString((char *)msg, kGameStrList, 0x183);
            CtoPstr((char *)msg);
            DisplayComm(msg, 0x28, 0, 0xc, &rgbWhite, 0, 0);
            destructing = 1;
            destructCount = -1.0f;
        }
    } else {
        destructing = 0;
    }

    if (destructCount <= 1.0f && destructing) {
        if ((QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
                && QuickKeyCheck(keyPrefs.selfDestruct)) {
            keyPressedThisFrame = 1;
            PlaySound(beepSnd[1], 0xf, gSoundVolume, gSoundVolume);
            GetIndString(msg, kGameStrList, 0x186);
            DisplayComm(msg, 0x28, 0, 0xc, &rgbWhite, 0, 0);
            player->shield = 0;
            player->armor = -1.0f;
            player->targetShip = -1;
            statusRedraw[3] = 1;
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active
                        && AIShipIsDockedWithShip(&shipData[i], player))
                    shipData[i].armor = -1.0f;
            }
        }
        destructCount = -1.0f;
    }

    if (QuickKeyCheck(keyPrefs.playerInfo)) {
        keyPressedThisFrame = 1;
        if (!playerInfoFlag) {
            playerInfoFlag = 1;
            if (player->jumpTimer <= 0.0f && player->escapePods <= 0
                    && frameCounter >= 0 && !dying) {
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                DoPlayerInfoDialog();
                gTimeScale = 0;
                RedrawStatus(1);
                RedrawComm(0);
                SetAllKeyCmdFlags();
            } else {
                PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        playerInfoFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.hail)) {
        keyPressedThisFrame = 1;
        if (!commTryFlag) {
            commTryFlag = 1;
            HandlePlayerCommunication();
        }
    } else {
        commTryFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.missions) && player->jumpTimer <= 0.0f
            && player->escapePods <= 0 && !dying && player->deathTimer <= 0.0f
            && frameCounter >= 0) {
        keyPressedThisFrame = 1;
        if (!missionInfoFlag) {
            missionInfoFlag = 1;
            n = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active
                        && !(currentMissionObjectives[i].flags & kMisnHidden))
                    n++;
            }
            if (n > 0) {
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                DoMissionInfoDialog();
                gTimeScale = 0;
                RedrawStatus(1);
                RedrawComm(0);
                SetAllKeyCmdFlags();
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                GetIndString(msg, kGameStrList, 0x162);
                DisplayComm(msg, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
            }
        }
    } else {
        missionInfoFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.cloak) && player->deathTimer <= 0.0f
            && player->jumpTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        if (!cloakFlag) {
            cloakFlag = 1;
            if (ShipCanCloak(player)) {
                if (player->cloak > 0.0f || player->cloakDir > 0)
                    DoShipUncloak(player);
                else
                    DoShipCloak(player);
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        cloakFlag = 0;
    }

    if (ShipIsCloaked(player)) {
        if (!ShipCanCloak(player) || disabled)
            DoShipUncloak(player);
        n = CloakFuelUsage(player);
        if (n > 0) {
            player->fuel = player->fuel - n * 0.03333 * gTimeScale;
            if (frameCounter % 10 == 0)
                statusRedraw[4] = 1;
            if (player->fuel <= 0.0f)
                player->fuel = 0;
        }
        if (CloakZeroesShields(player)) {
            player->shield = 0;
            if (frameCounter % 10 == 0)
                statusRedraw[1] = 1;
        } else {
            n = CloakShieldUsage(player);
            if (n > 0 && n <= player->shield) {
                player->shield -= n * 0.03333 * gTimeScale;
                if (frameCounter % 10 == 0)
                    statusRedraw[1] = 1;
                if (player->shield <= 0.0f)
                    player->shield = 0;
            }
        }
    }

    if (playerHasTribbles && frameCounter % 250 == 0) {
        n = TotalFleetHolds() - TotalCargo(player);
        for (i = 0; i < kMaxJunks; i++) {
            if ((junkData[i].flags & kJunkGrows) && junkData[i].held > 0
                    && n > 0) {
                junkData[i].held += 1;
                statusRedraw[2] = 1;
            }
        }
    }
    if (playerHasPerishableJunk && frameCounter % 250 == 0) {
        for (i = 0; i < kMaxJunks; i++) {
            if ((junkData[i].flags & kJunkPerishes) && junkData[i].held > 0
                    && n > 0) {
                junkData[i].held -= 1;
                statusRedraw[2] = 1;
            }
        }
    }

    if (newbieFlag < 2 && frameCounter % 60 == 0) {
        origin.y = 0;
        origin.x = origin.y;
        dist = FloatAbs(XYFloatSquaredDist(&player->pos,
                                         &origin));
        if (newbieFlag == -3 && newbieString[0][0] && newbieString[1][0]) {
            n = FindNearestStellar(shipData);
            if (n != -1) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                if (stellarData[n].flags & kSpobDestroyed)
                    pstrcpy(msg, newbieString[0]);
                else
                    pstrcpy(msg, newbieString[1]);
                PtoCstr(msg);
                strcat((char *)msg, " ");
                pstrcpy(name, stellarData[n].name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                pstrcpy(name, newbieString[2]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.land);
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[3]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                CtoPstr((char *)msg);
                DisplayComm(msg, 0x200, 0, 0xc, &rgbWhite, 0, 0);
            }
            newbieFlag = -2;
        }
        if (newbieFlag == -1 && newbieString[4][0]) {
            if (FindNearestStellar(shipData) != -1) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                pstrcpy(msg, newbieString[4]);
                PtoCstr(msg);
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.map);
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[5]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.jump);
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[6]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                CtoPstr((char *)msg);
                DisplayComm(msg, 0x200, 0, 0xc, &rgbWhite, 0, 0);
            }
            newbieFlag = 0;
        }
        if (dist > 2000000.0 && newbieFlag <= 0) {
            if (!disabled && newbieString[8][0]) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                DisplayComm(newbieString[8], 0x200, 0, 0xc, &rgbWhite, 0, 0);
                newbieFlag = 1;
            }
        } else if (dist > 10000000.0 && newbieFlag == 1 && !disabled
                && newbieString[9][0]) {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            DisplayComm(newbieString[9], 0x200, 0, 0xc, &rgbWhite, 0, 0);
            newbieFlag = 2;
        }
    }

    if (player->credits < 0) {
        player->credits = 0;
        statusRedraw[2] = 1;
    }
    if (player->fuel < 0.0f) {
        player->fuel = 0;
        statusRedraw[4] = 1;
    }
    n = FindFirstCoLocatedSystem(player->curSystem);
    if (gSystems[n].exploreLevel < 1)
        gSystems[n].exploreLevel = 1;
    if (gSystems[player->curSystem].exploreLevel < 1)
        gSystems[player->curSystem].exploreLevel = 1;
    if (player->heading != heading)
        playerTurnedThisFrame = 1;

    if (showFloatingMap > 0 && !keyComboBlocked
            && !QuickKeyCheck(kScanLShift) && !QuickKeyCheck(kScanRShift)
            && !QuickKeyCheck(kScanLCtrl) && !QuickKeyCheck(kScanRCtrl)
            && !QuickKeyCheck(kScanLAlt) && !QuickKeyCheck(kScanRAlt)) {
        if ((QuickKeyCheck(0xc) || QuickKeyCheck(0x4a)) && !mapZoomFlag
                && floatingMapZoom < 2.0) {
            floatingMapZoom = floatingMapZoom * 1.333333;
            gJumpStartTick = ReadTick();
            DrawMapInFloatingFrame();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }
        if ((QuickKeyCheck(0xd) || QuickKeyCheck(0x4e)) && !mapZoomFlag
                && floatingMapZoom > 0.5) {
            floatingMapZoom = floatingMapZoom * 0.75;
            gJumpStartTick = ReadTick();
            DrawMapInFloatingFrame();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }
        if (QuickKeyCheck(0xd) || QuickKeyCheck(0xc) || QuickKeyCheck(0x4a)
                || QuickKeyCheck(0x4e))
            mapZoomFlag = 1;
        else
            mapZoomFlag = 0;
    }
}

#define kHailKeyA           0x38   /* either modifier redirects the hail to the stellar */
#define kHailKeyB           0x6f

#define kRouteDeparture     2      /* navTargetKind: a departure route has been picked */

#define kSpobNoLanding      0x20   /* spöb Flags: cannot be landed on */
#define kSpobNoComm         0x3000 /* spöb Flags2: comm is refused */

#define kGovtNoComm         0x400  /* gövt Flags */

#define kPersNoComm         0x200  /* përs Flags: never offers over comm */
#define kPersOneShot        0x100  /* përs Flags: retire the person once used */
#define kPersLeavesSystem   0x800  /* përs Flags: the ship departs after the offer */
#define kPersSwapsShip      0x40   /* përs Flags: the offer swaps the person's ship */

#define kCommRefused        0x35
#define kCommDisabled       0x36

#define kCommTextWidth      0xf0
#define kCommTextLines      0xc

/*
 * The four mïsn fields this decision reads.  ONE typed window rather than
 * four per-field accessor macros: the macro form materialises the address
 * with its own add where the exe folds base and scaled index into the one
 * instruction (`cmp word ptr [R + R + 6]` against `add R,R` then
 * `cmp word ptr [R + 6]`).
 */


                                                         /* 0x0041cf40 */

/* HandlePlayerCommunication (mac _HandlePlayerCommunication). STATE: MATCH 100.0%, exe 504 / obj 504 instrs, byte-identical (masked).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00454910
void HandlePlayerCommunication(void)
{
    short i;
    ShipObj *me;
    short pers;
    short mission;
    short newShip;
    Str255 buf;
    short found;
    char canHail;
    char canLand;

    me = shipData;

    if (ShipIsCloaked(me) == 0 && IsDying(me) == 0 &&
            me->jumpTimer <= 0.0f) {
        if (me->targetShip == -1 || QuickKeyCheck(kHailKeyA) != 0 ||
                QuickKeyCheck(kHailKeyB) != 0) {
            if (me->navTargetKind == kRouteDeparture && me->targetStellar != -1) {
                canLand = (stellarData[me->targetStellar].flags
                           & kSpobNoLanding) == 0;
                if (StellarIsLandable(&stellarData[me->targetStellar]) == 0)
                    canLand = 0;
                if ((stellarData[me->targetStellar].flags2 & kSpobNoComm) != 0)
                    canLand = 0;
                if (stellarData[me->targetStellar].loaded != 0 && canLand) {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoPlanetCommDialog(me->targetStellar);
                    SetAllKeyCmdFlags();
                } else {
                    PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
                    GetIndString(buf, kGameStrList, kCommRefused);
                    DisplayComm(buf, kCommTextWidth, 0, kCommTextLines,
                                &rgbWhite, 0, 0);
                }
            } else {
                PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
            }
        } else if (shipData[me->targetShip].jumpTimer <= 0.0f) {
            canHail = 1;
            if (shipData[me->targetShip].parentShip != 0 ||
                    shipData[me->targetShip].specialMission != -1) {
                if (shipData[me->targetShip].govt != -1 &&
                        (govtData[shipData[me->targetShip].govt].flags
                            & kGovtNoComm) != 0)
                    canHail = 0;
                if (gShipTypes[shipData[me->targetShip].type].inherentGovtB != -1 &&
                        (govtData[gShipTypes[shipData[me->targetShip].type]
                            .inherentGovtB].flags & kGovtNoComm) != 0)
                    canHail = 0;
            }
            if (shipData[me->targetShip].type == kShipTypeEscapePod)
                canHail = 0;
            if (IsDisabled(&shipData[me->targetShip]) != 0)
                canHail = 0;
            pers = shipData[me->targetShip].persType;
            if (pers == kPersCaptainHector)
                canHail = 0;
            if (canHail) {
                if (pers != -1) {
                  if (personData[pers].linkMission != -1) {
                    if ((personData[pers].flags & kPersNoComm) == 0) {
                    offeringFromShip = 1;
                    offeringShipIndex = me->targetShip;
                    if (IsMissionAvailable(
                            personData[shipData[me->targetShip].persType].linkMission, 1) != 0) {
                        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                        HideCursorSprite();
                        RefreshSAT();
                        ShowTheCursor();
                        mission = OfferOneMission(
                            personData[shipData[me->targetShip].persType].linkMission);
                        SetAllKeyCmdFlags();
                        RefreshSAT();
                        if (mission == 1) {
                            if ((personData[shipData[me->targetShip].persType].flags
                                    & kPersOneShot) != 0)
                                personData[shipData[me->targetShip].persType].loaded = 0;
                            if ((personData[shipData[me->targetShip].persType].flags
                                    & kPersLeavesSystem) != 0)
                                AIMakeShipLeave(&shipData[me->targetShip]);
                            found = -1;
                            for (i = 0; i < kMaxActiveMissions; i++) {
                                if (currentMissionObjectives[i].active != 0 &&
                                        currentMissions[i].misnIndex ==
                                            personData[shipData[me->targetShip]
                                                .persType].linkMission) {
                                    found = i;
                                    break;
                                }
                            }
                            if (found >= 0 &&
                                    (personData[shipData[me->targetShip].persType].flags
                                        & kPersSwapsShip) != 0 &&
                                    currentMissions[found].shipCount == 1) {
                                newShip = MissionDudeSpawn(currentMissions[found].shipDude,
                                                       shipData[me->targetShip].type,
                                                       shipData->curSystem, found);
                                if (newShip != -1) {
                                    shipData[newShip].pos.x =
                                        shipData[me->targetShip].pos.x;
                                    shipData[newShip].pos.y =
                                        shipData[me->targetShip].pos.y;
                                    shipData[newShip].vel.x =
                                        shipData[me->targetShip].vel.x;
                                    shipData[newShip].vel.y =
                                        shipData[me->targetShip].vel.y;
                                    shipData[newShip].heading =
                                        shipData[me->targetShip].heading;
                                    if (currentMissions[found].shipGoal == 3)
                                        AIMakeEscortFlyInForm(&shipData[newShip]);
                                    shipData[me->targetShip].active = 0;
                                    me->targetShip = newShip;
                                    weapTargetExpand = 0.0F;
                                    statusRedraw[6] = 1;
                                }
                            }
                        }
                    } else {
                        HideCursorSprite();
                        RefreshSAT();
                        ShowTheCursor();
                        DoCommDialog(me->targetShip);
                        SetAllKeyCmdFlags();
                    }
                    offeringFromShip = 0;
                    offeringShipIndex = me->targetShip;
                    } else {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetAllKeyCmdFlags();
                    }
                  } else {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetAllKeyCmdFlags();
                  }
                } else {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetAllKeyCmdFlags();
                }
                RedrawStatus(1);
                RedrawComm(0);
            } else {
                PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
                GetIndString(buf, kGameStrList, kCommRefused);
                DisplayComm(buf, kCommTextWidth, 0, kCommTextLines,
                            &rgbWhite, 0, 0);
            }
        } else if (IsDisabled(&shipData[me->targetShip]) != 0) {
            PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, kCommRefused);
            DisplayComm(buf, kCommTextWidth, 0, kCommTextLines, &rgbWhite, 0, 0);
        } else {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, kCommDisabled);
            DisplayComm(buf, kCommTextWidth, 0, kCommTextLines, &rgbWhite, 0, 0);
        }
    } else {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    }
}

/* abs is inlined only when <stdlib.h> declares it; a local extern emits a call. */


/* gShipTypes[].shanFlags bits gating the "weapon is locked out" early return */
#define kShipTypeA24Bit7    0x0080
/* gWeapons[].flags2 */
#define kWeapU22NoAmmoUse   0x0200
/* gWeapons[].flags */
#define kWeapFlagSoundOnce  0x0010
#define kWeapFlagSoundNear  0x0002
#define kWeapFlagKeepReload 0x0040

/* FirePlayerWeapon (mac _FirePlayerWeapon). STATE: MATCH 100.0%, exe 829 / obj 829,
   byte-identical (3257 bytes), relocregcheck 0 of 137. OPEN: none. */
// FUNCTION: EVNOVA 0x00455150
void FirePlayerWeapon(short w)
{
    unsigned long long key;
    char keyed[4];
    short i;
    short shot;
    short n;
    short fired;
    char didFire;
    char soundOK;
    short angle;
    short diff;
    short ammoType;

    key = *(unsigned long long *)((char *)shipData + 0x4b7a0);
    keyed[0] = ((key ^ 0xf6100bf185059c3dULL) & 0x0fffffffffffffffULL)
                   != 0x0ec22bc220ac327aULL;
    keyed[1] = ((key ^ 0xa298c3a6ef99fff6ULL) & 0x0fffffffffffffffULL)
                   != 0x05ac9ee5e4ba1ec7ULL;
    keyed[2] = ((key ^ 0xa63b2518c45f7e5bULL) & 0x0fffffffffffffffULL)
                   != 0x010d795bcf7c9f6aULL;
    keyed[3] = ((key ^ 0xe38ddae6a46ec9deULL) & 0x0fffffffffffffffULL)
                   != 0x0659dbdcb54d0094ULL;
    for (i = 0; i < 4; i++) {
        if (keyed[i] == 0)
            return;
    }

    if (ShipIsCloaked(shipData)) {
        if ((gWeapons[w].flags2 & kWeapFireWhileCloaked) == 0)
            return;
    }

    shipData->fireTick = ReadTick();

    if ((gShipTypes[shipData->type].shanFlags & kShipTypeA24Bit1)
            && (gShipTypes[shipData->type].shanFlags & kShipTypeA24Bit7)
            && shipData->foldSet > 0) {
        shipData->foldDir = -1;
        return;
    }

    if (shipData->weap[w].reloadTimer <= 0.0f && 0.0f == shipData->jumpTimer) {
        fired = 0;
        n = WeaponMaxSimultShots(shipData, w);
        for (shot = 0; shot < n; shot++) {
            didFire = 0;
            if (!WeaponHasAmmo(shipData, w))
                continue;

            if (gWeapons[w].guidance == kGuidanceUnguided
                    || gWeapons[w].guidance == kGuidanceHoming
                    || gWeapons[w].guidance == kGuidanceFreefall
                    || gWeapons[w].guidance == kGuidanceRocket) {
                SpawnShot(0, shipData->targetShip, w, 0);
                fired = fired + 1;
                didFire = 1;
            }
            if (gWeapons[w].guidance == kGuidanceBeam) {
                SpawnBeam(0, shipData->targetShip, w, shipData->heading, -1, -1);
                fired++;
                didFire = 1;
            }
            if (gWeapons[w].guidance == kGuidanceTurretBeam
                    && shipData->targetShip != -1) {
                angle = CalcInterceptAngle(
                    &shipData->pos,
                    &shipData[shipData->targetShip].pos);
                if (!TurretBlindSpot(shipData, angle, w)) {
                    SpawnBeam(0, shipData->targetShip, w, angle, -1, -1);
                    fired++;
                    didFire = 1;
                }
            }
            if (gWeapons[w].guidance == kGuidanceTurret
                    && shipData->targetShip != -1) {
                if (!TurretBlindSpot(shipData, CalcInterceptAngle(
                        &shipData->pos,
                        &shipData[shipData->targetShip].pos), w)) {
                    SpawnShot(0, shipData->targetShip, w, 0);
                    fired++;
                    didFire = 1;
                }
            }
            if (gWeapons[w].guidance == kGuidanceFrontQuad
                    || gWeapons[w].guidance == kGuidanceRearQuad) {
                if (shipData->targetShip == -1) {
                    if (gWeapons[w].guidance == kGuidanceFrontQuad) {
                        SpawnShot(0, -1, w, 0);
                        fired++;
                        didFire = 1;
                    }
                } else {
                    angle = CalcInterceptAngle(
                        &shipData->pos,
                        &shipData[shipData->targetShip].pos);
                    if (gWeapons[w].guidance == kGuidanceFrontQuad)
                        diff = abs(AngularDifference(
                            angle, shipData->heading));
                    if (gWeapons[w].guidance == kGuidanceRearQuad)
                        diff = abs(AngularDifference(
                            angle,
                            ((short)(shipData->heading + 180.0f) % 360)));
                    if (diff > 45) {
                        if (gWeapons[w].guidance == kGuidanceFrontQuad) {
                            SpawnShot(0, -1, w, 0);
                            fired++;
                            didFire = 1;
                        }
                    } else {
                        SpawnShot(0, shipData->targetShip, w, 0);
                        fired++;
                        didFire = 1;
                    }
                }
            }
            if (gWeapons[w].guidance == kGuidanceBay) {
                if (LaunchFighter(shipData, w)) {
                    fired++;
                    didFire = 1;
                }
            }
            if (!didFire)
                continue;

            if (gWeapons[w].flags2 & kWeapU22NoAmmoUse)
                shipData->weapGlow = 32.0f;
            ammoType = gWeapons[w].ammoType;
            if (ammoType == -1)
                continue;
            if (gWeapons[w].flags3 & kWeapAmmoFlag)
                continue;
            if (ammoType >= 0) {
                if (gWeapons[w].guidance == kGuidanceBay)
                    ammoType = w;
                else if (ammoType < 0 || ammoType >= kMaxWeapTypes)
                    ammoType = -1;
                if (ammoType != -1) {
                    shipData->weap[ammoType].ammo--;
                    if (shipData->weap[ammoType].ammo < 0)
                        shipData->weap[ammoType].ammo = 0;
                }
            } else {
                if (ammoType > -kWeapAmmoFuelBase)
                    continue;
                shipData->fuel -= (abs(ammoType) - kWeapAmmoFuelBase) * 0.1;
                if (shipData->fuel < 0.0f)
                    shipData->fuel = 0.0f;
                statusRedraw[4] = 1;
                continue;
            }
            statusRedraw[3] = 1;
        }

        if (fired > 0) {
            soundOK = 1;
            if (gWeapons[w].flags & kWeapFlagSoundOnce) {
                if (gWeapons[w].sound >= 0) {
                    if (ST_IsSoundPlaying(shotSnd[gWeapons[w].sound]))
                        soundOK = 0;
                }
            }
            if (gWeapons[w].sound >= 0 && soundOK) {
                if (gWeapons[w].guidance == kGuidanceBeam
                        || gWeapons[w].guidance == kGuidanceTurretBeam) {
                    PlaySoundDistance(2, shotSnd[gWeapons[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                    soundOK = 0;
                }
            }
            if (gWeapons[w].sound < 0)
                soundOK = 0;
            if (soundOK) {
                if (gWeapons[w].guidance == kGuidanceBay)
                    PlaySoundDistance(-1, shotSnd[gWeapons[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                else if (gWeapons[w].flags & kWeapFlagSoundNear)
                    PlaySoundDistance(-1, shotSnd[gWeapons[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                else
                    PlaySoundDistance(-1, shotSnd[gWeapons[w].sound], 5,
                                      &shipData->pos, &shipData->pos);
            }

            /* recoil */
            if (gWeapons[w].recoil > 0) {
                AdjustedAccel(((short)(shipData->heading + 180.0f) % 360),
                              gWeapons[w].recoil
                                  / (float)gShipTypes[shipData->type].mass,
                              &shipData->vel,
                              gShipTypes[shipData->type].maxSpeed);
            }

            if (gWeapons[w].flags & kWeapFlagKeepReload)
                shipData->weap[w].reloadTimer = gWeapons[w].reload;
            else
                shipData->weap[w].reloadTimer =
                    gWeapons[w].reload / shipData->weap[w].count * fired;

            if (gWeapons[w].flags3 & kWeapExclusive) {
                for (i = 0; i < kMaxWeapTypes; i++) {
                    if (i != w) {
                        if (shipData->weap[i].reloadTimer
                                < shipData->weap[w].reloadTimer + 2.0f)
                            shipData->weap[i].reloadTimer =
                                shipData->weap[w].reloadTimer + 2.0f;
                    }
                }
            }

            if (gWeapons[w].burstCount > 0) {
                shipData->weap[w].burstCount++;

                /* per-burst ammo is charged once the burst completes */
                if ((gWeapons[w].flags3 & kWeapAmmoFlag)
                        && shipData->weap[w].burstCount
                            % gWeapons[w].burstCount == 0) {
                    if (gWeapons[w].flags & kWeapFlagKeepReload)
                        n = fired;
                    else
                        n = 1;
                    if (gWeapons[w].guidance == kGuidanceBay) {
                        shipData->weap[w].ammo -= n;
                        if (shipData->weap[w].ammo < 0)
                            shipData->weap[w].ammo = 0;
                    } else {
                        ammoType = gWeapons[w].ammoType;
                        if (ammoType >= 0 && ammoType < kMaxWeapTypes) {
                            shipData->weap[gWeapons[w].ammoType].ammo -= n;
                            if (shipData->weap[gWeapons[w].ammoType].ammo < 0)
                                shipData->weap[gWeapons[w].ammoType].ammo = 0;
                            statusRedraw[3] = 1;
                        } else if (ammoType <= -kWeapAmmoFuelBase) {
                            shipData->fuel -=
                                (abs(ammoType) - kWeapAmmoFuelBase) * 0.1 * n;
                            if (shipData->fuel < 0.0f)
                                shipData->fuel = 0.0f;
                            statusRedraw[4] = 1;
                        }
                    }
                }

                if (shipData->weap[w].burstCount
                        >= WeaponBurstInterval(w, shipData->weap[w].count)) {
                    shipData->weap[w].burstCount = 0;
                    shipData->weap[w].reloadTimer = gWeapons[w].burstReload;
                }
            }
        }
    }
}

/* PlayerLandOnStellar (mac _PlayerLandOnStellar). STATE: MATCH 100.0%, exe 397 / obj 397,
   relocregcheck 0 of 124; the landing-position test is written `!= -1' first. OPEN: none */
// FUNCTION: EVNOVA 0x00455e10
void PlayerLandOnStellar(void)
{
    ShipObj *ship;
    short sys;
    short i;

    ship = shipData;
    playerIsLanded = 1;
    dontMovePlayerAfterLanding = 0;
    PreloadPortDialog(&stellarData[ship->targetStellar], ship->targetStellar);
    ShipStatsToSystemInfo();
    SWHideAllSprites(gSpriteWorld);
    ClearParticles(1);
    EraseParticlesOffscreen(gSpriteWorld);
    SWUpdateSpriteWorld(gSpriteWorld, 1);
    UpdateParticlesInWindow(gSpriteWorld);
    HideCursorSprite();
    RefreshSAT();
    RedrawStatus(1);
    ShowTheCursor();
    if (!gEarlyOutFlag)
        PlayQTMovieFromDesc(ship->targetStellar + 128);
    if (!SpobUninhabited(&stellarData[ship->targetStellar]))
        DoAutoRefuel();
    else
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    RedrawStatus(1);
    OSCursorReset();
    cursorVisible = 0;
    ShowTheCursor();
    SWSetPortToWindow(gSpriteWorld);
    ship->targetShip = -1;
    absquatulateStr[0] = 0;
    statusRedraw[6] = 1;
    statusRedraw[2] = 1;
    DrawStatus();
    SWSetPortToWindow(gSpriteWorld);
    gSpriteWorld->fastBlit = 0;
    ShowTheCursor();
    DoPortDialog(&stellarData[ship->targetStellar], ship->targetStellar);
    gSpriteWorld->fastBlit = 1;
    SWSetPortToWindow(gSpriteWorld);
    RedrawStatus(1);
    ship->vel.x = ship->vel.y = 0;
    if (!dontMovePlayerAfterLanding) {
        if (ship->targetStellar != -1) {
            ship->pos.x = stellarData[ship->targetStellar].pos.h;
            ship->pos.y = stellarData[ship->targetStellar].pos.v;
        } else {
            ship->pos.x = ship->pos.y = 0;
        }
    } else {
        dontMovePlayerAfterLanding = 0;
    }
    ship->accelMag = 0;
    ship->vel.x = ship->vel.y = 0;
    ship->shield = ShipShieldCapacity(ship);
    ship->armor = ShipArmorCapacity(ship);
    IncrementGameTime();
    HandleGlobalPriceFlux();
    sys = FindFirstCoLocatedSystem(ship->curSystem);
    if (gSystems[sys].exploreLevel < 2)
        gSystems[sys].exploreLevel = 2;
    if (gSystems[ship->curSystem].exploreLevel < 2)
        gSystems[ship->curSystem].exploreLevel = 2;
    AutoSetExploration(ship->curSystem, 0, 2);
    ExploreNebulaeFromSystem(ship->curSystem);
    for (i = 0; i < 16; i++) {
        if (currentMissionObjectives[i].active != 0) {
            if ((currentMissions[i].flags & kMissionFlagRearmShips) != 0)
                currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
            currentMissions[i].auxShipDelay = Rand(0x46) + 0x46;
            currentMissions[i].auxShipsSpawned = 0;
        }
    }
    SavePilotFile(ship->targetStellar);
    ship->heading = Rand(0x168);
    SetAllKeyCmdFlags();
    playerIsLanded = 0;
    if (absquatulateStr[0] != 0) {
        DisplayComm((unsigned char *)absquatulateStr, 500, 0, 12, &rgbWhite, 0, 0);
        RedrawComm(1);
        absquatulateStr[0] = 0;
    } else if (newbieFlag > 2) {
        char name[256];
        char msg[256];
        long n;

        n = Rand(5) + 0x37;
        strcpy(msg, "");
        AppendIndString(msg, kGameStrList, n);
        strcat(msg, " ");
        strncpy(name, (char *)stellarData[dockTarget].name, 0x3f);
        PtoCstr((unsigned char *)name);
        strcat(msg, name);
        strcat(msg, " ");
        AppendIndString(msg, kGameStrList, 0x3c);
        strcat(msg, " ");
        DateToLongString(gameTime, name);
        strcat(msg, name);
        strcat(msg, ".");
        CtoPstr(msg);
        DisplayComm((unsigned char *)msg, 240, 0, 12, &rgbWhite, 0, 0);
        RedrawComm(1);
    } else {
        newbieFlag = -1;
    }
    dockTarget = -1;
    dockPermission = -1;
    gTimeScale = 0.0;
    killShots = 1;
    SWHideAllSprites(gSpriteWorld);
    for (i = 0; i < 128; i++)
        shotData[i].life = -2.0f;
    lastMouseMoveTime = ReadTick() - 60;
}


/* PlayerEnterHypergate (mac _PlayerEnterHypergate). STATE: MATCH 100.0%, exe 492 / obj 492,
   byte-identical, relocregcheck 0 of 161. The hyperlink count is held in `link', so the scan
   loop's `link' is a split web; `n' carries the scan's system and the nav-link count, as in
   PlayerEnterWormhole. OPEN: none. */
// FUNCTION: EVNOVA 0x00456480
short PlayerEnterHypergate(void)
{
    short i;
    short destStellar;
    short destSyst;
    short link;
    short n;
    ShipObj *ship;
    char name[256];
    char msg[256];

    ship = shipData;
    shipData->navTargetKind = -1;
    shipData->targetStellar = -1;
    link = StellarNumHyperLinks(&stellarData[dockTarget]);
    if (link == 0)
        return -1;
    ShowTheCursor();
    HideCursorSprite();
    RefreshSAT();
    hypergateMapMode = 1;
    hypergateMapModeStellar = dockTarget;
    DoSystemMap();
    hypergateMapMode = 0;
    hypergateMapModeStellar = -1;
    SetAllKeyCmdFlags();
    if (shipData->navTargetKind == 4 && shipData->targetStellar != -1) {
        destSyst = -1;
        for (i = 0; i < kSpobHyperLinks; i++) {
            link = stellarData[dockTarget].hyperLinks[i];
            if (link != -1) {
                n = stellarData[link].system;
                if (n < 0 || n >= kMaxSystems)
                    n = FindSystemFromStellar(link);
                n = FindActiveCoLocatedSystem(n);
                if (n == shipData->targetStellar) {
                    destStellar = stellarData[dockTarget].hyperLinks[i];
                    destSyst = n;
                    break;
                }
            }
        }
        if (destSyst != -1) {
            HideCursorSprite();
            RefreshSAT();
            if (!gEarlyOutFlag)
                PlayQTMovieFromDesc(ship->targetStellar + kFirstResourceID);
            SetAllKeyCmdFlags();
            ship->curSystem = destSyst;
            n = FindFirstCoLocatedSystem(destSyst);
            if (gSystems[n].exploreLevel < 1)
                gSystems[n].exploreLevel = 1;
            if (gSystems[destSyst].exploreLevel < 1)
                gSystems[destSyst].exploreLevel = 1;
            mapScroll.x = gSystems[destSyst].xPos;
            mapScroll.y = gSystems[destSyst].yPos;
            ship->pos.x = stellarData[destStellar].pos.h;
            ship->pos.y = stellarData[destStellar].pos.v;
            if (stellarData[destStellar].customSound < 0
                || stellarData[destStellar].customSound > kFullCircle - 1)
                ship->heading = Rand(kFullCircle);
            else
                ship->heading = stellarData[destStellar].customSound;
            ship->vel.x = ship->vel.y = 0;
            ship->accelMag = (ShipMaxSpeed(ship) * 0.5);
            Accel(ship->heading, ship->accelMag, &ship->vel);
            SWSetPortToWindow(gSpriteWorld);
            RGBForeColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            RGBForeColor(blackColor);
            ScreenEndScene();
            SWSetPortToWorkArea(gSpriteWorld);
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &gSpriteWorld->workFrameP->frameRect,
                         &gSpriteWorld->windowFrameP->frameRect);
            strcpy(msg, "");
            AppendIndString(msg, kGameStrList, 0x2e);
            strcat(msg, " ");
            pstrcpy((unsigned char *)name, gSystems[destSyst].name);
            PtoCstr((unsigned char *)name);
            strcat(msg, name);
            strcat(msg, " ");
            AppendIndString(msg, kGameStrList, 0x30);
            strcat(msg, " ");
            DateToLongString(gameTime, name);
            strcat(msg, name);
            strcat(msg, ".");
            n = 0;
            for (i = 0; i < kNavSlots; i++) {
                if (gSystems[ship->curSystem].navDefault[i] != -1)
                    n++;
            }
            if (n == 0) {
                strcat(msg, " ");
                AppendIndString(msg, kGameStrList, 0x31);
            }
            CtoPstr(msg);
            if (gSystems[ship->curSystem].murk < 0)
                ClearStarfield();
            else
                RandomizeStars();
            ScrollStarfield(0.0f, 0.0f);
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            hyperRoute[0] = shipData->curSystem;
            PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
            ClearParticles(1);
            EraseParticlesOffscreen(gSpriteWorld);
            SWHideAllSprites(gSpriteWorld);
            HandleStellarSystemVisibility();
            PrepareAllSprites();
            SWSetPortToWorkArea(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->backFrameP->frameRect);
            SWSetPortToWorkArea(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            SWSetPortToWindow(gSpriteWorld);
            RefreshSAT();
            DisplayComm((unsigned char *)msg, 0x168, 0, 12, &rgbWhite, 0, 0);
            killShots = 1;
            killBoxes = 1;
            killExplods = 1;
            killSmoke = 1;
            killAsteroids = 1;
            crashlog_print("travelled through hypergate to system ");
            crashlog_print_num(shipData->curSystem + kFirstResourceID);
            crashlog_print(" (");
            crashlog_print_pstr(gSystems[shipData->curSystem].name);
            crashlog_print(")\n");
            shipData->navTargetKind = -1;
            shipData->targetStellar = -1;
            shipData->targetShip = -1;
            hyperRoute[0] = shipData->curSystem;
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            statusRedraw[5] = 1;
            statusRedraw[6] = 1;
            statusRedraw[0] = 1;
            RedrawStatus(1);
            return destStellar;
        }
    }
    GetIndString((unsigned char *)msg, kGameStrList, 0x32);
    DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
    return -1;
}

/* PlayerEnterWormhole (mac _PlayerEnterWormhole). STATE: MATCH 100.0%, exe 564 / obj 564,
   relocregcheck 0 of 156. The customSound test is written in-range arm first; the navDefault
   count reuses `link'. OPEN: none. */
// FUNCTION: EVNOVA 0x00456ca0
short PlayerEnterWormhole(void)
{
    short i;
    short destStellar;
    short dest;
    short link;
    short pick;
    short n;
    ShipObj *ship;
    char name[256];
    char msg[256];

    dest = -1;
    destStellar = -1;
    ship = shipData;
    if (StellarNumHyperLinks(&stellarData[dockTarget]) > 0) {
        do {
            n = 0;
            for (i = 0; i < kSpobHyperLinks; i++) {
                link = stellarData[dockTarget].hyperLinks[i];
                if (link != -1) {
                    if (FindActiveCoLocatedSystem(stellarData[link].system) != -1)
                        n++;
                }
            }
            if (n == 0) {
                extern char *strcpy(char *, const char *);

                strcpy(msg, "");
                AppendIndString(msg, kGameStrList, 0x54);
                strcat(msg, " ");
                AppendIndString(msg, kGameStrList, 0x56);
                CtoPstr(msg);
                DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                return -1;
            }
            pick = Rand(8);
            link = stellarData[dockTarget].hyperLinks[pick];
            if (link != -1) {
                destStellar = link;
                dest = stellarData[link].system;
            }
        } while (dest == -1);
    } else {
        n = 0;
        for (i = 0; i < 0x800; i++) {
            if (i != dockTarget && stellarData[i].loaded != 0
                && (stellarData[i].flags2 & kSpobWormhole) != 0
                && stellarData[i].system != -1
                && ship->curSystem != stellarData[i].system
                && StellarNumHyperLinks(&stellarData[i]) == 0
                && FindActiveCoLocatedSystem(stellarData[i].system) != -1)
                n++;
        }
        if (n <= 0) {
            extern char *strcpy(char *, const char *);

            strcpy(msg, "");
            AppendIndString(msg, kGameStrList, 0x54);
            strcat(msg, " ");
            AppendIndString(msg, kGameStrList, 0x56);
            CtoPstr(msg);
            DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            return -1;
        }
        do {
            i = Rand(0x800);
        } while (i == dockTarget || stellarData[i].loaded == 0
                 || (stellarData[i].flags2 & kSpobWormhole) == 0
                 || stellarData[i].system == -1
                 || ship->curSystem == stellarData[i].system);
        if (StellarNumHyperLinks(&stellarData[i]) == 0
            && FindActiveCoLocatedSystem(stellarData[i].system) != -1) {
            destStellar = i;
            dest = stellarData[i].system;
        }
    }
    if (dest == -1) {
        dest = ship->curSystem;
        destStellar = dockTarget;
    }
    if (!gEarlyOutFlag)
        PlayQTMovieFromDesc(ship->targetStellar + 128);
    RefreshSAT();
    SetAllKeyCmdFlags();
    ship->curSystem = dest;
    n = FindFirstCoLocatedSystem(dest);
    if (gSystems[n].exploreLevel < 1)
        gSystems[n].exploreLevel = 1;
    if (gSystems[dest].exploreLevel < 1)
        gSystems[dest].exploreLevel = 1;
    mapScroll.x = gSystems[dest].xPos;
    mapScroll.y = gSystems[dest].yPos;
    ship->pos.x = stellarData[destStellar].pos.h;
    ship->pos.y = stellarData[destStellar].pos.v;
    if (stellarData[destStellar].customSound >= 0
        && stellarData[destStellar].customSound <= 0x167)
        ship->heading = stellarData[destStellar].customSound;
    else
        ship->heading = Rand(0x168);
    ship->vel.x = ship->vel.y = 0;
    ship->accelMag = ShipMaxSpeed(ship) * 0.5;
    Accel(ship->heading, ship->accelMag, &ship->vel);
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(whiteColor);
    PaintRect(&gSpriteWorld->windRect);
    RGBForeColor(blackColor);
    ScreenEndScene();
    PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
    strcpy(msg, "");
    AppendIndString(msg, kGameStrList, 0x2f);
    strcat(msg, " ");
    pstrcpy((unsigned char *)name, gSystems[dest].name);
    PtoCstr((unsigned char *)name);
    strcat(msg, name);
    strcat(msg, " ");
    AppendIndString(msg, kGameStrList, 0x30);
    strcat(msg, " ");
    DateToLongString(gameTime, name);
    strcat(msg, name);
    strcat(msg, ".");
    link = 0;
    for (i = 0; i < kNavSlots; i++) {
        if (gSystems[ship->curSystem].navDefault[i] != -1)
            link++;
    }
    if (link == 0) {
        strcat(msg, " ");
        AppendIndString(msg, kGameStrList, 0x31);
    }
    CtoPstr(msg);
    DisplayComm((unsigned char *)msg, 240, 0, 12, &rgbWhite, 0, 0);
    killShots = 1;
    killBoxes = 1;
    killExplods = 1;
    killSmoke = 1;
    killAsteroids = 1;
    if (gSystems[ship->curSystem].murk < 0)
        ClearStarfield();
    else
        RandomizeStars();
    ScrollStarfield(0.0f, 0.0f);
    ClearParticles(1);
    EraseParticlesOffscreen(gSpriteWorld);
    for (i = 1; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;
    hyperRoute[0] = shipData->curSystem;
    SWHideAllSprites(gSpriteWorld);
    HandleStellarSystemVisibility();
    PrepareAllSprites();
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    SWSetPortToWindow(gSpriteWorld);
    RefreshSAT();
    return destStellar;
}

/* HandlePlayerDockRequest (mac _HandlePlayerDockRequest). STATE: MATCH 100.0%, 2120/2120,
   relocregcheck 0 of 378; byte-identical yes (9159 bytes). OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00457580
void HandlePlayerDockRequest(void)
{
    ShipObj *ship = shipData;
    unsigned char msg[256];
    float fdx;
    float fdy;
    char granted;
    unsigned char sb;
    char cval;
    short i;
    unsigned int uVal2;
    int iVal2;
    short qtStellar;
    short sVal2;
    short spawnStellar;
    unsigned char pmov[3] = { 0x49, 0x71, 0xad };
    unsigned char key[17] = "EV NovaEV NovaEV";
    unsigned char scram;
    char nameMatch;
    short commResult;
    unsigned char pbuf[256];
    unsigned int distX;
    unsigned int distY;
    unsigned long long lic;
    unsigned long hash;
    unsigned char regBuf[17] = "";
    char *regName;
    int licSerial;
    short badLicense;
    short sVal;

    licSerial = shipData[36].unkC940;
    regName = shipData[17].unkC930;
    /* one 64-bit field; shipobj.h still spells it as unkC938/unkC93C */
    lic = *(unsigned long long *)&shipData[5].unkC938;
    sb = 0;
    scram = 0;
    if (regName != 0 && *regName != 0) {
        while (StringGetLength((unsigned char *)regName) != 0 &&
               StringGetLength(regBuf) + 1U < 0x11) {
            StringAppendSafe(regBuf, (unsigned char *)regName, 0x11);
        }
    }
    sb ^= licSerial;
    sb += -0x19;
    sb ^= licSerial;
    sb = ROTL(sb, 3) ^ key[5];
    sb += 0x31;
    sb = ROTL(sb, 7) ^ regBuf[0];
    sb = ROTL(sb, 4);
    sb = ROTL(sb, 8);
    sb += 0xb;
    sb ^= key[2];
    sb ^= regBuf[13];
    sb ^= key[13];
    sb = sb + 0xa6;
    sb = sb ^ licSerial ^ key[12] ^ licSerial ^ regBuf[1] ^ licSerial ^ regBuf[0] ^ regBuf[4];
    sb = sb + 0x67;
    sb = sb ^ key[6] ^ licSerial;
    sb = sb + 0x58;
    if (sb > (char)regBuf[1])
        sb ^= licSerial;
    sb = ROTL(sb, 2);
    sb ^= RegScramE();
    sb = (sb + 0x1d) ^ licSerial;
    sb += 0x5a;
    sb = ((ROTL(sb, 7) ^ licSerial) + 0x93) ^ licSerial;
    sb = ROTL(sb, 4);
    sb = ROTL(sb, 2) ^ key[9];
    hash = DRHash64(lic);
    sb += 0x78;
    sb = ROTL(sb, 2) ^ regBuf[2] ^ key[2] ^ licSerial ^ licSerial;
    sb = ROTL(sb, 5);
    sb = ROTL(sb, 5) ^ licSerial;
    if (sb < (char)regBuf[14])
        sb ^= licSerial;
    if (sb > (char)regBuf[4])
        sb ^= key[4];
    sb ^= licSerial;
    sb = ROTL(sb, 4) ^ regBuf[15] ^ key[14];
    sb = ROTL(sb, 2);
    sb += 0x75;
    RegScramC(sb);
    sb = ROTL(sb, 1);
    sb = ROTL(sb, 4);
    sb = ROTL(sb, 8);
    sb = ROTL(sb, 1);
    sb -= 0x65;
    sb = ROTL(sb, 1) ^ licSerial;
    sb = (sb + 0x58) ^ regBuf[15];
    sb = RegScramA(sb);
    sb -= 0x61;
    RegScramB(sb);
    RegScramC(sb);
    sb = ROTL(sb, 5) ^ regBuf[3];
    scram = sb;
    cval = RegScramD((char *)&scram);
    sb = (cval + 0xa4) ^ regBuf[1] ^ regBuf[14];
    sb += 0x29;
    sb = ((((ROTL(sb, 2) - 0x2f) ^ licSerial) + 0x82) ^ key[0] ^ key[1]) ^ licSerial;
    sb -= 0x7c;
    RegScramB(sb);
    sb = ROTL(sb, 5);
    sb -= 0x4e;
    sb = ROTL(sb, 1) ^ licSerial;
    sb = ((ROTL(sb, 5) + 0x5a) ^ key[11]) ^ licSerial;
    sb = ((((ROTL(sb, 5) ^ licSerial) + 0xe8) ^ regBuf[7]) + 0xdd) ^ regBuf[2] ^ regBuf[5];
    sb = ROTL(sb, 1) ^ licSerial ^ licSerial;
    sb = ROTL(sb, 7);
    sb = ((ROTL(sb, 1) ^ licSerial) + 0x36) ^ licSerial;
    if (sb < 0x23)
        sb = ROTL(sb, 4);
    sb ^= licSerial;
    sb ^= licSerial;
    sb ^= regBuf[11];
    if (sb < 3)
        sb = pmov[sb];
    while ((unsigned short)hash >= sb)
        hash -= sb;
    nameMatch = licSerial != 0 && (short)hash == 0;
    sb ^= regBuf[10];
    sb += 0x4d;
    sb = ROTL(sb, 3);
    sb = ROTL(sb, 4);
    sb = (sb - 0x12) ^ regBuf[9];
    sb ^= RegScramE();
    sb -= 0x4c;
    sb = ROTL(sb, 4);
    sb = ROTL(sb, 4);
    sb = ROTL(sb, 8) ^ regBuf[8];
    sb = ROTL(sb, 6) ^ regBuf[2];
    sb = ROTL(sb, 7);
    uVal2 = sb + 0x57;
    sb = uVal2;
    if (sb != 0)
        RegScramC(uVal2);
    badLicense = (shipData[36].unkC940 != 0 && nameMatch == 0)
        ? (unsigned char)1 : (unsigned char)0;
    if (badLicense && TimerGetElapsedSeconds() > 0x4b0)
        FT_FileDelete(-3, "EV Nova License.lcs");

    if (!StellarIsLandable(&stellarData[ship->targetStellar]) ||
        ship->curSystem != stellarData[ship->targetStellar].system ||
        stellarData[ship->targetStellar].loaded == 0)
        goto cantDock;

    iVal2 = stellarData[ship->targetStellar].landingFee;
    if (iVal2 > 0 && stellarData[ship->targetStellar].dominated == 0 &&
        iVal2 > shipData->credits) {
        strcpy((char *)msg, "");
        AppendIndString((char *)msg, kGameStrList, 0x3d);
        strcat((char *)msg, " ");
        strcat((char *)msg, creditsCString);
        strcat((char *)msg, " ");
        if (stellarData[ship->targetStellar].flags2 & 0x1000) {
            AppendIndString((char *)msg, kGameStrList, 0x3e);
        } else if (stellarData[ship->targetStellar].flags & 0x10) {
            AppendIndString((char *)msg, kGameStrList, 0x3f);
        } else {
            AppendIndString((char *)msg, kGameStrList, 0x40);
        }
        CtoPstr((char *)msg);
        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
        dockTarget = -1;
        dockPermission = -1;
        goto done;
    }
    if (ShipIsCloaked(ship)) {
        GetIndString(msg, kGameStrList, 0x49);
        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        goto done;
    }
    if (IsDisabled(ship))
        goto done;
    if (IsDying(ship))
        goto done;
    autoDockFlag = stellarData[ship->targetStellar].flags & 0x10;
    if (stellarData[ship->targetStellar].flags & 0x20) {
        dockPermission = 0x2ee;
        ship->aiTimer = 0;
        dockTarget = ship->targetStellar;
        GetIndString(msg, kGameStrList, 0x35);
        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
    }
    sVal = ship->targetStellar;
    if (dockTarget == sVal) {
        fdx = fabs((double)ship->vel.x);
        fdy = fabs((double)ship->vel.y);
        if (dockPermission >= 0x2ee &&
            ship->aiTimer <= 0.0f &&
            fdx <= 0.75 &&
            fdy <= 0.75) {

            distX = abs((short)(ship->pos.x
                    - stellarData[sVal].pos.h));
            distY = abs((short)(ship->pos.y
                    - stellarData[sVal].pos.v));
            if (stellarData[sVal].sprite->curFrameP) {
                sVal = GetStellarSpriteXSize(sVal) * 1.75;
            } else {
                sVal = 0x4b;
            }
            if ((short)distX < sVal && (short)distY < sVal) {

                offeringFromShip = 0;
                dockedPort = &stellarData[ship->targetStellar];
                if (lastMissionStellar != ship->targetStellar || lastMissionStellar < 0) {
                    sVal2 = 0;
                    do {
                        missionRandom[sVal2] = Rand(100) + 1;
                        sVal2++;
                    } while (sVal2 < 1000);
                    RandomizeMissionData();
                }
                lastMissionStellar = ship->targetStellar;
                commCount = 0;
                SWSetSpriteVisible(alertSprite, 0);
                alertSpritePending = 0;
                HandleComm();
                DrawStatus();
                KillVoiceSound();
                AdjustSoundVolume();
                iVal2 = stellarData[dockTarget].landingFee;
                if (iVal2 > 0 && stellarData[dockTarget].dominated == 0 &&
                    (shipData->credits -= iVal2, shipData->credits < 0)) {
                    shipData->credits = 0;
                }
                qtStellar = -1;
                commResult = -1;
                crashlog_print("landing on spob ");
                crashlog_print_num(ship->targetStellar + 128);
                crashlog_print(" (");
                crashlog_print_pstr(stellarData[ship->targetStellar].name);
                crashlog_print(")\n");
                FlushEvents(1, 0);
                FlushEvents(2, 0);
                FlushEvents(3, 0);
                FlushEvents(4, 0);
                FlushEvents(5, 0);
                if (stellarData[ship->targetStellar].flags2 & 0x1000) {
                    qtStellar = PlayerEnterHypergate();
                    if (qtStellar == -1) {
                        dockTarget = -1;
                        granted = 0;
                        dockPermission = 0;
                    } else {
                        granted = 1;
                    }
                } else if (stellarData[ship->targetStellar].flags2 & 0x2000) {
                    commResult = PlayerEnterWormhole();
                    granted = 1;
                } else {
                    PlayerLandOnStellar();
                    granted = 1;
                }
                if (!granted)
                    goto done;

                killShots = 1;
                killBoxes = 1;
                killExplods = 1;
                killSmoke = 1;
                killAsteroids = 1;
                if ((gShipTypes[ship->type].shanFlags & 2) != 0 &&
                    (gShipTypes[ship->type].shanFlags & 0x80) != 0) {
                    ship->foldSet = gShipTypes[ship->type].baseSets - 1;
                    ship->foldDir = 0;
                } else {
                    ship->foldSet = 0;
                    ship->foldDir = 1;
                }
                for (sVal = 0; sVal < 0x40; sVal++) {
                    if (beamData[sVal].state > -1)
                        beamData[sVal].state = -1;
                }
                ship->frameTimer = 0;
                ship->engineGlow = 0;
                ship->shieldGlow = 0;
                ship->weapGlow = 0;
                ship->boarded = 0;
                ship->ionization = 0;
                ship->ionColor = 0;
                statusRedraw[5] = 1;
                statusRedraw[6] = 1;
                statusRedraw[3] = 1;
                statusRedraw[0] = 1;
                statusRedraw[1] = 1;
                statusRedraw[2] = 1;
                killShots = 1;
                killBoxes = 1;
                killExplods = 1;
                killSmoke = 1;
                killAsteroids = 1;
                escortMenuOnscreen = 0;
                dockPermission = 0;
                ship->targetShip = -1;
                antiEscortDelay = Rand(0x1e) + 0x1e;
                DeleteAllComputerShips(0, 1);
                ResetPlayerPrecalcedValues();
                if (gSystems[ship->curSystem].murk < 0)
                    ClearStarfield();
                LoadIntfResource();
                SWSetPortToBackground(gSpriteWorld);
                SetSystemBackgroundColor();
                PaintRect(&gSpriteWorld->windRect);
                RGBForeColor(blackColor);
                SWSetPortToWindow(gSpriteWorld);
                SWFlagRectAsChanged(gSpriteWorld, &gSpriteWorld->windRect);
                RedrawComm(1);
                if (qtStellar == -1 && commResult == -1) {
                    SetupShipsInSystem(ship->curSystem, 1);
                } else {
                    MissionHandlePlayerEnteredNewSystem();
                    for (sVal = 0; sVal < 0x10; sVal++) {
                        if (currentMissionObjectives[sVal].active != 0 &&
                            currentMissions[sVal].shipSyst == -6 &&
                            currentMissions[sVal].shipCount > 0) {
                            if (currentMissions[sVal].shipBehav == 1) {
                                currentMissions[sVal].shipDelay = -1;
                                currentMissions[sVal].specialShipsJumpedIn = 0;
                            } else {
                                currentMissions[sVal].shipDelay = 0x7fff;
                                currentMissions[sVal].specialShipsJumpedIn =
                                    currentMissions[sVal].shipCount;
                                if (currentMissions[sVal].shipBehav == 0) {
                                    if (dudeData[currentMissions[sVal].shipDude].govt != -1) {
                                        if (qtStellar != -1 &&
                                            (govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & 0x20) == 0) {
                                            if ((govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & 0x40) != 0 ||
                                                Rand(2) == 0) {
                                                currentMissions[sVal].shipDelay = 0;
                                                currentMissions[sVal].specialShipsJumpedIn = 0;
                                            }
                                        } else if (commResult != -1 &&
                                                   ((govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & 0x80) != 0 ||
                                                    Rand(4) == 0)) {
                                            currentMissions[sVal].shipDelay = 0;
                                            currentMissions[sVal].specialShipsJumpedIn = 0;
                                        }
                                    }
                                }
                            }
                        }
                        if (currentMissions[sVal].auxShipSyst == -1)
                            currentMissions[sVal].auxShipDelay = 0x7fff;
                    }
                    SetupShipsInSystem(ship->curSystem, 0);
                    EnterMoreShips(ship->curSystem);
                    if (qtStellar == -1)
                        spawnStellar = commResult;
                    else
                        spawnStellar = qtStellar;
                    for (sVal2 = 0; sVal2 < 0x10; sVal2++) {
                        if (currentMissionObjectives[sVal2].active != 0 &&
                            currentMissions[sVal2].shipSyst == -6 &&
                            currentMissions[sVal2].shipCount > 0 &&
                            currentMissions[sVal2].shipBehav == 0 &&
                            currentMissions[sVal2].shipDelay == -1) {
                            for (i = 1; i < 0x40; i++) {
                                if (shipData[i].active != 0 &&
                                    sVal2 == shipData[i].specialMission) {
                                    shipData[i].vel.x = 0;
                                    shipData[i].vel.y = 0;
                                    shipData[i].accelMag = 0;
                                    shipData[i].grudge = -1;
                                    AIMakeShipEmergeFromHyperGate(&shipData[i], spawnStellar);
                                    shipData[i].pos.x = stellarData[spawnStellar].pos.h;
                                    shipData[i].pos.y = stellarData[spawnStellar].pos.v;
                                    shipData[i].heading = ship->heading;
                                    shipData[i].aiTimer = (Rand(0x32) + 100);
                                }
                            }
                        }
                    }
                }
                HandleStellarSystemVisibility();
                ship->targetStellar = -1;
                SelectHyperLinkToSelection();
                SpawnBountyHunter();
                if (qtStellar != -1 || commResult != -1) {
                    if (qtStellar != -1)
                        commResult = qtStellar;
                    for (sVal = 1; sVal < 0x40; sVal++) {
                        if (shipData[sVal].active != 0 &&
                            shipData[sVal].parentShip == 0) {
                            AIMakeShipEmergeFromHyperGate(&shipData[sVal], commResult);
                            shipData[sVal].pos.x = stellarData[commResult].pos.h;
                            shipData[sVal].pos.y = stellarData[commResult].pos.v;
                            shipData[sVal].heading = ship->heading;
                            shipData[sVal].aiTimer = (Rand(0x14) + 0xf);
                        }
                    }
                }
                for (sVal = 0; sVal < 0x800; sVal++) {
                    stellarData[sVal].curFrame = 0;
                    stellarData[sVal].nextFrame = 0;
                }
                if (qtStellar != -1) {
                    stellarData[qtStellar].curFrame = stellarData[qtStellar].numFrames - 1;
                    stellarData[qtStellar].nextFrame = stellarData[qtStellar].curFrame;
                    stellarData[qtStellar].frameTimer = -stellarData[qtStellar].animDelay;
                }
                PrepareAllSprites();
                HandleStellarSystemVisibility();
                PrepareAllSprites();
                RefreshSAT();
                RedrawStatus(1);
                AdjustSoundVolume();
                frameCounter = -0xf;
                CheckLoMem();
                return;
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                if (ship->targetStellar < 0)
                    goto done;
                if (stellarData[ship->targetStellar].flags2 & 0x1000) {
                    GetIndString(msg, kGameStrList, 0x41);
                } else if (stellarData[ship->targetStellar].flags2 & 0x2000) {
                    GetIndString(msg, kGameStrList, 0x42);
                } else if (autoDockFlag != 0) {
                    GetIndString(msg, kGameStrList, 0x43);
                } else {
                    GetIndString(msg, kGameStrList, 0x44);
                }
                DisplayComm(msg, 0x168, 0, 0xc, &rgbWhite, 0, 0);
                goto done;
            }
        } else {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            sVal = ship->targetStellar;
            if (sVal < 0)
                goto done;
            if (sVal >= 0x800)
                goto done;
            if (!stellarData[sVal].sprite->curFrameP) {
                sVal = 0x4b;
            } else {
                sVal = GetStellarSpriteXSize(sVal) * 1.75;
            }
            distX = abs((short)(ship->pos.x
                    - stellarData[ship->targetStellar].pos.h));
            distY = abs((short)(ship->pos.y
                    - stellarData[ship->targetStellar].pos.v));
            if ((short)distX < sVal && (short)distY < sVal &&
                dockPermission >= 0x2ee) {
                if (stellarData[ship->targetStellar].flags2 & 0x1000) {
                    GetIndString(msg, kGameStrList, 0x45);
                } else if (stellarData[ship->targetStellar].flags2 & 0x2000) {
                    GetIndString(msg, kGameStrList, 0x46);
                } else if (autoDockFlag != 0) {
                    GetIndString(msg, kGameStrList, 0x47);
                } else {
                    GetIndString(msg, kGameStrList, 0x48);
                }
                DisplayComm(msg, 0x168, 0, 0xc, &rgbWhite, 0, 0);
            } else {
                if (stellarData[ship->targetStellar].flags2 & 0x1000) {
                    GetIndString(msg, kGameStrList, 0x41);
                } else if (stellarData[ship->targetStellar].flags2 & 0x2000) {
                    GetIndString(msg, kGameStrList, 0x42);
                } else if (autoDockFlag != 0) {
                    GetIndString(msg, kGameStrList, 0x43);
                } else {
                    GetIndString(msg, kGameStrList, 0x44);
                }
                DisplayComm(msg, 0x168, 0, 0xc, &rgbWhite, 0, 0);
            }
            goto done;
        }
    }

    /* dockTarget != stellar: the player has just asked to dock here. */
    dockTarget = sVal;
    fdx = ship->pos.x - stellarData[sVal].pos.h;
    distX = abs((short)fdx);
    fdy = ship->pos.y - stellarData[sVal].pos.v;
    distY = abs((short)fdy);
    qtStellar = stellarData[sVal].minStatus;
    granted = 0;
    if ((playerLegalStatus[ship->curSystem] >= qtStellar || qtStellar == -0x7fff) &&
        qtStellar != 0x7fff)
        granted = 1;
    if (stellarData[sVal].dominated != 0)
        granted = 1;
    if (dockPermission >= 0x2ee)
        granted = 1;
    if (stellarData[sVal].govt != -1 &&
        !PlayerMeetsRequirements(
            govtData[stellarData[sVal].govt].requires[0],
            govtData[stellarData[sVal].govt].requires[1]))
        granted = 0;
    if (!granted) {
        for (sVal = 0; sVal < 0x10; sVal++) {
            if (currentMissionObjectives[sVal].active != 0) {
                if (shipData->targetStellar == currentMissions[sVal].travelStellar)
                    granted = 1;
                if (shipData->targetStellar == currentMissions[sVal].returnStellar)
                    granted = 1;
            }
        }
    }
    if (!granted && (sVal = stellarData[dockTarget].govt, sVal != -1) &&
        PlayerBlessedByGovt(sVal, 1))
        granted = 1;
    if (granted) {
        if ((short)distX < 0xfa && (short)distY < 0xfa) {
            dockPermission = 0x2ed;
            goto done;
        }
        dockPermission = 0;
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        strcpy((char *)msg, "");
        if ((stellarData[dockTarget].flags2 & 0x1000) == 0) {
            if (stellarData[dockTarget].flags & 0x10) {
                if (Rand(3) == 0) {
                    strncpy((char *)msg, (char *)stellarData[shipData->targetStellar].name, 0x1f);
                    PtoCstr(msg);
                    strcat((char *)msg, " ");
                    AppendIndString((char *)msg, kGameStrList, 0x4c);
                    goto greetTail;
                } else {
                    AppendIndString((char *)msg, kGameStrList, 0x4d);
                }
            } else {
                if (Rand(3) == 0) {
                    strncpy((char *)msg, (char *)stellarData[shipData->targetStellar].name, 0x1f);
                    PtoCstr(msg);
                    strcat((char *)msg, " ");
                    AppendIndString((char *)msg, kGameStrList, 0x4e);
                    goto greetTail;
                } else {
                    AppendIndString((char *)msg, kGameStrList, 0x4f);
                }
            }
        } else {
            AppendIndString((char *)msg, kGameStrList, Rand(2) + 0x4a);
        }
greetTail:
        if (Rand(2) == 0) {
            strncpy((char *)pbuf, (char *)shipName, 0x40);
            PtoCstr(pbuf);
            strcat((char *)msg, ", ");
            strcat((char *)msg, (char *)pbuf);
        }
        strcat((char *)msg, ". ");
        AppendIndString((char *)msg, kGameStrList, 0x50);
        CtoPstr((char *)msg);
        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
        goto done;
    } else {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (stellarData[dockTarget].flags2 & 0x1000) {
            GetIndString(msg, kGameStrList, 0x51);
        } else if (stellarData[dockTarget].flags & 0x10) {
            GetIndString(msg, kGameStrList, 0x52);
        } else {
            GetIndString(msg, kGameStrList, 0x53);
        }
        DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
        dockTarget = -1;
        dockPermission = -1;
        goto done;
    }

cantDock:
    {
        extern char *strcpy(char *, const char *);

        strcpy((char *)msg, "");
    }
    AppendIndString((char *)msg, kGameStrList, 0x54);
    strcat((char *)msg, " ");
    if (stellarData[ship->targetStellar].flags2 & 0x1000) {
        AppendIndString((char *)msg, kGameStrList, 0x55);
    } else if (stellarData[ship->targetStellar].flags2 & 0x2000) {
        AppendIndString((char *)msg, kGameStrList, 0x56);
    } else if (stellarData[ship->targetStellar].flags & 0x10) {
        AppendIndString((char *)msg, kGameStrList, 0x57);
    } else {
        AppendIndString((char *)msg, kGameStrList, 0x58);
    }
    if ((stellarData[ship->targetStellar].flags2 & 0x3000) == 0) {
        strcat((char *)msg, " ");
        pstrcpy(pbuf, stellarData[ship->targetStellar].name);
        PtoCstr(pbuf);
        strcat((char *)msg, (char *)pbuf);
        strcat((char *)msg, ". ");
        if (stellarData[ship->targetStellar].flags & 0x10)
            AppendIndString((char *)msg, kGameStrList, 0x59);
        else
            AppendIndString((char *)msg, kGameStrList, 0x5a);
    }
    CtoPstr((char *)msg);
    DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);

done:
    ;
}

/* STR# 2002 indexes */
#define kStrAutoClearance   53      /* uninhabited automated clearance */
#define kStrGreetSpecialA   91      /* flags2 & 0x1000 greetings */
#define kStrGreetSpecialB   92
#define kStrGreetSpecialC   93
#define kStrGreetBigA       94      /* flags & 0x10 greetings */
#define kStrGreetBigB       95
#define kStrGreetBigC       96
#define kStrGreetSmallA     97      /* other inhabited greetings */
#define kStrGreetSmallB     98
#define kStrGreetSmallC     99
#define kStrWelcomeShort    100
#define kStrWelcomeTo       101
#define kStrFeeSpecial      102     /* fee sentence lead-ins */
#define kStrFeeBig          103
#define kStrFeeSmall        104
#define kStrFeeTail         105

#define kSpobFlagBig        0x10    /* selects the 94-96 greeting set (name TBC) */
#define kSpobFlagUninhabited 0x20
#define kSpob2FlagSpecial   0x1000  /* selects the 91-93 greeting set (name TBC) */

/* HandlePlayerDockApproach. STATE: MATCH 675/675, byte-identical yes (2679 bytes),
   relocregcheck 0 of 146. OPEN: none -- `inhabited = !(...)' then `canDock = 0' before the
   govt read is what places the entry compare's registers; see notes. */
// FUNCTION: EVNOVA 0x00459950
void HandlePlayerDockApproach(void)
{
    ShipObj *me;
    long targ;
    short govt;
    char inhabited;
    char canDock;
    short i;
    short dx;
    short dy;
    short r;
    char tmp[256];
    char msg[256];

    me = shipData;
    targ = dockTarget;
    if (stellarData[targ].system == shipData[0].curSystem) {
        if (dockPermission <= -1)
            goto out;
        inhabited = !(stellarData[targ].flags & kSpobFlagUninhabited);
        canDock = 0;
        govt = stellarData[targ].govt;
        if (govt == -1)
            canDock = 1;
        else if (govt != -1) {
            if ((playerLegalStatus[me->curSystem] >= stellarData[targ].minStatus ||
                 stellarData[targ].minStatus == -32767) &&
                stellarData[targ].minStatus != 32767)
                canDock = 1;
        }
        if (stellarData[targ].dominated)
            canDock = 1;
        if (dockPermission >= 750)
            canDock = 1;
        if (!canDock) {
            for (i = 0; i < 16; i++) {
                if (currentMissionObjectives[i].active) {
                    if (shipData[0].targetStellar == currentMissions[i].travelStellar)
                        canDock = 1;
                    if (shipData[0].targetStellar == currentMissions[i].returnStellar)
                        canDock = 1;
                }
            }
        }
        if (!canDock && govt != -1 && PlayerBlessedByGovt(govt, 1))
            canDock = 1;
        if (inhabited && canDock) {
            if (dockPermission < 749) ;
            else
                dockPermission++;
            targ = shipData[0].targetStellar;
            dx = abs((short)(me->pos.x - stellarData[targ].pos.h));
            dy = abs((short)(me->pos.y - stellarData[targ].pos.v));
            if (dx < 250 && dy < 250 && dockPermission < 750)
                dockPermission = 750;
            if (dockPermission != 750) ;
            else {
                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                strcpy(msg, "");
                targ = shipData[0].targetStellar;
                if (stellarData[targ].flags2 & kSpob2FlagSpecial) {
                    r = Rand(3);
                    if (r == 0) {
                        AppendIndString(msg, kGameStrList, kStrGreetSpecialA);
                        strcat(msg, ", ");
                        strncpy(tmp, (char *)shipName, 0x40);
                        PtoCstr((unsigned char *)tmp);
                        strcat(msg, tmp);
                        strcat(msg, ". ");
                    } else if (r == 1) {
                        strncpy(msg, (char *)shipName, 0x40);
                        PtoCstr((unsigned char *)msg);
                        strcat(msg, ", ");
                        AppendIndString(msg, kGameStrList, kStrGreetSpecialB);
                    } else
                        AppendIndString(msg, kGameStrList, kStrGreetSpecialC);
                    CtoPstr(msg);
                } else {
                    if (stellarData[targ].flags & kSpobFlagBig) {
                        r = Rand(3);
                        if (r == 0) {
                            AppendIndString(msg, kGameStrList, kStrGreetBigA);
                            strcat(msg, ", ");
                            strncpy(tmp, (char *)shipName, 0x40);
                            PtoCstr((unsigned char *)tmp);
                            strcat(msg, tmp);
                            strcat(msg, ". ");
                        } else if (r == 1) {
                            strncpy(msg, (char *)shipName, 0x40);
                            PtoCstr((unsigned char *)msg);
                            strcat(msg, ", ");
                            AppendIndString(msg, kGameStrList, kStrGreetBigB);
                            strcat(msg, " ");
                        } else {
                            AppendIndString(msg, kGameStrList, kStrGreetBigC);
                            strcat(msg, " ");
                        }
                    } else {
                        r = Rand(3);
                        if (r == 0) {
                            AppendIndString(msg, kGameStrList, kStrGreetSmallA);
                            strcat(msg, ", ");
                            strncpy(tmp, (char *)shipName, 0x40);
                            PtoCstr((unsigned char *)tmp);
                            strcat(msg, tmp);
                            strcat(msg, ". ");
                        } else if (r == 1) {
                            strncpy(msg, (char *)shipName, 0x40);
                            PtoCstr((unsigned char *)msg);
                            strcat(msg, ", ");
                            AppendIndString(msg, kGameStrList, kStrGreetSmallB);
                            strcat(msg, " ");
                        } else {
                            AppendIndString(msg, kGameStrList, kStrGreetSmallC);
                            strcat(msg, " ");
                        }
                    }
                    r = Rand(2);
                    if (r == 0) {
                        AppendIndString(msg, kGameStrList, kStrWelcomeShort);
                    } else {
                        AppendIndString(msg, kGameStrList, kStrWelcomeTo);
                        strcat(msg, " ");
                        strncpy(tmp, (char *)stellarData[shipData[0].targetStellar].name, 0x1f);
                        PtoCstr((unsigned char *)tmp);
                        strcat(msg, tmp);
                        strcat(msg, ". ");
                    }
                    if (stellarData[dockTarget].landingFee > 0 &&
                        stellarData[dockTarget].dominated == 0) {
                        strcat(msg, "  ");
                        if (stellarData[dockTarget].flags2 & kSpob2FlagSpecial)
                            AppendIndString(msg, kGameStrList, kStrFeeSpecial);
                        else if (stellarData[dockTarget].flags & kSpobFlagBig)
                            AppendIndString(msg, kGameStrList, kStrFeeBig);
                        else
                            AppendIndString(msg, kGameStrList, kStrFeeSmall);
                        strcat(msg, " ");
                        StrcatCommaNumber(stellarData[dockTarget].landingFee, msg);
                        strcat(msg, " ");
                        strcat(msg, creditsCString);
                        AppendIndString(msg, kGameStrList, kStrFeeTail);
                    }
                    CtoPstr(msg);
                }
                DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
            }
            if (dockPermission >= 0x800) {
                dockPermission = -1;
                dockTarget = -1;
            }
        } else {
            if (!canDock)
                goto out;
            if (dockPermission < 749) ;
            else
                dockPermission++;
            targ = dockTarget;
            dx = abs((short)(me->pos.x - stellarData[targ].pos.h));
            dy = abs((short)(me->pos.y - stellarData[targ].pos.v));
            if (dx < 250 && dy < 250 && dockPermission < 750)
                dockPermission = 750;
            if (dockPermission == 750 && dockTryFlag) {
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                GetIndString((unsigned char *)msg, kGameStrList, kStrAutoClearance);
                DisplayComm((unsigned char *)msg, 240, 0, 12, &rgbWhite, 0, 0);
            }
            if (dockPermission >= 0x800) {
                dockPermission = -1;
                dockTarget = -1;
            }
        }
    } else {
        dockTarget = -1;
        dockPermission = -1;
    }
out: ;
}

/* STR# 2002 indexes */
#define kStrTookCargo       106     /* "You pick up the" */
#define kStrCargoSome       107
#define kStrCargoTail       108
#define kStrDropoffDone     126
#define kStrEscortJoins     127
#define kStrRefuseYield     128
#define kStrRefuseYieldAlt  129
#define kStrCannotBoard     130
#define kStrNotAligned      131
#define kStrTooFast         132

#define kPersFlagOffersMission  0x200
#define kPersFlagDeleteOnOffer  0x100
#define kShipFlagBigBonus       0x10    /* picks the 0.1 armor refit (name TBC) */
#define kMisnFlagBoardShip      0x1     /* flags55 bit 0 (name TBC) */
#define kMisnFlagShipName       0x8     /* flags55 bit 3 (name TBC) */


                                                            /* 0x004694a0 fighter bay */
                                                            /* 0x00466fc0 */

/* per-mission state, stride 0x14 */

/* HandlePlayerBoardAttempt (mac _HandlePlayerBoardAttempt). STATE: MATCH 100%, exe 1355 / obj 1355,
   byte-identical yes (relocs masked). OPEN: none. */
// FUNCTION: EVNOVA 0x0045a3d0
void HandlePlayerBoardAttempt(void)
{
    extern char *strcpy(char *dst, const char *src);   /* 0x004d6010 MSL, CALLED not inlined */

    char skip;
    unsigned char pbuf[256];
    char msg[256];
    float limit;
    float dist;
    float limit2;
    ShipObj *me;
    char canBoard;
    short j;
    short m;
    short k;

    me = shipData;
    keyPressedThisFrame = 1;
    if (boardTryFlag)
        goto out;
    if (me->targetShip == -1)
        goto out;
    if (ShipIsCloaked(me))
        goto out;

    boardTryFlag = 1;
    skip = 0;

    if (shipData[me->targetShip].boarded != 0 &&
        (shipData[me->targetShip].specialMission != -1 || shipData[me->targetShip].formerEscort < 0)) {
reject:
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        GetIndString((unsigned char *)msg, kGameStrList, kStrCannotBoard);
        DisplayComm((unsigned char *)msg, 360, 0, 12, &rgbWhite, 0, 0);
        goto out;
    }
    if (!IsDisabled(&shipData[me->targetShip]))
        goto reject;
    if (shipData[me->targetShip].active == 0)
        goto reject;
    if (me->curSystem != shipData[me->targetShip].curSystem)
        goto reject;
    if (shipData[me->targetShip].persType == kPersCaptainHector)
        goto reject;
    if (IsDying(me))
        goto reject;

    dist = fabs(shipData[me->targetShip].vel.x - me->vel.x);
    if (dist <= 0.5) ; else
        goto tooFast;
    dist = fabs(shipData[me->targetShip].vel.y - me->vel.y);
    if (dist <= 0.5) ; else
        goto tooFast;

    limit = GetSpriteXSize((SpriteRec *)shipData[me->targetShip].sprite) * 0.5;
    dist = fabs(shipData[me->targetShip].pos.x - me->pos.x);
    if (dist <= limit) ; else
        goto notClose;
    limit2 = GetSpriteYSize((SpriteRec *)shipData[me->targetShip].sprite) * 0.5;
    dist = fabs(shipData[me->targetShip].pos.y - me->pos.y);
    if (dist <= limit2) ; else
        goto notClose;

    if (abs(AngularDifference(me->heading, shipData[me->targetShip].heading)) > 30) {
        if (abs(AngularDifference(me->heading,
                ((short)shipData[me->targetShip].heading + 180) % 360)) > 30)
            goto out;
    }

    if (shipData[me->targetShip].specialMission != -1) {
        m = shipData[me->targetShip].specialMission;
        if (currentMissionObjectives[m].active) {
            if (currentMissions[m].pickupMode == 2) {
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                canBoard = DoMissionCargoPickup(currentMissions[m].cargoType, currentMissions[m].cargoQty);
                SetAllKeyCmdFlags();
                if (canBoard) {
                    if (shipData[me->targetShip].specialMission != -1 &&
                        currentMissionObjectives[shipData[me->targetShip].specialMission].active)
                        currentMissions[shipData[me->targetShip].specialMission].specialShipsBoarded++;
                    ClearTargetsOnShip(&shipData[me->targetShip]);
                    shipData[me->targetShip].boarded = 1;
                    currentMissions[m].hasCargo = 1;
                    statusRedraw[2] = 1;
                    strcpy(msg, "");
                    AppendIndString(msg, kGameStrList, kStrTookCargo);
                    strcat(msg, " ");
                    if (lcCargoName[currentMissions[m].cargoType][1] != '*') {
                        AppendIndString(msg, kGameStrList, kStrCargoSome);
                        strcat(msg, " ");
                    }
                    pstrcpy(pbuf, lcCargoName[currentMissions[m].cargoType]);
                    PtoCstr(pbuf);
                    if (lcCargoName[currentMissions[m].cargoType][1] == '*')
                        strcat(msg, (char *)pbuf + 1);
                    else
                        strcat(msg, (char *)pbuf);
                    strcat(msg, " ");
                    AppendIndString(msg, kGameStrList, kStrCargoTail);
                    CtoPstr(msg);
                    DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else {
                    skip = 1;
                }
            } else if ((currentMissions[m].shipGoal == 2 || currentMissions[m].shipGoal == 5) &&
                       (currentMissions[m].flags & kMisnFlagBoardShip) &&
                       currentMissions[m].shipCount == 1) {
                if (currentMissions[m].flags & kMisnFlagShipName) {
                    pstrcpy((unsigned char *)msg, gShipTypes[shipData[me->targetShip].type].name);
                    PtoCstr((unsigned char *)msg);
                    strcat(msg, ":  ");
                    pstrcpy(pbuf, fuelString[0]);
                    PtoCstr(pbuf);
                    strcat(msg, (char *)pbuf);
                    strcat(msg, ", ");
                    pstrcpy(pbuf, shipName);
                    PtoCstr(pbuf);
                    strcat(msg, (char *)pbuf);
                    strcat(msg, ".");
                } else {
                    strcpy(msg, "");
                    AppendIndString(msg, kGameStrList, kStrDropoffDone);
                }
                CtoPstr(msg);
                DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                if (currentMissionObjectives[shipData[me->targetShip].specialMission].active)
                    currentMissions[shipData[me->targetShip].specialMission].specialShipsBoarded++;
                shipData[me->targetShip].aiTimer = 100.0;
                shipData[me->targetShip].boarded = 1;
                ClearTargetsOnShip(&shipData[me->targetShip]);
                canBoard = 1;
            } else {
                canBoard = 1;
            }
        } else {
            canBoard = 1;
        }
    } else {
        if (gShipTypes[shipData[me->targetShip].type].crew > 0)
            canBoard = 1;
        else
            canBoard = 0;
    }

    if (canBoard == 0)
        goto cantBoard;
    {
        me->vel.x = shipData[me->targetShip].vel.x;
        me->vel.y = shipData[me->targetShip].vel.y;
        j = 0;
        k = me->targetShip;
        m = shipData[k].specialMission;
        if (m == -1)
            j = 1;
        else if (currentMissionObjectives[m].active && currentMissionObjectives[m].failed)
            j = 1;
        if (j == 1 && (j = shipData[me->targetShip].persType) != kPersCaptainHector && j != kPersBountyHunter) {
            SlapWithPenalty(shipData[0].curSystem, shipData[me->targetShip].govt, 2, m);
            if (shipData[me->targetShip].persType != -1) {
                if (personData[shipData[me->targetShip].persType].linkMission != -1) {
                    if (personData[shipData[me->targetShip].persType].flags &
                        kPersFlagOffersMission) {
                        offeringFromShip = 1;
                        offeringShipIndex = me->targetShip;
                        if (IsMissionAvailable(personData[shipData[me->targetShip].persType].linkMission, 1)) {
                            HideCursorSprite();
                            RefreshSAT();
                            ShowTheCursor();
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            if (OfferOneMission(personData[shipData[me->targetShip].persType].linkMission) != 1)
                                skip = 1;
                            SetAllKeyCmdFlags();
                            RefreshSAT();
                            if (personData[shipData[me->targetShip].persType].flags &
                                kPersFlagDeleteOnOffer)
                                personData[shipData[me->targetShip].persType].loaded = 0;
                        } else {
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            HideCursorSprite();
                            RefreshSAT();
                            DoPlunderDialog();
                            SetAllKeyCmdFlags();
                        }
                        offeringFromShip = 0;
                        offeringShipIndex = -1;
                    } else {
                        if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                            shipData[me->targetShip].formerEscort == 0) {
                            GetIndString((unsigned char *)msg, kGameStrList, kStrRefuseYield);
                            DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                            PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                            shipData[me->targetShip].formerEscort = -1;
                            shipData[me->targetShip].aiType = 5;
                            shipData[me->targetShip].parentShip = 0;
                            shipData[me->targetShip].shield = 0;
                            shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                            shipData[me->targetShip].targetStellar = -1;
                            AIAddFighterToParent(&shipData[me->targetShip]);
                        } else {
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            HideCursorSprite();
                            RefreshSAT();
                            DoPlunderDialog();
                            SetAllKeyCmdFlags();
                        }
                    }
                } else {
                    if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                        shipData[me->targetShip].formerEscort == 0) {
                        GetIndString((unsigned char *)msg, kGameStrList, kStrRefuseYield);
                        DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                        PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                        shipData[me->targetShip].formerEscort = -1;
                        shipData[me->targetShip].aiType = 5;
                        shipData[me->targetShip].parentShip = 0;
                        shipData[me->targetShip].shield = 0;
                        shipData[me->targetShip].targetStellar = -1;
                        shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                        AIAddFighterToParent(&shipData[me->targetShip]);
                    } else {
                        DoGoodSamaritan(&shipData[me->targetShip], me);
                        PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                        HideCursorSprite();
                        RefreshSAT();
                        DoPlunderDialog();
                        SetAllKeyCmdFlags();
                    }
                }
            } else {
                if (shipData[me->targetShip].formerEscort >= 1 && CanHireEscorts()) {
                    shipData[me->targetShip].aiType = 6;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].boarded = 0;
                    if (shipData[me->targetShip].formerEscort == 1)
                        shipData[me->targetShip].hired = 1;
                    else
                        shipData[me->targetShip].hired = 0;
                    shipData[me->targetShip].formerEscort = -1;
                    if (gShipTypes[shipData[me->targetShip].type].flags & kShipFlagBigBonus)
                        shipData[me->targetShip].armor =
                            ShipArmorCapacity(&shipData[me->targetShip]) * 0.1 + 1.0;
                    else
                        shipData[me->targetShip].armor =
                            ShipArmorCapacity(&shipData[me->targetShip]) * 0.3333 + 1.0;
                    AISetup(&shipData[me->targetShip]);
                    for (k = 0; k < 6; k++)
                        shipData[0].cargo[k] += shipData[me->targetShip].cargo[k];
                    statusRedraw[2] = 1;
                    GetIndString((unsigned char *)msg, kGameStrList, kStrEscortJoins);
                    DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                           shipData[me->targetShip].formerEscort == 0) {
                    GetIndString((unsigned char *)msg, kGameStrList, kStrRefuseYield);
                    DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                    shipData[me->targetShip].formerEscort = -1;
                    shipData[me->targetShip].aiType = 5;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].shield = 0;
                    shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                    shipData[me->targetShip].targetStellar = -1;
                    AIAddFighterToParent(&shipData[me->targetShip]);
                } else if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                           shipData[me->targetShip].formerEscort == -1) {
                    GetIndString((unsigned char *)msg, kGameStrList, kStrRefuseYieldAlt);
                    DisplayComm((unsigned char *)msg, 250, 0, 12, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                    shipData[me->targetShip].formerEscort = -1;
                    shipData[me->targetShip].aiType = 5;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].shield = 0;
                    shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                    shipData[me->targetShip].targetStellar = -1;
                    AIAddFighterToParent(&shipData[me->targetShip]);
                } else {
                    DoGoodSamaritan(&shipData[me->targetShip], me);
                    PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                    HideCursorSprite();
                    RefreshSAT();
                    DoPlunderDialog();
                    SetAllKeyCmdFlags();
                }
            }
        } else if (m == -1) {
            DoGoodSamaritan(&shipData[me->targetShip], me);
            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
            HideCursorSprite();
            RefreshSAT();
            DoPlunderDialog();
            SetAllKeyCmdFlags();
        }
        if (skip)
            goto out;
        if (me->targetShip == -1)
            goto out;
        shipData[me->targetShip].boarded = 1;
        ClearTargetsOnShip(&shipData[me->targetShip]);
    }
    goto out;

cantBoard:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString((unsigned char *)msg, kGameStrList, kStrCannotBoard);
    DisplayComm((unsigned char *)msg, 360, 0, 12, &rgbWhite, 0, 0);
    goto out;
notClose:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString((unsigned char *)msg, kGameStrList, kStrNotAligned);
    DisplayComm((unsigned char *)msg, 360, 0, 12, &rgbWhite, 0, 0);
    goto out;
tooFast:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString((unsigned char *)msg, kGameStrList, kStrTooFast);
    DisplayComm((unsigned char *)msg, 360, 0, 12, &rgbWhite, 0, 0);
out:;
}

/*
 * WrapNearbyObject skips anything further than 15000 units from the
 * player, which is why the shift is applied to the player LAST. beamData is
 * the odd one out: its endpoints are shorts, not floats, so the shift is
 * written inline and each store carries the float-to-int rounding correction
 * mwcc inlines for a cast at level=4.
 */

/* ONE FloatPoint, not two floats: the mac names _wrapDelta at +0/+4, and the
   aggregate spelling is what pins the two zeroing stores after the frame
   setup - as two separate externs mwcc pulls them up into the register
   pushes and the prologue splits. */

#define kWrapTrigger  15000.0f
#define kWrapStep     25000.0f

#define kMaxSmoke     0x40

/* WrapPlayerPosition. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045baa0
void WrapPlayerPosition(void)
{
    short i;
    char wrapped;

    wrapDelta.x = 0.0f;
    wrapDelta.y = 0.0f;
    wrapped = 0;
    if (shipData->pos.x > kWrapTrigger) {
        wrapped = 1;
        wrapDelta.x = -kWrapStep;
    } else if (shipData->pos.x < -kWrapTrigger) {
        wrapped = 1;
        wrapDelta.x = kWrapStep;
    }
    if (shipData->pos.y > kWrapTrigger) {
        wrapped = 1;
        wrapDelta.y = -kWrapStep;
    } else if (shipData->pos.y < -kWrapTrigger) {
        wrapped = 1;
        wrapDelta.y = kWrapStep;
    }
    if (!wrapped)
        return;

    for (i = 1; i < kMaxShips; i++)
        WrapNearbyObject(&shipData[i].pos.x);
    for (i = 0; i < kMaxShots; i++)
        WrapNearbyObject(&shotData[i].pos.x);
    for (i = 0; i < kMaxExplods; i++)
        WrapNearbyObject(&explodData[i].pos.x);
    for (i = 0; i < kMaxBoxes; i++)
        WrapNearbyObject(&boxData[i].pos.x);
    for (i = 0; i < kMaxSmoke; i++)
        WrapNearbyObject(&gDebrisSlots[i].pos.x);
    for (i = 0; i < kMaxPods; i++)
        WrapNearbyObject(&podData[i].pos.x);

    for (i = 0; i < kMaxBeams; i++) {
        beamData[i].head.h += wrapDelta.x;
        beamData[i].head.v += wrapDelta.y;
        beamData[i].tail.h += wrapDelta.x;
        beamData[i].tail.v += wrapDelta.y;
    }

    shipData->pos.x += wrapDelta.x;
    shipData->pos.y += wrapDelta.y;
    lastFramePlayerPos.x += wrapDelta.x;
    lastFramePlayerPos.y += wrapDelta.y;
}

#define kWrapRange 15000

/* WrapNearbyObject. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045c6d0
void WrapNearbyObject(float *pos)
{
    long dx;
    long dy;

    dx = (pos[0] - shipData->pos.x);
    dy = (pos[1] - shipData->pos.y);
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (dx < kWrapRange && dy < kWrapRange) {
        pos[0] += wrapDelta.x;
        pos[1] += wrapDelta.y;
    }
}

/*
 * The flags keep DAT_ names on purpose: the mac reaches each one through its own
 * pointer slot, and those slots are not in the 1.1.1 symbol table. The two tail
 * loops are the mac's - its twin walks 0xdee1c for 5 bytes and then for 4.
 */

/* SetAllKeyCmdFlags. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045c7a0
void SetAllKeyCmdFlags(void)
{
    short i;

    navModeSelectFlag = 1;
    navTargetSelectFlag = 1;
    navDefaultSelectFlag = 1;
    helpKeyFlag = 1;
    cloakFlag = 1;
    fighterCommandFlag = 1;
    weapModeSelectFlag = 1;
    weapTargetSelectFlag = 1;
    weapDefaultSelectFlag = 1;
    playerInfoFlag = 1;
    mapTryFlag = 1;
    jumpTryFlag = 1;
    weapSelectFlag = 1;
    spawnShipFlag = 1;
    weapCloseTargFlag = 1;
    missionInfoFlag = 1;
    commTryFlag = 1;
    mapZoomFlag = 1;
    framerateFlag = 1;
    escortMenuFlag = 1;
    pauseKeyFlag = 1;
    ackCommFlag = 1;
    for (i = 0; i < 5; i++)
        escortGroupFlag[i] = 1;
    for (i = 0; i < 4; i++)
        escortGroupFlag[i] = 1;
}

#define kStrEscortPrefix    0x86   /* + the AI-type filter's own name */
#define kStrFilterFighters  0x87
#define kStrFilterWarships  0x88
#define kStrFilterFreighters 0x89
#define kStrFilterEscorts   0x8a
#define kStrFilterAll       0x8b
#define kStrLoudBase        0x96   /* the five orders, said out loud */
#define kStrQuietBase       0x9b   /* the same five, said under fire */
#define kStrAckAttack       0x9a
#define kStrAckAttackEscort 0x9f

#define kShipTypeFighterBay 5      /* aiType == 5 launches from a bay */

#define kCommMessageID      0xfa
#define kCommHoldTicks      0xc

/* IssueNewEscortCommand (mac _IssueNewEscortCommand). STATE: MATCH 100.0%,
   exe 409 / obj 409 instrs, byte-identical yes.
   OPEN: nothing. [esp+4] is mwcc's CSE spill of `(long)cmd`, not a local; see notes */
// FUNCTION: EVNOVA 0x0045c880
char IssueNewEscortCommand(short aiTypeFilter, short cmd, char quiet)
{
    short spoken;
    short i;
    char sawVoice;
    short voiceClass;
    short govt;
    char changed;
    Str255 msg;

    changed = 0;
    sawVoice = 0;
    spoken = cmd;
    govt = -2;
    voiceClass = -1;
    i = 1;
    for (; i < kMaxShips; i++) {
        if (shipData[i].active == 0)
            continue;
        if (shipData[i].parentShip != 0)
            continue;
        if (aiTypeFilter != gShipTypes[shipData[i].type].escortType &&
            aiTypeFilter != -1)
            continue;
        if (cmd != shipData[i].escortOrder) {
            shipData[i].escortMayAttack = 1;
            if (shipData[i].aiType != kShipTypeFighterBay) {
                if ((long)cmd == kOrderRegroup) {
                    if ((long)shipData[i].escortOrder != kOrderFormation) {
                        shipData[i].escortOrder = kOrderFormation;
                        spoken = kOrderFormation;
                        changed = 1;
                        if ((gShipTypes[shipData[i].type].flags2 &
                             kShipFlagNoVoice) == 0)
                            sawVoice = 1;
                    }
                } else {
                    shipData[i].escortOrder = cmd;
                    changed = 1;
                    if ((gShipTypes[shipData[i].type].flags2 & kShipFlagNoVoice) == 0)
                        sawVoice = 1;
                }
            } else {
                shipData[i].escortOrder = cmd;
                changed = 1;
                if ((gShipTypes[shipData[i].type].flags2 & kShipFlagNoVoice) == 0)
                    sawVoice = 1;
            }
        }
        if (shipData[i].escortOrder == kOrderAttack && shipData[0].targetShip != -1 &&
            shipData[0].targetShip != shipData[i].targetShip &&
            AlliedWithPlayer(&shipData[shipData[0].targetShip]) == 0) {
            shipData[i].targetShip = shipData[0].targetShip;
            changed = 1;
        }
        if ((long)cmd != kOrderRegroup) {
            if (AIIsFighterReturning(&shipData[i]) != 0) {
                shipData[i].targetShip = -1;
                shipData[i].targetStellar = -1;
                if (govt < -1 || Rand(2) == 0) {
                    govt = gShipTypes[shipData[i].type].inherentGovtB;
                    voiceClass = shipData[i].voiceClass;
                }
                changed = 1;
            }
        }
        if (changed != 0 &&
            (gShipTypes[shipData[i].type].flags2 & kShipFlagNoVoice) == 0 &&
            (govt < -1 || Rand(2) == 0)) {
            govt = gShipTypes[shipData[i].type].inherentGovtB;
            voiceClass = shipData[i].voiceClass;
        }
    }
    if (changed != 0 && quiet == 0) {
        strcpy((char *)msg, "");
        AppendIndString((char *)msg, kGameStrList, kStrEscortPrefix);
        if (aiTypeFilter == 0)
            AppendIndString((char *)msg, kGameStrList, kStrFilterFighters);
        else if (aiTypeFilter == 1)
            AppendIndString((char *)msg, kGameStrList, kStrFilterWarships);
        else if (aiTypeFilter == 2)
            AppendIndString((char *)msg, kGameStrList, kStrFilterFreighters);
        else if (aiTypeFilter == 3)
            AppendIndString((char *)msg, kGameStrList, kStrFilterEscorts);
        else
            AppendIndString((char *)msg, kGameStrList, kStrFilterAll);
        strcat((char *)msg, " ");
        if (shipData[0].jumpTimer > 0.0f) {
            if ((long)spoken == kOrderRegroup)
                AppendIndString((char *)msg, kGameStrList, kStrLoudBase);
            else if ((long)spoken == kOrderFormation)
                AppendIndString((char *)msg, kGameStrList, kStrLoudBase + 1);
            else if ((long)spoken == kOrderHold)
                AppendIndString((char *)msg, kGameStrList, kStrLoudBase + 2);
            else if ((long)spoken == kOrderDefend)
                AppendIndString((char *)msg, kGameStrList, kStrLoudBase + 3);
            else if ((long)spoken == kOrderAttack)
                AppendIndString((char *)msg, kGameStrList, kStrAckAttack);
        } else {
            if ((long)spoken == kOrderRegroup)
                AppendIndString((char *)msg, kGameStrList, kStrQuietBase);
            else if ((long)spoken == kOrderFormation)
                AppendIndString((char *)msg, kGameStrList, kStrQuietBase + 1);
            else if ((long)spoken == kOrderHold)
                AppendIndString((char *)msg, kGameStrList, kStrQuietBase + 2);
            else if ((long)spoken == kOrderDefend)
                AppendIndString((char *)msg, kGameStrList, kStrQuietBase + 3);
            else if ((long)spoken == kOrderAttack) {
                if (shipData[0].targetShip == -1)
                    AppendIndString((char *)msg, kGameStrList, kStrAckAttack);
                else if (shipData[shipData[0].targetShip].parentShip == 0)
                    AppendIndString((char *)msg, kGameStrList, kStrAckAttack);
                else
                    AppendIndString((char *)msg, kGameStrList, kStrAckAttackEscort);
            }
        }
        CtoPstr((char *)msg);
        DisplayComm(msg, kCommMessageID, 0, kCommHoldTicks, &rgbWhite, 0, 0);
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (sawVoice != 0) {
            if (govt < -1)
                govt = -1;
            if ((long)spoken == kOrderAttack) {
                if (shipData[0].targetShip == -1)
                    SpeakVoiceMessage(0, govt, voiceClass);
                else if (shipData[shipData[0].targetShip].parentShip == 0)
                    SpeakVoiceMessage(0, govt, voiceClass);
                else
                    SpeakVoiceMessage(1, govt, voiceClass);
            } else {
                SpeakVoiceMessage(0, govt, voiceClass);
            }
        }
    }

    return changed;
}

}
