/* evprefsdialogwrapper.m -- the mac preferences layer: the NSUserDefaults load/save and the Cocoa entry points the Carbon engine calls; the mac 1.1.1 EVPrefsDialogWrapper.o (mac only). Notes: Research/notes/evprefsdialog.md */
#import <Cocoa/Cocoa.h>
#include "player.h"
#include "evprefsdialog.h"

extern double userSpeedSetting;

static EVPrefsDialogController *dialog;

/* InitCocoa (mac _InitCocoa, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _InitCocoa i386 0x000cd3f8 ppc 0x000d9140
void InitCocoa(void)
{
    NSApplicationLoad();
}

/* RunPrefsDialog (mac _RunPrefsDialog, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _RunPrefsDialog i386 0x000cd401 ppc 0x000d9144
void RunPrefsDialog(void)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (dialog == nil)
        dialog = [[EVPrefsDialogController alloc] initWithWindowNibName:@"Keys"];
    [dialog performSelector:@selector(bump:) withObject:nil afterDelay:1.0
                    inModes:[NSArray arrayWithObjects:NSModalPanelRunLoopMode, NSDefaultRunLoopMode, nil]];
    [NSApp runModalForWindow:[dialog window]];
    [dialog release];
    dialog = nil;
    [pool release];
}

/* checkForUpdates (mac _checkForUpdates, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _checkForUpdates i386 0x000cd54b ppc 0x000d9294
BOOL checkForUpdates(void)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    BOOL check = YES;
    id value = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"SUCheckAtStartup"];

    if (value)
        check = [value boolValue];
    [pool release];
    return check;
}

void loadKeys(void);

/* LoadPrefs (mac _LoadPrefs, mac-only: the win LoadPrefs is a different, file-based routine). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _LoadPrefs i386 0x000ce758 ppc 0x000da0bc
void LoadPrefs(Boolean loadWindowed)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSDictionary *initial = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithBool:YES], @"ShipAnimations",
        [NSNumber numberWithBool:YES], @"EngineGlows",
        [NSNumber numberWithBool:YES], @"RunningLights",
        [NSNumber numberWithBool:YES], @"WeaponEffects",
        [NSNumber numberWithBool:NO], @"SmokeTrails",
        [NSNumber numberWithBool:YES], @"ParallaxStarfield",
        [NSNumber numberWithBool:YES], @"IntroMusic",
        [NSNumber numberWithBool:NO], @"RunInAWindow",
        [NSNumber numberWithBool:NO], @"QuickTimeMovies",
        [NSNumber numberWithBool:YES], @"AmbientSounds",
        [NSNumber numberWithBool:NO], @"HyperspaceEffects",
        [NSNumber numberWithBool:YES], @"SUAutomaticVersionCheckEnabled",
        [NSNumber numberWithBool:YES], @"SUCheckOnStartup",
        [NSNumber numberWithBool:NO], @"SteakKnives",
        [NSNumber numberWithBool:YES], @"ShowMapBorders",
        [NSNumber numberWithBool:NO], @"VoicesOn",
        [NSNumber numberWithBool:NO], @"Clipping",
        [NSNumber numberWithInt:4], @"Volume",
        nil];
    NSUserDefaults *defaults;
    id value;
    double speed;

    [[NSUserDefaultsController sharedUserDefaultsController] setInitialValues:initial];
    [[NSUserDefaultsController sharedUserDefaultsController] setAppliesImmediately:NO];
    defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];

    value = [defaults objectForKey:@"IntroMusic"];
    settings.introMusic = value ? [value boolValue] : YES;
    value = [defaults objectForKey:@"VoicesOn"];
    settings.voicesOn = value ? [value boolValue] : NO;
    value = [defaults objectForKey:@"Clipping"];
    settings.clipping = value ? [value boolValue] : NO;
    value = [defaults objectForKey:@"QuickTimeMovies"];
    settings.noMovies = value ? [value boolValue] : NO;
    value = [defaults objectForKey:@"SmokeTrails"];
    settings.noDebris = value ? [value boolValue] : NO;
    value = [defaults objectForKey:@"ParallaxStarfield"];
    settings.parallaxStars = value ? [value boolValue] : YES;
    settings.yieldEvents = 0;
    value = [defaults objectForKey:@"AmbientSounds"];
    settings.ambientSounds = value ? [value boolValue] : YES;
    value = [defaults objectForKey:@"ShowMapBorders"];
    settings.showMapBorders = value ? [value boolValue] : YES;
    value = [defaults objectForKey:@"HyperspaceEffects"];
    settings.noHyperGamma = value ? [value boolValue] : NO;
    value = [defaults objectForKey:@"SUAutomaticVersionCheckEnabled"];
    settings.noVersionCheck = value ? [value boolValue] : YES;
    if (loadWindowed) {
        value = [defaults objectForKey:@"RunInAWindow"];
        settings.windowed = value ? [value boolValue] : NO;
        value = [defaults objectForKey:@"ShipAnimations"];
        settings.shipAnimations = value ? [value boolValue] : YES;
        value = [defaults objectForKey:@"EngineGlows"];
        settings.engineGlows = value ? [value boolValue] : YES;
        value = [defaults objectForKey:@"WeaponEffects"];
        settings.weaponGlows = value ? [value boolValue] : YES;
        value = [defaults objectForKey:@"RunningLights"];
        settings.runningLights = value ? [value boolValue] : YES;
    }
    value = [defaults objectForKey:@"Volume"];
    settings.volume = value ? [value intValue] : 4;
    value = [defaults objectForKey:@"UserSpeed"];
    speed = 1.0;
    if (value)
        speed = [value floatValue];
    userSpeedSetting = speed * 0.01;
    loadKeys();
    [defaults synchronize];
    [pool release];
}

/* loadKeys (mac _loadKeys, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _loadKeys i386 0x000cd5f8 ppc 0x000d9340
void loadKeys(void)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
    id value;

#define LOADKEY(name, field, dflt) \
    value = [[defaults objectForKey:[NSString stringWithFormat:@"ShortcutRecorder %@", @name]] valueForKey:@"keyCode"]; \
    keyPrefs.field = value ? [value intValue] : dflt;

    LOADKEY("accelKey", accelerate, 0x7e)
    LOADKEY("reverseKey", reverse, 0x7d)
    LOADKEY("rightTurn", turnRight, 0x7c)
    LOADKEY("leftTurn", turnLeft, 0x7b)
    LOADKEY("afterburner", afterburner, 6)
    LOADKEY("autopilot", faceTarget, 0)
    LOADKEY("hyperMode", hyperspace, 4)
    LOADKEY("hyperSel", nextSystem, 0x2a)
    LOADKEY("jumpKey", jump, 0x26)
    LOADKEY("navOff", clearTarget, 0x32)
    LOADKEY("hailKey", hail, 0x10)
    LOADKEY("landReq", land, 0x25)
    LOADKEY("fireKey0", firePrimary, 0x31)
    LOADKEY("fireKey1", fireSecondary, 0x3b)
    LOADKEY("weapSelect", selectSecondary, 0xd)
    LOADKEY("weapSafe", deselectSecondary, 1)
    LOADKEY("targSel", targetNext, 0x30)
    LOADKEY("closeTarg", targetNearest, 0xf)
    LOADKEY("escortMenuKey", escortMenu, 0xe)
    LOADKEY("escortCmdKey0", escortOrder[0], 3)
    LOADKEY("escortCmdKey1", escortOrder[1], 2)
    LOADKEY("escortCmdKey2", escortOrder[2], 9)
    LOADKEY("escortCmdKey3", escortOrder[3], 8)
    LOADKEY("pauseKey", pause, 0x35)
    LOADKEY("ackComm", dismiss, 0x24)
    LOADKEY("boardKey", board, 0xb)
    LOADKEY("dumpCargo", jettison, 0x28)
    LOADKEY("ejectKey", eject, 7)
    LOADKEY("destructKey", selfDestruct, 0x1b)
    LOADKEY("cloakKey", cloak, 0x20)
    LOADKEY("mapKey", map, 0x2e)
    LOADKEY("playerInfoKey", playerInfo, 0x23)
    LOADKEY("missionInfoKey", missions, 0x22)
    LOADKEY("framerateKey", toggleFPS, 0x6f)
    keyPrefs.navPreset[0] = 0x7a;
    keyPrefs.navPreset[1] = 0x78;
    keyPrefs.navPreset[2] = 0x63;
    keyPrefs.navPreset[3] = 0x76;
    keyPrefs.cheatSpawn = 0x72;
    keyPrefs.cheatKill = 0x75;
    keyPrefs.cheatRevive = 0x73;
    keyPrefs.cheatArmor = 0x74;
    keyPrefs.cheatDamage = 0x79;
    keyPrefs.cheatAmmo = 0x43;
    keyPrefs.cheatCredits = 0x15;
    keyPrefs.cheatSpare = 0x77;
    keyPrefs.cheatShipNext = 0x45;
    keyPrefs.cheatShipPrev = 0x4e;
    keyPrefs.selfDestruct = 0x1b;
    keyPrefs.quickKey[0] = 0x12;
    keyPrefs.quickKey[1] = 0x13;
    keyPrefs.quickKey[2] = 0x14;
    keyPrefs.quickKey[3] = 0x15;
    keyPrefs.quickKey[4] = 0x17;
    [pool release];
}

/* SteakKnivesAndSuch (mac _SteakKnivesAndSuch, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _SteakKnivesAndSuch i386 0x000cf01b ppc 0x000da858
void SteakKnivesAndSuch(void)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    id value = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"SteakKnives"];

    if (value && [value boolValue])
        [[NSAlert alertWithMessageText:@"Steak Knives, 3D, Multiplayer, OMGWTFBBQ!?!?" defaultButton:@"OK"
                       alternateButton:nil otherButton:nil
             informativeTextWithFormat:@"Please email pr@ambrosiasw.com for further information about EV4, EV for iPhone and the entire EV Series for Nintendo Wii and the Xbox Live Arcade"] runModal];
    [pool release];
}

/* SaveMapBordersState (mac _SaveMapBordersState, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _SaveMapBordersState i386 0x000cf110 ppc 0x000da940
void SaveMapBordersState(void)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];

    [defaults setObject:[NSNumber numberWithBool:settings.showMapBorders] forKey:@"ShowMapBorders"];
    [[NSUserDefaultsController sharedUserDefaultsController] save:nil];
    [pool release];
}

/* relative2absolute (mac _relative2absolute, mac-only). STATE: MATCH on both slices (mac gate). OPEN: none */
// MAC: _relative2absolute i386 0x000cf1f4 ppc 0x000daa20
NSString *relative2absolute(NSString *path)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *absolute = [[path stringByExpandingTildeInPath] retain];

    [pool release];
    return absolute;
}
