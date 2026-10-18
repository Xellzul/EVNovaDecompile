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
extern void DrawCommaNumber(long value);
extern void StrcatCommaNumber(long value, char *dst);
#pragma cplusplus on
extern "C++" {
extern unsigned char gRT3Table[];
}
#pragma cplusplus reset
#pragma cplusplus on
extern "C++" {
extern unsigned char shieldString[6][0x100];
extern unsigned char armorString[3][0x100];
extern unsigned char newbieString[10][0x100];
}
#pragma cplusplus reset
extern char LoadPluginString(short id, void *dst);
extern void AdjustHandleSize(Handle h, long newSize);
extern void PutSpriteInLayer(struct SpriteRec *sprite,
                             struct SpriteLayerRec *layer);
extern char EVBugResourceFieldTest(short field);
extern char ShipCanSelfRepair(ShipObj *ship);
struct RLEPixelData;
extern void FixupRLEPixelData(unsigned char *tokens, struct RLEPixelData *header);
extern void GetCharResourceIntroIDs(const unsigned char *name, short *a, short *b,
                         short *c);
extern void FixupResourceByType(Handle h, unsigned long type);
extern void *LinkedListNew(void);
extern short FindSystemFromStellar(short stellar);
extern short GetCharacterShipIndex(const unsigned char *name);
extern void GetDefaultCharacterName(unsigned char *name);
extern Handle GetNamedResource(unsigned long type, const unsigned char *name);
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
extern char ResourceExists(unsigned long type, short id);
extern char nv_ResourceExists(unsigned long type, short id);
extern ResRecord *nv_FindResourceRecord(unsigned long type, short id);
extern char ShipIsDerelict(ShipObj *ship);
extern short StellarNumHyperLinks(SpobRecord *spob);
extern void StripNameComment(unsigned char *name);
#pragma cplusplus on
extern "C++" {
extern Handle colorFileHand;
extern RGBColor escortMenuColor;
extern Handle gBoomHandle;
extern Handle gCronHandle;
extern unsigned char descMovieName[256];
extern short gDescFlags;
extern Handle gDescHandle;
extern short gDescRankGovt[2];
extern Handle gDudeHandle;
extern Handle gFletHandle;
extern Handle gGovtHandle;
extern Handle gJunkHandle;
extern Handle gNebuHandle;
extern Handle gOopsHandle;
extern Handle gOutfHandle;
extern unsigned char gOutfShortNames[512][256];
extern Handle gPersHandle;
extern unsigned long gRandSeed;
extern Handle gRankHandle;
extern long gResourcesReleased;
extern struct ListRec *gRezFiles;                /* the open RezFiles, BurgerLib's list */
extern Handle gRoidHandle;
extern Handle gShipHandle;
extern unsigned char gShipMovieFiles[768][256];
extern unsigned char gShipShortNames[768][256];
extern Handle gSpobHandle;
extern Handle gSystHandle;
extern Handle gWeapHandle;
extern short jumpSoundLength[2];
}
#pragma cplusplus reset
extern short pstrcmp(const unsigned char *a, const unsigned char *b);

extern void pstrcpy(unsigned char *dst, unsigned char *src);
extern void CullNameString(unsigned char *name);
extern short WarpSoundDuration(void);

extern long ResourceInit(RezFile *rez, const char *path, unsigned long start);
extern char *ResourceLoadByName(RezFile *rez, const char *name);
extern void *SaveBuffer(void);
struct _FILE;
extern short CountEVPlugins(const char *path, const char *ext, struct _FILE *log);
extern void ResourceDetach(RezFile *rez, long index);
extern Word ResourceGetIDFromHandle(RezFile *rez, Handle h, long *idFound,
                                    char *nameBuf, Word nameBufSize);
extern void ResourcePurgeCache(RezFile *rez);
extern void ResourceRelease(RezFile *rez, long index);
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
