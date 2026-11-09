/* evprefsdialog.h -- the mac preferences window's Objective-C interface (EVPrefsDialogController and the
   ShortcutRecorder control it holds). Notes: Research/notes/evprefsdialog.md */
#import <Cocoa/Cocoa.h>

/* ShortcutRecorder's key combination: modifier flags and a virtual key code */
typedef struct {
    unsigned int flags;
    short code;
} KeyCombo;

@interface SRRecorderControl : NSControl
- (KeyCombo)keyCombo;
- (void)setKeyCombo:(KeyCombo)newKeyCombo;
@end

@interface EVPrefsDialogController : NSWindowController {
    IBOutlet NSStepper *_volumeStepper;
    IBOutlet NSTextField *_volumeText;
    IBOutlet NSButton *_shipAnimations;
    IBOutlet NSButton *_engineGlows;
    IBOutlet NSButton *_runningLights;
    IBOutlet NSButton *_weaponGlows;
    IBOutlet NSButton *_windowedMode;
    IBOutlet NSButton *_hyperspaceEffects;
    IBOutlet NSButton *_ambientSounds;
    IBOutlet NSButton *_quicktimeMovies;
    IBOutlet NSButton *_introMusic;
    IBOutlet NSButton *_parallaxStarfield;
    IBOutlet NSButton *_smokeTrails;
    IBOutlet SRRecorderControl *_accelerate;
    IBOutlet SRRecorderControl *_reverseCourse;
    IBOutlet SRRecorderControl *_rotateRight;
    IBOutlet SRRecorderControl *_rotateLeft;
    IBOutlet SRRecorderControl *_afterburner;
    IBOutlet SRRecorderControl *_autopilot;
    IBOutlet SRRecorderControl *_hyperspaceMode;
    IBOutlet SRRecorderControl *_hyperspaceSelect;
    IBOutlet SRRecorderControl *_hyperspaceJump;
    IBOutlet SRRecorderControl *_navSystemOff;
    IBOutlet SRRecorderControl *_communicate;
    IBOutlet SRRecorderControl *_landDock;
    IBOutlet SRRecorderControl *_firePrimary;
    IBOutlet SRRecorderControl *_fireSecondary;
    IBOutlet SRRecorderControl *_selectSecondary;
    IBOutlet SRRecorderControl *_weaponSafety;
    IBOutlet SRRecorderControl *_targetSelect;
    IBOutlet SRRecorderControl *_closestTarget;
    IBOutlet SRRecorderControl *_escortMenu;
    IBOutlet SRRecorderControl *_attackTarget;
    IBOutlet SRRecorderControl *_defendMe;
    IBOutlet SRRecorderControl *_holdPosition;
    IBOutlet SRRecorderControl *_recall;
    IBOutlet SRRecorderControl *_pause;
    IBOutlet SRRecorderControl *_acknowledge;
    IBOutlet SRRecorderControl *_board;
    IBOutlet SRRecorderControl *_jettisonCargo;
    IBOutlet SRRecorderControl *_eject;
    IBOutlet SRRecorderControl *_selfDestruct;
    IBOutlet SRRecorderControl *_engageCloak;
    IBOutlet SRRecorderControl *_galaxyMap;
    IBOutlet SRRecorderControl *_playerInfo;
    IBOutlet SRRecorderControl *_missionInfo;
    IBOutlet SRRecorderControl *_showFramerate;
    IBOutlet NSTextField *_relaunchStatus;
    BOOL majorPrefsChange;
}
- (IBAction)bump:(id)sender;
- (void)assignKeys;
- (void)loadKeys;
- (IBAction)accept:(id)sender;
- (IBAction)cancel:(id)sender;
- (void)setStatusForVolume:(int)volume;
- (IBAction)changeVolume:(id)sender;
- (BOOL)shortcutRecorder:(SRRecorderControl *)aRecorder isKeyCode:(signed short)keyCode andFlagsTaken:(unsigned int)flags reason:(NSString **)aReason;
- (IBAction)settingsChange:(id)sender;
- (BOOL)shouldRunErrorAsSheet:(id)sender;
@end
