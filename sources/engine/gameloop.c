// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* gameloop.c -- the main-loop object (tu-map O3, win 0x00428340..0x0043b170). Notes: Research/notes/gameloop.md */
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#include "burgermath.h"
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "sound.h"
#include "space.h"
#include "prototypes.h"
#include "dialogs.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "spriteworld.h"
#include "burger.h"
#include "ai.h"
#include "game.h"
#include "stellar.h"
#include "missions.h"
#include "crashlog.h"
#include "gameutil.h"

#define kSpriteSizeSlots    6
#define kFullHilite         0x20
#define kNoHilite           (-32)

/* shïp shanFlags bits; what bit 0x0002 means is not settled (kShipRecBanks or
   kShipRespawnFighters elsewhere), 0x0080 is kShipKeepFireGroup. */
#define kShipRollAnim       0x0001
#define kShipEngineAnim     0x0002
#define kShipEngineHold     0x0080

/* shïp flags2 (+0x9ea) bit 0 picks the faster cloak rate */
#define kShipFastCloak      0x0001
/* shïp flags (+0x9e8): no death blast */
#define kShipNoDeathBlast   0x0400
/* mïsn objective flags: this one suppresses the failure message */
#define kMisnQuietFailure   0x0400

#define kEngineHoldTicks    0x2d

/* the six sprite slots at the head of ShipObj (+0x00..+0x14) */

#define SP(sh) ((ShipSprites *)(sh))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* përs flags bit 1 keeps the person alive across a death */
#define kPersRespawns       0x0002


                                                                 /* 0x00475f70 */
                                                                 /* 0x00438db0 */

/* HandleShipDisplay (mac _HandleShipDisplay). STATE: MATCH 100%, exe 3665 / obj 3665.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00428340
void HandleShipDisplay(ShipObj *ship)
{
    char hasGlow;
    short damage;
    char altTimed;
    unsigned char msg[256];
    short sizes[kSpriteSizeSlots];
    float dx;
    float dy;
    float rate;
    float ion;
    unsigned short nFrames;
    short h;
    short best;
    short bank;
    short i;
    short m;
    short n;
    short jitter;
    short fade;
    short impact;
    FloatPoint explod;
    char disabled;
    unsigned short blue;
    unsigned short red;
    unsigned short green;
    char hideAll;
    char hasWeap;
    short blastRadius;
    long shieldFrames;
    long frames;
    char hasLight;
    char hasAlt;
    float angleScale;
    char hasShield;
    char advanceAlt;
    short w;

    altTimed = advanceAlt = 0;

    if (ship->active == 0) {
        if (shipData[0].targetShip == ship->index) {
            shipData[0].targetShip = -1;
            statusRedraw[6] = 1;
        }
        if (SP(ship)->sprite != NULL && SP(ship)->sprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->sprite, 0);
        if (SP(ship)->altSprite != NULL && SP(ship)->altSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->altSprite, 0);
        if (SP(ship)->lightSprite != NULL && SP(ship)->lightSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->lightSprite, 0);
        if (SP(ship)->weapSprite != NULL && SP(ship)->weapSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->weapSprite, 0);
        if (SP(ship)->glowSprite != NULL && SP(ship)->glowSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->glowSprite, 0);
        if (SP(ship)->shieldSprite != NULL && SP(ship)->shieldSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->shieldSprite, 0);
        return;
    }

    sizes[0] = 0;
    sizes[1] = 0;
    sizes[2] = 0;
    sizes[3] = 0;
    sizes[4] = 0;
    sizes[5] = 0;
    disabled = IsDisabled(ship);
    hasGlow = hasLight = hasAlt = hasWeap = hasShield = 0;
    hideAll = 0;

    if (!masterShipGlowSprite[ship->type]) {
        if (SP(ship)->glowSprite != NULL && SP(ship)->glowSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->glowSprite, 0);
    } else {
        hasGlow = 1;
    }
    if (!masterShipLightSprite[ship->type]) {
        if (SP(ship)->lightSprite != NULL && SP(ship)->lightSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->lightSprite, 0);
    } else {
        hasLight = 1;
    }
    if (masterShipAltSprite[ship->type] != NULL) {
        if ((gShipTypes[ship->type].shanFlags & kShipNoAltDisabled) != 0 && disabled != 0)
            hasAlt = 0;
        else
            hasAlt = 1;
    } else if (SP(ship)->altSprite != NULL && SP(ship)->altSprite->isVisible != 0) {
        SWSetSpriteVisible(SP(ship)->altSprite, 0);
    }
    if (!masterShipWeaponSprite[ship->type]) {
        if (SP(ship)->weapSprite != NULL && SP(ship)->weapSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->weapSprite, 0);
    } else {
        hasWeap = 1;
    }
    if (!masterShipShieldSprite[ship->type]) {
        if (SP(ship)->shieldSprite != NULL && SP(ship)->shieldSprite->isVisible != 0)
            SWSetSpriteVisible(SP(ship)->shieldSprite, 0);
    } else {
        hasShield = 1;
    }

    nFrames = gShipTypes[ship->type].framesPer;
    angleScale = ((short)nFrames / 360.0);

    if (ship->curSystem == shipData[0].curSystem && ship->active != 0) {
        if (gShipTypes[ship->type].cost != -1) {
            SWSetSpriteVisible(SP(ship)->sprite, 1);
            SWCloneFramesFromMaster(SP(ship)->sprite, masterShipSprite[ship->type]);
            shieldFrames = frames = (short)nFrames;
            bank = 0;
            if (SP(ship)->sprite->numFrames > shieldFrames) {
                if ((gShipTypes[ship->type].shanFlags & kShipRollAnim) != 0) {
                    if (ship->bankDir < 0)
                        bank = 1;
                    else if (ship->bankDir > 0)
                        bank = 2;
                } else if ((gShipTypes[ship->type].shanFlags & kShipEngineAnim) != 0) {
                    if (ship->foldDir > 0) {
                        ship->frameTimer += gTimeScale;
                        if (gShipTypes[ship->type].animDelay < ship->frameTimer) {
                            ship->frameTimer = 0;
                            ship->foldSet += 1;
                            if (ship->foldSet >= gShipTypes[ship->type].baseSets) {
                                ship->foldSet = gShipTypes[ship->type].baseSets - 1;
                                ship->foldDir = 0;
                            }
                        }
                    } else if (ship->foldDir < 0) {
                        ship->frameTimer += gTimeScale;
                        if (gShipTypes[ship->type].animDelay < ship->frameTimer) {
                            ship->frameTimer = 0;
                            ship->foldSet -= 1;
                            if (ship->foldSet <= 0) {
                                ship->foldSet = 0;
                                ship->foldDir = 0;
                            }
                        }
                    } else {
                        ship->frameTimer = 0;
                    }
                    if ((gShipTypes[ship->type].shanFlags & kShipEngineHold) != 0
                            && ship->foldDir < 1
                            && ship->foldSet < gShipTypes[ship->type].baseSets - 1
                            && ReadTick() >= ship->fireTick + kEngineHoldTicks)
                        ship->foldDir = 1;
                    bank = ship->foldSet;
                } else if ((gShipTypes[ship->type].shanFlags & kShipBayAnim) != 0) {
                    if (KeyCarriedShipTypeOnboard(ship) != 0)
                        bank = 1;
                } else if ((gShipTypes[ship->type].shanFlags & kShipAltAnim) != 0) {
                    altTimed = 1;
                    if ((gShipTypes[ship->type].shanFlags & kShipAltWhenDisabled) == 0
                            || disabled == 0)
                        ship->animTimer += gTimeScale;
                    if (gShipTypes[ship->type].animDelay < ship->animTimer) {
                        advanceAlt = 1;
                        if (gShipTypes[ship->type].animDelay >= 1) {
                            while (gShipTypes[ship->type].animDelay < ship->animTimer)
                                ship->animTimer -= gShipTypes[ship->type].animDelay;
                        } else
                            ship->animTimer = 0;
                        if (ship->animSet < 0)
                            ship->animSet = 0;
                        ship->animSet += 1;
                        if (ship->animSet >= gShipTypes[ship->type].baseSets)
                            ship->animSet = 0;
                    }
                    bank = ship->animSet;
                }
            }
            if ((gShipTypes[ship->type].shanFlags & kShipAngleTable) != 0) {
                if (gShipTypes[ship->type].frameAngles) {
                    i = frames - 1;
                    w = i;
                    while (i >= 0) {
                        if (gShipTypes[ship->type].frameAngles[i] < ship->heading) {
                            w = i;
                            break;
                        }
                        i -= 1;
                    }
                } else {
                    w = (ship->heading * angleScale);
                }
            } else {
                w = (ship->heading * angleScale);
            }
            SWSetCurrentFrameIndex(SP(ship)->sprite, (bank * frames + w));
            if (hasGlow != 0) {
                SWCloneFramesFromMaster(SP(ship)->glowSprite, masterShipGlowSprite[ship->type]);
                SWSetCurrentFrameIndex(SP(ship)->glowSprite, (bank * frames + w));
            }
            if (hasLight != 0) {
                SWCloneFramesFromMaster(SP(ship)->lightSprite, masterShipLightSprite[ship->type]);
                SWSetCurrentFrameIndex(SP(ship)->lightSprite, (bank * frames + w));
            }
            if (hasWeap != 0) {
                SWCloneFramesFromMaster(SP(ship)->weapSprite, masterShipWeaponSprite[ship->type]);
                SWSetCurrentFrameIndex(SP(ship)->weapSprite, (bank * frames + w));
            }
            if (hasShield != 0) {
                SWCloneFramesFromMaster(SP(ship)->shieldSprite, masterShipShieldSprite[ship->type]);
                if (SP(ship)->shieldSprite->numFrames > shieldFrames)
                    SWSetCurrentFrameIndex(SP(ship)->shieldSprite,
                                           (bank * frames + w));
                else if (SP(ship)->shieldSprite->numFrames > 1)
                    SWSetCurrentFrameIndex(SP(ship)->shieldSprite, w);
                else
                    SWSetCurrentFrameIndex(SP(ship)->shieldSprite, 0);
            }
            if (disabled != 0) {
                PutSpriteInLayer(SP(ship)->sprite, disabledShipLayer);
                PutSpriteInLayer(SP(ship)->glowSprite, disabledShipLayer);
                PutSpriteInLayer(SP(ship)->lightSprite, disabledShipLayer);
                PutSpriteInLayer(SP(ship)->weapSprite, disabledShipLayer);
                PutSpriteInLayer(SP(ship)->altSprite, disabledShipLayer);
                PutSpriteInLayer(SP(ship)->shieldSprite, disabledShipLayer);
            } else if (ship->parentShip == 0) {
                PutSpriteInLayer(SP(ship)->sprite, escortLayer);
                PutSpriteInLayer(SP(ship)->glowSprite, escortLayer);
                PutSpriteInLayer(SP(ship)->lightSprite, escortLayer);
                PutSpriteInLayer(SP(ship)->weapSprite, escortLayer);
                PutSpriteInLayer(SP(ship)->altSprite, escortLayer);
                PutSpriteInLayer(SP(ship)->shieldSprite, escortLayer);
            } else {
                PutSpriteInLayer(SP(ship)->sprite, shipLayer);
                PutSpriteInLayer(SP(ship)->glowSprite, shipLayer);
                PutSpriteInLayer(SP(ship)->lightSprite, shipLayer);
                PutSpriteInLayer(SP(ship)->weapSprite, shipLayer);
                PutSpriteInLayer(SP(ship)->altSprite, shipLayer);
                PutSpriteInLayer(SP(ship)->shieldSprite, shipLayer);
            }

            SP(ship)->sprite->translucency = gShipTypes[ship->type].transparency;
            if (ship->index == 0) {
                if (gMaxSoundRange < 0)
                    gMaxSoundRange = PlayerEffectiveMurk();
                SP(ship)->sprite->tintMode = 0;
                SP(ship)->sprite->tintColor = gSystems[ship->curSystem].mapPixel;
            } else {
                HandleSpriteMurkiness(ship->pos.x, ship->pos.y,
                                      SP(ship)->sprite);
            }
            SP(ship)->sprite->tintRed = kNoHilite;
            SP(ship)->sprite->tintGreen = kNoHilite;
            SP(ship)->sprite->tintBlue = kNoHilite;
            GetShipPaintColor(ship, &red, &green, &blue);
            SP(ship)->sprite->tintRed += red;
            SP(ship)->sprite->tintGreen += green;
            SP(ship)->sprite->tintBlue += blue;

            if (ship->ionization > 0.0F) {
                ion = ShipIonizationFactor(ship);
                if (ion >= 0.33) {
                    h = (ion * 24.0F * 0.01);
                    if (h > 0x18)
                        h = 0x18;
                    if (h < 0x10)
                        h = 0x10;
                    h += Rand(5) - 2;
                    if (h > SP(ship)->sprite->tintMode || SP(ship)->sprite->tintMode < 8) {
                        SP(ship)->sprite->tintMode = h;
                        SP(ship)->sprite->tintColor = ship->ionColor;
                    } else if (gSpriteWorld->pixelDepth == 8) {
                        SP(ship)->sprite->tintColor = ship->ionColor;
                    } else {
                        SP(ship)->sprite->tintColor = kFullHilite - SP(ship)->sprite->tintMode;
                    }
                }
            } else {
                ship->ionColor = 0;
            }

            if (ship->index != 0) {
                if (AIShipIsEnteringHypergate(ship) != 0) {
                    SP(ship)->sprite->translucency = (32.0F - ship->aiTimer * 2.0F);
                    if (SP(ship)->sprite->translucency < 0)
                        SP(ship)->sprite->translucency = 0;
                    if (SP(ship)->sprite->translucency > kFullHilite)
                        SP(ship)->sprite->translucency = kFullHilite;
                    if (gSpriteWorld->pixelDepth == 0x10) {
                        SP(ship)->sprite->tintMode = SP(ship)->sprite->translucency * 2;
                        if (SP(ship)->sprite->tintMode > kFullHilite)
                            SP(ship)->sprite->tintMode = kFullHilite;
                        SP(ship)->sprite->tintColor = 0xffff;
                    }
                    if (ship->aiTimer < 1.0F) {
                        ship->active = 0;
                        ship->parentShip = -1;
                    }
                } else if (AIShipIsEmergingFromHypergate(ship) != 0) {
                    if (ship->aiTimer <= 16.0F && gSpriteWorld->pixelDepth == 0x10) {
                        fade = (16.0F - ship->aiTimer);
                        if (fade < 0)
                            fade = 0;
                        if (fade > 0x10)
                            fade = 0x10;
                        SP(ship)->sprite->translucency = kFullHilite - fade * 2;
                        if (SP(ship)->sprite->translucency < 0)
                            SP(ship)->sprite->translucency = 0;
                        if (SP(ship)->sprite->translucency > kFullHilite)
                            SP(ship)->sprite->translucency = kFullHilite;
                        SP(ship)->sprite->tintMode = SP(ship)->sprite->translucency * 2;
                        if (SP(ship)->sprite->tintMode > kFullHilite)
                            SP(ship)->sprite->tintMode = kFullHilite;
                        SP(ship)->sprite->tintColor = 0xffff;
                    } else {
                        hideAll = 1;
                    }
                }
            }

            if (hasGlow != 0) {
                SWSetSpriteVisible(SP(ship)->glowSprite, 1);
                n = Rand(6) + (ship->engineGlow - 4);
                if (n > 1) {
                    if (n > kFullHilite)
                        n = kFullHilite;
                    if (ship->index == 0) {
                        SP(ship)->glowSprite->tintMode = 0;
                        SP(ship)->glowSprite->tintColor = gSystems[ship->curSystem].mapPixel;
                    } else {
                        SP(ship)->glowSprite->tintColor = SP(ship)->sprite->tintColor;
                        SP(ship)->glowSprite->tintMode = SP(ship)->sprite->tintMode;
                        if (n > 32.0 - SP(ship)->glowSprite->tintMode * 1.5)
                            n = (32.0 - SP(ship)->glowSprite->tintMode * 1.5);
                        if (n < 0)
                            n = 0;
                    }
                    if (gSpriteWorld->pixelDepth == 8) {
                        SP(ship)->glowSprite->translucency = kFullHilite - n;
                    } else {
                        SP(ship)->glowSprite->translucency = kFullHilite;
                        SP(ship)->glowSprite->tintRed = n;
                        SP(ship)->glowSprite->tintGreen = n;
                        SP(ship)->glowSprite->tintBlue = n;
                    }
                } else {
                    SWSetSpriteVisible(SP(ship)->glowSprite, 0);
                }
            }

            if (hasLight != 0) {
                if (gShipTypes[ship->type].blinkMode == 1) {
                    if ((gShipTypes[ship->type].blinkValue[1] > gShipTypes[ship->type].blinkValue[3]
                                ? gShipTypes[ship->type].blinkValue[1]
                                : gShipTypes[ship->type].blinkValue[3]) < ship->lightTimer)
                        ship->lightTimer =
                            (gShipTypes[ship->type].blinkValue[1] > gShipTypes[ship->type].blinkValue[3]
                                ? gShipTypes[ship->type].blinkValue[1]
                                : gShipTypes[ship->type].blinkValue[3]);
                    if (ship->blinkCount < gShipTypes[ship->type].blinkValue[2]) {
                        if (ship->lightTimer < 0.0F) {
                            if (ship->lightLevel > 0.0F) {
                                ship->lightLevel = 0;
                                ship->blinkCount += 1;
                                ship->lightTimer = gShipTypes[ship->type].blinkValue[0];
                            } else {
                                ship->lightLevel = 32.0F;
                                ship->lightTimer = gShipTypes[ship->type].blinkValue[1];
                            }
                        } else {
                            ship->lightTimer -= gTimeScale;
                        }
                    } else if (ship->lightTimer < 0.0F) {
                        ship->blinkCount = 0;
                        ship->lightLevel = 0;
                        ship->lightTimer = gShipTypes[ship->type].blinkValue[3];
                    } else {
                        ship->lightTimer -= gTimeScale;
                    }
                } else if (gShipTypes[ship->type].blinkMode == 2) {
                    if (ship->blinkCount == 0) {
                        if (gShipTypes[ship->type].blinkValue[2] <= ship->lightLevel) {
                            ship->lightLevel = gShipTypes[ship->type].blinkValue[2];
                            ship->blinkCount = 1;
                        } else {
                            ship->lightLevel += gShipTypes[ship->type].blinkValue[1] * 0.01
                                           * gTimeScale;
                        }
                    } else if (gShipTypes[ship->type].blinkValue[0] >= ship->lightLevel) {
                        ship->lightLevel = gShipTypes[ship->type].blinkValue[0];
                        ship->blinkCount = 0;
                    } else {
                        ship->lightLevel -= gShipTypes[ship->type].blinkValue[3] * 0.01
                                       * gTimeScale;
                    }
                } else if (gShipTypes[ship->type].blinkMode == 3) {
                    if (ship->lightTimer <= 0.0F) {
                        ship->lightLevel = (Rand((gShipTypes[ship->type].blinkValue[1] + 1
                                          - gShipTypes[ship->type].blinkValue[0]))
                                      + gShipTypes[ship->type].blinkValue[0]);
                        ship->lightTimer = gShipTypes[ship->type].blinkValue[2];
                    } else {
                        ship->lightTimer -= gTimeScale;
                        if (gShipTypes[ship->type].blinkValue[2] < ship->lightTimer)
                            ship->lightTimer = gShipTypes[ship->type].blinkValue[2];
                    }
                } else {
                    ship->lightLevel = 32.0F;
                }
                if ((gShipTypes[ship->type].shanFlags & kShipLightsOffDisabled) != 0
                        && disabled != 0)
                    ship->lightLevel = 0;
                if (ship->lightLevel > 1.0F) {
                    SWSetSpriteVisible(SP(ship)->lightSprite, 1);
                    n = ship->lightLevel;
                    if (ship->index == 0) {
                        SP(ship)->lightSprite->tintMode = 0;
                    } else {
                        SP(ship)->lightSprite->tintColor = SP(ship)->sprite->tintColor;
                        SP(ship)->lightSprite->tintMode = (SP(ship)->sprite->tintMode * 0.333);
                        if (n > 32.0 - SP(ship)->lightSprite->tintMode * 1.5)
                            n = (32.0 - SP(ship)->lightSprite->tintMode * 1.5);
                        if (n < 0)
                            n = 0;
                    }
                    if (gSpriteWorld->pixelDepth == 8) {
                        SP(ship)->lightSprite->translucency = kFullHilite - n;
                    } else {
                        SP(ship)->lightSprite->translucency = kFullHilite;
                        SP(ship)->lightSprite->tintRed = n;
                        SP(ship)->lightSprite->tintGreen = n;
                        SP(ship)->lightSprite->tintBlue = n;
                    }
                } else {
                    SWSetSpriteVisible(SP(ship)->lightSprite, 0);
                }
            }

            if (hasWeap != 0) {
                if (ship->weapGlow > 0.0F) {
                    SWSetSpriteVisible(SP(ship)->weapSprite, 1);
                    if (ship->weapGlow > 32.0F)
                        ship->weapGlow = 32.0F;
                    n = ship->weapGlow;
                    if (ship->index == 0) {
                        SP(ship)->weapSprite->tintMode = 0;
                    } else {
                        SP(ship)->weapSprite->tintColor = SP(ship)->sprite->tintColor;
                        SP(ship)->weapSprite->tintMode = SP(ship)->sprite->tintMode;
                        if (n > 32.0 - SP(ship)->weapSprite->tintMode * 1.5)
                            n = (32.0 - SP(ship)->weapSprite->tintMode * 1.5);
                        if (n < 0)
                            n = 0;
                    }
                    if (gSpriteWorld->pixelDepth == 8) {
                        SP(ship)->weapSprite->translucency = kFullHilite - n;
                    } else {
                        SP(ship)->weapSprite->translucency = kFullHilite;
                        SP(ship)->weapSprite->tintRed = n;
                        SP(ship)->weapSprite->tintGreen = n;
                        SP(ship)->weapSprite->tintBlue = n;
                    }
                    ship->weapGlow -= gShipTypes[ship->type].weapDecay * gTimeScale;
                    if (ship->weapGlow < -1.0F)
                        ship->weapGlow = -1.0F;
                } else {
                    SWSetSpriteVisible(SP(ship)->weapSprite, 0);
                }
            }

            if (hasShield != 0) {
                if (ship->shieldGlow > 0.0F && ship->shield > 0.0F) {
                    SWSetSpriteVisible(SP(ship)->shieldSprite, 1);
                    SP(ship)->shieldSprite->translucency = kFullHilite;
                    n = ship->shieldGlow;
                    m = (ship->shield * 32.0 / gShipTypes[ship->type].shield);
                    if (m > kFullHilite)
                        m = kFullHilite;
                    if (m < 4)
                        m = 4;
                    if (m < ship->shieldGlow)
                        ship->shieldGlow = m;
                    if (n > m)
                        n = m;
                    else if (n < 1)
                        n = 1;
                    SP(ship)->shieldSprite->tintRed = n;
                    SP(ship)->shieldSprite->tintGreen = n;
                    SP(ship)->shieldSprite->tintBlue = n;
                    SP(ship)->shieldSprite->tintMode = SP(ship)->sprite->tintMode;
                    SP(ship)->shieldSprite->tintColor = SP(ship)->sprite->tintColor;
                } else {
                    SWSetSpriteVisible(SP(ship)->shieldSprite, 0);
                }
            }

            if (ship->shieldGlow > 0.0F)
                ship->shieldGlow -= gTimeScale;

            if (hasAlt != 0) {
                if ((gShipTypes[ship->type].shanFlags & kShipAltWhenDisabled) == 0
                        || disabled == 0)
                    ship->animTimer += gTimeScale;
                if (altTimed == 0
                        && gShipTypes[ship->type].animDelay < ship->animTimer) {
                    ship->animTimer = 0;
                    advanceAlt = 1;
                }
                if (advanceAlt != 0) {
                    ship->altSet += 1;
                    if (ship->altSet >= gShipTypes[ship->type].altSets)
                        ship->altSet = 0;
                }
                SWSetSpriteVisible(SP(ship)->altSprite, 1);
                SWCloneFramesFromMaster(SP(ship)->altSprite, masterShipAltSprite[ship->type]);
                SWSetCurrentFrameIndex(SP(ship)->altSprite,
                                       (ship->altSet * frames + w));
                SP(ship)->altSprite->translucency = SP(ship)->sprite->translucency;
                SP(ship)->altSprite->tintColor = SP(ship)->sprite->tintColor;
                SP(ship)->altSprite->tintMode = SP(ship)->sprite->tintMode;
                SP(ship)->altSprite->tintRed = SP(ship)->sprite->tintRed;
                SP(ship)->altSprite->tintGreen = SP(ship)->sprite->tintGreen;
                SP(ship)->altSprite->tintBlue = SP(ship)->sprite->tintBlue;
            }

            if (SP(ship)->sprite != NULL)
                SP(ship)->sprite->onTop = 1;
            if (SP(ship)->altSprite != NULL)
                SP(ship)->altSprite->onTop = 1;
            if (SP(ship)->glowSprite != NULL)
                SP(ship)->glowSprite->onTop = 1;
            if (SP(ship)->weapSprite != NULL)
                SP(ship)->weapSprite->onTop = 1;
            if (SP(ship)->lightSprite != NULL)
                SP(ship)->lightSprite->onTop = 1;
            if (SP(ship)->shieldSprite != NULL)
                SP(ship)->shieldSprite->onTop = 1;

            if (SP(ship)->sprite != NULL)
                sizes[0] = GetFrameBiggestSize(SP(ship)->sprite->curFrameP);
            if (hasAlt != 0 && SP(ship)->altSprite->isVisible != 0)
                sizes[1] = GetFrameBiggestSize(SP(ship)->altSprite->curFrameP);
            if (hasGlow != 0 && SP(ship)->glowSprite->isVisible != 0)
                sizes[2] = GetFrameBiggestSize(SP(ship)->glowSprite->curFrameP);
            if (hasWeap != 0 && SP(ship)->weapSprite->isVisible != 0)
                sizes[3] = GetFrameBiggestSize(SP(ship)->weapSprite->curFrameP);
            if (hasLight != 0 && SP(ship)->lightSprite->isVisible != 0)
                sizes[4] = GetFrameBiggestSize(SP(ship)->lightSprite->curFrameP);
            if (hasShield != 0 && SP(ship)->shieldSprite->isVisible != 0)
                sizes[5] = GetFrameBiggestSize(SP(ship)->shieldSprite->curFrameP);

            best = -1;
            n = 0;
            for (i = 0; i < kSpriteSizeSlots; i++) {
                if (sizes[i] > n) {
                    best = i;
                    n = sizes[i];
                }
            }
            if (best == 1)
                SP(ship)->altSprite->onTop = 0;
            else if (best == 2)
                SP(ship)->glowSprite->onTop = 0;
            else if (best == 3)
                SP(ship)->weapSprite->onTop = 0;
            else if (best == 4)
                SP(ship)->lightSprite->onTop = 0;
            else if (best == 5)
                SP(ship)->shieldSprite->onTop = 0;
            else
                SP(ship)->sprite->onTop = 0;
        } else {
            SWSetSpriteVisible(SP(ship)->sprite, 0);
            if (hasGlow != 0)
                SWSetSpriteVisible(SP(ship)->glowSprite, 0);
            if (hasLight != 0)
                SWSetSpriteVisible(SP(ship)->lightSprite, 0);
            if (hasAlt != 0)
                SWSetSpriteVisible(SP(ship)->altSprite, 0);
            if (hasWeap != 0)
                SWSetSpriteVisible(SP(ship)->weapSprite, 0);
            if (hasShield != 0)
                SWSetSpriteVisible(SP(ship)->shieldSprite, 0);
        }

        w = GetSpriteXSize(SP(ship)->sprite);
        h = GetSpriteYSize(SP(ship)->sprite);
        SWSetSpriteLocation(SP(ship)->sprite,
            (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
            (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        if (hasGlow != 0) {
            w = GetSpriteXSize(SP(ship)->glowSprite);
            h = GetSpriteYSize(SP(ship)->glowSprite);
            SWSetSpriteLocation(SP(ship)->glowSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasLight != 0) {
            w = GetSpriteXSize(SP(ship)->lightSprite);
            h = GetSpriteYSize(SP(ship)->lightSprite);
            SWSetSpriteLocation(SP(ship)->lightSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasAlt != 0) {
            w = GetSpriteXSize(SP(ship)->altSprite);
            h = GetSpriteYSize(SP(ship)->altSprite);
            SWSetSpriteLocation(SP(ship)->altSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasWeap != 0) {
            w = GetSpriteXSize(SP(ship)->weapSprite);
            h = GetSpriteYSize(SP(ship)->weapSprite);
            SWSetSpriteLocation(SP(ship)->weapSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasShield != 0) {
            w = GetSpriteXSize(SP(ship)->shieldSprite);
            h = GetSpriteYSize(SP(ship)->shieldSprite);
            SWSetSpriteLocation(SP(ship)->shieldSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }

        if (ship->cloakDir != 0) {
            if ((gShipTypes[ship->type].flags2 & kShipFastCloak) != 0)
                rate = 1.5F;
            else
                rate = 0.75F;
            ship->cloak += ship->cloakDir * rate * gTimeScale;
            if (ship->cloak <= 0.0F && ship->cloakDir < 0) {
                ship->cloak = 0;
                ship->cloakDir = 0;
            }
            if (ship->cloak >= 32.0F && ship->cloakDir > 0) {
                ship->cloak = 32.0F;
                ship->cloakDir = 0;
            }
        } else if (ship->cloak < 32.0F && ship->cloak > 0.0F) {
            ship->cloak = ship->cloak - 1.0F;
        }

        if (ship->cloak > 0.0F) {
            if (IsDying(ship) != 0)
                ship->cloakDir = -2;
            if (ship->cloak >= 32.0F) {
                if (ship->index == 0)
                    SP(ship)->sprite->translucency = 0x1e;
                else if (shipData[0].seesCloaked == 1 || ship->parentShip == 0)
                    SP(ship)->sprite->translucency = 0x1e;
                else
                    SP(ship)->sprite->translucency = 0x40;
                if (red >= 0x10)
                    SP(ship)->sprite->tintRed = 2;
                else
                    SP(ship)->sprite->tintRed = 0;
                if (green >= 0x10)
                    SP(ship)->sprite->tintGreen = 2;
                else
                    SP(ship)->sprite->tintGreen = 0;
                if (blue >= 0x10)
                    SP(ship)->sprite->tintBlue = 2;
                else
                    SP(ship)->sprite->tintBlue = 0;
                if (hasAlt != 0)
                    SP(ship)->altSprite->translucency = SP(ship)->sprite->translucency;
                if (hasGlow != 0)
                    SWSetSpriteVisible(SP(ship)->glowSprite, 0);
                if (hasLight != 0)
                    SWSetSpriteVisible(SP(ship)->lightSprite, 0);
                if (hasShield != 0)
                    SWSetSpriteVisible(SP(ship)->shieldSprite, 0);
                w = h = 0;
            } else if (SP(ship)->sprite != NULL) {
                jitter = (ship->cloak / 10.0F);
                h = jitter * 2 + 1;
                w = jitter - Rand(h);
                h = jitter - Rand(h);
                OffsetRect(&SP(ship)->sprite->destFrameRect, w, h);
                if (gSpriteWorld->pixelDepth == 0x10)
                    SP(ship)->sprite->translucency = kFullHilite;
                else
                    SP(ship)->sprite->translucency = 0x10;
                SP(ship)->sprite->tintRed = (red - ship->cloak);
                SP(ship)->sprite->tintGreen = (green - ship->cloak);
                SP(ship)->sprite->tintBlue = (blue - ship->cloak);
                if (SP(ship)->sprite->tintRed < 2) {
                    if (red >= 0x10)
                        SP(ship)->sprite->tintRed = 2;
                    else
                        SP(ship)->sprite->tintRed = 0;
                }
                if (SP(ship)->sprite->tintGreen < 2) {
                    if (green >= 0x10)
                        SP(ship)->sprite->tintGreen = 2;
                    else
                        SP(ship)->sprite->tintGreen = 0;
                }
                if (SP(ship)->sprite->tintBlue < 2) {
                    if (blue >= 0x10)
                        SP(ship)->sprite->tintBlue = 2;
                    else
                        SP(ship)->sprite->tintBlue = 0;
                }
                if (hasAlt != 0 && SP(ship)->altSprite != NULL) {
                    OffsetRect(&SP(ship)->altSprite->destFrameRect, w, h);
                    SP(ship)->altSprite->translucency = SP(ship)->sprite->translucency;
                    SP(ship)->altSprite->tintRed = SP(ship)->sprite->tintRed;
                    SP(ship)->altSprite->tintGreen = SP(ship)->sprite->tintGreen;
                    SP(ship)->altSprite->tintBlue = SP(ship)->sprite->tintBlue;
                }
                if (hasGlow != 0 && SP(ship)->glowSprite != NULL) {
                    OffsetRect(&SP(ship)->glowSprite->destFrameRect, w, h);
                    SP(ship)->glowSprite->translucency = kFullHilite;
                    SP(ship)->glowSprite->tintRed = MIN(SP(ship)->glowSprite->tintRed, 32.0F - ship->cloak);
                    SP(ship)->glowSprite->tintGreen = MIN(SP(ship)->glowSprite->tintGreen, 32.0F - ship->cloak);
                    SP(ship)->glowSprite->tintBlue = MIN(SP(ship)->glowSprite->tintBlue, 32.0F - ship->cloak);
                }
                if (hasLight != 0 && SP(ship)->lightSprite != NULL) {
                    OffsetRect(&SP(ship)->lightSprite->destFrameRect, w, h);
                    SP(ship)->lightSprite->translucency = kFullHilite;
                    SP(ship)->lightSprite->tintRed = MIN(SP(ship)->lightSprite->tintRed, 32.0F - ship->cloak);
                    SP(ship)->lightSprite->tintGreen = MIN(SP(ship)->lightSprite->tintGreen, 32.0F - ship->cloak);
                    SP(ship)->lightSprite->tintBlue = MIN(SP(ship)->lightSprite->tintBlue, 32.0F - ship->cloak);
                }
            }
            if (hasWeap != 0 && SP(ship)->weapSprite != NULL) {
                if (gSpriteWorld->pixelDepth == 8) {
                    SWSetSpriteVisible(SP(ship)->weapSprite, 0);
                } else {
                    OffsetRect(&SP(ship)->weapSprite->destFrameRect, w, h);
                    SP(ship)->weapSprite->translucency = kFullHilite;
                    SP(ship)->weapSprite->tintRed = MIN(SP(ship)->weapSprite->tintRed, 40.0F - ship->cloak);
                    SP(ship)->weapSprite->tintGreen = MIN(SP(ship)->weapSprite->tintGreen, 40.0F - ship->cloak);
                    SP(ship)->weapSprite->tintBlue = MIN(SP(ship)->weapSprite->tintBlue, 40.0F - ship->cloak);
                }
            }
        }

        if (hideAll != 0) {
            if (SP(ship)->sprite != NULL && SP(ship)->sprite->isVisible != 0)
                SWSetSpriteVisible(SP(ship)->sprite, 0);
            if (SP(ship)->altSprite != NULL && SP(ship)->altSprite->isVisible != 0)
                SWSetSpriteVisible(SP(ship)->altSprite, 0);
            if (SP(ship)->lightSprite != NULL && SP(ship)->lightSprite->isVisible != 0)
                SWSetSpriteVisible(SP(ship)->lightSprite, 0);
            if (SP(ship)->weapSprite != NULL && SP(ship)->weapSprite->isVisible != 0)
                SWSetSpriteVisible(SP(ship)->weapSprite, 0);
            if (SP(ship)->glowSprite != NULL && SP(ship)->glowSprite->isVisible != 0)
                SWSetSpriteVisible(SP(ship)->glowSprite, 0);
            if (SP(ship)->shieldSprite != NULL && SP(ship)->shieldSprite->isVisible != 0)
                SWSetSpriteVisible(SP(ship)->shieldSprite, 0);
        }

        if (IsDying(ship) != 0 && ship->type != kShipTypeEscapePod) {
            if (ship->deathTimer <= 0.0F) {
                ship->deathTimer = gShipTypes[ship->type].deathDelay;
                if (ship->index == 0)
                    ship->deathTimer = ship->deathTimer * 3.0F;
            } else if (ship->deathTimer <= 2.0F) {
                if (ship->index == 0) {
                    if (ST_IsSoundPlaying(klaxxonSnd) != 0)
                        ST_HaltSound(klaxxonSnd);
                    if (ST_IsSoundPlaying(warpUpSnd[0]) != 0)
                        ST_HaltSound(warpUpSnd[0]);
                    if (ST_IsSoundPlaying(warpUpSnd[1]) != 0)
                        ST_HaltSound(warpUpSnd[1]);
                    if (ST_IsSoundPlaying(warpOutSnd) != 0)
                        ST_HaltSound(warpOutSnd);
                }
                if ((gShipTypes[ship->type].flags & kShipNoDeathBlast) == 0) {
                    if (gShipTypes[ship->type].mass < 100) {
                        blastRadius = 0;
                        impact = 1;
                        damage = 0;
                    } else {
                        blastRadius = (gShipTypes[ship->type].mass * 0.075 + 50.0);
                        impact = 750;
                        damage = (gShipTypes[ship->type].mass * 0.0375 + 25.0);
                    }
                    if (blastRadius > 0) {
                        for (i = 0; i < kMaxShips; i++) {
                            if (i != ship->index && shipData[i].active != 0) {
                                dx = fabs(shipData[i].pos.x - ship->pos.x);
                                dy = fabs(shipData[i].pos.y - ship->pos.y);
                                if (blastRadius >= dx && blastRadius >= dy
                                        && ship->persType != kPersCaptainHector)
                                    DamageShip(&shipData[i], &ship->pos,
                                               impact, damage, damage, ship->index,
                                               0, 0, 1, 1, 0, 0);
                            }
                        }
                    }
                }
                if (ship->specialMission != -1 && ship->index != 0) {
                    if (currentMissionObjectives[ship->specialMission].active != 0
                            && currentMissions[ship->specialMission].specialShipsKilled == 0) {
                        if (currentMissions[ship->specialMission].shipGoal == 1
                                || (currentMissions[ship->specialMission].shipGoal == 2 && ship->boarded == 0)
                                || (currentMissions[ship->specialMission].shipGoal == 5 && ship->boarded == 0)
                                || currentMissions[ship->specialMission].shipGoal == 3) {
                            if (currentMissionObjectives[ship->specialMission].failed == 0
                                    && (currentMissionObjectives[ship->specialMission].flags
                                        & kMisnQuietFailure) == 0) {
                                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                                GetIndString(msg, kGameStrList, 0x11c);
                                DisplayComm(msg, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                                QuickMissionFailure(ship->specialMission);
                            }
                        }
                    }
                    currentMissions[ship->specialMission].specialShipsKilled += 1;
                    if (currentMissions[ship->specialMission].shipCount > 0)
                        currentMissions[ship->specialMission].shipCount -= 1;
                }
                if (ship->parentShip == 0 && ship->aiType == 6 && ship->specialMission == -1
                        && gShipTypes[ship->type].inherentAI <= 2) {
                    DestroyPartialFleetCargo(ship->index);
                    ShipStatsToSystemInfo();
                    statusRedraw[2] = 1;
                }
                if (ship->persType != -1) {
                    if (ship->persType == kPersBountyHunter) {
                        if (Rand(8) == 0)
                            personData[ship->persType].loaded = 0;
                    } else if ((personData[ship->persType].flags & kPersRespawns) == 0) {
                        personData[ship->persType].loaded = 0;
                    }
                }
                CreateExplosion(ship->pos,
                                gShipTypes[ship->type].explode2, blastRadius, 1);
                SWSetSpriteVisible(SP(ship)->sprite, 0);
                ship->active = 0;
                ship->parentShip = -1;
            } else {
                if (ship->deathTimer < 20.0F)
                    i = 1;
                else if (ship->deathTimer < 40.0F)
                    i = 2;
                else if (ship->deathTimer < 60.0F)
                    i = 4;
                else
                    i = 8;
                if (Rand(i) == 0) {
                    m = (GetSpriteXSize(SP(ship)->sprite) * 0.25);
                    if (m <= 0)
                        m = 1;
                    explod.x = Rand(m * 2) + ship->pos.x - m;
                    explod.y = Rand(m * 2) + ship->pos.y - m;
                    if (i <= 2 && gShipTypes[ship->type].deathDelay >= 0x3c)
                        Rand(2);
                    if (Rand(4) == 0)
                        CreateExplosion(explod,
                                        gShipTypes[ship->type].explode1, 0, 1);
                    else
                        CreateExplosion(explod,
                                        gShipTypes[ship->type].explode1, 0, 0);
                }
            }
        }
    } else if (gameFrozen == 0) {
        ship->active = 0;
        ship->parentShip = -1;
    }

    if (ship->active != 0 && ship->curSystem == shipData[0].curSystem) {
        for (i = 0; i <= 1; i++) {
            if ((&ship->seesCloaked)[i] < 0) {
                if (ShipCanSeeCloakedShips(ship, i) != 0)
                    (&ship->seesCloaked)[i] = 1;
                else
                    (&ship->seesCloaked)[i] = 0;
            }
        }
        if (ship->decloaksWhenHit < 0) {
            if (ShipDecloaksWhenHit(ship) != 0)
                ship->decloaksWhenHit = 1;
            else
                ship->decloaksWhenHit = 0;
        }
    } else {
        ship->seesCloaked = ship->seesCloakedRadar = -1;
        ship->decloaksWhenHit = -1;
    }
}

#define kSpinWrap               36.0f
#define kFadeStart              32.0f
#define kMaxMurk                0x1f


/* HandleBoxes. STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x0042c1b0
void HandleBoxes(void)
{
    short i;
    short murk;

    miningBoxesActive = 0;
    for (i = 0; i < kBoxSlots; i++) {
        if (boxData[i].timer < 0.0f || killBoxes != 0) {
            boxData[i].timer = -2.0f;
            if (boxData[i].sprite != 0)
                SWSetSpriteVisible(boxData[i].sprite, 0);
            continue;
        }

        boxData[i].angle += boxData[i].spin * gTimeScale;
        while (boxData[i].angle >= kSpinWrap)
            boxData[i].angle = boxData[i].angle - kSpinWrap;
        while (boxData[i].angle < 0.0f)
            boxData[i].angle = boxData[i].angle + kSpinWrap;

        boxData[i].timer -= gTimeScale;
        boxData[i].pos.x += boxData[i].vel.x * gTimeScale;
        boxData[i].pos.y += boxData[i].vel.y * gTimeScale;
        SWCloneFramesFromMaster(boxData[i].sprite, masterBoxSprite[boxData[i].pict]);

        if (boxData[i].sprite != 0) {
            boxData[i].sprite->tintRed =
                boxData[i].sprite->tintGreen =
                    boxData[i].sprite->tintBlue = 0;
            if (boxData[i].timer >= kFadeStart) {
                boxData[i].sprite->translucency = 0;
            }
            else {
                murk = (kFadeStart - boxData[i].timer);
                if (murk > kMaxMurk)
                    murk = kMaxMurk;
                boxData[i].sprite->translucency = murk;
            }
            HandleSpriteMurkiness(boxData[i].pos.x,
                                  boxData[i].pos.y,
                                  boxData[i].sprite);
            SWSetSpriteVisible(boxData[i].sprite, 1);
            SWSetCurrentFrameIndex(boxData[i].sprite,
                                   boxData[i].angle);
            SWSetSpriteLocation(boxData[i].sprite,
                (short)(boxData[i].pos.x - shipData[0].pos.x)
                    + screenCenter.h - GetSpriteXSize(boxData[i].sprite),
                (short)(boxData[i].pos.y - shipData[0].pos.y)
                    + screenCenter.v - GetSpriteYSize(boxData[i].sprite));
            boxData[i].sprite->userData = i;
        }
        if (boxData[i].mining != 0)
            miningBoxesActive = 1;
    }
}

#define kDebrisSlots            0x40
#define kFullBlend              0x20
#define kFadeSpan               16.0f
#define kFadeBase               32.0f
#define kAgeRate                0.25

/* HandleSmoke (mac _HandleSmoke). STATE: MATCH 100.0%, exe 380 / obj 380, relocregcheck 0 of
   56; the fade level and the half width share one `temp'. OPEN: none */
// FUNCTION: EVNOVA 0x0042c660
void HandleSmoke(void)
{
    short temp;
    short frame;
    short i;
    short halfY;

    for (i = 0; i < kDebrisSlots; i++) {
        if (gDebrisSlots[i].timer >= 0.0f && killSmoke == 0
                && gDebrisSlots[i].sprite != 0) {
            SWSetSpriteVisible(gDebrisSlots[i].sprite, 1);
            temp = (kFadeBase - gDebrisSlots[i].timer
                          / gDebrisSlots[i].sprite->numFrames
                          * kFadeSpan);
            if (temp < 0)
                temp = 0;
            if (temp > kFullBlend)
                temp = kFullBlend;
            gDebrisSlots[i].sprite->translucency = temp;
            HandleSpriteMurkiness(gDebrisSlots[i].pos.x,
                                  gDebrisSlots[i].pos.y,
                                  gDebrisSlots[i].sprite);
            gDebrisSlots[i].sprite->tintRed = kFullBlend;
            gDebrisSlots[i].sprite->tintGreen = kFullBlend;
            gDebrisSlots[i].sprite->tintBlue = kFullBlend;
            gDebrisSlots[i].sprite->translucency = kFullBlend;

            if (gameFrozen == 0)
                gDebrisSlots[i].timer += gTimeScale * kAgeRate;

            if (gDebrisSlots[i].type == 1 || gDebrisSlots[i].type == 3) {
                if (gDebrisSlots[i].type == 3)
                    frame = (short)gDebrisSlots[i].timer / 2;
                else
                    frame = gDebrisSlots[i].timer;
                if (frame <= 2) {
                    SWSetCurrentFrameIndex(gDebrisSlots[i].sprite, frame);
                }
                else if (frame > 5) {
                    SWSetSpriteVisible(gDebrisSlots[i].sprite, 0);
                    gDebrisSlots[i].timer = -1.0f;
                }
                else {
                    SWSetCurrentFrameIndex(gDebrisSlots[i].sprite, 5 - frame);
                }
            }
            else {
                if (gDebrisSlots[i].type == 2)
                    frame = (short)gDebrisSlots[i].timer / 2;
                else
                    frame = gDebrisSlots[i].timer;
                if (frame >= 8) {
                    SWSetSpriteVisible(gDebrisSlots[i].sprite, 0);
                    gDebrisSlots[i].timer = -1.0f;
                }
                else {
                    SWSetCurrentFrameIndex(gDebrisSlots[i].sprite, frame);
                }
            }

            temp = GetSpriteXSize(gDebrisSlots[i].sprite) / 2;
            halfY = GetSpriteYSize(gDebrisSlots[i].sprite) / 2;
            SWSetSpriteLocation(gDebrisSlots[i].sprite,
                (short)(gDebrisSlots[i].pos.x - shipData[0].pos.x)
                    + screenCenter.h - temp,
                (short)(gDebrisSlots[i].pos.y - shipData[0].pos.y)
                    + screenCenter.v - halfY);
        }
        else if (gDebrisSlots[i].sprite != 0) {
            SWSetSpriteVisible(gDebrisSlots[i].sprite, 0);
            gDebrisSlots[i].timer = -1.0f;
        }
    }
    if (killSmoke != 0) {
        ClearParticles(1);
        UpdateParticlesInWindow(gSpriteWorld);
        ClearParticles(0);
    }
}

/* HandleDoubleTimeSprite. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0042cbb0
void HandleDoubleTimeSprite(void)
{
    SWSetSpriteVisible(doubleTimeSprite, doubleTime);
    SWSetSpriteLocation(doubleTimeSprite, 0, 0);
    doubleTimeSprite->translucency = 0x20;
    doubleTimeSprite->tintRed = doubleTimeSprite->tintGreen = doubleTimeSprite->tintBlue = 0x20;
    doubleTimeSprite->tintMode = 0;
}

/* HandleAlertSprite. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0042cc30
void HandleAlertSprite(void)
{
    if (alertSpritePending > 0) {
        SWSetSpriteVisible(alertSprite, 1);
        alertSprite->translucency = 0x20;
        alertSprite->tintRed = alertSprite->tintGreen = alertSprite->tintBlue = 0x20;
        alertSprite->tintMode = 0;
        if (alertSpritePending % 2 == 0)
            SWSetCurrentFrameIndex(alertSprite, 0);
        else
            SWSetCurrentFrameIndex(alertSprite, 1);
        if (frameCounter % 5 == 0)
            alertSpritePending -= 1;
    } else {
        SWSetSpriteVisible(alertSprite, 0);
    }
}

#define kMasterSprites      0x100

#define kSpobFlagNoAnim     0x80
#define kSpobFlagHoldOpen   0x1000
#define kSpobFlagOneShot    0x1
#define kSpobFlagRandFrame  0x2

#define kApproachSpan       1.1      /* pooled double */

/* HandleStellarSprites (mac _HandleStellarSprites). STATE: MATCH 100.0%, exe 654 / obj 654,
   byte-identical yes (2942 bytes), relocregcheck 0 of 128. OPEN: none */
// FUNCTION: EVNOVA 0x0042cd10
void HandleStellarSprites(void)
{
    double lim;
    short i;
    short j;
    short k;
    short type;
    short open;
    short step;
    short reach;
    short w;
    short h;
    char animate;
    char approach;

    for (i = 0; i < kStellarSprites; i++) {
        if (stellarSprite[i] != NULL && stellarSprite[i]->isVisible != 0
            && stellarSprite[i]->userData >= 0
            && stellarSprite[i]->userData < kMaxStellars) {
            SWSetSpriteVisible(stellarSprite[i], 1);
            k = stellarSprite[i]->userData;
            if (StellarIsDestroyed(&stellarData[k]) != 0) {
                if ((stellarData[k].flags2 & kSpobFlagNoAnim) != 0)
                    animate = 1;
                else
                    animate = 0;
                type = stellarData[k].deadType;
            } else {
                if ((stellarData[k].flags2 & kSpobFlagNoAnim) != 0)
                    animate = 0;
                else
                    animate = 1;
                type = stellarData[k].type;
            }
            if (!masterStellarSprite[type])
                LoadMasterStellarSprite(type);
            SWCloneFramesFromMaster(stellarSprite[i], masterStellarSprite[type]);
            if (animate != 0 && stellarData[k].numFrames > 1) {
                stellarData[k].frameTimer += gTimeScale;
                if ((stellarData[k].flags2 & kSpobFlagHoldOpen) != 0) {
                    approach = 0;
                    if (stellarData[k].customPict >= 1
                        && stellarData[k].customPict < stellarData[k].numFrames - 1)
                        open = stellarData[k].customPict;
                    else
                        open = stellarData[k].numFrames / 2;
                    if (dockTarget == k && dockPermission > 0) {
                        approach = 1;
                    } else {
                        for (j = 1; j < kMaxShips; j++) {
                            if (shipData[j].active != 0
                                && IsDisabled(&shipData[j]) == 0) {
                                if (AIShipIsEmergingFromHypergate(&shipData[j]) != 0
                                    && k == shipData[j].targetStellar) {
                                    approach = 1;
                                    break;
                                }
                                if (AIShipIsShipGoingToStellar(&shipData[j], k) != 0) {
                                    reach = GetSpriteXSize(stellarSprite[i]) * 2;
                                    if (stellarData[k].curFrame >= open)
                                        reach *= kApproachSpan;
                                    lim = fabs(shipData[j].pos.x
                                               - stellarData[k].pos.h);
                                    if (reach > lim) {
                                        lim = fabs(shipData[j].pos.y
                                                   - stellarData[k].pos.v);
                                        if (reach > lim) {
                                            approach = 1;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (approach != 0) {
                        if (stellarData[k].curFrame == open
                            && stellarData[k].frame0Bias > 1)
                            step = stellarData[k].animDelay * stellarData[k].frame0Bias;
                        else
                            step = stellarData[k].animDelay;
                        if (stellarData[k].frameTimer >= step) {
                            stellarData[k].frameTimer = 0.0F;
                            if (stellarData[k].curFrame < open) {
                                stellarData[k].curFrame += 1;
                                stellarData[k].nextFrame = stellarData[k].curFrame;
                            } else if ((stellarData[k].flags2 & kSpobFlagOneShot) != 0) {
                                if (open != stellarData[k].curFrame) {
                                    stellarData[k].curFrame = open;
                                } else {
                                stellarData[k].curFrame = stellarData[k].nextFrame;
                                if ((stellarData[k].flags2 & kSpobFlagRandFrame) == 0) {
                                    stellarData[k].nextFrame =
                                        ((stellarData[k].nextFrame + 1)
                                                % stellarData[k].numFrames);
                                    if (stellarData[k].nextFrame < open)
                                        stellarData[k].nextFrame = open + 1;
                                } else {
                                    do {
                                        do {
                                            stellarData[k].nextFrame =
                                                Rand(stellarData[k].numFrames - open) + open;
                                        } while (open == stellarData[k].nextFrame);
                                    } while (stellarData[k].nextFrame
                                             == stellarData[k].curFrame);
                                }
                                }
                            } else {
                                stellarData[k].curFrame = stellarData[k].nextFrame;
                                if ((stellarData[k].flags2 & kSpobFlagRandFrame) == 0) {
                                    stellarData[k].nextFrame =
                                        ((stellarData[k].nextFrame + 1)
                                                % stellarData[k].numFrames);
                                    if (stellarData[k].nextFrame < open)
                                        stellarData[k].nextFrame = open;
                                } else {
                                    do {
                                        stellarData[k].nextFrame =
                                            Rand(stellarData[k].numFrames - open) + open;
                                    } while (stellarData[k].nextFrame
                                             == stellarData[k].curFrame);
                                }
                            }
                        }
                    } else if (stellarData[k].frameTimer
                               >= stellarData[k].animDelay) {
                        stellarData[k].frameTimer = 0.0F;
                        if (stellarData[k].curFrame < open) {
                            if (stellarData[k].curFrame > 0)
                                stellarData[k].curFrame -= 1;
                        } else if ((stellarData[k].flags2 & kSpobFlagRandFrame) == 0) {
                            if (stellarData[k].curFrame < stellarData[k].numFrames - 1)
                                stellarData[k].curFrame += 1;
                            else
                                stellarData[k].curFrame = open - 1;
                        } else {
                            stellarData[k].curFrame = open - 1;
                        }
                    }
                    SWSetCurrentFrameIndex(stellarSprite[i],
                                           stellarData[k].curFrame);
                } else {
                    if (stellarData[k].curFrame == 0
                        && stellarData[k].frame0Bias > 1)
                        step = stellarData[k].animDelay * stellarData[k].frame0Bias;
                    else
                        step = stellarData[k].animDelay;
                    if (stellarData[k].frameTimer >= step) {
                        stellarData[k].frameTimer = 0.0F;
                        if ((stellarData[k].flags2 & kSpobFlagOneShot) != 0) {
                            if (stellarData[k].curFrame == 0) {
                            stellarData[k].curFrame = stellarData[k].nextFrame;
                            if ((stellarData[k].flags2 & kSpobFlagRandFrame) != 0) {
                                do {
                                    do {
                                        stellarData[k].nextFrame =
                                            Rand(stellarData[k].numFrames);
                                    } while (stellarData[k].nextFrame == 0);
                                } while (stellarData[k].nextFrame
                                         == stellarData[k].curFrame);
                            } else {
                                stellarData[k].nextFrame =
                                    ((stellarData[k].nextFrame + 1)
                                            % stellarData[k].numFrames);
                                if (stellarData[k].nextFrame == 0)
                                    stellarData[k].nextFrame += 1;
                            }
                            } else {
                            stellarData[k].curFrame = 0;
                            }
                        } else {
                            stellarData[k].curFrame = stellarData[k].nextFrame;
                            if ((stellarData[k].flags2 & kSpobFlagRandFrame) == 0) {
                                stellarData[k].nextFrame =
                                    ((stellarData[k].nextFrame + 1)
                                            % stellarData[k].numFrames);
                            } else {
                                do {
                                    stellarData[k].nextFrame =
                                        Rand(stellarData[k].numFrames);
                                } while (stellarData[k].nextFrame
                                         == stellarData[k].curFrame);
                            }
                        }
                        SWSetCurrentFrameIndex(stellarSprite[i],
                                               stellarData[k].curFrame);
                    }
                }
            } else {
                SWSetCurrentFrameIndex(stellarSprite[i], 0);
            }
            w = GetSpriteXSize(stellarSprite[i]) / 2;
            h = GetSpriteYSize(stellarSprite[i]) / 2;
            stellarSprite[i]->translucency = 0;
            stellarSprite[i]->tintRed = 0;
            stellarSprite[i]->tintGreen = 0;
            stellarSprite[i]->tintBlue = 0;
            HandleSpriteMurkiness(stellarData[k].pos.h, stellarData[k].pos.v,
                                  stellarSprite[i]);
            SWSetSpriteLocation(stellarSprite[i],
                (short)(stellarData[k].pos.h - shipData[0].pos.x)
                    + screenCenter.h - w,
                (short)(stellarData[k].pos.v - shipData[0].pos.y)
                    + screenCenter.v - h);
        }
    }
}

#define kShipSlots              64
#define kSystemNavSlots         16
#define kWeapGlowSlots          4
#define kWeapFlagNoSpin         0x4     /* WeapRecord flags bit 2 (name TBC) */
#define kShotSpinRange          0x24

/* one live shot, stride 0x48 in shotData; only the fields this TU writes */

/* HandleStellarWeapons (mac _HandleStellarWeapons). STATE: MATCH 100.0%, exe 540 / obj 540,
   relocregcheck 0 of 112. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0042d890
void HandleStellarWeapons(void)
{
    FloatPoint pos;
    float range;
    float best;
    float dist;
    unsigned char disableOnly;
    short i;
    short j;
    short k;
    short stel;
    short target;
    short reload;

    if (gameFrozen != 0)
        return;

    for (i = 0; i < kSystemNavSlots; i++) {
        stel = gSystems[shipData->curSystem].navDefault[i];
        if (stel == -1)
            continue;
        if (stellarData[stel].loaded == 0)
            continue;
        if (stellarData[stel].weapon == -1)
            continue;
        if (StellarIsDestroyed(&stellarData[stel]))
            continue;
        if (stellarData[stel].reloadTimer <= 0.0) {
            target = -1;
            range = gWeapons[stellarData[stel].weapon].totalRange;
            range = range * range;
            if ((gWeapons[stellarData[stel].weapon].flags2 & kWeapNotDestroying) != 0
                    || gWeapons[stellarData[stel].weapon].massDmg == 0)
                disableOnly = 1;
            else
                disableOnly = 0;

            pos.x = stellarData[stel].pos.h;
            pos.y = stellarData[stel].pos.v;
            for (k = 0; k < kShipSlots; k++) {
                if (shipData[k].active == 0)
                    continue;
                if (ShipIsCloaked(&shipData[k]))
                    continue;
                if (!IsThreatToStellar(&shipData[k], &stellarData[stel]))
                    continue;
                dist = XYFloatSquaredDist(&pos, &shipData[k].pos);
                if (!(dist <= range))
                    continue;
                if (disableOnly != 0 && IsDisabled(&shipData[k]))
                    continue;
                if (dist < best || target == -1) {
                    target = k;
                    best = dist;
                }
            }
            if (target == -1)
                continue;

            j = -1;
            for (k = 0; k < kMaxShots; k++) {
                if (shotData[k].life < -1.0f) {
                    j = k;
                    break;
                }
            }
            if (j == -1)
                continue;

            shotData[j].pos.x = stellarData[stel].pos.h;
            shotData[j].pos.y = stellarData[stel].pos.v;
            shotData[j].vel.x = 0.0f;
            shotData[j].vel.y = 0.0f;
            shotData[j].weapType = stellarData[stel].weapon;
            shotData[j].owner = -1;
            shotData[j].target = target;
            shotData[j].disableOnly = WeaponDisableOnly(shotData[j].weapType);
            shotData[j].life = gWeapons[shotData[j].weapType].lifetime;
            shotData[j].subDepth = 0;
            shotData[j].murk = 0;
            if ((gWeapons[shotData[j].weapType].flags & kWeapFlagNoSpin) != 0)
                shotData[j].frame = 0;
            else
                shotData[j].frame = Rand(kShotSpinRange);
            shotData[j].frameTimer = 0;
            shotData[j].trailCount = 0;
            if (gWeapons[shotData[j].weapType].decay > 0)
                shotData[j].trailTimer = 0.0f;
            else
                shotData[j].trailTimer = -1.0f;
            shotData[j].curSystem = shipData->curSystem;
            shotData[j].guidanceState = 0;
            if (gWeapons[shotData[j].weapType].durability > 0)
                shotData[j].energy = gWeapons[shotData[j].weapType].durability;
            else
                shotData[j].energy = 0;
            shotData[j].angle = CalcSpobLeadAngle(&stellarData[stel], &shipData[target]);
            if (gWeapons[shotData[j].weapType].inaccuracy > 0) {
                shotData[j].angle = shotData[j].angle
                    + (Rand(gWeapons[shotData[j].weapType].inaccuracy * 2)
                       - gWeapons[shotData[j].weapType].inaccuracy);
                if (shotData[j].angle < 0.0f)
                    shotData[j].angle += kFullCircleF;
                if (shotData[j].angle >= kFullCircleF)
                    shotData[j].angle -= kFullCircleF;
            }
            Accel(shotData[j].angle,
                  gWeapons[shotData[j].weapType].speed100,
                  &shotData[j].vel);
            for (k = 0; k < kWeapGlowSlots; k++) {
                if (gWeapons[shotData[j].weapType].jamVuln[k] > 0)
                    shotData[j].glow[k] =
                        Rand(gWeapons[shotData[j].weapType].jamVuln[k] + 1);
                else
                    shotData[j].glow[k] = 0;
            }
            PutSpriteInLayer(shotData[j].sprite, shotLayer);
            SWSetSpriteCollideProc(shotData[j].sprite, 0);
            SWCloneFramesFromMaster(shotData[j].sprite,
                         masterShotSprite[gWeapons[shotData[j].weapType].graphic]);

            reload = gWeapons[stellarData[stel].weapon].reload;
            if (gWeapons[stellarData[stel].weapon].burstCount > 0) {
                stellarData[stel].burstShots++;
                if (stellarData[stel].burstShots
                        >= WeaponBurstInterval(stellarData[stel].weapon, 1)) {
                    stellarData[stel].burstShots = 0;
                    reload = gWeapons[stellarData[stel].weapon].burstReload;
                }
            }
            stellarData[stel].reloadTimer = reload;
            PlaySoundDistance(-1, shotSnd[gWeapons[shotData[j].weapType].sound], 5,
                            &shotData[j].pos, &shipData->pos);
        } else {
            stellarData[stel].reloadTimer -= gTimeScale;
        }
    }
}

#define kMurkBase           32.0F
#define kMurkSpan           16.0F
#define kSlotFree           (-1.0)

#define kStarSlots          0x14
#define kTranslucencyOpaque 0x20
#define kAlphaMax           0x1d
#define kAlphaMin           2
#define kInterferenceScale  0.9      /* a pooled double */


/* HandleExplods. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0042e160
void HandleExplods(void)
{
    short i;
    short w;
    short h;
    short murk;

    for (i = 0; i < kMaxExplods; i++) {
        if (explodData[i].timer >= 0.0F && killExplods == 0) {
            if (explodData[i].delay > 0.0F) {
                SWSetSpriteVisible(explodData[i].sprite, 0);
                if (gameFrozen == 0)
                    explodData[i].delay -=
                        explodType[explodData[i].type].frameRate * gTimeScale;
            } else {
                SWSetSpriteVisible(explodData[i].sprite, 1);
                SWSetCurrentFrameIndex(explodData[i].sprite,
                                       explodData[i].timer);
                w = (GetSpriteXSize(explodData[i].sprite) / 2);
                h = (GetSpriteYSize(explodData[i].sprite) / 2);
                SWSetSpriteLocation(explodData[i].sprite,
                    (short)(explodData[i].pos.x - shipData[0].pos.x)
                        + screenCenter.h - w,
                    (short)(explodData[i].pos.y - shipData[0].pos.y)
                        + screenCenter.v - h);
                murk = (kMurkBase - explodData[i].timer
                               / explodData[i].sprite->numFrames * kMurkSpan);
                if (murk < 0)
                    murk = 0;
                if (murk > kMurkFull)
                    murk = kMurkFull;
                explodData[i].sprite->translucency = murk;
                HandleSpriteMurkiness(explodData[i].pos.x,
                                      explodData[i].pos.y,
                                      explodData[i].sprite);
                explodData[i].sprite->tintRed = kMurkFull;
                explodData[i].sprite->tintGreen = kMurkFull;
                explodData[i].sprite->tintBlue = kMurkFull;
                explodData[i].sprite->translucency = kMurkFull;
                if (gameFrozen == 0)
                    explodData[i].timer +=
                        explodType[explodData[i].type].frameRate * gTimeScale;
                if ((short)explodData[i].timer
                        >= explodData[i].sprite->numFrames)
                    explodData[i].timer = kSlotFree;
            }
        } else {
            SWSetSpriteVisible(explodData[i].sprite, 0);
            explodData[i].timer = kSlotFree;
        }
    }
}

/* HandleStars (mac _HandleStars). STATE: MATCH 351/351, byte-identical. Matches only with
   HandleExplods compiled above it (92.3% alone): the predecessor's allocator state decides
   the frame. OPEN: none */
// FUNCTION: EVNOVA 0x0042e590
void HandleStars(void)
{
    short i;
    short viewW;
    short viewH;
    short w;
    short h;
    short edgeX;
    short edgeY;
    short alpha;
    short interference;

    viewW = gSpriteWorld->windRect.right - gSpriteWorld->windRect.left;
    viewH = gSpriteWorld->windRect.bottom - gSpriteWorld->windRect.top;
    interference = gSystems[shipData[0].curSystem].murk;
    if (interference == 0) {
        alpha = kTranslucencyOpaque;
    } else {
        alpha = interference * kInterferenceScale;
        if (alpha > kAlphaMax)
            alpha = kAlphaMax;
        if (alpha < kAlphaMin)
            alpha = kAlphaMin;
    }
    for (i = 0; i < kStarSlots; i++) {
        if (starData[i].live != 0) {
            SWSetSpriteVisible(starData[i].sprite, 1);
            SWSetCurrentFrameIndex(starData[i].sprite, starData[i].frame);
            SWSetSpriteDrawProc(starData[i].sprite,
                                BlitPixieRLETranslucentDrawProc);
            if (gSpriteWorld->pixelDepth == kDepth16) {
                starData[i].sprite->translucency = kTranslucencyOpaque;
                starData[i].sprite->tintMode = 0;
                starData[i].sprite->tintRed = alpha;
                starData[i].sprite->tintGreen = alpha;
                starData[i].sprite->tintBlue = alpha;
            } else {
                if (alpha == kTranslucencyOpaque)
                    starData[i].sprite->translucency = 0;
                else
                    starData[i].sprite->translucency = alpha;
                starData[i].sprite->tintMode = 0;
                starData[i].sprite->tintRed = 0;
                starData[i].sprite->tintGreen = 0;
                starData[i].sprite->tintBlue = 0;
            }
            w = GetSpriteXSize(starData[i].sprite);
            h = GetSpriteYSize(starData[i].sprite);
            w = GetSpriteXSize(starData[i].sprite);
            h = GetSpriteYSize(starData[i].sprite);
            edgeX = w / 2 + starData[i].sprite->destFrameRect.left;
            edgeY = h / 2 + starData[i].sprite->destFrameRect.top;
            if (edgeX < -w * 2) {
                starData[i].pos.x = starData[i].pos.x + (w * 2 + viewW);
                starData[i].pos.y = Rand(viewH) + shipData[0].pos.y
                                   - viewH / 2;
            } else if (edgeX > w * 2 + viewW) {
                starData[i].pos.x = starData[i].pos.x - (w * 2 + viewW);
                starData[i].pos.y = Rand(viewH) + shipData[0].pos.y
                                   - viewH / 2;
            }
            if (edgeY < -h * 2) {
                starData[i].pos.y = starData[i].pos.y + (h * 2 + viewH);
                starData[i].pos.x = Rand(viewW) + shipData[0].pos.x
                                   - viewW / 2;
            } else if (edgeY > h * 2 + viewH) {
                starData[i].pos.y = starData[i].pos.y - (h * 2 + viewH);
                starData[i].pos.x = Rand(viewW) + shipData[0].pos.x
                                   - viewW / 2;
            }
            SWSetSpriteLocation(starData[i].sprite,
                (short)((short)(starData[i].pos.x - shipData[0].pos.x)
                        + screenCenter.h) - w,
                (short)((short)(starData[i].pos.y - shipData[0].pos.y)
                        + screenCenter.v) - h);
        } else {
            SWSetSpriteVisible(starData[i].sprite, 0);
        }
    }
}

#define kNavTargetCorners   4
#define kNoSpriteHalfSize   0x40

#define kFrameUnvisited     0
#define kFrameVisited       4

#define kRouteChosen        2        /* shipData[0].navTargetKind */

/* only the two fields this function reads */


/* HandleNavTargetSprites (mac _HandleNavTargetSprites). STATE: MATCH 225/225, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0042eac0
void HandleNavTargetSprites(void)
{
    short i;
    short frame;
    short size;
    short half;
    short x;
    short y;
    long f;

    if (shipData[0].targetStellar != -1 && shipData[0].navTargetKind == kRouteChosen) {
        if (navTargetExpand > 0.0F)
            navTargetExpand = navTargetExpand - gTimeScale * kExpandRate;
        if (navTargetExpand < 0.0F)
            navTargetExpand = 0.0F;
        for (i = 0; i < kNavTargetCorners; i++)
            SWSetSpriteVisible(navTargetSprite[i], 1);
        if (stellarData[shipData[0].targetStellar].dominated != 0)
            frame = kFrameVisited;
        else
            frame = kFrameUnvisited;
        for (i = 0, f = frame; i < kNavTargetCorners; i++) {
            SWSetCurrentFrameIndex(navTargetSprite[i], f);
            f = f + 1;
        }
        x = stellarData[shipData[0].targetStellar].pos.h
            - (short)shipData[0].pos.x;
        x = x + screenCenter.h;
        y = stellarData[shipData[0].targetStellar].pos.v
            - (short)shipData[0].pos.y;
        y = y + screenCenter.v;
        half = kNoSpriteHalfSize;
        for (i = 0; i < kStellarSprites; i++) {
            if (stellarSprite[i]->isVisible != 0
                && shipData[0].targetStellar == stellarSprite[i]->userData) {
                if (GetSpriteXSize(stellarSprite[i])
                        > GetSpriteYSize(stellarSprite[i]))
                    size = GetSpriteXSize(stellarSprite[i]);
                else
                    size = GetSpriteYSize(stellarSprite[i]);
                half = size / 2;
                break;
            }
        }
        half += (short)navTargetExpand;
        SWSetSpriteLocation(navTargetSprite[0], x - half - kBracketInset,
                            y - half - kBracketInset);
        SWSetSpriteLocation(navTargetSprite[1], x + half,
                            y - half - kBracketInset);
        SWSetSpriteLocation(navTargetSprite[2], x + half, y + half);
        SWSetSpriteLocation(navTargetSprite[3], x - half - kBracketInset,
                            y + half);
    } else {
        navTargetExpand = 0.0F;
        for (i = 0; i < kNavTargetCorners; i++)
            SWSetSpriteVisible(navTargetSprite[i], 0);
    }
}

#define kWeapTargetCorners  4

#define kFrameHostile       0
#define kFrameNeutral       4
#define kFrameAlly          8
#define kFrameDisabled      0xc

/* HandleWeapTargetSprites. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0042ede0
void HandleWeapTargetSprites(void)
{
    short i;
    short frame;
    short x;
    short y;
    short half;
    short leader;
    long f;

    if (shipData[0].targetShip != -1) {
        if (weapTargetExpand > 0.0F)
            weapTargetExpand = weapTargetExpand - gTimeScale * kExpandRate;
        if (weapTargetExpand < 0.0F)
            weapTargetExpand = 0.0F;
        for (i = 0; i < kWeapTargetCorners; i++)
            SWSetSpriteVisible(weapTargetSprite[i], 1);
        if (IsDisabled(&shipData[shipData[0].targetShip]) != 0) {
            frame = kFrameDisabled;
        } else {
            if (shipData[shipData[0].targetShip].parentShip == 0
                && shipData[shipData[0].targetShip].defenseStellar == -1) {
                frame = kFrameAlly;
            } else if (IsThreatToPlayer(&shipData[shipData[0].targetShip]) != 0) {
                frame = kFrameHostile;
            } else {
                frame = kFrameNeutral;
            }
            leader = shipData[shipData[0].targetShip].parentShip;
            if (leader > 0 && shipData[leader].parentShip == 0)
                frame = kFrameAlly;
        }
        for (i = 0, f = frame; i < kWeapTargetCorners; i++) {
            SWSetCurrentFrameIndex(weapTargetSprite[i], f);
            f = f + 1;
        }
        x = (short)(shipData[shipData[0].targetShip].pos.x - shipData[0].pos.x)
            + screenCenter.h;
        y = (short)(shipData[shipData[0].targetShip].pos.y - shipData[0].pos.y)
            + screenCenter.v;
        if (GetSpriteXSize((SpriteRec *)shipData[shipData[0].targetShip].sprite)
                > GetSpriteYSize((SpriteRec *)shipData[shipData[0].targetShip].sprite))
            half = GetSpriteXSize((SpriteRec *)shipData[shipData[0].targetShip].sprite);
        else
            half = GetSpriteYSize((SpriteRec *)shipData[shipData[0].targetShip].sprite);
        half = (half / 2) + (short)weapTargetExpand;
        SWSetSpriteLocation(weapTargetSprite[0], x - half - kBracketInset,
                            y - half - kBracketInset);
        SWSetSpriteLocation(weapTargetSprite[1], x + half,
                            y - half - kBracketInset);
        SWSetSpriteLocation(weapTargetSprite[2], x + half, y + half);
        SWSetSpriteLocation(weapTargetSprite[3], x - half - kBracketInset,
                            y + half);
    } else {
        weapTargetExpand = 0.0F;
        for (i = 0; i < kWeapTargetCorners; i++)
            SWSetSpriteVisible(weapTargetSprite[i], 0);
    }
}

#define kJumpTimeoutTicks 500

/* HandleComm. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0042f1b0
void HandleComm(void)
{
    if (commCount >= 0) {
        commCount -= 1;
        if (commCount <= 0) {
            SWSetPortToBackground(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&commRect);
            RGBForeColor(blackColor);
            SWSetPortToWindow(gSpriteWorld);
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     &commRect, &commRect, 0, NULL);
        }
    }
    if (showFloatingMap > 0) {
        if (ReadTick() > gJumpStartTick + kJumpTimeoutTicks)
            showFloatingMap = 0;
    }
}

/* shotData, stride 0x48 */


static __inline float FloatSqrt(float x)
{
    return FUN_004eed20(x);
}

#define kMaxCoronaFrames        0x10

#define kWeap22SeedOwnerC8E8    0x200   /* WeapRecord flags2 bit 9 (name TBC) */
#define kDudeFlagNoBeamTarget   0x100   /* dudeData +0x44 bit 8 (name TBC) */

/* HandleBeams (mac _HandleBeams). STATE: MATCH 100.0% (1667 exact, 335 reloc-equiv);
   exe 2002 / obj 2002; byte-identical yes; relocregcheck 0 of 335. OPEN: none. */
// FUNCTION: EVNOVA 0x0042f270
void HandleBeams(unsigned char advance)
{
    /* FloatPoint spelling: the definition takes this width. */
    extern void SpawnExplod(FloatPoint pos, short owner, short type,
                            short angle);

    short *dstHead;
    short *dstTail;
    short *src;
    FloatPoint muzzle;
    FloatPoint off;
    short i;
    short j;
    FloatPoint drawPt;
    long bestDist;
    char notSelf;
    char corona;
    char isAlly;
    char reachable;
    char eligible;
    short shipType;
    short ownerType;
    float drift;
    short best;
    short angle;
    short shipAngle;
    short size;
    long range;
    long dist;
    short impact;
    short massDmg;
    short energyDmg;
    long len;
    short hitAngle;
    short parent;
    long diff;

    for (i = 0; i < kMaxBeams; i++) {
        if (beamData[i].state > -1 && beamData[i].owner != -1) {
            corona = 0;
            if (beamData[i].state == 0
                && gWeapons[beamData[i].weapType].decay > 0) {
                beamData[i].frame++;
                if ((short)(beamData[i].frame
                    + gWeapons[beamData[i].weapType].coronaFalloff) < kMaxCoronaFrames)
                    corona = 1;
            }
            if (!corona && gameFrozen == 0
                && (beamData[i].state > 0 || advance))
                beamData[i].state--;
            if (killShots)
                beamData[i].state = -1;
            if (advance) {
                dstHead = (short *)&beamData[i].lastHead;
                src = (short *)&beamData[i].head;
                dstHead[0] = src[0];
                dstHead[1] = src[1];
                src = (short *)&beamData[i].tail;
                dstTail = (short *)&beamData[i].lastTail;
                dstTail[0] = src[0];
                dstTail[1] = src[1];
            }
            if (advance) {
                angle = *(long *)(shipData[beamData[i].owner].sprite + 0x68)
                        % gShipTypes[shipData[beamData[i].owner].type].framesPer;
                angle *= 360.0 / gShipTypes[shipData[beamData[i].owner].type].framesPer;
                shipAngle = angle;

                if (beamData[i].target != -1
                    && gWeapons[beamData[i].weapType].guidance != kGuidanceBeam) {
                    off.x = off.y = 0.0f;
                    if (beamData[i].owner != -1
                        && beamData[i].lane >= 0 && beamData[i].lane <= 3
                        && beamData[i].mount >= 0 && beamData[i].mount <= 3) {
                        ownerType = shipData[beamData[i].owner].type;
                        Accel(angle,
                              gShipTypes[ownerType]
                                  .weapPos[beamData[i].mount][beamData[i].lane][1],
                              &off);
                        Accel((angle + kQuarterCircle) % kFullCircle,
                              gShipTypes[ownerType]
                                  .weapPos[beamData[i].mount][beamData[i].lane][0],
                              &off);
                        if (off.y < 0.0) {
                            off.x = off.x
                                * gShipTypes[ownerType].compress[0][0];
                            off.y = off.y
                                * gShipTypes[ownerType].compress[0][1];
                        } else {
                            off.x = off.x
                                * gShipTypes[ownerType].compress[1][0];
                            off.y = off.y
                                * gShipTypes[ownerType].compress[1][1];
                        }
                        off.y -= gShipTypes[ownerType]
                                     .weapPosZ[beamData[i].mount][beamData[i].lane];
                    }
                    off.x += shipData[beamData[i].owner].pos.x;
                    off.y += shipData[beamData[i].owner].pos.y;

                    if (gWeapons[beamData[i].weapType].guidance == kGuidancePDBeam
                        && beamData[i].targetIsShot == 1) {
                        if (shotData[beamData[i].target].life >= 0.0f)
                            angle = CalcInterceptAngle(&off,
                                &shotData[beamData[i].target].pos);
                        else {
                            beamData[i].state = -1;
                            beamData[i].target = -1;
                        }
                    } else {
                        if (shipData[beamData[i].target].active)
                            angle = CalcInterceptAngle(&off,
                                &shipData[beamData[i].target].pos);
                        else
                            beamData[i].state = -1;
                    }
                }

                if (gWeapons[beamData[i].weapType].inaccuracy > 0
                    && gWeapons[beamData[i].weapType].guidance != kGuidancePDBeam)
                    angle += Rand(gWeapons[beamData[i].weapType].inaccuracy * 2)
                             - gWeapons[beamData[i].weapType].inaccuracy;

                muzzle.x = shipData[beamData[i].owner].pos.x;
                muzzle.y = shipData[beamData[i].owner].pos.y;
                if (beamData[i].owner != -1
                    && beamData[i].lane >= 0 && beamData[i].lane <= 3
                    && beamData[i].mount >= 0 && beamData[i].mount <= 3) {
                    off.x = off.y = 0.0f;
                    ownerType = shipData[beamData[i].owner].type;
                    Accel(shipAngle,
                          gShipTypes[ownerType]
                              .weapPos[beamData[i].mount][beamData[i].lane][1],
                          &off);
                    Accel((shipAngle + kQuarterCircle) % kFullCircle,
                          gShipTypes[ownerType]
                              .weapPos[beamData[i].mount][beamData[i].lane][0],
                          &off);
                    if (off.y < 0.0) {
                        off.x = off.x
                            * gShipTypes[ownerType].compress[0][0];
                        off.y = off.y
                            * gShipTypes[ownerType].compress[0][1];
                    } else {
                        off.x = off.x
                            * gShipTypes[ownerType].compress[1][0];
                        off.y = off.y
                            * gShipTypes[ownerType].compress[1][1];
                    }
                    off.y -= gShipTypes[ownerType]
                                 .weapPosZ[beamData[i].mount][beamData[i].lane];
                    muzzle.x += off.x;
                    muzzle.y += off.y;
                }

                drawPt.x = muzzle.x - shipData[0].pos.x + screenCenter.h;
                drawPt.y = muzzle.y - shipData[0].pos.y + screenCenter.v;
                beamData[i].head.h = drawPt.x;
                beamData[i].head.v = drawPt.y;

                if (beamData[i].state >= 0) {
                    if (gWeapons[beamData[i].weapType].guidance == kGuidancePDBeam
                        && beamData[i].targetIsShot == 1) {
                        if (beamData[i].target != -1) {
                            beamData[i].tail.h = shotData[beamData[i].target].pos.x
                                                 - shipData[0].pos.x + screenCenter.h;
                            beamData[i].tail.v = shotData[beamData[i].target].pos.y
                                                 - shipData[0].pos.y + screenCenter.v;
                            massDmg = gWeapons[beamData[i].weapType].massDmg;
                            energyDmg = gWeapons[beamData[i].weapType].energyDmg / 2;
                            if (doubleTime) {
                                massDmg *= 2;
                                energyDmg *= 2;
                            }
                            if (shotData[beamData[i].target].energy > 0)
                                shotData[beamData[i].target].energy -= massDmg + energyDmg;
                            else {
                                shotData[beamData[i].target].life = 0;
                                SWSetSpriteVisible(shotData[beamData[i].target].sprite, 0);
                                SpawnExplod(shotData[beamData[i].target].pos,
                                            beamData[i].owner, 0, 0);
                            }
                        }
                    } else {
                        if (gWeapons[beamData[i].weapType].flags2 & kWeap22SeedOwnerC8E8)
                            shipData[beamData[i].owner].weapGlow = 32.0f;
                        shipData[beamData[i].owner].fireTick = ReadTick();

                        best = -1;
                        for (j = 0; j < kMaxShips; j++) {
                            notSelf = 1;
                            eligible = 1;
                            if (shipData[j].parentShip != -1) {
                                if (shipData[j].parentShip == beamData[i].owner)
                                    notSelf = 0;
                                if (AlliedWithPlayer(&shipData[j])
                                    && AlliedWithPlayer(&shipData[beamData[i].owner]))
                                    notSelf = 0;
                            }
                            if (j == 0)
                                isAlly = 1;
                            else
                                isAlly = AlliedWithPlayer(&shipData[j]);

                            reachable = 0;
                            parent = beamData[i].owner;
                            for (;;) {
                                if (parent == 0) {
                                    reachable = 1;
                                    break;
                                }
                                if (parent == -1)
                                    break;
                                parent = shipData[parent].parentShip;
                                if (parent == -1)
                                    break;
                            }

                            if (reachable && shipData[j].dude != -1
                                && (dudeData[shipData[j].dude].flags
                                    & kDudeFlagNoBeamTarget))
                                eligible = 0;
                            if (eligible && isAlly) {
                                if (shipData[beamData[i].owner].dude == -1) {
                                    if (shipData[beamData[i].owner].parentShip != -1
                                        && shipData[shipData[beamData[i].owner].parentShip]
                                               .dude != -1
                                        && (dudeData[shipData[shipData[beamData[i].owner]
                                                .parentShip].dude].flags
                                            & kDudeFlagNoBeamTarget))
                                        eligible = 0;
                                } else if (dudeData[shipData[beamData[i].owner].dude].flags
                                           & kDudeFlagNoBeamTarget)
                                    eligible = 0;
                            }
                            if (j == shipData[beamData[i].owner].parentShip)
                                notSelf = 0;
                            shipType = shipData[j].type;
                            if ((gWeapons[beamData[i].weapType].flags & kWeapArcMatch)
                                != (gShipTypes[shipData[j].type].flags & kWeapArcMatch))
                                eligible = 0;

                            if (j != beamData[i].owner && notSelf && eligible
                                && shipData[j].active
                                && shipData[j].curSystem
                                   == shipData[beamData[i].owner].curSystem
                                && shipType != kShipTypeEscapePod) {
                                size = GetMasterShipSpriteXSize(shipType) * 0.66;
                                range = gWeapons[beamData[i].weapType].beamLength
                                        + size / 2;
                                if (XYFloatSquaredDist(&shipData[j].pos,
                                                       &muzzle) <= range * range) {
                                    off.x = shipData[beamData[i].owner].pos.x;
                                    off.y = shipData[beamData[i].owner].pos.y;
                                    diff = __builtin_abs(CalcInterceptAngle(&muzzle,
                                        &shipData[j].pos) - angle);
                                    if (diff <= (short)(size * 10 / 32)) {
                                        dist = XYFloatSquaredDist(
                                            &shipData[j].pos, &muzzle);
                                        if (dist < bestDist || best == -1) {
                                            bestDist = dist;
                                            best = j;
                                        }
                                    }
                                }
                            }
                        }

                        if (beamData[i].owner > 0 && best != -1
                            && AIShipIsAttackingAsteroid(&shipData[beamData[i].owner]))
                            best = -1;

                        if (best != -1) {
                            off.x = muzzle.x;
                            off.y = muzzle.y;
                            len = FloatSqrt(XYFloatSquaredDist(&muzzle,
                                      &shipData[best].pos))
                                  - GetMasterShipSpriteXSize(shipData[best].type) * 0.2;
                            Accel(angle, len, &off);
                            CreateExplosion(off,
                                gWeapons[beamData[i].weapType].explodType,
                                gWeapons[beamData[i].weapType].blastRadius, 1);
                            IonizeShip(&shipData[best],
                                &gWeapons[beamData[i].weapType], 0);
                            if (gWeapons[beamData[i].weapType].hitParticles > 0)
                                SpawnParticles(off.x, off.y,
                                    gWeapons[beamData[i].weapType].hitPartVel, 0x19,
                                    gWeapons[beamData[i].weapType].hitPartLife,
                                    gWeapons[beamData[i].weapType].hitPartLife * 1.25,
                                    gWeapons[beamData[i].weapType].hitPartColor, 0x20,
                                    gWeapons[beamData[i].weapType].hitParticles, 0);

                            massDmg = gWeapons[beamData[i].weapType].massDmg;
                            energyDmg = gWeapons[beamData[i].weapType].energyDmg;
                            impact = gWeapons[beamData[i].weapType].impact;
                            if (doubleTime) {
                                massDmg *= 2;
                                energyDmg *= 2;
                            }
                            if (shipData[best].persType != kPersCaptainHector) {
                                if (impact < 0 && beamData[i].owner != -1) {
                                    if (gShipTypes[shipData[best].type].mass > 0
                                        && (gShipTypes[shipData[best].type].flags
                                            & kShipFlagNoMove) == 0) {
                                        if (gShipTypes[shipData[best].type].mass * 0.75
                                                 <= gShipTypes[shipData[beamData[i].owner]
                                                        .type].mass) {
                                            shipData[best].tractorShip = beamData[i].owner;
                                            shipData[best].tractorTick = ReadTick();
                                        } else {
                                            if (shipData[beamData[i].owner].tractorShip == -1)
                                                shipData[beamData[i].owner].tractorShip =
                                                    beamData[i].owner;
                                            shipData[beamData[i].owner].tractorTick = ReadTick();
                                            drift = fabs(muzzle.x - shipData[best].pos.x);
                                            if (drift >= 50.0f)
                                                goto shipKnock;
                                            drift = fabs(muzzle.y - shipData[best].pos.y);
                                            if (drift >= 50.0f)
                                                goto shipKnock;
                                            goto shipKnockDone;
                                        shipKnock:
                                            if (gShipTypes[shipData[beamData[i].owner]
                                                       .type].mass > 0
                                                && (gShipTypes[shipData[beamData[i].owner]
                                                        .type].flags & kShipFlagNoMove) == 0) {
                                                hitAngle = CalcInterceptAngle(
                                                    &shipData[best].pos, &muzzle);
                                                AdjustedAccel(hitAngle,
                                                    impact / (float)gShipTypes[
                                                        shipData[beamData[i].owner].type].mass,
                                                    &shipData[beamData[i].owner].vel,
                                                    gShipTypes[shipData[beamData[i].owner]
                                                        .type].maxSpeed);
                                                SpeedLimit(
                                                    &shipData[beamData[i].owner].vel,
                                                    ShipMaxSpeed(
                                                        &shipData[beamData[i].owner]));
                                            }
                                        shipKnockDone:
                                            impact = 0;
                                        }
                                    } else
                                        impact = 0;
                                }
                                if (beamData[i].target != -1) {
                                    if (best == beamData[i].target)
                                        DamageShip(&shipData[best], &muzzle, impact,
                                            massDmg, energyDmg, beamData[i].owner, 1, 1,
                                            beamData[i].disableOnly, 0,
                                            gWeapons[beamData[i].weapType].flags
                                                & kWeapFlagIonizing, 1);
                                    else
                                        DamageShip(&shipData[best], &muzzle, impact,
                                            massDmg, energyDmg, beamData[i].owner, 1, 0,
                                            beamData[i].disableOnly, 0,
                                            gWeapons[beamData[i].weapType].flags
                                                & kWeapFlagIonizing, 1);
                                } else {
                                    if (best == shipData[beamData[i].owner].targetShip)
                                        DamageShip(&shipData[best], &muzzle, impact,
                                            massDmg, energyDmg, beamData[i].owner, 1, 1,
                                            beamData[i].disableOnly, 0,
                                            gWeapons[beamData[i].weapType].flags
                                                & kWeapFlagIonizing, 1);
                                    else
                                        DamageShip(&shipData[best], &muzzle, impact,
                                            massDmg, energyDmg, beamData[i].owner, 1, 0,
                                            beamData[i].disableOnly, 0,
                                            gWeapons[beamData[i].weapType].flags
                                                & kWeapFlagIonizing, 1);
                                }
                            }
                            if (best == 0)
                                statusRedraw[1] = 1;
                        } else {
                            best = -1;
                            if ((gWeapons[beamData[i].weapType].seeker
                                 & kWeapSeekPassesAsteroids) == 0) {
                                for (j = 0; j < kMaxAsteroids; j++) {
                                    if (asteroidData[j].active) {
                                        size = GetSpriteYSize(
                                            masterAsteroidSprite[asteroidData[j].type]);
                                        range = gWeapons[beamData[i].weapType].beamLength
                                            + size / 2;
                                        if (XYFloatSquaredDist(
                                                &asteroidData[j].pos,
                                                &muzzle) <= range * range) {
                                            off.x = muzzle.x;
                                            off.y = muzzle.y;
                                            diff = __builtin_abs(CalcInterceptAngle(&muzzle,
                                                &asteroidData[j].pos) - angle);
                                            if (diff <= (short)(size * 10 / 32)) {
                                                dist = XYFloatSquaredDist(
                                                    &shipData[j].pos,
                                                    &muzzle);
                                                if (dist < bestDist || best == -1) {
                                                    bestDist = dist;
                                                    best = j;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            if (best == -1) {
                                len = gWeapons[beamData[i].weapType].beamLength;
                            } else {
                                off.x = muzzle.x;
                                off.y = muzzle.y;
                                len = FloatSqrt(XYFloatSquaredDist(&muzzle,
                                          &asteroidData[best].pos))
                                      - GetSpriteYSize(
                                          masterAsteroidSprite[asteroidData[best].type])
                                        * 0.2;
                                Accel(angle, len, &off);
                                CreateExplosion(off,
                                    gWeapons[beamData[i].weapType].explodType,
                                    gWeapons[beamData[i].weapType].blastRadius, 1);
                                if (gWeapons[beamData[i].weapType].hitParticles > 0)
                                    SpawnParticles(off.x, off.y,
                                        gWeapons[beamData[i].weapType].hitPartVel, 0x19,
                                        gWeapons[beamData[i].weapType].hitPartLife,
                                        gWeapons[beamData[i].weapType].hitPartLife * 1.25,
                                        gWeapons[beamData[i].weapType].hitPartColor, 0x20,
                                        gWeapons[beamData[i].weapType].hitParticles, 0);

                                massDmg = gWeapons[beamData[i].weapType].massDmg;
                                impact = gWeapons[beamData[i].weapType].impact;
                                if (doubleTime)
                                    massDmg *= 2;
                                if (gWeapons[beamData[i].weapType].flags2
                                    & kWeapBigRockDamage)
                                    massDmg *= 10;
                                asteroidData[best].strength -= massDmg;
                                if (asteroidData[best].strength < 0)
                                    DestroyAsteroid(&asteroidData[best]);
                                else if (impact != 0
                                         && asteroidType[asteroidData[best].type].mass > 0) {
                                    if (impact < 0 && beamData[i].owner != -1
                                        && gShipTypes[shipData[beamData[i].owner].type]
                                               .mass
                                           < asteroidType[asteroidData[best].type].mass * 0.5
                                        && gShipTypes[shipData[beamData[i].owner].type]
                                               .mass > 0
                                        && (gShipTypes[shipData[beamData[i].owner].type]
                                                .flags & kShipFlagNoMove) == 0) {
                                        drift = fabs(muzzle.x - asteroidData[best].pos.x);
                                        if (drift >= 50.0f)
                                            goto roidKnock;
                                        drift = fabs(muzzle.y - asteroidData[best].pos.y);
                                        if (drift >= 50.0f)
                                            goto roidKnock;
                                        goto roidKnockDone;
                                    roidKnock:
                                        if (gShipTypes[shipData[beamData[i].owner]
                                                   .type].mass > 0
                                            && (gShipTypes[shipData[beamData[i].owner]
                                                    .type].flags & kShipFlagNoMove) == 0) {
                                            hitAngle = CalcInterceptAngle(
                                                &asteroidData[best].pos, &muzzle);
                                            AdjustedAccel(hitAngle,
                                                impact / (float)gShipTypes[
                                                    shipData[beamData[i].owner].type].mass,
                                                &shipData[beamData[i].owner].vel,
                                                gShipTypes[shipData[beamData[i].owner]
                                                    .type].maxSpeed);
                                            SpeedLimit(
                                                &shipData[beamData[i].owner].vel,
                                                ShipMaxSpeed(
                                                    &shipData[beamData[i].owner]));
                                        }
                                    roidKnockDone:
                                        if (shipData[beamData[i].owner].tractorShip == -1)
                                            shipData[beamData[i].owner].tractorShip =
                                                beamData[i].owner;
                                        shipData[beamData[i].owner].tractorTick = ReadTick();
                                    } else {
                                        AdjustedAccel(angle,
                                            impact / (float)asteroidType[
                                                asteroidData[best].type].mass,
                                            &asteroidData[best].vel, 2.0f);
                                        SpeedLimit(
                                            &asteroidData[best].vel, 2.0f);
                                    }
                                }
                            }
                        }

                        off.x = drawPt.x;
                        off.y = drawPt.y;
                        Accel(angle, len, &off);
                        beamData[i].tail.h = off.x;
                        beamData[i].tail.v = off.y;
                    }
                }
            }
        } else if (beamData[i].state == -1) {
            if (advance)
                beamData[i].state--;
        } else {
            beamData[i].owner = -1;
        }
    }
}

#define kVoiceBaseID 1000


/* HandleVoiceSound. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004311f0
void HandleVoiceSound(void)
{
    short id;
    short voice;
    short count;

    if (voiceSnd != NULL) {
        if (ST_IsSoundPlaying(voiceSnd) == 0) {
            ST_DisposeSound(voiceSnd);
            voiceSnd = NULL;
        }
    }
    if (pendingVoiceMsg != -1) {
        if (voiceSnd != NULL) {
            if (ST_IsSoundPlaying(voiceSnd) != 0) {
                ST_HaltSound(voiceSnd);
            } else {
                ST_DisposeSound(voiceSnd);
                voiceSnd = NULL;
            }
        } else {
            if (pendingVoiceMsg >= 0 && pendingVoiceMsg < kVoiceMessages) {
                if (pendingVoiceGovt >= 0 && pendingVoiceGovt < kMaxGovts)
                    voice = govtData[pendingVoiceGovt].voiceType;
                else
                    voice = 0;
                if (voice != -1) {
                    count = voiceResourceCount[voice][pendingVoiceMsg];
                    if (count > 0) {
                        id = kVoiceBaseID + voice * 100 + pendingVoiceMsg * 10;
                        if (pendingVoiceParity >= 0 &&
                            pendingVoiceParity <= 1 && count % 2 == 0) {
                            if (count == 2)
                                id += pendingVoiceParity;
                            else
                                id += Rand(count >> 1) * 2 + pendingVoiceParity;
                        } else {
                            id += Rand(count);
                        }
                        voiceSnd = LoadSoundResource(id);
                        PlaySound(voiceSnd, 15, gSoundVolume, gSoundVolume);
                    }
                }
            }
            pendingVoiceMsg = -1;
        }
    }
}

/* stop and release the active sound, then forget the selection */

/* KillVoiceSound. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004313c0
void KillVoiceSound(void)
{
    if (voiceSnd != NULL) {
        ST_HaltSound(voiceSnd);
        if (ST_IsSoundPlaying(voiceSnd) == 0) {
            ST_DisposeSound(voiceSnd);
            voiceSnd = NULL;
        }
    }
    pendingVoiceMsg = -1;
    pendingVoiceGovt = -1;
    pendingVoiceParity = -1;
}

/* start the music once, restarting it if it is still running */
/* WarpUpCallback (mac _WarpUpCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00431420
void WarpUpCallback(int which, int a, int b)
{
    if (which != 1)
        return;
    if (warpSoundDone == 0) {
        warpSoundDone = 1;
        if (ST_IsSoundPlaying(warpOutSnd) != 0)
            ST_HaltSound(warpOutSnd);
        PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
    }
}

#define kFlickerMin 0x55
#define kFlickerMax 0x73

/* HandleGlobalPriceFlux. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00431480
void HandleGlobalPriceFlux(void)
{
    short i;
    short step;

    for (i = 0; i <= 1; i++) {
        step = Rand(3);
        if (step == 0)
            globalPriceFlux[i] -= 1;
        if (step == 1)
            globalPriceFlux[i] += 1;
        if (globalPriceFlux[i] > kFlickerMax)
            globalPriceFlux[i] = kFlickerMax;
        if (globalPriceFlux[i] < kFlickerMin)
            globalPriceFlux[i] = kFlickerMin;
    }
}

/* HandleLocalPriceFlux (mac _HandleLocalPriceFlux). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00431500
void HandleLocalPriceFlux(void)
{
    short i;

    for (i = 0; i <= 1; i++)
        localPriceFlux[i] = Rand(0x15) + 0x5a;
}

/* shot->guidanceState, the guidance state machine (names TBC) */
#define kShotStateNormal    0
#define kShotStateRock      1     /* chasing the asteroid in shot->target */
#define kShotStateHeld      998   /* guidance suspended for this shot */
#define kShotStateWobble    999   /* swaying left/right off frameCounter */


/* WeapRecord.seeker bits (names TBC) */
#define kWeapU26SeeksRocks      0x0002  /* may re-target a nearby asteroid */
#define kWeapU26JamReverses     0x0010  /* jamming flips the turn instead of stopping it */
#define kWeapU26LosesLock       0x4000  /* drops a target more than 45 degrees off */
#define kWeapU26TurnsOnOwner    0x8000  /* a jammed shot may come back at its firer */


/* HandleShotGuidance (mac _HandleShotGuidance). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00431530
void HandleShotGuidance(ShotObj *shot)
{
    short g;
    FloatPoint prev;
    float turn;
    float off;
    short dir;
    short diff;
    short lock;
    short delta;
    short i;
    FloatPoint v;

    if (gWeapons[shot->weapType].guidance == kGuidancePointDef)
        return;

    if (shot->guidanceState == kShotStateNormal) {
        if (gWeapons[shot->weapType].guidance == kGuidanceHoming) {
            if (shot->target == -1 ||
                (shot->target != -1 && shipData[shot->target].active != 0 &&
                 shot->curSystem == shipData[shot->target].curSystem)) {
                if (shot->target == -1)
                    dir = shot->angle;
                else
                    dir = CalcInterceptAngle(&shot->pos,
                                             &shipData[shot->target].pos);
                turn = gWeapons[shot->weapType].guidedTurn;
                if (gWeapons[shot->weapType].lifetime - shot->life > gTimeScale * 15.0) {
                    if (shot->target != -1) {
                        for (g = 0; g <= 3; g++) {
                            if (shot->glow[g] > 0) {
                                if (ShipECM(&shipData[shot->target], g) >
                                    100 - shot->glow[g]) {
                                    if (turn > 0.0f) {
                                        if ((gWeapons[shot->weapType].seeker &
                                             kWeapU26JamReverses) != 0)
                                            turn = turn * -1.0f;
                                        else
                                            turn = 0.0f;
                                    }
                                    if ((gWeapons[shot->weapType].seeker &
                                         kWeapU26TurnsOnOwner) != 0 && Rand(500) == 0 &&
                                        shot->owner >= 0 && shot->owner < kMaxShips) {
                                        shot->target = shot->owner;
                                        shot->owner = -1;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    if (shot->guidanceState == kShotStateNormal) {
                        if (shot->target != -1 && shot->owner >= 0 &&
                            shot->owner < kMaxShips &&
                            ShipVisibleToShip(&shipData[shot->target],
                                              &shipData[shot->owner]) == 0) {
                            turn = 0.0f;
                            if ((gWeapons[shot->weapType].seeker &
                                 kWeapU26TurnsOnOwner) != 0 && Rand(1000) == 0 &&
                                shot->owner != -1) {
                                shot->target = shot->owner;
                                shot->owner = -1;
                            }
                        }
                        if ((gWeapons[shot->weapType].seeker & kWeapU26LosesLock) != 0 &&
                            shot->target != -1 &&
                            abs((short)(shipData[shot->target].pos.x - shot->pos.x)) < 250 &&
                            abs((short)(shipData[shot->target].pos.y - shot->pos.y)) < 250) {
                            off = CalcInterceptAngle(&shot->pos,
                                  &shipData[shot->target].pos) -
                                  shot->angle;
                            off = fabs(off);
                            lock = off;
                            lock = lock % 360;
                            if (lock > 45)
                                shot->target = -1;
                        }
                    }
                    diff = abs(AngularDifference(dir, shot->angle));
                    if (diff > abs((short)turn)) {
                        delta = dir - (long)shot->angle;
                        if (delta >= 360)
                            delta -= 360;
                        if (delta < 0)
                            delta += 360;
                        if (delta > 180)
                            shot->angle -= turn * gTimeScale;
                        else
                            shot->angle += turn * gTimeScale;
                    }
                }
                if (shot->angle >= 360.0f)
                    shot->angle = shot->angle - 360.0f;
                if (shot->angle < 0.0f)
                    shot->angle = shot->angle + 360.0f;
                shot->vel.x = shot->vel.y = 0.0f;
                Accel(shot->angle, gWeapons[shot->weapType].speed100, &shot->vel);
                SpeedLimit(&shot->vel, gWeapons[shot->weapType].speed100);
            }
        }
        if (gWeapons[shot->weapType].guidance == kGuidanceHoming &&
            (gWeapons[shot->weapType].seeker & kWeapU26SeeksRocks) != 0 && Rand(10) == 0) {
            for (i = 0; i < kMaxAsteroids; i++) {
                if (asteroidData[i].active != 0) {
                    if (abs((short)(asteroidData[i].pos.x - shot->pos.x)) < 200 &&
                        abs((short)(asteroidData[i].pos.y - shot->pos.y)) < 200) {
                        off = CalcInterceptAngle(&shot->pos, &asteroidData[i].pos) -
                              shot->angle;
                        off = fabs(off);
                        diff = off;
                        diff = diff % 360;
                        if (diff <= 15) {
                            shot->guidanceState = kShotStateRock;
                            shot->target = i;
                            break;
                        }
                    }
                }
            }
        }
    } else if (shot->guidanceState == kShotStateWobble) {
        if (gWeapons[shot->weapType].lifetime - shot->life > 15.0) {
            if (frameCounter % 300 < 150)
                shot->angle = shot->angle - gWeapons[shot->weapType].guidedTurn;
            else
                shot->angle = shot->angle + gWeapons[shot->weapType].guidedTurn;
        }
        if (shot->angle >= 360.0f)
            shot->angle = shot->angle - 360.0f;
        if (shot->angle < 0.0f)
            shot->angle = shot->angle + 360.0f;
        shot->vel.x = shot->vel.y = 0.0f;
        Accel(shot->angle, gWeapons[shot->weapType].speed100, &shot->vel);
        SpeedLimit(&shot->vel, gWeapons[shot->weapType].speed100);
        if ((gWeapons[shot->weapType].seeker & kWeapU26TurnsOnOwner) != 0 &&
            Rand(1000) == 0 && shot->owner >= 0 && shot->owner < kMaxShips) {
            shot->guidanceState = kShotStateNormal;
            shot->target = shot->owner;
            shot->owner = -1;
        }
    } else if (shot->guidanceState != kShotStateHeld) {
        if (shot->guidanceState == kShotStateRock) {
            if (shot->target != -1) {
                if (asteroidData[shot->target].active != 0)
                    dir = CalcInterceptAngle(&shot->pos,
                                             &asteroidData[shot->target].pos);
                else
                    shot->target = -1;
            } else {
                dir = shot->angle;
            }
        }
        if (gWeapons[shot->weapType].lifetime - shot->life > 15.0) {
            diff = abs(AngularDifference(dir, shot->angle));
            if (diff > gWeapons[shot->weapType].guidedTurn) {
                delta = dir - shot->angle;
                if (delta >= 360)
                    delta -= 360;
                if (delta < 0)
                    delta += 360;
                if (delta > 180)
                    shot->angle = shot->angle - gWeapons[shot->weapType].guidedTurn;
                else
                    shot->angle = shot->angle + gWeapons[shot->weapType].guidedTurn;
            }
        }
        if (shot->angle >= 360.0f)
            shot->angle = shot->angle - 360.0f;
        if (shot->angle < 0.0f)
            shot->angle = shot->angle + 360.0f;
        shot->vel.x = shot->vel.y = 0.0f;
        Accel(shot->angle, gWeapons[shot->weapType].speed100, &shot->vel);
        SpeedLimit(&shot->vel, gWeapons[shot->weapType].speed100);
    }

    if (gWeapons[shot->weapType].guidance == kGuidanceRocket) {
        prev.x = shot->vel.x;
        prev.y = shot->vel.y;
        v.x = v.y = 0.0f;
        Accel(shot->angle, gWeapons[shot->weapType].speed100, &v);
        shot->vel.x = (prev.x * 95.0f + v.x * 5.0f) * 0.01;
        shot->vel.y = (prev.y * 95.0f + v.y * 5.0f) * 0.01;
    }
    if (gWeapons[shot->weapType].guidance == kGuidanceFreefall) {
        prev.x = prev.y = 0.0f;
        v.x = shot->vel.x * 1000.0;
        v.y = shot->vel.y * 1000.0;
        dir = CalcInterceptAngle(&prev, &v);
        diff = abs(AngularDifference(dir, shot->angle));
        if (diff > 0) {
            delta = dir - shot->angle;
            if (delta >= 360)
                delta -= 360;
            if (delta < 0)
                delta += 360;
            if (delta > 180)
                shot->angle = shot->angle - 1.0f;
            else
                shot->angle = shot->angle + 1.0f;
        }
    }
}

/* HandleStellarSystemVisibility (mac _HandleStellarSystemVisibility). STATE: MATCH 100.0%
   598/598, byte-identical yes (2608 bytes), relocregcheck 0 of 137. OPEN: none. */
// FUNCTION: EVNOVA 0x00432470
void HandleStellarSystemVisibility(void)
{
    char fresh;
    short n;
    char used[256];
    short i;
    short j;
    short sys;
    short other;
    short sprID;
    short err;

    for (j = 0; j < kMaxHyperRoute; j++) {
        sys = hyperRoute[j];
        if (sys != -1 && !gSystems[sys].loaded) {
            hyperRoute[j] = FindActiveCoLocatedSystem(sys);
            if (hyperRoute[j] == -1) {
                for (i = j; i < kMaxHyperRoute; i++)
                    hyperRoute[i] = -1;
                break;
            }
        }
    }

    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].prepared) {
            other = FindFirstCoLocatedSystem(i);
            if (other != i && other >= 0 && other < kMaxSystems) {
                if (gSystems[i].exploreLevel < gSystems[other].exploreLevel)
                    gSystems[i].exploreLevel = gSystems[other].exploreLevel;
            }
        }
        gSystems[i].known = 0;
    }

    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].loaded && gSystems[i].prepared && gSystems[i].exploreLevel > 0) {
            gSystems[i].known = 1;
            for (j = 0; j < kMaxSystemLinks; j++) {
                if (gSystems[i].links[j] != -1) {
                    other = FindActiveCoLocatedSystem(gSystems[i].links[j]);
                    if (other != -1)
                        gSystems[other].known = 1;
                }
            }
        }
    }

    for (i = 0; i < kMasterStellarSprites; i++)
        used[i] = 0;

    for (i = 0; i < kMaxStellars; i++) {
        stellarData[i].loaded = 0;
        if (stellarData[i].prepared) {
            if (stellarData[i].system < 0 || stellarData[i].system >= kMaxSystems) {
                for (j = 0; j < kNavSlots; j++) {
                    if (gSystems[shipData[0].curSystem].navDefault[j] == i) {
                        stellarData[i].system = shipData[0].curSystem;
                        break;
                    }
                }
            }
            if (stellarData[i].system >= 0 && stellarData[i].system < kMaxSystems) {
                if (gSystems[stellarData[i].system].loaded) {
                    stellarData[i].loaded = 1;
                    if (stellarData[i].flags2 & kSpobFlag2Bit5)
                        stellarData[i].dominated = 1;
                }
            }
        }
        if (stellarData[i].loaded) {
            if (shipData[0].curSystem == stellarData[i].system &&
                stellarData[i].system != -1) {
                sprID = StellarIsDestroyed(&stellarData[i]) ? stellarData[i].deadType
                                                            : stellarData[i].type;
                if (masterStellarSprite[sprID] != NULL)
                    used[sprID] = 1;
            }
        }
        stellarData[i].defenseActive = 0;
    }

    for (i = 0; i < kMasterStellarSprites; i++) {
        if (masterStellarSprite[i] != NULL && used[i] == 0) {
            SWUnlockSprite(masterStellarSprite[i]);
            if (masterStellarSprite[i]->frameArray != NULL) {
                if (masterStellarSprite[i]->rleDataH != NULL) {
                    DeallocAHandle(masterStellarSprite[i]->rleDataH);
                    masterStellarSprite[i]->rleDataH = NULL;
                }
                for (j = 0; j < masterStellarSprite[i]->numFrames; j++) {
                    DeallocAPointer(masterStellarSprite[i]->frameArray[j]);
                    masterStellarSprite[i]->frameArray[j] = NULL;
                }
                masterStellarSprite[i]->numFrames = 0;
                DeallocAPointer(masterStellarSprite[i]->frameArray);
                masterStellarSprite[i]->frameArray = NULL;
            }
            SWDisposeSprite(&masterStellarSprite[i]);
            masterStellarSprite[i] = NULL;
        }
    }

    for (i = 0; i < kStellarSprites; i++) {
        if (stellarSprite[i] != NULL) {
            if (stellarSprite[i]->isEmptySprite) {
                stellarSprite[i]->frameArray = NULL;
                stellarSprite[i]->curFrameP = NULL;
                stellarSprite[i]->rleDataH = NULL;
            }
            stellarSprite[i]->userData = -1;
            SWSetSpriteVisible(stellarSprite[i], 0);
        }
    }

    n = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].loaded &&
            shipData[0].curSystem == stellarData[i].system &&
            stellarData[i].system != -1) {
            sprID = StellarIsDestroyed(&stellarData[i]) ? stellarData[i].deadType
                                                        : stellarData[i].type;
            if (masterStellarSprite[sprID] == NULL)
                LoadMasterStellarSprite(sprID);
            fresh = 0;
            if (stellarSprite[n] == NULL) {
                fresh = 1;
                err = SWCreateEmptySprite(&stellarSprite[n], NULL);
                if ((long)err != 0)
                    ReportSpriteLoadError(1, 0, err);
            }
            stellarData[i].sprite = stellarSprite[n];
            stellarSprite[n]->userData = i;
            if (masterStellarSprite[sprID] != NULL)
                SWCloneFramesFromMaster(stellarSprite[n], masterStellarSprite[sprID]);
            SWLockSprite(stellarSprite[n]);
            if (fresh)
                SWAddSprite(stellarLayer, stellarSprite[n]);
            SWSetSpriteDrawProc(stellarSprite[n], BlitPixieRLETranslucentDrawProc);
            SWSetSpriteVisible(stellarSprite[n], 1);
            SWSetCurrentFrameIndex(stellarSprite[n], stellarData[i].curFrame);
            SWSetSpriteLocation(stellarSprite[n],
                (stellarData[i].pos.h - shipData[0].pos.x + screenCenter.h
                       - GetSpriteXSize(stellarSprite[n]) / 2),
                (stellarData[i].pos.v - shipData[0].pos.y + screenCenter.v
                       - GetSpriteYSize(stellarSprite[n]) / 2));
            stellarData[i].numFrames = stellarSprite[n]->numFrames;
            if (fresh) {
                stellarData[i].frameTimer = 0;
                stellarData[i].curFrame = 0;
                stellarData[i].nextFrame = 1;
            }
            n++;
            if (n >= kStellarSprites)
                break;
        }
    }

    for (i = 0; i < kStellarSprites; i++) {
        if (stellarSprite[i] == NULL)
            continue;
        if (stellarSprite[i]->frameArray == NULL || stellarSprite[i]->rleDataH == NULL) {
            if (stellarSprite[i]->userData >= 0 && stellarSprite[i]->userData < kMaxStellars) {
                crashlog_print("warning: stellar sprite ");
                crashlog_print_num(i);
                crashlog_print(" (spob ID ");
                crashlog_print_num(stellarSprite[i]->userData + 128);
                crashlog_print(") had no data\n");
            }
            if (masterStellarSprite[0] == NULL)
                LoadMasterStellarSprite(0);
            SWCloneFramesFromMaster(stellarSprite[i], masterStellarSprite[0]);
            SWSetCurrentFrameIndex(stellarSprite[i], 0);
        }
        if (stellarSprite[i]->curFrameP == NULL) {
            crashlog("warning: stellar sprite had invalid frame pointer");
            SWSetCurrentFrameIndex(stellarSprite[i], 0);
        } else if (stellarSprite[i]->curFrameP->isFrameLocked == 0) {
            crashlog("warning: stellar sprite frame was not locked");
        }
        SWLockSprite(stellarSprite[i]);
    }
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* framepace -- hold the frame to a floor length and track how fast the machine is running (win 0x00432ea0). Notes: Research/notes/framepace.md */

/* HandleTimeAdjustment (mac _HandleTimeAdjustment). STATE: MATCH 100.0%, exe 105 / obj 105
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00432ea0
void HandleTimeAdjustment(void)
{
    UnsignedWide now;
    unsigned long ms;
    float rate;

    do {
        Microseconds(&now);
        if (now.hi > lastFrameMicroseconds.hi)
            ms = (0xffffffff - (lastFrameMicroseconds.lo - now.lo)) / 1000;
        else
            ms = (now.lo - lastFrameMicroseconds.lo) / 1000;
        if (ms < 10 && (shipData[0].jumpTimer <= 0.0f || shipData[0].active == 0)) {
            if (yieldEventsPref != 0) {
                IterateInGameEventLoop(1);
                lastEventLoopTime = ReadTick();
            }
        }
    } while (ms <= 20);

    if (lastFrameTime == 0)
        rate = 1.0;
    else
        rate = ms * 0.03;
    if (rate >= 0.01 && rate < 10.0)
        gTimeScale = (rate + gTimeScale * 3.0) * 0.25;
    lastFrameTime += ms;
    lastFrameMicroseconds.lo = now.lo;
    lastFrameMicroseconds.hi = now.hi;
    if (gTimeScale > 1.7)
        gFrameStep = 8;
    else if (gTimeScale > 1.3)
        gFrameStep = 4;
    else if (gTimeScale > 1.0)
        gFrameStep = 2;
    else
        gFrameStep = 1;
}

#define kPersDisabledOnly   0x0020
#define kPersNeedsLoaded    0x0004
#define kPersNeedsEscort    0x0008
#define kPersBoardable      0x0010
#define kPersHasMission     0x0400
#define kPersNeedsRating    0x0800
#define kPersAIOne          0x1000
#define kPersAITwo          0x2000
#define kPersAIThree        0x4000
#define kPersOnceOnly       0x0080
#define kPersDeathSpawn     0x0002
#define kGovtNoDeathSpawn   0x0100

/* HandleShip (mac _HandleShip). STATE: MATCH 100.0%, exe 2290 / obj 2290; relocregcheck 0 of
   342; byte-identical: yes. OPEN: none. */
// FUNCTION: EVNOVA 0x00433050
void HandleShip(ShipObj *ship)
{
    float d;
    unsigned char msg[256];
    short dir;
    short i;
    char disabled;
    char cloaked;
    char ok;
    char hailNow;
    short ion;
    float delta;
    float maxSpeed;
    float turnRate;
    double span;
    float v;
    float vy;
    float vx;
    double factor;
    short n;
    short j;
    long inRange;

    dir = 0;
    if (ship->type < 0 || ship->type >= kMaxShipTypes) {
        ship->active = 0;
        return;
    }
    if (gShipTypes[ship->type].techLevel == kResAbsent) {
        ship->active = 0;
        return;
    }
    if (ship->type < 0 || ship->type >= kMaxShipTypes) {
        ship->type = 0;
        crashlog_print("warning: ship index ");
        crashlog_print_num(ship->index);
        crashlog_print(" had bad type, setting to 0\n");
    }
    if (ship->govt < -1 || ship->govt >= kMaxGovts) {
        ship->govt = -1;
        crashlog_print("warning: ship index ");
        crashlog_print_num(ship->index);
        crashlog_print(" (type ");
        crashlog_print_num(ship->type + 128);
        crashlog_print(") had bad govt, setting to -1\n");
    }
    if (ship->dude < -1 || ship->dude >= kMaxDudes) {
        ship->dude = -1;
        crashlog_print("warning: ship index ");
        crashlog_print_num(ship->index);
        crashlog_print(" (type ");
        crashlog_print_num(ship->type + 128);
        crashlog_print(") had bad dude class, setting to -1\n");
    }
    if (ship->persType < -1 || ship->persType >= kMaxPersons) {
        ship->persType = -1;
        crashlog_print("warning: ship index ");
        crashlog_print_num(ship->index);
        crashlog_print(" (type ");
        crashlog_print_num(ship->type + 128);
        crashlog_print(") had bad person class, setting to -1\n");
    }
    if (ship->parentShip < -1 || ship->parentShip >= kMaxShips) {
        ship->parentShip = -1;
        crashlog_print("warning: ship index ");
        crashlog_print_num(ship->index);
        crashlog_print(" (type ");
        crashlog_print_num(ship->type + 128);
        crashlog_print(") had bad parent, setting to -1\n");
    }
    /* BUG: range-checks defenseStellar but resets parentShip */
    if (ship->defenseStellar < -1 || ship->defenseStellar >= kMaxStellars) {
        ship->parentShip = -1;
        crashlog_print("warning: ship index ");
        crashlog_print_num(ship->index);
        crashlog_print(" (type ");
        crashlog_print_num(ship->type + 128);
        crashlog_print(") had bad defensive index, setting to -1\n");
    }
    if (ship->specialMission < -1 || ship->specialMission >= kMaxActiveMissions) {
        ship->specialMission = -1;
        crashlog_print("warning: ship index ");
        crashlog_print_num(ship->index);
        crashlog_print(" (type ");
        crashlog_print_num(ship->type + 128);
        crashlog_print(") was linked to invalid mission index, setting to -1\n");
    }
    if (ship->targetShip < -1 || ship->targetShip >= kMaxShips)
        ship->targetShip = -1;

    ion = ShipIonizationFactor(ship);
    disabled = IsDisabled(ship);
    cloaked = ShipIsCloaked(ship);

    if (ship->targetShip != -1 && !shipData[ship->targetShip].active)
        ship->targetShip = -1;

    for (j = 0; j < kMaxWeapTypes; j++) {
        if (ship->weap[j].count > 0) {
            if (ship->weap[j].reloadTimer > 0.0f)
                ship->weap[j].reloadTimer -= gTimeScale;
            else
                ship->weap[j].reloadTimer = 0;
            if ((gWeapons[j].seeker & kWeapSeekNoFireIonized) && ion > 0)
                ship->weap[j].reloadTimer = 1.0f;
        }
    }
    if (ship->targetShip == -1) {
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (gWeapons[i].guidance == kGuidanceBay
                    && ship->weap[i].count > 0)
                ship->weap[i].reloadTimer = gWeapons[i].reload;
        }
    }

    if (disabled) {
        ship->vel.x *= 0.995;
        ship->vel.y *= 0.995;
        ship->accelMag *= 0.995;
        if (ship->specialMission == -1 && ship->defenseStellar == -1 && ship->parentShip == 0) {
            if (gShipTypes[ship->type].inherentAI <= 2)
                DestroyPartialFleetCargo(ship->index);
            ship->parentShip = -1;
            ship->hired = 0;
            ship->boarded = 1;
            ship->aiType = gShipTypes[ship->type].inherentAI;
            ShipStatsToSystemInfo();
            statusRedraw[2] = 1;
        } else if (ShipCanSelfRepair(ship)) {
            if (gShipTypes[ship->type].flags & kShipFlagHalfArmor)
                ship->armor = ShipArmorCapacity(ship) * 0.1 + 1.0;
            else
                ship->armor = ShipArmorCapacity(ship) * 0.3333 + 1.0;
            if (shipData->targetShip == ship->index)
                statusRedraw[6] = 1;
            if (ship->formerEscort >= 0) {
                ship->parentShip = 0;
                ship->shield = 0;
                ship->targetStellar = -1;
                ship->targetShip = -1;
                if (ship->formerEscort == 0) {
                    ship->aiType = 5;
                    GetIndString(msg, kGameStrList, 0x80);
                    DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else {
                    if (ship->formerEscort == 1)
                        ship->hired = 1;
                    else
                        ship->hired = 0;
                    ship->aiType = 6;
                    GetIndString(msg, kGameStrList, 0x7f);
                    DisplayComm(msg, 0xfa, 0, 0xc, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                }
            }
        }
    } else {
        ship->formerEscort = -1;
    }

    if (ship->persType != -1) {
        if (personData[ship->persType].shieldMod < 0.0f) {
            ship->shield = ShipShieldCapacity(ship);
            ship->armor = ShipArmorCapacity(ship);
        }
        if (personData[ship->persType].hailQuote != -1
                && personData[ship->persType].loaded != 0
                && shipData->jumpTimer <= 0.0f
                && ShipVisibleToShip(shipData, ship)
                && !IsDying(ship)
                && !AIIsShipJumping(ship)) {
            hailNow = 0;
            ok = 1;
            if (IsDisabled(ship)) {
                if (!(personData[ship->persType].flags & kPersDisabledOnly))
                    ok = 0;
            } else {
                if (personData[ship->persType].flags & kPersDisabledOnly)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersBoardable) {
                if (!IsDisabled(ship)) {
                    if (ship->hailed != 0)
                        ok = 0;
                    else if (IsThreatToPlayer(ship)) {
                        ok = 1;
                        hailNow = 1;
                    } else
                        ok = 0;
                }
            }
            if (personData[ship->persType].flags & kPersNeedsLoaded) {
                if (personData[ship->persType].grudge == 0)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersNeedsEscort) {
                if (!AIDoesShipLikePlayer(ship))
                    ok = 0;
            }
            if (personData[ship->persType].linkMission != -1
                    && (personData[ship->persType].flags & kPersHasMission)) {
                offeringFromShip = 1;
                offeringShipIndex = ship->index;
                if (!IsMissionAvailable(personData[ship->persType].linkMission, 0))
                    ok = 0;
                offeringFromShip = 0;
                offeringShipIndex = -1;
            }
            if (personData[ship->persType].flags & kPersNeedsRating) {
                if (AIIsShipLeaving(ship))
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersAIOne) {
                if (gShipTypes[shipData->type].inherentAI == 1)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersAITwo) {
                if (gShipTypes[shipData->type].inherentAI == 2)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersAIThree) {
                if (gShipTypes[shipData->type].inherentAI >= 3)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersOnceOnly) {
                if (ship->hailed != 0)
                    ok = 0;
            }
            if (ok) {
                if ((Rand(0x8c) == 0 && commCount <= 0
                        && ReadTick() > ship->hailTick + 0xa8c)
                        || hailNow) {
                    offeringShipIndex = ship->index;
                    ShowPersonHailQuote(personData[ship->persType].hailQuote);
                    offeringShipIndex = -1;
                    ship->hailed = 1;
                    ship->hailTick = ReadTick();
                }
            }
        }
    }

    /* three consecutive ifs, each with its arm parked far away */
    if (ship->tractorShip != -1) {
        d = ShipAccelRate(shipData) * 0.25;
        if (ship->tractorShip == ship->index) {
            vx = 0.0f;
            vy = 0.0f;
        } else {
            vx = shipData[ship->tractorShip].vel.x;
            vy = shipData[ship->tractorShip].vel.y;
        }
        if (vx + d < ship->vel.x)
            ship->vel.x -= d;
        if (vx - d > ship->vel.x)
            ship->vel.x += d;
        if (vy + d < ship->vel.y)
            ship->vel.y -= d;
        if (vy - d > ship->vel.y)
            ship->vel.y += d;
        {
            float dv;

            dv = fabs(ship->vel.x - vx);
            if (dv < d)
                ship->vel.x = vx;
            dv = fabs(ship->vel.y - vy);
            if (dv < d)
                ship->vel.y = vy;
        }
    }
    if (cloaked) {
        if (!ShipCanCloak(ship))
            DoShipUncloak(ship);
        n = CloakFuelUsage(ship);
        if (n > 0) {
            ship->fuel -= n * 0.03333 * gTimeScale;
            if (ship->fuel <= 0.0f)
                ship->fuel = 0;
        }
        n = CloakShieldUsage(ship);
        if (n > 0 && n <= ship->shield) {
            ship->shield -= n * 0.03333 * gTimeScale;
            if (ship->shield <= 0.0f)
                ship->shield = 0;
        }
    }

    if (gShipTypes[ship->type].accel == 0.0f
            && gShipTypes[ship->type].maxSpeed == 0.0f) {
        ship->vel.x = 0;
        ship->vel.y = 0;
    } else {
        ShipMaxSpeed(ship);                 /* result DISCARDED; not dead */
        if (ShipIsInertialess(ship))
            AdjustInertialessShipVelocity(ship);
        if (gameFrozen == 0) {
            ship->pos.x += ship->vel.x * gTimeScale;
            ship->pos.y += ship->vel.y * gTimeScale;
        }
    }

    if (ship->ionization > 0.0f) {
        ship->ionization -= ShipDeionizationRate(ship) * gTimeScale;
        d = ShipIonizationFactor(ship);
        if (d > 0.7)
            d = 0.7f;
        d = ShipMaxSpeed(ship) * (1.0f - d);
        if (ship->vel.x > d)
            ship->vel.x -= gTimeScale * 0.025;
        if (-d > ship->vel.x)
            ship->vel.x += gTimeScale * 0.025;
        if (ship->vel.y > d)
            ship->vel.y -= gTimeScale * 0.025;
        if (-d > ship->vel.y)
            ship->vel.y += gTimeScale * 0.025;
    } else {
        ship->ionization = 0;
    }

    if (!disabled && gameFrozen == 0) {
        if (ship->aiTimer <= 0.0f || AIIsShipDoingCountWait(ship)) {
            delta = ship->desiredHeading - ship->heading;
            if (delta >= 360.0f)
                delta -= 360.0f;
            if (delta < 0.0f)
                delta += 360.0f;
            v = abs(AngularDifference(ship->desiredHeading, ship->heading));
            if (v > ShipTurnRate(ship) * gTimeScale) {
                if (delta > 180.0f) {
                    ship->heading -= ShipTurnRate(ship) * gTimeScale;
                    dir = -1;
                } else {
                    ship->heading += ShipTurnRate(ship) * gTimeScale;
                    dir = 1;
                }
            } else {
                ship->heading = ship->desiredHeading;
            }
        }
        if (ShipShieldCapacity(ship) > ship->shield)
            ship->shield += ShipShieldRechargeRate(ship) * gTimeScale;
        if (ShipArmorCapacity(ship) > ship->armor)
            ship->armor += ShipArmorRechargeRate(ship) * gTimeScale;
    }
    if (ship->heading >= 360.0f)
        ship->heading -= 360.0f;
    if (ship->heading < 0.0f)
        ship->heading += 360.0f;

    if (ship->jumpTimer > 0.0f) {
        if (!IsDisabled(ship) && gameFrozen == 0) {
            turnRate = ShipTurnRate(ship);
            inRange = abs(AngularDifference(ship->heading, ship->desiredHeading)) <= turnRate;
            if (inRange) {
                if (AIIsShipJumping(ship)) {
                    ship->vel.x *= 0.95;
                    ship->vel.y *= 0.95;
                    d = 0.0f;
                    if (ship->parentShip != -1)
                        ship->desiredHeading = shipData[ship->parentShip].desiredHeading;
                    if (ship->parentShip == 0) {
                        if (shipData->jumpTimer > 1.0f) {
                            span = WarpSoundDuration() * 0.01;
                            factor = doubleTime ? 0.667 : 1.0;
                            {
                                double t;
                                float mobility;

                                t = (ReadTick() - shipData->jumpTick)
                                     * factor
                                     * gShipTypes[shipData->type].mobilityFactor
                                     / span;
                                d = t;
                                mobility = gShipTypes[shipData->type].mobilityFactor;
                                if (mobility != 0)
                                    d = d - 45.0 / mobility;
                            }
                        }
                    } else {
                        span = WarpSoundDuration() * 0.01;
                        factor = doubleTime ? 0.5 : 1.0;
                        {
                            double t;
                            float mobility;

                            t = (ReadTick() - ship->jumpTick)
                                 * factor
                                 * gShipTypes[ship->type].mobilityFactor / span;
                            d = t;
                            mobility = gShipTypes[ship->type].mobilityFactor;
                            if (mobility != 0)
                                d = d - 35.0 / mobility;
                        }
                    }
                    if (d > 50.0)
                        d = 50.0f;
                    if (d > 0.0) {
                        Accel(ship->heading, d * gTimeScale,
                              &ship->pos);
                        if (ship->engineGlow < 0x20) {
                            ship->engineGlow += 3;
                            if (ship->engineGlow > 0x20)
                                ship->engineGlow = 0x20;
                        }
                    }
                } else {
                    ship->jumpTimer = 0;
                }
            } else {
                ship->jumpTick = ReadTick();
            }
        }
    }

    if (ship->aiTimer <= 0.0f || AIIsShipDoingCountWait(ship)
            || ship->type == kShipTypeSpecial) {
        if (ship->thrust != 0.0) {
            v = ship->speedLimit;
            if (v == 0.0) {
                if (ship->jumpTimer <= 0.0f) {
                    maxSpeed = ShipMaxSpeed(ship);
                    if (ShipIsInertialess(ship)) {
                        ship->accelMag = ship->thrust * gTimeScale
                                         + ship->accelMag;
                        if (ship->accelMag > maxSpeed)
                            ship->accelMag = maxSpeed;
                        if (ship->accelMag < 0.0)
                            ship->accelMag = 0;
                    } else {
                        AdjustedAccel(ship->heading,
                                     ship->thrust * gTimeScale,
                                     &ship->vel, maxSpeed);
                    }
                }
            } else if (v > 0.0) {
                if (ShipIsInertialess(ship)) {
                    ship->accelMag = ship->thrust * gTimeScale + ship->accelMag;
                    if (ship->accelMag > v)
                        ship->accelMag = v;
                    if (ship->accelMag < 0.0)
                        ship->accelMag = 0;
                } else {
                    AdjustedAccel(ship->heading,
                                 ship->thrust * gTimeScale,
                                 &ship->vel, v);
                }
            } else {
                {
                    float a;

                    if (ShipIsInertialess(ship)) {
                        a = fabs(ship->speedLimit);
                        ship->accelMag = a;
                    } else {
                        ship->vel.x = ship->vel.y = 0;
                        a = fabs(ship->speedLimit);
                        Accel(ship->heading, a, &ship->vel);
                    }
                }
                {
                    float f;

                    f = fabs(ship->thrust);
                    ship->speedLimit = ship->speedLimit + f;
                }
                if (ship->parentShip == -1 || ship->parentShip >= kMaxShips)
                    d = ShipMaxSpeed(ship);
                else if (ShipMaxSpeed(ship)
                             < ShipMaxSpeed(&shipData[ship->parentShip]))
                    d = ShipMaxSpeed(ship);
                else
                    d = ShipMaxSpeed(&shipData[ship->parentShip]);
                if (d < -1.0)
                    d = -1.0f;
                if (-d <= ship->speedLimit || ship->speedLimit >= 0.0f) {
                    AIMakeShipStopJumping(ship);
                    if (ShipIsInertialess(ship)) {
                        ship->speedLimit = d;
                        ship->vel.x = 0;
                        ship->vel.y = 0;
                    }
                    if (ship->parentShip == -1
                            && ship->persType != kPersCaptainHector)
                        ship->aiTimer = (Rand(0x1e) + 0x1e);
                }
            }
        }
        if (gShipTypes[ship->type].shanFlags & kShipRecRollable) {
            if (dir == 1) {
                if (ship->frameTimer < 8.0f)
                    ship->frameTimer += gTimeScale;
            } else if (dir == -1) {
                if (ship->frameTimer > -8.0f)
                    ship->frameTimer -= gTimeScale;
            } else if (ship->frameTimer >= gTimeScale) {
                ship->frameTimer -= gTimeScale;
            } else if (-gTimeScale >= ship->frameTimer) {
                ship->frameTimer += gTimeScale;
            } else {
                ship->frameTimer = 0;
            }
            if (ship->frameTimer > 4.0f)
                ship->bankDir = 1;
            else if (ship->frameTimer < -4.0f)
                ship->bankDir = -1;
            else
                ship->bankDir = 0;
            if (ship->bankDir != 0) {
                if ((gShipTypes[ship->type].shanFlags & kShipRecBanks)
                        && ship->engineGlow < 0x18)
                    ship->engineGlow += 2;
            }
        }
        if (ship->thrust > 0.0f
                && (ship->aiTimer <= 0.0f || AIIsShipDoingCountWait(ship))) {
            if (gShipTypes[ship->type].shanFlags & kShipRecRollable)
                ship->bankDir = dir;
            if (ShipAccelRate(ship) * 2.0f <= ship->thrust) {
                if (ship->engineGlow < 0x20)
                    ship->engineGlow += 1;
            } else if (ship->engineGlow < 0x18) {
                ship->engineGlow += 1;
            } else if (ship->engineGlow > 0x18) {
                ship->engineGlow -= 1;
            }
        } else if (ship->engineGlow > 0) {
            ship->engineGlow -= 1;
        }
        if (ship->fireQueued != 0 && ship->selectedWeapon != -1) {
            if (gWeapons[ship->selectedWeapon].flags & kWeapFlagTurret) {
                FireAIShipWeapon(ship);
            } else {
                FireAIShipWeapon(ship);
                ship->selectedWeapon = -1;
                ship->fireQueued = 0;
            }
        }
    }
    if (ship->aiTimer > 0.0f && ship->type != kShipTypeSpecial) {
        ship->aiTimer -= gTimeScale;
        if (!AIIsShipDoingCountWait(ship) && ship->engineGlow > 0)
            ship->engineGlow -= 1;
    }

    if (ship->deathTimer > 0.0f) {
        if (gameFrozen == 0)
            ship->deathTimer -= 1.0f;
        if (ship->parentShip == 0) {
            if (ship->aiType == 6) {
                if (gShipTypes[ship->type].inherentAI <= 2) {
                    DestroyPartialFleetCargo(ship->index);
                    statusRedraw[2] = 1;
                    DrawStatusCargo();
                }
            }
            ship->parentShip = -1;
            ship->aiType = gShipTypes[ship->type].inherentAI;
        }
        if (ship->deathTimer == gShipTypes[ship->type].deathDelay * 0.5) {
            if (ship->persType != -1) {
                if (personData[ship->persType].flags & kPersDeathSpawn) {
                    if (ship->govt == -1)
                        ok = 1;
                    else if (!(govtData[ship->govt].flags & kGovtNoDeathSpawn))
                        ok = 1;
                    else
                        ok = 0;
                } else
                    ok = 0;
                if (ok) {
                    LaunchEscapePod(ship);
                    PlaySoundDistance(-1, escapePodSnd, 5,
                                 &ship->pos,
                                 &shipData->pos);
                }
            }
        }
        if (ship->escapePods > 0
                && ship->deathTimer <= gShipTypes[ship->type].deathDelay * 0.5
                && gShipTypes[ship->type].podCount > 0) {
            n = (gShipTypes[ship->type].deathDelay
                    / gShipTypes[ship->type].podCount * 0.4);
            if (n < 10)
                n = 10;
            if (gTimeScale > 0.0)
                n = (n / gTimeScale);
            if (frameCounter % n == 0
                    || ship->escapePods == gShipTypes[ship->type].podCount) {
                ship->escapePods -= 1;
                LaunchEscapePod(ship);
                PlaySoundDistance(-1, escapePodSnd, 5, &ship->pos,
                             &shipData->pos);
            }
        }
    }

    if (ShipFuelGenRate(ship, &d))
        ship->fuel += d * gTimeScale;
    n = ShipFuelCapacity(ship);
    {
        float cap;

        cap = n;
        if (ship->fuel > cap)
            ship->fuel = cap;
    }
    if (ship->fuel < 0.0f)
        ship->fuel = 0;
    if (ship->annoyance > 0.0f) {
        ship->annoyance -= gTimeScale * 0.5;
        if (ship->annoyance < 0.0f)
            ship->annoyance = 0;
    }
    if (ship->tractorShip != -1) {
        if (ship->tractorShip != ship->index) {
            if (!shipData[ship->tractorShip].active
                    || IsDisabled(&shipData[ship->tractorShip])
                    || IsDying(&shipData[ship->tractorShip])) {
                ship->tractorShip = -1;
                ship->tractorTick = 0;
            }
        }
        if (ReadTick() >= ship->tractorTick + 0x1e)
            ship->tractorShip = -1;
    }
    if (ShipIsDerelict(ship))
        ship->engineGlow = 0;

    if (IsDying(ship) && HasEscapeShip(ship)) {
        if (ship->deathTimer <= gShipTypes[ship->type].deathDelay * 0.5) {
            if (Rand(3) == 0) {
                j = LaunchFighter(ship, EscapeShipWeapType(ship));
                if (j != -1) {
                    if (shipData[j].shield > 1.0f)
                        shipData[j].shield =
                            Rand(shipData[j].shield);
                    shipData[j].vel.x *= 0.5;
                    shipData[j].vel.y *= 0.5;
                }
            }
            for (i = 0; i < kMaxWeapTypes; i++) {
                ship->weap[i].count = 0;
                ship->weap[i].ammo = 0;
            }
        }
    }
}

#define kMurkMax                31
#define kMurkMin                4
#define kMinHiliteDepth         16      /* hilites only at 16-bit and deeper */
#define kAngleWrap              360
#define kShotDeadLife           -2.0f
#define kShotSpentLife          -1.0f
#define kShotKilledLife         -32000.0f
#define kProximityFuseFrames    30.0    /* gTimeScale units before a fuse arms */
#define kShotSpentCode          0x3e6


#define kWeapFlagAnimated       0x1     /* frame index comes from the timer */
#define kWeapFlagSmokeA         0x200
#define kWeapFlagSmokeB         0x400
#define kWeapFlagSmokeC         0x800
#define kWeapFlagBlast          0x8000

#define kWeap22FrameByRange     0x1
#define kWeap22HoldLastFrame    0x2
#define kWeap22NoSubmunitions   0x20
#define kWeap22NoBlastOnShips   0x400

/* the shot's sprite; only the fields this TU touches */

/* one live shot, stride 0x48 in shotData */

/* HandleShot (mac _HandleShot). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00435830
void HandleShot(ShotObj *shot)
{
    FloatPoint pt;
    float f;
    short frame;
    short half;              /* re-used as the smoke type below */
    short i;
    char armed;
    char noSub;
    char hit;

    noSub = 0;
    if (shot->curSystem == shipData->curSystem && shot->life > 0.0f) {
        if (shot->owner < -1 || shot->owner >= kMaxShips)
            shot->owner = -1;
        if (shot->weapType < 0 || shot->weapType >= kMaxWeapTypes) {
            shot->life = kShotSpentLife;
            return;
        }
        if (gWeapons[shot->weapType].guidance == kGuidancePointDef)
            shot->target = -1;
        else if (shot->target < 0 || shot->target >= kMaxShips)
            shot->target = -1;
        else if (shot->guidanceState == 0 && shipData[shot->target].active == 0) {
            shot->guidanceState = kShotSpentCode;
            shot->target = -1;
        }

        armed = 0;
        if (gWeapons[shot->weapType].guidance == kGuidanceHoming
                && shot->owner != -1
                && shot->target > 0
                && shot->guidanceState == 0
                && gWeapons[shot->weapType].lifetime - shot->life
                   > gTimeScale * kProximityFuseFrames)
            armed = 1;

        if (gameFrozen == 0)
            shot->life -= gTimeScale;
        if (killShots != 0)
            shot->life = kShotKilledLife;
        else if (shot->life <= 0.0f)
            shot->life = kShotSpentLife;
        if (gameFrozen == 0) {
            HandleShotGuidance(shot);
            shot->pos.x += shot->vel.x * gTimeScale;
            shot->pos.y += shot->vel.y * gTimeScale;
        }

        if (gameFrozen == 0 && armed == 0 && shot->owner != -1
                && gWeapons[shot->weapType].guidance == kGuidanceHoming
                && shot->target > 0
                && shot->guidanceState == 0
                && shipData[shot->target].parentShip != 0
                && shot->owner <= 0
                && shot->owner < kMaxShips
                && gWeapons[shot->weapType].lifetime - shot->life
                   > gTimeScale * kProximityFuseFrames
                && ShipsShareParent(&shipData[shot->target],
                                    &shipData[shot->owner]) == 0)
            DamageShip(&shipData[shot->target],
                       &shipData[shot->target].pos,
                       0, kShotKilledLife, kShotKilledLife, shot->owner,
                       1, 1, 1, 1, 0,
                       gWeapons[shot->weapType].reload);

        if ((char)(gWeapons[shot->weapType].flags & kWeapFlagAnimated) != 0) {
            frame = shot->frame;
            shot->frameTimer += gTimeScale;
            if (gWeapons[shot->weapType].beamWidth <= shot->frameTimer
                    || gWeapons[shot->weapType].beamWidth <= 0) {
                shot->frame++;
                shot->frameTimer = 0.0f;
            }
            if (shot->frame >= shot->sprite->numFrames) {
                if ((gWeapons[shot->weapType].flags2 & kWeap22HoldLastFrame) != 0)
                    shot->frame = shot->sprite->numFrames - 1;
                else
                    shot->frame = 0;
            }
            if ((gWeapons[shot->weapType].flags2 & kWeap22FrameByRange) != 0
                    && gWeapons[shot->weapType].lifetime
                       - gWeapons[shot->weapType].proxSafety < shot->life) {
                frame = 0;
                if ((gWeapons[shot->weapType].flags2 & kWeap22HoldLastFrame) != 0)
                    shot->frame = 0;
            }
        } else {
            frame = (shot->sprite->numFrames * shot->angle / 360.0);
        }
        if (frame >= shot->sprite->numFrames)
            frame = shot->sprite->numFrames - 1;
        if (frame < 0)
            frame = 0;

        if (killShots != 0)
            SWSetSpriteVisible(shot->sprite, 0);
        else
            SWSetSpriteVisible(shot->sprite, 1);
        SWSetCurrentFrameIndex(shot->sprite, frame);
        shot->sprite->translucency = 0;
        shot->sprite->tintRed = 0;
        shot->sprite->tintGreen = 0;
        shot->sprite->tintBlue = 0;
        shot->sprite->tintMode = 0;

        if (gWeapons[shot->weapType].decayShots < 0 && shot->murk < 32.0f) {
            if ((gWeapons[shot->weapType].flags3 & kWeapTranslucent) != 0
                    && gSpriteWorld->pixelDepth >= kMinHiliteDepth) {
                shot->sprite->translucency = kMurkFull;
                shot->sprite->tintRed = shot->sprite->tintGreen =
                    shot->sprite->tintBlue = shot->murk;
            } else {
                shot->sprite->translucency = (32.0f - shot->murk);
                shot->murk -= gWeapons[shot->weapType].decayShots * gTimeScale;
            }
        }
        if (gWeapons[shot->weapType].decayShots > 0 && shot->life < 32.0f) {
            if ((gWeapons[shot->weapType].flags3 & kWeapTranslucent) != 0
                    && gSpriteWorld->pixelDepth >= kMinHiliteDepth) {
                shot->sprite->translucency = kMurkFull;
                shot->sprite->tintRed = (32.0f - shot->murk);
                if (shot->sprite->tintRed < kMurkMin)
                    shot->sprite->tintRed = kMurkMin;
                shot->sprite->tintGreen = shot->sprite->tintBlue =
                    shot->sprite->tintRed;
            } else {
                shot->sprite->translucency = shot->murk;
                if (shot->sprite->translucency > kMurkMax)
                    shot->sprite->translucency = kMurkMax;
            }
            if (shot->murk < 32.0f)
                shot->murk += gWeapons[shot->weapType].decayShots * gTimeScale;
        }
        if ((gWeapons[shot->weapType].flags3 & kWeapTranslucent) != 0
                && gWeapons[shot->weapType].decayShots == 0
                && gSpriteWorld->pixelDepth >= kMinHiliteDepth) {
            shot->sprite->translucency = kMurkFull;
            shot->sprite->tintRed = kMurkFull;
            shot->sprite->tintGreen = kMurkFull;
            shot->sprite->tintBlue = kMurkFull;
        }
        if ((gWeapons[shot->weapType].flags3 & kWeapOneShotAtATime) != 0
                && shot->owner != -1)
            shipData[shot->owner].weap[shot->weapType].reloadTimer =
                gWeapons[shot->weapType].reload;

        HandleSpriteMurkiness(shot->pos.x, shot->pos.y, shot->sprite);
        half = GetSpriteXSize(shot->sprite) / 2;
        SWSetSpriteLocation(shot->sprite,
                            (short)(shot->pos.x - shipData->pos.x)
                            + screenCenter.h - half,
                            (short)(shot->pos.y - shipData->pos.y)
                            + screenCenter.v - half);

        if (updateSmokeThisFrame != 0 && gameFrozen == 0) {
            half = -1;
            if ((gWeapons[shot->weapType].flags & kWeapFlagSmokeA) != 0)
                half = 1;
            else if ((gWeapons[shot->weapType].flags & kWeapFlagSmokeB) != 0)
                half = 0;
            if (half != -1) {
                if ((gWeapons[shot->weapType].flags & kWeapFlagSmokeC) != 0)
                    half += 2;
                SpawnSmoke(shot->pos.x, shot->pos.y, half,
                             gWeapons[shot->weapType].smokeSet);
            }
        }

        if (gWeapons[shot->weapType].particles > 0 && gNoDebris == 0
                && gameFrozen == 0) {
            pt.x = shot->pos.x;
            pt.y = shot->pos.y;
            Accel((short)(shot->angle + kHalfTurn) % kAngleWrap,
                  GetSpriteXSize(shot->sprite) / 2, &pt);
            SpawnParticles(pt.x, pt.y,
                         gWeapons[shot->weapType].partVel[Rand(8)], 0,
                         gWeapons[shot->weapType].partLifeMin,
                         gWeapons[shot->weapType].partLifeMax,
                         gWeapons[shot->weapType].partColor[Rand(8)],
                         kMurkFull - shot->sprite->translucency,
                         gWeapons[shot->weapType].particles, 0);
        }

        if (gWeapons[shot->weapType].decay > 0) {
            shot->trailTimer += gTimeScale;
            if (gWeapons[shot->weapType].decay < shot->trailTimer) {
                shot->trailTimer = 0.0f;
                shot->trailCount++;
            }
        }
    } else {
        if (shot->curSystem == shipData->curSystem
                && shot->life > kShotKilledLife
                && shot->sprite->isVisible != 0) {
            if ((gWeapons[shot->weapType].flags2 & kWeap22NoSubmunitions) != 0)
                noSub = 1;
            if (gWeapons[shot->weapType].subCount > 0 && noSub == 0)
                SpawnShotSubmunitions(shot, shot->target);
            if ((gWeapons[shot->weapType].flags & kWeapFlagBlast) != 0) {
                CreateExplosion(shot->pos,
                                gWeapons[shot->weapType].explodType,
                                gWeapons[shot->weapType].blastRadius, 1);
                if (gWeapons[shot->weapType].blastRadius > 0
                        && (gWeapons[shot->weapType].flags2
                            & kWeap22NoBlastOnShips) == 0) {
                    for (i = 0; i < kMaxShips; i++) {
                        hit = 1;
                        if (i == shot->owner
                                && ((gWeapons[shot->weapType].flags
                                     & kWeapFlagHitsOwner) != 0
                                    || shot->owner != 0))
                            hit = 0;
                        if (shipData[i].active != 0 && hit != 0
                                && (gShipTypes[shipData[i].type].flags
                                    & kShipFlagNoMove) == 0) {
                            f = FloatAbs(shipData[i].pos.x - shot->pos.x);
                            pt.x = f;
                            f = FloatAbs(shipData[i].pos.y - shot->pos.y);
                            pt.y = f;
                            if (gWeapons[shot->weapType].blastRadius >= pt.x
                                    && gWeapons[shot->weapType].blastRadius >= pt.y) {
                                DamageShip(&shipData[i], &shot->pos,
                                           gWeapons[shot->weapType].impact,
                                           gWeapons[shot->weapType].massDmg,
                                           gWeapons[shot->weapType].energyDmg,
                                           shot->owner, 0, 0, shot->disableOnly, 0,
                                           (gWeapons[shot->weapType].flags
                                                           & kWeapFlagIonizing),
                                           gWeapons[shot->weapType].reload);
                                IonizeShip(&shipData[i], &gWeapons[shot->weapType],
                                           &shot->pos);
                            }
                        }
                    }
                }
            } else if (gWeapons[shot->weapType].explodType >= 1) {
                SpawnExplod(shot->pos.x, shot->pos.y, 0, 0, 0);
            }
        }
        shot->life = kShotDeadLife;
        SWSetSpriteVisible(shot->sprite, 0);
    }
}

#define kMinSpriteSpan          8
#define kWrapMargin             32
#define kDeadStrength           (-32000)


/* HandleAsteroids (mac _HandleAsteroids). STATE: MATCH 100.0%, exe 442 / obj 442 instrs,
   byte-identical yes. OPEN: nothing; the eight screenCenter reads are plain .h/.v and the
   X/Y load asymmetry is the compiler's (notes 2026-09-16). */
// FUNCTION: EVNOVA 0x00436910
void HandleAsteroids(void)
{
    short i;
    short spriteW;
    short maxW;
    short maxH;
    short spriteH;
    char wrapped;

    if (miningShipsActive != 0 && asteroidData[0].active == 0) {
        for (i = 1; i < kMaxAsteroids; i++) {
            if (asteroidData[i].active != 0) {
                asteroidData[0].pos.x = asteroidData[i].pos.x;
                asteroidData[0].pos.y = asteroidData[i].pos.y;
                asteroidData[0].vel.x = asteroidData[i].vel.x;
                asteroidData[0].vel.y = asteroidData[i].vel.y;
                asteroidData[0].type = asteroidData[i].type;
                asteroidData[0].frame = asteroidData[i].frame;
                asteroidData[0].frameRate = asteroidData[i].frameRate;
                asteroidData[0].strength = asteroidData[i].strength;
                asteroidData[0].active = 1;
                asteroidData[i].active = 0;
                break;
            }
        }
    }

    maxW = maxH = kMinSpriteSpan;
    for (i = 0; i < kMaxAsteroids; i++) {
        if (masterAsteroidSprite[i] != NULL) {
            spriteW = GetSpriteXSize(masterAsteroidSprite[i]);
            if (spriteW > maxW)
                maxW = spriteW;
            spriteH = GetSpriteYSize(masterAsteroidSprite[i]);
            if (spriteH > maxH)
                maxH = spriteH;
        }
    }
    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].active == 0 || asteroidData[i].strength <= kDeadStrength
                || killAsteroids != 0) {
            asteroidData[i].active = 0;
            SWSetSpriteVisible(asteroidData[i].sprite, 0);
        } else {
            if (gameFrozen == 0) {
                asteroidData[i].pos.x += asteroidData[i].vel.x * gTimeScale;
                asteroidData[i].pos.y += asteroidData[i].vel.y * gTimeScale;
                asteroidData[i].frame += asteroidData[i].frameRate * gTimeScale;
            }
            SWSetSpriteVisible(asteroidData[i].sprite, 1);
            SWCloneFramesFromMaster(asteroidData[i].sprite,
                         masterAsteroidSprite[asteroidData[i].type]);
            if (asteroidData[i].frame < 0.0f)
                asteroidData[i].frame = asteroidData[i].sprite->numFrames
                                        + asteroidData[i].frame;
            if (asteroidData[i].sprite->numFrames <= asteroidData[i].frame)
                asteroidData[i].frame = asteroidData[i].frame
                                        - asteroidData[i].sprite->numFrames;
            SWSetCurrentFrameIndex(asteroidData[i].sprite, asteroidData[i].frame);
            asteroidData[i].sprite->translucency = 0;
            HandleSpriteMurkiness(asteroidData[i].pos.x,
                                  asteroidData[i].pos.y,
                                  asteroidData[i].sprite);
            spriteW = GetSpriteXSize(asteroidData[i].sprite);
            spriteH = GetSpriteYSize(asteroidData[i].sprite);
            SWSetSpriteLocation(asteroidData[i].sprite,
                         ((short)(asteroidData[i].pos.x - shipData->pos.x)
                                 + screenCenter.h - spriteW / 2),
                         ((short)(asteroidData[i].pos.y - shipData->pos.y)
                                 + screenCenter.v - spriteH / 2));
            wrapped = 0;
            if (asteroidData[i].sprite->destFrameRect.left > screenCenter.h * 2 + kWrapMargin) {
                float newPos = shipData->pos.x - screenCenter.h;
                asteroidData[i].pos.x = newPos - maxW * 2;
                wrapped = 1;
            }
            if (asteroidData[i].sprite->destFrameRect.left < -spriteW * 2 - kWrapMargin) {
                float newPos = shipData->pos.x + screenCenter.h;
                asteroidData[i].pos.x = newPos + maxW * 2;
                wrapped = 1;
            }
            if (asteroidData[i].sprite->destFrameRect.top > screenCenter.v * 2 + kWrapMargin) {
                float newPos = shipData->pos.y - screenCenter.v;
                asteroidData[i].pos.y = newPos - maxH * 2;
                wrapped = 1;
            }
            if (asteroidData[i].sprite->destFrameRect.top < -spriteH * 2 - kWrapMargin) {
                float newPos = shipData->pos.y + screenCenter.v;
                asteroidData[i].pos.y = newPos + maxH * 2;
                wrapped = 1;
            }
            if (wrapped) {
                SWSetSpriteVisible(asteroidData[i].sprite, 0);
                asteroidData[i].active = 0;
            }
        }
    }
}

/* A SpriteWorld CollideProcPtr: sectRect is the intersection the layer walker
   hands every collide proc, and this one has no use for it. */

/* AsteroidCollisionHandler. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00436f70
void AsteroidCollisionHandler(SpriteRec *roidSpr, SpriteRec *otherSpr, Rect *sectRect)
{
    ShotObj *shot;

    shot = &shotData[otherSpr->userData];
    if ((gWeapons[shot->weapType].seeker & kWeapSeekPassesAsteroids) != 0)
        return;
    if (!SWRLECollision(roidSpr, otherSpr))
        return;
    HandleAsteroidHit(shot, &asteroidData[roidSpr->userData]);
}

#define kWeapNoBlastOnShips     0x400   /* WeapRecord flags2 bit 10 (name TBC) */
#define kRockDamageScale        10
#define kRockSpeedLimit         2.0f

/* one live shot, stride 0x48 in shotData; only the fields this TU touches */

/* the shot's sprite; only the byte this TU sets */

/* loaddata.c's rock record, stride 0x1c; only the mass this TU divides by */

/* HandleAsteroidHit. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00436ff0
void HandleAsteroidHit(ShotObj *shot, AsteroidObj *roid)
{
    /* loose floats here, not game.h's FloatPoint: this body passes x and y
       separately and mwcc refuses the conversion */
    float dx;
    float dy;
    short i;
    char inRange;

    CreateExplosion(shot->pos,
                    gWeapons[shot->weapType].explodType,
                    gWeapons[shot->weapType].blastRadius, 1);
    if (gWeapons[shot->weapType].hitParticles > 0)
        SpawnParticles(shot->pos.x, shot->pos.y,
                     gWeapons[shot->weapType].hitPartVel, kParticleSpread,
                     gWeapons[shot->weapType].hitPartLife,
                     (gWeapons[shot->weapType].hitPartLife
                                   * kParticleLifeScale),
                     gWeapons[shot->weapType].hitPartColor, kParticleColor,
                     gWeapons[shot->weapType].hitParticles, 0);
    if (shot->owner == 0 && gWeapons[shot->weapType].blastRadius > 0
            && (gWeapons[shot->weapType].flags2 & kWeapNoBlastOnShips) == 0) {
        for (i = 0; i < kMaxShips; i++) {
            inRange = 1;
            if (i == shot->owner
                    && ((gWeapons[shot->weapType].flags & kWeapFlagHitsOwner) != 0
                        || shot->owner != 0))
                inRange = 0;
            if (shipData[i].active != 0 && inRange != 0
                    && (gShipTypes[shipData[i].type].flags & kShipFlagNoMove) == 0) {
                dx = FloatAbs(shipData[i].pos.x - shot->pos.x);
                dy = FloatAbs(shipData[i].pos.y - shot->pos.y);
                if (gWeapons[shot->weapType].blastRadius >= dx
                        && gWeapons[shot->weapType].blastRadius >= dy) {
                    DamageShip(&shipData[i], &shot->pos,
                               gWeapons[shot->weapType].impact,
                               gWeapons[shot->weapType].massDmg,
                               gWeapons[shot->weapType].energyDmg,
                               shot->owner, 0, 0, shot->disableOnly, 0,
                               (gWeapons[shot->weapType].flags & kWeapFlagIonizing),
                               gWeapons[shot->weapType].reload);
                    IonizeShip(&shipData[i], &gWeapons[shot->weapType],
                               &shot->pos);
                }
            }
        }
    }

    if ((gWeapons[shot->weapType].flags2 & kWeapBigRockDamage) != 0)
        roid->strength -= gWeapons[shot->weapType].energyDmg * kRockDamageScale;
    else
        roid->strength -= gWeapons[shot->weapType].energyDmg;
    if (roid->strength < 0) {
        DestroyAsteroid(roid);
    } else if (gWeapons[shot->weapType].impact != 0
               && asteroidType[roid->type].mass > 0) {
        AdjustedAccel(shot->angle,
                      gWeapons[shot->weapType].impact
                      / (float)asteroidType[roid->type].mass,
                      &roid->vel, kRockSpeedLimit);
        SpeedLimit(&roid->vel, kRockSpeedLimit);
    }
    shot->life = -1.0f;
    SWSetSpriteVisible(shot->sprite, 0);
    ((SpriteRec *)shot->sprite)->needsToBeErased = 1;
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* shipcollide -- the SpriteWorld collision callback for a ship sprite (win 0x004374f0, mac _ShipCollisionHandler). Notes: Research/notes/shipcollide.md */
/* A box is picked up into shipData[ship->unk86], i.e. the OWNER's hold rather
   than the toucher's, with cargo 0..5 going to the six commodity counts and
   1000..1127 to the junk records; unk86 == 0 means the PLAYER owns the pickup.
   gTimeScale is loaded here as a QWORD, so it is a double; another TU declares
   it `long' -- a per-file window, not a disagreement to fix.
   A SpriteWorld CollideProcPtr: sectRect is the intersection the layer walker
   hands every collide proc, and this one has no use for it. */

#define kJunkBase   1000    /* box cargoType of jünk n is kJunkBase + n */

#define kBigSpriteWidth 0x20
#define kCloseZoom      2.0

/* ShipCollisionHandler. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004374f0
void ShipCollisionHandler(SpriteRec *shipSpr, SpriteRec *otherSpr, Rect *sectRect)
{
    ShipObj *ship;
    ShotObj *shot;
    BoxObj *box;
    long index;

    index = shipSpr->userData;
    if (index < 0 || index >= kMaxShips)
        return;
    ship = &shipData[index];
    if (otherSpr->parentSpriteLayerP == shotLayer || otherSpr->parentSpriteLayerP == guidedShotLayer
        || otherSpr->parentSpriteLayerP == turretedShotLayer) {
        index = otherSpr->userData;
        if (index < 0 || index >= kMaxShots)
            return;
        shot = &shotData[index];
        if (!ShotCanHitShip(shot, ship))
            return;
        if (gTimeScale >= kCloseZoom
            || GetSpriteXSize(shipSpr) <= kBigSpriteWidth) {
            if (!SWRadiusCollision(shipSpr, otherSpr))
                return;
        } else {
            if (!SWRLECollision(shipSpr, otherSpr))
                return;
        }
        if (gWeapons[shot->weapType].proxSafety > 0
            && (gWeapons[shot->weapType].lifetime
                       - gWeapons[shot->weapType].proxSafety) < shot->life)
            return;
        HandleShipHit(shot, ship, 0);
    } else if (otherSpr->parentSpriteLayerP == boxLayer) {
        if ((ship->index == 0 && shipData[0].canScoop != 0)
            || (ship->index != 0 && AIShipIsScooping(ship))) {
            index = otherSpr->userData;
            if (index < 0 || index >= kMaxBoxes)
                return;
            box = &boxData[index];
            if (box->mining != 0 && SWRLECollision(shipSpr, otherSpr)) {
                box->timer = -1.0f;
                SWSetSpriteVisible(otherSpr, 0);
                if (box->cargoType >= 0
                    && box->cargoType <= kCargoTypes - 1)
                    shipData[ship->index].cargo[box->cargoType]++;
                else if (ship->index == 0 && box->cargoType >= kJunkBase
                         && box->cargoType < kJunkBase + kMaxJunks)
                    junkData[box->cargoType - kJunkBase].held++;
                if (ship->index == 0) {
                    statusRedraw[2] = 1;
                    ResetPlayerPrecalcedValues();
                }
            }
        }
    }
}

#define kEscortSpeaksAI         3       /* aiType >= 3 will call the kill in */
#define kVoiceShipKilled        2       /* _SpeakVoiceMessage's first argument */

/* one live shot, stride 0x48 in shotData; only the fields this TU touches */

/* the shot's sprite; only the two fields this TU touches */

/* HandleShipHit. STATE: MATCH, byte-identical. OPEN: none.
   Was DIFF 98.3% and recorded CLOSED-as-PREFERENCE; that verdict was wrong (338).
   Passing shot->pos whole, against game.h's FloatPoint row, closed all 8 rows. */
// FUNCTION: EVNOVA 0x00437780
void HandleShipHit(ShotObj *shot, ShipObj *ship, char spawnSubs)
{
    float dx;
    float dy;
    short i;
    short armorDmg;
    short shieldDmg;
    short owner;
    char wasDying;
    char inRange;

    if (ship->type < 0 || ship->type >= kMaxShipTypes)
        return;
    if (shot->weapType < 0 || shot->weapType >= kMaxWeapTypes)
        return;

    CreateExplosion(shot->pos,
                    gWeapons[shot->weapType].explodType,
                    gWeapons[shot->weapType].blastRadius, 1);
    armorDmg = gWeapons[shot->weapType].massDmg;
    shieldDmg = gWeapons[shot->weapType].energyDmg;
    wasDying = IsDying(ship);
    if (shot->trailCount > 0) {
        armorDmg -= shot->trailCount;
        shieldDmg -= shot->trailCount;
        if (armorDmg < 0)
            armorDmg = 0;
        if (shieldDmg < 0)
            shieldDmg = 0;
    }
    IonizeShip(ship, &gWeapons[shot->weapType], 0);
    if (gWeapons[shot->weapType].hitParticles > 0)
        SpawnParticles(shot->pos.x, shot->pos.y,
                     gWeapons[shot->weapType].hitPartVel, kParticleSpread,
                     gWeapons[shot->weapType].hitPartLife,
                     (gWeapons[shot->weapType].hitPartLife
                                   * kParticleLifeScale),
                     gWeapons[shot->weapType].hitPartColor, kParticleColor,
                     gWeapons[shot->weapType].hitParticles, 0);
    if (shot->target == ship->index)
        DamageShip(ship, &shot->pos,
                   gWeapons[shot->weapType].impact, armorDmg, shieldDmg,
                   shot->owner, 1, 1, shot->disableOnly, 0,
                   (gWeapons[shot->weapType].flags & kWeapFlagIonizing),
                   gWeapons[shot->weapType].reload);
    else
        DamageShip(ship, &shot->pos,
                   gWeapons[shot->weapType].impact, armorDmg, shieldDmg,
                   shot->owner, 1, 0, shot->disableOnly, 0,
                   (gWeapons[shot->weapType].flags & kWeapFlagIonizing),
                   gWeapons[shot->weapType].reload);

    if (IsDying(ship) && wasDying == 0) {
        for (i = 1; i < kMaxShips; i++) {
            if (ship->index != i && shipData[i].active != 0
                    && shipData[i].parentShip == 0
                    && ship->index == shipData[i].targetShip
                    && shipData[i].aiType >= kEscortSpeaksAI
                    && IsShipAttacking(&shipData[i])
                    && shipData[i].type >= 0 && shipData[i].type < kMaxShipTypes
                    && (gShipTypes[shipData[i].type].flags2 & kShipFlagNoVoice) == 0)
                SpeakVoiceMessage(kVoiceShipKilled,
                             gShipTypes[shipData[i].type].inherentGovtB,
                             shipData[i].voiceClass);
        }
    }

    if (gWeapons[shot->weapType].blastRadius > 0) {
        for (i = 0; i < kMaxShips; i++) {
            owner = shot->owner;
            inRange = 1;
            if (i == owner
                    && ((gWeapons[shot->weapType].flags & kWeapFlagHitsOwner) != 0
                        || owner != 0))
                inRange = 0;
            if (i != ship->index && shipData[i].active != 0 && inRange != 0
                    && shipData[i].type >= 0 && shipData[i].type < kMaxShipTypes) {
                dx = FloatAbs(shipData[i].pos.x - shot->pos.x);
                dy = FloatAbs(shipData[i].pos.y - shot->pos.y);
                if (gWeapons[shot->weapType].blastRadius >= dx
                        && gWeapons[shot->weapType].blastRadius >= dy) {
                    DamageShip(&shipData[i], &shot->pos,
                               gWeapons[shot->weapType].impact,
                               gWeapons[shot->weapType].massDmg,
                               gWeapons[shot->weapType].energyDmg,
                               owner, 0, 0, shot->disableOnly, 0,
                               (gWeapons[shot->weapType].flags
                                               & kWeapFlagIonizing),
                               0);
                    IonizeShip(&shipData[i], &gWeapons[shot->weapType],
                               &shot->pos);
                }
            }
        }
    }

    if (gWeapons[shot->weapType].subCount > 0 && spawnSubs != 0)
        SpawnShotSubmunitions(shot, ship->index);
    shot->life = -1.0f;
    if (shot->sprite != NULL && shot->sprite->curFrameP != 0) {
        SWSetSpriteVisible(shot->sprite, 0);
        shot->sprite->needsToBeErased = 1;
    }
}

#define kStellarPenaltyCount    10
#define kWeapHitsStellars       0x400  /* WeapRecord flags2 bit 10 (name TBC) */
#define kWeapProxAlwaysArmed    0x1    /* WeapRecord seeker bit 0 (name TBC) */
#define kProxSpriteFraction     0.333

/* CheckShotProximities (mac _CheckShotProximities). STATE: MATCH 558/558, byte-identical,
   at the plain level=4,intrinsics. OPEN: none */
// FUNCTION: EVNOVA 0x00437e20
void CheckShotProximities(void)
{
    long range;
    long dx;
    long dy;
    long ay;
    long ax;
    float f;
    short i;
    short k;
    short j;
    short a;
    short s;
    short stel;
    char hit;

    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life >= 0.0f
                && gWeapons[shotData[i].weapType].lifetime
                   - gWeapons[shotData[i].weapType].proxSafety > shotData[i].life) {
            if ((gWeapons[shotData[i].weapType].flags2 & kWeapHitsStellars) != 0) {
                for (j = 0; j < kNavSlots; j++) {
                    stel = gSystems[shipData->curSystem].navDefault[j];
                    if (stel != -1 && stellarData[stel].strength > 0
                            && !StellarIsDestroyed(&stellarData[stel])
                            && SWRLECollision(shotData[i].sprite,
                                              stellarData[stel].sprite)) {
                        stellarData[stel].deadCounter -=
                            gWeapons[shotData[i].weapType].massDmg
                            + gWeapons[shotData[i].weapType].energyDmg;
                        CreateExplosion(shotData[i].pos,
                                        gWeapons[shotData[i].weapType].explodType,
                                        gWeapons[shotData[i].weapType].blastRadius, 1);
                        if (gWeapons[shotData[i].weapType].hitParticles > 0)
                            SpawnParticles(shotData[i].pos.x,
                                         shotData[i].pos.y,
                                         gWeapons[shotData[i].weapType].hitPartVel,
                                         kParticleSpread,
                                         gWeapons[shotData[i].weapType].hitPartLife,
                                         (gWeapons[shotData[i].weapType].hitPartLife
                                                       * kParticleLifeScale),
                                         gWeapons[shotData[i].weapType].hitPartColor,
                                         kParticleColor,
                                         gWeapons[shotData[i].weapType].hitParticles, 0);
                        shotData[i].life = -1.0f;
                        SWSetSpriteVisible(shotData[i].sprite, 0);
                        shotData[i].sprite->needsToBeErased = 1;
                        if (StellarIsDestroyed(&stellarData[stel])) {
                            if (stellarData[stel].explodType != -1)
                                CreateExplosion(
                                    XYShortToFloat(stellarData[stel].pos),
                                    stellarData[stel].explodType, 0, 1);
                            crashlog_mission_bit_set("destroying stellar",
                                                     (char *)stellarData[stel].name, 1,
                                                     stel + kFirstResourceID,
                                                     stellarData[stel].onDestroy);
                            EvalMissionBitSetString(stellarData[stel].onDestroy);
                            stellarData[stel].deadCounter = -1;
                            stellarData[stel].deadDaysLeft = stellarData[stel].deadDays;
                            if (shotData[i].owner == 0) {
                                for (k = 0; k < kStellarPenaltyCount; k++)
                                    SlapWithPenalty(shipData->curSystem,
                                                    stellarData[stel].govt, 3, -1);
                            }
                        }
                        break;
                    }
                }
            }

            if (shotData[i].life >= 0.0f
                    && gWeapons[shotData[i].weapType].proxRadius > 0) {
                hit = 0;
                if ((gWeapons[shotData[i].weapType].seeker & kWeapProxAlwaysArmed) != 0)
                    hit = 1;
                for (s = 0; s < kMaxShips; s++) {
                    if (shipData[s].active != 0
                            && shipData->curSystem == shipData[s].curSystem
                            && ShotCanHitShip(&shotData[i], &shipData[s])) {
                        range = gWeapons[shotData[i].weapType].proxRadius;
                        range += GetSpriteXSize((SpriteRec *)shipData[s].sprite)
                                 * kProxSpriteFraction;
                        range *= range;
                        dx = abs((long)shipData[s].pos.x - (long)shotData[i].pos.x);
                        dy = abs((long)shipData[s].pos.y - (long)shotData[i].pos.y);
                        if (dx * dx + dy * dy <= range) {
                            HandleShipHit(&shotData[i], &shipData[s], 1);
                            hit = 1;
                            break;
                        }
                    }
                }
                if (hit == 0) {
                    for (a = 0; a < kMaxAsteroids; a++) {
                        if (asteroidData[a].active != 0) {
                            range = gWeapons[shotData[i].weapType].proxRadius;
                            range *= range;
                            f = FloatAbs(asteroidData[a].pos.x - shotData[i].pos.x);
                            ax = f;
                            f = FloatAbs(asteroidData[a].pos.y - shotData[i].pos.y);
                            ay = f;
                            if (ax * ax + ay * ay <= range) {
                                HandleAsteroidHit(&shotData[i], &asteroidData[a]);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

/*
 * All three are referenced only from function-pointer dispatchers
 * (PostDrawCallback/PostEraseCallback), so no direct win caller pins their names, and
 * none has an established mac twin yet.
 */

/* PointDefenseCollisionHandler (mac _PointDefenseCollisionHandler). STATE: MATCH 53/53. SpawnExplod takes the FloatPoint by value; the energy
   subtraction is one expression. OPEN: none */
// FUNCTION: EVNOVA 0x00438630
void PointDefenseCollisionHandler(SpriteRec *shotSpr, SpriteRec *otherSpr, Rect *sectRect)
{
    ShotObj *shot1;
    ShotObj *shot2;
    extern void SpawnExplod(FloatPoint pos, short unused, short type,
                            short angle);

    shot1 = &shotData[shotSpr->userData];
    if (otherSpr->parentSpriteLayerP == guidedShotLayer) {
        shot2 = &shotData[otherSpr->userData];
        shot1->life = 0;
        if (shot2->energy <= 0) {
            shot2->life = 0;
            SWSetSpriteVisible(otherSpr, 0);
            SpawnExplod(shot1->pos, shot1->owner, 0, 0);
        } else {
            shot2->energy -= gWeapons[shot1->weapType].massDmg + gWeapons[shot1->weapType].energyDmg / 2;
        }
        SWSetSpriteVisible(shotSpr, 0);
    }
}

/* BeamEraseCallback (mac _BeamEraseCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004386f0
void BeamEraseCallback(SpriteWorldPtr world)
{
    short count;
    short density;
    Point p1, p2;

    count = 0;
    do {
        if (beamData[count].state >= -1 &&
            beamData[count].owner != -1) {
            p1.h = beamData[count].lastHead.h;
            p1.v = beamData[count].lastHead.v;
            p2.h = beamData[count].lastTail.h;
            p2.v = beamData[count].lastTail.v;
            density = gWeapons[beamData[count].weapType].lightningDensity;
            if (density <= 0) {
                EraseBeam(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP, p1, p2,
                             gWeapons[beamData[count].weapType].beamWidth,
                             gWeapons[beamData[count].weapType].coronaFalloff);
            } else {
                EraseLightningBeam(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP, p1, p2,
                             gWeapons[beamData[count].weapType].beamWidth, density,
                             gWeapons[beamData[count].weapType].lightningAmplitude);
            }
        }
        count++;
    } while (count < kMaxBeams);
}

#define kWeapFlagUnk2000 0x2000  /* gWeapons[].flags2 bit, meaning TBC */

/* BeamDrawCallback (mac _BeamDrawCallback). STATE: MATCH 100.0% (197 exact, 57 reloc-equivalent, 0 mismatched;
   exe 254 / obj 254), relocregcheck 0 of 57, byte identity unmeasured until the next
   regate --update.
   OPEN: none. `density' is named only in loop 2's draw arm; both erase arms read the
   field twice (notes 2026-09-09). */
// FUNCTION: EVNOVA 0x00438810
void BeamDrawCallback(SpriteWorldPtr param_1)
{
    short count;
    short density;
    Point p1, p2;

    if (param_1->fastBlit != 0) {
        count = 0;
        do {
            if (beamData[count].state >= -1 &&
                beamData[count].owner != -1) {
                p1.h = beamData[count].lastHead.h;
                p1.v = beamData[count].lastHead.v;
                p2.h = beamData[count].lastTail.h;
                p2.v = beamData[count].lastTail.v;
                if (gWeapons[beamData[count].weapType].lightningDensity <= 0) {
                    EraseBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                 0x20, 0x10);
                } else {
                    EraseLightningBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                 gWeapons[beamData[count].weapType].beamWidth,
                                 gWeapons[beamData[count].weapType].lightningDensity,
                                 gWeapons[beamData[count].weapType].lightningAmplitude);
                }
            }
            count++;
        } while (count < kMaxBeams);
    }

    count = 0;
    do {
        if (beamData[count].state >= 0 &&
            beamData[count].owner != -1) {
            if ((gWeapons[beamData[count].weapType].flags2 & kWeapFlagUnk2000) != 0) {
                if (param_1->fastBlit != 0) {
                    p1.h = beamData[count].head.h;
                    p1.v = beamData[count].head.v;
                    p2.h = beamData[count].tail.h;
                    p2.v = beamData[count].tail.v;
                    if (gWeapons[beamData[count].weapType].lightningDensity <= 0) {
                        EraseBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                     gWeapons[beamData[count].weapType].beamWidth,
                                     gWeapons[beamData[count].weapType].coronaFalloff);
                    } else {
                        EraseLightningBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                     gWeapons[beamData[count].weapType].beamWidth,
                                     gWeapons[beamData[count].weapType].lightningDensity,
                                     gWeapons[beamData[count].weapType].lightningAmplitude);
                    }
                }
            } else {
                p1.h = beamData[count].head.h;
                p1.v = beamData[count].head.v;
                p2.h = beamData[count].tail.h;
                p2.v = beamData[count].tail.v;
                density = gWeapons[beamData[count].weapType].lightningDensity;
                if (density <= 0) {
                    DrawBeam(gSpriteWorld->workFrameP, p1, p2,
                                 gWeapons[beamData[count].weapType].beamColor,
                                 gWeapons[beamData[count].weapType].beamWidth,
                                 gWeapons[beamData[count].weapType].coronaColor,
                                 gWeapons[beamData[count].weapType].coronaFalloff +
                                     beamData[count].frame);
                    if (param_1->fastBlit != 0) {
                        DrawBeam(gSpriteWorld->windowFrameP, p1, p2,
                                     gWeapons[beamData[count].weapType].beamColor,
                                     gWeapons[beamData[count].weapType].beamWidth,
                                     gWeapons[beamData[count].weapType].coronaColor,
                                     gWeapons[beamData[count].weapType].coronaFalloff +
                                         beamData[count].frame);
                    }
                } else if (param_1->fastBlit != 0) {
                    DrawLightningBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                 gWeapons[beamData[count].weapType].beamColor,
                                 gWeapons[beamData[count].weapType].beamWidth, density,
                                 gWeapons[beamData[count].weapType].lightningAmplitude,
                                 0x20 - beamData[count].frame);
                } else {
                    DrawLightningBeam(gSpriteWorld->workFrameP, 0, p1, p2,
                                 gWeapons[beamData[count].weapType].beamColor,
                                 gWeapons[beamData[count].weapType].beamWidth, density,
                                 gWeapons[beamData[count].weapType].lightningAmplitude,
                                 0x20 - beamData[count].frame);
                }
            }
        }
        count++;
    } while (count < kMaxBeams);
}

/*
 * Same table, same weapon fields as BeamEraseCallback, but gated by gWeapons[].flags2
 * bit 0x2000, drawing from a SECOND coordinate pair (unk0/unk2/unk4/unk6,
 * immediately before x1/y1) with the beam and corona COLOURS added.
 */

/* BeamDrawOffscreenCallback (mac _BeamDrawOffscreenCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00438c40
void BeamDrawOffscreenCallback(SpriteWorldPtr world)
{
    short count;
    short density;
    Point p1, p2;

    count = 0;
    do {
        if (beamData[count].state >= 0 &&
            beamData[count].owner != -1 &&
            (gWeapons[beamData[count].weapType].flags2 & kWeapFlagUnk2000) != 0) {
            p1.h = beamData[count].head.h;
            p1.v = beamData[count].head.v;
            p2.h = beamData[count].tail.h;
            p2.v = beamData[count].tail.v;
            density = gWeapons[beamData[count].weapType].lightningDensity;
            if (density <= 0) {
                DrawBeam(gSpriteWorld->workFrameP, p1, p2,
                             gWeapons[beamData[count].weapType].beamColor,
                             gWeapons[beamData[count].weapType].beamWidth,
                             gWeapons[beamData[count].weapType].coronaColor,
                             gWeapons[beamData[count].weapType].coronaFalloff +
                                 beamData[count].frame);
            } else {
                DrawLightningBeam(gSpriteWorld->workFrameP, 0, p1, p2,
                             gWeapons[beamData[count].weapType].beamColor,
                             gWeapons[beamData[count].weapType].beamWidth,
                             density,
                             gWeapons[beamData[count].weapType].lightningAmplitude,
                             0x20 - beamData[count].frame);
            }
        }
        count++;
    } while (count < kMaxBeams);
}

#define kSoundVolumeScale 1.2e-05

/* HandleSpriteMurkiness. STATE: MATCH 100.0%, exe 110 / obj 110, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00438db0
void HandleSpriteMurkiness(short x, short y, SpriteRec *sprite)
{
    long dx, dy;
    long dist;
    long volume;
    float f;

    f = fabs(shipData->pos.x - x);
    dx = f;
    f = fabs(shipData->pos.y - y);
    dy = f;
    dist = dx * dx + dy * dy;
    volume = dist * gMaxSoundRange * kSoundVolumeScale;
    sprite->tintMode = volume;
    if (gSpriteWorld->pixelDepth == 8) {
        if (sprite->tintMode > 0x18)
            sprite->tintMode = 0x18;
    } else {
        if (sprite->tintMode > 0x1f)
            sprite->tintMode = 0x1f;
    }
    if (sprite->tintMode < 0)
        sprite->tintMode = 0;
    sprite->tintColor = gSystems[shipData->curSystem].mapPixel;
}

/* floatingMapFrame/floatingEscortMenuFrame are new (name invented) - a per-strip
   SWFrame FlushOverlayStrips doesn't need because it blits the whole rect rather
   than drawing text into it. */


/* ---- HandleMouse (0x00438f40) ---- */

#define kCursorFullAlpha    0x20
#define kCursorHiliteFull   0x1c
#define kCursorFadeTicks    0x3c
#define kCursorAnimTicks       3
#define kMinHotRectSize     0x30
#define kHotRectInset      -0x10
#define kStatusBarWidth     0xc2

/* SpriteWorld sprite record - only the fields this function reaches.
   sprites.c's SpriteRec is the same object (+0x1a frame, +0x54 frameCount,
   +0x68 curFrame); the hilite quartet at +0xa2 is mainscreen.c's unkA2..unkA8
   on the same kind of record. */


/*
 * The window rect's RIGHT edge is pulled in by the status panel's width before
 * the scan, and that store is dead on every path - `box` is address-taken,
 * which is what keeps mwcc from deleting it.
 */
/* HandleMouse. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00438f40
void HandleMouse(void)
{
    Rect box;
    char canUntarget;
    char canCloaked;
    Point mouse;
    short frame;
    Rect *port;
    Rect *fr;
    SpriteRec *spr;
    short i;
    short j;
    char hit;

    SWSetPortToWindow(gSpriteWorld);
    GetGlobalMouse(&mouse);
    port = GetCWindowPortRect(gWind);
    box.top = port->top;
    box.left = port->left;
    box.bottom = port->bottom;
    box.right = port->right;
    if (PtInRect(mouse.h, mouse.v, &box))
        HideTheCursor();
    else
        ShowTheCursor();
    box.right -= kStatusBarWidth;

    if (mouse.h != lastMousePos.h || mouse.v != lastMousePos.v || Button()) {
        lastMousePos.h = mouse.h;
        lastMousePos.v = mouse.v;
        lastMouseMoveTime = ReadTick();
        cursorAlpha = kCursorFullAlpha;
    }
    if (cursorAlpha < 0)
        cursorAlpha = 0;
    if (cursorAlpha > kCursorFullAlpha)
        cursorAlpha = kCursorFullAlpha;
    if (cursorSprite != NULL) {
    if (inBackground == 0 && cursorVisible > 0 && cursorAlpha > 0) {

    hit = 0;
    canUntarget = ShipCanTargetUntargetableShips(shipData);
    canCloaked = ShipCanTargetCloakedShips(shipData);
    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].active == 0)
            continue;
        if (IsDying(&shipData[i]))
            continue;
        if (ShipIsCloaked(&shipData[i]) && canCloaked == 0
            && shipData[i].parentShip != 0 && i != 0)
            continue;
        if (shipData->curSystem != shipData[i].curSystem)
            continue;
        if (canUntarget == 0
            && (gShipTypes[shipData[i].type].flags2 & kShipFlag2NoTarget) != 0)
            continue;
        spr = (SpriteRec *)shipData[i].sprite;
        if (spr == NULL)
            continue;
        fr = &spr->destFrameRect;
        box.top = fr->top;
        box.left = fr->left;
        box.bottom = fr->bottom;
        box.right = fr->right;
        if (box.right - box.left < kMinHotRectSize)
            InsetRect(&box, kHotRectInset, kHotRectInset);
        if (PtInRect(mouse.h, mouse.v, &box)) {
            if (shipData->targetShip == -1 && i == 0)
                continue;
            hit = 1;
            break;
        }
    }

    if (hit == 0 && shipData->jumpTimer <= 0.0f) {
        for (j = 0; j < kStellarSprites; j++) {
            spr = stellarSprite[j];
            if (spr == NULL)
                continue;
            if (spr->isVisible == 0)
                continue;
            if (stellarData[(short)spr->userData].loaded == 0)
                continue;
            fr = &spr->destFrameRect;
            box.top = fr->top;
            box.left = fr->left;
            box.bottom = fr->bottom;
            box.right = fr->right;
            if (box.right - box.left < kMinHotRectSize)
                InsetRect(&box, kHotRectInset, kHotRectInset);
            if (PtInRect(mouse.h, mouse.v, &box)) {
                hit = 1;
                break;
            }
        }
    }

    if (hit) {
        frame = cursorSprite->curFrameIndex;
        if (ReadTick() >= lastCursorAnimTime + kCursorAnimTicks) {
            lastCursorAnimTime = ReadTick();
            frame = ((frame + 1) % cursorSprite->numFrames);
        }
    } else {
        frame = 0;
    }

    SWSetSpriteVisible(cursorSprite, 1);
    SWSetCurrentFrameIndex(cursorSprite, frame);
    SWSetSpriteLocation(cursorSprite,
                        (mouse.h - GetSpriteXSize(cursorSprite) / 2),
                        (mouse.v - GetSpriteYSize(cursorSprite) / 2));

    if (gSpriteWorld->pixelDepth == kDepth16) {
        cursorSprite->translucency = kCursorHiliteFull;
        frame = cursorSprite->translucency;
        if (cursorAlpha < frame)
            frame = cursorAlpha;
        cursorSprite->tintRed = frame;
        frame = cursorSprite->translucency;
        if (cursorAlpha < frame)
            frame = cursorAlpha;
        cursorSprite->tintGreen = frame;
        frame = cursorSprite->translucency;
        if (cursorAlpha < frame)
            frame = cursorAlpha;
        cursorSprite->tintBlue = frame;
    } else {
        cursorSprite->translucency = (kCursorFullAlpha - cursorAlpha);
        cursorSprite->tintRed = 0;
        cursorSprite->tintGreen = 0;
        cursorSprite->tintBlue = 0;
    }

    if (ReadTick() > lastMouseMoveTime + kCursorFadeTicks || keyPressedThisFrame != 0)
        cursorAlpha -= 1;
    } else {
        SWSetSpriteVisible(cursorSprite, 0);
    }
    }
}

#define kCronIterateOnActivate 0x0001
#define kCronIterateOnEnd      0x0002
#define kCronIterationLimit    10000
#define kCronRandomRange       101

/* CronEventHandler (mac _CronEventHandler). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439500
void CronEventHandler(void)
{
    short i;

    for (i = 0; i < kMaxCrons; i++) {
        if (gCrons[i].duration < 0)
            continue;
        if (gCrons[i].loaded == 0)
            continue;
        if (gCrons[i].active != 0) {
            if (gCrons[i].holdoffLeft > 0) {
                gCrons[i].holdoffLeft -= 1;
                if (gCrons[i].holdoffLeft > 0)
                    continue;
                if (gCrons[i].daysLeft < 0) {
                } else {
                    ActivateCron(i);
                    if (gCrons[i].daysLeft != 0)
                        continue;
                    TerminateCron(i);
                    continue;
                }
                gCrons[i].active = 0;
                continue;
            }
            gCrons[i].daysLeft -= 1;
            if (gCrons[i].daysLeft > 0)
                continue;
            TerminateCron(i);
            if (gCrons[i].postHoldoff > 0) {
                gCrons[i].holdoffLeft = gCrons[i].preHoldoff;
                continue;
            }
            gCrons[i].active = 0;
            continue;
        }
        if (Rand(kCronRandomRange) > gCrons[i].random)
            continue;
        if (CronDateIsValid(&gCrons[i]) == 0)
            continue;
        if (PlayerMeetsRequirements(gCrons[i].requires[0], gCrons[i].requires[1]) == 0)
            continue;
        if (EvalMissionBitTestString(gCrons[i].enableOn) == 0)
            continue;
        gCrons[i].active = 1;
        gCrons[i].daysLeft = gCrons[i].duration;
        if (gCrons[i].preHoldoff > 0) {
            crashlog_print("preparing to activate cron ");
            crashlog_print_num(i + kFirstResourceID);
            crashlog_print(" in ");
            crashlog_print_num(gCrons[i].preHoldoff);
            crashlog_print(" days\n");
            gCrons[i].holdoffLeft = gCrons[i].preHoldoff;
            continue;
        }
        gCrons[i].holdoffLeft = 0;
        ActivateCron(i);
        if (gCrons[i].duration != 0)
            continue;
        TerminateCron(i);
        gCrons[i].daysLeft = -1;
        if (gCrons[i].postHoldoff > 0)
            gCrons[i].holdoffLeft = gCrons[i].postHoldoff;
    }
}

/* ActivateCron (mac _ActivateCron). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439750
void ActivateCron(short index)
{
    int count;

    count = 0;
    if ((gCrons[index].flags & kCronIterateOnActivate) != 0) {
        for (;;) {
            if (PlayerMeetsRequirements(gCrons[index].requires[0],
                                        gCrons[index].requires[1]) == 0)
                break;
            if (EvalMissionBitTestString(gCrons[index].enableOn) == 0)
                break;
            crashlog_mission_bit_set("iteratively activating cron", 0,
                                     (index + kFirstResourceID), -1,
                                     gCrons[index].onStart);
            EvalMissionBitSetString(gCrons[index].onStart);
            count = count + 1;
            if ((short)count > kCronIterationLimit) {
                crashlog("warning: iterative cron activation is stuck in a loop, bailing out");
                break;
            }
        }
        crashlog_print("cron ");
        crashlog_print_num(index + kFirstResourceID);
        crashlog_print(" iterated ");
        crashlog_print_num((short)count);
        crashlog_print(" time");
        if ((short)count != 1)
            crashlog_print("s");
        crashlog_print(" on activation\n");
    } else {
        crashlog_mission_bit_set("activating cron", 0,
                                 (index + kFirstResourceID), -1,
                                 gCrons[index].onStart);
        EvalMissionBitSetString(gCrons[index].onStart);
    }
}

/* TerminateCron (mac _TerminateCron). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004398b0
void TerminateCron(short index)
{
    int count;

    count = 0;
    if ((gCrons[index].flags & kCronIterateOnEnd) != 0) {
        for (;;) {
            if (PlayerMeetsRequirements(gCrons[index].requires[0],
                                        gCrons[index].requires[1]) == 0)
                break;
            if (EvalMissionBitTestString(gCrons[index].enableOn) == 0)
                break;
            crashlog_mission_bit_set("iteratively terminating cron", 0,
                                     (index + kFirstResourceID), -1,
                                     gCrons[index].onEnd);
            EvalMissionBitSetString(gCrons[index].onEnd);
            count = count + 1;
            if ((short)count > kCronIterationLimit) {
                crashlog("warning: iterative cron termination is stuck in a loop, bailing out");
                break;
            }
        }
        crashlog_print("cron ");
        crashlog_print_num(index + kFirstResourceID);
        crashlog_print(" iterated ");
        crashlog_print_num((short)count);
        crashlog_print(" time");
        if ((short)count != 1)
            crashlog_print("s");
        crashlog_print(" on termination\n");
    } else {
        crashlog_mission_bit_set("terminating cron", 0,
                                 (index + kFirstResourceID), -1,
                                 gCrons[index].onEnd);
        EvalMissionBitSetString(gCrons[index].onEnd);
    }
}

/*
 * The caller passes an argument that this body never reads (the exe is
 * frameless and nothing touches [esp+4]); gameutils.c already declares the
 * one-parameter form, so it is kept. 0x004703e0 is declared here with two
 * SWFrameRec pointers because that is what both of this file's argument slots
 * hold.
 */

/* FloaterEraseCallback (mac _FloaterEraseCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439a10
void FloaterEraseCallback(void *a)
{
    if (commCount >= 0) {
        if (inBackground == 0 && gSpriteWorld->fastBlit != 0 &&
            AreWeFrontProcess() != 0) {
            BlitPixieRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                         &commRect, &commRect);
        } else {
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortBitMapForCopyBits(
                         gSpriteWorld->workFrameP->framePort),
                     &commRect, &commRect, 0, NULL);
        }
    }
    if (showFloatingMap >= 0) {
        if (inBackground == 0 && gSpriteWorld->fastBlit != 0 &&
            AreWeFrontProcess() != 0) {
            BlitPixieRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                         &floatingMapRect, &floatingMapRect);
        } else {
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortBitMapForCopyBits(
                         gSpriteWorld->workFrameP->framePort),
                     &floatingMapRect, &floatingMapRect, 0, NULL);
        }
    }
    if (escortMenuOnscreen >= 0) {
        if (inBackground == 0 && gSpriteWorld->fastBlit != 0 &&
            AreWeFrontProcess() != 0) {
            BlitPixieRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                         &escortMenuRect, &escortMenuRect);
        } else {
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortBitMapForCopyBits(
                         gSpriteWorld->workFrameP->framePort),
                     &escortMenuRect, &escortMenuRect, 0, NULL);
        }
    }
}

/* FloaterDrawCallback (mac _FloaterDrawCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439bb0
void FloaterDrawCallback(SpriteWorldPtr world)
{
    unsigned long tick;
    short blink;

    if (inBackground != 0 || world->fastBlit == 0)
        return;

    if (commCount >= 0) {
        BlitPixieFloatingRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                     &commRect, &commRect, 0, gSpriteWorld->backgroundValue);
    }
    if (showFloatingMap > 0) {
        tick = ReadTick();
        blink = (gJumpStartTick + 0x96) - tick;
        if (blink <= 0)
            blink = 0x20;
        else if (blink < 0x20)
            blink = 0x20 - blink;
        else
            blink = 0;
        BlitPixieFloatingRectDrawProc(floatingMapFrame, gSpriteWorld->workFrameP,
                     &floatingMapFrame->frameRect, &floatingMapRect, blink,
                     gSpriteWorld->backgroundValue);
    }
    if (escortMenuOnscreen > 0) {
        blink = 0x20 - escortMenuOnscreen;
        BlitPixieFloatingRectDrawProc(floatingEscortMenuFrame, gSpriteWorld->workFrameP,
                     &floatingEscortMenuFrame->frameRect, &escortMenuRect,
                     blink, gSpriteWorld->backgroundValue);
        if (gCommDone != 0) {
            DrawEscortMenuInFrame();
            gCommDone = 0;
        }
    }
}

/* PostDrawCallback (mac _PostDrawCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439d00
void PostDrawCallback(SpriteWorldPtr world)
{
    FloaterDrawCallback(world);
    DrawParticlesOffscreen(world);
}

/* PostEraseCallback (mac _PostEraseCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439d20
void PostEraseCallback(SpriteWorldPtr world)
{
    BeamEraseCallback(world);
    FloaterEraseCallback(world);
    EraseParticlesOffscreen(world);
}

/* FlushOverlayStrips. STATE: MATCH, byte-identical.
   OPEN: return type. The exe leaves eax to the last call and never sets it before
   the epilogue, so int against void is undecidable from the body and the one
   caller discards it; render.h:62 pins int, so the body has no return statement.
   see notes */
// FUNCTION: EVNOVA 0x00439d40
int FlushOverlayStrips(void)
{
    int i;
    int passes;

    if (commCount >= 0) {
        if (inBackground == 0 && gSpriteWorld->fastBlit != 0 &&
            AreWeFrontProcess() != 0) {
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &commRect, &commRect);
        } else {
            SWSetPortToWindow(gSpriteWorld);
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(
                         *(CGrafPtr *)gSpriteWorld->windowFrameP),
                     &commRect, &commRect, 0, NULL);
        }
    }
    if (showFloatingMap >= 0) {
        if (inBackground == 0 && gSpriteWorld->fastBlit != 0 &&
            AreWeFrontProcess() != 0) {
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &floatingMapRect, &floatingMapRect);
        } else {
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(
                         *(CGrafPtr *)gSpriteWorld->windowFrameP),
                     &floatingMapRect, &floatingMapRect, 0, NULL);
        }
        if (showFloatingMap == 0)
            showFloatingMap = -1;
    }
    if (escortMenuOnscreen >= 0) {
        if (inBackground == 0 && gSpriteWorld->fastBlit != 0 &&
            AreWeFrontProcess() != 0) {
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &escortMenuRect, &escortMenuRect);
        } else {
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(
                         *(CGrafPtr *)gSpriteWorld->windowFrameP),
                     &escortMenuRect, &escortMenuRect, 0, NULL);
        }
        if (escortMenuOnscreen == 0)
            escortMenuOnscreen = -1;
    }
    UpdateParticlesInWindow(gSpriteWorld);
    i = 0;
    for (;;) {
        if (doubleTime != 0)
            passes = 2;
        else
            passes = 1;
        if ((short)i >= passes)
            break;
        SetParticleScrollOffset((short)(shipData->pos.x - screenCenter.h),
                     (short)(shipData->pos.y - screenCenter.v));
        i = i + 1;
    }
}

#define kReinforceIndexA    0x132
#define kReinforceIndexB    0x133

#define kAnnounceFraction   0.25     /* of reinforceTime, a pooled double */
#define kReinforceReserve       4

/* HandleReinforcements (mac _HandleReinforcements). STATE: MATCH 100.0%, exe 173 / obj 173
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0043a020
void HandleReinforcements(void)
{
    unsigned char govtName[256];
    char msg[256];
    short sys;

    sys = shipData[0].curSystem;
    if (gSystems[sys].reinforceFleet != -1 && gSystems[sys].reinforceTimer > 0.0F) {
        gSystems[sys].reinforceTimer -= gTimeScale;
        if (gSystems[sys].reinforceTimer <= 0.0F) {
            HyperSpawnFleet(sys, gSystems[sys].reinforceFleet, kReinforceReserve, 1);
            gSystems[sys].reinforceTimer = -1.0;
            gSystems[sys].reinforceDaysLeft = gSystems[sys].reinforceInterval;
            if (gSystems[sys].reinforceDaysLeft < 1)
                gSystems[sys].reinforceDaysLeft = 1;
        } else {
            if (gSystems[sys].reinforceTimer
                    < gSystems[sys].reinforceTime * kAnnounceFraction
                && gSystems[sys].reinforceDaysLeft <= 0) {
                if (gGovtNames3[fleetData[gSystems[sys].reinforceFleet].govt][0]
                        != 0) {
                    strcpy(msg, "");
                    AppendIndString(msg, kGameStrList, kReinforceIndexA);
                    strcat(msg, " ");
                    pstrcpy(govtName,
                            gGovtNames3[fleetData[gSystems[sys].reinforceFleet].govt]);
                    PtoCstr(govtName);
                    strcat(msg, (char *)govtName);
                    strcat(msg, " ");
                    AppendIndString(msg, kGameStrList, kReinforceIndexB);
                    CtoPstr(msg);
                    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                    DisplayComm((unsigned char *)msg, kCommWidth, 0, kCommHold,
                                &rgbWhite, 0, 0);
                }
                gSystems[sys].reinforceDaysLeft = gSystems[sys].reinforceInterval;
                if (gSystems[sys].reinforceDaysLeft < 1)
                    gSystems[sys].reinforceDaysLeft = 1;
            }
        }
    }
}

#define kWeapFlagNoPDTarget     0x80    /* WeapRecord flags: immune to point defence (TBC) */
#define kWeapFlagOneSound       0x10    /* WeapRecord flags: never overlap the sound (TBC) */
#define kShipFlagPDTarget       0x8     /* ShipRec flags2 bit 3: point defence may shoot it */
#define kPDRangeSlack           1.5     /* the shot mount reaches 1.5x totalRange */
#define kFuelPerAmmoUnit        0.1     /* a negative ammoType spends fuel instead */

/* one live shot, stride 0x48 in shotData; only the fields this TU touches */

/* HandleShipPointDefense (mac _HandleShipPointDefense). STATE: MATCH 100.0%
   (618 exact, 93 reloc-equivalent, 0 mismatched), exe 711 / obj 711, blind
   711; frame 0x38, 14 slots, 14/14; relocregcheck 93 rows 0 differ;
   byte-identical yes -- every instruction sits at the exe's own offset and
   .text 0xa91 is the exe extent 0x0043a310..0x0043ada1 (read off
   build/pointdefense.s; bytediff's baseline is behind). OPEN: none. */
// FUNCTION: EVNOVA 0x0043a310
void HandleShipPointDefense(ShipObj *ship)
{
    short targetIsShip;
    short angle;
    long dist;
    long bestDist;
    short bestAngle;
    long range;
    short i;
    short weap;
    short target;
    short shot;
    short ammo;
    short slot;
    char fired;

    if (ship->jumpTimer > 0.0f)
        return;

    weap = -1;
    target = -1;
    targetIsShip = -1;
    fired = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if ((gWeapons[i].guidance == kGuidancePointDef
                    || gWeapons[i].guidance == kGuidancePDBeam)
                && ship->weap[i].count > 0
                && ship->weap[i].reloadTimer <= 0.0f
                && ((gWeapons[i].flags2 & kWeapFireWhileCloaked) != 0
                    || !ShipIsCloaked(ship))
                && WeaponHasAmmo(ship, i)) {
            weap = i;
            break;
        }
    }
    if (weap == -1)
        return;

    if (gWeapons[weap].guidance == kGuidancePointDef) {
        range = gWeapons[weap].totalRange;
        range *= kPDRangeSlack;
    } else {
        range = gWeapons[weap].beamLength;
    }
    range = range * range;

    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life > 0.0f
                && gWeapons[shotData[i].weapType].guidance == kGuidanceHoming
                && shotData[i].guidanceState == 0
                && (gWeapons[shotData[i].weapType].flags & kWeapFlagNoPDTarget) == 0
                && (ship->index == shotData[i].target
                    || (ship->parentShip != -1
                        && ship->parentShip == shotData[i].target))) {
            dist = XYFloatSquaredDist(&shotData[i].pos,
                                            &ship->pos);
            if (dist > range)
                continue;
            if (dist >= bestDist && target != -1)
                continue;
            angle = CalcInterceptAngle(&ship->pos,
                                       &shotData[i].pos);
            if (TurretBlindSpot(ship, angle, weap))
                continue;
            bestDist = dist;
            bestAngle = angle;
            target = i;
            targetIsShip = 0;
        }
    }

    if (target == -1) {
        for (i = 0; i < kMaxShips; i++) {
            if (i == ship->index)
                continue;
            if (shipData[i].active == 0)
                continue;
            if ((gShipTypes[shipData[i].type].flags2 & kShipFlagPDTarget) == 0)
                continue;
            if (i == ship->parentShip)
                continue;
            if (IsDisabled(&shipData[i]))
                continue;
            if (!ShipVisibleToShip(&shipData[i], ship))
                continue;
            dist = XYFloatSquaredDist(&shipData[i].pos,
                                            &ship->pos);
            if (dist > range)
                continue;
            if (dist >= bestDist && target != -1)
                continue;
            angle = CalcInterceptAngle(&ship->pos,
                                       &shipData[i].pos);
            if (TurretBlindSpot(ship, angle, weap))
                continue;
            if ((i == 0 && ExtendedIsThreatToPlayer(ship))
                    || (i != 0
                        && (IsShipFightingMe(&shipData[i], ship)
                            || (ship->parentShip != -1
                                && IsShipFightingMe(&shipData[i],
                                                    &shipData[ship->parentShip]))))) {
                bestAngle = angle;
                bestDist = dist;
                target = i;
                targetIsShip = 1;
            }
        }
    }
    if (target == -1)
        return;

    if (gWeapons[weap].guidance == kGuidancePointDef) {
        shot = SpawnShot(ship->index, -1, weap, 0);
        if (shot != -1) {
            shotData[shot].pos.x = ship->pos.x;
            shotData[shot].pos.y = ship->pos.y;
            shotData[shot].vel.x = ship->vel.x;
            shotData[shot].vel.y = ship->vel.y;
            ModifyShotStartPosition(ship, &shotData[shot].pos, weap,
                                    &shotData[target].pos);
            if (targetIsShip == 0)
                shotData[shot].angle =
                    CalcInterceptAngle(&shotData[shot].pos,
                                       &shotData[target].pos);
            else if (targetIsShip == 1)
                shotData[shot].angle =
                    CalcLeadAngle(ship, &shipData[target], weap,
                                 &shotData[shot].pos);
            if (gWeapons[weap].inaccuracy > 0)
                shotData[shot].angle = (Rand(gWeapons[weap].inaccuracy * 2)
                                        - gWeapons[weap].inaccuracy)
                                       + shotData[shot].angle;
            if (shotData[shot].angle < 0.0f)
                shotData[shot].angle = shotData[shot].angle + kFullCircleF;
            if (shotData[shot].angle >= kFullCircleF)
                shotData[shot].angle = shotData[shot].angle - kFullCircleF;
            Accel(shotData[shot].angle, gWeapons[weap].speed100,
                  &shotData[shot].vel);
            fired = 1;
            if (targetIsShip == 1)
                PutSpriteInLayer(shotData[shot].sprite, shotLayer);
        }
    } else {
        if (targetIsShip == 0)
            SpawnBeam(ship->index, target, weap, bestAngle, -1, 1);
        else
            SpawnBeam(ship->index, target, weap, bestAngle, -1, -1);
        fired = 1;
    }

    if (fired != 0) {
        ship->weap[weap].reloadTimer = gWeapons[weap].reload / ship->weap[weap].count
                                 + ship->weap[weap].reloadTimer;
        if (gWeapons[weap].sound >= 0) {
            if ((gWeapons[weap].flags & kWeapFlagOneSound) != 0) {
                if (!ST_IsSoundPlaying(shotSnd[gWeapons[weap].sound]))
                    PlaySoundDistance(-1, shotSnd[gWeapons[weap].sound], 3,
                                    &ship->pos, &shipData->pos);
            } else {
                PlaySoundDistance(-1, shotSnd[gWeapons[weap].sound], 3,
                                &ship->pos, &shipData->pos);
            }
        }

        ammo = gWeapons[weap].ammoType;
        if (ammo != -1) {
            if (ammo >= 0) {
                if ((gWeapons[weap].flags3 & kWeapAmmoFlag) == 0) {
                    if (ship->index == 0) {
                        if (ammo >= 0 && ammo < kMaxWeapTypes)
                            slot = ammo;
                        else
                            slot = -1;
                    } else {
                        slot = weap;
                    }
                    if (slot != -1) {
                        ship->weap[slot].ammo -= 1;
                        if (ship->weap[slot].ammo < 0)
                            ship->weap[slot].ammo = 0;
                    }
                    statusRedraw[3] = 1;
                }
            } else if (ammo <= -kWeapAmmoFuelBase) {
                ship->fuel = ship->fuel
                             - (abs(ammo) - kWeapAmmoFuelBase) * kFuelPerAmmoUnit;
                if (ship->fuel < 0.0f)
                    ship->fuel = 0.0f;
                if (ship->index == 0)
                    statusRedraw[4] = 1;
            }
        }

        if (gWeapons[weap].burstCount > 0) {
            ship->weap[weap].burstCount += 1;
            if (ship->weap[weap].burstCount
                    >= WeaponBurstInterval(weap, ship->weap[weap].count)) {
                ship->weap[weap].burstCount = 0;
                ship->weap[weap].reloadTimer = gWeapons[weap].burstReload;
                if ((gWeapons[weap].flags3 & kWeapAmmoFlag) != 0
                        && gWeapons[weap].ammoType >= 0
                        && gWeapons[weap].ammoType < kMaxWeapTypes) {
                    if (ship->index == 0) {
                        ship->weap[gWeapons[weap].ammoType].ammo -= 1;
                        statusRedraw[3] = 1;
                    } else {
                        ship->weap[weap].ammo -= 1;
                    }
                }
            }
        }
    }
}

/* HandleGravity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043adb0
void HandleGravity(void)
{
    short i;
    short j;
    short stellar;
    FloatPoint target;

    for (i = 0; i < kNavSlots; i++) {
        stellar = gSystems[shipData[0].curSystem].navDefault[i];
        if (stellar == -1)
            continue;
        if (stellarData[stellar].gravity == 0)
            continue;
        gravitySourcesPresent = 1;
        target.x = stellarData[stellar].pos.h;
        target.y = stellarData[stellar].pos.v;
        for (j = 0; j < 64; j++) {
            if (shipData[j].active == 0)
                continue;
            if (ShipResistsGravity(&shipData[j]))
                continue;
            ApplyGravity(&shipData[j].pos, &target,
                         stellarData[stellar].gravity * gTimeScale,
                         &shipData[j].vel);
        }
    }
}

#define kSpobFlag2Deadly 0x100  /* name TBC */

/* HandleDeadlyStellars. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043aed0
void HandleDeadlyStellars(void)
{
    short i;
    short j;
    short stellar;

    for (i = 0; i < kNavSlots; i++) {
        stellar = gSystems[shipData[0].curSystem].navDefault[i];
        if (stellar == -1)
            continue;
        if (*(long *)&stellarData[stellar] == 0)
            continue;
        if ((stellarData[stellar].flags2 & kSpobFlag2Deadly) == 0)
            continue;
        for (j = 0; j < 64; j++) {
            if (shipData[j].active == 0)
                continue;
            if (IsDying(&shipData[j]))
                continue;
            if (ShipResistsDeadlyStellars(&shipData[j]))
                continue;
            if (!SWRLECollision(stellarData[stellar].sprite,
                                (SpritePtr)shipData[j].sprite))
                continue;
            shipData[j].active = 0;
            shipData[j].armor = -1000.0f;
            shipData[j].deathTimer = 0;
            if (shipData[0].targetShip == j) {
                shipData[0].targetShip = -1;
                statusRedraw[6] = 1;
            }
            CreateExplosion(shipData[j].pos,
                            gShipTypes[shipData[j].type].explode2, 0, 1);
        }
    }
}

/* mac twin _ShipAccelRate, matched by AdjustInertialessShipVelocity's whole
   call shape (Accel + this + gSpeedMult/gTimeScale clamp), same field
   offsets */

/* stride 0x18, 32 entries */


#define kPodFadeLife    32.0f

/* AdjustInertialessShipVelocity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b020
void AdjustInertialessShipVelocity(ShipObj *ship)
{
    float limit;
    float velX;
    float velY;

    velX = ship->vel.x;
    velY = ship->vel.y;
    ship->vel.x = ship->vel.y = 0.0f;
    Accel(ship->heading, ship->accelMag, &ship->vel);
    limit = ShipAccelRate(ship) * 4.0f * gTimeScale;
    if (velX <= ship->vel.x - limit)
        velX = velX + limit;
    else if (ship->vel.x + limit <= velX)
        velX = velX - limit;
    else
        velX = ship->vel.x;
    if (velY <= ship->vel.y - limit)
        velY = velY + limit;
    else if (ship->vel.y + limit <= velY)
        velY = velY - limit;
    else
        velY = ship->vel.y;
    ship->vel.x = velX;
    ship->vel.y = velY;
}

/* HandleEscapePods. STATE: MATCH, 229/229; byte check pending the next regate (bytediff
   reads the baseline). OPEN: none */
// FUNCTION: EVNOVA 0x0043b170
void HandleEscapePods(void)
{
    short i;
    short xSize;
    short ySize;

    for (i = 0; i < kMaxPods; i++) {
        if (podData[i].life >= 0.0f) {
            if (killShots != 0)
                podData[i].life = -1.0f;
            podData[i].pos.x += podData[i].vel.x * gTimeScale;
            podData[i].pos.y += podData[i].vel.y * gTimeScale;
            xSize = GetSpriteXSize(podData[i].sprite);
            ySize = GetSpriteYSize(podData[i].sprite);
            SWSetSpriteLocation(podData[i].sprite,
                ((short)(podData[i].pos.x - shipData->pos.x)
                        + screenCenter.h - xSize / 2),
                ((short)(podData[i].pos.y - shipData->pos.y)
                        + screenCenter.v - ySize / 2));
            podData[i].sprite->translucency = 0;
            HandleSpriteMurkiness(podData[i].pos.x, podData[i].pos.y,
                         podData[i].sprite);
            podData[i].life -= gTimeScale;
            if (podData[i].life >= kPodFadeLife) {
                podData[i].sprite->translucency = 0;
            } else {
                if (podData[i].life > 0.0f)
                    podData[i].sprite->translucency =
                        (kPodFadeLife - podData[i].life);
                else
                    SWSetSpriteVisible(podData[i].sprite, 0);
            }
        } else {
            SWSetSpriteVisible(podData[i].sprite, 0);
        }
    }
}
