/* evprefsdialogcontroller.m -- the preferences window controller (volume, graphics options, the key assignments through ShortcutRecorder); the mac 1.1.1 EVPrefsDialogController.o (mac only). Notes: Research/notes/evprefsdialog.md */
#import <Cocoa/Cocoa.h>
#include "player.h"
#include "sound.h"
#include "render.h"
#include "game.h"
#include "evprefsdialog.h"

@implementation EVPrefsDialogController

/* +[EVPrefsDialogController initialize] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: +[EVPrefsDialogController initialize] i386 0x000cf268 ppc 0x000daaa0
+ (void)initialize
{
}

/* -[EVPrefsDialogController dealloc] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController dealloc] i386 0x000cf26d ppc 0x000daaa4
- (void)dealloc
{
    [super dealloc];
}

/* -[EVPrefsDialogController initWithWindowNibName:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController initWithWindowNibName i386 0x000cf297 ppc 0x000daae8
- (id)initWithWindowNibName:(NSString *)windowNibName
{
    return [super initWithWindowNibName:windowNibName];
}

/* -[EVPrefsDialogController windowDidLoad] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController windowDidLoad] i386 0x000cf2c8 ppc 0x000dab2c
- (void)windowDidLoad
{
    majorPrefsChange = NO;
    settings.windowedPending = settings.windowed;
    if (CGDisplayIsCaptured(CGMainDisplayID()))
        [[self window] setLevel:CGShieldingWindowLevel()];
    [self changeVolume:self];
    [self assignKeys];
}

/* -[EVPrefsDialogController bump:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController bump i386 0x000cf350 ppc 0x000dabd0
- (void)bump:(id)sender
{
    if (CGDisplayIsCaptured(CGMainDisplayID()))
        [[self window] setLevel:CGShieldingWindowLevel()];
}

/* -[EVPrefsDialogController assignKeys] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController assignKeys] i386 0x000cf3a1 ppc 0x000dac48
- (void)assignKeys
{
    KeyCombo combo;

    combo.flags = 0;
#define ASSIGNKEY(ivar, field) \
    combo.code = keyPrefs.field; \
    [ivar setKeyCombo:combo];

    ASSIGNKEY(_accelerate, accelerate)
    ASSIGNKEY(_reverseCourse, reverse)
    ASSIGNKEY(_rotateRight, turnRight)
    ASSIGNKEY(_rotateLeft, turnLeft)
    ASSIGNKEY(_afterburner, afterburner)
    ASSIGNKEY(_autopilot, faceTarget)
    ASSIGNKEY(_hyperspaceMode, hyperspace)
    ASSIGNKEY(_hyperspaceSelect, nextSystem)
    ASSIGNKEY(_hyperspaceJump, jump)
    ASSIGNKEY(_navSystemOff, clearTarget)
    ASSIGNKEY(_communicate, hail)
    ASSIGNKEY(_landDock, land)
    ASSIGNKEY(_firePrimary, firePrimary)
    ASSIGNKEY(_fireSecondary, fireSecondary)
    ASSIGNKEY(_selectSecondary, selectSecondary)
    ASSIGNKEY(_weaponSafety, deselectSecondary)
    ASSIGNKEY(_targetSelect, targetNext)
    ASSIGNKEY(_closestTarget, targetNearest)
    ASSIGNKEY(_escortMenu, escortMenu)
    ASSIGNKEY(_attackTarget, escortOrder[0])
    ASSIGNKEY(_defendMe, escortOrder[1])
    ASSIGNKEY(_holdPosition, escortOrder[2])
    ASSIGNKEY(_recall, escortOrder[3])
    ASSIGNKEY(_pause, pause)
    ASSIGNKEY(_acknowledge, dismiss)
    ASSIGNKEY(_board, board)
    ASSIGNKEY(_jettisonCargo, jettison)
    ASSIGNKEY(_eject, eject)
    ASSIGNKEY(_selfDestruct, selfDestruct)
    ASSIGNKEY(_engageCloak, cloak)
    ASSIGNKEY(_galaxyMap, map)
    ASSIGNKEY(_playerInfo, playerInfo)
    ASSIGNKEY(_missionInfo, missions)
    ASSIGNKEY(_showFramerate, toggleFPS)
}

/* -[EVPrefsDialogController loadKeys] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController loadKeys] i386 0x000cfaa8 ppc 0x000db1d4
- (void)loadKeys
{
    KeyCombo combo;

#define LOADKEY(ivar, field) \
    combo = [ivar keyCombo]; \
    keyPrefs.field = combo.code;

    LOADKEY(_accelerate, accelerate)
    LOADKEY(_reverseCourse, reverse)
    LOADKEY(_rotateRight, turnRight)
    LOADKEY(_rotateLeft, turnLeft)
    LOADKEY(_afterburner, afterburner)
    LOADKEY(_autopilot, faceTarget)
    LOADKEY(_hyperspaceMode, hyperspace)
    LOADKEY(_hyperspaceSelect, nextSystem)
    LOADKEY(_hyperspaceJump, jump)
    LOADKEY(_navSystemOff, clearTarget)
    LOADKEY(_communicate, hail)
    LOADKEY(_landDock, land)
    LOADKEY(_firePrimary, firePrimary)
    LOADKEY(_fireSecondary, fireSecondary)
    LOADKEY(_selectSecondary, selectSecondary)
    LOADKEY(_weaponSafety, deselectSecondary)
    LOADKEY(_targetSelect, targetNext)
    LOADKEY(_closestTarget, targetNearest)
    LOADKEY(_escortMenu, escortMenu)
    LOADKEY(_attackTarget, escortOrder[0])
    LOADKEY(_defendMe, escortOrder[1])
    LOADKEY(_holdPosition, escortOrder[2])
    LOADKEY(_recall, escortOrder[3])
    LOADKEY(_pause, pause)
    LOADKEY(_acknowledge, dismiss)
    LOADKEY(_board, board)
    LOADKEY(_jettisonCargo, jettison)
    LOADKEY(_eject, eject)
    LOADKEY(_selfDestruct, selfDestruct)
    LOADKEY(_engageCloak, cloak)
    LOADKEY(_galaxyMap, map)
    LOADKEY(_playerInfo, playerInfo)
    LOADKEY(_missionInfo, missions)
    LOADKEY(_showFramerate, toggleFPS)
}

/* -[EVPrefsDialogController accept:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController accept i386 0x000cfe3a ppc 0x000db868
- (void)accept:(id)sender
{
    [NSApp stopModal];
    [self close];
    [[NSUserDefaultsController sharedUserDefaultsController] save:self];
    settings.introMusic = [_introMusic state];
    settings.noMovies = [_quicktimeMovies state] == 0;
    settings.noDebris = [_smokeTrails state] == 0;
    settings.parallaxStars = [_parallaxStarfield state];
    settings.yieldEvents = 0;
    settings.ambientSounds = [_ambientSounds state];
    settings.noHyperGamma = [_hyperspaceEffects state] == 0;
    settings.windowed = [_windowedMode state];
    settings.volume = [_volumeStepper intValue];
    [self loadKeys];
}

/* -[EVPrefsDialogController cancel:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController cancel i386 0x000cff7c ppc 0x000db99c
- (void)cancel:(id)sender
{
    [NSApp stopModal];
    [self close];
    [[NSUserDefaultsController sharedUserDefaultsController] revert:self];
    if (settings.volume != gSoundVolume / 8) {
        HandleIntroMusic(1);
        AdjustSoundVolume();
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
        [self setStatusForVolume:settings.volume];
    }
}

/* -[EVPrefsDialogController setStatusForVolume:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController setStatusForVolume i386 0x000d0059 ppc 0x000dba8c
- (void)setStatusForVolume:(int)volume
{
    NSString *text;

    switch (volume) {
    case 0:
        text = @"No Bloody Noise";
        break;
    case 1:
        text = @"Damn Quiet";
        break;
    case 2:
        text = @"Quieter";
        break;
    case 3:
        text = @"Quiet";
        break;
    case 4:
        text = @"Normal";
        break;
    case 5:
        text = @"Loud";
        break;
    case 6:
        text = @"Louder";
        break;
    case 7:
        text = @"Damn Loud";
        break;
    case 8:
        text = @"Hoo Boy!";
        break;
    default:
        text = @"You won a set of Steak Knives!";
        break;
    }
    [_volumeText setStringValue:text];
}

/* -[EVPrefsDialogController changeVolume:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController changeVolume i386 0x000d00c7 ppc 0x000dbb58
- (void)changeVolume:(id)sender
{
    int volume = [_volumeStepper intValue];

    [self setStatusForVolume:volume];
    if (![sender isEqual:self]) {
        settings.volume = volume;
        HandleIntroMusic(1);
        AdjustSoundVolume();
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    }
}

/* -[EVPrefsDialogController shortcutRecorder:isKeyCode:andFlagsTaken:reason:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController shortcutRecorder i386 0x000d0162 ppc 0x000dbc14
- (BOOL)shortcutRecorder:(SRRecorderControl *)aRecorder isKeyCode:(signed short)keyCode andFlagsTaken:(unsigned int)flags reason:(NSString **)aReason
{
#define CHECKKEY(ivar, text) \
    if (![aRecorder isEqualTo:ivar] && keyCode == [ivar keyCombo].code) { \
        *aReason = @"it is already in use by " text; \
        return YES; \
    }

    CHECKKEY(_accelerate, "Accelerate")
    CHECKKEY(_reverseCourse, "Reverse")
    CHECKKEY(_rotateRight, "Right")
    CHECKKEY(_rotateLeft, "Left")
    CHECKKEY(_afterburner, "Afterburner")
    CHECKKEY(_autopilot, "Autopilot")
    CHECKKEY(_hyperspaceMode, "Hyperspace Mode")
    CHECKKEY(_hyperspaceSelect, "Hyperspace Select")
    CHECKKEY(_hyperspaceJump, "Hyperspace Jump")
    CHECKKEY(_communicate, "Communicate")
    CHECKKEY(_landDock, "Land")
    CHECKKEY(_firePrimary, "Fire Primary")
    CHECKKEY(_fireSecondary, "Fire Secondary")
    CHECKKEY(_selectSecondary, "Select Secondary")
    CHECKKEY(_weaponSafety, "Weapon Safety")
    CHECKKEY(_targetSelect, "Target Select")
    CHECKKEY(_closestTarget, "Closest Target")
    CHECKKEY(_escortMenu, "Escort Menu")
    CHECKKEY(_attackTarget, "Attack Target")
    CHECKKEY(_defendMe, "Defend Me")
    CHECKKEY(_holdPosition, "Hold Position")
    CHECKKEY(_recall, "Recall")
    CHECKKEY(_pause, "Pause")
    CHECKKEY(_acknowledge, "Acknowledge")
    CHECKKEY(_board, "Board")
    CHECKKEY(_jettisonCargo, "Jettison Cargo")
    CHECKKEY(_eject, "Eject")
    CHECKKEY(_selfDestruct, "Self Destruct")
    CHECKKEY(_engageCloak, "Engage Cloak")
    CHECKKEY(_galaxyMap, "Galaxy Map")
    CHECKKEY(_playerInfo, "Player Info")
    CHECKKEY(_missionInfo, "Mission Info")
    CHECKKEY(_showFramerate, "Show Framerate")
    if (keyCode == keyPrefs.navPreset[0]) {
        *aReason = @"it is already in use by Planet 1";
        return YES;
    }
    if (keyCode == keyPrefs.navPreset[1]) {
        *aReason = @"it is already in use by Planet 2";
        return YES;
    }
    if (keyCode == keyPrefs.navPreset[2]) {
        *aReason = @"it is already in use by Planet 3";
        return YES;
    }
    if (keyCode == keyPrefs.navPreset[3]) {
        *aReason = @"it is already in use by Planet 4";
        return YES;
    }
    return NO;
}

/* -[EVPrefsDialogController settingsChange:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController settingsChange i386 0x000d0b01 ppc 0x000dc8c4
- (void)settingsChange:(id)sender
{
    if ([sender isEqual:_windowedMode]) {
        settings.windowedPending = [_windowedMode state];
        if (settings.windowedPending)
            [_hyperspaceEffects setState:0];
        else
            [_hyperspaceEffects setState:settings.noHyperGamma == 0];
    }
    if (gameLoaded) {
        if (settings.shipAnimations != [_shipAnimations state]
            || settings.engineGlows != [_engineGlows state]
            || settings.runningLights != [_runningLights state]
            || settings.weaponGlows != [_weaponGlows state]
            || settings.windowed != settings.windowedPending) {
            majorPrefsChange = YES;
            [_relaunchStatus setHidden:NO];
        } else {
            majorPrefsChange = NO;
            [_relaunchStatus setHidden:YES];
        }
    }
}

/* -[EVPrefsDialogController shouldRunErrorAsSheet:] (mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: -[EVPrefsDialogController shouldRunErrorAsSheet i386 0x000d0c6c ppc 0x000dca34
- (BOOL)shouldRunErrorAsSheet:(id)sender
{
    return YES;
}

@end
