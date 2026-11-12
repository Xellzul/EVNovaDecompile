// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
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
#include "cefix.h"

#ifdef MACBUILD
#ifdef MAC1010
#define RT3_HELPERS_IN_UNIT
#endif
#include "../mac/include/rt3_ev_nova.h"
#ifdef MAC1010
/* -inline auto expands the call; this copy ranks `table` ahead of the unit's globals, and the linker strips it */
static void RegScramB2(unsigned char x)
{
    static unsigned char table[256];

    table[x]++;
}
#endif

extern void SetRetainedWindow(WindowRef window, Boolean retained);
extern void AIShipBoardedByPlayer(ShipObj *ship);
extern void FadeWhiteOut(void);
extern void FadeWhiteIn(void);
extern Boolean QuickKeyCheckPlusEscorts(short code);
#else
/* the port keeps no retained-window state */
#define SetRetainedWindow(window, retained) ((void)(window))
#endif

/* keys HandlePlayer reads by code: win scan codes, mac virtual key codes */
#ifdef MACBUILD
#define kScanKey1           MacKey(0x12)
#define kScanKey2           MacKey(0x13)
#define kScanKey3           MacKey(0x14)
#define kScanKey4           MacKey(0x15)
#define kScanKey5           MacKey(0x17)
#define kScanF5             MacKey(0x60)
#define kScanShift          MacKey(0x38)
#define kScanCommand        MacKey(0x37)
#define kScanExploreCheat   MacKey(0x51)
#define kScanAttackCheat    MacKey(0x4b)
#define kScanWarpCheat      MacKey(0x41)
#define kScanSpawnCheat     MacKey(0x61)
#define kScanFleetCheat     kScanCommand
#define kScanKillCheat      MacKey(0x62)
#ifdef MAC1010
#define kScanKillCheat2     MacKey(0x47)
#else
#define kScanKillCheat2     MacKey(0x33)
#endif
#define kScanArmorCheat     MacKey(0x64)
#define kScanDamageCheat    MacKey(0x65)
#define kScanCheatsOff      MacKey(0x69)
#define kScanInfoCheat      MacKey(0x6d)
#define kScanMinus          MacKey(0x1b)
#define kScanPadMinus       MacKey(0x4e)
#define kScanEquals         MacKey(0x18)
#define kScanPadPlus        MacKey(0x45)
#define AltKeyQuick()       QuickKeyCheck(kScanOption)
#define CmdKeyQuick()       QuickKeyCheck(kScanCommand)
#define ShiftKeyQuick()     QuickKeyCheck(kScanShift)
#define CtrlKeyQuick()      QuickKeyCheck(kScanControl)
#else
#define kScanKey1           0x02
#define kScanKey2           0x03
#define kScanKey3           0x04
#define kScanKey4           0x05
#define kScanKey5           0x06
#define kScanF5             0x3f
#define kScanExploreCheat   0x72
#define kScanAttackCheat    0x6c
#define kScanWarpCheat      0x53
#define kScanSpawnCheat     0x40
#define kScanFleetCheat     0x30
#define kScanKillCheat      0x41
#define kScanKillCheat2     0x45
#define kScanArmorCheat     0x42
#define kScanDamageCheat    0x43
#define kScanCheatsOff      0x6d
#define kScanMinus          0x0c
#define kScanPadMinus       0x4a
#define kScanEquals         0x0d
#define kScanPadPlus        0x4e
#define AltKeyQuick()       (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
#define CmdKeyQuick()       (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
#define ShiftKeyQuick()     (QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
#define CtrlKeyQuick()      (QuickKeyCheck(kScanLCtrl) || QuickKeyCheck(kScanRCtrl))
#endif

Boolean warpSoundDone;
Boolean playerTurnedThisFrame;
Boolean dontMovePlayerAfterLanding;
Boolean abOn;
short newbieFlag;
float sabotageCounter;
#ifndef MACBUILD
unsigned char DAT_007cab24[4];
#endif
Boolean dockLand;
Boolean dockTryFlag;
Boolean boardTryFlag;
#ifndef MACBUILD
Boolean DAT_007cab2b;
#endif
FloatPoint wrapDelta;
Boolean lastJumpDistOk;
Boolean ackCommFlag;
Boolean navModeSelectFlag;
Boolean navTargetSelectFlag;
Boolean navDefaultSelectFlag;
Boolean helpKeyFlag;
Boolean cloakFlag;
Boolean fighterCommandFlag;
Boolean weapModeSelectFlag;
Boolean weapTargetSelectFlag;
Boolean weapDefaultSelectFlag;
Boolean playerInfoFlag;
Boolean mapTryFlag;
Boolean jumpTryFlag;
Boolean weapSelectFlag;
Boolean spawnShipFlag;
Boolean weapCloseTargFlag;
Boolean missionInfoFlag;
Boolean commTryFlag;
Boolean mapZoomFlag;
Boolean framerateFlag;
Boolean escortMenuFlag;
Boolean escortGroupFlag[5];
Boolean escortCommandFlag[4];
Boolean pauseKeyFlag;
short selEscortGroup;
short lastEscortCount;
#ifndef MACBUILD
unsigned char DAT_007cab58[16];
#endif

EXTERN_C_BEGIN

/* the sprite record the engine hangs off ShipObj.unk0 and stellarSprite[];
   gameloop.c's CursorSpriteRec is the same object -- only the three fields
   this TU hit-tests are transcribed */

/* SpriteWorldRec +0x1c is the third SWFrameRec in the arrival blit, so the
   macro casts it. */
#define SWWindowFrame(sw)   ((sw)->windowFrameP)
#define SWWindowRect(sw)    (&(sw)->windRect)

#define kOutfModType47      0x2f
#define kOutfModType50      0x32

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

#ifdef MACBUILD
#define mapLink i
#else
#define mapLink j
#endif

/* HandlePlayer (mac _HandlePlayer). STATE: win MATCH 100%, exe 9548 / obj 9548, frame 0x29c,
   byte-identical yes (relocregcheck 0). mac MATCH on both slices.
   OPEN: none; the V111 `ammo` in the third ammo loop is a spelling with no witness (notes). */
// FUNCTION: EVNOVA 0x0044aa70
// MAC: _HandlePlayer i386 0x00068390 ppc 0x00070b28
void HandlePlayer(ShipObj *player)
{
#ifdef MACBUILD
    Boolean jumping;
    Str255 msg;
    short i;
    short j;
    short n;
    short count;
    short days;
    short fuelCap;
    short turnStep;
    float turnLim;
    float dist;
    float heading;
    float angle;
    Point pt;
    float step;
    Str255 name;
    float dmg;
    Boolean ok;
    Boolean linked[kNavSlots];
#ifdef MAC1010 /* 1.0.10 spelling */
    Rect box;
    Rect hit;
    FloatPoint pos;
    FloatPoint dest;
    FloatPoint origin;
    short groupCount[4];
#else
    FloatPoint origin;
    short groupCount[4];
    FloatPoint pos;
    Rect box;
    FloatPoint dest;
    Rect hit;
#endif
    Rect *portRect;
    Boolean canJump;
    short lost;
    short dir;
    short k;
    Boolean found;
    Boolean escortHit;
    Boolean turned;
    Boolean targetUncloaked;
    Boolean targetCloaked;
    Boolean disabled;
    Boolean dying;
    Boolean routeDone;
    Boolean flag;
    Boolean destructing;
    short sys;
    short waiting;
    short rank;
    double span;
#else
    Boolean jumping;
    Boolean turned;
    Str255 msg;
    short i;
    short j;
    short n;
    short count;
    short days;
    short fuelCap;
    short turnStep;
    float turnLim;
    float heading;
    float angle;
    float step;
    Str255 name;
    float dist;
    float dmg;
    Boolean dying;
    Boolean disabled;
    Boolean ok;
    Boolean targetUncloaked;
    Boolean targetCloaked;
    Boolean linked[kNavSlots];
    Point pt;
    Rect box;
    Rect hit;
    Rect *portRect;
    FloatPoint stelPos;
    FloatPoint pos;
    short groupCount[4];
    FloatPoint dest;
    FloatPoint origin;
    Boolean canJump;
    Boolean routeDone;
    short lost;
    short dir;
    short k;
    Boolean found;
    Boolean escortHit;
    Boolean flag;
    Boolean destructing;
    short rank;
    double span;
    short waiting;
#endif

    quickKeyCheckResult = 0;
    if (settings.windowed == 0 && settings.noHyperGamma == 0 && ignoreKeys == 0) {
        if (player->jumpTimer > 0.0f) {
            if (hyperGamma < 0.0)
                hyperGamma = 0;
            if (hyperGamma > 100.0)
                hyperGamma = 100.0f;
#if defined(MACBUILD) && V111
            if (!screenFaded && hyperGamma > 0.0)
                FadeWhiteIn();
#else
            OnTheFlyWhiteFade(hyperGamma);
#endif
        } else if (hyperGamma >= 0.0) {
            hyperGamma = -1.0f;
#ifdef MAC1010
#if TARGET_API_MAC_CARBON
            SetGamma(gScreenRef, &gOrigGamma);
#else
            GammaFadeTo(1, 100, 0);
#endif
#elif defined(MACBUILD)
            FadeWhiteOut();
#else
            FadeGammaTo(100, 0);
#endif
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
        if (player->vel.x > pos.x + step)
            player->vel.x -= step;
        if (player->vel.x < pos.x - step)
            player->vel.x += step;
        if (player->vel.y > pos.y + step)
            player->vel.y -= step;
        if (player->vel.y < pos.y - step)
            player->vel.y += step;
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
    if (player->unk6e == 1 && player->targetShip != -1) {
        if (!shipData[player->targetShip].active
                || IsDying(&shipData[player->targetShip])
                || AIShipIsEmergingFromHypergate(&shipData[player->targetShip])
                || (ShipIsCloaked(&shipData[player->targetShip])
                    && shipData[player->targetShip].parentShip != 0
                    && !ShipCanTargetCloakedShips(player))) {
            player->targetShip = -1;
            statusRedraw[kRedrawTarg] = 1;
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
                            if (outfitSystem[i].modType[j] == kOutfModType47
                                    && outfitSystem[i].modVal[j] > 0) {
                                SetRetainedWindow((WindowRef)gWind, false);
                                HideCursorSprite();
                                RefreshSAT();
                                ShowTheCursor();
                                LoadDescription(textShowString,
                                                outfitSystem[i].modVal[j]);
                                ShowTextDialog(0, 0, 1);
                                SetRetainedWindow((WindowRef)gWind, true);
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
               * shipClass[player->type].mobilityFactor / (WarpSoundDuration() * 0.01);
        step = span;
        if (shipClass[player->type].mobilityFactor != 0)
            step = step - 35.0 / shipClass[player->type].mobilityFactor;
        if (step > 0.0) {
            if (step > ShipMaxSpeed(player))
                step = ShipMaxSpeed(player);
            player->vel.x = 0;
            player->vel.y = 0;
            Accel(player->heading, step, &player->vel);
        }
        player->jumpTimer = -1.0f;
        GetIndString(msg, kGameStrList, 0x23);
        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
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
        player->vel.x *= 0.995;
        player->vel.y *= 0.995;
    } else {
        player->boarded = 0;
    }
    if (playerRepairInhibit >= 0.0)
        playerRepairInhibit = playerRepairInhibit - gSpeedMult;
    if (disabled && playerRepairInhibit < 0.0 && ShipCanSelfRepair(player)) {
        if (shipClass[player->type].flags & kShipArmorDisableTenth)
            player->armor = ShipArmorCapacity(player) * 0.1 + 1.0;
        else
            player->armor = ShipArmorCapacity(player) * 0.3333 + 1.0;
        GetIndString(msg, kGameStrList, 0x25);
        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
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
            if (settings.volume <= 1)
                alertSpritePending = 0x1e;
        }
    }

    if (player->escapePods > 0) {
        AdjustedAccel(player->heading,
                      ShipAccelRate(player) * gSpeedMult,
                      &player->vel, ShipMaxSpeed(player));
        player->pos.x += player->vel.x * gSpeedMult;
        player->pos.y += player->vel.y * gSpeedMult;
        player->escapePods -= 1;
        if (player->escapePods == 0) {
            FadeScreenOut();
            ClearStarfield();
            ClearParticles(0);
            SWHideAllSprites(gSpriteWorld);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            FadeScreenIn();
            ShowTheCursor();
            SetRetainedWindow((WindowRef)gWind, false);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            ShowTheCursor();
            LoadDescription(textShowString, 0x36af);
            ShowTextDialog(0, 0, 1);
            SetRetainedWindow((WindowRef)gWind, true);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            FadeScreenOut();
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active) {
                    AbortMission(i, 1);
                    currentMissionObjectives[i].active = 0;
                }
            }
            n = player->curSystem;
            ResetPlayer(0);
            EvalMissionBitSetString(shipClass[shipData->type].onPurchase);
            DeleteAllComputerShips(1, 1);
            j = EscapeStellarSearch(n);
            if (j == -1)
                player->curSystem = 0;
            else
                player->curSystem = stellarData[j].system;
            mapScroll.h = systemData[player->curSystem].pos.h;
            mapScroll.v = systemData[player->curSystem].pos.v;
            for (i = 0; i < kMaxWeapTypes; i++) {
                player->weap[0].count = 0;  /* BUG: clears weap[0] every pass, not weap[i] */
                player->weap[0].ammo = 0;
            }
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (shipClass[player->type].weapAmmo[i] > 0) {
                    if (weaponStats[i].ammoType >= 0
                            && weaponStats[i].ammoType < kMaxWeapTypes
                            && weaponStats[i].guidance != kGuidanceBay)
                        player->weap[weaponStats[i].ammoType].ammo =
                            shipClass[player->type].weapAmmo[i];
                    else
                        player->weap[i].ammo =
                            shipClass[player->type].weapAmmo[i];
                }
                player->weap[i].count = shipClass[player->type].weapCount[i];
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
            pstrcpy(shipName, shipClass->name);
            PtoCstr(shipName);
            strcat((char *)shipName, " ");
            for (i = 0; i <= 3; i++) {
                NumToString(Rand(9) + 1, msg);
                PtoCstr(msg);
                strcat((char *)shipName, (char *)msg);
            }
            CtoPstr((char *)shipName);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            SetSystemBackgroundColor();
            PaintRect(&SWWindowFrame(gSpriteWorld)->frameRect);
            ForeColor(blackColor);
            RefreshSAT();
            RedrawStatus(1);
            RedrawComm(0);
            statusRedraw[kRedrawTarg] = statusRedraw[kRedrawNav] = statusRedraw[kRedrawWeap] =
                statusRedraw[kRedrawFuel] = statusRedraw[kRedrawShield] = 1;
            for (i = 0; i < kMaxSystems; i++) {
                if (systemData[i].govt >= 0)
                    playerRecord[i] =
                        govtData[systemData[i].govt].initialRec;
                else
                    playerRecord[i] = 0;
            }
            if (strictPlay) {
                ok = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (systemData[player->curSystem].navDefault[i] != -1) {
                        SavePilotFile(
                            systemData[player->curSystem].navDefault[i]);
                        ok = 1;
                        break;
                    }
                }
                if (!ok)
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
            sabotageCounter = sabotageCounter + gSpeedMult;
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
                                if (outfitSystem[i].modType[j] == kOutfModType47) {
                                    if (shipSystems[i] > 1
                                            && lcPluralOutfitName[i][0]) {
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcPluralOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x29);
                                        CtoPstr((char *)msg);
                                    } else if (shipSystems[i] == 1
                                            && lcOutfitName[i][0]) {
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x28);
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
                    DisplayComm((PStrChar *)msg, 0x168, 0, kCommTextSize, &rgbWhite, 0, 0);
                } else {
                    for (i = 0; i < kMaxOutfits; i++) {
#ifdef MACBUILD
                        if (shipSystems[i] > 0) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (outfitSystem[i].modType[j] == kOutfModType50) {
                                    if (shipSystems[i] > 1) {
#else
                        count = shipSystems[i];
                        if (count > 0) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (outfitSystem[i].modType[j] == kOutfModType50) {
                                    if (count > 1) {
#endif
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcPluralOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x29);
                                        CtoPstr((char *)msg);
                                    } else {
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x28);
                                        CtoPstr((char *)msg);
                                    }
                                    DisplayComm((PStrChar *)msg, 0x190, 0, kCommTextSize, &rgbWhite, 0, 0);
                                    if (outfitSystem[i].modVal[j] >= 0x80 && outfitSystem[i].modVal[j] < 0xc0)
                                        CreateExplosion(
                                            player->pos,
                                            outfitSystem[i].modVal[j] - 0x80, 0, 1);
                                    shipSystems[i] = 0;
                                    DamageShip(player, &player->pos, 0,
                                               Rand((ShipArmorCapacity(player) * 0.5 + 1.0))
                                               + ShipArmorCapacity(player),
                                               0, -1, 0, 0, 1, 1, 1, 0);
                                    break;
                                }
                            }
                        }
                    }
                    statusRedraw[kRedrawShield] = 1;
                    ResetPlayerPrecalcedValues();
                }
            }
        }
    } else {
        sabotageCounter = 0;
    }

#ifdef MACBUILD
    if (QuickKeyCheck(keyPrefs.clearTarget) && KeyCheck(kScanOption)) {
#else
    if (QuickKeyCheck(keyPrefs.clearTarget) && (KeyCheck(kScanLAlt) || KeyCheck(kScanRAlt))) {
#endif
        keyPressedThisFrame = 1;
        if (!navModeSelectFlag) {
            navModeSelectFlag = 1;
            if (player->targetShip != -1) {
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                player->targetShip = -1;
                statusRedraw[kRedrawTarg] = 1;
            }
        }
    }
#ifdef MACBUILD
    if (((QuickKeyCheck(keyPrefs.clearTarget) && !KeyCheck(kScanOption))
#else
    if (((QuickKeyCheck(keyPrefs.clearTarget) && !KeyCheck(kScanLAlt) && !KeyCheck(kScanRAlt))
#endif
            || QuickKeyCheck(keyPrefs.hyperspace))
        && player->jumpTimer <= 0.0f) {
        keyPressedThisFrame = 1;
        if (!navModeSelectFlag) {
            navModeSelectFlag = 1;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            statusRedraw[kRedrawNav] = 1;
            dockTarget = -1;
            dockPermission = -1;
            if (QuickKeyCheck(keyPrefs.clearTarget)) {
                player->navTargetKind = -1;
                player->targetStellar = -1;
                statusRedraw[kRedrawNav] = 1;
            }
            if (QuickKeyCheck(keyPrefs.hyperspace)
                    && player->navTargetKind != kNavTargetLink) {
                player->navTargetKind = kNavTargetLink;
                player->targetStellar = -1;
                CueNextHyperRouteDest();
            }
        }
        if (QuickKeyCheck(keyPrefs.hyperspace))
            ShowFloatingMap();
    } else {
        navModeSelectFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.nextSystem)) {
        keyPressedThisFrame = 1;
        if (!navTargetSelectFlag && player->jumpTimer <= 0.0f) {
            navTargetSelectFlag = 1;
            statusRedraw[kRedrawNav] = 1;
            n = 0;
            for (i = 0; i < kNavSlots; i++) {
                linked[i] = 0;
                if (systemData[player->curSystem].links[i] != -1
                        && FindActiveCoLocatedSystem(
                               systemData[player->curSystem].links[i]) != -1) {
                    linked[i] = 1;
                    n++;
                }
            }
            if (n > 0) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (player->navTargetKind != kNavTargetLink)
                    player->targetStellar = -1;
                player->navTargetKind = kNavTargetLink;
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
        if (player->navTargetKind == kNavTargetLink)
            ShowFloatingMap();
    } else {
        navTargetSelectFlag = 0;
    }

    LICENSE_STEP_BEGIN
    shipClass[gameIsRegisteredIndex].registered =
        (shipData[36].licenseCopies != 0) ? (unsigned char)1 : (unsigned char)0;
    LICENSE_STEP_END

    if (player->jumpTimer <= 0.0f) {
        j = -1;
        if (QuickKeyCheck(kScanKey1))
            j = 0;
        if (QuickKeyCheck(kScanKey2))
            j = 1;
        if (QuickKeyCheck(kScanKey3))
            j = 2;
        if (QuickKeyCheck(kScanKey4))
            j = 3;
        for (i = 0; i <= 3; i++) {
            if (QuickKeyCheck(keyPrefs.navPreset[i]))
                j = i;
        }
        if (QuickKeyCheck(kScanF5)
                || (CmdKeyQuick() && QuickKeyCheck(kScanKey5)))
            j = 0x10;
        if (j != -1) {
            keyPressedThisFrame = 1;
            if (!navDefaultSelectFlag) {
                navDefaultSelectFlag = 1;
                if (j != 0x10)
                    n = systemData[player->curSystem].navDefault[j];
                else
                    n = FindNearestStellar(shipData);
                if (n != -1
                        && stellarData[n].system == player->curSystem) {
                    PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                    if (player->navTargetKind != kNavTargetStellar || player->targetStellar != n) {
                        navTargetExpand = 256.0f;
                        player->targetStellar = n;
                        statusRedraw[kRedrawNav] = 1;
                        player->navTargetKind = kNavTargetStellar;
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
            statusRedraw[kRedrawTarg] = 1;
            if (player->unk6e == 1) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (ShiftKeyQuick())
                    n = FindPrevShipInSystem(player->targetShip, player->curSystem);
                else
                    n = FindNextShipInSystem(player->targetShip, player->curSystem);
                if (n != player->targetShip && n != player->index) {
                    player->targetShip = n;
                    if (SWIsSpriteInRect(shipData[n].sprite,
                                         SWWindowRect(gSpriteWorld)))
                        weapTargetExpand = 256.0f;
                    else
                        weapTargetExpand = 0;
                } else {
                    player->targetShip = -1;
                }
            }
        }
    } else if (Button()) {
        if (!weapTargetSelectFlag) {
            weapTargetSelectFlag = 1;
            n = player->targetShip;
            SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
            GetMouse(&pt);
#else
            GetGlobalMouse(&pt);
#endif
            portRect = GetCWindowPortRect((WindowRef)gWind);
            box = *portRect;
#ifdef CEFIX /* F14.02 */
            box.right -= ceStatusBarWidth;
#else
            box.right -= kTargPanelInset;
#endif
            if (escortMenuOnscreen > 0
                    && PtInRectPoint(pt, &escortMenuRect)) {
                for (i = 0; i < 5; i++) {
                    hit.left = escortMenuRect.left + 4;
                    hit.right = escortMenuRect.right - 4;
                    hit.top = escortMenuRect.top + 0x1e + i * 0x12;
                    hit.bottom = escortMenuRect.top + 0x2b + i * 0x12;
                    if (PtInRectPoint(pt, &hit)) {
                        if (selEscortGroup != i - 1) {
                            selEscortGroup = i - 1;
                            ShowEscortMenu();
                            lastEscortMenuTime = ReadTick();
                        }
                        break;
                    }
                }
            } else if (showFloatingMap > 0 && player->jumpTimer <= 0.0f
                    && PtInRectPoint(pt, &floatingMapRect)) {
                hit.left = hit.right =
                    (floatingMapRect.left + floatingMapRect.right) / 2;
                hit.top = hit.bottom =
                    (floatingMapRect.top + floatingMapRect.bottom) / 2;
                InsetRect(&hit, -8, -8);
                if (PtInRectPoint(pt, &hit)) {
                    player->navTargetKind = kNavTargetLink;
                    player->targetStellar = -1;
                    statusRedraw[kRedrawNav] = 1;
                    ShowFloatingMap();
                    PlaySound(beepSnd[2], 1, gSoundVolume,
                              gSoundVolume);
                } else {
                    for (mapLink = 0; mapLink < kNavSlots; mapLink++) {
                        if (systemData[player->curSystem].links[mapLink] != -1
                                && FindActiveCoLocatedSystem(
                                       systemData[player->curSystem].links[mapLink])
                                   != -1) {
                            hit.left = hit.right =
                                (floatingMapRect.left + floatingMapRect.right) / 2
                                + (systemData[systemData[player->curSystem].links[mapLink]].pos.h
                                   - systemData[player->curSystem].pos.h)
                                  / floatingMapZoom;
                            hit.top = hit.bottom =
                                (floatingMapRect.top + floatingMapRect.bottom) / 2
                                + (systemData[systemData[player->curSystem].links[mapLink]].pos.v
                                   - systemData[player->curSystem].pos.v)
                                  / floatingMapZoom;
                            if (floatingMapZoom <= 0.5)
                                InsetRect(&hit, -12, -12);
                            else
                                InsetRect(&hit, -8, -8);
                            if (PtInRectPoint(pt, &hit)) {
                                player->targetStellar = mapLink;
                                player->navTargetKind = kNavTargetLink;
                                statusRedraw[kRedrawNav] = 1;
                                ShowFloatingMap();
                                PlaySound(beepSnd[2], 1, gSoundVolume,
                                          gSoundVolume);
                                break;
                            }
                        }
                    }
                }
            } else if (PtInRectPoint(pt, &box)) {
                targetUncloaked = ShipCanTargetUntargetableShips(shipData);
                targetCloaked = ShipCanTargetCloakedShips(shipData);
                if (player->active && player->sprite != NULL) {
                    box = ((SpriteRec *)player->sprite)->destFrameRect;
                    if (box.right - box.left < 0x30)
                        InsetRect(&box, -0x10, -0x10);
                    if (PtInRectPoint(pt, &box))
                        player->targetShip = -1;
                }
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active
                            && !IsDying(&shipData[i])
                            && (!ShipIsCloaked(&shipData[i]) || targetCloaked
                                || shipData[i].parentShip == 0)
                            && shipData[i].curSystem == player->curSystem
                            && (targetUncloaked
                                || !(shipClass[shipData[i].type].flags2
                                     & kShipUntargetable))
                            && shipData[i].sprite != NULL) {
                        box = ((SpriteRec *)shipData[i].sprite)->destFrameRect;
                        if (box.right - box.left < 0x30)
                            InsetRect(&box, -0x10, -0x10);
                        if (PtInRectPoint(pt, &box)) {
                            player->targetShip = i;
                            break;
                        }
                    }
                }
                if (player->targetShip != n) {
                    if (player->unk6e == 1) {
                        statusRedraw[kRedrawTarg] = 1;
                        if (player->targetShip != -1)
                            PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                        else
                            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                        if (player->targetShip != -1) {
                            if (SWIsSpriteInRect(shipData[player->targetShip].sprite,
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
                                box = stellarSprite[i]->destFrameRect;
                                if (box.right - box.left < 0x30)
                                    InsetRect(&box, -0x10, -0x10);
                                if (PtInRectPoint(pt, &box)) {
                                    if (player->targetStellar != j
                                            || player->navTargetKind != kNavTargetStellar) {
                                        PlaySound(beepSnd[0], 1, gSoundVolume,
                                                  gSoundVolume);
                                        player->targetStellar = j;
                                        navTargetExpand = 256.0f;
                                        statusRedraw[kRedrawNav] = 1;
                                        player->navTargetKind = kNavTargetStellar;
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
            SetRetainedWindow((WindowRef)gWind, false);
            HideCursorSprite();
            RefreshSAT();
            ShowTheCursor();
            hypergateMapMode = 0;
            DoSystemMap();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            SetRetainedWindow((WindowRef)gWind, true);
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
        if (player->navTargetKind != kNavTargetStellar || player->targetStellar == -1) {
            n = FindNearestStellar(player);
            if (n != -1) {
                statusRedraw[kRedrawNav] = 1;
                player->navTargetKind = kNavTargetStellar;
                player->targetStellar = n;
                navTargetExpand = 256.0f;
            }
        }
        if (!dockTryFlag && player->navTargetKind == kNavTargetStellar
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
            if (AltKeyQuick())
                n = FindNearestShipToPlayer();
            else
                n = FindNearestThreatToPlayer();
            weapCloseTargFlag = 1;
            if (n != -1 && player->unk6e == 1
                    && n != player->targetShip) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                statusRedraw[kRedrawTarg] = 1;
                player->unk6e = 1;
                player->targetShip = n;
                if (SWIsSpriteInRect(shipData[n].sprite,
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
                    && !(weaponStats[i].flags & kWeapSecondary))
                FirePlayerWeapon(i);
        }
    }
    if (player->jumpTimer <= 0.0f && QuickKeyCheck(keyPrefs.fireSecondary)
            && player->selectedWeapon != -1 && player->aiTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        FirePlayerWeapon(player->selectedWeapon);
    }
    if (player->selectedWeapon != -1
            && (weaponStats[player->selectedWeapon].flags & kWeapPersistentSmoke) /* BUG: Flags, not Flags2 */
            && !WeaponHasAmmo(player, player->selectedWeapon)) {
        player->selectedWeapon = -1;
        statusRedraw[kRedrawWeap] = 1;
    }

    if (QuickKeyCheck(keyPrefs.selectSecondary)) {
        keyPressedThisFrame = 1;
        if (!weapSelectFlag) {
            weapSelectFlag = 1;
            n = 0;
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (player->weap[i].count > 0
                        && (weaponStats[i].flags & kWeapSecondary)
                        && (weaponStats[i].guidance < kGuidancePointDef
                            || weaponStats[i].guidance == kGuidanceBay)) {
                    if (weaponStats[i].flags2 & kWeapHideIfNoAmmo) {
                        if (WeaponHasAmmo(player, i))
                            n++;
                    } else {
                        n++;
                    }
                }
            }
            if (n > 0) {
                statusRedraw[kRedrawWeap] = 1;
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (AltKeyQuick())
                    j = -1;
                else
                    j = 1;
                ok = 0;
                while (!ok) {
                    player->selectedWeapon += j;
                    while (player->selectedWeapon >= kMaxWeapTypes)
                        player->selectedWeapon -= kMaxWeapTypes;
                    while (player->selectedWeapon < 0)
                        player->selectedWeapon += kMaxWeapTypes;
                    if (player->selectedWeapon >= 0 && player->selectedWeapon < kMaxWeapTypes) {
                        if (player->weap[player->selectedWeapon].count > 0
                                && (weaponStats[player->selectedWeapon].flags
                                    & kWeapSecondary)
                                && (weaponStats[player->selectedWeapon].guidance < kGuidancePointDef
                                    || weaponStats[player->selectedWeapon].guidance
                                       == kGuidanceBay)) {
                            if (weaponStats[player->selectedWeapon].flags2 & kWeapHideIfNoAmmo) {
                                if (WeaponHasAmmo(player, player->selectedWeapon))
                                    ok = 1;
                            } else
                                ok = 1;
                        }
                    } else {
                        ok = 0;
                    }
                }
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
        statusRedraw[kRedrawWeap] = 1;
        PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
    }

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (player->weap[i].count > 0) {
            if (player->weap[i].reloadTimer > 0.0f)
                player->weap[i].reloadTimer -= gSpeedMult;
            else
                player->weap[i].reloadTimer = 0;
            if ((weaponStats[i].seeker & kWeapSeekNoFireIonized)
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
            } else if (AltKeyQuick()) {
                origin = XYShortToFloat(
                    stellarData[player->targetStellar].pos);
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos, &origin);
            } else {
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos,
                        &shipData[player->targetShip].pos);
            }
            turned = 1;
        } else if (player->targetStellar != -1) {
            origin = XYShortToFloat(
                stellarData[player->targetStellar].pos);
            player->desiredHeading = CalcInterceptAngle(&player->pos, &origin);
            turned = 1;
        }
    }
    if (player->navTargetKind == kNavTargetLink) {
        dockTarget = -1;
        dockPermission = -1;
        if (player->targetStellar != -1 && player->jumpTimer > 0.0f
                && (ShipCanExpiditeJumps(player)
                    || (abs((short)player->vel.x) < 2
                        && abs((short)player->vel.y) < 2))) {
            pos.x = systemData[player->curSystem].pos.h;
            pos.y = systemData[player->curSystem].pos.v;
            j = systemData[player->curSystem].links[player->targetStellar];
            dest.x = systemData[j].pos.h;
            dest.y = systemData[j].pos.v;
            player->desiredHeading = CalcInterceptAngle(&pos, &dest);
            turned = 1;
        }
    }

    jumping = 0;
    if ((QuickKeyCheck(keyPrefs.jump) && !IsDisabled(player))
            || player->jumpTimer > 0.0f) {
        keyPressedThisFrame = 1;
        if (player->navTargetKind == kNavTargetLink
                && player->targetStellar != -1
                && -999.0f != player->jumpTimer
                && player->fuel >= 100.0f) {
            canJump = 1;
            for (i = 0; i < kNavSlots; i++) {
                if (systemData[player->curSystem].navDefault[i] != -1
                        && !(stellarData[systemData[player->curSystem].navDefault[i]].flags2
                             & kSpobGateOrWormhole)) {
                    origin.x = origin.y = 0;
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
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x24);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1e);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1f);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                }
                if (canJump && player->jumpTimer <= 0.0f) {
                    waiting = 0;
                    n = -1;
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active
                                && shipData[i].parentShip == 0
                                && shipData[i].aiType == kShipTypeFighter
                                && !AIShipHasFuelForJump(&shipData[i])
                                && !(shipClass[shipData[i].type].flags2
                                     & kShipNoFighterVoice)) {
                            waiting = waiting + 1;
                            n = shipData[i].type;
                        }
                    }
                    if (waiting > 0) {
                        pstrcpy(msg, shipClass[n].name);
                        PtoCstr(msg);
                        strcat((char *)msg, ":  ");
                        if (waiting == 1)
                            StrcatIndCString((char *)msg, kGameStrList,
                                            Rand(2) + 0xa0);
                        else
                            StrcatIndCString((char *)msg, kGameStrList,
                                            Rand(2) + 0xa2);
                        CtoPstr((char *)msg);
                        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
                        PlaySound(beepSnd[4], 0x32, gSoundVolume, gSoundVolume);
                    }
                }
                if (ShipCanExpiditeJumps(player)
                        || (abs((short)player->vel.x) < 2
                            && abs((short)player->vel.y) < 2)) {
                    jumping = 1;
                    if ((shipClass[player->type].shanFlags & kShanFoldingParts)
                            && !(shipClass[player->type].shanFlags & kShanUnfoldsWhenFiring)
                            && player->foldSet > 0)
                        player->foldDir = -1;
                    if (player->jumpTimer <= 1.0f) {
                        player->jumpTimer = 2.0f;
                        player->jumpTick = ReadTick();
                        statusRedraw[kRedrawNav] = 1;
                        newbieFlag = 0x7fff;
                        if (doubleTime) {
                            if (!ST_IsSoundPlaying(warpUpSnd[1])) {
                                warpUpSoundSpec.handle = (void **)warpUpSnd[1];
                                AdjustWarpSoundSpeed(
                                    shipClass[player->type].mobilityFactor);
                                ST_PlaySoundParam(&warpUpSoundSpec);
                                warpSoundDone = 0;
                            }
                        } else if (!ST_IsSoundPlaying(warpUpSnd[0])) {
                            warpUpSoundSpec.handle = (void **)warpUpSnd[0];
                            AdjustWarpSoundSpeed(
                                shipClass[player->type].mobilityFactor);
                            ST_PlaySoundParam(&warpUpSoundSpec);
                            warpSoundDone = 0;
                        }
                    }
                } else {
                    player->jumpTick = ReadTick();
                    player->jumpTimer = 1.0f;
                    if (ShipIsInertialess(player)) {
                        player->accelMag = player->accelMag
                            - ShipAccelRate(player) * gSpeedMult;
                        if (player->accelMag < 0.0f)
                            player->accelMag = 0;
                    } else {
                        pos.x = pos.y = 0;
                        dest.x = player->vel.x * 100.0;
                        dest.y = player->vel.y * 100.0;
                        player->desiredHeading =
                            CalcInterceptAngle(&dest, &pos);
                        turned = 1;
                        if (abs(AngularDifference(
                                 player->desiredHeading,
                                 player->heading))
                                  < (ShipTurnRate(player) + 1.0f > 20.0f
                                     ? ShipTurnRate(player) + 1.0f : 20.0f)) {
                            Accel(player->heading, ShipAccelRate(player),
                                  &player->vel);
                            if (player->engineGlow < 0x18)
                                player->engineGlow += 3;
                        }
                        player->vel.x *= 0.992;
                        player->vel.y *= 0.992;
                    }
                }
            } else {
                if (!jumpTryFlag) {
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                    if (player->tractorShip != -1
                            && player->tractorShip != player->index)
                        GetIndString(msg, kGameStrList, 0x24);
                    else
                        GetIndString(msg, kGameStrList, 0x2a);
                    DisplayComm((PStrChar *)msg, 0x168, 0, kCommTextSize, &rgbWhite, 0, 0);
                } else {
                    player->jumpTimer = 0;
                }
            }
        } else if (!jumpTryFlag) {
            if (newbieFlag <= 2 && !disabled && newbieString[7][0] != 0) {
                if (player->navTargetKind != kNavTargetLink
                        || player->targetStellar == -1) {
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                    DisplayComm((PStrChar *)newbieString[7], 0x168, 0, kCommTextSize, &rgbWhite, 0, 0);
                }
            } else if (player->navTargetKind == kNavTargetLink
                    && player->targetStellar != -1
                    && -999.0f != player->jumpTimer
                    && player->fuel < 100.0f) {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                DisplayComm((PStrChar *)fuelString[7], kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
            }
        }
        jumpTryFlag = 1;
    } else {
        jumpTryFlag = 0;
    }

    if ((jumping || player->jumpTimer > 1.0f) && !disabled) {
        MakeEscortsHyperWithParent(player);
        player->jumpTimer = player->jumpTimer + gSpeedMult;
        if (!ShipCanExpiditeJumps(player)) {
            player->vel.x *= 0.98;
            player->vel.y *= 0.98;
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
            player->pos.x = player->pos.y = 0;
            pos.x = systemData[systemData[player->curSystem]
                         .links[player->targetStellar]].pos.h;
            pos.y = systemData[systemData[player->curSystem]
                         .links[player->targetStellar]].pos.v;
            dest.x = systemData[player->curSystem].pos.h;
            dest.y = systemData[player->curSystem].pos.v;
            Accel(((CalcInterceptAngle(&dest, &pos) + kHalfCircle) % kFullCircle),
                  1350.0f, &player->pos);
            player->vel.x = player->vel.y = 0;
            if (ShipIsInertialess(player))
                player->accelMag = ShipMaxSpeed(player);
            else
                Accel(player->heading, ShipMaxSpeed(player),
                      &player->vel);
#ifdef MACBUILD
            sys = systemData[player->curSystem].links[player->targetStellar];
            player->prevSystem = player->curSystem;
            routeDone = 0;
            if (hyperRoute[1] != -1
                    && FindActiveCoLocatedSystem(sys)
                       == FindActiveCoLocatedSystem(hyperRoute[1])
                    && (k = ShipJumpsPerJump(player)) > 1) {
                for (i = 1; i < k; i++) {
                    sys = FindActiveCoLocatedSystem(hyperRoute[1]);
                    if (sys == -1)
                        break;
                    player->curSystem = sys;
                    ExploreNebulaeFromSystem(sys);
                    SpoolNextHyperRouteDest();
                    if (hyperRoute[1] == -1)
                        break;
                }
                routeDone = 1;
            }
            player->curSystem = sys;
#else
            n = systemData[player->curSystem].links[player->targetStellar];
            player->prevSystem = player->curSystem;
            routeDone = 0;
            if (hyperRoute[1] != -1
                    && FindActiveCoLocatedSystem(n)
                       == FindActiveCoLocatedSystem(hyperRoute[1])
                    && (count = k = ShipJumpsPerJump(player)) > 1) {
                for (i = 1; i < count; i++) {
                    n = FindActiveCoLocatedSystem(hyperRoute[1]);
                    if (n == -1)
                        break;
                    player->curSystem = n;
                    ExploreNebulaeFromSystem(n);
                    SpoolNextHyperRouteDest();
                    if (hyperRoute[1] == -1)
                        break;
                }
                routeDone = 1;
            }
            player->curSystem = n;
#endif
            ExploreNebulaeFromSystem(player->curSystem);
            player->navTargetKind = -1;
            player->targetStellar = -1;
            player->targetShip = -1;
            player->fuel = player->fuel - 100.0f;
#if V111
            SWSetPortToWorkArea(gSpriteWorld);
            ForeColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
            if (useLegacyRenderingPipeline) {
                BlitPixieRectDrawProc(gSpriteWorld->workFrameP, SWWindowFrame(gSpriteWorld),
                             &gSpriteWorld->workFrameP->frameRect,
                             &SWWindowFrame(gSpriteWorld)->frameRect);
            } else {
                SWSetPortToWindow(gSpriteWorld);
                CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                         GetPortPixMapBits(SWWindowFrame(gSpriteWorld)->framePort),
                         &gSpriteWorld->workFrameP->frameRect,
                         &SWWindowFrame(gSpriteWorld)->frameRect, 0, NULL);
                QDFlushPortBuffer(SWWindowFrame(gSpriteWorld)->framePort, NULL);
            }
#else
#ifdef MAC1010
            SWSetPortToWorkArea(gSpriteWorld);
            ForeColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
#else
            SWSetPortToWindow(gSpriteWorld);
#ifdef CEFIX /* F22.01 */
            CE_SetFlashColor(whiteColor);
#else
            ForeColor(whiteColor);
#endif
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
            ScreenEndScene();
            SWSetPortToWorkArea(gSpriteWorld);
#endif
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, SWWindowFrame(gSpriteWorld),
                         &gSpriteWorld->workFrameP->frameRect,
                         &SWWindowFrame(gSpriteWorld)->frameRect);
#endif
            PropagateMissionBitEffects();
            if (systemData[shipData->curSystem].murk < 0)
                ClearStarfield();
            else
                RandomizeStars();
            ScrollStarfield(0.0f, 0.0f);
            EraseParticlesOffscreen(gSpriteWorld);
            ClearParticles(1);
            statusRedraw[kRedrawNav] = 1;
            statusRedraw[kRedrawTarg] = 1;
            statusRedraw[kRedrawCargo] = 1;
            statusRedraw[kRedrawWeap] = 1;
            statusRedraw[kRedrawFuel] = 1;
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
            for (i = 0; i < kMaxMissions; i++)
                missionRandom[i] = Rand(kDailyRollRange) + 1;
            mapScroll.h = systemData[player->curSystem].pos.h;
            mapScroll.v = systemData[player->curSystem].pos.v;
            days = ShipHyperTransitTime(player);
            lost = 0;
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active && shipData[i].parentShip == 0
                        && !IsDisabled(&shipData[i])) {
                    if (shipData[i].aiType == kShipTypeFighter
                            && !AIShipHasFuelForJump(&shipData[i])) {
                        shipData[i].active = 0;
                        lost++;
                    }
                    if (shipData[i].active) {
                        rank = ShipHyperTransitTime(&shipData[i]);
                        if (rank > days)
                            days = rank;
                    }
                }
            }
            for (i = 0; i < days; i++)
                IncrementGameTime();
            HandleGlobalPriceFlux();
            HandleLocalPriceFlux();
            MissionHandlePlayerEnteredNewSystem();
            j = FindFirstCoLocatedSystem(player->curSystem);
            if (systemData[j].exploreLevel < 1)
                systemData[j].exploreLevel = 1;
            if (systemData[player->curSystem].exploreLevel < 1)
                systemData[player->curSystem].exploreLevel = 1;
            AutoSetExploration(player->curSystem, 0, 1);
            player->jumpTimer = -999.0f;
            SetupShipsInSystem(player->curSystem, 0);
            SpawnBountyHunter();
            HandleStellarSystemVisibility();
            player->jumpTimer = 0;
            crashlog_print("entering system ");
            crashlog_print_num(j + kFirstResourceID);
            crashlog_print(" (");
            crashlog_print_pstr(systemData[j].name);
            crashlog_print(")\n");
            if (systemData[j].message != -1) {
                DisplayMessage(systemData[j].message);
            } else {
                n = Rand(3);
                strcpy((char *)msg, "");
                StrcatIndCString((char *)msg, kGameStrList, n + 0x2b);
                strcat((char *)msg, " ");
                pstrcpy(name, systemData[j].name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x30);
                strcat((char *)msg, " ");
                DateToLongString(gameTime, (char *)name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, ".");
                n = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (systemData[player->curSystem].navDefault[i] != -1)
                        n++;
                }
                if (n == 0) {
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x31);
                }
                if (lost > 0) {
                    strcat((char *)msg, "  (");
                    StrcatOrdinalNumber(lost, (char *)msg, 1);
                    strcat((char *)msg, " ");
                    if (lost == 1)
                        StrcatIndCString((char *)msg, kGameStrList, 0xa4);
                    else
                        StrcatIndCString((char *)msg, kGameStrList, 0xa5);
                    strcat((char *)msg, ")");
                }
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
            }
            if (hyperRoute[1] != -1 && !routeDone) {
                j = FindActiveCoLocatedSystem(hyperRoute[1]);
                if (j != FindActiveCoLocatedSystem(shipData->curSystem)) {
                    for (i = 0; i < kMaxHyperRoute; i++)
                        hyperRoute[i] = -1;
                }
            }
            SpoolNextHyperRouteDest();
            CueNextHyperRouteDest();
            DrawMapInFloatingFrame();
            killShots = 1;
            killBoxes = 1;
            killExplods = 1;
            killSmoke = 1;
            killAsteroids = 1;
            missionSoundID = -1;
            escortMenuOnscreen = 0;
#ifdef MACBUILD
            CreateAsteroids(0);
#else
            CreateAsteroids();
#endif
            ResetPlayerPrecalcedValues();
            if (shipClass[player->type].shanFlags & kShanUnfoldsWhenFiring) {
                player->foldSet =
                    shipClass[player->type].baseSets - 1;
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
#if V111
            if (useLegacyRenderingPipeline)
                SWSetPortToWorkArea(gSpriteWorld);
            else
                SWSetPortToBackground(gSpriteWorld);
#else
            SWSetPortToWorkArea(gSpriteWorld);
#endif
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->backFrameP->frameRect);
            SWSetPortToWorkArea(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            SWSetPortToWindow(gSpriteWorld);
            RefreshSAT();
            RedrawComm(1);
            DoEscortPayment(days);
            return;
        } else if (player->jumpTimer > 30.0f) {
            if ((ReadTick() - player->jumpTick)
                    > WarpSoundDuration()
                      / shipClass[player->type].mobilityFactor) {
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
                               - ShipAccelRate(player) * gSpeedMult;
            if (player->accelMag < 0.0)
                player->accelMag = 0;
        } else if (FloatAbs(player->vel.x) >= 0.05 || FloatAbs(player->vel.y) >= 0.05) {
            pos.x = pos.y = 0;
            dest.x = player->vel.x * 100.0;
            dest.y = player->vel.y * 100.0;
            player->desiredHeading =
                (CalcInterceptAngle(&pos, &dest) + kHalfCircle) % kFullCircle;
            turned = 1;
        }
    }

    turnStep = ShipTurnRate(player);
    dir = 0;
    if (turned && abs(AngularDifference(player->desiredHeading, player->heading)) > turnStep * gSpeedMult
            && !disabled) {
        angle = player->desiredHeading - player->heading;
        while (angle >= kFullCircleF)
            angle = angle - kFullCircleF;
        while (angle < 0.0f)
            angle += kFullCircleF;
        if (angle > kHalfTurn) {
            player->heading -= (float)turnStep * gSpeedMult;
            dir = -1;
        } else {
            player->heading += (float)turnStep * gSpeedMult;
            dir = 1;
        }
    } else if (player->jumpTimer <= 0.0f && !disabled && !turned
            && (QuickKeyCheck(keyPrefs.turnLeft) || QuickKeyCheck(keyPrefs.turnRight))) {
        keyPressedThisFrame = 1;
        if (QuickKeyCheck(keyPrefs.turnLeft) && !QuickKeyCheck(keyPrefs.turnRight)) {
            player->heading = player->heading - (float)turnStep * gSpeedMult;
            dir = -1;
        } else if (QuickKeyCheck(keyPrefs.turnRight)
                && !QuickKeyCheck(keyPrefs.turnLeft)) {
            player->heading = player->heading + (float)turnStep * gSpeedMult;
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
                && player->fuel >= ShipAfterburnerFuelUsage(player))
            abOn = 1;
    }

    if (player->aiTimer <= 0.0f && QuickKeyCheck(keyPrefs.accelerate)
            && player->jumpTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        if (ShipIsInertialess(player)) {
            player->accelMag = player->accelMag + ShipAccelRate(player) * gSpeedMult;
            if (player->accelMag > ShipMaxSpeed(player))
                player->accelMag = ShipMaxSpeed(player);
        } else if (abOn && !gravitySourcesPresent) {
            AdjustedAccel(player->heading,
                          ShipAccelRate(player) * gSpeedMult,
                          &player->vel,
                          ShipMaxSpeed(player) * 1.8);
        } else {
            AdjustedAccel(player->heading,
                          ShipAccelRate(player) * gSpeedMult,
                          &player->vel, ShipMaxSpeed(player));
        }
        if (player->engineGlow < 0x18)
            player->engineGlow += 1;
    } else if (player->engineGlow > 0) {
        player->engineGlow -= 1;
    }

    player->bankDir = 0;
    if (shipClass[player->type].shanFlags & kShanBanks) {
        if (dir != 0) {
            if (dir > 0 && player->frameTimer < 16.0f)
                player->frameTimer += gSpeedMult;
            if (dir < 0 && player->frameTimer > -16.0f)
                player->frameTimer -= gSpeedMult;
        } else {
            if (player->frameTimer >= gSpeedMult * 2.0)
                player->frameTimer -= gSpeedMult * 2.0;
            else if (player->frameTimer <= -gSpeedMult * 2.0)
                player->frameTimer += gSpeedMult * 2.0;
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
                && (shipClass[player->type].shanFlags & kShanFoldingParts)
                && player->engineGlow < 0x18)
            player->engineGlow += 2;
    }

    if (player->aiTimer > 0.0f || player->jumpTimer > 0.0f) {
        if ((shipClass[player->type].shanFlags & kShanBanks)
                && player->jumpTimer > 30.0f)
            player->frameTimer = 0;
        if (player->engineGlow > 0)
            player->engineGlow -= 1;
    }

    if (player->shield < ShipShieldCapacity(player) && !dying && !disabled) {
        player->shield = player->shield + ShipShieldRechargeRate(player) * gSpeedMult;
        statusRedraw[kRedrawShield] = 1;
    }
    if (player->armor < ShipArmorCapacity(player) && !dying
            && !IsDisabled(player) && playerRepairInhibit < 0.0) {
        player->armor = player->armor + ShipArmorRechargeRate(player) * gSpeedMult;
        statusRedraw[kRedrawShield] = 1;
    }

    if (player->jumpTimer > 1.0f && !disabled) {
        pos.x = systemData[player->curSystem].pos.h;
        pos.y = systemData[player->curSystem].pos.v;
        n = systemData[player->curSystem].links[player->targetStellar];
        dest.x = systemData[n].pos.h;
        dest.y = systemData[n].pos.v;
        n = CalcInterceptAngle(&pos, &dest);
        if (abs(AngularDifference(player->heading, n))
                  <= (ShipTurnRate(player) > 30.0f ? ShipTurnRate(player) : 30.0f)) {
            span = (ReadTick() - player->jumpTick)
                   * shipClass[player->type].mobilityFactor / (WarpSoundDuration() * 0.01);
            step = span;
            if (shipClass[player->type].mobilityFactor != 0) {
                step = step - (doubleTime ? 1.5 : 1.0) * 35.0
                       / shipClass[player->type].mobilityFactor;
            }
            if (step > 0.0) {
                if (settings.noHyperGamma == 0)
#if V111
                    hyperGamma = (step - 55.0f) * 5.0f;
#elif defined(MAC1010)
#if TARGET_API_MAC_CARBON
                    hyperGamma = 5.0f * (step - 55.0f);
#else
                    hyperGamma = step * 0.3 - 15.0;
#endif
#else
                    hyperGamma = step * 0.3 - 15.0;
#endif
                if (step > 50.0f)
                    step = 50.0f;
                Accel(player->heading, step * gSpeedMult,
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
            player->accelMag *= 0.985;
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
    player->pos.x += player->vel.x * gSpeedMult;
    player->pos.y += player->vel.y * gSpeedMult;

    if (player->navTargetKind == kNavTargetLink
            && player->targetStellar != -1) {
        canJump = 1;
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[player->curSystem].navDefault[i] != -1
                    && !(stellarData[systemData[player->curSystem].navDefault[i]].flags2
                         & kSpobGateOrWormhole)) {
                origin.x = origin.y = 0;
                turnLim = ShipHyperSafeDist(player);
                if (FloatAbs(XYFloatSquaredDist(&origin,
                            &player->pos)) <= turnLim)
                    canJump = 0;
            }
        }
        if (canJump) {
            if (!lastJumpDistOk && player->jumpTimer <= 0.0f
                    && player->fuel >= kHyperFuelUnit) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                statusRedraw[kRedrawNav] = 1;
                if (commMsg[0] > 0) {
                    GetIndString(msg, kGameStrList, 0x2a);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x24);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1e);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                    GetIndString(msg, kGameStrList, 0x1f);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
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
        player->ionization = player->ionization - ShipDeionizationRate(player) * gSpeedMult;
        step = ShipIonizationFactor(player);
        if (step > 0.7)
            step = 0.7f;
        step = ShipMaxSpeed(player) * (1.0f - step);
        if (player->vel.x > step)
            player->vel.x -= gSpeedMult * 0.025;
        if (player->vel.x < -step)
            player->vel.x += gSpeedMult * 0.025;
        if (player->vel.y > step)
            player->vel.y -= gSpeedMult * 0.025;
        if (player->vel.y < -step)
            player->vel.y += gSpeedMult * 0.025;
    } else {
        player->ionization = 0;
    }

    if (ShipFuelGenRate(player, &step)) {
        n = player->fuel;
        player->fuel += step * gSpeedMult;
        if ((short)player->fuel != n)
            statusRedraw[kRedrawFuel] = 1;
    }
    fuelCap = ShipFuelCapacity(player);
    if (player->fuel > fuelCap)
        player->fuel = fuelCap;
    if (player->fuel < 0.0f)
        player->fuel = 0;

#ifdef CEFIX /* F21.01-02 */
    if (CE_CheckDebugKeys(kCEScanF10) && cheatsActive) {
#else
    if (QuickKeyCheck(kScanExploreCheat) && cheatsActive) {
#endif
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (AltKeyQuick()) {
            for (i = 0; i < kMaxSystems; i++)
                systemData[i].exploreLevel = 0;
            for (i = 0; i < kMaxNebulae; i++)
                nebulaData[i].explored = 0;
        } else {
            for (i = 0; i < kMaxSystems; i++)
                systemData[i].exploreLevel = 2;
            for (i = 0; i < kMaxNebulae; i++)
                nebulaData[i].explored = 1;
        }
        HandleStellarSystemVisibility();
        while (KeyCheck(kScanExploreCheat))
            BusyIdle();
    }

    if (QuickKeyCheck(keyPrefs.cheatAmmo) && cheatsActive) {
        PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (shipClass[player->type].weapAmmo[i] > 0) {
                if (weaponStats[i].ammoType >= 0
                        && weaponStats[i].ammoType < kMaxWeapTypes
                        && weaponStats[i].guidance != kGuidanceBay)
                    player->weap[weaponStats[i].ammoType].ammo =
                        shipClass[player->type].weapAmmo[i];
                else
                    player->weap[i].ammo =
                        shipClass[player->type].weapAmmo[i];
            }
        }
        if (AltKeyQuick())
            player->fuel = 0;
        else
            player->fuel = ShipFuelCapacity(player);
        player->shield = ShipShieldCapacity(player);
#ifdef CEFIX /* F21.04 */
        player->armor = ShipArmorCapacity(player);
#else
        player->armor = ShipShieldCapacity(player);
#endif
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
        statusRedraw[kRedrawFuel] = 1;
        statusRedraw[kRedrawShield] = 1;
        statusRedraw[kRedrawWeap] = 1;
        ResetPlayerPrecalcedValues();
    }

#ifdef CEFIX /* F21.03 */
    if (cheatsActive && QuickKeyCheck(kCEScanF7) && player->targetShip != -1
            && shipData[player->targetShip].active) {
#else
    if (cheatsActive && QuickKeyCheck(kScanAttackCheat) && player->targetShip != -1
            && shipData[player->targetShip].active) {
#endif
        if (AltKeyQuick())
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
                DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
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
                    selEscortGroup = -1;
                    GetIndString(msg, kGameStrList, 0x33);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1, 0, kCommTextSize, &rgbWhite, 0, 0);
                    ShowEscortMenu();
                    lastEscortMenuTime = ReadTick();
                } else {
                    GetIndString(msg, kGameStrList, 0x33);
                    DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
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
                selEscortGroup = -1;
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
#ifdef MACBUILD
                if (QuickKeyCheckPlusEscorts(keyPrefs.quickKey[k])) {
#else
                if (RawKeyCheck(keyPrefs.quickKey[k])) {
#endif
                    if (!escortGroupFlag[k]) {
                        escortGroupFlag[k] = 1;
                        if (k > 0) {
                            ok = 0;
                            for (j = 1; j < kMaxShips; j++) {
                                if (shipData[j].active
                                        && shipData[j].parentShip == 0
                                        && shipClass[shipData[j].type].escortType
                                           == k - 1)
                                    ok = 1;
                            }
                        } else {
                            ok = 1;
                        }
                        if (ok) {
                            selEscortGroup = k - 1;
                            ShowEscortMenu();
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
                    if (AltKeyQuick())
                        n = kOrderRegroup;
                    else
                        n = kOrderFormation;
                }
                if (selEscortGroup == -1 || escortMenuOnscreen <= 0) {
                    for (j = 0; j < 4; j++)
                        playerEscortOrders[j] = n;
                } else {
                    playerEscortOrders[selEscortGroup] = n;
                }
                if (escortMenuOnscreen > 0) {
                    if (IssueNewEscortCommand(selEscortGroup, n, 0)) {
                        ShowEscortMenu();
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
                        && shipClass[shipData[j].type].escortType == i
                        && shipData[j].aiType == kShipTypeFighter
                        && shipData[j].escortMayAttack) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                playerEscortOrders[i] = kOrderFormation;
                if (escortMenuOnscreen > 0)
                    ShowEscortMenu();
            }
        }
    }

    for (i = 0; i < 4; i++)
        groupCount[i] = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active && shipData[i].parentShip == 0
                && shipData[i].escortMayAttack) {
            if (shipClass[shipData[i].type].escortType >= 0
                    && shipClass[shipData[i].type].escortType < 4)
                groupCount[shipClass[shipData[i].type].escortType] += 1;
        }
    }
    for (i = 0; i < 4; i++) {
        if (groupCount[i] == 0 && (long)playerEscortOrders[i] != kOrderFormation) {
            playerEscortOrders[i] = kOrderFormation;
            if (escortMenuOnscreen > 0)
                ShowEscortMenu();
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
#ifdef CEFIX /* F21.05 */
        if (QuickKeyCheck(kCEScanBackspace)) {
#else
        if (QuickKeyCheck(kScanWarpCheat)) {
#endif
            if (ST_IsSoundPlaying(warpUpSnd[0])) {
                ST_HaltSound(warpUpSnd[0]);
                WarpUpCallback(1, 0, 0);
            } else if (ST_IsSoundPlaying(warpUpSnd[1])) {
                ST_HaltSound(warpUpSnd[1]);
                WarpUpCallback(1, 0, 0);
            }
        }

#ifdef MACBUILD
        if (QuickKeyCheck(keyPrefs.cheatSpare) || QuickKeyCheck(kScanInfoCheat)) {
            static long stealQty;
            static long memFree;
            static Size grow;

            memFree = MaxMem(&grow);
            NumToString(memFree, msg);
            PtoCstr(msg);
            strcat((char *)msg, " bytes free, gSpeedMult = ");
            NumToString(gSpeedMult * 100.0, name);
            PtoCstr(name);
            strcat((char *)msg, (char *)name);
            strcat((char *)msg, "%, ");
            strcat((char *)msg, "X:");
            NumToString(shipData->pos.x, name);
            PtoCstr(name);
            strcat((char *)msg, (char *)name);
            strcat((char *)msg, ", Y:");
            NumToString(shipData->pos.y, name);
            PtoCstr(name);
            strcat((char *)msg, (char *)name);
            CtoPstr((char *)msg);
            DisplayComm((PStrChar *)msg, 0x3c, 0, kCommTextSize, &rgbWhite, 0, 0);
        }

#endif
#ifdef CEFIX /* F21.07 */
        if (QuickKeyCheck(keyPrefs.cheatSpawn) || QuickKeyCheck(kCEScanNumEnter)) {
#else
        if (QuickKeyCheck(keyPrefs.cheatSpawn) || QuickKeyCheck(kScanSpawnCheat)) {
#endif
            if (!spawnShipFlag) {
                spawnShipFlag = 1;
                if (AltKeyQuick()) {
                    if (CtrlKeyQuick()) {
                        HyperSpawnFleet(player->curSystem, 6, -1, 1);
#ifdef CEFIX /* F21.06 */
                    } else if (QuickKeyCheck(kScanLShift)) {
#else
                    } else if (QuickKeyCheck(kScanFleetCheat)) {
#endif
                        HyperSpawnFleet(player->curSystem, 0x15, -1, 1);
                    } else {
                        HyperSpawnFleet(player->curSystem, 1, -1, 1);
                    }
                } else if (CtrlKeyQuick()) {
                    player->targetShip = SpawnPerson(player->curSystem, 1, 0x201);
                } else {
                    player->targetShip = HyperShipSpawn(player->curSystem);
                }
                statusRedraw[kRedrawTarg] = 1;
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            }
        } else {
            spawnShipFlag = 0;
        }

#ifdef CEFIX /* F21.08-09 */
        if (QuickKeyCheck(keyPrefs.cheatKill) && player->targetShip != -1) {
#else
        if ((QuickKeyCheck(keyPrefs.cheatKill) || QuickKeyCheck(kScanKillCheat)
                || QuickKeyCheck(kScanKillCheat2)) && player->targetShip != -1) {
#endif
            shipData[player->targetShip].active = 0;
            statusRedraw[kRedrawTarg] = 1;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }

        if (QuickKeyCheck(keyPrefs.cheatArmor) || QuickKeyCheck(kScanArmorCheat)) {
            if (player->targetShip != -1) {
                if (AltKeyQuick()) {
                    if (IsDisabled(&shipData[player->targetShip])) {
                        shipData[player->targetShip].armor =
                            shipClass[shipData[player->targetShip].type].armor;
                        shipData[player->targetShip].shield = 0;
                    }
                    shipData[player->targetShip].aiState = kAIStateHiJack;
                    shipData[player->targetShip].aiSubstate = kSubLoJack;
                    shipData[player->targetShip].targetShip = -1;
                    shipData[player->targetShip].targetStellar = -1;
                    shipData[player->targetShip].govt = -1;
                    shipData[player->targetShip].parentShip = 0;
                    shipData[player->targetShip].aiType = kShipTypeEscort;
                    shipData[player->targetShip].sellOnLanding = 0;
                    shipData[player->targetShip].upgradeOnLanding = 0;
                    shipData[player->targetShip].fuelRegenState = -1;
                    if (CmdKeyQuick())
                        shipData[player->targetShip].hired = 1;
                    else
                        shipData[player->targetShip].hired = 0;
                    shipData[player->targetShip].armor =
                        ShipArmorCapacity(&shipData[player->targetShip]);
                    AISetup(&shipData[player->targetShip]);
                    AIShipCapturedByPlayer(&shipData[player->targetShip]);
                } else {
                    if (shipData[player->targetShip].parentShip == 0) {
                        if (shipData[player->targetShip].aiType == kShipTypeEscort
                                && shipClass[shipData[player->targetShip].type]
                                   .inherentAI <= 2) {
                            DestroyPartialFleetCargo(player->targetShip);
                            statusRedraw[kRedrawCargo] = 1;
                        }
                        if (shipData[player->targetShip].aiType == kShipTypeFighter)
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
                statusRedraw[kRedrawTarg] = 1;
            } else {
                while (!IsDisabled(player) && !IsDying(player))
                    player->armor = player->armor - 1.0f;
                player->armor = 1.0f;
                playerRepairInhibit = 300.0f;
                statusRedraw[kRedrawShield] = 1;
            }
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }

        if (QuickKeyCheck(keyPrefs.cheatDamage) || QuickKeyCheck(kScanDamageCheat)) {
            if (AltKeyQuick()) {
                player->armor = 1.0f;
                statusRedraw[kRedrawShield] = 1;
            } else if (player->targetShip != -1
                    && shipData[player->targetShip].armor > 0.0f) {
                if (ShiftKeyQuick()
                        && !ShipIsDerelict(&shipData[player->targetShip])) {
                    SlapWithPenalty(shipData->curSystem,
                                    shipData[player->targetShip].govt, 3,
                                    shipData[player->targetShip].specialMission);
                    AddToPlayerKills(
                        shipClass[shipData[player->targetShip].type].strength);
                }
                if (shipData[player->targetShip].aiType == kShipTypeEscort
                        && shipClass[shipData[player->targetShip].type]
                           .inherentAI <= 2) {
                    DestroyPartialFleetCargo(player->targetShip);
                    statusRedraw[kRedrawCargo] = 1;
                    DrawStatusCargo();
                }
                shipData[player->targetShip].armor = -1.0f;
                statusRedraw[kRedrawTarg] = 1;
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            }
        }

        if (QuickKeyCheck(keyPrefs.cheatShipNext) || QuickKeyCheck(keyPrefs.cheatShipPrev)) {
            if (QuickKeyCheck(keyPrefs.cheatShipNext)) {
                do {
                    player->type += 1;
                    if (player->type >= kShipTypeSpecial)
                        player->type = 0;
                } while (shipClass[player->type].mass < 0);
            } else {
                do {
                    player->type -= 1;
                    if (player->type < 0)
                        player->type = kShipTypeSpecial - 1;
                } while (shipClass[player->type].mass < 0);
            }
            while (KeyCheck(keyPrefs.cheatShipNext) || KeyCheck(keyPrefs.cheatShipPrev))
                ;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            player->shield = ShipShieldCapacity(player);
            player->armor = ShipArmorCapacity(player);
            player->fuel = shipClass[player->type].fuel;
            player->foldSet =
                shipClass[player->type].baseSets - 1;
            player->foldDir = 0;
            player->frameTimer = 0;
            statusRedraw[kRedrawFuel] = statusRedraw[kRedrawShield] = statusRedraw[kRedrawWeap] =
                statusRedraw[kRedrawCargo] = 1;
            for (i = 0; i < kMaxOutfits; i++)
                shipSystems[i] = 0;
            for (i = 0; i < kMaxWeapTypes; i++) {
                player->weap[i].count = 0;
                player->weap[i].ammo = 0;
            }
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (shipClass[player->type].weapCount[i] > 0)
                    player->weap[i].count =
                        shipClass[player->type].weapCount[i];
                if (shipClass[player->type].weapAmmo[i] > 0) {
                    if (weaponStats[i].ammoType >= 0
                            && weaponStats[i].ammoType < kMaxWeapTypes
                            && weaponStats[i].guidance != kGuidanceBay)
                        player->weap[weaponStats[i].ammoType].ammo =
                            shipClass[player->type].weapAmmo[i];
                    else {
                        short ammo = shipClass[player->type].weapAmmo[i];
                        player->weap[i].ammo = ammo;
                    }
                }
            }
            for (i = 0; i < kDefaultOutfitSlots; i++) {
                if (shipClass[player->type].defOutfCount[i] > 0)
                    shipSystems[shipClass[player->type].defOutfID[i]] +=
                        shipClass[player->type].defOutfCount[i];
            }
            ShipStatsToSystemInfo();
            LoadIntfResource();
            player->selectedWeapon = -1;
            ResetPlayerPrecalcedValues();
            player->armor = ShipArmorCapacity(player);
            player->shield = ShipShieldCapacity(player);
        }

        if (ShiftKeyQuick()
                && QuickKeyCheck(keyPrefs.cheatCredits)) {
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            if (AltKeyQuick()) {
                if (player->credits >= 10000)
                    player->credits -= 10000;
                else
                    player->credits = 0;
            } else {
                player->credits += 50000;
            }
            statusRedraw[kRedrawCargo] = 1;
        }

#ifdef CEFIX /* F21.10 */
        if (QuickKeyCheck(kCEScanEnd)) {
#else
        if (QuickKeyCheck(kScanCheatsOff)) {
#endif
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            cheatsActive = 0;
        }
    }

    if (player->credits < 0) {
        player->credits = 0x5fffffff;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (player->credits > 0x5fffffff) {
        player->credits = 0x5fffffff;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (player->hasFormation)
        AICalcFormationPositions(player, 0);
    if (player->deathTimer > 1.0f) {
        if (!ST_IsSoundPlaying(klaxxonSnd))
            PlaySound(klaxxonSnd, 0xf, gSoundVolume, gSoundVolume);
    }

    if (disabled || dying) {
        ok = 0;
        if (dying) {
            if (player->deathTimer <= shipClass[player->type].deathDelay * 0.5
                    || player->deathTimer <= 30.0f) {
                if (HasAutoEject(player))
                    ok = 1;
            }
        }
        if ((CmdKeyQuick()
                && QuickKeyCheck(keyPrefs.eject)) || ok) {
            keyPressedThisFrame = 1;
            if (HasEscapePod(player)) {
                n = GenericRandomShipSpawn(player->curSystem, 0);
                if (n != -1) {
                shipData[n].aiType = -1;
                shipData[n].pos = player->pos;
                shipData[n].vel = player->vel;
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
                SWCloneFramesFromMaster(shipData[n].sprite,
                                        masterShipSprite[shipData[n].type]);
                SWSetCurrentFrameIndex(shipData[n].sprite, 0);
                player->deathTimer = -1.0f;
                player->targetShip = -1;
                player->targetStellar = -1;
                player->selectedWeapon = -1;
                statusRedraw[kRedrawFuel] = statusRedraw[kRedrawNav] = statusRedraw[kRedrawTarg] =
                    statusRedraw[kRedrawCargo] = statusRedraw[kRedrawWeap] = 1;
                crashlog_mission_bit_set("retiring",
                    (char *)shipClass[shipData->type].name, 1,
                    shipData->type + kFirstResourceID,
                    shipClass[shipData->type].onRetire);
                EvalMissionBitSetString(shipClass[shipData->type].onRetire);
                if (HasEscapeShip(player)) {
                    player->type = EscapeShipType(player);
                    player->shield = shipClass[player->type].shield
                        * (Rand(0x1e) + 0x32) * 0.01;
                    player->armor = shipClass[player->type].armor
                        * (Rand(0x1e) + 0x32) * 0.01;
                    player->fuel = shipClass[player->type].fuel
                        * (Rand(0x1e) + 0x32) * 0.01;
                    player->escapePods = -1;
                    strcpy((char *)msg, "");
                    StrcatIndCString((char *)msg, kGameStrList, 0x34);
                    strcat((char *)msg, " ");
                    pstrcpy(name, shipyardLongNames[player->type]);
                    PtoCstr(name);
                    strcat((char *)msg, (char *)name);
                    strcat((char *)msg, ".");
                    CtoPstr((char *)msg);
                    DisplayComm((PStrChar *)msg, 0x200, 0, kCommTextSize, &rgbWhite, 0, 0);
                } else {
                    player->type = kShipTypeSpecial;
                    player->shield = shipClass[player->type].shield;
                    player->armor = shipClass[player->type].armor;
                    player->escapePods = 0x15e;
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active && shipData[i].targetShip == 0)
                            shipData[i].targetShip = -1;
                    }
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
                SWCloneFramesFromMaster(player->sprite,
                                        masterShipSprite[player->type]);
                SWSetCurrentFrameIndex(player->sprite, 0);
                player->vel.x = player->vel.y;
                Accel(player->heading, ShipMaxSpeed(player),
                      &player->vel);
                for (i = 0; i < kMaxOutfits; i++) {
                    if (!outfitSystem[i].persistent)
                        shipSystems[i] = 0;
                }
                for (i = 0; i < kMaxWeapTypes; i++) {
                    player->weap[i].count = 0;
                    player->weap[i].ammo = 0;
                }
                for (i = 0; i < kMaxWeapTypes; i++) {
                    player->weap[i].count =
                        shipClass[player->type].weapCount[i];
                    if (shipClass[player->type].weapAmmo[i] > 0) {
                        if (weaponStats[i].ammoType >= 0
                                && weaponStats[i].ammoType < kMaxWeapTypes
                                && weaponStats[i].guidance != kGuidanceBay)
                            player->weap[weaponStats[i].ammoType].ammo =
                                shipClass[player->type].weapAmmo[i];
                        else
                            player->weap[i].ammo =
                                shipClass[player->type].weapAmmo[i];
                    }
                    player->weap[i].reloadTimer = 0;
                    player->weap[i].burstCount = 0;
                }
                ShipStatsToSystemInfo();
                for (i = 0; i < kDefaultOutfitSlots; i++) {
                    if (shipClass[player->type].defOutfCount[i] > 0)
                        shipSystems[
                            shipClass[player->type].defOutfID[i]] +=
                            shipClass[player->type].defOutfCount[i];
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
            player->accelMag = player->accelMag + ShipAccelRate(player) * 2.75;
            if (player->accelMag > ShipMaxSpeed(player) * 1.8)
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
                player->vel.y *= 0.99;
            if ((pos.x > 0.0f && player->vel.x < dest.x)
                    || (pos.x <= 0.0f && player->vel.x > dest.x))
                player->vel.x = player->vel.x + pos.x;
            else
                player->vel.x *= 0.99;
        }
        player->fuel = player->fuel - ShipAfterburnerFuelUsage(player) * gSpeedMult;
        if (player->fuel < 0.0)
            player->fuel = 0;
        if (frameCounter % 4 == 0)
            statusRedraw[kRedrawFuel] = 1;
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

    if (CmdKeyQuick()
            && QuickKeyCheck(keyPrefs.jettison)) {
        keyPressedThisFrame = 1;
#ifdef MACBUILD
        if (QuickKeyCheck(kScanOption))
#else
        if (QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
#endif
            JettisonCargo(0);
        else
            JettisonCargo(1);
        statusRedraw[kRedrawCargo] = 1;
        ResetPlayerPrecalcedValues();
    }

    if (CmdKeyQuick()
            && QuickKeyCheck(keyPrefs.selfDestruct) && destructCount < 0.0f
            && !dying) {
        keyPressedThisFrame = 1;
        PlaySound(beepSnd[3], 0xf, gSoundVolume, gSoundVolume);
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, 0x181);
        strcat((char *)msg, " ");
        StrcatIndCString((char *)msg, kGameStrList, 0x182);
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, 0x28, 0, kCommTextSize, &rgbWhite, 0, 0);
        destructCount = 150.0f;
    }

    if (destructCount > 0.0f) {
        if (CmdKeyQuick()
                && QuickKeyCheck(keyPrefs.selfDestruct)) {
            destructing = 1;
            if (destructCount >= 1.0)
                destructCount = destructCount - gSpeedMult;
            if ((short)destructCount % 30 == 0 && destructCount <= 120.0f) {
                extern char *strcpy(char *, const char *);
                strcpy((char *)msg, "");
                StrcatIndCString((char *)msg, kGameStrList, 0x181);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x18b);
                strcat((char *)msg, " ");
                NumToString(destructCount / 30.0f, name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                if (destructCount > 30.0f)
                    StrcatIndCString((char *)msg, kGameStrList, 0x185);
                else
                    StrcatIndCString((char *)msg, kGameStrList, 0x185);
                strcat((char *)msg, ".");
                CtoPstr((char *)msg);
                PlaySound(beepSnd[2], 0xf, gSoundVolume,
                          gSoundVolume);
                DisplayComm((PStrChar *)msg, 0x46, 0, kCommTextSize, &rgbWhite, 0, 0);
            }
        } else {
            extern char *strcpy(char *, const char *);
            PlaySound(beepSnd[1], 0xf, gSoundVolume, gSoundVolume);
            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, 0x181);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, 0x183);
            CtoPstr((char *)msg);
            DisplayComm((PStrChar *)msg, 0x28, 0, kCommTextSize, &rgbWhite, 0, 0);
            destructing = 1;
            destructCount = -1.0f;
        }
    } else {
        destructing = 0;
    }

    if (destructCount <= 1.0f && destructing) {
        if (CmdKeyQuick()
                && QuickKeyCheck(keyPrefs.selfDestruct)) {
            keyPressedThisFrame = 1;
            PlaySound(beepSnd[1], 0xf, gSoundVolume, gSoundVolume);
            GetIndString(msg, kGameStrList, 0x186);
            DisplayComm((PStrChar *)msg, 0x28, 0, kCommTextSize, &rgbWhite, 0, 0);
            player->shield = 0;
            player->armor = -1.0f;
            player->targetShip = -1;
            statusRedraw[kRedrawWeap] = 1;
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
                SetRetainedWindow((WindowRef)gWind, false);
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                DoPlayerInfoDialog();
                gSpeedMult = 0;
                RedrawStatus(1);
                RedrawComm(0);
                SetRetainedWindow((WindowRef)gWind, true);
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
                        && !(currentMissionObjectives[i].flags & kMissionHidden))
                    n++;
            }
            if (n > 0) {
                SetRetainedWindow((WindowRef)gWind, false);
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                DoMissionInfoDialog();
                gSpeedMult = 0;
                SetRetainedWindow((WindowRef)gWind, true);
                RedrawStatus(1);
                RedrawComm(0);
                SetAllKeyCmdFlags();
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                GetIndString(msg, kGameStrList, 0x162);
                DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
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
            player->fuel = player->fuel - n * 0.03333 * gSpeedMult;
            if (frameCounter % 10 == 0)
                statusRedraw[kRedrawFuel] = 1;
            if (player->fuel <= 0.0f)
                player->fuel = 0;
        }
        if (CloakZeroesShields(player)) {
            player->shield = 0;
            if (frameCounter % 10 == 0)
                statusRedraw[kRedrawShield] = 1;
        } else {
            n = CloakShieldUsage(player);
            if (n > 0 && player->shield >= n) {
                player->shield -= n * 0.03333 * gSpeedMult;
                if (frameCounter % 10 == 0)
                    statusRedraw[kRedrawShield] = 1;
                if (player->shield <= 0.0f)
                    player->shield = 0;
            }
        }
    }

    if (playerHasTribbles && frameCounter % 250 == 0) {
        n = TotalFleetHolds() - TotalCargo(player);
        for (i = 0; i < kMaxJunks; i++) {
            if ((junkData[i].flags & kJunkTribbles) && junkData[i].held > 0) {
                if (n > 0) {
                    junkData[i].held += 1;
                    statusRedraw[kRedrawCargo] = 1;
                }
            }
        }
    }
    if (playerHasPerishableJunk && frameCounter % 250 == 0) {
        for (i = 0; i < kMaxJunks; i++) {
            if ((junkData[i].flags & kJunkPerishable) && junkData[i].held > 0) {
                if (n > 0) {
                    junkData[i].held -= 1;
                    statusRedraw[kRedrawCargo] = 1;
                }
            }
        }
    }

    if (newbieFlag < 2 && frameCounter % 60 == 0) {
        origin.x = origin.y = 0;
        dist = FloatAbs(XYFloatSquaredDist(&player->pos,
                                         &origin));
        if (newbieFlag == -3 && newbieString[0][0] && newbieString[1][0]) {
            n = FindNearestStellar(shipData);
            if (n != -1) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                if (stellarData[n].flags & kSpobIsStation)
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
#ifdef MACBUILD
                GetIndString(name, kKeyNameStrList, keyPrefs.land + 1);
                PtoCstr(name);
#else
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.land);
#endif
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[3]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 0x200, 0, kCommTextSize, &rgbWhite, 0, 0);
            }
            newbieFlag = -2;
        }
        if (newbieFlag == -1 && newbieString[4][0]) {
            if (FindNearestStellar(shipData) != -1) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                pstrcpy(msg, newbieString[4]);
                PtoCstr(msg);
#ifdef MACBUILD
                GetIndString(name, kKeyNameStrList, keyPrefs.map + 1);
                PtoCstr(name);
#else
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.map);
#endif
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[5]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
#ifdef MACBUILD
                GetIndString(name, kKeyNameStrList, keyPrefs.jump + 1);
                PtoCstr(name);
#else
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.jump);
#endif
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[6]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 0x200, 0, kCommTextSize, &rgbWhite, 0, 0);
            }
            newbieFlag = 0;
        }
        if (dist > 2000000.0 && newbieFlag <= 0) {
            if (!disabled && newbieString[8][0]) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                DisplayComm((PStrChar *)newbieString[8], 0x200, 0, kCommTextSize, &rgbWhite, 0, 0);
                newbieFlag = 1;
            }
        } else if (dist > 10000000.0 && newbieFlag == 1) {
            if (!disabled && newbieString[9][0]) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                DisplayComm((PStrChar *)newbieString[9], 0x200, 0, kCommTextSize, &rgbWhite, 0, 0);
                newbieFlag = 2;
            }
        }
    }

    if (player->credits < 0) {
        player->credits = 0;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (player->fuel < 0.0f) {
        player->fuel = 0;
        statusRedraw[kRedrawFuel] = 1;
    }
    n = FindFirstCoLocatedSystem(player->curSystem);
    if (systemData[n].exploreLevel < 1)
        systemData[n].exploreLevel = 1;
    if (systemData[player->curSystem].exploreLevel < 1)
        systemData[player->curSystem].exploreLevel = 1;
    if (player->heading != heading)
        playerTurnedThisFrame = 1;

    if (showFloatingMap > 0 && !quickKeyCheckResult
#ifdef MACBUILD
            && !QuickKeyCheck(kScanShift) && !QuickKeyCheck(kScanOption)
            && !QuickKeyCheck(kScanControl) && !QuickKeyCheck(kScanCommand)) {
#else
            && !QuickKeyCheck(kScanLShift) && !QuickKeyCheck(kScanRShift)
            && !QuickKeyCheck(kScanLCtrl) && !QuickKeyCheck(kScanRCtrl)
            && !QuickKeyCheck(kScanLAlt) && !QuickKeyCheck(kScanRAlt)) {
#endif
        if ((QuickKeyCheck(kScanMinus) || QuickKeyCheck(kScanPadMinus)) && !mapZoomFlag
                && floatingMapZoom < 2.0) {
            floatingMapZoom *= 1.333333;
            floatingMapStartTime = ReadTick();
            DrawMapInFloatingFrame();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }
        if ((QuickKeyCheck(kScanEquals) || QuickKeyCheck(kScanPadPlus)) && !mapZoomFlag
                && floatingMapZoom > 0.5) {
            floatingMapZoom *= 0.75;
            floatingMapStartTime = ReadTick();
            DrawMapInFloatingFrame();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }
        if (QuickKeyCheck(kScanEquals) || QuickKeyCheck(kScanMinus) || QuickKeyCheck(kScanPadMinus)
                || QuickKeyCheck(kScanPadPlus))
            mapZoomFlag = 1;
        else
            mapZoomFlag = 0;
    }
}

#define kCommRefused        0x35
#define kCommDisabled       0x36

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
// MAC: _HandlePlayerCommunication i386 0x00061f48 ppc 0x00069e14
void HandlePlayerCommunication(void)
{
    ShipObj *me;
    short pers;
    short mission;
    Str255 buf;
    short found;
    Boolean canHail;
    short i;
    short newShip;
    Boolean canLand;

    me = shipData;

    if (ShipIsCloaked(me) == 0 && IsDying(me) == 0 &&
            me->jumpTimer <= 0.0f) {
#ifdef MACBUILD
        if (me->targetShip == -1 || QuickKeyCheck(kScanOption) != 0) {
#else
        if (me->targetShip == -1 || QuickKeyCheck(kScanLAlt) != 0 ||
                QuickKeyCheck(kScanRAlt) != 0) {
#endif
            if (me->navTargetKind == kNavTargetStellar && me->targetStellar != -1) {
                canLand = !(stellarData[me->targetStellar].flags
                           & kSpobUninhabited);
                if (StellarIsLandable(&stellarData[me->targetStellar]) == 0)
                    canLand = 0;
                if ((stellarData[me->targetStellar].flags2 & kSpobGateOrWormhole) != 0)
                    canLand = 0;
                if (stellarData[me->targetStellar].loaded == 0 || !canLand) {
                    PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
                    GetIndString(buf, kGameStrList, kCommRefused);
                    DisplayComm((PStrChar *)buf, kCommFrames, 0, kCommTextSize,
                                &rgbWhite, 0, 0);
                } else {
                    SetRetainedWindow((WindowRef)gWind, false);
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoPlanetCommDialog(me->targetStellar);
                    SetRetainedWindow((WindowRef)gWind, true);
                    SetAllKeyCmdFlags();
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
                if (shipClass[shipData[me->targetShip].type].inherentGovtB != -1 &&
                        (govtData[shipClass[shipData[me->targetShip].type]
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
                    if ((personData[pers].flags & kPersOfferWhenBoarded) == 0) {
                    offeringFromShip = 1;
                    offeringShipIndex = me->targetShip;
                    if (IsMissionAvailable(
                            personData[shipData[me->targetShip].persType].linkMission, 1) != 0) {
                        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                        SetRetainedWindow((WindowRef)gWind, false);
                        HideCursorSprite();
                        RefreshSAT();
                        ShowTheCursor();
                        mission = OfferOneMission(
                            personData[shipData[me->targetShip].persType].linkMission);
                        SetRetainedWindow((WindowRef)gWind, true);
                        SetAllKeyCmdFlags();
                        RefreshSAT();
                        if (mission == 1) {
                            if ((personData[shipData[me->targetShip].persType].flags
                                    & kPersDeactivateOnAccept) != 0)
                                personData[shipData[me->targetShip].persType].loaded = 0;
                            if ((personData[shipData[me->targetShip].persType].flags
                                    & kPersLeavesOnAccept) != 0)
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
                                    if (currentMissions[found].shipGoal == kMissionGoalEscort)
                                        AIMakeEscortFlyInForm(&shipData[newShip]);
                                    shipData[me->targetShip].active = 0;
                                    me->targetShip = newShip;
                                    weapTargetExpand = 0.0F;
                                    statusRedraw[kRedrawTarg] = 1;
                                }
                            }
                        }
                    } else {
                        SetRetainedWindow((WindowRef)gWind, false);
                        HideCursorSprite();
                        RefreshSAT();
                        ShowTheCursor();
                        DoCommDialog(me->targetShip);
                        SetRetainedWindow((WindowRef)gWind, true);
                        SetAllKeyCmdFlags();
                    }
                    offeringFromShip = 0;
                    offeringShipIndex = me->targetShip;
                    } else {
                    SetRetainedWindow((WindowRef)gWind, false);
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetRetainedWindow((WindowRef)gWind, true);
                    SetAllKeyCmdFlags();
                    }
                  } else {
                    SetRetainedWindow((WindowRef)gWind, false);
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetRetainedWindow((WindowRef)gWind, true);
                    SetAllKeyCmdFlags();
                  }
                } else {
                    SetRetainedWindow((WindowRef)gWind, false);
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetRetainedWindow((WindowRef)gWind, true);
                    SetAllKeyCmdFlags();
                }
                RedrawStatus(1);
                RedrawComm(0);
            } else {
                PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
                GetIndString(buf, kGameStrList, kCommRefused);
                DisplayComm((PStrChar *)buf, kCommFrames, 0, kCommTextSize,
                            &rgbWhite, 0, 0);
            }
        } else if (IsDisabled(&shipData[me->targetShip]) != 0) {
            PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, kCommRefused);
            DisplayComm((PStrChar *)buf, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
        } else {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, kCommDisabled);
            DisplayComm((PStrChar *)buf, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
        }
    } else {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    }
}

/* abs is inlined only when <stdlib.h> declares it; a local extern emits a call. */


/* FirePlayerWeapon (mac _FirePlayerWeapon). STATE: MATCH 100.0%, exe 829 / obj 829,
   byte-identical (3257 bytes), relocregcheck 0 of 137. mac: MATCH both slices. OPEN: none */
// FUNCTION: EVNOVA 0x00455150
// MAC: _FirePlayerWeapon i386 0x00062702 ppc 0x0006a510
void FirePlayerWeapon(short w)
{
    unsigned long long key;
    KeyCheckFlag keyed[4];
    short i;
    Boolean soundOK;
    Boolean didFire;
    short shot;
    short n;
    short fired;
    short beamAngle;
    short angle;
    short diff;
    short ammoType;
    short slot;

#ifndef MAC1010
    key = shipData[5].licenseCode;
#endif
    KeyMismatch(keyed[0], key, shipData[5].licenseCode, 0xf6100bf185059c3dULL, 0x0ec22bc220ac327aULL);
    KeyMismatch(keyed[1], key, shipData[5].licenseCode, 0xa298c3a6ef99fff6ULL, 0x05ac9ee5e4ba1ec7ULL);
    KeyMismatch(keyed[2], key, shipData[5].licenseCode, 0xa63b2518c45f7e5bULL, 0x010d795bcf7c9f6aULL);
    KeyMismatch(keyed[3], key, shipData[5].licenseCode, 0xe38ddae6a46ec9deULL, 0x0659dbdcb54d0094ULL);
    for (i = 0; i < 4; i++) {
        if (keyed[i] == 0)
            return;
    }

    if (ShipIsCloaked(shipData)) {
        if ((weaponStats[w].flags2 & kWeapFireWhileCloaked) == 0)
            return;
    }

    shipData->fireTick = ReadTick();

    if ((shipClass[shipData->type].shanFlags & kShanFoldingParts)
            && (shipClass[shipData->type].shanFlags & kShanUnfoldsWhenFiring)
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

            if (weaponStats[w].guidance == kGuidanceUnguided
                    || weaponStats[w].guidance == kGuidanceHoming
                    || weaponStats[w].guidance == kGuidanceFreefall
                    || weaponStats[w].guidance == kGuidanceRocket) {
                SpawnShot(0, shipData->targetShip, w, 0);
                fired = fired + 1;
                didFire = 1;
            }
            if (weaponStats[w].guidance == kGuidanceBeam) {
                SpawnBeam(0, shipData->targetShip, w, shipData->heading, -1, -1);
                fired++;
                didFire = 1;
            }
            if (weaponStats[w].guidance == kGuidanceTurretBeam
                    && shipData->targetShip != -1) {
                beamAngle = CalcInterceptAngle(
                    &shipData->pos,
                    &shipData[shipData->targetShip].pos);
                if (!TurretBlindSpot(shipData, beamAngle, w)) {
                    SpawnBeam(0, shipData->targetShip, w, beamAngle, -1, -1);
                    fired++;
                    didFire = 1;
                }
            }
            if (weaponStats[w].guidance == kGuidanceTurret
                    && shipData->targetShip != -1) {
                if (!TurretBlindSpot(shipData, CalcInterceptAngle(
                        &shipData->pos,
                        &shipData[shipData->targetShip].pos), w)) {
                    SpawnShot(0, shipData->targetShip, w, 0);
                    fired++;
                    didFire = 1;
                }
            }
            if (weaponStats[w].guidance == kGuidanceFrontQuad
                    || weaponStats[w].guidance == kGuidanceRearQuad) {
                if (shipData->targetShip != -1) {
                    angle = CalcInterceptAngle(
                        &shipData->pos,
                        &shipData[shipData->targetShip].pos);
                    if (weaponStats[w].guidance == kGuidanceFrontQuad)
                        diff = abs(AngularDifference(
                            angle, shipData->heading));
                    if (weaponStats[w].guidance == kGuidanceRearQuad)
                        diff = abs(AngularDifference(
                            angle,
                            ((short)(shipData->heading + kHalfTurn) % kFullCircle)));
                    if (diff <= 45) {
                        SpawnShot(0, shipData->targetShip, w, 0);
                        fired++;
                        didFire = 1;
                    } else {
                        if (weaponStats[w].guidance == kGuidanceFrontQuad) {
                            SpawnShot(0, -1, w, 0);
                            fired++;
                            didFire = 1;
                        }
                    }
                } else {
                    if (weaponStats[w].guidance == kGuidanceFrontQuad) {
                        SpawnShot(0, -1, w, 0);
                        fired++;
                        didFire = 1;
                    }
                }
            }
            if (weaponStats[w].guidance == kGuidanceBay) {
                if (LaunchFighter(shipData, w)) {
                    fired++;
                    didFire = 1;
                }
            }
            if (!didFire)
                continue;

            if (weaponStats[w].flags2 & kWeapUsesShipSprite)
                shipData->weapGlow = 32.0f;
            ammoType = weaponStats[w].ammoType;
            if (ammoType == -1)
                continue;
            if (weaponStats[w].flags3 & kWeapAmmoAtBurstEnd)
                continue;
            if (ammoType >= 0) {
                if (weaponStats[w].guidance == kGuidanceBay)
                    slot = w;
                else if (ammoType >= 0 && ammoType < kMaxWeapTypes)
                    slot = ammoType;
                else
                    slot = -1;
                if (slot != -1) {
                    shipData->weap[slot].ammo--;
                    if (shipData->weap[slot].ammo < 0)
                        shipData->weap[slot].ammo = 0;
                }
                statusRedraw[kRedrawWeap] = 1;
            } else if (ammoType <= -kWeapAmmoFuelBase) {
                shipData->fuel -= (abs(ammoType) - kWeapAmmoFuelBase) * 0.1;
                if (shipData->fuel < 0.0f)
                    shipData->fuel = 0.0f;
                statusRedraw[kRedrawFuel] = 1;
            }
        }

        if (fired > 0) {
            soundOK = 1;
            if (weaponStats[w].flags & kWeapLoopSound) {
                if (weaponStats[w].sound >= 0) {
                    if (ST_IsSoundPlaying(shotSnd[weaponStats[w].sound]))
                        soundOK = 0;
                }
            }
            if (weaponStats[w].sound >= 0 && soundOK) {
                if (weaponStats[w].guidance == kGuidanceBeam
                        || weaponStats[w].guidance == kGuidanceTurretBeam) {
                    PlaySoundDistance(2, shotSnd[weaponStats[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                    soundOK = 0;
                }
            }
            if (weaponStats[w].sound < 0)
                soundOK = 0;
            if (soundOK) {
                if (weaponStats[w].guidance == kGuidanceBay)
                    PlaySoundDistance(-1, shotSnd[weaponStats[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                else if (weaponStats[w].flags & kWeapSecondary)
                    PlaySoundDistance(-1, shotSnd[weaponStats[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                else
                    PlaySoundDistance(-1, shotSnd[weaponStats[w].sound], 5,
                                      &shipData->pos, &shipData->pos);
            }

            /* recoil */
            if (weaponStats[w].recoil > 0) {
                AdjustedAccel(((short)(shipData->heading + kHalfTurn) % kFullCircle),
                              weaponStats[w].recoil
                                  / (float)shipClass[shipData->type].mass,
                              &shipData->vel,
                              shipClass[shipData->type].maxSpeed);
            }

            if (weaponStats[w].flags & kWeapFireSimultaneously)
                shipData->weap[w].reloadTimer = weaponStats[w].reload;
            else
                shipData->weap[w].reloadTimer =
                    fired * (weaponStats[w].reload / shipData->weap[w].count);

            if (weaponStats[w].flags3 & kWeapExclusive) {
                for (i = 0; i < kMaxWeapTypes; i++) {
                    if (i != w) {
                        if (shipData->weap[i].reloadTimer
                                < shipData->weap[w].reloadTimer + 2.0f)
                            shipData->weap[i].reloadTimer =
                                shipData->weap[w].reloadTimer + 2.0f;
                    }
                }
            }

            if (weaponStats[w].burstCount > 0) {
                shipData->weap[w].burstCount++;

                /* per-burst ammo is charged once the burst completes */
                if ((weaponStats[w].flags3 & kWeapAmmoAtBurstEnd)
                        && shipData->weap[w].burstCount
                            % weaponStats[w].burstCount == 0) {
                    if (weaponStats[w].flags & kWeapFireSimultaneously)
                        shot = fired;
                    else
                        shot = 1;
                    if (weaponStats[w].guidance == kGuidanceBay) {
                        shipData->weap[w].ammo -= shot;
                        if (shipData->weap[w].ammo < 0)
                            shipData->weap[w].ammo = 0;
                    } else {
                        ammoType = weaponStats[w].ammoType;
                        if (ammoType >= 0 && ammoType < kMaxWeapTypes) {
                            shipData->weap[weaponStats[w].ammoType].ammo -= shot;
                            if (shipData->weap[weaponStats[w].ammoType].ammo < 0)
                                shipData->weap[weaponStats[w].ammoType].ammo = 0;
                            statusRedraw[kRedrawWeap] = 1;
                        } else if (ammoType <= -kWeapAmmoFuelBase) {
                            shipData->fuel -=
                                (abs(ammoType) - kWeapAmmoFuelBase) * 0.1 * shot;
                            if (shipData->fuel < 0.0f)
                                shipData->fuel = 0.0f;
                            statusRedraw[kRedrawFuel] = 1;
                        }
                    }
                }

                if (shipData->weap[w].burstCount
                        >= WeaponBurstInterval(w, shipData->weap[w].count)) {
                    shipData->weap[w].burstCount = 0;
                    shipData->weap[w].reloadTimer = weaponStats[w].burstReload;
                }
            }
        }
    }
}

/* PlayerLandOnStellar (mac _PlayerLandOnStellar). STATE: MATCH 100.0%, exe 397 / obj 397,
   relocregcheck 0 of 124; the landing-position test is written `!= -1' first. OPEN: none */
// FUNCTION: EVNOVA 0x00455e10
// MAC: _PlayerLandOnStellar i386 0x000631c4 ppc 0x0006b244
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
    SetRetainedWindow((WindowRef)gWind, false);
    HideCursorSprite();
    RefreshSAT();
    RedrawStatus(1);
    ShowTheCursor();
    if (!settings.noMovies)
        PlayQTMovieFromDesc(ship->targetStellar + kFirstResourceID);
    if (!SpobUninhabited(&stellarData[ship->targetStellar]))
        DoAutoRefuel();
    else
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    RedrawStatus(1);
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    SWSetPortToWindow(gSpriteWorld);
    ship->targetShip = -1;
    absquatulateStr[0] = 0;
    statusRedraw[kRedrawTarg] = 1;
    statusRedraw[kRedrawCargo] = 1;
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
    if (systemData[sys].exploreLevel < 2)
        systemData[sys].exploreLevel = 2;
    if (systemData[ship->curSystem].exploreLevel < 2)
        systemData[ship->curSystem].exploreLevel = 2;
    AutoSetExploration(ship->curSystem, 0, 2);
    ExploreNebulaeFromSystem(ship->curSystem);
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0) {
            if ((currentMissions[i].flags & kMissionInfiniteAuxShips) != 0)
                currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
            currentMissions[i].auxShipDelay = Rand(0x46) + 0x46;
            currentMissions[i].auxShipsSpawned = 0;
        }
    }
    SavePilotFile(ship->targetStellar);
    ship->heading = Rand(kFullCircle);
    SetRetainedWindow((WindowRef)gWind, true);
    SetAllKeyCmdFlags();
    playerIsLanded = 0;
    if (absquatulateStr[0] != 0) {
        DisplayComm((PStrChar *)absquatulateStr, 500, 0, kCommTextSize, &rgbWhite, 0, 0);
        RedrawComm(1);
        absquatulateStr[0] = 0;
    } else if (newbieFlag > 2) {
        unsigned char msg[256];
        unsigned char name[256];
        short n;

        n = Rand(5);
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, n + 0x37);
        strcat((char *)msg, " ");
        strncpy((char *)name, (char *)stellarData[dockTarget].name, 0x3f);
        PtoCstr(name);
        strcat((char *)msg, (char *)name);
        strcat((char *)msg, " ");
        StrcatIndCString((char *)msg, kGameStrList, 0x3c);
        strcat((char *)msg, " ");
        DateToLongString(gameTime, (char *)name);
        strcat((char *)msg, (char *)name);
        strcat((char *)msg, ".");
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
        RedrawComm(1);
    } else {
        newbieFlag = -1;
    }
    dockTarget = -1;
    dockPermission = -1;
    gSpeedMult = 0.0;
    killShots = 1;
    SWHideAllSprites(gSpriteWorld);
    for (i = 0; i < kMaxShots; i++)
        shotData[i].life = -2.0f;
    lastMouseMoveTime = ReadTick() - 60;
}


/* PlayerEnterHypergate (mac _PlayerEnterHypergate). STATE: MATCH 100.0%, exe 492 / obj 492,
   byte-identical, relocregcheck 0 of 161. The hyperlink count is held in `link', so the scan
   loop's `link' is a split web; `n' carries the scan's system and the nav-link count, as in
   PlayerEnterWormhole. OPEN: none. */
// FUNCTION: EVNOVA 0x00456480
// MAC: _PlayerEnterHypergate i386 0x000637bf ppc 0x0006b87c
short PlayerEnterHypergate(void)
{
    short i;
    short destStellar;
    short destSyst;
    short link;
    short n;
    ShipObj *ship;
    unsigned char msg[256];
    unsigned char name[256];

    ship = shipData;
    shipData->navTargetKind = -1;
    shipData->targetStellar = -1;
    link = StellarNumHyperLinks(&stellarData[dockTarget]);
    if (link == 0)
        return -1;
    ShowTheCursor();
    SetRetainedWindow((WindowRef)gWind, false);
    HideCursorSprite();
    RefreshSAT();
    hypergateMapMode = 1;
    hypergateMapModeStellar = dockTarget;
    DoSystemMap();
    hypergateMapMode = 0;
    hypergateMapModeStellar = -1;
    SetRetainedWindow((WindowRef)gWind, true);
    SetAllKeyCmdFlags();
    if (shipData->navTargetKind == kNavTargetGate && shipData->targetStellar != -1) {
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
            SetRetainedWindow((WindowRef)gWind, false);
            HideCursorSprite();
            RefreshSAT();
            if (!settings.noMovies)
                PlayQTMovieFromDesc(ship->targetStellar + kFirstResourceID);
            SetRetainedWindow((WindowRef)gWind, true);
            SetAllKeyCmdFlags();
            ship->curSystem = destSyst;
            n = FindFirstCoLocatedSystem(destSyst);
            if (systemData[n].exploreLevel < 1)
                systemData[n].exploreLevel = 1;
            if (systemData[destSyst].exploreLevel < 1)
                systemData[destSyst].exploreLevel = 1;
            mapScroll.h = systemData[destSyst].pos.h;
            mapScroll.v = systemData[destSyst].pos.v;
            ship->pos.x = stellarData[destStellar].pos.h;
            ship->pos.y = stellarData[destStellar].pos.v;
            if (stellarData[destStellar].customSound >= 0
                && stellarData[destStellar].customSound <= kFullCircle - 1)
                ship->heading = stellarData[destStellar].customSound;
            else
                ship->heading = Rand(kFullCircle);
            ship->vel.x = ship->vel.y = 0;
            ship->accelMag = (ShipMaxSpeed(ship) * 0.5);
            Accel(ship->heading, ship->accelMag, &ship->vel);
#if V111
            SWSetPortToWorkArea(gSpriteWorld);
            ForeColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
            if (useLegacyRenderingPipeline) {
                BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                             &gSpriteWorld->workFrameP->frameRect,
                             &gSpriteWorld->windowFrameP->frameRect);
            } else {
                SWSetPortToWindow(gSpriteWorld);
                CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                         GetPortPixMapBits(gSpriteWorld->windowFrameP->framePort),
                         &gSpriteWorld->workFrameP->frameRect,
                         &gSpriteWorld->windowFrameP->frameRect, 0, NULL);
                QDFlushPortBuffer(gSpriteWorld->windowFrameP->framePort, NULL);
            }
#ifdef MACBUILD
            if (!settings.windowed && !settings.noHyperGamma && !ignoreKeys)
                FadeWhiteOut();
#endif
#else
#ifdef MAC1010
            SWSetPortToWorkArea(gSpriteWorld);
            ForeColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
#else
            SWSetPortToWindow(gSpriteWorld);
#ifdef CEFIX /* F22.02 */
            CE_SetFlashColor(whiteColor);
#else
            ForeColor(whiteColor);
#endif
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
            ScreenEndScene();
            SWSetPortToWorkArea(gSpriteWorld);
#endif
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &gSpriteWorld->workFrameP->frameRect,
                         &gSpriteWorld->windowFrameP->frameRect);
#endif
            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, 0x2e);
            strcat((char *)msg, " ");
            pstrcpy(name, systemData[destSyst].name);
            PtoCstr(name);
            strcat((char *)msg, (char *)name);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, 0x30);
            strcat((char *)msg, " ");
            DateToLongString(gameTime, (char *)name);
            strcat((char *)msg, (char *)name);
            strcat((char *)msg, ".");
            n = 0;
            for (i = 0; i < kNavSlots; i++) {
                if (systemData[ship->curSystem].navDefault[i] != -1)
                    n++;
            }
            if (n == 0) {
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x31);
            }
            CtoPstr((char *)msg);
            if (systemData[ship->curSystem].murk < 0)
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
            DisplayComm((PStrChar *)msg, 0x168, 0, kCommTextSize, &rgbWhite, 0, 0);
            killShots = 1;
            killBoxes = 1;
            killExplods = 1;
            killSmoke = 1;
            killAsteroids = 1;
            crashlog_print("travelled through hypergate to system ");
            crashlog_print_num(shipData->curSystem + kFirstResourceID);
            crashlog_print(" (");
            crashlog_print_pstr(systemData[shipData->curSystem].name);
            crashlog_print(")\n");
            shipData->navTargetKind = -1;
            shipData->targetStellar = -1;
            shipData->targetShip = -1;
            hyperRoute[0] = shipData->curSystem;
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            statusRedraw[kRedrawRadar] = 1;
            statusRedraw[kRedrawTarg] = 1;
            statusRedraw[kRedrawNav] = 1;
            RedrawStatus(1);
            return destStellar;
        }
    }
    GetIndString(msg, kGameStrList, 0x32);
    DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
    return -1;
}

/* PlayerEnterWormhole (mac _PlayerEnterWormhole). STATE: MATCH 100.0%, exe 564 / obj 564,
   relocregcheck 0 of 156. The customSound test is written in-range arm first. OPEN: none. */
// FUNCTION: EVNOVA 0x00456ca0
// MAC: _PlayerEnterWormhole i386 0x00064005 ppc 0x0006c064
short PlayerEnterWormhole(void)
{
    short count;
    short i;
    short destStellar;
    short dest;
    short link;
    short pick;
    short n;
    ShipObj *ship;
    unsigned char msg[256];
    unsigned char name[256];

    ship = shipData;
    dest = -1;
    destStellar = -1;
    if (StellarNumHyperLinks(&stellarData[dockTarget]) > 0) {
        do {
            n = 0;
            for (i = 0; i < kSpobHyperLinks; i++) {
                if (stellarData[dockTarget].hyperLinks[i] != -1) {
                    if (FindActiveCoLocatedSystem(stellarData[stellarData[dockTarget].hyperLinks[i]].system) != -1)
                        n++;
                }
            }
            if (n == 0) {
                extern char *strcpy(char *, const char *);

                strcpy((char *)msg, "");
                StrcatIndCString((char *)msg, kGameStrList, 0x54);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x56);
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                return -1;
            }
            pick = Rand(8);
            i = pick;
            link = stellarData[dockTarget].hyperLinks[i];
            if (link != -1) {
                destStellar = link;
                dest = stellarData[link].system;
            }
        } while (dest == -1);
    } else {
        count = 0;
        for (i = 0; i < kMaxStellars; i++) {
            if (i != dockTarget && stellarData[i].loaded != 0
                && (stellarData[i].flags2 & kSpobWormhole) != 0
                && stellarData[i].system != -1
                && ship->curSystem != stellarData[i].system
                && StellarNumHyperLinks(&stellarData[i]) == 0
                && FindActiveCoLocatedSystem(stellarData[i].system) != -1)
                count++;
        }
        if (count > 0) {
            while (dest == -1) {
                i = Rand(kMaxStellars);
                if (i != dockTarget && stellarData[i].loaded != 0
                    && (stellarData[i].flags2 & kSpobWormhole) != 0
                    && stellarData[i].system != -1
                    && ship->curSystem != stellarData[i].system) {
                    if (StellarNumHyperLinks(&stellarData[i]) == 0
                        && FindActiveCoLocatedSystem(stellarData[i].system) != -1) {
                        destStellar = i;
                        dest = stellarData[i].system;
                    }
                    break;
                }
            }
        } else {
            extern char *strcpy(char *, const char *);

            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, 0x54);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, 0x56);
            CtoPstr((char *)msg);
            DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            return -1;
        }
    }
    if (dest == -1) {
        dest = ship->curSystem;
        destStellar = dockTarget;
    }
    SetRetainedWindow((WindowRef)gWind, false);
    if (!settings.noMovies)
        PlayQTMovieFromDesc(ship->targetStellar + kFirstResourceID);
    RefreshSAT();
    SetRetainedWindow((WindowRef)gWind, true);
    SetAllKeyCmdFlags();
    ship->curSystem = dest;
    n = FindFirstCoLocatedSystem(dest);
    if (systemData[n].exploreLevel < 1)
        systemData[n].exploreLevel = 1;
    if (systemData[dest].exploreLevel < 1)
        systemData[dest].exploreLevel = 1;
    mapScroll.h = systemData[dest].pos.h;
    mapScroll.v = systemData[dest].pos.v;
    ship->pos.x = stellarData[destStellar].pos.h;
    ship->pos.y = stellarData[destStellar].pos.v;
    if (stellarData[destStellar].customSound >= 0
        && stellarData[destStellar].customSound <= 0x167)
        ship->heading = stellarData[destStellar].customSound;
    else
        ship->heading = Rand(kFullCircle);
    ship->vel.x = ship->vel.y = 0;
    ship->accelMag = ShipMaxSpeed(ship) * 0.5;
    Accel(ship->heading, ship->accelMag, &ship->vel);
    SWSetPortToWindow(gSpriteWorld);
#ifdef CEFIX /* F22.03 */
    CE_SetFlashColor(whiteColor);
#else
    ForeColor(whiteColor);
#endif
    PaintRect(&gSpriteWorld->windRect);
    ForeColor(blackColor);
#ifndef MACBUILD
    ScreenEndScene();
#endif
    PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
    strcpy((char *)msg, "");
    StrcatIndCString((char *)msg, kGameStrList, 0x2f);
    strcat((char *)msg, " ");
    pstrcpy(name, systemData[dest].name);
    PtoCstr(name);
    strcat((char *)msg, (char *)name);
    strcat((char *)msg, " ");
    StrcatIndCString((char *)msg, kGameStrList, 0x30);
    strcat((char *)msg, " ");
    DateToLongString(gameTime, (char *)name);
    strcat((char *)msg, (char *)name);
    strcat((char *)msg, ".");
    count = 0;
    for (i = 0; i < kNavSlots; i++) {
        if (systemData[ship->curSystem].navDefault[i] != -1)
            count++;
    }
    if (count == 0) {
        strcat((char *)msg, " ");
        StrcatIndCString((char *)msg, kGameStrList, 0x31);
    }
    CtoPstr((char *)msg);
    DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
    killShots = 1;
    killBoxes = 1;
    killExplods = 1;
    killSmoke = 1;
    killAsteroids = 1;
    if (systemData[ship->curSystem].murk < 0)
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

#ifdef MACBUILD /* mac spelling */
#define dockMission badLicense
#define dockDistX spawnStellar
#else
#define dockMission sVal2
#define dockDistX distX
#endif
#ifdef MAC1010
#define sVal2 sVal
#define badLicense sVal
#define distX spawnStellar
#endif

/* HandlePlayerDockRequest (mac _HandlePlayerDockRequest). STATE: win MATCH 100.0%, 2120/2120,
   relocregcheck 0 of 378; byte-identical yes (9159 bytes). mac MATCH both slices.
   OPEN: nothing. */
// FUNCTION: EVNOVA 0x00457580
// MAC: _HandlePlayerDockRequest i386 0x00066691 ppc 0x0006e890
void HandlePlayerDockRequest(void)
{
#ifdef MAC1010
    short spawnStellar;
    short distY;
    unsigned char msg[256];
    Boolean granted;
    short i;
    int iVal2;
    short qtStellar;
    short commResult;
    unsigned char t1;
    ShipObj *ship = shipData;
    unsigned char pbuf[256];
    short sVal;
#elif defined(MACBUILD)
    short distY;
    unsigned char msg[256];
    Boolean granted;
    short i;
    int iVal2;
    short spawnStellar;
    short qtStellar;
    short sVal2;
    short commResult;
    ShipObj *ship = shipData;
    unsigned char pbuf[256];
    short distX;
    short badLicense;
    short sVal;
#else
    ShipObj *ship = shipData;
    unsigned char msg[256];
    Boolean granted;
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
    Boolean nameMatch;
    short commResult;
    unsigned char pbuf[256];
    unsigned int distX;
    unsigned int distY;
    unsigned long long lic;
    unsigned long hash;
    unsigned char regBuf[17] = "";
    unsigned char *regName;
    int licSerial;
    short badLicense;
    short sVal;
#endif

#ifdef MAC1010
    do {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
        long g;
        char *p;
        long long mem;
        unsigned char x;
        unsigned char y;
        unsigned char c;
        char t2;
        unsigned short rem;
        Boolean ok;
        unsigned int v;

        g = shipData[36].licenseCopies;
        p = (char *)shipData[17].licenseName;
        mem = shipData[5].licenseCode;
        x = 0;
        if (p != 0 && *p != 0) {
            while (StringGetLength((const unsigned char *)p) != 0 && StringGetLength(key) + 1 < 17)
                StringAppendSafe(key, (const unsigned char *)p, 17);
        }
        x ^= (unsigned char)g;
        x += (char)0xe7;
        x ^= (unsigned char)g;
        x = ROTL(x, 3);
        x ^= name[5];
        x += 0x31;
        x = ROTL(x, 7);
        c = key[0];
        x ^= c;
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x += 0x0b;
        t1 = name[2];
        x ^= t1;
        x ^= key[13];
        x ^= name[13];
        x += (char)0xa6;
        x ^= (unsigned char)g;
        x ^= name[12];
        x ^= (unsigned char)g;
        t2 = key[1];
        x ^= t2;
        x ^= (unsigned char)g;
        x ^= c;
        x ^= key[4];
        x += 0x67;
        x ^= name[6];
        x ^= (unsigned char)g;
        x += 0x58;
        if (x > t2)
            x ^= (unsigned char)g;
        x = ROTL(x, 2);
        x = ((x + (char)0xa0) & 0xff) + 0x7d;
        x ^= (unsigned char)g;
        x += 0x5a;
        x = ROTL(x, 7);
        x ^= (unsigned char)g;
        x += (char)0x93;
        x ^= (unsigned char)g;
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x ^= name[9];
        rem = RT3_ExtractLicenseBlock2(mem);
        x += 0x78;
        x = ROTL(x, 2);
        x ^= key[2];
        x ^= t1;
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x = ROTL(x, 5);
        x = ROTL(x, 5);
        x ^= (unsigned char)g;
        if (x < (char)key[14])
            x ^= (unsigned char)g;
        if (x > (char)key[4])
            x ^= name[4];
        x ^= (unsigned char)g;
        x = ROTL(x, 4);
        x ^= key[15];
        x ^= name[14];
        x = ROTL(x, 2);
        x += 0x75;
        x = ROTL(x, 1);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x = ROTL(x, 1);
        x += -0x65;
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x += 0x58;
        x ^= key[15];
        LICENSE_STEP_BEGIN
        y = x * IsPlatformOpen() + -0x61;
        RegScramB2(y);
        x = ROTL(y, 5);
        LICENSE_STEP_END
        x ^= key[3];
        TimerGetSeconds();
        x += (char)0xa4;
        x ^= key[1];
        x ^= key[14];
        x += 0x29;
        x = ROTL(x, 2);
        x += (char)0xd1;
        x ^= (unsigned char)g;
        x += 0x82;
        x ^= name[0];
        x ^= name[1];
        x ^= (unsigned char)g;
        x += -0x7c;
        RegScramB2(x);
        y = x & 0xff;
        x = ROTL(y, 5);
        x += -0x4e;
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x = ROTL(x, 5);
        x += 0x5a;
        x ^= name[11];
        x ^= (unsigned char)g;
        x = ROTL(x, 5);
        x ^= (unsigned char)g;
        x += (char)0xe8;
        x ^= key[7];
        x += (char)0xdd;
        x ^= key[2];
        x ^= key[5];
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x = ROTL(x, 7);
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x += 0x36;
        x ^= (unsigned char)g;
        if (x < 0x23)
            x = ROTL(x, 4);
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x ^= key[11];
        if (x < 3)
            x = blob[x];
        while (rem >= x)
            rem -= x;
        ok = (g != 0 && rem == 0);
        badLicense = ((v = shipData[36].licenseCopies) != 0
                && ok == 0 ? (unsigned char)1 : (unsigned char)0);
    } while (0);
#elif defined(MACBUILD)
    {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
        unsigned long g;
        char *p;
        long long mem;
        unsigned char x;
        unsigned char y;
        unsigned char c;
        char b;
        unsigned char t1;
        char t2;
        unsigned short rem;
        Boolean ok;
        unsigned int v;

        g = shipData[36].licenseCopies;
        p = RT3_CheckLicenseName(shipData[17].licenseName);
        mem = shipData[5].licenseCode;
        x = 0;
        if (p != 0 && *p >= '0' && *p <= '9')
            g = 1;
        if (p != 0 && *p != 0) {
            while (StringGetLength(p) != 0 && StringGetLength(key) + 1 < 17)
                StringAppendSafe(key, p, 17);
        }
        x ^= g;
        x += 0xe7;
        x ^= g;
        x = ROTL(x, 3);
        x ^= name[5];
        x += 0x31;
        x = ROTL(x, 7);
        c = key[0];
        x ^= c;
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x += 0x0b;
        t1 = name[2];
        x ^= t1;
        x ^= key[13];
        x ^= name[13];
        x += 0xa6;
        x ^= g;
        x ^= name[12];
        x ^= g;
        t2 = key[1];
        x ^= (unsigned char)t2;
        x ^= g;
        x ^= c;
        x ^= key[4];
        x += 0x67;
        x ^= name[6];
        x ^= g;
        x += 0x58;
        if (x > t2)
            x ^= g;
        x = ROTL(x, 2);
        x ^= RegScramE2();
        y = x + 0x1d;
        y ^= g;
        y += 0x5a;
        x = ROTL(y, 7);
        x ^= g;
        x += 0x93;
        x ^= g;
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x ^= name[9];
        rem = RT3_ExtractLicenseBlock2(mem);
        x += 0x78;
        x = ROTL(x, 2);
        x ^= key[2];
        x ^= t1;
        x ^= g;
        x ^= g;
        x = ROTL(x, 5);
        x = ROTL(x, 5);
        x ^= g;
        if (x < (char)key[14])
            x ^= g;
        if (x > (char)key[4])
            x ^= name[4];
        x ^= g;
        x = ROTL(x, 4);
        x ^= key[15];
        x ^= name[14];
        x = ROTL(x, 2);
        x += 0x75;
        RegScramC2(x);
        x = ROTL(x, 1);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x = ROTL(x, 1);
        x += -0x65;
        x = ROTL(x, 1);
        x ^= g;
        x += 0x58;
        x ^= key[15];
        x = RegScramA2(x);
        x += -0x61;
        RegScramB2(x);
        RegScramC2(x);
        x = ROTL(x, 5);
        x ^= key[3];
        b = x;
        x = RegScramD2(&b);
        x += 0xa4;
        x ^= key[1];
        x ^= key[14];
        x += 0x29;
        x = ROTL(x, 2);
        x += 0xd1;
        x ^= g;
        x = (x + 0x82) % 256;
        x ^= name[0];
        x ^= name[1];
        x ^= g;
        x += -0x7c;
        RegScramB2(x);
        x = ROTL(x, 0);
        x = ROTL(x, 5);
        x += -0x4e;
        x = ROTL(x, 1);
        x ^= g;
        x = ROTL(x, 5);
        x += 0x5a;
        x ^= name[11];
        x ^= g;
        x = ROTL(x, 5);
        x ^= g;
        x += 0xe8;
        x ^= key[7];
        x += 0xdd;
        x ^= key[2];
        x ^= key[5];
        x = ROTL(x, 1);
        x ^= g;
        x ^= g;
        x = ROTL(x, 7);
        x = ROTL(x, 1);
        x ^= g;
        x += 0x36;
        x ^= g;
        if (x < 0x23)
            x = ROTL(x, 4);
        x ^= g;
        x ^= g;
        x ^= key[11];
        if (x < 3)
            x = blob[x];
        while (rem >= x)
            rem -= x;
        ok = (g != 0 && rem == 0);
        x ^= key[10];
        x += 0x4d;
        x = ROTL(x, 3);
        x = ROTL(x, 4);
        x += 0xee;
        x ^= key[9];
        x ^= RegScramE2();
        x += -0x4c;
        x = ROTL(x, 4);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x ^= key[8];
        x = ROTL(x, 6);
        x ^= key[2];
        x = ROTL(x, 7);
        v = x + 0x57;
        x = v;
        if (x != 0)
            RegScramC2(v);

        badLicense = shipData[36].licenseCopies != 0 && ok == 0;
    }
#else
    licSerial = shipData[36].licenseCopies;
    regName = shipData[17].licenseName;
    lic = shipData[5].licenseCode;
    sb = 0;
    scram = 0;
    if (regName != 0 && *regName != 0) {
        while (StringGetLength(regName) != 0 &&
               StringGetLength(regBuf) + 1U < 0x11) {
            StringAppendSafe(regBuf, regName, 0x11);
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
    badLicense = (shipData[36].licenseCopies != 0 && nameMatch == 0)
        ? (unsigned char)1 : (unsigned char)0;
#endif
    if (badLicense && TimerGetElapsedSeconds() > 0x4b0)
#ifdef MACBUILD
        FT_FileDelete(-3, "EV Nova License");
#else
        FT_FileDelete(-3, "EV Nova License.lcs");
#endif

    if (!StellarIsLandable(&stellarData[ship->targetStellar]) ||
        ship->curSystem != stellarData[ship->targetStellar].system ||
        stellarData[ship->targetStellar].loaded == 0)
        goto cantDock;

    iVal2 = stellarData[ship->targetStellar].landingFee;
    if (iVal2 > 0 && stellarData[ship->targetStellar].dominated == 0 &&
        iVal2 > shipData->credits) {
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, 0x3d);
        strcat((char *)msg, " ");
        strcat((char *)msg, creditsCString);
        strcat((char *)msg, " ");
        if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
            StrcatIndCString((char *)msg, kGameStrList, 0x3e);
        } else if (stellarData[ship->targetStellar].flags & kSpobIsStation) {
            StrcatIndCString((char *)msg, kGameStrList, 0x3f);
        } else {
            StrcatIndCString((char *)msg, kGameStrList, 0x40);
        }
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
        dockTarget = -1;
        dockPermission = -1;
        goto done;
    }
    if (ShipIsCloaked(ship)) {
        GetIndString(msg, kGameStrList, 0x49);
        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        goto done;
    }
    if (IsDisabled(ship))
        goto done;
    if (IsDying(ship))
        goto done;
    dockLand = stellarData[ship->targetStellar].flags & kSpobIsStation;
#ifdef MACBUILD
    iVal2 = !(stellarData[ship->targetStellar].flags & kSpobUninhabited);
    if (iVal2 == 0) {
#else
    if (stellarData[ship->targetStellar].flags & kSpobUninhabited) {
#endif
        dockPermission = 0x2ee;
        ship->aiTimer = 0;
        dockTarget = ship->targetStellar;
        GetIndString(msg, kGameStrList, 0x35);
        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
    }
    if (dockTarget == ship->targetStellar) {
        float fdy;
        float fdx;

        fdx = FloatAbs(ship->vel.x);
        fdy = FloatAbs(ship->vel.y);
        if (dockPermission >= 0x2ee &&
            ship->aiTimer <= 0.0f &&
            fdx <= 0.75 &&
            fdy <= 0.75) {
            distX = abs((short)(ship->pos.x
                    - stellarData[ship->targetStellar].pos.h));
            distY = abs((short)(ship->pos.y
                    - stellarData[ship->targetStellar].pos.v));
            if (stellarData[ship->targetStellar].sprite->curFrameP) {
                sVal = GetStellarSpriteXSize(ship->targetStellar) * 1.75;
            } else {
                sVal = 0x4b;
            }
            if ((short)distX < sVal && (short)distY < sVal) {

                offeringFromShip = 0;
                dockedPort = &stellarData[ship->targetStellar];
                if (lastMissionStellar != ship->targetStellar || lastMissionStellar < 0) {
                    for (sVal2 = 0; sVal2 < kMaxMissions; sVal2++)
                        missionRandom[sVal2] = Rand(kDailyRollRange) + 1;
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
                if (stellarData[dockTarget].landingFee > 0 && stellarData[dockTarget].dominated == 0) {
                    shipData->credits -= stellarData[dockTarget].landingFee;
                    if (shipData->credits < 0)
                        shipData->credits = 0;
                }
                qtStellar = -1;
                commResult = -1;
                crashlog_print("landing on spob ");
                crashlog_print_num(ship->targetStellar + kFirstResourceID);
                crashlog_print(" (");
                crashlog_print_pstr(stellarData[ship->targetStellar].name);
                crashlog_print(")\n");
                FlushEvents(1, 0);
                FlushEvents(2, 0);
                FlushEvents(3, 0);
                FlushEvents(4, 0);
                FlushEvents(5, 0);
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    qtStellar = PlayerEnterHypergate();
                    if (qtStellar == -1) {
                        dockTarget = -1;
                        dockPermission = 0;
                        granted = 0;
                    } else {
                        granted = 1;
                    }
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
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
                if ((shipClass[ship->type].shanFlags & kShanFoldingParts) != 0 &&
                    (shipClass[ship->type].shanFlags & kShanUnfoldsWhenFiring) != 0) {
                    ship->foldSet = shipClass[ship->type].baseSets - 1;
                    ship->foldDir = 0;
                } else {
                    ship->foldSet = 0;
                    ship->foldDir = 1;
                }
                for (sVal = 0; sVal < kMaxBeams; sVal++) {
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
                statusRedraw[kRedrawRadar] = 1;
                statusRedraw[kRedrawTarg] = 1;
                statusRedraw[kRedrawWeap] = 1;
                statusRedraw[kRedrawNav] = 1;
                statusRedraw[kRedrawShield] = 1;
                statusRedraw[kRedrawCargo] = 1;
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
                if (systemData[ship->curSystem].murk < 0)
                    ClearStarfield();
                LoadIntfResource();
                SWSetPortToBackground(gSpriteWorld);
                SetSystemBackgroundColor();
                PaintRect(&gSpriteWorld->windRect);
                ForeColor(blackColor);
                SWSetPortToWindow(gSpriteWorld);
                SWFlagRectAsChanged(gSpriteWorld, &gSpriteWorld->windRect);
                RedrawComm(1);
                if (qtStellar != -1 || commResult != -1) {
                    MissionHandlePlayerEnteredNewSystem();
                    for (sVal = 0; sVal < kMaxActiveMissions; sVal++) {
                        if (currentMissionObjectives[sVal].active != 0 &&
                            currentMissions[sVal].shipSyst == -6 &&
                            currentMissions[sVal].shipCount > 0) {
                            if (currentMissions[sVal].shipBehav == kMissionModeEscort) {
                                currentMissions[sVal].shipDelay = -1;
                                currentMissions[sVal].specialShipsJumpedIn = 0;
                            } else {
                                currentMissions[sVal].shipDelay = 0x7fff;
                                currentMissions[sVal].specialShipsJumpedIn =
                                    currentMissions[sVal].shipCount;
                                if (currentMissions[sVal].shipBehav == kMissionModeAttackPlayer) {
                                    if (dudeData[currentMissions[sVal].shipDude].govt != -1) {
                                        if (qtStellar != -1 &&
                                            (govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & kGovtNoHypergates) == 0) {
                                            if ((govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & kGovtPrefersHypergates) != 0 ||
                                                Rand(2) == 0) {
                                                currentMissions[sVal].shipDelay = 0;
                                                currentMissions[sVal].specialShipsJumpedIn = 0;
                                            }
                                        } else if (commResult != -1 &&
                                                   ((govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & kGovtPrefersWormholes) != 0 ||
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
                    if (qtStellar != -1)
                        spawnStellar = qtStellar;
                    else
                        spawnStellar = commResult;
                    for (dockMission = 0; dockMission < kMaxActiveMissions; dockMission++) {
                        if (currentMissionObjectives[dockMission].active != 0 &&
                            currentMissions[dockMission].shipSyst == -6 &&
                            currentMissions[dockMission].shipCount > 0 &&
                            currentMissions[dockMission].shipBehav == kMissionModeAttackPlayer &&
                            currentMissions[dockMission].shipDelay == -1) {
                            for (i = 1; i < kMaxShips; i++) {
                                if (shipData[i].active != 0 &&
                                    dockMission == shipData[i].specialMission) {
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
                } else {
                    SetupShipsInSystem(ship->curSystem, 1);
                }
                HandleStellarSystemVisibility();
                ship->targetStellar = -1;
                CueNextHyperRouteDest();
                SpawnBountyHunter();
                if (qtStellar != -1 || commResult != -1) {
                    if (qtStellar != -1)
                        spawnStellar = qtStellar;
                    else
                        spawnStellar = commResult;
                    for (sVal = 1; sVal < kMaxShips; sVal++) {
                        if (shipData[sVal].active != 0 &&
                            shipData[sVal].parentShip == 0) {
                            AIMakeShipEmergeFromHyperGate(&shipData[sVal], spawnStellar);
                            shipData[sVal].pos.x = stellarData[spawnStellar].pos.h;
                            shipData[sVal].pos.y = stellarData[spawnStellar].pos.v;
                            shipData[sVal].heading = ship->heading;
                            shipData[sVal].aiTimer = (Rand(0x14) + 0xf);
                        }
                    }
                }
                for (sVal = 0; sVal < kMaxStellars; sVal++) {
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
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    GetIndString(msg, kGameStrList, 0x41);
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
                    GetIndString(msg, kGameStrList, 0x42);
                } else if (dockLand != 0) {
                    GetIndString(msg, kGameStrList, 0x43);
                } else {
                    GetIndString(msg, kGameStrList, 0x44);
                }
                DisplayComm((PStrChar *)msg, 0x168, 0, kCommTextSize, &rgbWhite, 0, 0);
                goto done;
            }
        } else {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            if (ship->targetStellar < 0 || ship->targetStellar >= kMaxStellars)
                goto done;
            if (!stellarData[ship->targetStellar].sprite->curFrameP) {
                sVal = 0x4b;
            } else {
                sVal = GetStellarSpriteXSize(ship->targetStellar) * 1.75;
            }
            distX = abs((short)(ship->pos.x
                    - stellarData[ship->targetStellar].pos.h));
            distY = abs((short)(ship->pos.y
                    - stellarData[ship->targetStellar].pos.v));
            if ((short)distX < sVal && (short)distY < sVal &&
                dockPermission >= 0x2ee) {
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    GetIndString(msg, kGameStrList, 0x45);
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
                    GetIndString(msg, kGameStrList, 0x46);
                } else if (dockLand != 0) {
                    GetIndString(msg, kGameStrList, 0x47);
                } else {
                    GetIndString(msg, kGameStrList, 0x48);
                }
                DisplayComm((PStrChar *)msg, 0x168, 0, kCommTextSize, &rgbWhite, 0, 0);
            } else {
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    GetIndString(msg, kGameStrList, 0x41);
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
                    GetIndString(msg, kGameStrList, 0x42);
                } else if (dockLand != 0) {
                    GetIndString(msg, kGameStrList, 0x43);
                } else {
                    GetIndString(msg, kGameStrList, 0x44);
                }
                DisplayComm((PStrChar *)msg, 0x168, 0, kCommTextSize, &rgbWhite, 0, 0);
            }
            goto done;
        }
    }

    /* dockTarget != stellar: the player has just asked to dock here. */
    dockTarget = ship->targetStellar;
    dockDistX = abs((short)(ship->pos.x - stellarData[dockTarget].pos.h));
    distY = abs((short)(ship->pos.y - stellarData[dockTarget].pos.v));
    granted = 0;
    if ((playerRecord[ship->curSystem] >= stellarData[dockTarget].minStatus ||
         stellarData[dockTarget].minStatus == -0x7fff) &&
        stellarData[dockTarget].minStatus != 0x7fff)
        granted = 1;
    if (stellarData[dockTarget].dominated != 0)
        granted = 1;
    if (dockPermission >= 0x2ee)
        granted = 1;
    if (stellarData[dockTarget].govt != -1 &&
        !PlayerMeetsRequirements(
            govtData[stellarData[dockTarget].govt].requires[0],
            govtData[stellarData[dockTarget].govt].requires[1]))
        granted = 0;
    if (!granted) {
        for (sVal = 0; sVal < kMaxActiveMissions; sVal++) {
            if (currentMissionObjectives[sVal].active != 0) {
                if (currentMissions[sVal].travelStellar == shipData->targetStellar)
                    granted = 1;
                if (shipData->targetStellar == currentMissions[sVal].returnStellar)
                    granted = 1;
            }
        }
    }
    if (!granted && stellarData[dockTarget].govt != -1 &&
        PlayerBlessedByGovt(stellarData[dockTarget].govt, 1))
        granted = 1;
    if (granted) {
        if ((short)dockDistX < 0xfa && (short)distY < 0xfa) {
            dockPermission = 0x2ed;
            goto done;
        }
        dockPermission = 0;
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        strcpy((char *)msg, "");
        if (stellarData[dockTarget].flags2 & kSpobHypergate) {
            StrcatIndCString((char *)msg, kGameStrList, Rand(2) + 0x4a);
        } else if (stellarData[dockTarget].flags & kSpobIsStation) {
            if (Rand(3) == 0) {
                strncpy((char *)msg, (char *)stellarData[shipData->targetStellar].name, 0x1f);
                PtoCstr(msg);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x4c);
            } else {
                StrcatIndCString((char *)msg, kGameStrList, 0x4d);
            }
        } else {
            if (Rand(3) == 0) {
                strncpy((char *)msg, (char *)stellarData[shipData->targetStellar].name, 0x1f);
                PtoCstr(msg);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x4e);
            } else {
                StrcatIndCString((char *)msg, kGameStrList, 0x4f);
            }
        }
        if (Rand(2) == 0) {
            strncpy((char *)pbuf, (char *)shipName, 0x40);
            PtoCstr(pbuf);
            strcat((char *)msg, ", ");
            strcat((char *)msg, (char *)pbuf);
        }
        strcat((char *)msg, ". ");
        StrcatIndCString((char *)msg, kGameStrList, 0x50);
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
        goto done;
    } else {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (stellarData[dockTarget].flags2 & kSpobHypergate) {
            GetIndString(msg, kGameStrList, 0x51);
        } else if (stellarData[dockTarget].flags & kSpobIsStation) {
            GetIndString(msg, kGameStrList, 0x52);
        } else {
            GetIndString(msg, kGameStrList, 0x53);
        }
        DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
        dockTarget = -1;
        dockPermission = -1;
        goto done;
    }

cantDock:
    {
        extern char *strcpy(char *, const char *);

        strcpy((char *)msg, "");
    }
    StrcatIndCString((char *)msg, kGameStrList, 0x54);
    strcat((char *)msg, " ");
    if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
        StrcatIndCString((char *)msg, kGameStrList, 0x55);
    } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
        StrcatIndCString((char *)msg, kGameStrList, 0x56);
    } else if (stellarData[ship->targetStellar].flags & kSpobIsStation) {
        StrcatIndCString((char *)msg, kGameStrList, 0x57);
    } else {
        StrcatIndCString((char *)msg, kGameStrList, 0x58);
    }
    if ((stellarData[ship->targetStellar].flags2 & (kSpobHypergate | kSpobWormhole)) == 0) {
        strcat((char *)msg, " ");
        pstrcpy(pbuf, stellarData[ship->targetStellar].name);
        PtoCstr(pbuf);
        strcat((char *)msg, (char *)pbuf);
        strcat((char *)msg, ". ");
        if (stellarData[ship->targetStellar].flags & kSpobIsStation)
            StrcatIndCString((char *)msg, kGameStrList, 0x59);
        else
            StrcatIndCString((char *)msg, kGameStrList, 0x5a);
    }
    CtoPstr((char *)msg);
    DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
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

/* HandlePlayerDockApproach. STATE: MATCH 675/675, byte-identical yes (2679 bytes),
   relocregcheck 0 of 146. OPEN: none -- `inhabited = !(...)' then `canDock = 0' before the
   govt read is what places the entry compare's registers; see notes. */
// FUNCTION: EVNOVA 0x00459950
// MAC: _HandlePlayerDockApproach i386 0x000647a6 ppc 0x0006c824
void HandlePlayerDockApproach(void)
{
    short govt;
    Boolean canDock;
    Boolean inhabited;
    ShipObj *me;
    long targ;
    short i;
    short dx;
    short dy;
    short r;
    unsigned char msg[256];
    unsigned char tmp[256];

    me = shipData;
#ifdef MAC1010
#define targ dockTarget
    if (me->curSystem == stellarData[targ].system) {
#else
    targ = dockTarget;
    if (stellarData[targ].system == shipData[0].curSystem) {
#endif
        if (dockPermission <= -1)
            goto out;
        inhabited = !(stellarData[targ].flags & kSpobUninhabited);
        canDock = 0;
        govt = stellarData[targ].govt;
        if (govt == -1)
            canDock = 1;
        else if (govt != -1) {
            if ((playerRecord[me->curSystem] >= stellarData[targ].minStatus ||
                 stellarData[targ].minStatus == -32767) &&
                stellarData[targ].minStatus != 32767)
                canDock = 1;
        }
        if (stellarData[targ].dominated)
            canDock = 1;
        if (dockPermission >= 750)
            canDock = 1;
        if (!canDock) {
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active) {
                    if (currentMissions[i].travelStellar == shipData[0].targetStellar)
                        canDock = 1;
                    if (shipData[0].targetStellar == currentMissions[i].returnStellar)
                        canDock = 1;
                }
            }
        }
        if (!canDock) {
            if (govt != -1 && PlayerBlessedByGovt(govt, 1))
                canDock = 1;
        }
        if (inhabited && canDock) {
            if (dockPermission < 749) ;
            else
                dockPermission++;
            dx = abs((short)(me->pos.x - stellarData[shipData[0].targetStellar].pos.h));
            dy = abs((short)(me->pos.y - stellarData[shipData[0].targetStellar].pos.v));
            if (dx < 250 && dy < 250 && dockPermission < 750)
                dockPermission = 750;
            if (dockPermission != 750) ;
            else {
                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                strcpy((char *)msg, "");
                if (stellarData[shipData[0].targetStellar].flags2 & kSpobHypergate) {
                    r = Rand(3);
                    if (r == 0) {
                        StrcatIndCString((char *)msg, kGameStrList, kStrGreetSpecialA);
                        strcat((char *)msg, ", ");
                        strncpy((char *)tmp, (char *)shipName, 0x40);
                        PtoCstr(tmp);
                        strcat((char *)msg, (char *)tmp);
                        strcat((char *)msg, ". ");
                    } else if (r == 1) {
                        strncpy((char *)msg, (char *)shipName, 0x40);
                        PtoCstr(msg);
                        strcat((char *)msg, ", ");
                        StrcatIndCString((char *)msg, kGameStrList, kStrGreetSpecialB);
                    } else
                        StrcatIndCString((char *)msg, kGameStrList, kStrGreetSpecialC);
                    CtoPstr((char *)msg);
                } else {
                    if (stellarData[shipData[0].targetStellar].flags & kSpobIsStation) {
                        r = Rand(3);
                        if (r == 0) {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetBigA);
                            strcat((char *)msg, ", ");
                            strncpy((char *)tmp, (char *)shipName, 0x40);
                            PtoCstr(tmp);
                            strcat((char *)msg, (char *)tmp);
                            strcat((char *)msg, ". ");
                        } else if (r == 1) {
                            strncpy((char *)msg, (char *)shipName, 0x40);
                            PtoCstr(msg);
                            strcat((char *)msg, ", ");
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetBigB);
                            strcat((char *)msg, " ");
                        } else {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetBigC);
                            strcat((char *)msg, " ");
                        }
                    } else {
                        r = Rand(3);
                        if (r == 0) {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetSmallA);
                            strcat((char *)msg, ", ");
                            strncpy((char *)tmp, (char *)shipName, 0x40);
                            PtoCstr(tmp);
                            strcat((char *)msg, (char *)tmp);
                            strcat((char *)msg, ". ");
                        } else if (r == 1) {
                            strncpy((char *)msg, (char *)shipName, 0x40);
                            PtoCstr(msg);
                            strcat((char *)msg, ", ");
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetSmallB);
                            strcat((char *)msg, " ");
                        } else {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetSmallC);
                            strcat((char *)msg, " ");
                        }
                    }
                    if (Rand(2) == 0) {
                        StrcatIndCString((char *)msg, kGameStrList, kStrWelcomeShort);
                    } else {
                        StrcatIndCString((char *)msg, kGameStrList, kStrWelcomeTo);
                        strcat((char *)msg, " ");
                        strncpy((char *)tmp, (char *)stellarData[shipData[0].targetStellar].name, 0x1f);
                        PtoCstr(tmp);
                        strcat((char *)msg, (char *)tmp);
                        strcat((char *)msg, ". ");
                    }
                    if (stellarData[dockTarget].landingFee > 0 &&
                        stellarData[dockTarget].dominated == 0) {
                        strcat((char *)msg, "  ");
                        if (stellarData[dockTarget].flags2 & kSpobHypergate)
                            StrcatIndCString((char *)msg, kGameStrList, kStrFeeSpecial);
                        else if (stellarData[dockTarget].flags & kSpobIsStation)
                            StrcatIndCString((char *)msg, kGameStrList, kStrFeeBig);
                        else
                            StrcatIndCString((char *)msg, kGameStrList, kStrFeeSmall);
                        strcat((char *)msg, " ");
                        StrcatCommaNumber(stellarData[dockTarget].landingFee, (char *)msg);
                        strcat((char *)msg, " ");
                        strcat((char *)msg, creditsCString);
                        StrcatIndCString((char *)msg, kGameStrList, kStrFeeTail);
                    }
                    CtoPstr((char *)msg);
                }
                DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
            }
            if (dockPermission >= 0x800) {
                dockPermission = -1;
                dockTarget = -1;
            }
        } else if (canDock) {
            if (dockPermission < 749) ;
            else
                dockPermission++;
            dx = abs((short)(me->pos.x - stellarData[dockTarget].pos.h));
            dy = abs((short)(me->pos.y - stellarData[dockTarget].pos.v));
            if (dx < 250 && dy < 250 && dockPermission < 750)
                dockPermission = 750;
            if (dockPermission == 750 && dockTryFlag) {
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                GetIndString(msg, kGameStrList, kStrAutoClearance);
                DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
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
#ifdef MAC1010
#undef targ
#endif

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

                                                            /* 0x004694a0 fighter bay */
                                                            /* 0x00466fc0 */

/* per-mission state, stride 0x14 */

/* HandlePlayerBoardAttempt (mac _HandlePlayerBoardAttempt). STATE: win MATCH 100%, exe 1355 / obj
   1355, byte-identical yes (relocs masked); mac MATCH on both slices. OPEN: none. */
// FUNCTION: EVNOVA 0x0045a3d0
// MAC: _HandlePlayerBoardAttempt i386 0x00065000 ppc 0x0006d078
void HandlePlayerBoardAttempt(void)
{
    extern char *strcpy(char *dst, const char *src);   /* 0x004d6010 MSL, CALLED not inlined */

    unsigned char msg[256];
    unsigned char pbuf[256];
    float limit;
    float dist;
    float limit2;
    ShipObj *me;
    Boolean canBoard;
    Boolean skip;
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
        (shipData[me->targetShip].specialMission != -1 || shipData[me->targetShip].formerEscort < 0))
#if defined(MACBUILD) || V111
        goto reject;
#else
    {
reject:
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        GetIndString(msg, kGameStrList, kStrCannotBoard);
        DisplayComm(msg, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
        goto out;
    }
#endif
    if (!IsDisabled(&shipData[me->targetShip]))
        goto reject;
    if (shipData[me->targetShip].active == 0)
        goto reject;
    if (shipData[me->targetShip].curSystem != me->curSystem)
        goto reject;
    if (shipData[me->targetShip].persType == kPersCaptainHector)
        goto reject;
    if (IsDying(me))
        goto reject;

    if (FloatAbs(shipData[me->targetShip].vel.x - me->vel.x) <= 0.5) ; else
        goto tooFast;
    if (FloatAbs(shipData[me->targetShip].vel.y - me->vel.y) <= 0.5) ; else
        goto tooFast;
    if (FloatAbs(shipData[me->targetShip].pos.x - me->pos.x)
            <= (float)(GetSpriteXSize((SpriteRec *)shipData[me->targetShip].sprite) * 0.5)) ; else
        goto notClose;
    if (FloatAbs(shipData[me->targetShip].pos.y - me->pos.y)
            <= (float)(GetSpriteYSize((SpriteRec *)shipData[me->targetShip].sprite) * 0.5)) ; else
        goto notClose;

    if (abs(AngularDifference(me->heading, shipData[me->targetShip].heading)) > 30) {
        if (abs(AngularDifference(me->heading,
                ((short)shipData[me->targetShip].heading + kHalfCircle) % kFullCircle)) > 30)
            goto out;
    }

    if (shipData[me->targetShip].specialMission != -1) {
        m = shipData[me->targetShip].specialMission;
        if (currentMissionObjectives[m].active) {
            if (currentMissions[m].pickupMode == 2) {
                SetRetainedWindow((WindowRef)gWind, false);
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                canBoard = DoMissionCargoPickup(currentMissions[m].cargoType, currentMissions[m].cargoQty);
                SetRetainedWindow((WindowRef)gWind, true);
                SetAllKeyCmdFlags();
                if (canBoard) {
                    if (shipData[me->targetShip].specialMission != -1 &&
                        currentMissionObjectives[shipData[me->targetShip].specialMission].active)
                        currentMissions[shipData[me->targetShip].specialMission].specialShipsBoarded++;
#ifdef MACBUILD
                    AIShipBoardedByPlayer(&shipData[me->targetShip]);
#else
                    ClearTargetsOnShip(&shipData[me->targetShip]);
#endif
                    shipData[me->targetShip].boarded = 1;
                    currentMissions[m].hasCargo = 1;
                    statusRedraw[kRedrawCargo] = 1;
                    strcpy((char *)msg, "");
                    StrcatIndCString((char *)msg, kGameStrList, kStrTookCargo);
                    strcat((char *)msg, " ");
                    if (lcCargoName[currentMissions[m].cargoType][1] != '*') {
                        StrcatIndCString((char *)msg, kGameStrList, kStrCargoSome);
                        strcat((char *)msg, " ");
                    }
                    pstrcpy(pbuf, lcCargoName[currentMissions[m].cargoType]);
                    PtoCstr(pbuf);
                    if (lcCargoName[currentMissions[m].cargoType][1] == '*')
                        strcat((char *)msg, (char *)pbuf + 1);
                    else
                        strcat((char *)msg, (char *)pbuf);
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, kStrCargoTail);
                    CtoPstr((char *)msg);
                    DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else {
                    skip = 1;
                }
            } else if ((currentMissions[m].shipGoal == kMissionGoalBoard || currentMissions[m].shipGoal == kMissionGoalRescue) &&
                       (currentMissions[m].flags & kMissionAutoAbort) &&
                       currentMissions[m].shipCount == 1) {
                if (currentMissions[m].flags & kMissionTakesFuel) {
                    pstrcpy(msg, shipClass[shipData[me->targetShip].type].name);
                    PtoCstr(msg);
                    strcat((char *)msg, ":  ");
                    pstrcpy(pbuf, fuelString[0]);
                    PtoCstr(pbuf);
                    strcat((char *)msg, (char *)pbuf);
                    strcat((char *)msg, ", ");
                    pstrcpy(pbuf, shipName);
                    PtoCstr(pbuf);
                    strcat((char *)msg, (char *)pbuf);
                    strcat((char *)msg, ".");
                } else {
                    strcpy((char *)msg, "");
                    StrcatIndCString((char *)msg, kGameStrList, kStrDropoffDone);
                }
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                if (currentMissionObjectives[shipData[me->targetShip].specialMission].active)
                    currentMissions[shipData[me->targetShip].specialMission].specialShipsBoarded++;
                shipData[me->targetShip].aiTimer = 100.0;
                shipData[me->targetShip].boarded = 1;
#ifdef MACBUILD
                AIShipBoardedByPlayer(&shipData[me->targetShip]);
#else
                ClearTargetsOnShip(&shipData[me->targetShip]);
#endif
                canBoard = 1;
            } else {
                canBoard = 1;
            }
        } else {
            canBoard = 1;
        }
    } else {
        if (shipClass[shipData[me->targetShip].type].crew > 0)
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
#ifdef MAC1010
        m = shipData[me->targetShip].specialMission;
#else
        k = me->targetShip;
        m = shipData[k].specialMission;
#endif
        if (m == -1)
            j = 1;
        else if (currentMissionObjectives[m].active && currentMissionObjectives[m].failed)
            j = 1;
        if (j == 1 && (j = shipData[me->targetShip].persType) != kPersCaptainHector && j != kPersBountyHunter) {
            SlapWithPenalty(shipData[0].curSystem, shipData[me->targetShip].govt, 2, m);
            if (shipData[me->targetShip].persType != -1) {
                if (personData[shipData[me->targetShip].persType].linkMission != -1) {
                    if (personData[shipData[me->targetShip].persType].flags &
                        kPersOfferWhenBoarded) {
                        offeringFromShip = 1;
                        offeringShipIndex = me->targetShip;
                        if (IsMissionAvailable(personData[shipData[me->targetShip].persType].linkMission, 1)) {
                            SetRetainedWindow((WindowRef)gWind, false);
                            HideCursorSprite();
                            RefreshSAT();
                            ShowTheCursor();
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            if (OfferOneMission(personData[shipData[me->targetShip].persType].linkMission) != 1)
                                skip = 1;
                            SetRetainedWindow((WindowRef)gWind, true);
                            SetAllKeyCmdFlags();
                            RefreshSAT();
                            if (personData[shipData[me->targetShip].persType].flags &
                                kPersDeactivateOnAccept)
                                personData[shipData[me->targetShip].persType].loaded = 0;
                        } else {
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            SetRetainedWindow((WindowRef)gWind, false);
                            HideCursorSprite();
                            RefreshSAT();
                            DoPlunderDialog();
                            SetRetainedWindow((WindowRef)gWind, true);
                            SetAllKeyCmdFlags();
                        }
                        offeringFromShip = 0;
                        offeringShipIndex = -1;
                    } else {
                        if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                            shipData[me->targetShip].formerEscort == 0) {
                            GetIndString(msg, kGameStrList, kStrRefuseYield);
                            DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                            PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                            shipData[me->targetShip].formerEscort = -1;
                            shipData[me->targetShip].aiType = kShipTypeFighter;
                            shipData[me->targetShip].parentShip = 0;
                            shipData[me->targetShip].shield = 0;
                            shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                            shipData[me->targetShip].targetStellar = -1;
                            AIAddFighterToParent(&shipData[me->targetShip]);
                        } else {
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            SetRetainedWindow((WindowRef)gWind, false);
                            HideCursorSprite();
                            RefreshSAT();
                            DoPlunderDialog();
                            SetRetainedWindow((WindowRef)gWind, true);
                            SetAllKeyCmdFlags();
                        }
                    }
                } else {
                    if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                        shipData[me->targetShip].formerEscort == 0) {
                        GetIndString(msg, kGameStrList, kStrRefuseYield);
                        DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                        PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                        shipData[me->targetShip].formerEscort = -1;
                        shipData[me->targetShip].aiType = kShipTypeFighter;
                        shipData[me->targetShip].parentShip = 0;
                        shipData[me->targetShip].shield = 0;
                        shipData[me->targetShip].targetStellar = -1;
                        shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                        AIAddFighterToParent(&shipData[me->targetShip]);
                    } else {
                        DoGoodSamaritan(&shipData[me->targetShip], me);
                        PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                        SetRetainedWindow((WindowRef)gWind, false);
                        HideCursorSprite();
                        RefreshSAT();
                        DoPlunderDialog();
                        SetRetainedWindow((WindowRef)gWind, true);
                        SetAllKeyCmdFlags();
                    }
                }
            } else {
                if (shipData[me->targetShip].formerEscort >= 1 && CanHireEscorts()) {
                    shipData[me->targetShip].aiType = kShipTypeEscort;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].boarded = 0;
                    if (shipData[me->targetShip].formerEscort == 1)
                        shipData[me->targetShip].hired = 1;
                    else
                        shipData[me->targetShip].hired = 0;
                    shipData[me->targetShip].formerEscort = -1;
                    if (shipClass[shipData[me->targetShip].type].flags & kShipArmorDisableTenth)
                        shipData[me->targetShip].armor =
                            ShipArmorCapacity(&shipData[me->targetShip]) * 0.1 + 1.0;
                    else
                        shipData[me->targetShip].armor =
                            ShipArmorCapacity(&shipData[me->targetShip]) * 0.3333 + 1.0;
                    AISetup(&shipData[me->targetShip]);
                    for (k = 0; k < kCargoTypes; k++)
                        shipData[0].cargo[k] += shipData[me->targetShip].cargo[k];
                    statusRedraw[kRedrawCargo] = 1;
                    GetIndString(msg, kGameStrList, kStrEscortJoins);
                    DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                           shipData[me->targetShip].formerEscort == 0) {
                    GetIndString(msg, kGameStrList, kStrRefuseYield);
                    DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                    shipData[me->targetShip].formerEscort = -1;
                    shipData[me->targetShip].aiType = kShipTypeFighter;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].shield = 0;
                    shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                    shipData[me->targetShip].targetStellar = -1;
                    AIAddFighterToParent(&shipData[me->targetShip]);
                } else if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                           shipData[me->targetShip].formerEscort == -1) {
                    GetIndString(msg, kGameStrList, kStrRefuseYieldAlt);
                    DisplayComm((PStrChar *)msg, 250, 0, kCommTextSize, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                    shipData[me->targetShip].formerEscort = -1;
                    shipData[me->targetShip].aiType = kShipTypeFighter;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].shield = 0;
                    shipData[me->targetShip].armor = ShipArmorCapacity(&shipData[me->targetShip]);
                    shipData[me->targetShip].targetStellar = -1;
                    AIAddFighterToParent(&shipData[me->targetShip]);
                } else {
                    DoGoodSamaritan(&shipData[me->targetShip], me);
                    PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                    SetRetainedWindow((WindowRef)gWind, false);
                    HideCursorSprite();
                    RefreshSAT();
                    DoPlunderDialog();
                    SetRetainedWindow((WindowRef)gWind, true);
                    SetAllKeyCmdFlags();
                }
            }
        } else if (m == -1) {
            DoGoodSamaritan(&shipData[me->targetShip], me);
            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
            SetRetainedWindow((WindowRef)gWind, false);
            HideCursorSprite();
            RefreshSAT();
            DoPlunderDialog();
            SetRetainedWindow((WindowRef)gWind, true);
            SetAllKeyCmdFlags();
        }
        if (skip)
            goto out;
        if (me->targetShip == -1)
            goto out;
        shipData[me->targetShip].boarded = 1;
#ifdef MACBUILD
        AIShipBoardedByPlayer(&shipData[me->targetShip]);
#else
        ClearTargetsOnShip(&shipData[me->targetShip]);
#endif
    }
    goto out;

#if V111 /* V111 spelling */
#else
cantBoard:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString(msg, kGameStrList, kStrCannotBoard);
    DisplayComm((PStrChar *)msg, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
    goto out;
#endif
notClose:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString(msg, kGameStrList, kStrNotAligned);
    DisplayComm((PStrChar *)msg, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
    goto out;
tooFast:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString(msg, kGameStrList, kStrTooFast);
    DisplayComm((PStrChar *)msg, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
    goto out;
#ifdef MAC1010
reject:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString(msg, kGameStrList, kStrCannotBoard);
    DisplayComm((PStrChar *)msg, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
#elif V111 /* V111 spelling */
reject:
cantBoard:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString(msg, kGameStrList, kStrCannotBoard);
    DisplayComm(msg, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
#endif
out:;
}

/*
 * WrapObject skips anything further than 15000 units from the
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
// MAC: _WrapPlayerPosition i386 0x00067efa ppc 0x00070490
void WrapPlayerPosition(void)
{
    short i;
    Boolean wrapped;

    wrapped = 0;
    wrapDelta.x = 0.0f;
    wrapDelta.y = 0.0f;
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
        WrapObject(&shipData[i].pos);
    for (i = 0; i < kMaxShots; i++)
        WrapObject(&shotData[i].pos);
    for (i = 0; i < kMaxExplods; i++)
        WrapObject(&explodData[i].pos);
    for (i = 0; i < kMaxBoxes; i++)
        WrapObject(&boxData[i].pos);
    for (i = 0; i < kMaxSmoke; i++)
        WrapObject(&smokeData[i].pos);
    for (i = 0; i < kMaxPods; i++)
        WrapObject(&podData[i].pos);

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

/* WrapObject. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045c6d0
// MAC: _WrapObject i386 0x00066065 ppc 0x0006e268
void WrapObject(FloatPoint *pos)
{
    long dx;
    long dy;

    dx = (pos->x - shipData->pos.x);
    dy = (pos->y - shipData->pos.y);
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (dx < kWrapRange && dy < kWrapRange) {
        pos->x += wrapDelta.x;
        pos->y += wrapDelta.y;
    }
}

/*
 * The flags keep DAT_ names on purpose: the mac reaches each one through its own
 * pointer slot, and those slots are not in the 1.1.1 symbol table. The two tail
 * loops are the mac's - its twin walks 0xdee1c for 5 bytes and then for 4.
 */

/* SetAllKeyCmdFlags. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045c7a0
// MAC: _SetAllKeyCmdFlags i386 0x00061e6d ppc 0x00069cc8
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

#define kEscortAckFrames    250    /* an escort's acknowledgement stays up 250 frames */

/* IssueNewEscortCommand (mac _IssueNewEscortCommand). STATE: MATCH 100.0%,
   exe 409 / obj 409 instrs, byte-identical yes.
   OPEN: nothing. [esp+4] is mwcc's CSE spill of `(long)cmd`, not a local; see notes */
// FUNCTION: EVNOVA 0x0045c880
// MAC: _IssueNewEscortCommand i386 0x000660d5 ppc 0x0006e2f8
Boolean IssueNewEscortCommand(short aiTypeFilter, short cmd, Boolean quiet)
{
    Boolean changed;
    Boolean sawVoice;
    short i;
    short govt;
    short voiceClass;
    short spoken;
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
        if (aiTypeFilter == shipClass[shipData[i].type].escortType ||
            aiTypeFilter == -1) {
            if (cmd != shipData[i].escortOrder) {
                shipData[i].escortMayAttack = 1;
                if (shipData[i].aiType != kShipTypeFighter) {
                    if ((long)cmd == kOrderRegroup) {
                        if ((long)shipData[i].escortOrder != kOrderFormation) {
                            shipData[i].escortOrder = kOrderFormation;
                            spoken = kOrderFormation;
                            changed = 1;
                            if ((shipClass[shipData[i].type].flags2 &
                                 kShipNoFighterVoice) == 0)
                                sawVoice = 1;
                        }
                    } else {
                        shipData[i].escortOrder = cmd;
                        changed = 1;
                        if ((shipClass[shipData[i].type].flags2 & kShipNoFighterVoice) == 0)
                            sawVoice = 1;
                    }
                } else {
                    shipData[i].escortOrder = cmd;
                    changed = 1;
                    if ((shipClass[shipData[i].type].flags2 & kShipNoFighterVoice) == 0)
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
                        govt = shipClass[shipData[i].type].inherentGovtB;
                        voiceClass = shipData[i].voiceClass;
                    }
                    changed = 1;
                }
            }
            if (changed != 0 &&
                (shipClass[shipData[i].type].flags2 & kShipNoFighterVoice) == 0 &&
                (govt < -1 || Rand(2) == 0)) {
                govt = shipClass[shipData[i].type].inherentGovtB;
                voiceClass = shipData[i].voiceClass;
            }
        }
    }
    if (changed != 0 && quiet == 0) {
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, kStrEscortPrefix);
        if (aiTypeFilter == 0)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterFighters);
        else if (aiTypeFilter == 1)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterWarships);
        else if (aiTypeFilter == 2)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterFreighters);
        else if (aiTypeFilter == 3)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterEscorts);
        else
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterAll);
        strcat((char *)msg, " ");
        if (shipData[0].jumpTimer > 0.0f) {
            if ((long)spoken == kOrderRegroup)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase);
            else if ((long)spoken == kOrderFormation)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase + 1);
            else if ((long)spoken == kOrderHold)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase + 2);
            else if ((long)spoken == kOrderDefend)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase + 3);
            else if ((long)spoken == kOrderAttack)
                StrcatIndCString((char *)msg, kGameStrList, kStrAckAttack);
        } else {
            if ((long)spoken == kOrderRegroup)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase);
            else if ((long)spoken == kOrderFormation)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase + 1);
            else if ((long)spoken == kOrderHold)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase + 2);
            else if ((long)spoken == kOrderDefend)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase + 3);
            else if ((long)spoken == kOrderAttack) {
                if (shipData[0].targetShip != -1) {
                    if (shipData[shipData[0].targetShip].parentShip == 0)
                        StrcatIndCString((char *)msg, kGameStrList, kStrAckAttack);
                    else
                        StrcatIndCString((char *)msg, kGameStrList, kStrAckAttackEscort);
                } else
                    StrcatIndCString((char *)msg, kGameStrList, kStrAckAttack);
            }
        }
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, kEscortAckFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
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

EXTERN_C_END
