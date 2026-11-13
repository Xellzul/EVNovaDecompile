/* gameutil.h -- strings, numbers, dates, randomness and resource plumbing. Notes: Research/notes/gameutil.md */
#ifndef GAMEUTIL_H
#define GAMEUTIL_H
#include "burger.h"
#include "gametypes.h"
#include "shipobj.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void CenterPict(void *pict, Rect *bounds);
extern void CheckLoMem(void);
extern void DateToLongString(DateTimeRec date, char *out);
extern void DateToMedString(DateTimeRec date, char *out);
extern void DrawCommaNumber(int32_t value);
extern void StrcatCommaNumber(int32_t value, char *dst);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern unsigned char gRT3Table[];
EXTERN_CPP_END
#pragma cplusplus reset
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern unsigned char shieldString[6][0x100];
extern unsigned char armorString[3][0x100];
extern unsigned char newbieString[10][0x100];
EXTERN_CPP_END
#pragma cplusplus reset
extern Boolean LoadPluginString(short id, void *dst);
extern void AdjustHandleSize(Handle h, int32_t newSize);
extern void PutSpriteInLayer(struct SpriteRec *sprite,
                             struct SpriteLayerRec *layer);
extern Boolean EVBugResourceFieldTest(short field);
extern Boolean ShipCanSelfRepair(ShipObj *ship);
struct RLEPixelData;
extern void FixupRLEPixelData(unsigned char *tokens, struct RLEPixelData *header);
extern void GetCharResourceIntroIDs(const unsigned char *name, short *a, short *b,
                         short *c);
extern void FixupResourceByType(Handle h, uint32_t type);
extern void *LinkedListNew(void);
extern short FindSystemFromStellar(short stellar);
extern short GetCharResourceShipType(const unsigned char *name);
extern void GetPlayerContributeBits(uint32_t *lo, uint32_t *hi);
extern void GetDefaultCharResourceName(unsigned char *name);
#ifndef MAC1010
extern Handle GetNamedResource(unsigned long type, const unsigned char *name);
#endif
extern void GetRandomIndString(unsigned char *str, short strListID);
extern void HideCursorSprite(void);
extern void LoadAdvice(void);
extern void LoadDescription(char *dst, short id);
extern void LoadObjectData(void);
extern void LoadStrings(void);
extern void ParseTextDescription(void);
extern void PlayQTMovieFromDesc(short id);
extern short PlayerHasFightersOfOutfitType(short outfitIndex);
extern short Rand(short range);
#ifdef MACBUILD
#ifndef IMPLICIT_RESOURCEEXISTS /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern Boolean ResourceExists(uint32_t type, short id);
#endif
#else
extern Boolean ResourceExists(uint32_t type, short id);
#endif
extern Boolean nv_ResourceExists(uint32_t type, short id);
extern ResRecord *nv_FindResourceRecord(uint32_t type, short id);
extern Boolean ShipIsDerelict(ShipObj *ship);
extern short StellarNumHyperLinks(SpobRecord *spob);
extern void TrimResourceName(unsigned char *name);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Handle colorFileHand;
extern RGBColor escortMenuColor;
extern Handle boomFileHand;
extern Handle cronFileHand;
extern unsigned char descMovieName[256];
extern short descFlags;
extern Handle textFileHand;
extern short descRankGovt[2];
extern Handle dudeFileHand;
extern Handle fleetFileHand;
extern Handle govtFileHand;
extern Handle junkFileHand;
extern Handle nebulaFileHand;
extern Handle disasterFileHand;
extern Handle outfitSystemFileHand;
extern unsigned char outfitShortNames[512][256];
extern Handle personFileHand;
extern uint32_t randomSeed;
extern Handle rankFileHand;
extern int32_t gResourcesReleased;
extern struct ListRec *gRezFiles;                /* the open RezFiles, BurgerLib's list */
extern Handle asteroidFileHand;
extern Handle shipClassFileHand;
extern unsigned char shipMovieFilenames[768][256];
extern unsigned char shipyardShortNames[768][256];
extern Handle stellarFileHand;
extern Handle systemFileHand;
extern Handle weaponFileHand;
extern short jumpSoundLength[2];
EXTERN_CPP_END
#pragma cplusplus reset
extern short pstrcmp(const unsigned char *a, const unsigned char *b);

extern void pstrcpy(unsigned char *dst, unsigned char *src);
extern void CullNameString(unsigned char *name);
extern short WarpSoundDuration(void);

extern int32_t ResourceInit(RezFile *rez, const char *path, uint32_t start);
extern char *ResourceLoadByName(RezFile *rez, const char *name);
extern void *SaveBuffer(void);
struct _FILE;
#ifdef MACBUILD
/* the plug-in scan's file type: any */
#define kAnyPluginType '????'
/* a plug-in file's type, 'Np' 0x95 'f' (MacRoman) */
#define kPlugInFileType 0x4e709566
#ifdef MAC1010
extern short CountEVPlugins(short refNum, ConstStr255Param folderName, OSType type, evn_FILE *log);
extern OSErr GetSubfolderOfFile(short refNum, ConstStr255Param name, int32_t *dirID, short *vRefNum);
extern OSErr GetPluginSubfolder(short refNum, ConstStr255Param name, int32_t *dirID, short *vRefNum);
extern short gARef;
#else
extern short CountEVPlugins(FSRef folder, OSType type);
#endif
#else
extern short CountEVPlugins(const char *path, const char *ext, evn_FILE *log);
#endif
extern void ResourceDetach(RezFile *rez, int32_t index);
extern Word ResourceGetIDFromHandle(RezFile *rez, Handle h, int32_t *idFound,
                                    char *nameBuf, Word nameBufSize);
extern void ResourcePurgeCache(RezFile *rez);
extern void ResourceRelease(RezFile *rez, int32_t index);
extern void MungeForSymbol(char *needle, void *replacement);
struct DudeRecord;
extern short SelectShipFieldFromDude(struct DudeRecord *dude,
                                     unsigned char allowAll);
extern short GenericRandomShipSpawn(short sys, short reserve);
extern short SpecificDudeSpawn(short dude, short sys, short reserve,
                               unsigned char allowAll);
extern void ShowPersonHailQuote(short persID);

#ifdef __cplusplus
}
#endif

#endif
