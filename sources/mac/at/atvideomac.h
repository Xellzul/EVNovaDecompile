/* atvideomac.h -- the Monitor Tool's shared types, Classic. Notes: Research/notes/maclibs.md */
#ifndef ATVIDEOMAC_H
#define ATVIDEOMAC_H
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include <Displays.h>
#include <Video.h>
#include <Gestalt.h>
#include <Devices.h>
#include <Palettes.h>

#define kNotSupported   -7003
#define kVideoNoMode    -7005
#define kVideoNoDevice  -7005
#define kVideoError     -8300
#define kVideoBusy      -8301
#define kModeSize       0x50
#define kNoScore        0x3ffff
#define kFadeTicks      200
#define kGammaHead      12

typedef struct VTMode {
    ULong index;
    ULong depth;
    unsigned char hasColor;
} VTMode;

typedef struct VTModeEntry {
    char name[0x20];
    ULong width;
    ULong height;
    ULong rate;
    ULong depths;
    ULong depthMin;
    ULong depthMax;
    unsigned char stretched;
    unsigned char interlaced;
    VDSwitchInfoRec info;
    char pad4a[2];
    unsigned long displayMode;
} VTModeEntry;

typedef struct VTColor {
    ULong red;
    ULong green;
    ULong blue;
} VTColor;

typedef struct VTClut {
    ULong count;
    VTColor entries[1];
} VTClut;

typedef struct VTDevice {
    VTMode mode;
    VTMode saved;
    long pad18[3];
    VTClut *clutWork;
    VTClut *clutSaved;
    long pad2c;
    void *table30;
    void *table34;
    GammaTblPtr gammaWork;
    GammaTblPtr gammaSaved;
    long pad40;
    GammaTblPtr fadeFrom;
    GammaTblPtr fadeTo;
    VTModeEntry *modes;
    ULong modeCount;
    unsigned char ready;
    unsigned char switched;
    unsigned char isMain;
    unsigned char pad57;
    GDHandle gd;
    DisplayIDType displayID;
} VTDevice;

extern long PlatformOpen(void);
extern void SystemSetQuitProc(void (*proc)(void));
extern void _AtomicEnter(void);
extern void _AtomicLeave(void);

extern GDHandle gVTSavedDevice;
extern VTDevice *gVTCaptured;
extern ULong gVTDeviceCount;
extern VTDevice *gVTDevices;
extern unsigned char gVTOpen;
extern unsigned char gVTMenuVisible;
extern unsigned char gVTMenuWasShown;

void DisposeGammaTools(void);
long VT_DeviceOpen(VTDevice *device, unsigned short index);
void VT_DeviceClose(VTDevice *device);
long VT_DeviceSwitch(VTDevice *device, VTMode *mode);
long GammaFadeToColor(ULong id, void *target, ULong percent, ULong ticks, unsigned char linear);
long VT_Release(void);
long ShowMenuBar(void);
long HideMenuBar(void);
unsigned short VT_CountDevices(void);
ULong VT_ScoreSize(VTModeEntry *entry, VTRequest *req);
ULong VT_ScoreRate(VTModeEntry *entry, VTRequest *req);
ULong VT_ScoreDepth(VTModeEntry *entry, VTRequest *req, ULong *depth);
unsigned char VT_DeviceHasGamma(VTDevice *device);
unsigned char VT_DeviceIsIndexed(VTDevice *device);
VTDevice *VT_DeviceFind(ULong id);
long _ShowMenuBar(void);
long _HideMenuBar(void);
void VT_LockDevices(void);
unsigned char VT_CheckScreens(void);
VTClut *VT_ClutCopy(VTDevice *device);
long VT_ClutSet(VTDevice *device, VTClut *clut);
GammaTblPtr VT_GammaGet(VTDevice *device);
long VT_GammaSet(VTDevice *device, GammaTblPtr table);
GammaTblPtr VT_GammaCopy(VTDevice *device, void *ref);
void VT_GammaFree(VTDevice *device, GammaTblPtr table);
void VT_GammaBlend(VTDevice *device, GammaTblPtr out, GammaTblPtr from, GammaTblPtr to, ULong num, ULong den);
void VT_GammaFill(VTDevice *device, GammaTblPtr table, VTMode *color);
long VT_DeviceModes(VTDevice *device);
long VT_DMCurrentMode(VTDevice *device, VTMode *mode);
long VT_DMSetMode(VTDevice *device, VTMode *mode);
static unsigned char VT_HasDisplayManager(void);
static long VT_DisplayManagerVersion(void);


#endif
