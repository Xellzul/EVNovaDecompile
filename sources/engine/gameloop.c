// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* gameloop.c -- the main-loop object (tu-map O3, win 0x00428340..0x0043b170). Notes: Research/notes/gameloop.md */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

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
#include "cefix.h"
#include "missions.h"
#include "crashlog.h"
#include "gameutil.h"

#define kSpriteSizeSlots    6
#define kFullHilite         0x20
#define kNoHilite           (-32)

#define kEngineHoldTicks    0x2d

#define MIN(a, b) ((a) < (b) ? (a) : (b))

                                                                 /* 0x00475f70 */
                                                                 /* 0x00438db0 */

/* HandleShipDisplay (mac _HandleShipDisplay). STATE: win MATCH 100%, exe 3665 / obj 3665; mac MATCH on
   both slices. OPEN: none. Win's `w` is one variable, the mac splits off `frame`. notes/gameloop.md */
// FUNCTION: EVNOVA 0x00428340
// MAC: _HandleShipDisplay i386 0x0002b514 ppc 0x0002fb54
void HandleShipDisplay(ShipObj *ship)
{
#ifdef MAC1010
    short impact;
    short damage;
    unsigned char msg[256];
    short sizes[kSpriteSizeSlots];
    float dy;
    float rate;
    float ion;
    short h;
    short best;
    short i;
    short jitter;
    FloatPoint explod;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
    short w;
    short blastRadius;
    short nFrames;
    short frame;
    Boolean hasGlow;
    Boolean hasAlt;
    Boolean hasLight;
    Boolean hasWeap;
    Boolean hasShield;
    Boolean altTimed;
    Boolean advanceAlt;
    Boolean disabled;
    Boolean hideAll;
    float angleScale;
    float dx;
#define n w
#define bank w
#elif defined(MACBUILD)
    unsigned char msg[256];
    short sizes[kSpriteSizeSlots];
    float dy;
    float rate;
    float ion;
    short h;
    short best;
    short i;
    short n;
    short jitter;
    int32_t impact;
    FloatPoint explod;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
    short bank;
    short blastRadius;
    short nFrames;
    short frame;
    short w;
    Boolean hasGlow;
    Boolean hasAlt;
    Boolean hasLight;
    Boolean hasWeap;
    Boolean hasShield;
    Boolean altTimed;
    Boolean advanceAlt;
    Boolean disabled;
    Boolean hideAll;
    float angleScale;
    float dx;
    short damage;
#else
    Boolean hasGlow;
    short damage;
    Boolean altTimed;
    unsigned char msg[256];
    short sizes[kSpriteSizeSlots];
    float dx;
    float dy;
    float rate;
    float ion;
    short nFrames;
    short h;
    short best;
    short bank;
    short i;
    short n;
    short jitter;
    short impact;
    FloatPoint explod;
    Boolean disabled;
    unsigned short blue;
    unsigned short red;
    unsigned short green;
    Boolean hideAll;
    Boolean hasWeap;
    short blastRadius;
    Boolean hasLight;
    Boolean hasAlt;
    float angleScale;
    Boolean hasShield;
    Boolean advanceAlt;
    short w;
#define frame w
#endif

    altTimed = advanceAlt = 0;

    if (ship->active == 0) {
        if (shipData[0].targetShip == ship->index) {
            shipData[0].targetShip = -1;
            statusRedraw[kRedrawTarg] = 1;
        }
        if (ship->sprite != NULL && ship->sprite->isVisible != 0)
            SWSetSpriteVisible(ship->sprite, 0);
        if (ship->altSprite != NULL && ship->altSprite->isVisible != 0)
            SWSetSpriteVisible(ship->altSprite, 0);
        if (ship->lightSprite != NULL && ship->lightSprite->isVisible != 0)
            SWSetSpriteVisible(ship->lightSprite, 0);
        if (ship->weapSprite != NULL && ship->weapSprite->isVisible != 0)
            SWSetSpriteVisible(ship->weapSprite, 0);
        if (ship->glowSprite != NULL && ship->glowSprite->isVisible != 0)
            SWSetSpriteVisible(ship->glowSprite, 0);
        if (ship->shieldSprite != NULL && ship->shieldSprite->isVisible != 0)
            SWSetSpriteVisible(ship->shieldSprite, 0);
        return;
    }

    for (i = 0; i < kSpriteSizeSlots; i++)
        sizes[i] = 0;
    disabled = IsDisabled(ship);
    hasGlow = hasLight = hasAlt = hasWeap = hasShield = 0;
    hideAll = 0;

    if (masterShipGlowSprite[ship->type]) {
        hasGlow = 1;
    } else if (ship->glowSprite != NULL && ship->glowSprite->isVisible != 0) {
        SWSetSpriteVisible(ship->glowSprite, 0);
    }
    if (masterShipLightSprite[ship->type]) {
        hasLight = 1;
    } else if (ship->lightSprite != NULL && ship->lightSprite->isVisible != 0) {
        SWSetSpriteVisible(ship->lightSprite, 0);
    }
    if (masterShipAltSprite[ship->type] != NULL) {
        if ((shipClass[ship->type].shanFlags & kShanHideAltDisabled) != 0 && disabled != 0)
            hasAlt = 0;
        else
            hasAlt = 1;
    } else if (ship->altSprite != NULL && ship->altSprite->isVisible != 0) {
        SWSetSpriteVisible(ship->altSprite, 0);
    }
    if (masterShipWeaponSprite[ship->type]) {
        hasWeap = 1;
    } else if (ship->weapSprite != NULL && ship->weapSprite->isVisible != 0) {
        SWSetSpriteVisible(ship->weapSprite, 0);
    }
    if (masterShipShieldSprite[ship->type]) {
        hasShield = 1;
    } else if (ship->shieldSprite != NULL && ship->shieldSprite->isVisible != 0) {
        SWSetSpriteVisible(ship->shieldSprite, 0);
    }

    nFrames = shipClass[ship->type].framesPer;
    angleScale = ((float)nFrames / 360.0);

    if (ship->curSystem == shipData[0].curSystem && ship->active != 0) {
        if (shipClass[ship->type].cost != -1) {
            SWSetSpriteVisible(ship->sprite, 1);
            SWCloneFramesFromMaster(ship->sprite, masterShipSprite[ship->type]);
            bank = 0;
            if (ship->sprite->numFrames > nFrames) {
                if ((shipClass[ship->type].shanFlags & kShanBanks) != 0) {
                    if (ship->bankDir < 0)
                        bank = 1;
                    else if (ship->bankDir > 0)
                        bank = 2;
                } else if ((shipClass[ship->type].shanFlags & kShanFoldingParts) != 0) {
                    if (ship->foldDir > 0) {
                        ship->frameTimer += gSpeedMult;
                        if (ship->frameTimer > shipClass[ship->type].animDelay) {
                            ship->frameTimer = 0;
                            ship->foldSet += 1;
                            if (ship->foldSet >= shipClass[ship->type].baseSets) {
                                ship->foldSet = shipClass[ship->type].baseSets - 1;
                                ship->foldDir = 0;
                            }
                        }
                    } else if (ship->foldDir < 0) {
                        ship->frameTimer += gSpeedMult;
                        if (ship->frameTimer > shipClass[ship->type].animDelay) {
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
                    if ((shipClass[ship->type].shanFlags & kShanUnfoldsWhenFiring) != 0
                            && ship->foldDir < 1
                            && ship->foldSet < shipClass[ship->type].baseSets - 1
                            && ReadTick() >= ship->fireTick + kEngineHoldTicks)
                        ship->foldDir = 1;
                    bank = ship->foldSet;
                } else if ((shipClass[ship->type].shanFlags & kShanKeyCarriedFrames) != 0) {
                    if (KeyCarriedShipTypeOnboard(ship) != 0)
                        bank = 1;
                } else if ((shipClass[ship->type].shanFlags & kShanAnimatedBase) != 0) {
                    altTimed = 1;
                    if ((shipClass[ship->type].shanFlags & kShanStopAnimDisabled) == 0
                            || disabled == 0)
                        ship->animTimer += gSpeedMult;
                    if (ship->animTimer > shipClass[ship->type].animDelay) {
                        advanceAlt = 1;
                        if (shipClass[ship->type].animDelay >= 1) {
                            while (ship->animTimer > shipClass[ship->type].animDelay)
                                ship->animTimer -= shipClass[ship->type].animDelay;
                        } else
                            ship->animTimer = 0;
                        if (ship->animSet < 0)
                            ship->animSet = 0;
                        ship->animSet += 1;
                        if (ship->animSet >= shipClass[ship->type].baseSets)
                            ship->animSet = 0;
                    }
                    bank = ship->animSet;
                }
            }
            if ((shipClass[ship->type].shanFlags & kShanSkewCorrect) != 0) {
                if (shipClass[ship->type].frameAngles) {
                    frame = nFrames - 1;
                    i = nFrames - 1;
                    while (i >= 0) {
                        if (shipClass[ship->type].frameAngles[i] < ship->heading) {
                            frame = i;
                            break;
                        }
                        i -= 1;
                    }
                } else {
                    frame = (ship->heading * angleScale);
                }
            } else {
                frame = (ship->heading * angleScale);
            }
            SWSetCurrentFrameIndex(ship->sprite, (bank * nFrames + frame));
            if (hasGlow != 0) {
                SWCloneFramesFromMaster(ship->glowSprite, masterShipGlowSprite[ship->type]);
                SWSetCurrentFrameIndex(ship->glowSprite, (bank * nFrames + frame));
            }
            if (hasLight != 0) {
                SWCloneFramesFromMaster(ship->lightSprite, masterShipLightSprite[ship->type]);
                SWSetCurrentFrameIndex(ship->lightSprite, (bank * nFrames + frame));
            }
            if (hasWeap != 0) {
                SWCloneFramesFromMaster(ship->weapSprite, masterShipWeaponSprite[ship->type]);
                SWSetCurrentFrameIndex(ship->weapSprite, (bank * nFrames + frame));
            }
            if (hasShield != 0) {
                SWCloneFramesFromMaster(ship->shieldSprite, masterShipShieldSprite[ship->type]);
                if (ship->shieldSprite->numFrames > nFrames)
                    SWSetCurrentFrameIndex(ship->shieldSprite,
                                           (bank * nFrames + frame));
                else if (ship->shieldSprite->numFrames > 1)
                    SWSetCurrentFrameIndex(ship->shieldSprite, frame);
                else
                    SWSetCurrentFrameIndex(ship->shieldSprite, 0);
            }
            if (disabled != 0) {
                PutSpriteInLayer(ship->sprite, disabledShipLayer);
                PutSpriteInLayer(ship->glowSprite, disabledShipLayer);
                PutSpriteInLayer(ship->lightSprite, disabledShipLayer);
                PutSpriteInLayer(ship->weapSprite, disabledShipLayer);
                PutSpriteInLayer(ship->altSprite, disabledShipLayer);
                PutSpriteInLayer(ship->shieldSprite, disabledShipLayer);
            } else if (ship->parentShip == 0) {
                PutSpriteInLayer(ship->sprite, escortLayer);
                PutSpriteInLayer(ship->glowSprite, escortLayer);
                PutSpriteInLayer(ship->lightSprite, escortLayer);
                PutSpriteInLayer(ship->weapSprite, escortLayer);
                PutSpriteInLayer(ship->altSprite, escortLayer);
                PutSpriteInLayer(ship->shieldSprite, escortLayer);
            } else {
                PutSpriteInLayer(ship->sprite, shipLayer);
                PutSpriteInLayer(ship->glowSprite, shipLayer);
                PutSpriteInLayer(ship->lightSprite, shipLayer);
                PutSpriteInLayer(ship->weapSprite, shipLayer);
                PutSpriteInLayer(ship->altSprite, shipLayer);
                PutSpriteInLayer(ship->shieldSprite, shipLayer);
            }

            ship->sprite->translucency = shipClass[ship->type].transparency;
            if (ship->index == 0) {
                if (playerEffectiveMurk < 0)
                    playerEffectiveMurk = PlayerEffectiveMurk();
                ship->sprite->tintMode = 0;
                ship->sprite->tintColor = systemData[ship->curSystem].mapPixel;
            } else {
                HandleSpriteMurkiness(ship->pos.x, ship->pos.y,
                                      ship->sprite);
            }
            ship->sprite->tintRed = kNoHilite;
            ship->sprite->tintGreen = kNoHilite;
            ship->sprite->tintBlue = kNoHilite;
            GetShipPaintColor(ship, &red, &green, &blue);
            ship->sprite->tintRed += red;
            ship->sprite->tintGreen += green;
            ship->sprite->tintBlue += blue;

            if (ship->ionization > 0.0F) {
                ion = ShipIonizationFactor(ship);
                if (ion >= 0.33) {
#ifdef MACBUILD
                    w = (ion * 24.0F * 0.01);
                    if (w > 0x18)
                        w = 0x18;
                    if (w < 0x10)
                        w = 0x10;
                    w += Rand(5) - 2;
                    if (w > ship->sprite->tintMode || ship->sprite->tintMode < 8) {
                        ship->sprite->tintMode = w;
#else
                    h = (ion * 24.0F * 0.01);
                    if (h > 0x18)
                        h = 0x18;
                    if (h < 0x10)
                        h = 0x10;
                    h += Rand(5) - 2;
                    if (h > ship->sprite->tintMode || ship->sprite->tintMode < 8) {
                        ship->sprite->tintMode = h;
#endif
                        ship->sprite->tintColor = ship->ionColor;
                    } else if (gSpriteWorld->pixelDepth == 8) {
                        ship->sprite->tintColor = ship->ionColor;
                    } else {
                        ship->sprite->tintColor = kFullHilite - ship->sprite->tintMode;
                    }
                }
            } else {
                ship->ionColor = 0;
            }

            if (ship->index != 0) {
                if (AIShipIsEnteringHypergate(ship) != 0) {
                    ship->sprite->translucency = (32.0F - ship->aiTimer * 2.0F);
                    if (ship->sprite->translucency < 0)
                        ship->sprite->translucency = 0;
                    if (ship->sprite->translucency > kFullHilite)
                        ship->sprite->translucency = kFullHilite;
                    if (gSpriteWorld->pixelDepth == 0x10) {
                        ship->sprite->tintMode = ship->sprite->translucency * 2;
                        if (ship->sprite->tintMode > kFullHilite)
                            ship->sprite->tintMode = kFullHilite;
                        ship->sprite->tintColor = 0xffff;
                    }
                    if (ship->aiTimer < 1.0F) {
                        ship->active = 0;
                        ship->parentShip = -1;
                    }
                } else if (AIShipIsEmergingFromHypergate(ship) != 0) {
                    if (ship->aiTimer <= 16.0F && gSpriteWorld->pixelDepth == 0x10) {
                        n = (16.0F - ship->aiTimer);
                        if (n < 0)
                            n = 0;
                        if (n > 0x10)
                            n = 0x10;
                        ship->sprite->translucency = kFullHilite - n * 2;
                        if (ship->sprite->translucency < 0)
                            ship->sprite->translucency = 0;
                        if (ship->sprite->translucency > kFullHilite)
                            ship->sprite->translucency = kFullHilite;
                        ship->sprite->tintMode = ship->sprite->translucency * 2;
                        if (ship->sprite->tintMode > kFullHilite)
                            ship->sprite->tintMode = kFullHilite;
                        ship->sprite->tintColor = 0xffff;
                    } else {
                        hideAll = 1;
                    }
                }
            }

            if (hasGlow != 0) {
                SWSetSpriteVisible(ship->glowSprite, 1);
                n = ship->engineGlow - 4 + Rand(6);
                if (n <= 1) {
                    SWSetSpriteVisible(ship->glowSprite, 0);
                } else {
                    if (n > kFullHilite)
                        n = kFullHilite;
                    if (ship->index == 0) {
                        ship->glowSprite->tintMode = 0;
                        ship->glowSprite->tintColor = systemData[ship->curSystem].mapPixel;
                    } else {
                        ship->glowSprite->tintColor = ship->sprite->tintColor;
                        ship->glowSprite->tintMode = ship->sprite->tintMode;
                        if (n > 32.0 - ship->glowSprite->tintMode * 1.5)
                            n = (32.0 - ship->glowSprite->tintMode * 1.5);
                        if (n < 0)
                            n = 0;
                    }
                    if (gSpriteWorld->pixelDepth == 8) {
                        ship->glowSprite->translucency = kFullHilite - n;
                    } else {
                        ship->glowSprite->translucency = kFullHilite;
                        ship->glowSprite->tintRed = n;
                        ship->glowSprite->tintGreen = n;
                        ship->glowSprite->tintBlue = n;
                    }
                }
            }

            if (hasLight != 0) {
                if (shipClass[ship->type].blinkMode == 1) {
                    if (ship->lightTimer > (shipClass[ship->type].blinkValue[1] > shipClass[ship->type].blinkValue[3]
                                ? shipClass[ship->type].blinkValue[1]
                                : shipClass[ship->type].blinkValue[3]))
                        ship->lightTimer =
                            (shipClass[ship->type].blinkValue[1] > shipClass[ship->type].blinkValue[3]
                                ? shipClass[ship->type].blinkValue[1]
                                : shipClass[ship->type].blinkValue[3]);
                    if (ship->blinkCount < shipClass[ship->type].blinkValue[2]) {
                        if (ship->lightTimer < 0.0F) {
                            if (ship->lightLevel > 0.0F) {
                                ship->lightLevel = 0;
                                ship->blinkCount += 1;
                                ship->lightTimer = shipClass[ship->type].blinkValue[0];
                            } else {
                                ship->lightLevel = 32.0F;
                                ship->lightTimer = shipClass[ship->type].blinkValue[1];
                            }
                        } else {
                            ship->lightTimer -= gSpeedMult;
                        }
                    } else if (ship->lightTimer < 0.0F) {
                        ship->blinkCount = 0;
                        ship->lightLevel = 0;
                        ship->lightTimer = shipClass[ship->type].blinkValue[3];
                    } else {
                        ship->lightTimer -= gSpeedMult;
                    }
                } else if (shipClass[ship->type].blinkMode == 2) {
                    if (ship->blinkCount == 0) {
                        if (ship->lightLevel >= shipClass[ship->type].blinkValue[2]) {
                            ship->lightLevel = shipClass[ship->type].blinkValue[2];
                            ship->blinkCount = 1;
                        } else {
                            ship->lightLevel += shipClass[ship->type].blinkValue[1] * 0.01
                                           * gSpeedMult;
                        }
                    } else if (ship->lightLevel <= shipClass[ship->type].blinkValue[0]) {
                        ship->lightLevel = shipClass[ship->type].blinkValue[0];
                        ship->blinkCount = 0;
                    } else {
                        ship->lightLevel -= shipClass[ship->type].blinkValue[3] * 0.01
                                       * gSpeedMult;
                    }
                } else if (shipClass[ship->type].blinkMode == 3) {
                    if (ship->lightTimer <= 0.0F) {
                        ship->lightLevel = shipClass[ship->type].blinkValue[0]
                                           + Rand(shipClass[ship->type].blinkValue[1] + 1
                                                  - shipClass[ship->type].blinkValue[0]);
                        ship->lightTimer = shipClass[ship->type].blinkValue[2];
                    } else {
                        ship->lightTimer -= gSpeedMult;
                        if (ship->lightTimer > shipClass[ship->type].blinkValue[2])
                            ship->lightTimer = shipClass[ship->type].blinkValue[2];
                    }
                } else {
                    ship->lightLevel = 32.0F;
                }
                if ((shipClass[ship->type].shanFlags & kShanHideLightsDisabled) != 0
                        && disabled != 0)
                    ship->lightLevel = 0;
                if (ship->lightLevel > 1.0F) {
                    SWSetSpriteVisible(ship->lightSprite, 1);
                    n = ship->lightLevel;
                    if (ship->index == 0) {
                        ship->lightSprite->tintMode = 0;
                    } else {
                        ship->lightSprite->tintColor = ship->sprite->tintColor;
                        ship->lightSprite->tintMode = (ship->sprite->tintMode * 0.333);
                        if (n > 32.0 - ship->lightSprite->tintMode * 1.5)
                            n = (32.0 - ship->lightSprite->tintMode * 1.5);
                        if (n < 0)
                            n = 0;
                    }
                    if (gSpriteWorld->pixelDepth == 8) {
                        ship->lightSprite->translucency = kFullHilite - n;
                    } else {
                        ship->lightSprite->translucency = kFullHilite;
                        ship->lightSprite->tintRed = n;
                        ship->lightSprite->tintGreen = n;
                        ship->lightSprite->tintBlue = n;
                    }
                } else {
                    SWSetSpriteVisible(ship->lightSprite, 0);
                }
            }

            if (hasWeap != 0) {
                if (ship->weapGlow > 0.0F) {
                    SWSetSpriteVisible(ship->weapSprite, 1);
                    if (ship->weapGlow > 32.0F)
                        ship->weapGlow = 32.0F;
                    n = ship->weapGlow;
                    if (ship->index == 0) {
                        ship->weapSprite->tintMode = 0;
                    } else {
                        ship->weapSprite->tintColor = ship->sprite->tintColor;
                        ship->weapSprite->tintMode = ship->sprite->tintMode;
                        if (n > 32.0 - ship->weapSprite->tintMode * 1.5)
                            n = (32.0 - ship->weapSprite->tintMode * 1.5);
                        if (n < 0)
                            n = 0;
                    }
                    if (gSpriteWorld->pixelDepth == 8) {
                        ship->weapSprite->translucency = kFullHilite - n;
                    } else {
                        ship->weapSprite->translucency = kFullHilite;
                        ship->weapSprite->tintRed = n;
                        ship->weapSprite->tintGreen = n;
                        ship->weapSprite->tintBlue = n;
                    }
                    ship->weapGlow -= shipClass[ship->type].weapDecay * gSpeedMult;
                    if (ship->weapGlow < -1.0F)
                        ship->weapGlow = -1.0F;
                } else {
                    SWSetSpriteVisible(ship->weapSprite, 0);
                }
            }

            if (hasShield != 0) {
                if (ship->shieldGlow > 0.0F && ship->shield > 0.0F) {
                    SWSetSpriteVisible(ship->shieldSprite, 1);
                    ship->shieldSprite->translucency = kFullHilite;
                    n = ship->shieldGlow;
                    h = (ship->shield * 32.0 / (float)shipClass[ship->type].shield);
                    if (h > kFullHilite)
                        h = kFullHilite;
                    if (h < 4)
                        h = 4;
                    if (ship->shieldGlow > h)
                        ship->shieldGlow = h;
                    if (n > h)
                        n = h;
                    else if (n < 1)
                        n = 1;
                    ship->shieldSprite->tintRed = n;
                    ship->shieldSprite->tintGreen = n;
                    ship->shieldSprite->tintBlue = n;
                    ship->shieldSprite->tintMode = ship->sprite->tintMode;
                    ship->shieldSprite->tintColor = ship->sprite->tintColor;
                } else {
                    SWSetSpriteVisible(ship->shieldSprite, 0);
                }
            }

            if (ship->shieldGlow > 0.0F)
                ship->shieldGlow -= gSpeedMult;

            if (hasAlt != 0) {
                if ((shipClass[ship->type].shanFlags & kShanStopAnimDisabled) == 0
                        || disabled == 0)
                    ship->animTimer += gSpeedMult;
                if (altTimed == 0
                        && ship->animTimer > shipClass[ship->type].animDelay) {
                    ship->animTimer = 0;
                    advanceAlt = 1;
                }
                if (advanceAlt != 0) {
                    ship->altSet += 1;
                    if (ship->altSet >= shipClass[ship->type].altSets)
                        ship->altSet = 0;
                }
                SWSetSpriteVisible(ship->altSprite, 1);
                SWCloneFramesFromMaster(ship->altSprite, masterShipAltSprite[ship->type]);
                SWSetCurrentFrameIndex(ship->altSprite,
                                       (nFrames * ship->altSet + frame));
                ship->altSprite->translucency = ship->sprite->translucency;
                ship->altSprite->tintColor = ship->sprite->tintColor;
                ship->altSprite->tintMode = ship->sprite->tintMode;
                ship->altSprite->tintRed = ship->sprite->tintRed;
                ship->altSprite->tintGreen = ship->sprite->tintGreen;
                ship->altSprite->tintBlue = ship->sprite->tintBlue;
            }

            if (ship->sprite != NULL)
                ship->sprite->onTop = 1;
            if (ship->altSprite != NULL)
                ship->altSprite->onTop = 1;
            if (ship->glowSprite != NULL)
                ship->glowSprite->onTop = 1;
            if (ship->weapSprite != NULL)
                ship->weapSprite->onTop = 1;
            if (ship->lightSprite != NULL)
                ship->lightSprite->onTop = 1;
            if (ship->shieldSprite != NULL)
                ship->shieldSprite->onTop = 1;

            if (ship->sprite != NULL)
                sizes[0] = GetFrameBiggestSize(ship->sprite->curFrameP);
            if (hasAlt != 0 && ship->altSprite->isVisible != 0)
                sizes[1] = GetFrameBiggestSize(ship->altSprite->curFrameP);
            if (hasGlow != 0 && ship->glowSprite->isVisible != 0)
                sizes[2] = GetFrameBiggestSize(ship->glowSprite->curFrameP);
            if (hasWeap != 0 && ship->weapSprite->isVisible != 0)
                sizes[3] = GetFrameBiggestSize(ship->weapSprite->curFrameP);
            if (hasLight != 0 && ship->lightSprite->isVisible != 0)
                sizes[4] = GetFrameBiggestSize(ship->lightSprite->curFrameP);
            if (hasShield != 0 && ship->shieldSprite->isVisible != 0)
                sizes[5] = GetFrameBiggestSize(ship->shieldSprite->curFrameP);

            best = -1;
            n = 0;
            for (i = 0; i < kSpriteSizeSlots; i++) {
                if (sizes[i] > n) {
                    best = i;
                    n = sizes[i];
                }
            }
            if (best == 1)
                ship->altSprite->onTop = 0;
            else if (best == 2)
                ship->glowSprite->onTop = 0;
            else if (best == 3)
                ship->weapSprite->onTop = 0;
            else if (best == 4)
                ship->lightSprite->onTop = 0;
            else if (best == 5)
                ship->shieldSprite->onTop = 0;
            else
                ship->sprite->onTop = 0;
        } else {
            SWSetSpriteVisible(ship->sprite, 0);
            if (hasGlow != 0)
                SWSetSpriteVisible(ship->glowSprite, 0);
            if (hasLight != 0)
                SWSetSpriteVisible(ship->lightSprite, 0);
            if (hasAlt != 0)
                SWSetSpriteVisible(ship->altSprite, 0);
            if (hasWeap != 0)
                SWSetSpriteVisible(ship->weapSprite, 0);
            if (hasShield != 0)
                SWSetSpriteVisible(ship->shieldSprite, 0);
        }

        w = GetSpriteXSize(ship->sprite);
        h = GetSpriteYSize(ship->sprite);
        SWSetSpriteLocation(ship->sprite,
            (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
            (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        if (hasGlow != 0) {
            w = GetSpriteXSize(ship->glowSprite);
            h = GetSpriteYSize(ship->glowSprite);
            SWSetSpriteLocation(ship->glowSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasLight != 0) {
            w = GetSpriteXSize(ship->lightSprite);
            h = GetSpriteYSize(ship->lightSprite);
            SWSetSpriteLocation(ship->lightSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasAlt != 0) {
            w = GetSpriteXSize(ship->altSprite);
            h = GetSpriteYSize(ship->altSprite);
            SWSetSpriteLocation(ship->altSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasWeap != 0) {
            w = GetSpriteXSize(ship->weapSprite);
            h = GetSpriteYSize(ship->weapSprite);
            SWSetSpriteLocation(ship->weapSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }
        if (hasShield != 0) {
            w = GetSpriteXSize(ship->shieldSprite);
            h = GetSpriteYSize(ship->shieldSprite);
            SWSetSpriteLocation(ship->shieldSprite,
                (short)(ship->pos.x - shipData[0].pos.x) + screenCenter.h - w / 2,
                (short)(ship->pos.y - shipData[0].pos.y) + screenCenter.v - h / 2);
        }

        if (ship->cloakDir != 0) {
            if ((shipClass[ship->type].flags2 & kShipSwarms) != 0)
                rate = 1.5F;
            else
                rate = 0.75F;
            ship->cloak += ship->cloakDir * rate * gSpeedMult;
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
                    ship->sprite->translucency = 0x1e;
                else if (shipData[0].seesCloaked[0] == 1 || ship->parentShip == 0)
                    ship->sprite->translucency = 0x1e;
                else
                    ship->sprite->translucency = 0x40;
                if (red >= 0x10)
                    ship->sprite->tintRed = 2;
                else
                    ship->sprite->tintRed = 0;
                if (green >= 0x10)
                    ship->sprite->tintGreen = 2;
                else
                    ship->sprite->tintGreen = 0;
                if (blue >= 0x10)
                    ship->sprite->tintBlue = 2;
                else
                    ship->sprite->tintBlue = 0;
                if (hasAlt != 0)
                    ship->altSprite->translucency = ship->sprite->translucency;
                if (hasGlow != 0)
                    SWSetSpriteVisible(ship->glowSprite, 0);
                if (hasLight != 0)
                    SWSetSpriteVisible(ship->lightSprite, 0);
                if (hasShield != 0)
                    SWSetSpriteVisible(ship->shieldSprite, 0);
                w = h = 0;
            } else if (ship->sprite != NULL) {
                jitter = (ship->cloak / 10.0F);
                w = jitter - Rand(jitter * 2 + 1);
                h = jitter - Rand(jitter * 2 + 1);
                OffsetRect(&ship->sprite->destFrameRect, w, h);
                if (gSpriteWorld->pixelDepth == 0x10)
                    ship->sprite->translucency = kFullHilite;
                else
                    ship->sprite->translucency = 0x10;
                ship->sprite->tintRed = (red - ship->cloak);
                ship->sprite->tintGreen = (green - ship->cloak);
                ship->sprite->tintBlue = (blue - ship->cloak);
                if (ship->sprite->tintRed < 2) {
                    if (red >= 0x10)
                        ship->sprite->tintRed = 2;
                    else
                        ship->sprite->tintRed = 0;
                }
                if (ship->sprite->tintGreen < 2) {
                    if (green >= 0x10)
                        ship->sprite->tintGreen = 2;
                    else
                        ship->sprite->tintGreen = 0;
                }
                if (ship->sprite->tintBlue < 2) {
                    if (blue >= 0x10)
                        ship->sprite->tintBlue = 2;
                    else
                        ship->sprite->tintBlue = 0;
                }
                if (hasAlt != 0 && ship->altSprite != NULL) {
                    OffsetRect(&ship->altSprite->destFrameRect, w, h);
                    ship->altSprite->translucency = ship->sprite->translucency;
                    ship->altSprite->tintRed = ship->sprite->tintRed;
                    ship->altSprite->tintGreen = ship->sprite->tintGreen;
                    ship->altSprite->tintBlue = ship->sprite->tintBlue;
                }
                if (hasGlow != 0 && ship->glowSprite != NULL) {
                    OffsetRect(&ship->glowSprite->destFrameRect, w, h);
                    ship->glowSprite->translucency = kFullHilite;
                    ship->glowSprite->tintRed = MIN(ship->glowSprite->tintRed, 32.0F - ship->cloak);
                    ship->glowSprite->tintGreen = MIN(ship->glowSprite->tintGreen, 32.0F - ship->cloak);
                    ship->glowSprite->tintBlue = MIN(ship->glowSprite->tintBlue, 32.0F - ship->cloak);
                }
                if (hasLight != 0 && ship->lightSprite != NULL) {
                    OffsetRect(&ship->lightSprite->destFrameRect, w, h);
                    ship->lightSprite->translucency = kFullHilite;
                    ship->lightSprite->tintRed = MIN(ship->lightSprite->tintRed, 32.0F - ship->cloak);
                    ship->lightSprite->tintGreen = MIN(ship->lightSprite->tintGreen, 32.0F - ship->cloak);
                    ship->lightSprite->tintBlue = MIN(ship->lightSprite->tintBlue, 32.0F - ship->cloak);
                }
            }
            if (hasWeap != 0 && ship->weapSprite != NULL) {
                if (gSpriteWorld->pixelDepth == 8) {
                    SWSetSpriteVisible(ship->weapSprite, 0);
                } else {
                    OffsetRect(&ship->weapSprite->destFrameRect, w, h);
                    ship->weapSprite->translucency = kFullHilite;
                    ship->weapSprite->tintRed = MIN(ship->weapSprite->tintRed, 40.0F - ship->cloak);
                    ship->weapSprite->tintGreen = MIN(ship->weapSprite->tintGreen, 40.0F - ship->cloak);
                    ship->weapSprite->tintBlue = MIN(ship->weapSprite->tintBlue, 40.0F - ship->cloak);
                }
            }
        }

        if (hideAll != 0) {
            if (ship->sprite != NULL && ship->sprite->isVisible != 0)
                SWSetSpriteVisible(ship->sprite, 0);
            if (ship->altSprite != NULL && ship->altSprite->isVisible != 0)
                SWSetSpriteVisible(ship->altSprite, 0);
            if (ship->lightSprite != NULL && ship->lightSprite->isVisible != 0)
                SWSetSpriteVisible(ship->lightSprite, 0);
            if (ship->weapSprite != NULL && ship->weapSprite->isVisible != 0)
                SWSetSpriteVisible(ship->weapSprite, 0);
            if (ship->glowSprite != NULL && ship->glowSprite->isVisible != 0)
                SWSetSpriteVisible(ship->glowSprite, 0);
            if (ship->shieldSprite != NULL && ship->shieldSprite->isVisible != 0)
                SWSetSpriteVisible(ship->shieldSprite, 0);
        }

        if (IsDying(ship) != 0 && ship->type != kShipTypeEscapePod) {
            if (ship->deathTimer <= 0.0F) {
                ship->deathTimer = shipClass[ship->type].deathDelay;
                if (ship->index == 0)
                    ship->deathTimer *= 3.0F;
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
                if ((shipClass[ship->type].flags & kShipPlanetType) == 0) {
                    if (shipClass[ship->type].mass < 100) {
                        blastRadius = 0;
                        impact = 1;
                        damage = 0;
                    } else {
                        blastRadius = (shipClass[ship->type].mass * 0.075 + 50.0);
                        impact = 750;
                        damage = (shipClass[ship->type].mass * 0.0375 + 25.0);
                    }
                    if (blastRadius > 0) {
                        for (i = 0; i < kMaxShips; i++) {
                            if (i != ship->index && shipData[i].active != 0) {
                                dx = FloatAbs(shipData[i].pos.x - ship->pos.x);
                                dy = FloatAbs(shipData[i].pos.y - ship->pos.y);
                                if (dx <= blastRadius && dy <= blastRadius
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
                        if (currentMissions[ship->specialMission].shipGoal == kMissionGoalDisable
                                || (currentMissions[ship->specialMission].shipGoal == kMissionGoalBoard && ship->boarded == 0)
                                || (currentMissions[ship->specialMission].shipGoal == kMissionGoalRescue && ship->boarded == 0)
                                || currentMissions[ship->specialMission].shipGoal == kMissionGoalEscort) {
                            if (currentMissionObjectives[ship->specialMission].failed == 0
                                    && (currentMissionObjectives[ship->specialMission].flags
                                        & kMissionHidden) == 0) {
                                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                                GetIndString(msg, kGameStrList, 0x11c);
                                DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
                                QuickMissionFailure(ship->specialMission);
                            }
                        }
                    }
                    currentMissions[ship->specialMission].specialShipsKilled++;
                    if (currentMissions[ship->specialMission].shipCount > 0)
                        currentMissions[ship->specialMission].shipCount--;
                }
                if (ship->parentShip == 0 && ship->aiType == kShipTypeEscort && ship->specialMission == -1
                        && shipClass[ship->type].inherentAI <= kShipTypeBraveTrader) {
                    DestroyPartialFleetCargo(ship->index);
                    ShipStatsToSystemInfo();
                    statusRedraw[kRedrawCargo] = 1;
                }
                if (ship->persType != -1) {
                    if (ship->persType == kPersBountyHunter) {
                        if (Rand(8) == 0)
                            personData[ship->persType].loaded = 0;
                    } else if ((personData[ship->persType].flags & kPersEscapePod) == 0) {
                        personData[ship->persType].loaded = 0;
                    }
                }
                CreateExplosion(ship->pos,
                                shipClass[ship->type].explode2, blastRadius, 1);
                SWSetSpriteVisible(ship->sprite, 0);
                ship->active = 0;
                ship->parentShip = -1;
            } else {
                if (ship->deathTimer < 20.0F)
                    w = 1;
                else if (ship->deathTimer < 40.0F)
                    w = 2;
                else if (ship->deathTimer < 60.0F)
                    w = 4;
                else
                    w = 8;
                if (Rand(w) == 0) {
                    h = (GetSpriteXSize(ship->sprite) * 0.25);
                    if (h <= 0)
                        h = 1;
                    explod.x = ship->pos.x + Rand(h * 2) - h;
                    explod.y = ship->pos.y + Rand(h * 2) - h;
                    if (w <= 2 && shipClass[ship->type].deathDelay >= 0x3c)
                        Rand(2);
                    if (Rand(4) == 0)
                        CreateExplosion(explod,
                                        shipClass[ship->type].explode1, 0, 1);
                    else
                        CreateExplosion(explod,
                                        shipClass[ship->type].explode1, 0, 0);
                }
            }
        }
    } else if (gameFrozen == 0) {
        ship->active = 0;
        ship->parentShip = -1;
    }

    if (ship->active != 0 && ship->curSystem == shipData[0].curSystem) {
        for (i = 0; i <= 1; i++) {
            if (ship->seesCloaked[i] < 0) {
                if (ShipCanSeeCloakedShips(ship, i) != 0)
                    ship->seesCloaked[i] = 1;
                else
                    ship->seesCloaked[i] = 0;
            }
        }
        if (ship->decloaksWhenHit < 0) {
            if (ShipDecloaksWhenHit(ship) != 0)
                ship->decloaksWhenHit = 1;
            else
                ship->decloaksWhenHit = 0;
        }
    } else {
        ship->seesCloaked[0] = ship->seesCloaked[1] = -1;
        ship->decloaksWhenHit = -1;
    }
}
#undef frame
#ifdef MAC1010
#undef n
#undef bank
#endif

#define kSpinWrap               36.0f
#define kFadeStart              32.0f
#define kMaxMurk                0x1f


/* HandleBoxes. STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x0042c1b0
// MAC: _HandleBoxes i386 0x0002df85 ppc 0x00032d4c
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

        boxData[i].angle += boxData[i].spin * gSpeedMult;
        while (boxData[i].angle >= kSpinWrap)
            boxData[i].angle -= kSpinWrap;
        while (boxData[i].angle < 0.0f)
            boxData[i].angle += kSpinWrap;

        boxData[i].timer -= gSpeedMult;
        boxData[i].pos.x += boxData[i].vel.x * gSpeedMult;
        boxData[i].pos.y += boxData[i].vel.y * gSpeedMult;
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
// MAC: _HandleSmoke i386 0x0002e29f ppc 0x000330d4
void HandleSmoke(void)
{
    short i;
    short frame;
    short temp;
    short halfY;

    for (i = 0; i < kDebrisSlots; i++) {
        if (smokeData[i].timer >= 0.0f && killSmoke == 0
                && smokeData[i].sprite != 0) {
            SWSetSpriteVisible(smokeData[i].sprite, 1);
            temp = (kFadeBase - smokeData[i].timer
                          / smokeData[i].sprite->numFrames
                          * kFadeSpan);
            if (temp < 0)
                temp = 0;
            if (temp > kFullBlend)
                temp = kFullBlend;
            smokeData[i].sprite->translucency = temp;
            HandleSpriteMurkiness(smokeData[i].pos.x,
                                  smokeData[i].pos.y,
                                  smokeData[i].sprite);
            smokeData[i].sprite->tintRed = kFullBlend;
            smokeData[i].sprite->tintGreen = kFullBlend;
            smokeData[i].sprite->tintBlue = kFullBlend;
            smokeData[i].sprite->translucency = kFullBlend;

            if (gameFrozen == 0)
                smokeData[i].timer += gSpeedMult * kAgeRate;

            if (smokeData[i].type == 1 || smokeData[i].type == 3) {
                if (smokeData[i].type == 3)
                    frame = (short)smokeData[i].timer / 2;
                else
                    frame = smokeData[i].timer;
                if (frame <= 2) {
                    SWSetCurrentFrameIndex(smokeData[i].sprite, frame);
                }
                else if (frame <= 5) {
                    SWSetCurrentFrameIndex(smokeData[i].sprite, 5 - frame);
                }
                else {
                    SWSetSpriteVisible(smokeData[i].sprite, 0);
                    smokeData[i].timer = -1.0f;
                }
            }
            else {
                if (smokeData[i].type == 2)
                    frame = (short)smokeData[i].timer / 2;
                else
                    frame = smokeData[i].timer;
                if (frame < 8) {
                    SWSetCurrentFrameIndex(smokeData[i].sprite, frame);
                }
                else {
                    SWSetSpriteVisible(smokeData[i].sprite, 0);
                    smokeData[i].timer = -1.0f;
                }
            }

            frame = GetSpriteXSize(smokeData[i].sprite) / 2;
            halfY = GetSpriteYSize(smokeData[i].sprite) / 2;
            SWSetSpriteLocation(smokeData[i].sprite,
                (short)(smokeData[i].pos.x - shipData[0].pos.x)
                    + screenCenter.h - frame,
                (short)(smokeData[i].pos.y - shipData[0].pos.y)
                    + screenCenter.v - halfY);
        }
        else if (smokeData[i].sprite != 0) {
            SWSetSpriteVisible(smokeData[i].sprite, 0);
            smokeData[i].timer = -1.0f;
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
// MAC: _HandleDoubleTimeSprite i386 0x0002e5a7 ppc 0x0003347c
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
// MAC: _HandleAlertSprite i386 0x0002e61d ppc 0x000334f8
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

#define kApproachSpan       1.1      /* pooled double */

/* HandleStellarSprites (mac _HandleStellarSprites). STATE: MATCH 100.0%, exe 654 / obj 654,
   byte-identical yes (2942 bytes), relocregcheck 0 of 128. OPEN: none */
// FUNCTION: EVNOVA 0x0042cd10
// MAC: _HandleStellarSprites i386 0x0002e6f1 ppc 0x00033600
void HandleStellarSprites(void)
{
    short i;
    short k;
    short j;
    short type;
    short open;
    short w;
    short h;
    short reach;
    Boolean animate;
    Boolean approach;
    short delay;

    for (i = 0; i < kStellarSprites; i++) {
        if (stellarSprite[i] != NULL && stellarSprite[i]->isVisible != 0
            && stellarSprite[i]->userData >= 0
            && stellarSprite[i]->userData < kMaxStellars) {
            SWSetSpriteVisible(stellarSprite[i], 1);
            k = stellarSprite[i]->userData;
            if (StellarIsDestroyed(&stellarData[k]) != 0) {
                if ((stellarData[k].flags2 & kSpobAnimWhenDestroyed) != 0)
                    animate = 1;
                else
                    animate = 0;
                type = stellarData[k].deadType;
            } else {
                if ((stellarData[k].flags2 & kSpobAnimWhenDestroyed) != 0)
                    animate = 0;
                else
                    animate = 1;
                type = stellarData[k].type;
            }
            if (!masterStellarSprite[type])
                LoadMasterStellarSprite(type);
            SWCloneFramesFromMaster(stellarSprite[i], masterStellarSprite[type]);
            if (animate != 0 && stellarData[k].numFrames > 1) {
                stellarData[k].frameTimer += gSpeedMult;
                if ((stellarData[k].flags2 & kSpobHypergate) != 0) {
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
                                    if (FloatAbs(shipData[j].pos.x - stellarData[k].pos.h)
                                            < reach) {
                                        if (FloatAbs(shipData[j].pos.y - stellarData[k].pos.v)
                                                < reach) {
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
                            j = stellarData[k].animDelay * stellarData[k].frame0Bias;
                        else
                            j = stellarData[k].animDelay;
                        if (stellarData[k].frameTimer >= j) {
                            stellarData[k].frameTimer = 0.0F;
                            if (stellarData[k].curFrame < open) {
                                stellarData[k].curFrame += 1;
                                stellarData[k].nextFrame = stellarData[k].curFrame;
                            } else if ((stellarData[k].flags2 & kSpobFrame0Between) != 0) {
                                if (open == stellarData[k].curFrame) {
                                    stellarData[k].curFrame = stellarData[k].nextFrame;
                                    if ((stellarData[k].flags2 & kSpobRandomFrames) != 0) {
                                        do {
                                            stellarData[k].nextFrame =
                                                Rand(stellarData[k].numFrames - open) + open;
                                        } while (open == stellarData[k].nextFrame
                                                 || stellarData[k].nextFrame == stellarData[k].curFrame);
                                    } else {
                                        stellarData[k].nextFrame =
                                            ((stellarData[k].nextFrame + 1)
                                                    % stellarData[k].numFrames);
                                        if (stellarData[k].nextFrame < open)
                                            stellarData[k].nextFrame = open + 1;
                                    }
                                } else {
                                    stellarData[k].curFrame = open;
                                }
                            } else {
                                stellarData[k].curFrame = stellarData[k].nextFrame;
                                if ((stellarData[k].flags2 & kSpobRandomFrames) != 0) {
                                    do {
                                        stellarData[k].nextFrame =
                                            Rand(stellarData[k].numFrames - open) + open;
                                    } while (stellarData[k].nextFrame
                                             == stellarData[k].curFrame);
                                } else {
                                    stellarData[k].nextFrame =
                                        ((stellarData[k].nextFrame + 1)
                                                % stellarData[k].numFrames);
                                    if (stellarData[k].nextFrame < open)
                                        stellarData[k].nextFrame = open;
                                }
                            }
                        }
                    } else if (delay = stellarData[k].animDelay, stellarData[k].frameTimer >= delay) {
                        stellarData[k].frameTimer = 0.0F;
                        if (stellarData[k].curFrame >= open) {
                            if ((stellarData[k].flags2 & kSpobRandomFrames) != 0) {
                                stellarData[k].curFrame = open - 1;
                            } else if (stellarData[k].curFrame < stellarData[k].numFrames - 1) {
                                stellarData[k].curFrame += 1;
                            } else {
                                stellarData[k].curFrame = open - 1;
                            }
                        } else if (stellarData[k].curFrame > 0) {
                            stellarData[k].curFrame -= 1;
                        }
                    }
                    SWSetCurrentFrameIndex(stellarSprite[i],
                                           stellarData[k].curFrame);
                } else {
                    if (stellarData[k].curFrame == 0
                        && stellarData[k].frame0Bias > 1)
                        j = stellarData[k].animDelay * stellarData[k].frame0Bias;
                    else
                        j = stellarData[k].animDelay;
                    if (stellarData[k].frameTimer >= j) {
                        stellarData[k].frameTimer = 0.0F;
                        if ((stellarData[k].flags2 & kSpobFrame0Between) != 0) {
                            if (stellarData[k].curFrame == 0) {
                            stellarData[k].curFrame = stellarData[k].nextFrame;
                            if ((stellarData[k].flags2 & kSpobRandomFrames) != 0) {
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
                            if ((stellarData[k].flags2 & kSpobRandomFrames) != 0) {
                                do {
                                    stellarData[k].nextFrame =
                                        Rand(stellarData[k].numFrames);
                                } while (stellarData[k].nextFrame
                                         == stellarData[k].curFrame);
                            } else {
                                stellarData[k].nextFrame =
                                    ((stellarData[k].nextFrame + 1)
                                            % stellarData[k].numFrames);
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
#define kShotSpinRange          0x24

/* one live shot, stride 0x48 in shotData; only the fields this TU writes */

/* HandleStellarWeapons (mac _HandleStellarWeapons). STATE: MATCH 100.0%, exe 540 / obj 540,
   relocregcheck 0 of 112. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0042d890
// MAC: _HandleStellarWeapons i386 0x0002ef3a ppc 0x00033eac
void HandleStellarWeapons(void)
{
    FloatPoint pos;
    short i;
    short j;
    short k;
    short stel;
    short reload;
    short target;
    float dist;
    float range;
    float best;
    unsigned char disableOnly;

    if (gameFrozen != 0)
        return;

    for (i = 0; i < kSystemNavSlots; i++) {
        stel = systemData[shipData->curSystem].navDefault[i];
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
            range = weaponStats[stellarData[stel].weapon].totalRange;
            range = range * range;
            if ((weaponStats[stellarData[stel].weapon].flags2 & kWeapNotDestroying) != 0
                    || weaponStats[stellarData[stel].weapon].damage[0] == 0)
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
                if (dist <= range) {
                    if (disableOnly != 0 && IsDisabled(&shipData[k]))
                        continue;
                    if (dist < best || target == -1) {
                        target = k;
                        best = dist;
                    }
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
            shotData[j].life = weaponStats[shotData[j].weapType].lifetime;
            shotData[j].subDepth = 0;
            shotData[j].murk = 0;
            if ((weaponStats[shotData[j].weapType].flags & kWeapCycleFromFirstFrame) != 0)
                shotData[j].frame = 0;
            else
                shotData[j].frame = Rand(kShotSpinRange);
            shotData[j].frameTimer = 0;
            shotData[j].trailCount = 0;
            if (weaponStats[shotData[j].weapType].decay > 0)
                shotData[j].trailTimer = 0.0f;
            else
                shotData[j].trailTimer = -1.0f;
            shotData[j].curSystem = shipData->curSystem;
            shotData[j].guidanceState = 0;
            if (weaponStats[shotData[j].weapType].durability > 0)
                shotData[j].energy = weaponStats[shotData[j].weapType].durability;
            else
                shotData[j].energy = 0;
            shotData[j].angle = CalcSpobLeadAngle(&stellarData[stel], &shipData[target]);
            if (weaponStats[shotData[j].weapType].inaccuracy > 0) {
                shotData[j].angle += Rand(weaponStats[shotData[j].weapType].inaccuracy * 2)
                       - weaponStats[shotData[j].weapType].inaccuracy;
                if (shotData[j].angle < 0.0f)
                    shotData[j].angle += kFullCircleF;
                if (shotData[j].angle >= kFullCircleF)
                    shotData[j].angle -= kFullCircleF;
            }
            Accel(shotData[j].angle,
                  weaponStats[shotData[j].weapType].speed100,
                  &shotData[j].vel);
            for (k = 0; k < kWeapGlowSlots; k++) {
                if (weaponStats[shotData[j].weapType].jamVuln[k] > 0)
                    shotData[j].glow[k] =
                        Rand(weaponStats[shotData[j].weapType].jamVuln[k] + 1);
                else
                    shotData[j].glow[k] = 0;
            }
            PutSpriteInLayer(shotData[j].sprite, shotLayer);
            SWSetSpriteCollideProc(shotData[j].sprite, 0);
            SWCloneFramesFromMaster(shotData[j].sprite,
                         masterShotSprite[weaponStats[shotData[j].weapType].graphic]);

            reload = weaponStats[stellarData[stel].weapon].reload;
            if (weaponStats[stellarData[stel].weapon].burstCount > 0) {
                stellarData[stel].burstShots++;
                if (stellarData[stel].burstShots
                        >= WeaponBurstInterval(stellarData[stel].weapon, 1)) {
                    stellarData[stel].burstShots = 0;
                    reload = weaponStats[stellarData[stel].weapon].burstReload;
                }
            }
            stellarData[stel].reloadTimer = reload;
            PlaySoundDistance(-1, shotSnd[weaponStats[shotData[j].weapType].sound], 5,
                            &shotData[j].pos, &shipData->pos);
        } else {
            stellarData[stel].reloadTimer -= gSpeedMult;
        }
    }
}

#define kMurkBase           32.0F
#define kMurkSpan           16.0F
#define kSlotFree           (-1.0)

#define kStarSlots          kMaxStars
#define kTranslucencyOpaque 0x20
#define kAlphaMax           0x1d
#define kAlphaMin           2
#define kInterferenceScale  0.9      /* a pooled double */


/* HandleExplods. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0042e160
// MAC: _HandleExplods i386 0x0002f5d2 ppc 0x000346c8
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
                        explodType[explodData[i].type].frameRate * gSpeedMult;
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
                        explodType[explodData[i].type].frameRate * gSpeedMult;
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
// MAC: _HandleStars i386 0x0002f87d ppc 0x00034a14
void HandleStars(void)
{
    short i;
    short edgeY;
    short viewW;
    short viewH;
    short w;
    short h;
    short edgeX;
    short alpha;
    short interference;

    viewW = gSpriteWorld->windRect.right - gSpriteWorld->windRect.left;
    viewH = gSpriteWorld->windRect.bottom - gSpriteWorld->windRect.top;
    interference = systemData[shipData[0].curSystem].murk;
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
            if (doubleTime) {
                w = GetSpriteXSize(starData[i].sprite);
                h = GetSpriteYSize(starData[i].sprite);
            } else {
                w = GetSpriteXSize(starData[i].sprite);
                h = GetSpriteYSize(starData[i].sprite);
            }
            w = GetSpriteXSize(starData[i].sprite);
            h = GetSpriteYSize(starData[i].sprite);
            edgeX = w / 2 + starData[i].sprite->destFrameRect.left;
            edgeY = h / 2 + starData[i].sprite->destFrameRect.top;
            if (edgeX < -w * 2) {
                starData[i].pos.x = starData[i].pos.x + (viewW + w * 2);
                starData[i].pos.y = shipData[0].pos.y + Rand(viewH) - viewH / 2;
            } else if (edgeX > viewW + w * 2) {
                starData[i].pos.x = starData[i].pos.x - (viewW + w * 2);
                starData[i].pos.y = shipData[0].pos.y + Rand(viewH) - viewH / 2;
            }
            if (edgeY < -h * 2) {
                starData[i].pos.y = starData[i].pos.y + (viewH + h * 2);
                starData[i].pos.x = shipData[0].pos.x + Rand(viewW) - viewW / 2;
            } else if (edgeY > viewH + h * 2) {
                starData[i].pos.y = starData[i].pos.y - (viewH + h * 2);
                starData[i].pos.x = shipData[0].pos.x + Rand(viewW) - viewW / 2;
            }
            edgeX = screenCenter.h + (short)(starData[i].pos.x - shipData[0].pos.x);
            edgeY = screenCenter.v + (short)(starData[i].pos.y - shipData[0].pos.y);
            SWSetSpriteLocation(starData[i].sprite, edgeX - w, edgeY - h);
        } else {
            SWSetSpriteVisible(starData[i].sprite, 0);
        }
    }
}

#define kNavTargetCorners   4
#define kNoSpriteHalfSize   0x40

#define kFrameUnvisited     0
#define kFrameVisited       4

/* only the two fields this function reads */


/* HandleNavTargetSprites (mac _HandleNavTargetSprites). STATE: MATCH 225/225, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0042eac0
// MAC: _HandleNavTargetSprites i386 0x0002fcaf ppc 0x00034f18
void HandleNavTargetSprites(void)
{
#ifdef V111 /* V111 spelling */
    int i;
#else
    short i;
#endif
    short frame;
    short size;
    short half;
    short x;
    short y;

    if (shipData[0].targetStellar != -1 && shipData[0].navTargetKind == kNavTargetStellar) {
        if (navTargetExpand > 0.0F)
            navTargetExpand = navTargetExpand - gSpeedMult * kExpandRate;
        if (navTargetExpand < 0.0F)
            navTargetExpand = 0.0F;
        for (i = 0; i < kNavTargetCorners; i++)
            SWSetSpriteVisible(navTargetSprite[i], 1);
        if (stellarData[shipData[0].targetStellar].dominated != 0)
            frame = kFrameVisited;
        else
            frame = kFrameUnvisited;
        for (i = 0; i < kNavTargetCorners; i++)
            SWSetCurrentFrameIndex(navTargetSprite[i], i + frame);
#if V111 /* V111 spelling */
        x = stellarData[shipData[0].targetStellar].pos.h
            - (short)shipData[0].pos.x;
        x = x + screenCenter.h;
        y = stellarData[shipData[0].targetStellar].pos.v
            - (short)shipData[0].pos.y;
        y = y + screenCenter.v;
        half = kNoSpriteHalfSize;
        for (i = 0; i < kStellarSprites; i++) {
            if (stellarSprite[i]->isVisible != 0
                && stellarSprite[i]->userData == shipData[0].targetStellar) {
                half = (GetSpriteXSize(stellarSprite[i]) > GetSpriteYSize(stellarSprite[i])
                        ? GetSpriteXSize(stellarSprite[i])
                        : GetSpriteYSize(stellarSprite[i])) / 2;
                break;
            }
        }
#else
        x = screenCenter.h + (stellarData[shipData[0].targetStellar].pos.h
            - (short)shipData[0].pos.x);
        y = screenCenter.v + (stellarData[shipData[0].targetStellar].pos.v
            - (short)shipData[0].pos.y);
        half = kNoSpriteHalfSize;
        for (frame = 0; frame < kStellarSprites; frame++) {
            if (stellarSprite[frame]->isVisible != 0
                && stellarSprite[frame]->userData == shipData[0].targetStellar) {
                half = (GetSpriteXSize(stellarSprite[frame]) > GetSpriteYSize(stellarSprite[frame])
                        ? GetSpriteXSize(stellarSprite[frame])
                        : GetSpriteYSize(stellarSprite[frame])) / 2;
                break;
            }
        }
#endif
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
// MAC: _HandleWeapTargetSprites i386 0x0002ff1a ppc 0x00035200
void HandleWeapTargetSprites(void)
{
    short i;
    short frame;
    short x;
    short y;
    short half;

    if (shipData[0].targetShip != -1) {
        if (weapTargetExpand > 0.0F)
            weapTargetExpand = weapTargetExpand - gSpeedMult * kExpandRate;
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
            if (shipData[shipData[0].targetShip].parentShip > 0
                && shipData[shipData[shipData[0].targetShip].parentShip].parentShip == 0)
                frame = kFrameAlly;
        }
        for (i = 0; i < kWeapTargetCorners; i++)
            SWSetCurrentFrameIndex(weapTargetSprite[i], i + frame);
        x = (short)(shipData[shipData[0].targetShip].pos.x - shipData[0].pos.x)
            + screenCenter.h;
        y = (short)(shipData[shipData[0].targetShip].pos.y - shipData[0].pos.y)
            + screenCenter.v;
#ifdef V111 /* V111 spelling */
        half = (GetSpriteXSize((SpriteRec *)shipData[shipData[0].targetShip].sprite)
                    > GetSpriteYSize((SpriteRec *)shipData[shipData[0].targetShip].sprite)
                ? GetSpriteXSize((SpriteRec *)shipData[shipData[0].targetShip].sprite)
                : GetSpriteYSize((SpriteRec *)shipData[shipData[0].targetShip].sprite)) / 2
               + (short)weapTargetExpand;
#else
        if (GetSpriteXSize((SpriteRec *)shipData[shipData[0].targetShip].sprite)
                > GetSpriteYSize((SpriteRec *)shipData[shipData[0].targetShip].sprite))
            half = GetSpriteXSize((SpriteRec *)shipData[shipData[0].targetShip].sprite);
        else
            half = GetSpriteYSize((SpriteRec *)shipData[shipData[0].targetShip].sprite);
        half = half / 2;
        half += (short)weapTargetExpand;
#endif
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
// MAC: _HandleComm i386 0x000301b9 ppc 0x00035524
void HandleComm(void)
{
    if (commCount >= 0) {
        commCount -= 1;
        if (commCount <= 0) {
            SWSetPortToBackground(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&commRect);
            ForeColor(blackColor);
            SWSetPortToWindow(gSpriteWorld);
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                     &commRect, &commRect, 0, NULL);
        }
    }
    if (showFloatingMap > 0) {
        if (ReadTick() > floatingMapStartTime + kJumpTimeoutTicks)
            showFloatingMap = 0;
    }
}

/* shotData, stride 0x48 */


#ifdef MACBUILD
#define FloatSqrt(x) sqrt(x)
#else
static __inline float FloatSqrt(float x)
{
    return FUN_004eed20(x);
}
#endif

#define kMaxCoronaFrames        0x10

/* HandleBeams (mac _HandleBeams). STATE: MATCH 100.0% (1667 exact, 335 reloc-equiv);
   exe 2002 / obj 2002; byte-identical yes; relocregcheck 0 of 335. 1.0.10 MATCH 1869 of 1869
   on both fragments, padded. OPEN: none. */
// FUNCTION: EVNOVA 0x0042f270
// MAC: _HandleBeams i386 0x00030295 ppc 0x0003561c
void HandleBeams(unsigned char advance)
{
    FloatPoint off;
    FloatPoint drawPt;
    FloatPoint muzzle;
#ifdef MAC1010 /* 1.0.10 spelling */
    Boolean notSelf;
#endif
    short i;
    short roid;
    short j;
    short angle;
#ifdef MAC1010 /* 1.0.10 spelling */
    short massDmg;
#else
    short impact;
#endif
    short best;
    short bestRoid;
    int32_t range;
    int32_t bestDist;
#ifdef MAC1010 /* 1.0.10 spelling */
    int32_t jOff;
#else
    Boolean notSelf;
#endif
    Boolean corona;
    Boolean isAlly;
    Boolean reachable;
    Boolean eligible;
    short ownerType;
    short shipAngle;
    short size;
    int32_t dist;
#ifdef MAC1010 /* 1.0.10 spelling */
    int32_t len;
    short energyDmg;
    short impact;
#else
    short massDmg;
    short energyDmg;
    int32_t len;
#endif
    short hitAngle;
    short parent;
    short diff;

    for (i = 0; i < kMaxBeams; i++) {
        if (beamData[i].state > -1 && beamData[i].owner != -1) {
            corona = 0;
            if (beamData[i].state == 0
                && weaponStats[beamData[i].weapType].decay > 0) {
                beamData[i].frame++;
                if ((short)(weaponStats[beamData[i].weapType].coronaFalloff
                    + beamData[i].frame) < kMaxCoronaFrames)
                    corona = 1;
            }
            if (!corona && gameFrozen == 0
                && (beamData[i].state > 0 || advance))
                beamData[i].state--;
            if (killShots)
                beamData[i].state = -1;
            if (advance) {
                beamData[i].lastHead = beamData[i].head;
                beamData[i].lastTail = beamData[i].tail;
            }
            if (advance) {
                angle = shipData[beamData[i].owner].sprite->curFrameIndex
                        % shipClass[shipData[beamData[i].owner].type].framesPer;
                angle *= 360.0 / (float)shipClass[shipData[beamData[i].owner].type].framesPer;
                shipAngle = angle;

                if (beamData[i].target != -1
                    && weaponStats[beamData[i].weapType].guidance != kGuidanceBeam) {
                    off.x = off.y = 0.0f;
                    if (beamData[i].owner != -1
                        && beamData[i].lane >= 0 && beamData[i].lane <= 3
                        && beamData[i].mount >= 0 && beamData[i].mount <= 3) {
                        ownerType = shipData[beamData[i].owner].type;
                        Accel(angle,
                              shipClass[ownerType]
                                  .weapPos[beamData[i].mount][beamData[i].lane].v,
                              &off);
                        Accel((angle + kQuarterCircle) % kFullCircle,
                              shipClass[ownerType]
                                  .weapPos[beamData[i].mount][beamData[i].lane].h,
                              &off);
                        if (off.y < 0.0) {
                            off.x = off.x
                                * shipClass[ownerType].compress[0].x;
                            off.y = off.y
                                * shipClass[ownerType].compress[0].y;
                        } else {
                            off.x = off.x
                                * shipClass[ownerType].compress[1].x;
                            off.y = off.y
                                * shipClass[ownerType].compress[1].y;
                        }
                        off.y -= shipClass[ownerType]
                                     .weapPosZ[beamData[i].mount][beamData[i].lane];
                    }
                    off.x += shipData[beamData[i].owner].pos.x;
                    off.y += shipData[beamData[i].owner].pos.y;

                    if (weaponStats[beamData[i].weapType].guidance == kGuidancePDBeam
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

                if (weaponStats[beamData[i].weapType].inaccuracy > 0
                    && weaponStats[beamData[i].weapType].guidance != kGuidancePDBeam)
                    angle += Rand(weaponStats[beamData[i].weapType].inaccuracy * 2)
                             - weaponStats[beamData[i].weapType].inaccuracy;

                muzzle.x = shipData[beamData[i].owner].pos.x;
                muzzle.y = shipData[beamData[i].owner].pos.y;
                if (beamData[i].owner != -1
                    && beamData[i].lane >= 0 && beamData[i].lane <= 3
                    && beamData[i].mount >= 0 && beamData[i].mount <= 3) {
                    off.x = off.y = 0.0f;
                    ownerType = shipData[beamData[i].owner].type;
                    Accel(shipAngle,
                          shipClass[ownerType]
                              .weapPos[beamData[i].mount][beamData[i].lane].v,
                          &off);
                    Accel((shipAngle + kQuarterCircle) % kFullCircle,
                          shipClass[ownerType]
                              .weapPos[beamData[i].mount][beamData[i].lane].h,
                          &off);
                    if (off.y < 0.0) {
                        off.x = off.x
                            * shipClass[ownerType].compress[0].x;
                        off.y = off.y
                            * shipClass[ownerType].compress[0].y;
                    } else {
                        off.x = off.x
                            * shipClass[ownerType].compress[1].x;
                        off.y = off.y
                            * shipClass[ownerType].compress[1].y;
                    }
                    off.y -= shipClass[ownerType]
                                 .weapPosZ[beamData[i].mount][beamData[i].lane];
                    muzzle.x += off.x;
                    muzzle.y += off.y;
                }

                drawPt.x = muzzle.x - shipData[0].pos.x + screenCenter.h;
                drawPt.y = muzzle.y - shipData[0].pos.y + screenCenter.v;
                beamData[i].head.h = drawPt.x;
                beamData[i].head.v = drawPt.y;

                if (beamData[i].state >= 0) {
                    if (weaponStats[beamData[i].weapType].guidance == kGuidancePDBeam
                        && beamData[i].targetIsShot == 1) {
                        if (beamData[i].target != -1) {
                            beamData[i].tail.h = shotData[beamData[i].target].pos.x
                                                 - shipData[0].pos.x + screenCenter.h;
                            beamData[i].tail.v = shotData[beamData[i].target].pos.y
                                                 - shipData[0].pos.y + screenCenter.v;
                            massDmg = weaponStats[beamData[i].weapType].damage[0];
                            energyDmg = weaponStats[beamData[i].weapType].damage[1] / 2;
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
                        if (weaponStats[beamData[i].weapType].flags2 & kWeapUsesShipSprite)
                            shipData[beamData[i].owner].weapGlow = 32.0f;
                        shipData[beamData[i].owner].fireTick = ReadTick();

                        best = -1;
                        for (j = 0; j < kMaxShips; j++) {
                            notSelf = 1;
                            eligible = 1;
#ifdef MAC1010
                            jOff = (int32_t)shipData; /* 1.0.10 pad: shipData read ahead of the offset; the parentShip test reuses the load */
                            jOff = j * sizeof(ShipObj); /* 1.0.10 pad: the stride product first defined in a named local, so it colours in the named band */
#endif
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
                                    & kDudeUntouchable))
                                eligible = 0;
                            if (eligible && isAlly) {
                                if (shipData[beamData[i].owner].dude != -1) {
                                    if (dudeData[shipData[beamData[i].owner].dude].flags
                                        & kDudeUntouchable)
                                        eligible = 0;
                                } else if (shipData[beamData[i].owner].parentShip != -1
                                           && shipData[shipData[beamData[i].owner].parentShip]
                                                  .dude != -1
                                           && (dudeData[shipData[shipData[beamData[i].owner]
                                                   .parentShip].dude].flags
                                               & kDudeUntouchable))
                                    eligible = 0;
                            }
                            if (j == shipData[beamData[i].owner].parentShip)
                                notSelf = 0;
                            if ((weaponStats[beamData[i].weapType].flags & kWeapBigSmoke)
                                != (shipClass[size = shipData[j].type].flags & kShipPlanetType)) /* BUG: Flags, not Flags2 */
                                eligible = 0;

                            if (j != beamData[i].owner && notSelf && eligible
                                && shipData[j].active
                                && shipData[j].curSystem
                                   == shipData[beamData[i].owner].curSystem
                                && shipData[j].type != kShipTypeEscapePod) {
                                hitAngle = GetMasterShipSpriteXSize(size) * 0.66;
                                range = weaponStats[beamData[i].weapType].beamLength
                                        + hitAngle / 2;
                                if (XYFloatSquaredDist(&shipData[j].pos,
                                                       &muzzle) <= range * range) {
                                    off.x = shipData[beamData[i].owner].pos.x;
                                    off.y = shipData[beamData[i].owner].pos.y;
                                    diff = CalcInterceptAngle(&muzzle,
                                        &shipData[j].pos);
                                    hitAngle = hitAngle * 10 / 32;
                                    if (abs(diff - angle) <= hitAngle) {
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
#ifdef MACBUILD
                            off = muzzle;
#else
                            off.x = muzzle.x;
                            off.y = muzzle.y;
#endif
                            len = FloatSqrt(XYFloatSquaredDist(&muzzle,
                                      &shipData[best].pos))
                                  - GetMasterShipSpriteXSize(shipData[best].type) * 0.2;
                            Accel(angle, len, &off);
                            CreateExplosion(off,
                                weaponStats[beamData[i].weapType].explodType,
                                weaponStats[beamData[i].weapType].blastRadius, 1);
                            IonizeShip(&shipData[best],
                                &weaponStats[beamData[i].weapType], 0);
                            if (weaponStats[beamData[i].weapType].hitParticles > 0)
                                SpawnParticles(off.x, off.y,
                                    weaponStats[beamData[i].weapType].hitPartVel, 0x19,
                                    weaponStats[beamData[i].weapType].hitPartLife,
                                    weaponStats[beamData[i].weapType].hitPartLife * 1.25,
                                    weaponStats[beamData[i].weapType].hitPartColor, 0x20,
                                    weaponStats[beamData[i].weapType].hitParticles, 0);

                            massDmg = weaponStats[beamData[i].weapType].damage[0];
                            energyDmg = weaponStats[beamData[i].weapType].damage[1];
                            impact = weaponStats[beamData[i].weapType].impact;
                            if (doubleTime) {
                                massDmg *= 2;
                                energyDmg *= 2;
                            }
                            if (shipData[best].persType != kPersCaptainHector) {
                                if (impact < 0 && beamData[i].owner != -1) {
                                    if (shipClass[shipData[best].type].mass > 0
                                        && (shipClass[shipData[best].type].flags
                                            & kShipPlanetType) == 0) {
                                        if (shipClass[shipData[beamData[i].owner].type].mass
                                                >= shipClass[shipData[best].type].mass * 0.75) {
                                            shipData[best].tractorShip = beamData[i].owner;
                                            shipData[best].tractorTick = ReadTick();
                                        } else {
                                            if (shipData[beamData[i].owner].tractorShip == -1)
                                                shipData[beamData[i].owner].tractorShip =
                                                    beamData[i].owner;
                                            shipData[beamData[i].owner].tractorTick = ReadTick();
                                            if (FloatAbs(muzzle.x - shipData[best].pos.x) >= 50.0f)
                                                goto shipKnock;
                                            if (FloatAbs(muzzle.y - shipData[best].pos.y) >= 50.0f)
                                                goto shipKnock;
                                            goto shipKnockDone;
                                        shipKnock:
                                            if (shipClass[shipData[beamData[i].owner]
                                                       .type].mass > 0
                                                && (shipClass[shipData[beamData[i].owner]
                                                        .type].flags & kShipPlanetType) == 0) {
                                                hitAngle = CalcInterceptAngle(
                                                    &shipData[best].pos, &muzzle);
                                                AdjustedAccel(hitAngle,
                                                    impact / (float)shipClass[
                                                        shipData[beamData[i].owner].type].mass,
                                                    &shipData[beamData[i].owner].vel,
                                                    shipClass[shipData[beamData[i].owner]
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
                                            weaponStats[beamData[i].weapType].flags
                                                & kWeapPassesShields, 1);
                                    else
                                        DamageShip(&shipData[best], &muzzle, impact,
                                            massDmg, energyDmg, beamData[i].owner, 1, 0,
                                            beamData[i].disableOnly, 0,
                                            weaponStats[beamData[i].weapType].flags
                                                & kWeapPassesShields, 1);
                                } else {
                                    if (best == shipData[beamData[i].owner].targetShip)
                                        DamageShip(&shipData[best], &muzzle, impact,
                                            massDmg, energyDmg, beamData[i].owner, 1, 1,
                                            beamData[i].disableOnly, 0,
                                            weaponStats[beamData[i].weapType].flags
                                                & kWeapPassesShields, 1);
                                    else
                                        DamageShip(&shipData[best], &muzzle, impact,
                                            massDmg, energyDmg, beamData[i].owner, 1, 0,
                                            beamData[i].disableOnly, 0,
                                            weaponStats[beamData[i].weapType].flags
                                                & kWeapPassesShields, 1);
                                }
                            }
                            if (best == 0)
                                statusRedraw[kRedrawShield] = 1;
                        } else {
                            bestRoid = -1;
                            if ((weaponStats[beamData[i].weapType].seeker
                                 & kWeapSeekPassesAsteroids) == 0) {
                                for (roid = 0; roid < kMaxAsteroids; roid++) {
                                    if (asteroidData[roid].active) {
                                        size = GetSpriteYSize(
                                            masterAsteroidSprite[asteroidData[roid].type]);
                                        range = weaponStats[beamData[i].weapType].beamLength
                                            + size / 2;
                                        if (XYFloatSquaredDist(
                                                &asteroidData[roid].pos,
                                                &muzzle) <= range * range) {
                                            off.x = muzzle.x;
                                            off.y = muzzle.y;
                                            diff = CalcInterceptAngle(&muzzle,
                                                &asteroidData[roid].pos);
                                            hitAngle = size * 10 / 32;
                                            if (abs(diff - angle) <= hitAngle) {
                                                dist = XYFloatSquaredDist(
                                                    &shipData[roid].pos,
                                                    &muzzle);
                                                if (dist < bestDist || bestRoid == -1) {
                                                    bestDist = dist;
                                                    bestRoid = roid;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            if (bestRoid != -1) {
#ifdef MACBUILD
                                off = muzzle;
#else
                                off.x = muzzle.x;
                                off.y = muzzle.y;
#endif
                                len = FloatSqrt(XYFloatSquaredDist(&muzzle,
                                          &asteroidData[bestRoid].pos))
                                      - GetSpriteYSize(
                                          masterAsteroidSprite[asteroidData[bestRoid].type])
                                        * 0.2;
                                Accel(angle, len, &off);
                                CreateExplosion(off,
                                    weaponStats[beamData[i].weapType].explodType,
                                    weaponStats[beamData[i].weapType].blastRadius, 1);
                                if (weaponStats[beamData[i].weapType].hitParticles > 0)
                                    SpawnParticles(off.x, off.y,
                                        weaponStats[beamData[i].weapType].hitPartVel, 0x19,
                                        weaponStats[beamData[i].weapType].hitPartLife,
                                        weaponStats[beamData[i].weapType].hitPartLife * 1.25,
                                        weaponStats[beamData[i].weapType].hitPartColor, 0x20,
                                        weaponStats[beamData[i].weapType].hitParticles, 0);

                                massDmg = weaponStats[beamData[i].weapType].damage[0];
                                impact = weaponStats[beamData[i].weapType].impact;
                                if (doubleTime)
                                    massDmg *= 2;
                                if (weaponStats[beamData[i].weapType].flags2
                                    & kWeapBigRockDamage)
                                    massDmg *= 10;
                                asteroidData[bestRoid].strength -= massDmg;
                                if (asteroidData[bestRoid].strength < 0)
                                    DestroyAsteroid(&asteroidData[bestRoid]);
                                else if (impact != 0
                                         && asteroidType[asteroidData[bestRoid].type].mass > 0) {
                                    if (impact < 0 && beamData[i].owner != -1
                                        && shipClass[shipData[beamData[i].owner].type]
                                               .mass
                                           < asteroidType[asteroidData[bestRoid].type].mass * 0.5
                                        && shipClass[shipData[beamData[i].owner].type]
                                               .mass > 0
                                        && (shipClass[shipData[beamData[i].owner].type]
                                                .flags & kShipPlanetType) == 0) {
                                        if (FloatAbs(muzzle.x - asteroidData[bestRoid].pos.x)
                                                >= 50.0f)
                                            goto roidKnock;
                                        if (FloatAbs(muzzle.y - asteroidData[bestRoid].pos.y)
                                                >= 50.0f)
                                            goto roidKnock;
                                        goto roidKnockDone;
                                    roidKnock:
                                        if (shipClass[shipData[beamData[i].owner]
                                                   .type].mass > 0
                                            && (shipClass[shipData[beamData[i].owner]
                                                    .type].flags & kShipPlanetType) == 0) {
                                            hitAngle = CalcInterceptAngle(
                                                &asteroidData[bestRoid].pos, &muzzle);
                                            AdjustedAccel(hitAngle,
                                                impact / (float)shipClass[
                                                    shipData[beamData[i].owner].type].mass,
                                                &shipData[beamData[i].owner].vel,
                                                shipClass[shipData[beamData[i].owner]
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
                                                asteroidData[bestRoid].type].mass,
                                            &asteroidData[bestRoid].vel, 2.0f);
                                        SpeedLimit(
                                            &asteroidData[bestRoid].vel, 2.0f);
                                    }
                                }
                            } else {
                                len = weaponStats[beamData[i].weapType].beamLength;
                            }
                        }

#ifdef MACBUILD
                        off = drawPt;
#else
                        off.x = drawPt.x;
                        off.y = drawPt.y;
#endif
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
#undef roid

#define kVoiceBaseID 1000


/* HandleVoiceSound. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004311f0
// MAC: _HandleVoiceSound i386 0x00031e20 ppc 0x0003728c
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
                        voiceSnd = ST_LoadSndResource(id);
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
// MAC: _KillVoiceSound i386 0x00031f95 ppc 0x0003743c
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
// MAC: _WarpUpCallback i386 0x00031ff3 ppc 0x000374c4
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
// MAC: _HandleGlobalPriceFlux i386 0x00032054 ppc 0x00037554
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
// MAC: _HandleLocalPriceFlux i386 0x000320a6 ppc 0x000375f4
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


/* HandleShotGuidance (mac _HandleShotGuidance). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00431530
// MAC: _HandleShotGuidance i386 0x000320dc ppc 0x00037640
void HandleShotGuidance(ShotObj *shot)
{
    short i;
    short g;
    FloatPoint prev;
    float turn;
    float off;
    short dir;
    short diff;
    short lock;
    short delta;
    FloatPoint v;

    if (weaponStats[shot->weapType].guidance == kGuidancePointDef)
        return;

    if (shot->guidanceState == kShotStateNormal) {
        if (weaponStats[shot->weapType].guidance == kGuidanceHoming) {
            if (shot->target == -1 ||
                (shot->target != -1 && shipData[shot->target].active != 0 &&
                 shipData[shot->target].curSystem == shot->curSystem)) {
                if (shot->target != -1)
                    dir = CalcInterceptAngle(&shot->pos,
                                             &shipData[shot->target].pos);
                else
                    dir = shot->angle;
                turn = weaponStats[shot->weapType].guidedTurn;
                if (weaponStats[shot->weapType].lifetime - (double)shot->life > 15.0 * gSpeedMult) {
                    if (shot->target != -1) {
                        for (g = 0; g <= 3; g++) {
                            if (shot->glow[g] > 0) {
                                if (ShipECM(&shipData[shot->target], g) >
                                    100 - shot->glow[g]) {
                                    if (turn > 0.0f) {
                                        if ((weaponStats[shot->weapType].seeker &
                                             kWeapSeekTurnsAwayJammed) != 0)
                                            turn *= -1.0f;
                                        else
                                            turn = 0.0f;
                                    }
                                    if ((weaponStats[shot->weapType].seeker &
                                         kWeapSeekAttacksParent) != 0 && Rand(500) == 0 &&
                                        (shot->owner >= 0 && shot->owner < kMaxShips)) {
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
                            if ((weaponStats[shot->weapType].seeker &
                                 kWeapSeekAttacksParent) != 0 && Rand(1000) == 0 &&
                                shot->owner != -1) {
                                shot->target = shot->owner;
                                shot->owner = -1;
                            }
                        }
                        if ((weaponStats[shot->weapType].seeker & kWeapSeekLosesLock) != 0 &&
                            shot->target != -1 &&
                            abs((short)(shipData[shot->target].pos.x - shot->pos.x)) < 250 &&
                            abs((short)(shipData[shot->target].pos.y - shot->pos.y)) < 250) {
#ifdef MAC1010
                            lock = abs((CalcInterceptAngle(&shot->pos, &shipData[shot->target].pos) -
                                shot->angle)) % 360;
#else
                            off = CalcInterceptAngle(&shot->pos,
                                  &shipData[shot->target].pos) -
                                  shot->angle;
#ifdef MACBUILD
                            lock = abs(off) % 360;
#else
                            off = fabs(off);
                            lock = off;
                            lock = lock % 360;
#endif
#endif
                            if (lock > 45)
                                shot->target = -1;
                        }
                    }
                    diff = abs(AngularDifference(dir, shot->angle));
                    if (diff > abs((short)turn)) {
                        delta = dir - (int)shot->angle;
                        if (delta >= 360)
                            delta -= 360;
                        if (delta < 0)
                            delta += 360;
                        if (delta > 180)
                            shot->angle -= turn * gSpeedMult;
                        else
                            shot->angle += turn * gSpeedMult;
                    }
                }
                if (shot->angle >= 360.0f)
                    shot->angle = shot->angle - 360.0f;
                if (shot->angle < 0.0f)
                    shot->angle += 360.0f;
                shot->vel.x = shot->vel.y = 0.0f;
                Accel(shot->angle, weaponStats[shot->weapType].speed100, &shot->vel);
                SpeedLimit(&shot->vel, weaponStats[shot->weapType].speed100);
            }
        }
        if (weaponStats[shot->weapType].guidance == kGuidanceHoming &&
            (weaponStats[shot->weapType].seeker & kWeapSeekDecoyedByAsteroids) != 0 && Rand(10) == 0) {
            for (i = 0; i < kMaxAsteroids; i++) {
                if (asteroidData[i].active != 0) {
                    if (abs((short)(asteroidData[i].pos.x - shot->pos.x)) < 200 &&
                        abs((short)(asteroidData[i].pos.y - shot->pos.y)) < 200) {
#ifdef MAC1010
                        lock = abs((CalcInterceptAngle(&shot->pos, &asteroidData[i].pos) -
                              shot->angle)) % 360;
#else
                        off = CalcInterceptAngle(&shot->pos, &asteroidData[i].pos) -
                              shot->angle;
#ifdef MACBUILD
                        lock = abs(off) % 360;
#else
                        off = fabs(off);
                        lock = off;
                        lock = lock % 360;
#endif
#endif
                        if (lock <= 15) {
                            shot->guidanceState = kShotStateRock;
                            shot->target = i;
                            break;
                        }
                    }
                }
            }
        }
    } else if (shot->guidanceState == kShotStateWobble) {
        if (weaponStats[shot->weapType].lifetime - (double)shot->life > 15.0) {
#ifdef MAC1010
            off = weaponStats[shot->weapType].guidedTurn;
            if (frameCounter % 300 < 150)
                shot->angle -= off;
            else
                shot->angle += off;
#else
            if (frameCounter % 300 < 150)
                shot->angle = shot->angle - weaponStats[shot->weapType].guidedTurn;
            else
                shot->angle = shot->angle + weaponStats[shot->weapType].guidedTurn;
#endif
        }
        if (shot->angle >= 360.0f)
            shot->angle = shot->angle - 360.0f;
        if (shot->angle < 0.0f)
            shot->angle += 360.0f;
        shot->vel.x = shot->vel.y = 0.0f;
        Accel(shot->angle, weaponStats[shot->weapType].speed100, &shot->vel);
        SpeedLimit(&shot->vel, weaponStats[shot->weapType].speed100);
        if ((weaponStats[shot->weapType].seeker & kWeapSeekAttacksParent) != 0 &&
            Rand(1000) == 0 && (shot->owner >= 0 && shot->owner < kMaxShips)) {
            shot->guidanceState = kShotStateNormal;
            shot->target = shot->owner;
            shot->owner = -1;
        }
    } else if (shot->guidanceState == kShotStateHeld) {
        dir = shot->angle;
    } else {
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
        if (weaponStats[shot->weapType].lifetime - (double)shot->life > 15.0) {
            diff = abs(AngularDifference(dir, shot->angle));
            off = weaponStats[shot->weapType].guidedTurn;
            if (diff > off) {
                delta = dir - shot->angle;
                if (delta >= 360)
                    delta -= 360;
                if (delta < 0)
                    delta += 360;
                if (delta > 180)
                    shot->angle -= weaponStats[shot->weapType].guidedTurn;
                else
                    shot->angle += weaponStats[shot->weapType].guidedTurn;
            }
        }
        if (shot->angle >= 360.0f)
            shot->angle = shot->angle - 360.0f;
        if (shot->angle < 0.0f)
            shot->angle += 360.0f;
        shot->vel.x = shot->vel.y = 0.0f;
        Accel(shot->angle, weaponStats[shot->weapType].speed100, &shot->vel);
        SpeedLimit(&shot->vel, weaponStats[shot->weapType].speed100);
    }

    if (weaponStats[shot->weapType].guidance == kGuidanceRocket) {
        prev.x = shot->vel.x;
        prev.y = shot->vel.y;
        v.x = v.y = 0.0f;
        Accel(shot->angle, weaponStats[shot->weapType].speed100, &v);
        shot->vel.x = (prev.x * 95.0f + v.x * 5.0f) * 0.01;
        shot->vel.y = (prev.y * 95.0f + v.y * 5.0f) * 0.01;
    }
    if (weaponStats[shot->weapType].guidance == kGuidanceFreefall) {
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
                shot->angle -= 1.0f;
            else
                shot->angle += 1.0f;
        }
    }
}

/* HandleStellarSystemVisibility (mac _HandleStellarSystemVisibility). STATE: MATCH 100.0%
   598/598, byte-identical yes (2608 bytes), relocregcheck 0 of 137. OPEN: none. */
// FUNCTION: EVNOVA 0x00432470
// MAC: _HandleStellarSystemVisibility i386 0x00032aa5 ppc 0x000382cc
void HandleStellarSystemVisibility(void)
{
#ifdef MAC1010 /* 1.0.10 spelling: one counter for n and j */
#define n j
#else
    short n;
#endif
    Boolean fresh;
    Boolean used[256];
    short i;
    short j;
    short sys;
    short other;
    short sprID;
    short err;

    for (j = 0; j < kMaxHyperRoute; j++) {
        i = hyperRoute[j];
        if (i != -1 && !systemData[i].loaded) {
            hyperRoute[j] = FindActiveCoLocatedSystem(i);
            if (hyperRoute[j] == -1) {
                for (sys = j; sys < kMaxHyperRoute; sys++)
                    hyperRoute[sys] = -1;
                break;
            }
        }
    }

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].prepared) {
            other = FindFirstCoLocatedSystem(i);
            if (other != i && other >= 0 && other < kMaxSystems) {
                if (systemData[i].exploreLevel < systemData[other].exploreLevel)
                    systemData[i].exploreLevel = systemData[other].exploreLevel;
            }
        }
        systemData[i].known = 0;
    }

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded && systemData[i].prepared && systemData[i].exploreLevel > 0) {
            systemData[i].known = 1;
            for (j = 0; j < kMaxSystemLinks; j++) {
                if (systemData[i].links[j] != -1) {
                    other = FindActiveCoLocatedSystem(systemData[i].links[j]);
                    if (other != -1)
                        systemData[other].known = 1;
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
                    if (systemData[shipData[0].curSystem].navDefault[j] == i) {
                        stellarData[i].system = shipData[0].curSystem;
                        break;
                    }
                }
            }
            if (stellarData[i].system >= 0 && stellarData[i].system < kMaxSystems) {
                if (systemData[stellarData[i].system].loaded) {
                    stellarData[i].loaded = 1;
                    if (stellarData[i].flags2 & kSpobAlwaysDominated)
                        stellarData[i].dominated = 1;
                }
            }
        }
        if (stellarData[i].loaded) {
            if (stellarData[i].system == shipData[0].curSystem &&
                stellarData[i].system != -1) {
                if (StellarIsDestroyed(&stellarData[i]))
                    sprID = stellarData[i].deadType;
                else
                    sprID = stellarData[i].type;
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
#ifdef MACBUILD
                    HUnlock(masterStellarSprite[i]->rleDataH);
#endif
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
            stellarData[i].system == shipData[0].curSystem &&
            stellarData[i].system != -1) {
            if (StellarIsDestroyed(&stellarData[i]))
                sprID = stellarData[i].deadType;
            else
                sprID = stellarData[i].type;
            if (masterStellarSprite[sprID] == NULL)
                LoadMasterStellarSprite(sprID);
            fresh = 0;
            if (stellarSprite[n] == NULL) {
                fresh = 1;
                err = SWCreateEmptySprite(&stellarSprite[n], NULL);
                if ((int32_t)err != 0)
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
#undef n
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* framepace -- hold the frame to a floor length and track how fast the machine is running (win 0x00432ea0). Notes: Research/notes/framepace.md */

/* HandleTimeAdjustment (mac _HandleTimeAdjustment). STATE: MATCH 100.0%, exe 105 / obj 105
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00432ea0
// MAC: _HandleTimeAdjustment i386 0x00033332 ppc 0x00038be0
void HandleTimeAdjustment(void)
{
    UnsignedWide now;
    uint32_t ms;
    float rate;

#ifdef V111    /* POTENTIAL BUG (win V111): 1.1.1 drops the 21 ms frame wait; win may run uncapped */
    Microseconds(&now);
    if (now.hi > lastFrameMicroseconds.hi)
        ms = (0xffffffff - (lastFrameMicroseconds.lo - now.lo)) / 1000;
    else
        ms = (now.lo - lastFrameMicroseconds.lo) / 1000;
    if (ms < 10 && (shipData[0].jumpTimer <= 0.0f || shipData[0].active == 0)) {
        IterateInGameEventLoop(1);
        lastEventLoopTime = ReadTick();
    }
#else
    do {
        Microseconds(&now);
        if (now.hi > lastFrameMicroseconds.hi)
            ms = (0xffffffff - (lastFrameMicroseconds.lo - now.lo)) / 1000;
        else
            ms = (now.lo - lastFrameMicroseconds.lo) / 1000;
        if (ms < 10 && (shipData[0].jumpTimer <= 0.0f || shipData[0].active == 0)) {
#if !TARGET_API_MAC_CARBON
            if (settings.yieldEvents != 0)
#endif
            {
                IterateInGameEventLoop(1);
                lastEventLoopTime = ReadTick();
            }
        }
    } while (ms <= 20);
#endif

    if (lastFrameTime == 0)
        rate = 1.0;
    else
        rate = ms * 0.03;
    if (rate >= 0.01 && rate < 10.0)
        gSpeedMult = (rate + gSpeedMult * 3.0) * 0.25;
    lastFrameTime += ms;
    lastFrameMicroseconds = now;
    if (gSpeedMult > 2.0)
        aiComplexity = 16;    /* BUG: always overwritten by the chain below */
    if (gSpeedMult > 1.7)
        aiComplexity = 8;
    else if (gSpeedMult > 1.3)
        aiComplexity = 4;
    else if (gSpeedMult > 1.0)
        aiComplexity = 2;
    else
        aiComplexity = 1;
}

#ifdef V111 /* V111 spelling */
#define JumpDrift(s, fast, push) \
    d = (ReadTick() - (s)->jumpTick) \
        * (doubleTime ? fast : 1.0) \
        * shipClass[(s)->type].mobilityFactor \
        / (WarpSoundDuration() * 0.01); \
    if (shipClass[(s)->type].mobilityFactor != 0) \
        d = d - push / shipClass[(s)->type].mobilityFactor
#else
#ifdef MAC1010 /* 1.0.10 spelling */
#define JumpDrift(s, fast, push) \
    d = (ReadTick() - (s)->jumpTick) \
        * (doubleTime ? fast : 1.0) \
        * shipClass[(s)->type].mobilityFactor \
        / (WarpSoundDuration() * 0.01); \
    if (shipClass[(s)->type].mobilityFactor != 0) \
        d = d - push / shipClass[(s)->type].mobilityFactor
#else
#define JumpDrift(s, fast, push) \
    span = WarpSoundDuration() * 0.01; \
    factor = doubleTime ? fast : 1.0; \
    { \
        double t; \
        float mobility; \
        \
        t = (ReadTick() - (s)->jumpTick) \
             * factor \
             * shipClass[(s)->type].mobilityFactor \
             / span; \
        d = t; \
        mobility = shipClass[(s)->type].mobilityFactor; \
        if (mobility != 0) \
            d = d - push / mobility; \
    }
#endif
#endif

static inline void TractorDrag(ShipObj *ship, float *drag)
{
    float vy;
    float vx;

    if (ship->tractorShip == ship->index) {
        vx = 0.0f;
        vy = 0.0f;
    } else {
        vx = shipData[ship->tractorShip].vel.x;
        vy = shipData[ship->tractorShip].vel.y;
    }
    if (ship->vel.x > vx + *drag)
        ship->vel.x -= *drag;
    if (ship->vel.x < vx - *drag)
        ship->vel.x += *drag;
    if (ship->vel.y > vy + *drag)
        ship->vel.y -= *drag;
    if (ship->vel.y < vy - *drag)
        ship->vel.y += *drag;
    if (FloatAbs(ship->vel.x - vx) < *drag)
        ship->vel.x = vx;
    if (FloatAbs(ship->vel.y - vy) < *drag)
        ship->vel.y = vy;
}

/* HandleShip (mac _HandleShip). STATE: MATCH 100.0%, exe 2290 / obj 2290; relocregcheck 0 of
   342; byte-identical: yes. OPEN: none. */
// FUNCTION: EVNOVA 0x00433050
// MAC: _HandleShip i386 0x00033581 ppc 0x00038f0c
void HandleShip(ShipObj *ship)
{
    float d;
    unsigned char msg[256];
    short dir;
    short i;
    Boolean ok;
    Boolean hailNow;
    Boolean disabled;
    Boolean cloaked;
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
    Boolean spawn;
    int32_t inRange;

    dir = 0;
    if (ship->type < 0 || ship->type >= kMaxShipTypes) {
        ship->active = 0;
        return;
    }
    if (shipClass[ship->type].techLevel == kResAbsent) {
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
                ship->weap[j].reloadTimer -= gSpeedMult;
            else
                ship->weap[j].reloadTimer = 0;
            if ((weaponStats[j].seeker & kWeapSeekNoFireIonized) && ion > 0)
                ship->weap[j].reloadTimer = 1.0f;
        }
    }
    if (ship->targetShip == -1) {
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (weaponStats[i].guidance == kGuidanceBay
                    && ship->weap[i].count > 0)
                ship->weap[i].reloadTimer = weaponStats[i].reload;
        }
    }

    if (disabled) {
        ship->vel.x *= 0.995;
        ship->vel.y *= 0.995;
        ship->accelMag *= 0.995;
        if (ship->specialMission == -1 && ship->defenseStellar == -1 && ship->parentShip == 0) {
            if (shipClass[ship->type].inherentAI <= kShipTypeBraveTrader)
                DestroyPartialFleetCargo(ship->index);
            ship->parentShip = -1;
            ship->hired = 0;
            ship->boarded = 1;
            ship->aiType = shipClass[ship->type].inherentAI;
            ShipStatsToSystemInfo();
            statusRedraw[kRedrawCargo] = 1;
        } else if (ShipCanSelfRepair(ship)) {
            if (shipClass[ship->type].flags & kShipArmorDisableTenth)
                ship->armor = ShipArmorCapacity(ship) * 0.1 + 1.0;
            else
                ship->armor = ShipArmorCapacity(ship) * 0.3333 + 1.0;
            if (shipData->targetShip == ship->index)
                statusRedraw[kRedrawTarg] = 1;
            if (ship->formerEscort >= 0) {
                ship->parentShip = 0;
                ship->shield = 0;
                ship->targetStellar = -1;
                ship->targetShip = -1;
                if (ship->formerEscort == 0) {
                    ship->aiType = kShipTypeFighter;
                    GetIndString(msg, kGameStrList, 0x80);
                    DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else {
                    if (ship->formerEscort == 1)
                        ship->hired = 1;
                    else
                        ship->hired = 0;
                    ship->aiType = kShipTypeEscort;
                    GetIndString(msg, kGameStrList, 0x7f);
                    DisplayComm((PStrChar *)msg, 0xfa, 0, kCommTextSize, &rgbWhite, 0, 0);
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
            ok = 1;
            hailNow = 0;
            if (IsDisabled(ship)) {
                if (!(personData[ship->persType].flags & kPersQuoteIfDisabled))
                    ok = 0;
            } else {
                if (personData[ship->persType].flags & kPersQuoteIfDisabled)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersQuoteOnAttack) {
                if (!IsDisabled(ship)) {
                    if (ship->hailed == 0) {
                        if (IsThreatToPlayer(ship)) {
                            ok = 1;
                            hailNow = 1;
                        } else
                            ok = 0;
                    } else
                        ok = 0;
                }
            }
            if (personData[ship->persType].flags & kPersQuoteIfGrudge) {
                if (personData[ship->persType].grudge == 0)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersQuoteIfLikes) {
                if (!AIDoesShipLikePlayer(ship))
                    ok = 0;
            }
            if (personData[ship->persType].linkMission != -1
                    && (personData[ship->persType].flags & kPersQuoteNeedsMission)) {
                offeringFromShip = 1;
                offeringShipIndex = ship->index;
                if (!IsMissionAvailable(personData[ship->persType].linkMission, 0))
                    ok = 0;
                offeringFromShip = 0;
                offeringShipIndex = -1;
            }
            if (personData[ship->persType].flags & kPersLeavesOnAccept) {
                if (AIIsShipLeaving(ship))
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersNotForWimpyFreighter) {
                if (shipClass[shipData->type].inherentAI == kShipTypeWimpyTrader)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersNotForBeefyFreighter) {
                if (shipClass[shipData->type].inherentAI == kShipTypeBraveTrader)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersNotForWarship) {
                if (shipClass[shipData->type].inherentAI >= kShipTypeWarship)
                    ok = 0;
            }
            if (personData[ship->persType].flags & kPersQuoteOnce) {
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
#ifdef MAC1010
        TractorDrag(ship, &d);
#else
        if (ship->tractorShip == ship->index) {
            vx = 0.0f;
            vy = 0.0f;
        } else {
            vx = shipData[ship->tractorShip].vel.x;
            vy = shipData[ship->tractorShip].vel.y;
        }
        if (ship->vel.x > vx + d)
            ship->vel.x -= d;
        if (ship->vel.x < vx - d)
            ship->vel.x += d;
        if (ship->vel.y > vy + d)
            ship->vel.y -= d;
        if (ship->vel.y < vy - d)
            ship->vel.y += d;
#ifdef MACBUILD
        if (FloatAbs(ship->vel.x - vx) < d)
            ship->vel.x = vx;
        if (FloatAbs(ship->vel.y - vy) < d)
            ship->vel.y = vy;
#else
        {
            float dv;

            dv = fabs(ship->vel.x - vx);
            if (dv < d)
                ship->vel.x = vx;
            dv = fabs(ship->vel.y - vy);
            if (dv < d)
                ship->vel.y = vy;
        }
#endif
#endif
    }
    if (cloaked) {
        if (!ShipCanCloak(ship))
            DoShipUncloak(ship);
        n = CloakFuelUsage(ship);
        if (n > 0) {
            ship->fuel -= n * 0.03333 * gSpeedMult;
            if (ship->fuel <= 0.0f)
                ship->fuel = 0;
        }
        n = CloakShieldUsage(ship);
        if (n > 0 && ship->shield >= n) {
            ship->shield -= n * 0.03333 * gSpeedMult;
            if (ship->shield <= 0.0f)
                ship->shield = 0;
        }
    }

    if (shipClass[ship->type].accel == 0.0f
            && shipClass[ship->type].maxSpeed == 0.0f) {
        ship->vel.x = 0;
        ship->vel.y = 0;
    } else {
        ShipMaxSpeed(ship);                 /* result DISCARDED; not dead */
        if (ShipIsInertialess(ship))
            AdjustInertialessShipVelocity(ship);
        if (gameFrozen == 0) {
            ship->pos.x += ship->vel.x * gSpeedMult;
            ship->pos.y += ship->vel.y * gSpeedMult;
        }
    }

    if (ship->ionization > 0.0f) {
        ship->ionization -= ShipDeionizationRate(ship) * gSpeedMult;
        d = ShipIonizationFactor(ship);
        if (d > 0.7)
            d = 0.7f;
        d = ShipMaxSpeed(ship) * (1.0f - d);
        if (ship->vel.x > d)
            ship->vel.x -= gSpeedMult * 0.025;
        if (ship->vel.x < -d)
            ship->vel.x += gSpeedMult * 0.025;
        if (ship->vel.y > d)
            ship->vel.y -= gSpeedMult * 0.025;
        if (ship->vel.y < -d)
            ship->vel.y += gSpeedMult * 0.025;
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
            if (v > ShipTurnRate(ship) * gSpeedMult) {
                if (delta > 180.0f) {
                    ship->heading -= ShipTurnRate(ship) * gSpeedMult;
                    dir = -1;
                } else {
                    ship->heading += ShipTurnRate(ship) * gSpeedMult;
                    dir = 1;
                }
            } else {
                ship->heading = ship->desiredHeading;
            }
        }
        if (ship->shield < ShipShieldCapacity(ship))
            ship->shield += ShipShieldRechargeRate(ship) * gSpeedMult;
        if (ship->armor < ShipArmorCapacity(ship))
            ship->armor += ShipArmorRechargeRate(ship) * gSpeedMult;
    }
    if (ship->heading >= 360.0f)
        ship->heading -= 360.0f;
    if (ship->heading < 0.0f)
        ship->heading += 360.0f;

    if (ship->jumpTimer > 0.0f) {
        if (!IsDisabled(ship) && gameFrozen == 0) {
#ifdef MACBUILD
            if (abs(AngularDifference(ship->heading, ship->desiredHeading))
                    <= ShipTurnRate(ship)) {
#else
            turnRate = ShipTurnRate(ship);
            inRange = abs(AngularDifference(ship->heading, ship->desiredHeading)) <= turnRate;
            if (inRange) {
#endif
                if (AIIsShipJumping(ship)) {
                    ship->vel.x *= 0.95;
                    ship->vel.y *= 0.95;
                    d = 0.0f;
                    if (ship->parentShip != -1)
                        ship->desiredHeading = shipData[ship->parentShip].desiredHeading;
                    if (ship->parentShip == 0) {
                        if (shipData->jumpTimer > 1.0f) {
                            JumpDrift(shipData, 0.667, 45.0);
                        }
                    } else {
                        JumpDrift(ship, 0.5, 35.0);
                    }
                    if (d > 50.0)
                        d = 50.0f;
                    if (d > 0.0) {
                        Accel(ship->heading, d * gSpeedMult,
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
            if (ship->speedLimit == 0.0) {
                if (ship->jumpTimer <= 0.0f) {
                    maxSpeed = ShipMaxSpeed(ship);
                    if (ShipIsInertialess(ship)) {
                        ship->accelMag += ship->thrust * gSpeedMult;
                        if (ship->accelMag > maxSpeed)
                            ship->accelMag = maxSpeed;
                        if (ship->accelMag < 0.0)
                            ship->accelMag = 0;
                    } else {
                        AdjustedAccel(ship->heading,
                                     ship->thrust * gSpeedMult,
                                     &ship->vel, maxSpeed);
                    }
                }
            } else if (ship->speedLimit > 0.0) {
                maxSpeed = (float)ship->speedLimit;
                if (ShipIsInertialess(ship)) {
                    ship->accelMag += ship->thrust * gSpeedMult;
                    if (ship->accelMag > maxSpeed)
                        ship->accelMag = maxSpeed;
                    if (ship->accelMag < 0.0)
                        ship->accelMag = 0;
                } else {
                    AdjustedAccel(ship->heading,
                                 ship->thrust * gSpeedMult,
                                 &ship->vel, maxSpeed);
                }
            } else {
                if (ShipIsInertialess(ship)) {
                    ship->accelMag = FloatAbs(ship->speedLimit);
                } else {
                    ship->vel.x = ship->vel.y = 0;
                    Accel(ship->heading, FloatAbs(ship->speedLimit), &ship->vel);
                }
                ship->speedLimit = ship->speedLimit + FloatAbs(ship->thrust);
                if (ship->parentShip != -1 && ship->parentShip < kMaxShips) {
                    if (ShipMaxSpeed(ship) < ShipMaxSpeed(&shipData[ship->parentShip]))
                        d = ShipMaxSpeed(ship);
                    else
                        d = ShipMaxSpeed(&shipData[ship->parentShip]);
                } else
                    d = ShipMaxSpeed(ship);
                if (d < -1.0)
                    d = -1.0f;
                if (ship->speedLimit >= -d || ship->speedLimit >= 0.0f) {
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
        if (shipClass[ship->type].shanFlags & kShanBanks) {
            if (dir == 1) {
                if (ship->frameTimer < 8.0f)
                    ship->frameTimer += gSpeedMult;
            } else if (dir == -1) {
                if (ship->frameTimer > -8.0f)
                    ship->frameTimer -= gSpeedMult;
            } else if (ship->frameTimer >= gSpeedMult) {
                ship->frameTimer -= gSpeedMult;
            } else if (ship->frameTimer <= -gSpeedMult) {
                ship->frameTimer += gSpeedMult;
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
                if ((shipClass[ship->type].shanFlags & kShanFoldingParts)
                        && ship->engineGlow < 0x18)
                    ship->engineGlow += 2;
            }
        }
        if (ship->thrust > 0.0f
                && (ship->aiTimer <= 0.0f || AIIsShipDoingCountWait(ship))) {
            if (shipClass[ship->type].shanFlags & kShanBanks)
                ship->bankDir = dir;
            if (ship->thrust >= ShipAccelRate(ship) * 2.0f) {
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
            if (weaponStats[ship->selectedWeapon].flags & kWeapSecondary) {
                FireAIShipWeapon(ship);
            } else {
                FireAIShipWeapon(ship);
                ship->selectedWeapon = -1;
                ship->fireQueued = 0;
            }
        }
    }
    if (ship->aiTimer > 0.0f && ship->type != kShipTypeSpecial) {
        ship->aiTimer -= gSpeedMult;
        if (!AIIsShipDoingCountWait(ship) && ship->engineGlow > 0)
            ship->engineGlow -= 1;
    }

    if (ship->deathTimer > 0.0f) {
        if (gameFrozen == 0)
            ship->deathTimer -= 1.0f;
        if (ship->parentShip == 0) {
            if (ship->aiType == kShipTypeEscort) {
                if (shipClass[ship->type].inherentAI <= kShipTypeBraveTrader) {
                    DestroyPartialFleetCargo(ship->index);
                    statusRedraw[kRedrawCargo] = 1;
                    DrawStatusCargo();
                }
            }
            ship->parentShip = -1;
            ship->aiType = shipClass[ship->type].inherentAI;
        }
        if (ship->deathTimer == shipClass[ship->type].deathDelay * 0.5) {
            if (ship->persType != -1) {
                if (personData[ship->persType].flags & kPersEscapePod) {
                    if (ship->govt != -1) {
                        if (govtData[ship->govt].flags & kGovtPersNoEscapePod)
                            spawn = 0;
                        else
                            spawn = 1;
                    } else
                        spawn = 1;
                } else
                    spawn = 0;
                if (spawn) {
                    SpawnEscapePod(ship);
                    PlaySoundDistance(-1, escapePodSnd, 5,
                                 &ship->pos,
                                 &shipData->pos);
                }
            }
        }
        if (ship->escapePods > 0
                && ship->deathTimer <= shipClass[ship->type].deathDelay * 0.5
                && shipClass[ship->type].podCount > 0) {
            n = (shipClass[ship->type].deathDelay
                    / shipClass[ship->type].podCount * 0.4);
            if (n < 10)
                n = 10;
            if (gSpeedMult > 0.0)
                n = (n / gSpeedMult);
            if (frameCounter % n == 0
                    || ship->escapePods == shipClass[ship->type].podCount) {
                ship->escapePods -= 1;
                SpawnEscapePod(ship);
                PlaySoundDistance(-1, escapePodSnd, 5, &ship->pos,
                             &shipData->pos);
            }
        }
    }

    if (ShipFuelGenRate(ship, &d))
        ship->fuel += d * gSpeedMult;
    v = ShipFuelCapacity(ship);
    if (ship->fuel > v)
        ship->fuel = v;
    if (ship->fuel < 0.0f)
        ship->fuel = 0;
    if (ship->annoyance > 0.0f) {
        ship->annoyance -= gSpeedMult * 0.5;
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
        if (ship->deathTimer <= shipClass[ship->type].deathDelay * 0.5) {
            if (Rand(3) == 0) {
                n = LaunchFighter(ship, EscapeShipWeapType(ship));
                if (n != -1) {
                    if (shipData[n].shield > 1.0f)
                        shipData[n].shield =
                            Rand(shipData[n].shield);
                    shipData[n].vel.x *= 0.5;
                    shipData[n].vel.y *= 0.5;
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
#define kProximityFuseFrames    30.0    /* gSpeedMult units before a fuse arms */
#define kShotSpentCode          0x3e6


#define kWeap22NoBlastOnShips   0x400

/* the shot's sprite; only the fields this TU touches */

/* one live shot, stride 0x48 in shotData */

/* HandleShot (mac _HandleShot). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00435830
// MAC: _HandleShot i386 0x00035586 ppc 0x0003ae1c
void HandleShot(ShotObj *shot)
{
    FloatPoint pt;
    float f;
    short frame;
    short half;              /* re-used as the smoke type below */
    short i;
    Boolean armed;
    Boolean noSub;
    Boolean hit;

    noSub = 0;
    if (shot->curSystem == shipData->curSystem && shot->life > 0.0f) {
        if (shot->owner < -1 || shot->owner >= kMaxShips)
            shot->owner = -1;
        if (shot->weapType < 0 || shot->weapType >= kMaxWeapTypes) {
            shot->life = kShotSpentLife;
            return;
        }
        if (weaponStats[shot->weapType].guidance == kGuidancePointDef)
            shot->target = -1;
        else if (shot->target >= 0 && shot->target < kMaxShips) {
            if (shot->guidanceState == 0 && shipData[shot->target].active == 0) {
                shot->guidanceState = kShotSpentCode;
                shot->target = -1;
            }
        } else
            shot->target = -1;

        armed = 0;
        if (weaponStats[shot->weapType].guidance == kGuidanceHoming
                && shot->owner != -1
                && shot->target > 0
                && shot->guidanceState == 0
                && weaponStats[shot->weapType].lifetime - (double)shot->life
                   > kProximityFuseFrames * gSpeedMult)
            armed = 1;

        if (gameFrozen == 0)
            shot->life -= gSpeedMult;
        if (killShots != 0)
            shot->life = kShotKilledLife;
        else if (shot->life <= 0.0f) {
            shot->life = kShotSpentLife;
            if ((weaponStats[shot->weapType].flags2 & kWeapNoSubsOnExpire) != 0)
                half = 1;
        }
        if (gameFrozen == 0) {
            HandleShotGuidance(shot);
            shot->pos.x += shot->vel.x * gSpeedMult;
            shot->pos.y += shot->vel.y * gSpeedMult;
        }

        if (gameFrozen == 0 && armed == 0 && shot->owner != -1
                && weaponStats[shot->weapType].guidance == kGuidanceHoming
                && shot->target > 0
                && shot->guidanceState == 0
                && shipData[shot->target].parentShip != 0
                && shot->owner <= 0
                && shot->owner < kMaxShips
                && weaponStats[shot->weapType].lifetime - (double)shot->life
                   > kProximityFuseFrames * gSpeedMult
                && ShipsShareParent(&shipData[shot->target],
                                    &shipData[shot->owner]) == 0)
            DamageShip(&shipData[shot->target],
                       &shipData[shot->target].pos,
                       0, kShotKilledLife, kShotKilledLife, shot->owner,
                       1, 1, 1, 1, 0,
                       weaponStats[shot->weapType].reload);

        if ((Boolean)(weaponStats[shot->weapType].flags & kWeapSpins)) {
            frame = shot->frame;
            shot->frameTimer += gSpeedMult;
            if (shot->frameTimer >= weaponStats[shot->weapType].beamWidth
                    || weaponStats[shot->weapType].beamWidth <= 0) {
                shot->frame++;
                shot->frameTimer = 0.0f;
            }
            if (shot->frame >= shot->sprite->numFrames) {
                if ((weaponStats[shot->weapType].flags2 & kWeapStopOnLastFrame) != 0)
                    shot->frame = shot->sprite->numFrames - 1;
                else
                    shot->frame = 0;
            }
            if ((weaponStats[shot->weapType].flags2 & kWeapHoldFirstFrame) != 0
                    && shot->life > weaponStats[shot->weapType].lifetime
                       - weaponStats[shot->weapType].proxSafety) {
                frame = 0;
                if ((weaponStats[shot->weapType].flags2 & kWeapStopOnLastFrame) != 0)
                    shot->frame = 0;
            }
        } else {
            frame = (shot->angle * shot->sprite->numFrames / 360.0);
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

        if (weaponStats[shot->weapType].decayShots < 0 && shot->murk < 32.0f) {
            if ((weaponStats[shot->weapType].flags3 & kWeapTranslucent) != 0
                    && gSpriteWorld->pixelDepth >= kMinHiliteDepth) {
                shot->sprite->translucency = kMurkFull;
                shot->sprite->tintRed = shot->sprite->tintGreen =
                    shot->sprite->tintBlue = shot->murk;
            } else {
                shot->sprite->translucency = (32.0f - shot->murk);
                shot->murk -= weaponStats[shot->weapType].decayShots * gSpeedMult;
            }
        }
        if (weaponStats[shot->weapType].decayShots > 0 && shot->life < 32.0f) {
            if ((weaponStats[shot->weapType].flags3 & kWeapTranslucent) != 0
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
                shot->murk += weaponStats[shot->weapType].decayShots * gSpeedMult;
        }
        if ((weaponStats[shot->weapType].flags3 & kWeapTranslucent) != 0
                && weaponStats[shot->weapType].decayShots == 0
                && gSpriteWorld->pixelDepth >= kMinHiliteDepth) {
            shot->sprite->translucency = kMurkFull;
            shot->sprite->tintRed = kMurkFull;
            shot->sprite->tintGreen = kMurkFull;
            shot->sprite->tintBlue = kMurkFull;
        }
        if ((weaponStats[shot->weapType].flags3 & kWeapOneShotAtATime) != 0
                && shot->owner != -1)
            shipData[shot->owner].weap[shot->weapType].reloadTimer =
                weaponStats[shot->weapType].reload;

        HandleSpriteMurkiness(shot->pos.x, shot->pos.y, shot->sprite);
        half = GetSpriteXSize(shot->sprite) / 2;
        SWSetSpriteLocation(shot->sprite,
                            (short)(shot->pos.x - shipData->pos.x)
                            + screenCenter.h - half,
                            (short)(shot->pos.y - shipData->pos.y)
                            + screenCenter.v - half);

        if (updateSmokeThisFrame != 0 && gameFrozen == 0) {
            frame = -1;
            if ((weaponStats[shot->weapType].flags & kWeapSmallSmoke) != 0)
                frame = 1;
            else if ((weaponStats[shot->weapType].flags & kWeapBigSmoke) != 0)
                frame = 0;
            if (frame != -1) {
                if ((weaponStats[shot->weapType].flags & kWeapPersistentSmoke) != 0)
                    frame += 2;
#ifdef MACBUILD
                SpawnSmoke(shot->pos, frame, weaponStats[shot->weapType].smokeSet);
#else
                SpawnSmoke(shot->pos.x, shot->pos.y, frame,
                             weaponStats[shot->weapType].smokeSet);
#endif
            }
        }

        if (weaponStats[shot->weapType].particles > 0 && settings.noDebris == 0
                && gameFrozen == 0) {
#ifdef MACBUILD
            pt = shot->pos;
#else
            pt.x = shot->pos.x;
            pt.y = shot->pos.y;
#endif
            Accel((short)(shot->angle + kHalfTurn) % kAngleWrap,
                  GetSpriteXSize(shot->sprite) / 2, &pt);
            SpawnParticles(pt.x, pt.y,
                         weaponStats[shot->weapType].partVel[Rand(8)], 0,
                         weaponStats[shot->weapType].partLifeMin,
                         weaponStats[shot->weapType].partLifeMax,
                         weaponStats[shot->weapType].partColor[Rand(8)],
                         kMurkFull - shot->sprite->translucency,
                         weaponStats[shot->weapType].particles, 0);
        }

        if (weaponStats[shot->weapType].decay > 0) {
            shot->trailTimer += gSpeedMult;
            if (shot->trailTimer > weaponStats[shot->weapType].decay) {
                shot->trailTimer = 0.0f;
                shot->trailCount++;
            }
        }
    } else {
        if (shot->curSystem == shipData->curSystem
                && shot->life > kShotKilledLife
                && shot->sprite->isVisible != 0) {
            if ((weaponStats[shot->weapType].flags2 & kWeapNoSubsOnExpire) != 0)
                noSub = 1;
            if (weaponStats[shot->weapType].subCount > 0 && noSub == 0)
                SpawnShotSubmunitions(shot, shot->target);
            if ((weaponStats[shot->weapType].flags & kWeapDetonatesAtEnd) != 0) {
                CreateExplosion(shot->pos,
                                weaponStats[shot->weapType].explodType,
                                weaponStats[shot->weapType].blastRadius, 1);
                if (weaponStats[shot->weapType].blastRadius > 0
                        && (weaponStats[shot->weapType].flags2
                            & kWeap22NoBlastOnShips) == 0) {
                    for (i = 0; i < kMaxShips; i++) {
                        hit = 1;
                        if (i == shot->owner
                                && ((weaponStats[shot->weapType].flags
                                     & kWeapBlastSparesPlayer) != 0
                                    || shot->owner != 0))
                            hit = 0;
                        if (shipData[i].active != 0 && hit != 0
                                && (shipClass[shipData[i].type].flags
                                    & kShipPlanetType) == 0) {
                            f = FloatAbs(shipData[i].pos.x - shot->pos.x);
                            pt.x = f;
                            f = FloatAbs(shipData[i].pos.y - shot->pos.y);
                            pt.y = f;
                            if (pt.x <= weaponStats[shot->weapType].blastRadius
                                    && pt.y <= weaponStats[shot->weapType].blastRadius) {
                                DamageShip(&shipData[i], &shot->pos,
                                           weaponStats[shot->weapType].impact,
                                           weaponStats[shot->weapType].damage[0],
                                           weaponStats[shot->weapType].damage[1],
                                           shot->owner, 0, 0, shot->disableOnly, 0,
                                           (weaponStats[shot->weapType].flags
                                                           & kWeapPassesShields),
                                           weaponStats[shot->weapType].reload);
                                IonizeShip(&shipData[i], &weaponStats[shot->weapType],
                                           &shot->pos);
                            }
                        }
                    }
                }
            } else if (weaponStats[shot->weapType].explodType >= 1) {
                SpawnExplod(shot->pos, 0, 0, 0);
            }
        }
        shot->life = kShotDeadLife;
        SWSetSpriteVisible(shot->sprite, 0);
    }
}

#define kMinSpriteSpan          8
#define kWrapMargin             32
#define kDeadStrength           (-32000)

#ifdef MAC1010 /* 1.0.10 spelling */
static inline void AsteroidWrapLimits(short i, short maxW, short maxH)
{
    int rangeH;
    int edgeH;
    int rangeV;
    int canWrap;
    int sum;

    rangeH = maxW;
    edgeH = screenCenter.h * 2;
    rangeV = maxH;
    canWrap = 1;
    if (i == 0 && miningShipsActive != 0) {
        canWrap = 0; /* 1.0.10 pad @_HandleAsteroids: a multi-def local the colourer sees, no word, one more neighbour */
        rangeH = 0; /* 1.0.10 pad @_HandleAsteroids: same, rangeH */
        edgeH = 0; /* 1.0.10 pad @_HandleAsteroids: same, edgeH */
        rangeV = 0; /* 1.0.10 pad @_HandleAsteroids: same, rangeV */
    }
    sum = i + rangeH + edgeH + rangeV + canWrap; /* 1.0.10 pad @_HandleAsteroids: the read keeps the four locals live to the colourer */
}
#endif


/* HandleAsteroids (mac _HandleAsteroids). STATE: MATCH 100.0%, exe 442 / obj 442 instrs,
   byte-identical yes. 1.0.10 MATCH 414 of 414 on both fragments. OPEN: none; the eight
   screenCenter reads are plain .h/.v and the X/Y load asymmetry is the compiler's (notes
   2026-09-16). */
// FUNCTION: EVNOVA 0x00436910
// MAC: _HandleAsteroids i386 0x000361ac ppc 0x0003bd5c
void HandleAsteroids(void)
{
    short i;
    short spriteW;
    short maxW;
    short maxH;
    short spriteH;
    Boolean wrapped;

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
                asteroidData[i].pos.x += asteroidData[i].vel.x * gSpeedMult;
                asteroidData[i].pos.y += asteroidData[i].vel.y * gSpeedMult;
                asteroidData[i].frame += asteroidData[i].frameRate * gSpeedMult;
            }
            SWSetSpriteVisible(asteroidData[i].sprite, 1);
            SWCloneFramesFromMaster(asteroidData[i].sprite,
                         masterAsteroidSprite[asteroidData[i].type]);
            if (asteroidData[i].frame < 0.0f)
                asteroidData[i].frame = asteroidData[i].frame
                                        + asteroidData[i].sprite->numFrames;
            if (asteroidData[i].frame >= asteroidData[i].sprite->numFrames)
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
#ifdef MAC1010 /* 1.0.10 spelling */
            AsteroidWrapLimits(i, maxW, maxH);
#endif
            wrapped = 0;
            if (asteroidData[i].sprite->destFrameRect.left > screenCenter.h * 2 + kWrapMargin) {
                asteroidData[i].pos.x = shipData->pos.x - screenCenter.h - maxW * 2;
                wrapped = 1;
            }
            if (asteroidData[i].sprite->destFrameRect.left < -spriteW * 2 - kWrapMargin) {
                asteroidData[i].pos.x = shipData->pos.x + screenCenter.h + maxW * 2;
                wrapped = 1;
            }
            if (asteroidData[i].sprite->destFrameRect.top > screenCenter.v * 2 + kWrapMargin) {
                asteroidData[i].pos.y = shipData->pos.y - screenCenter.v - maxH * 2;
                wrapped = 1;
            }
            if (asteroidData[i].sprite->destFrameRect.top < -spriteH * 2 - kWrapMargin) {
                asteroidData[i].pos.y = shipData->pos.y + screenCenter.v + maxH * 2;
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
// MAC: _AsteroidCollisionHandler i386 0x000369c8 ppc 0x0003c79c
void AsteroidCollisionHandler(SpriteRec *roidSpr, SpriteRec *otherSpr, Rect *sectRect)
{
    ShotObj *shot;

    shot = &shotData[otherSpr->userData];
    if ((weaponStats[shot->weapType].seeker & kWeapSeekPassesAsteroids) != 0)
        return;
    if (!SWRLECollision(roidSpr, otherSpr))
        return;
    HandleAsteroidHit(shot, &asteroidData[roidSpr->userData]);
}

#define kRockDamageScale        10
#define kRockSpeedLimit         2.0f

/* one live shot, stride 0x48 in shotData; only the fields this TU touches */

/* the shot's sprite; only the byte this TU sets */

/* loaddata.c's rock record, stride 0x1c; only the mass this TU divides by */

/* HandleAsteroidHit. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00436ff0
// MAC: _HandleAsteroidHit i386 0x00036637 ppc 0x0003c360
void HandleAsteroidHit(ShotObj *shot, AsteroidObj *roid)
{
    /* loose floats here, not game.h's FloatPoint: this body passes x and y
       separately and mwcc refuses the conversion */
    float dx;
    float dy;
    short i;
    Boolean inRange;

    CreateExplosion(shot->pos,
                    weaponStats[shot->weapType].explodType,
                    weaponStats[shot->weapType].blastRadius, 1);
    if (weaponStats[shot->weapType].hitParticles > 0)
        SpawnParticles(shot->pos.x, shot->pos.y,
                     weaponStats[shot->weapType].hitPartVel, kParticleSpread,
                     weaponStats[shot->weapType].hitPartLife,
                     (weaponStats[shot->weapType].hitPartLife
                                   * kParticleLifeScale),
                     weaponStats[shot->weapType].hitPartColor, kParticleColor,
                     weaponStats[shot->weapType].hitParticles, 0);
    if (shot->owner == 0 && weaponStats[shot->weapType].blastRadius > 0
            && (weaponStats[shot->weapType].flags2 & kWeapPlanetType) == 0) {
        for (i = 0; i < kMaxShips; i++) {
            inRange = 1;
            if (i == shot->owner
                    && ((weaponStats[shot->weapType].flags & kWeapBlastSparesPlayer) != 0
                        || shot->owner != 0))
                inRange = 0;
            if (shipData[i].active != 0 && inRange != 0
                    && (shipClass[shipData[i].type].flags & kShipPlanetType) == 0) {
                dx = FloatAbs(shipData[i].pos.x - shot->pos.x);
                dy = FloatAbs(shipData[i].pos.y - shot->pos.y);
                if (dx <= weaponStats[shot->weapType].blastRadius
                        && dy <= weaponStats[shot->weapType].blastRadius) {
                    DamageShip(&shipData[i], &shot->pos,
                               weaponStats[shot->weapType].impact,
                               weaponStats[shot->weapType].damage[0],
                               weaponStats[shot->weapType].damage[1],
                               shot->owner, 0, 0, shot->disableOnly, 0,
                               (weaponStats[shot->weapType].flags & kWeapPassesShields),
                               weaponStats[shot->weapType].reload);
                    IonizeShip(&shipData[i], &weaponStats[shot->weapType],
                               &shot->pos);
                }
            }
        }
    }

    if ((weaponStats[shot->weapType].flags2 & kWeapBigRockDamage) != 0)
        roid->strength -= weaponStats[shot->weapType].damage[1] * kRockDamageScale;
    else
        roid->strength -= weaponStats[shot->weapType].damage[1];
    if (roid->strength < 0) {
        DestroyAsteroid(roid);
    } else if (weaponStats[shot->weapType].impact != 0
               && asteroidType[roid->type].mass > 0) {
        AdjustedAccel(shot->angle,
                      weaponStats[shot->weapType].impact
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
   gSpeedMult is loaded here as a QWORD, so it is a double; another TU declares
   it `long' -- a per-file window, not a disagreement to fix.
   A SpriteWorld CollideProcPtr: sectRect is the intersection the layer walker
   hands every collide proc, and this one has no use for it. */

#define kJunkBase   1000    /* box cargoType of jünk n is kJunkBase + n */

#define kBigSpriteWidth 0x20
#define kCloseZoom      2.0

/* ShipCollisionHandler. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004374f0
// MAC: _ShipCollisionHandler i386 0x00036f87 ppc 0x0003cde8
void ShipCollisionHandler(SpriteRec *shipSpr, SpriteRec *otherSpr, Rect *sectRect)
{
    ShipObj *ship;
    ShotObj *shot;
    BoxObj *box;
    int32_t index;
    int32_t index2;

    index = shipSpr->userData;
    if (index < 0 || index >= kMaxShips)
        return;
    ship = &shipData[index];
    if (otherSpr->parentSpriteLayerP == shotLayer || otherSpr->parentSpriteLayerP == guidedShotLayer
        || otherSpr->parentSpriteLayerP == turretedShotLayer) {
        index2 = otherSpr->userData;
        if (index2 < 0 || index2 >= kMaxShots)
            return;
        shot = &shotData[index2];
        if (!ShotCanHitShip(shot, ship))
            return;
        if (gSpeedMult >= kCloseZoom
            || GetSpriteXSize(shipSpr) <= kBigSpriteWidth) {
            if (!SWRadiusCollision(shipSpr, otherSpr))
                return;
        } else {
            if (!SWRLECollision(shipSpr, otherSpr))
                return;
        }
        if (weaponStats[shot->weapType].proxSafety > 0
            && shot->life > (weaponStats[shot->weapType].lifetime
                       - weaponStats[shot->weapType].proxSafety))
            return;
        HandleShipHit(shot, ship, 0);
    } else if (otherSpr->parentSpriteLayerP == boxLayer) {
        if ((ship->index == 0 && shipData[0].canScoop != 0)
            || (ship->index != 0 && AIShipIsScooping(ship))) {
            index2 = otherSpr->userData;
            if (index2 < 0 || index2 >= kMaxBoxes)
                return;
            box = &boxData[index2];
            if (box->mining != 0 && SWRLECollision(shipSpr, otherSpr)) {
                box->timer = -1.0f;
                SWSetSpriteVisible(otherSpr, 0);
                if (box->cargoType >= 0
                    && box->cargoType <= kLastCommodity)
                    shipData[ship->index].cargo[box->cargoType]++;
                else if (ship->index == 0 && box->cargoType >= kJunkBase
                         && box->cargoType < kJunkBase + kMaxJunks)
                    junkData[box->cargoType - kJunkBase].held++;
                if (ship->index == 0) {
                    statusRedraw[kRedrawCargo] = 1;
                    ResetPlayerPrecalcedValues();
                }
            }
        }
    }
}

#define kVoiceShipKilled        2       /* _SpeakVoiceMessage's first argument */

/* one live shot, stride 0x48 in shotData; only the fields this TU touches */

/* the shot's sprite; only the two fields this TU touches */

/* HandleShipHit. STATE: MATCH, byte-identical. OPEN: none.
   Was DIFF 98.3% and recorded CLOSED-as-PREFERENCE; that verdict was wrong (338).
   Passing shot->pos whole, against game.h's FloatPoint row, closed all 8 rows. */
// FUNCTION: EVNOVA 0x00437780
// MAC: _HandleShipHit i386 0x00036a45 ppc 0x0003c84c
void HandleShipHit(ShotObj *shot, ShipObj *ship, Boolean spawnSubs)
{
    float dy;
    short i;
    short armorDmg;
    short shieldDmg;
    Boolean wasDying;
    Boolean inRange;
    float dx;

    if (ship->type < 0 || ship->type >= kMaxShipTypes)
        return;
    if (shot->weapType < 0 || shot->weapType >= kMaxWeapTypes)
        return;

    CreateExplosion(shot->pos,
                    weaponStats[shot->weapType].explodType,
                    weaponStats[shot->weapType].blastRadius, 1);
    armorDmg = weaponStats[shot->weapType].damage[0];
    shieldDmg = weaponStats[shot->weapType].damage[1];
    wasDying = IsDying(ship);
    if (shot->trailCount > 0) {
        armorDmg -= shot->trailCount;
        shieldDmg -= shot->trailCount;
        if (armorDmg < 0)
            armorDmg = 0;
        if (shieldDmg < 0)
            shieldDmg = 0;
    }
    IonizeShip(ship, &weaponStats[shot->weapType], 0);
    if (weaponStats[shot->weapType].hitParticles > 0)
        SpawnParticles(shot->pos.x, shot->pos.y,
                     weaponStats[shot->weapType].hitPartVel, kParticleSpread,
                     weaponStats[shot->weapType].hitPartLife,
                     (weaponStats[shot->weapType].hitPartLife
                                   * kParticleLifeScale),
                     weaponStats[shot->weapType].hitPartColor, kParticleColor,
                     weaponStats[shot->weapType].hitParticles, 0);
    if (shot->target == ship->index)
        DamageShip(ship, &shot->pos,
                   weaponStats[shot->weapType].impact, armorDmg, shieldDmg,
                   shot->owner, 1, 1, shot->disableOnly, 0,
                   (weaponStats[shot->weapType].flags & kWeapPassesShields),
                   weaponStats[shot->weapType].reload);
    else
        DamageShip(ship, &shot->pos,
                   weaponStats[shot->weapType].impact, armorDmg, shieldDmg,
                   shot->owner, 1, 0, shot->disableOnly, 0,
                   (weaponStats[shot->weapType].flags & kWeapPassesShields),
                   weaponStats[shot->weapType].reload);

    if (IsDying(ship) && wasDying == 0) {
        for (i = 1; i < kMaxShips; i++) {
            if (ship->index != i && shipData[i].active != 0
                    && shipData[i].parentShip == 0
                    && ship->index == shipData[i].targetShip
                    && shipData[i].aiType >= kShipTypeWarship
                    && IsShipAttacking(&shipData[i])
                    && shipData[i].type >= 0 && shipData[i].type < kMaxShipTypes
                    && (shipClass[shipData[i].type].flags2 & kShipNoFighterVoice) == 0)
                SpeakVoiceMessage(kVoiceShipKilled,
                             shipClass[shipData[i].type].inherentGovtB,
                             shipData[i].voiceClass);
        }
    }

    if (weaponStats[shot->weapType].blastRadius > 0) {
        for (i = 0; i < kMaxShips; i++) {
            inRange = 1;
            if (i == shot->owner
                    && ((weaponStats[shot->weapType].flags & kWeapBlastSparesPlayer) != 0
                        || shot->owner != 0))
                inRange = 0;
            if (i != ship->index && shipData[i].active != 0 && inRange != 0
                    && shipData[i].type >= 0 && shipData[i].type < kMaxShipTypes) {
                dx = FloatAbs(shipData[i].pos.x - shot->pos.x);
                dy = FloatAbs(shipData[i].pos.y - shot->pos.y);
                if (dx <= weaponStats[shot->weapType].blastRadius
                        && dy <= weaponStats[shot->weapType].blastRadius) {
                    DamageShip(&shipData[i], &shot->pos,
                               weaponStats[shot->weapType].impact,
                               weaponStats[shot->weapType].damage[0],
                               weaponStats[shot->weapType].damage[1],
                               shot->owner, 0, 0, shot->disableOnly, 0,
                               (weaponStats[shot->weapType].flags
                                               & kWeapPassesShields),
                               0);
                    IonizeShip(&shipData[i], &weaponStats[shot->weapType],
                               &shot->pos);
                }
            }
        }
    }

    if (weaponStats[shot->weapType].subCount > 0 && spawnSubs != 0)
        SpawnShotSubmunitions(shot, ship->index);
    shot->life = -1.0f;
    if (shot->sprite != NULL && shot->sprite->curFrameP != 0) {
        SWSetSpriteVisible(shot->sprite, 0);
        shot->sprite->needsToBeErased = 1;
    }
}

#define kStellarPenaltyCount    10
#define kProxSpriteFraction     0.333

/* CheckShotProximities (mac _CheckShotProximities). STATE: MATCH 558/558, byte-identical,
   at the plain level=4,intrinsics. OPEN: none */
// FUNCTION: EVNOVA 0x00437e20
// MAC: _CheckShotProximities i386 0x000371bc ppc 0x0003d0f0
void CheckShotProximities(void)
{
    int32_t range;
    int32_t dx;
    int32_t dy;
#ifdef MAC1010 /* 1.0.10 spelling */
    int32_t ax;
    int32_t ay;
#else
    int32_t ay;
    int32_t ax;
#endif
    float f;
    short i;
    short k;
    short j;
    short stel;
    Boolean hit;

    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life >= 0.0f
                && shotData[i].life < weaponStats[shotData[i].weapType].lifetime
                   - weaponStats[shotData[i].weapType].proxSafety) {
            if ((weaponStats[shotData[i].weapType].flags2 & kWeapPlanetType) != 0) {
                for (j = 0; j < kNavSlots; j++) {
                    stel = systemData[shipData->curSystem].navDefault[j];
                    if (stel != -1 && stellarData[stel].strength > 0
                            && !StellarIsDestroyed(&stellarData[stel])
                            && SWRLECollision(shotData[i].sprite,
                                              stellarData[stel].sprite)) {
                        stellarData[stel].deadCounter -=
                            weaponStats[shotData[i].weapType].damage[0]
                            + weaponStats[shotData[i].weapType].damage[1];
                        CreateExplosion(shotData[i].pos,
                                        weaponStats[shotData[i].weapType].explodType,
                                        weaponStats[shotData[i].weapType].blastRadius, 1);
                        if (weaponStats[shotData[i].weapType].hitParticles > 0)
                            SpawnParticles(shotData[i].pos.x,
                                         shotData[i].pos.y,
                                         weaponStats[shotData[i].weapType].hitPartVel,
                                         kParticleSpread,
                                         weaponStats[shotData[i].weapType].hitPartLife,
                                         (weaponStats[shotData[i].weapType].hitPartLife
                                                       * kParticleLifeScale),
                                         weaponStats[shotData[i].weapType].hitPartColor,
                                         kParticleColor,
                                         weaponStats[shotData[i].weapType].hitParticles, 0);
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
                    && weaponStats[shotData[i].weapType].proxRadius > 0) {
                hit = 0;
                if ((weaponStats[shotData[i].weapType].seeker & kWeapSeekPassesAsteroids) != 0)
                    hit = 1;
                for (j = 0; j < kMaxShips; j++) {
                    if (shipData[j].active != 0
                            && shipData[j].curSystem == shipData->curSystem
                            && ShotCanHitShip(&shotData[i], &shipData[j])) {
                        range = weaponStats[shotData[i].weapType].proxRadius;
                        range += GetSpriteXSize((SpriteRec *)shipData[j].sprite)
                                 * kProxSpriteFraction;
                        range *= range;
                        dx = abs((int32_t)shipData[j].pos.x - (int32_t)shotData[i].pos.x);
                        dy = abs((int32_t)shipData[j].pos.y - (int32_t)shotData[i].pos.y);
                        if (dx * dx + dy * dy <= range) {
                            HandleShipHit(&shotData[i], &shipData[j], 1);
                            hit = 1;
                            break;
                        }
                    }
                }
                if (hit == 0) {
                    for (j = 0; j < kMaxAsteroids; j++) {
                        if (asteroidData[j].active != 0) {
                            range = weaponStats[shotData[i].weapType].proxRadius;
                            range *= range;
#ifdef MACBUILD
                            ax = abs((asteroidData[j].pos.x - shotData[i].pos.x));
                            ay = abs((asteroidData[j].pos.y - shotData[i].pos.y));
#else
                            f = FloatAbs(asteroidData[j].pos.x - shotData[i].pos.x);
                            ax = f;
                            f = FloatAbs(asteroidData[j].pos.y - shotData[i].pos.y);
                            ay = f;
#endif
                            if (ax * ax + ay * ay <= range) {
                                HandleAsteroidHit(&shotData[i], &asteroidData[j]);
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
// MAC: _PointDefenseCollisionHandler i386 0x00037776 ppc 0x0003d738
void PointDefenseCollisionHandler(SpriteRec *shotSpr, SpriteRec *otherSpr, Rect *sectRect)
{
    ShotObj *shot1;
    ShotObj *shot2;

    shot1 = &shotData[shotSpr->userData];
    if (otherSpr->parentSpriteLayerP == guidedShotLayer) {
        shot2 = &shotData[otherSpr->userData];
        shot1->life = 0;
        if (shot2->energy > 0) {
            shot2->energy -= weaponStats[shot1->weapType].damage[0] + weaponStats[shot1->weapType].damage[1] / 2;
        } else {
            shot2->life = 0;
            SWSetSpriteVisible(otherSpr, 0);
            SpawnExplod(shot1->pos, shot1->owner, 0, 0);
        }
        SWSetSpriteVisible(shotSpr, 0);
    }
}

/* BeamEraseCallback (mac _BeamEraseCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004386f0
// MAC: _BeamEraseCallback i386 0x0003785e ppc 0x0003d83c
#ifdef MAC1010
inline
#endif
void BeamEraseCallback(SpriteWorldPtr world)
{
    short count;
    short density;
    Point p1, p2;

    count = 0;
    while (count < kMaxBeams) {
        if (beamData[count].state >= -1 &&
            beamData[count].owner != -1) {
            p1.h = beamData[count].lastHead.h;
            p1.v = beamData[count].lastHead.v;
            p2.h = beamData[count].lastTail.h;
            p2.v = beamData[count].lastTail.v;
            density = weaponStats[beamData[count].weapType].lightningDensity;
            if (density > 0) {
                EraseLightningBeam(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP, p1, p2,
                             weaponStats[beamData[count].weapType].beamWidth, density,
                             weaponStats[beamData[count].weapType].lightningAmplitude);
            } else {
                EraseBeam(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP, p1, p2,
                             weaponStats[beamData[count].weapType].beamWidth,
                             weaponStats[beamData[count].weapType].coronaFalloff);
            }
        }
        count++;
    }
}

/* BeamDrawCallback (mac _BeamDrawCallback). STATE: MATCH 100.0% (197 exact, 57 reloc-equivalent, 0 mismatched;
   exe 254 / obj 254), relocregcheck 0 of 57, byte identity unmeasured until the next
   regate --update.
   OPEN: none. `density' is named only in loop 2's draw arm; both erase arms read the
   field twice (notes 2026-09-09). */
// FUNCTION: EVNOVA 0x00438810
// MAC: _BeamDrawCallback i386 0x00037952 ppc 0x0003d928
void BeamDrawCallback(SpriteWorldPtr world)
{
    short count;
    short density;
    Point p1, p2;

#ifdef V111
    if (world->fastBlit != 0) {
        count = 0;
        do {
            if (beamData[count].state >= -1 &&
                beamData[count].owner != -1) {
                p1.h = beamData[count].lastHead.h;
                p1.v = beamData[count].lastHead.v;
                p2.h = beamData[count].lastTail.h;
                p2.v = beamData[count].lastTail.v;
                density = weaponStats[beamData[count].weapType].lightningDensity;
                if (density > 0) {
                    if (useLegacyRenderingPipeline)
                        EraseLightningBeam(gSpriteWorld->workFrameP,
                                     gSpriteWorld->windowFrameP,
                                     p1, p2,
                                     weaponStats[beamData[count].weapType].beamWidth, density,
                                     weaponStats[beamData[count].weapType].lightningAmplitude);
                    else
                        EraseLightningBeam(gSpriteWorld->workFrameP,
                                     gSpriteWorld->workFrameP,
                                     p1, p2,
                                     weaponStats[beamData[count].weapType].beamWidth, density,
                                     weaponStats[beamData[count].weapType].lightningAmplitude);
                } else {
                    if (useLegacyRenderingPipeline)
                        EraseBeam(gSpriteWorld->workFrameP,
                                     gSpriteWorld->windowFrameP,
                                     p1, p2, 0x20, 0x10);
                    else
                        EraseBeam(gSpriteWorld->workFrameP,
                                     gSpriteWorld->workFrameP,
                                     p1, p2, 0x20, 0x10);
                }
            }
            count++;
        } while (count < kMaxBeams);
    }

    count = 0;
    do {
        if (beamData[count].state >= 0 &&
            beamData[count].owner != -1) {
            if ((weaponStats[beamData[count].weapType].flags2 & kWeapBeamUnderShip) != 0) {
                if (world->fastBlit != 0) {
                    p1.h = beamData[count].head.h;
                    p1.v = beamData[count].head.v;
                    p2.h = beamData[count].tail.h;
                    p2.v = beamData[count].tail.v;
                    density = weaponStats[beamData[count].weapType].lightningDensity;
                    if (density > 0) {
                        if (useLegacyRenderingPipeline)
                            EraseLightningBeam(gSpriteWorld->workFrameP,
                                         gSpriteWorld->windowFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamWidth, density,
                                         weaponStats[beamData[count].weapType].lightningAmplitude);
                        else
                            EraseLightningBeam(gSpriteWorld->workFrameP,
                                         gSpriteWorld->workFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamWidth, density,
                                         weaponStats[beamData[count].weapType].lightningAmplitude);
                    } else {
                        if (useLegacyRenderingPipeline)
                            EraseBeam(gSpriteWorld->workFrameP,
                                         gSpriteWorld->windowFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamWidth,
                                         weaponStats[beamData[count].weapType].coronaFalloff);
                        else
                            EraseBeam(gSpriteWorld->workFrameP,
                                         gSpriteWorld->workFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamWidth,
                                         weaponStats[beamData[count].weapType].coronaFalloff);
                    }
                }
            } else {
                p1.h = beamData[count].head.h;
                p1.v = beamData[count].head.v;
                p2.h = beamData[count].tail.h;
                p2.v = beamData[count].tail.v;
                density = weaponStats[beamData[count].weapType].lightningDensity;
                if (density > 0) {
                    if (world->fastBlit != 0) {
                        if (useLegacyRenderingPipeline)
                            DrawLightningBeam(gSpriteWorld->workFrameP,
                                         gSpriteWorld->windowFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamColor,
                                         weaponStats[beamData[count].weapType].beamWidth, density,
                                         weaponStats[beamData[count].weapType].lightningAmplitude,
                                         0x20 - beamData[count].frame);
                        else
                            DrawLightningBeam(gSpriteWorld->workFrameP,
                                         gSpriteWorld->workFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamColor,
                                         weaponStats[beamData[count].weapType].beamWidth, density,
                                         weaponStats[beamData[count].weapType].lightningAmplitude,
                                         0x20 - beamData[count].frame);
                    } else {
                        if (useLegacyRenderingPipeline)
                            DrawLightningBeam(gSpriteWorld->workFrameP,
                                         0,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamColor,
                                         weaponStats[beamData[count].weapType].beamWidth, density,
                                         weaponStats[beamData[count].weapType].lightningAmplitude,
                                         0x20 - beamData[count].frame);
                        else
                            DrawLightningBeam(gSpriteWorld->workFrameP,
                                         gSpriteWorld->workFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamColor,
                                         weaponStats[beamData[count].weapType].beamWidth, density,
                                         weaponStats[beamData[count].weapType].lightningAmplitude,
                                         0x20 - beamData[count].frame);
                    }
                } else {
                    DrawBeam(gSpriteWorld->workFrameP, p1, p2,
                                 weaponStats[beamData[count].weapType].beamColor,
                                 weaponStats[beamData[count].weapType].beamWidth,
                                 weaponStats[beamData[count].weapType].coronaColor,
                                 weaponStats[beamData[count].weapType].coronaFalloff +
                                     beamData[count].frame);
                    if (world->fastBlit != 0) {
                        if (useLegacyRenderingPipeline)
                            DrawBeam(gSpriteWorld->windowFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamColor,
                                         weaponStats[beamData[count].weapType].beamWidth,
                                         weaponStats[beamData[count].weapType].coronaColor,
                                         weaponStats[beamData[count].weapType].coronaFalloff +
                                             beamData[count].frame);
                        else
                            DrawBeam(gSpriteWorld->workFrameP,
                                         p1, p2,
                                         weaponStats[beamData[count].weapType].beamColor,
                                         weaponStats[beamData[count].weapType].beamWidth,
                                         weaponStats[beamData[count].weapType].coronaColor,
                                         weaponStats[beamData[count].weapType].coronaFalloff +
                                             beamData[count].frame);
                    }
                }
            }
        }
        count++;
    } while (count < kMaxBeams);
#else
    if (world->fastBlit != 0) {
        count = 0;
        while (count < kMaxBeams) {
            if (beamData[count].state >= -1 &&
                beamData[count].owner != -1) {
                p1.h = beamData[count].lastHead.h;
                p1.v = beamData[count].lastHead.v;
                p2.h = beamData[count].lastTail.h;
                p2.v = beamData[count].lastTail.v;
                if (weaponStats[beamData[count].weapType].lightningDensity > 0) {
                    EraseLightningBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                 weaponStats[beamData[count].weapType].beamWidth,
                                 weaponStats[beamData[count].weapType].lightningDensity,
                                 weaponStats[beamData[count].weapType].lightningAmplitude);
                } else {
                    EraseBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                 0x20, 0x10);
                }
            }
            count++;
        }
    }

    count = 0;
    while (count < kMaxBeams) {
        if (beamData[count].state >= 0 &&
            beamData[count].owner != -1) {
            if ((weaponStats[beamData[count].weapType].flags2 & kWeapBeamUnderShip) != 0) {
                if (world->fastBlit != 0) {
                    p1.h = beamData[count].head.h;
                    p1.v = beamData[count].head.v;
                    p2.h = beamData[count].tail.h;
                    p2.v = beamData[count].tail.v;
                    if (weaponStats[beamData[count].weapType].lightningDensity > 0) {
                        EraseLightningBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                     weaponStats[beamData[count].weapType].beamWidth,
                                     weaponStats[beamData[count].weapType].lightningDensity,
                                     weaponStats[beamData[count].weapType].lightningAmplitude);
                    } else {
                        EraseBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                     weaponStats[beamData[count].weapType].beamWidth,
                                     weaponStats[beamData[count].weapType].coronaFalloff);
                    }
                }
            } else {
                p1.h = beamData[count].head.h;
                p1.v = beamData[count].head.v;
                p2.h = beamData[count].tail.h;
                p2.v = beamData[count].tail.v;
                density = weaponStats[beamData[count].weapType].lightningDensity;
                if (density > 0) {
                    if (world->fastBlit != 0) {
                        DrawLightningBeam(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP, p1, p2,
                                     weaponStats[beamData[count].weapType].beamColor,
                                     weaponStats[beamData[count].weapType].beamWidth, density,
                                     weaponStats[beamData[count].weapType].lightningAmplitude,
                                     0x20 - beamData[count].frame);
                    } else {
                        DrawLightningBeam(gSpriteWorld->workFrameP, 0, p1, p2,
                                     weaponStats[beamData[count].weapType].beamColor,
                                     weaponStats[beamData[count].weapType].beamWidth, density,
                                     weaponStats[beamData[count].weapType].lightningAmplitude,
                                     0x20 - beamData[count].frame);
                    }
                } else {
                    DrawBeam(gSpriteWorld->workFrameP, p1, p2,
                                 weaponStats[beamData[count].weapType].beamColor,
                                 weaponStats[beamData[count].weapType].beamWidth,
                                 weaponStats[beamData[count].weapType].coronaColor,
                                 beamData[count].frame +
                                     weaponStats[beamData[count].weapType].coronaFalloff);
                    if (world->fastBlit != 0) {
                        DrawBeam(gSpriteWorld->windowFrameP, p1, p2,
                                     weaponStats[beamData[count].weapType].beamColor,
                                     weaponStats[beamData[count].weapType].beamWidth,
                                     weaponStats[beamData[count].weapType].coronaColor,
                                     beamData[count].frame +
                                         weaponStats[beamData[count].weapType].coronaFalloff);
                    }
                }
            }
        }
        count++;
    }
#endif
}

/*
 * Same table, same weapon fields as BeamEraseCallback, but gated by weaponStats[].flags2
 * bit 0x2000, drawing from a SECOND coordinate pair (unk0/unk2/unk4/unk6,
 * immediately before x1/y1) with the beam and corona COLOURS added.
 */

/* BeamDrawOffscreenCallback (mac _BeamDrawOffscreenCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00438c40
// MAC: _BeamDrawOffscreenCallback i386 0x00037e90 ppc 0x0003dd70
void BeamDrawOffscreenCallback(SpriteWorldPtr world)
{
    short count;
    short density;
    Point p1, p2;

    count = 0;
    while (count < kMaxBeams) {
        if (beamData[count].state >= 0 &&
            beamData[count].owner != -1 &&
            (weaponStats[beamData[count].weapType].flags2 & kWeapBeamUnderShip) != 0) {
            p1.h = beamData[count].head.h;
            p1.v = beamData[count].head.v;
            p2.h = beamData[count].tail.h;
            p2.v = beamData[count].tail.v;
            density = weaponStats[beamData[count].weapType].lightningDensity;
            if (density > 0) {
                DrawLightningBeam(gSpriteWorld->workFrameP, 0, p1, p2,
                             weaponStats[beamData[count].weapType].beamColor,
                             weaponStats[beamData[count].weapType].beamWidth,
                             density,
                             weaponStats[beamData[count].weapType].lightningAmplitude,
                             0x20 - beamData[count].frame);
            } else {
                DrawBeam(gSpriteWorld->workFrameP, p1, p2,
                             weaponStats[beamData[count].weapType].beamColor,
                             weaponStats[beamData[count].weapType].beamWidth,
                             weaponStats[beamData[count].weapType].coronaColor,
                             weaponStats[beamData[count].weapType].coronaFalloff +
                                 beamData[count].frame);
            }
        }
        count++;
    }
}

#define kSoundVolumeScale 1.2e-05

/* HandleSpriteMurkiness. STATE: MATCH 100.0%, exe 110 / obj 110, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00438db0
// MAC: _HandleSpriteMurkiness i386 0x0002b42f ppc 0x0002fa00
void HandleSpriteMurkiness(short x, short y, SpriteRec *sprite)
{
    int32_t dx, dy;
    int32_t dist;
    float f;

#ifdef MACBUILD
    dx = abs((shipData->pos.x - x));
    dy = abs((shipData->pos.y - y));
#else
    f = fabs(shipData->pos.x - x);
    dx = f;
    f = fabs(shipData->pos.y - y);
    dy = f;
#endif
    dist = dx * dx + dy * dy;
    sprite->tintMode = dist * playerEffectiveMurk * kSoundVolumeScale;
    if (gSpriteWorld->pixelDepth == 8) {
        if (sprite->tintMode > 0x18)
            sprite->tintMode = 0x18;
    } else {
        if (sprite->tintMode > 0x1f)
            sprite->tintMode = 0x1f;
    }
    if (sprite->tintMode < 0)
        sprite->tintMode = 0;
    sprite->tintColor = systemData[shipData->curSystem].mapPixel;
}

/* floatingMapFrame/floatingEscortMenuFrame are new (name invented) - a per-strip
   SWFrame PostAnimateHandler doesn't need because it blits the whole rect rather
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
// MAC: _HandleMouse i386 0x00037fba ppc 0x0003de90
void HandleMouse(void)
{
    short s;
    Rect box;
    Boolean canUntarget;
    Boolean canCloaked;
    Point mouse;
    short frame;
    Rect *port;
    Rect *fr;
    SpriteRec *spr;
    short i;
    short j;
    Boolean hit;

    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    GetMouse(&mouse);
    port = GetCWindowPortRect((WindowRef)gWind);
#else
    GetGlobalMouse(&mouse);
    port = GetCWindowPortRect(gWind);
#endif
    box = *port;
    if (PtInRectPoint(mouse, &box))
        HideTheCursor();
    else
        ShowTheCursor();
#ifdef CEFIX /* F14.01 */
    box.right -= ceStatusBarWidth;
#else
    box.right -= kStatusBarWidth;
#endif

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
    if (inBackground == 0 && cursorHidden > 0 && cursorAlpha > 0) {

    hit = 0;
    canUntarget = ShipCanTargetUntargetableShips(shipData);
    canCloaked = ShipCanTargetCloakedShips(shipData);
    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].active == 0)
            continue;
        if (IsDying(&shipData[i]))
            continue;
        if (!ShipIsCloaked(&shipData[i]) || canCloaked != 0
            || shipData[i].parentShip == 0 || i == 0) {
            if (shipData[i].curSystem != shipData->curSystem)
                continue;
            if (canUntarget == 0
                && (shipClass[shipData[i].type].flags2 & kShipUntargetable) != 0)
                continue;
            spr = (SpriteRec *)shipData[i].sprite;
            if (spr == NULL)
                continue;
            fr = &spr->destFrameRect;
            box = *fr;
            if (box.right - box.left < kMinHotRectSize)
                InsetRect(&box, kHotRectInset, kHotRectInset);
            if (PtInRectPoint(mouse, &box)) {
                if (shipData->targetShip != -1 || i != 0) {
                    hit = 1;
                    break;
                }
            }
        }
    }

    if (hit == 0 && shipData->jumpTimer <= 0.0f) {
        for (j = 0; j < kStellarSprites; j++) {
            spr = stellarSprite[j];
            if (spr == NULL)
                continue;
            if (spr->isVisible == 0)
                continue;
            s = spr->userData;
            if (stellarData[s].loaded == 0)
                continue;
            fr = &spr->destFrameRect;
            box = *fr;
            if (box.right - box.left < kMinHotRectSize)
                InsetRect(&box, kHotRectInset, kHotRectInset);
            if (PtInRectPoint(mouse, &box)) {
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
        cursorSprite->tintRed = (cursorAlpha < cursorSprite->translucency)
            ? cursorAlpha : cursorSprite->translucency;
        cursorSprite->tintGreen = (cursorAlpha < cursorSprite->translucency)
            ? cursorAlpha : cursorSprite->translucency;
        cursorSprite->tintBlue = (cursorAlpha < cursorSprite->translucency)
            ? cursorAlpha : cursorSprite->translucency;
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

#define kCronIterationLimit    10000
#define kCronRandomRange       101

/* CronEventHandler (mac _CronEventHandler). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439500
// MAC: _CronEventHandler i386 0x0003874f ppc 0x0003e6d4
void CronEventHandler(void)
{
    short i;

    for (i = 0; i < kMaxCrons; i++) {
        if (cronEvent[i].duration < 0)
            continue;
        if (cronEvent[i].loaded == 0)
            continue;
        if (cronEvent[i].active != 0) {
            if (cronEvent[i].holdoffLeft > 0) {
                cronEvent[i].holdoffLeft -= 1;
                if (cronEvent[i].holdoffLeft > 0)
                    continue;
                if (cronEvent[i].daysLeft < 0) {
                } else {
                    ActivateCron(i);
                    if (cronEvent[i].daysLeft != 0)
                        continue;
                    TerminateCron(i);
                    continue;
                }
                cronEvent[i].active = 0;
                continue;
            }
            cronEvent[i].daysLeft -= 1;
            if (cronEvent[i].daysLeft > 0)
                continue;
            TerminateCron(i);
            if (cronEvent[i].postHoldoff > 0) {
                cronEvent[i].holdoffLeft = cronEvent[i].preHoldoff;
                continue;
            }
            cronEvent[i].active = 0;
            continue;
        }
        if (Rand(kCronRandomRange) > cronEvent[i].random)
            continue;
        if (CronDateInRange(&cronEvent[i]) == 0)
            continue;
        if (PlayerMeetsRequirements(cronEvent[i].requires[0], cronEvent[i].requires[1]) == 0)
            continue;
        if (EvalMissionBitTestString(cronEvent[i].enableOn) == 0)
            continue;
        cronEvent[i].active = 1;
        cronEvent[i].daysLeft = cronEvent[i].duration;
        if (cronEvent[i].preHoldoff > 0) {
            crashlog_print("preparing to activate cron ");
            crashlog_print_num(i + kFirstResourceID);
            crashlog_print(" in ");
            crashlog_print_num(cronEvent[i].preHoldoff);
            crashlog_print(" days\n");
            cronEvent[i].holdoffLeft = cronEvent[i].preHoldoff;
            continue;
        }
        cronEvent[i].holdoffLeft = 0;
        ActivateCron(i);
        if (cronEvent[i].duration != 0)
            continue;
        TerminateCron(i);
        cronEvent[i].daysLeft = -1;
        if (cronEvent[i].postHoldoff > 0)
            cronEvent[i].holdoffLeft = cronEvent[i].postHoldoff;
    }
}

/* ActivateCron (mac _ActivateCron). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439750
// MAC: _ActivateCron i386 0x000385db ppc 0x0003e564
void ActivateCron(short index)
{
    short count;

    count = 0;
    if ((cronEvent[index].flags & kCronIterateOnActivate) != 0) {
        while (PlayerMeetsRequirements(cronEvent[index].requires[0],
                                       cronEvent[index].requires[1]) != 0
               && EvalMissionBitTestString(cronEvent[index].enableOn) != 0) {
            crashlog_mission_bit_set("iteratively activating cron", 0,
                                     (index + kFirstResourceID), -1,
                                     cronEvent[index].onStart);
            EvalMissionBitSetString(cronEvent[index].onStart);
            count++;
            if (count > kCronIterationLimit) {
                crashlog("warning: iterative cron activation is stuck in a loop, bailing out");
                break;
            }
        }
        crashlog_print("cron ");
        crashlog_print_num(index + kFirstResourceID);
        crashlog_print(" iterated ");
        crashlog_print_num(count);
        crashlog_print(" time");
        if (count != 1)
            crashlog_print("s");
        crashlog_print(" on activation\n");
    } else {
        crashlog_mission_bit_set("activating cron", 0,
                                 (index + kFirstResourceID), -1,
                                 cronEvent[index].onStart);
        EvalMissionBitSetString(cronEvent[index].onStart);
    }
}

/* TerminateCron (mac _TerminateCron). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004398b0
// MAC: _TerminateCron i386 0x00038467 ppc 0x0003e3f4
void TerminateCron(short index)
{
    short count;

    count = 0;
    if ((cronEvent[index].flags & kCronIterateOnEnd) != 0) {
        while (PlayerMeetsRequirements(cronEvent[index].requires[0],
                                       cronEvent[index].requires[1]) != 0
               && EvalMissionBitTestString(cronEvent[index].enableOn) != 0) {
            crashlog_mission_bit_set("iteratively terminating cron", 0,
                                     (index + kFirstResourceID), -1,
                                     cronEvent[index].onEnd);
            EvalMissionBitSetString(cronEvent[index].onEnd);
            count++;
            if (count > kCronIterationLimit) {
                crashlog("warning: iterative cron termination is stuck in a loop, bailing out");
                break;
            }
        }
        crashlog_print("cron ");
        crashlog_print_num(index + kFirstResourceID);
        crashlog_print(" iterated ");
        crashlog_print_num(count);
        crashlog_print(" time");
        if (count != 1)
            crashlog_print("s");
        crashlog_print(" on termination\n");
    } else {
        crashlog_mission_bit_set("terminating cron", 0,
                                 (index + kFirstResourceID), -1,
                                 cronEvent[index].onEnd);
        EvalMissionBitSetString(cronEvent[index].onEnd);
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
// MAC: _FloaterEraseCallback i386 0x00038920 ppc 0x0003e924
void FloaterEraseCallback(void *a)
{
    if (commCount >= 0) {
        if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
            AreWeFrontProcess() == 0) {
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     &commRect, &commRect, 0, NULL);
        } else {
            BlitPixieRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                         &commRect, &commRect);
        }
    }
    if (showFloatingMap >= 0) {
        if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
            AreWeFrontProcess() == 0) {
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     &floatingMapRect, &floatingMapRect, 0, NULL);
        } else {
            BlitPixieRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                         &floatingMapRect, &floatingMapRect);
        }
    }
    if (escortMenuOnscreen >= 0) {
        if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
            AreWeFrontProcess() == 0) {
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->backFrameP->framePort),
                     GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     &escortMenuRect, &escortMenuRect, 0, NULL);
        } else {
            BlitPixieRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                         &escortMenuRect, &escortMenuRect);
        }
    }
}

/* FloaterDrawCallback (mac _FloaterDrawCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439bb0
// MAC: _FloaterDrawCallback i386 0x00038b2d ppc 0x0003eb98
void FloaterDrawCallback(SpriteWorldPtr world)
{
    short tick;
    short blink;

    if (inBackground != 0 || world->fastBlit == 0)
        return;

    if (commCount >= 0) {
        BlitPixieFloatingRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                     &commRect, &commRect, 0, gSpriteWorld->backgroundValue);
    }
    if (showFloatingMap > 0) {
        tick = (floatingMapStartTime + 0x96) - ReadTick();
        if (tick <= 0)
            blink = 0x20;
        else if (tick < 0x20)
            blink = 0x20 - tick;
        else
            blink = 0;
        BlitPixieFloatingRectDrawProc(floatingMapFrame, gSpriteWorld->workFrameP,
                     &floatingMapFrame->frameRect, &floatingMapRect, blink,
                     gSpriteWorld->backgroundValue);
    }
    if (escortMenuOnscreen > 0) {
        BlitPixieFloatingRectDrawProc(floatingEscortMenuFrame, gSpriteWorld->workFrameP,
                     &floatingEscortMenuFrame->frameRect, &escortMenuRect,
                     0x20 - escortMenuOnscreen, gSpriteWorld->backgroundValue);
        if (redrawEscortMenu != 0) {
#ifdef MACBUILD
            DrawEscortMenuInFrame(-1);
#else
            DrawEscortMenuInFrame();
#endif
            redrawEscortMenu = 0;
        }
    }
}

/* PostDrawCallback (mac _PostDrawCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439d00
// MAC: _PostDrawCallback i386 0x00038c80 ppc 0x0003ed10
void PostDrawCallback(SpriteWorldPtr world)
{
    FloaterDrawCallback(world);
    DrawParticlesOffscreen(world);
}

/* PostEraseCallback (mac _PostEraseCallback). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439d20
// MAC: _PostEraseCallback i386 0x00038c9f ppc 0x0003ed40
void PostEraseCallback(SpriteWorldPtr world)
{
    BeamEraseCallback(world);
    FloaterEraseCallback(world);
    EraseParticlesOffscreen(world);
}

/* PostAnimateHandler (mac _PostAnimateHandler). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00439d40
// MAC: _PostAnimateHandler i386 0x00038cc6 ppc 0x0003ed78
void PostAnimateHandler(void)
{
    int i;
    int passes;

#ifdef MAC1010
    if (commCount >= 0) {
        if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
            AreWeFrontProcess() == 0) {
            SWSetPortToWindow(gSpriteWorld);
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     GetPortPixMapBits(
                         *(CGrafPtr *)gSpriteWorld->windowFrameP),
                     &commRect, &commRect, 0, NULL);
        } else {
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &commRect, &commRect);
        }
    }
    if (showFloatingMap >= 0) {
        if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
            AreWeFrontProcess() == 0) {
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     GetPortPixMapBits(
                         *(CGrafPtr *)gSpriteWorld->windowFrameP),
                     &floatingMapRect, &floatingMapRect, 0, NULL);
        } else {
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &floatingMapRect, &floatingMapRect);
        }
        if (showFloatingMap == 0)
            showFloatingMap = -1;
    }
    if (escortMenuOnscreen >= 0) {
        if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
            AreWeFrontProcess() == 0) {
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     GetPortPixMapBits(
                         *(CGrafPtr *)gSpriteWorld->windowFrameP),
                     &escortMenuRect, &escortMenuRect, 0, NULL);
        } else {
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &escortMenuRect, &escortMenuRect);
        }
        if (escortMenuOnscreen == 0)
            escortMenuOnscreen = -1;
    }
    UpdateParticlesInWindow(gSpriteWorld);
    for (i = 0; (short)i < (doubleTime ? 2 : 1); i++)
        SetParticleScrollOffset((short)(shipData->pos.x - screenCenter.h),
                     (short)(shipData->pos.y - screenCenter.v));
#elif defined(V111)
    if (useLegacyRenderingPipeline) {
        if (commCount >= 0) {
            if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
                AreWeFrontProcess() == 0) {
                CopyBits(GetPortPixMapBits(
                             gSpriteWorld->workFrameP->framePort),
                         GetPortPixMapBits(
                             *(CGrafPtr *)gSpriteWorld->windowFrameP),
                         &commRect, &commRect, 0, NULL);
            } else {
                BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                             &commRect, &commRect);
            }
        }
        if (showFloatingMap >= 0) {
            if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
                AreWeFrontProcess() == 0) {
                CopyBits(GetPortPixMapBits(
                             gSpriteWorld->workFrameP->framePort),
                         GetPortPixMapBits(
                             *(CGrafPtr *)gSpriteWorld->windowFrameP),
                         &floatingMapRect, &floatingMapRect, 0, NULL);
            } else {
                BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                             &floatingMapRect, &floatingMapRect);
            }
            if (showFloatingMap == 0)
                showFloatingMap = -1;
        }
        if (escortMenuOnscreen >= 0) {
            if (inBackground != 0 || gSpriteWorld->fastBlit == 0 ||
                AreWeFrontProcess() == 0) {
                CopyBits(GetPortPixMapBits(
                             gSpriteWorld->workFrameP->framePort),
                         GetPortPixMapBits(
                             *(CGrafPtr *)gSpriteWorld->windowFrameP),
                         &escortMenuRect, &escortMenuRect, 0, NULL);
            } else {
                BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                             &escortMenuRect, &escortMenuRect);
            }
            if (escortMenuOnscreen == 0)
                escortMenuOnscreen = -1;
        }
        UpdateParticlesInWindow(gSpriteWorld);
        i = 0;
        while ((short)i++ < (doubleTime ? 2 : 1))
            SetParticleScrollOffset((short)(shipData->pos.x - screenCenter.h),
                         (short)(shipData->pos.y - screenCenter.v));
    } else {
        SWSetPortToWindow(gSpriteWorld);
        UpdateParticlesInWindow(gSpriteWorld);
        i = 0;
        while ((short)i++ < (doubleTime ? 2 : 1))
            SetParticleScrollOffset((short)(shipData->pos.x - screenCenter.h),
                         (short)(shipData->pos.y - screenCenter.v));
        CopyBits(GetPortPixMapBits(
                     gSpriteWorld->workFrameP->framePort),
                 GetPortPixMapBits(
                     *(CGrafPtr *)gSpriteWorld->windowFrameP),
                 &gSpriteWorld->workFrameP->frameRect,
                 &gSpriteWorld->windowFrameP->frameRect, 0, NULL);
    }
#else
    if (commCount >= 0) {
        if (inBackground == 0 && gSpriteWorld->fastBlit != 0 &&
            AreWeFrontProcess() != 0) {
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &commRect, &commRect);
        } else {
            SWSetPortToWindow(gSpriteWorld);
            CopyBits(GetPortBitMapForCopyBits(
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
            CopyBits(GetPortBitMapForCopyBits(
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
            CopyBits(GetPortBitMapForCopyBits(
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
#endif
}

#define kReinforceIndexA    0x132
#define kReinforceIndexB    0x133

#define kAnnounceFraction   0.25     /* of reinforceTime, a pooled double */
#define kReinforceReserve       4

/* HandleReinforcements (mac _HandleReinforcements). STATE: MATCH 100.0%, exe 173 / obj 173
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0043a020
// MAC: _HandleReinforcements i386 0x00039048 ppc 0x0003f228
void HandleReinforcements(void)
{
    Str255 msg;
    Str255 govtName;
    short sys;

    sys = shipData[0].curSystem;
    if (systemData[sys].reinforceFleet != -1 && systemData[sys].reinforceTimer > 0.0F) {
        systemData[sys].reinforceTimer -= gSpeedMult;
        if (systemData[sys].reinforceTimer <= 0.0F) {
            HyperSpawnFleet(sys, systemData[sys].reinforceFleet, kReinforceReserve, 1);
            systemData[sys].reinforceTimer = -1.0;
            systemData[sys].reinforceDaysLeft = systemData[sys].reinforceInterval;
            if (systemData[sys].reinforceDaysLeft < 1)
                systemData[sys].reinforceDaysLeft = 1;
        } else {
            if (systemData[sys].reinforceTimer
                    < systemData[sys].reinforceTime * kAnnounceFraction
                && systemData[sys].reinforceDaysLeft <= 0) {
                if (govtMediumNames[fleetData[systemData[sys].reinforceFleet].govt][0]
                        != 0) {
                    strcpy((char *)msg, "");
                    StrcatIndCString((char *)msg, kGameStrList, kReinforceIndexA);
                    strcat((char *)msg, " ");
                    pstrcpy(govtName,
                            govtMediumNames[fleetData[systemData[sys].reinforceFleet].govt]);
                    PtoCstr(govtName);
                    strcat((char *)msg, (char *)govtName);
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, kReinforceIndexB);
                    CtoPstr((char *)msg);
                    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                    DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize,
                                &rgbWhite, 0, 0);
                }
                systemData[sys].reinforceDaysLeft = systemData[sys].reinforceInterval;
                if (systemData[sys].reinforceDaysLeft < 1)
                    systemData[sys].reinforceDaysLeft = 1;
            }
        }
    }
}

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
// MAC: _HandleShipPointDefense i386 0x000392c4 ppc 0x0003f48c
void HandleShipPointDefense(ShipObj *ship)
{
    short i;
    short weap;
    short target;
    short angle;
    short targetIsShip;
    short bestAngle;
    int32_t dist;
    int32_t bestDist;
    int32_t range;
    short shot;
    short slot;
    Boolean fired;

    if (ship->jumpTimer > 0.0f)
        return;

    weap = -1;
    target = -1;
    targetIsShip = -1;
    fired = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if ((weaponStats[i].guidance == kGuidancePointDef
                    || weaponStats[i].guidance == kGuidancePDBeam)
                && ship->weap[i].count > 0
                && ship->weap[i].reloadTimer <= 0.0f
                && ((weaponStats[i].flags2 & kWeapFireWhileCloaked) != 0
                    || !ShipIsCloaked(ship))
                && WeaponHasAmmo(ship, i)) {
            weap = i;
            break;
        }
    }
    if (weap == -1)
        return;

    if (weaponStats[weap].guidance == kGuidancePointDef) {
        range = weaponStats[weap].totalRange;
        range *= kPDRangeSlack;
    } else {
        range = weaponStats[weap].beamLength;
    }
    range *= range;

    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life > 0.0f
                && weaponStats[shotData[i].weapType].guidance == kGuidanceHoming
                && shotData[i].guidanceState == 0
                && (weaponStats[shotData[i].weapType].flags & kWeapNoPDTarget) == 0
                && (shotData[i].target == ship->index
                    || (ship->parentShip != -1
                        && ship->parentShip == shotData[i].target))) {
            dist = XYFloatSquaredDist(&shotData[i].pos,
                                            &ship->pos);
            if (dist > range)
                continue;
            if (dist < bestDist || target == -1) {
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
    }

    if (target == -1) {
        for (i = 0; i < kMaxShips; i++) {
            if (i == ship->index)
                continue;
            if (shipData[i].active == 0)
                continue;
            if ((shipClass[shipData[i].type].flags2 & kShipPDTarget) == 0)
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
            if (dist < bestDist || target == -1) {
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
                    bestDist = dist;
                    bestAngle = angle;
                    target = i;
                    targetIsShip = 1;
                }
            }
        }
    }
    if (target == -1)
        return;

    if (weaponStats[weap].guidance == kGuidancePointDef) {
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
            if (weaponStats[weap].inaccuracy > 0)
                shotData[shot].angle += (Rand(weaponStats[weap].inaccuracy * 2)
                                          - weaponStats[weap].inaccuracy);
            if (shotData[shot].angle < 0.0f)
                shotData[shot].angle += kFullCircleF;
            if (shotData[shot].angle >= kFullCircleF)
                shotData[shot].angle -= kFullCircleF;
            Accel(shotData[shot].angle, weaponStats[weap].speed100,
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
        ship->weap[weap].reloadTimer += weaponStats[weap].reload / ship->weap[weap].count;
        if (weaponStats[weap].sound >= 0) {
            if ((weaponStats[weap].flags & kWeapLoopSound) != 0) {
                if (!ST_IsSoundPlaying(shotSnd[weaponStats[weap].sound]))
                    PlaySoundDistance(-1, shotSnd[weaponStats[weap].sound], 3,
                                    &ship->pos, &shipData->pos);
            } else {
                PlaySoundDistance(-1, shotSnd[weaponStats[weap].sound], 3,
                                &ship->pos, &shipData->pos);
            }
        }

        if (weaponStats[weap].ammoType != -1) {
            if (weaponStats[weap].ammoType >= 0) {
                if ((weaponStats[weap].flags3 & kWeapAmmoAtBurstEnd) == 0) {
                    if (ship->index == 0) {
                        if (weaponStats[weap].ammoType >= 0
                                && weaponStats[weap].ammoType < kMaxWeapTypes)
                            slot = weaponStats[weap].ammoType;
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
                    statusRedraw[kRedrawWeap] = 1;
                }
            } else if (weaponStats[weap].ammoType <= -kWeapAmmoFuelBase) {
                ship->fuel -= (abs(weaponStats[weap].ammoType) - kWeapAmmoFuelBase)
                             * kFuelPerAmmoUnit;
                if (ship->fuel < 0.0f)
                    ship->fuel = 0.0f;
                if (ship->index == 0)
                    statusRedraw[kRedrawFuel] = 1;
            }
        }

        if (weaponStats[weap].burstCount > 0) {
            ship->weap[weap].burstCount += 1;
            if (ship->weap[weap].burstCount
                    >= WeaponBurstInterval(weap, ship->weap[weap].count)) {
                ship->weap[weap].burstCount = 0;
                ship->weap[weap].reloadTimer = weaponStats[weap].burstReload;
                if ((weaponStats[weap].flags3 & kWeapAmmoAtBurstEnd) != 0
                        && (weaponStats[weap].ammoType >= 0
                            && weaponStats[weap].ammoType < kMaxWeapTypes)) {
                    if (ship->index == 0) {
                        ship->weap[weaponStats[weap].ammoType].ammo -= 1;
                        statusRedraw[kRedrawWeap] = 1;
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
// MAC: _HandleGravity i386 0x00039bc3 ppc 0x0003fe94
void HandleGravity(void)
{
    short i;
    short j;
    short stellar;
    FloatPoint target;

    for (i = 0; i < kNavSlots; i++) {
        stellar = systemData[shipData[0].curSystem].navDefault[i];
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
                         stellarData[stellar].gravity * gSpeedMult,
                         &shipData[j].vel);
        }
    }
}

/* HandleDeadlyStellars. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043aed0
// MAC: _HandleDeadlyStellars i386 0x00039ccd ppc 0x00040058
void HandleDeadlyStellars(void)
{
    short i;
    short j;
    short stellar;

    for (i = 0; i < kNavSlots; i++) {
        stellar = systemData[shipData[0].curSystem].navDefault[i];
        if (stellar == -1)
            continue;
        if (stellarData[stellar].sprite == NULL)
            continue;
        if ((stellarData[stellar].flags2 & kSpobDeadly) == 0)
            continue;
        for (j = 0; j < 64; j++) {
            if (shipData[j].active == 0)
                continue;
            if (IsDying(&shipData[j]))
                continue;
            if (ShipResistsDeadlyStellars(&shipData[j]))
                continue;
            if (!SWRLECollision(stellarData[stellar].sprite,
                                shipData[j].sprite))
                continue;
            shipData[j].active = 0;
            shipData[j].armor = -1000.0f;
            shipData[j].deathTimer = 0;
            if (shipData[0].targetShip == j) {
                shipData[0].targetShip = -1;
                statusRedraw[kRedrawTarg] = 1;
            }
            CreateExplosion(shipData[j].pos,
                            shipClass[shipData[j].type].explode2, 0, 1);
        }
    }
}

/* mac twin _ShipAccelRate, matched by AdjustInertialessShipVelocity's whole
   call shape (Accel + this + gSpeedMult clamp), same field
   offsets */

/* stride 0x18, 32 entries */


#define kPodFadeLife    32.0f

/* AdjustInertialessShipVelocity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b020
// MAC: _AdjustInertialessShipVelocity i386 0x0003349c ppc 0x00038e10
void AdjustInertialessShipVelocity(ShipObj *ship)
{
    float limit;
    float velX;
    float velY;

    velX = ship->vel.x;
    velY = ship->vel.y;
    ship->vel.x = ship->vel.y = 0.0f;
    Accel(ship->heading, ship->accelMag, &ship->vel);
    limit = ShipAccelRate(ship) * 4.0f * gSpeedMult;
    if (velX <= ship->vel.x - limit)
        velX = velX + limit;
    else if (velX >= ship->vel.x + limit)
        velX = velX - limit;
    else
        velX = ship->vel.x;
    if (velY <= ship->vel.y - limit)
        velY = velY + limit;
    else if (velY >= ship->vel.y + limit)
        velY = velY - limit;
    else
        velY = ship->vel.y;
    ship->vel.x = velX;
    ship->vel.y = velY;
}

/* HandleEscapePods. STATE: MATCH, 229/229; byte check pending the next regate (bytediff
   reads the baseline). OPEN: none */
// FUNCTION: EVNOVA 0x0043b170
// MAC: _HandleEscapePods i386 0x00039e48 ppc 0x00040204
void HandleEscapePods(void)
{
    short i;
    short xSize;
    short ySize;

    for (i = 0; i < kMaxPods; i++) {
        if (podData[i].life >= 0.0f) {
            if (killShots != 0)
                podData[i].life = -1.0f;
            podData[i].pos.x += podData[i].vel.x * gSpeedMult;
            podData[i].pos.y += podData[i].vel.y * gSpeedMult;
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
            podData[i].life -= gSpeedMult;
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
