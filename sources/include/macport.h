/* macport.h -- Mac OS Toolbox types and shims for the Windows port. Notes: Research/notes/macport.md */
#include "gametypes.h"
#ifndef MACBUILD
#include "win32.h"
#endif
#include "burgertypes.h"

#ifndef MACPORT_H
#define MACPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MACBUILD
typedef unsigned char *StringPtr;
typedef uint32_t OSType;

/* QuickDraw basics (partial) */
typedef struct Point {
    short v;
    short h;
} Point;
#endif

#ifndef MACBUILD
#pragma options align=packed
typedef struct EventRecord {
    unsigned short what;
    int32_t message;
    int32_t when;
    Point where;
    short modifiers;
} EventRecord;
#pragma options align=reset
#endif

#ifndef MACBUILD
typedef struct FSSpec {
    short vRefNum;
    int32_t parID;
    Str255 name;
} FSSpec;

typedef struct CGrafPort *CWindowPtr;
#endif
typedef struct ControlRecord {
    int32_t kind;
    int32_t itemNo;
    int32_t value;
    int32_t minValue;
    int32_t maxValue;
    Rect box;
    void *data;
    DialogPtr owner;
    Boolean enabled;
} ControlRecord;
typedef struct ControlRecord *ControlPtr;
#ifndef MACBUILD
/* the Toolbox names the port's records answer to */
typedef ControlPtr ControlRef;
typedef Image_t *CIconHandle;
typedef Image_t *PixPatHandle;
typedef void *Movie;
typedef struct ComponentDescription {
    int32_t componentType;
    int32_t componentSubType;
    int32_t componentManufacturer;
    uint32_t componentFlags;
    uint32_t componentFlagsMask;
} ComponentDescription;

typedef struct PixMap {
    Rect bounds;
    int32_t pixelSize;
    void *baseAddr;
    unsigned char *ctab;
    int32_t originV;
    int32_t width;
    int32_t height;
    int32_t rowBytes;
} PixMap, *PixMapPtr;
typedef struct CGrafPort {
    Boolean offscreen;
    char filler1[3];
    PixMap pix;
    int32_t depth;
    void *hdc;
    void *bitmap;
    RGBColor foreColor;
    RGBColor backColor;
    Point pnLoc;
    Point pnSize;
    short txFont;
    short txFace;
    short txSize;
} CGrafPort, *CGrafPtr;
/* one port record serves every port, so these are Carbon's: a GrafPtr is a CGrafPtr */
typedef CGrafPtr GrafPtr;
typedef CWindowPtr WindowRef;
typedef struct BitMap *BitMapPtr;
#endif

typedef struct SrcBitMap {
    Rect bounds;
    uint32_t rowBytes;
    Image_t image;
    int32_t unk28[2];
} SrcBitMap;

typedef struct GWorldContainer {
    CGrafPtr world;
    Rect bounds;
#ifdef MACBUILD
    PixMapPtr pixMap;
#endif
} GWorldContainer;

#ifndef MACBUILD
extern void GetIndString(unsigned char *str, int strListID, int index);
#endif
extern void GetIniIndString(char *out, int32_t listID, int32_t index);
#ifdef MACBUILD
/* the mac draws a dialog straight to its window: nothing to push */
#define nv_UpdateDialogWindow(all) ((void)0)
#else
extern void nv_UpdateDialogWindow(int all);
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern GWorldContainer iconsPort;
#ifndef MACBUILD
EXTERN_CPP_END
#pragma cplusplus reset
extern void SelectDialogItemText(DialogPtr dlg, int32_t item, int32_t start, int32_t end);
extern void FrameRect(const Rect *r);
extern void GetFNum(const unsigned char *name, short *familyID);
#endif
extern void DrawTextRun(const void *text, uint32_t byteCount, Rect *box,
                        Boolean clipped, int height);
extern CGrafPtr CurrentGrafPort;
#ifndef MACBUILD
extern void CopyBits(BitMapPtr srcBits, BitMapPtr dstBits,
                     const Rect *srcRect, const Rect *dstRect,
                     int32_t mode, void *maskRgn);
extern BitMapPtr GetPortBitMapForCopyBits(CGrafPtr port);
extern void BlockMove(const void *src, void *dst, uint32_t len);
extern void CopyMask(BitMapPtr srcBits, BitMapPtr maskBits, BitMapPtr dstBits,
                     Rect *srcRect, Rect *maskRect, Rect *dstRect);
#endif
/* the engine's older spelling of a port's bits, (BitMap *)*GetPortPixMap; the port has one call */
#ifdef MACBUILD
#define GetPortPixMapBits(port) ((BitMapPtr)*GetPortPixMap(port))
#else
#define GetPortPixMapBits(port) GetPortBitMapForCopyBits(port)
#endif
/* a GWorldContainer's bits for CopyBits: the mac takes its GWorld's port, the win port the container */
#ifdef MACBUILD
#define GWorldContainerBits(gwc) GetPortBitMapForCopyBits((gwc)->world)
#else
#define GWorldContainerBits(gwc) ((BitMapPtr)GetGWCPixMap(gwc))
#endif
/* the same as a CopyBits source through the GWorld's pixmap (the status panels' spelling) */
#ifdef MACBUILD
#define GWorldContainerPixMap(gwc) ((BitMapPtr)*GetPortPixMap((gwc)->world))
#else
#define GWorldContainerPixMap(gwc) GWorldContainerBits(gwc)
#endif
/* drawing into a GWorldContainer: the mac sets its GWorld's port, the win port the container */
#ifdef MACBUILD
#define GWorldContainerSetPort(gwc) SetPort((GrafPtr)(gwc)->world)
#else
#define GWorldContainerSetPort(gwc) SetGWorldContainer(gwc)
#endif
/* a Point in hand: the mac passes it whole, the win port's PtInRect takes h and v */
#ifdef MACBUILD
#define PtInRectPoint(pt, r) (PtInRect)(pt, r)
#else
#define PtInRectPoint(pt, r) PtInRect((pt).h, (pt).v, r)
#endif
#ifndef MACBUILD
extern void *GetPortVisRgn(void *port);
extern short NewGWorld(CGrafPtr *out, int depth, const Rect *bounds,
                       void *theGDH);
extern PixMapPtr GetGWorldPixMap(CGrafPtr world);
extern PixMapPtr GetWindowPixMap(CWindowPtr window);
#endif
extern void QDGetCIconBounds(Image_t *icon, Rect *r);
extern PicHandle GetPictureNoGamma(int picID);

extern void CopyBitsNone(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsCopy8(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsOr8(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsCopy16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsCopy24(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsAddOver16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsAdMax16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsScale8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xStart, int32_t yStart);
extern void CopyBitsScale16Average(PixMapPtr srcBits, PixMapPtr dstBits, Rect *srcRect,
        Rect *dstRect, int32_t xStep, int32_t yStep, int32_t xFrac,
        int32_t yFrac);
extern void CopyBitsScale24(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
extern void CopyMaskNone(PixMapPtr src, PixMapPtr mask, PixMapPtr dst,
        Rect *srcRect, Rect *maskRect, Rect *dstRect);
extern void CopyMask16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
        Rect *srcRect, Rect *maskRect, Rect *dstRect);
extern void CopyMaskScale16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
        Rect *srcRect, Rect *maskRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);

#ifdef MACBUILD
/* EVCarbonHack's accessors, as sources/mac/port/evcarbonhack*.c define them */
extern RgnHandle GetPortVisRgn(CGrafPtr port);
extern PixMapHandle GetWindowPixMap(WindowRef window);
extern CGrafPtr GetCWindowGrafPort(WindowRef window);
extern Rect *GetWindowPortRect(WindowRef window);
extern Rect *GetCPortPortRect(CGrafPtr port);
extern Rect *GetCWindowPortRect(WindowRef window);
extern PixMapHandle GetCWindowPixMap(WindowRef window);
extern RgnHandle GetCWindowVisRgn(WindowRef window);
#else
extern CGrafPtr GetCWindowGrafPort(CWindowPtr window);
extern Rect *GetWindowPortRect(CWindowPtr window);
extern Rect *GetCWindowPortRect(CWindowPtr window);
extern PixMapPtr GetCWindowPixMap(CWindowPtr window);
#endif
#ifdef MACBUILD
extern PixMapHandle GetGWCPixMap(GWorldContainer *gwc);
#else
extern PixMapPtr GetGWCPixMap(GWorldContainer *gwc);
#endif

#if defined(MAC1010) && !TARGET_API_MAC_CARBON /* SpriteWorld's records: 68k alignment on 1.0.10 Classic */
#pragma options align=mac68k
#endif
typedef struct SWFrameRec {
    CGrafPtr framePort;
    void *frameDevice;
#ifdef MACBUILD
    PixMapHandle framePixHndl;
#else
    PixMapPtr framePixHndl;
#endif
    PixMapPtr framePix;
    char *frameBaseAddr;
    uint32_t frameRowBytes;
    short leftAlignFactor;
    short rightAlignFactor;
    Boolean isFrameLocked;
    Boolean isWindowFrame;
    Boolean interlacingIsOn;
    Boolean skipOddLines;
    short frameDepth;
    Rect frameRect;
    short hotSpotH;
    short hotSpotV;
    Rect collisionInset;
    Point offsetPoint;
    void *maskRgn;
    CGrafPtr maskPort;
    void *maskDevice;
#ifdef MACBUILD
    PixMapHandle maskPixHndl;
#else
    PixMapPtr maskPixHndl;
#endif
    PixMapPtr maskPix;
    char *maskBaseAddr;
    Boolean tileMaskIsSolid;
    Boolean sharesGWorld;
    Boolean usesCollisionInset;
    Boolean framePad57;
    unsigned short useCount;
    short worldRectOffset;
    unsigned short numScanLines;
    unsigned short pixelShift;
    uint32_t *scanLinePtrArray;
    void *rleDataH;
    void *rleTokenStart;
    int32_t userData;
} SWFrameRec;

typedef SWFrameRec *FramePtr;

typedef void (*DrawProcPtr)(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                            Rect *srcRect, Rect *dstRect);
struct SpriteWorldRec;
typedef void (*WorldProcPtr)(struct SpriteWorldRec *spriteWorldP);

typedef struct SpriteWorldRec {
    struct SpriteLayerRec *headSpriteLayerP;
    struct SpriteLayerRec *tailSpriteLayerP;
    struct SpriteLayerRec *deadSpriteLayerP;
    struct UpdateRectStruct *headUpdateRectP;
    SWFrameRec *extraBackFrameP;
    SWFrameRec *backFrameP;
    SWFrameRec *workFrameP;
    SWFrameRec *windowFrameP;
    DrawProcPtr offscreenDrawProc;
    DrawProcPtr screenDrawProc;
    void (*doubleRectDrawProc)(SWFrameRec *, SWFrameRec *, Rect *, Rect *, Rect *, Rect *);
    WorldProcPtr postEraseCallBack;
    WorldProcPtr postDrawCallBack;
    Rect windRect;
    Rect backRect;
    Rect originalWindRect;
    Rect originalBackRect;
    Rect visScrollRect;
    Rect oldVisScrollRect;
    Rect offscreenScrollRect;
    short horizScrollRectOffset;
    short vertScrollRectOffset;
    short horizScrollDelta;
    short vertScrollDelta;
    Rect scrollRectMoveBounds;
    void (*worldMoveProc)(struct SpriteWorldRec *, struct SpriteRec *);
    struct SpriteRec *followSpriteP;
    struct TileMapStruct **tileLayerArray;
    short lastActiveTileLayer;
    Boolean tilingIsInitialized;
    Boolean tilingIsOn;
    short **tilingCache;
    short numTilingCacheRows;
    short numTilingCacheCols;
    SWFrameRec **tileFrameArray;
    short *curTileImage;
    short maxNumTiles;
    short tileWidth;
    short tileHeight;
    int32_t numTilesChanged;
    Rect *changedTiles;
    int32_t changedTilesArraySize;
    WorldProcPtr tileChangeProc;
    void (*tileRectDrawProc)(struct SpriteWorldRec *, Rect *, Boolean);
    DrawProcPtr tileMaskDrawProc;
    DrawProcPtr partialMaskDrawProc;
    void *mainSWGDH;
    short pixelDepth;
    short frameInterval;
    uint32_t runningTimeCount;
#ifdef MACBUILD
    UnsignedWide lastMicroseconds;
#else
    unsigned long long lastMicroseconds;
#endif
    uint32_t timeOfLastFrame;
    Boolean frameHasOccurred;
    Boolean thereAreNonScrollingLayers;
    Boolean pad1;
    RGBColor backgroundColor;
    int32_t backgroundValue;
    Boolean fastBlit;
#ifdef MACBUILD
    RgnHandle windowRgn;
    char swFillerF0[0xf4 - 0xf0];
#else
    char swFillerED[0xf8 - 0xed];
#endif
} SpriteWorldRec, *SpriteWorldPtr;

extern void SWSetPortToWindow(SpriteWorldPtr spriteWorld);

typedef struct SpriteLayerRec {
    struct SpriteLayerRec *nextSpriteLayerP;
    struct SpriteLayerRec *prevSpriteLayerP;
    struct SpriteRec *headSpriteP;
    struct SpriteRec *tailSpriteP;
    void (*layerCallBack)(struct SpriteWorldRec *);
    int32_t userData;
    short tileLayer;
    Boolean isPaused;
    Boolean layerIsNonScrolling;
} SpriteLayerRec, *SpriteLayerPtr;
typedef struct RLEPixelData {
    unsigned short width;
    unsigned short height;
    unsigned short depth;
    unsigned short palette;
    unsigned short frames;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned short reserved3;
} RLEPixelData, *RLEPixelDataPtr, **RLEPixelDataHdl;
#define kRLEResourceHeaderSize  sizeof(RLEPixelData)

#ifndef MACBUILD
#define noErr           0
#define memFullErr      (-108)
#define resNotFound     (-192)
#define srcCopy         0
#define addOver         34              /* QuickDraw arithmetic transfer modes */
#define adMax           37
#define kControlInactivePart 255
#define noNewDevice     2               /* QuickTime GWorldFlags */
#define k16BE555PixelFormat 0x00000010  /* QuickTime pixel formats */
#define k16LE555PixelFormat 0x4c353535  /* 'L555' */
#endif

#define SW_ROWBYTES(frame, rows) \
        ((frame)->scanLinePtrArray[(rows) - (frame)->frameRect.top])
#define SW_PIXELBYTES(frame, pixels)    ((pixels) << (frame)->pixelShift)
#define SW_SPLIT15(c) ( (((c) & 0x3e0) << 15) | ((c) & 0x7c1f) )

#define ABS(x) (((x)<0) ? (-(x)) : (x))
#define SIGN(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define SW_PIXELPTR(frame, x, y) \
    ((frame)->frameBaseAddr + SW_ROWBYTES(frame, y) + SW_PIXELBYTES(frame, x))
#define SW_PIXEL16AT(frame, xBytes, y)  \
    (*(unsigned short *)( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + (xBytes) ))

#define SW_PIXELPTR8(frame, x, y)   \
    ((unsigned char  *) ( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + (x) ))
#define SW_PIXELPTR16(frame, x, y)  \
    ((unsigned short *) ( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + ((x)+(x)) ))
#define SW_PIXELPTR32(frame, x, y)  \
    ((uint32_t  *) ( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + ((x) << 2) ))

/* DEPTH is the switch variable FOREACH_DEPTH_DO binds. */
#define FOREACH_DEPTH_DO(depth, instructions)   \
    switch ( depth )    \
    {   case  8:    { int DEPTH=8;  instructions } break;   \
        case 16:    { int DEPTH=16; instructions } break;   \
        case 32:    { int DEPTH=32; instructions } break; }

#define GET_DEPTH_PIXEL(frame, x, y)            \
    ( DEPTH==8  ?   *SW_PIXELPTR8(frame,x,y) :  \
    ( DEPTH==16 ?   *SW_PIXELPTR16(frame,x,y) : \
                    *SW_PIXELPTR32(frame,x,y) ))

#define SET_DEPTH_PIXEL(frame, x, y, c)         \
      if(DEPTH==8)  *SW_PIXELPTR8(frame,x,y)=c;  else   \
    { if(DEPTH==16) *SW_PIXELPTR16(frame,x,y)=c; else   \
                    *SW_PIXELPTR32(frame,x,y)=c; }

#define BP_CLIP_RECT(frameRectP, srcRect, dstRect) \
    if (dstRect.top < frameRectP->top) \
    { \
        srcRect.top += frameRectP->top - dstRect.top; \
        dstRect.top =  frameRectP->top; \
    } \
    if (dstRect.bottom > frameRectP->bottom) \
    { \
        srcRect.bottom -= dstRect.bottom - frameRectP->bottom; \
        dstRect.bottom = frameRectP->bottom; \
    } \
    if (dstRect.bottom <= dstRect.top) \
        return; \
    if (dstRect.left < frameRectP->left) \
    { \
        srcRect.left += frameRectP->left - dstRect.left; \
        dstRect.left = frameRectP->left; \
    } \
    if (dstRect.right > frameRectP->right) \
    { \
        srcRect.right -= dstRect.right - frameRectP->right; \
        dstRect.right = frameRectP->right; \
    } \
    if (dstRect.right <= dstRect.left) \
        return;

#define SW_MIN(a, b)            ((a) < (b) ? (a) : (b))
#define SW_MAX(a, b)            ((a) > (b) ? (a) : (b))

/* a sprite frame's RLE stream: a token word carries a kTokenShift-shifted opcode and a byte count */
#define kEndShapeToken          0
#define kLineStartToken         1
#define kDrawPixelsToken        2
#define kSkipPixelsToken        3

typedef struct SpriteRec SpriteRec, *SpritePtr;

typedef void (*FrameProcPtr)(SpritePtr srcSpriteP, SWFrameRec *curFrameP,
                             int32_t *frameIndex);
typedef void (*MoveProcPtr)(SpritePtr srcSpriteP);
typedef void (*CollideProcPtr)(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                               Rect *sectRect);

typedef enum RemovalType {
    kSWDontRemoveSprite = 0,
    kSWRemoveSprite,
    kSWRemoveAndDisposeSprite
} RemovalType;

typedef enum AdvanceType {
    kSWWrapAroundMode = 0,
    kSWPatrollingMode
} AdvanceType;

struct SpriteRec {
    SpriteLayerRec *parentSpriteLayerP;
    SpritePtr nextSpriteP;
    SpritePtr prevSpriteP;
    SpritePtr nextActiveSpriteP;
    SpritePtr nextIdleSpriteP;
    unsigned char isVisible;
    Boolean needsToBeDrawn;
    Boolean needsToBeErased;
    Boolean onTop;
    Boolean justBecameVisible;
    char pad1b;
    Rect destFrameRect;
    Rect oldFrameRect;
    Rect deltaFrameRect;
    DrawProcPtr frameDrawProc;
    short tileDepth;
    Boolean destRectIsVisible;
    Boolean oldRectIsVisible;
    Rect clippedSourceRect;
    Rect destOffscreenRect;
    SWFrameRec **frameArray;
    SWFrameRec *curFrameP;
    int32_t numFrames;
    int32_t maxFrames;
    int32_t frameTimeInterval;
    uint32_t timeOfLastFrameChange;
    int32_t frameAdvance;
    int32_t curFrameIndex;
    int32_t firstFrameIndex;
    int32_t lastFrameIndex;
    FrameProcPtr frameChangeProc;
    int32_t moveTimeInterval;
    uint32_t timeOfLastMove;
    short horizMoveDelta;
    short vertMoveDelta;
    Rect moveBoundsRect;
    MoveProcPtr spriteMoveProc;
    MoveProcPtr realSpriteMoveProc;
    CollideProcPtr spriteCollideProc;
    CGrafPtr sharedPictGWorld;
    CGrafPtr sharedMaskGWorld;
    RemovalType spriteRemoval;
    AdvanceType frameAdvanceMode;
    short translucency;
    short tintRed;
    short tintGreen;
    short tintBlue;
    short tintMode;
    uint32_t tintColor;
    uint32_t rotation;
    short scaledWidth;
    short scaledHeight;
    uint32_t colorValue;
    RGBColor colorRGB;
    Boolean flippedHorizontal;
    Boolean flippedVertical;
    short pad2;
    int32_t userData;
    char **rleDataH;
    Boolean collisionsDisabled;
    Boolean ownsRLEData;
    Boolean isEmptySprite;
    Boolean srD3;
};
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
#pragma options align=reset
#endif

#ifndef MACBUILD
typedef struct TERec {
    char                  *text;
    int32_t                   textLength;
    short                  txFont;
    short                  txSize;
    Rect                   destRect;
    Rect                   viewRect;
    int32_t                   height;
    CGrafPtr               inPort;
} TERec, **TEHandle;
#endif

#ifdef MACBUILD
#define TESetLineMetrics(te)     do {         FontInfo fi_;         GetFontInfo(&fi_);         (*(te))->fontAscent = fi_.ascent;         (*(te))->lineHeight = fi_.ascent + fi_.descent + fi_.leading;     } while (0)
#define TEMeasureText(te, width) TETextHeight(te)
#define TETextHeight(te) TEGetHeight((*(te))->nLines, 1, (te))
#define TEWrappedHeight(te) (((*(te))->nLines + 2) * (*(te))->lineHeight)
#else
#define TESetLineMetrics(te) ((void)0)
#define TEMeasureText(te, width)     MeasureWrappedText((*(te))->text, (*(te))->textLength, (width))
#define TETextHeight(te) ((*(te))->height)
#define TEWrappedHeight(te) ((*(te))->height)
#endif

struct ScrollRec;
typedef void (*ListDrawProcPtr)(short lMessage, unsigned char lSelect, Rect *lRect,
                                Point *lCell, short lDataOffset, short lDataLen,
                                struct ScrollRec **lHandle);
#ifdef MACBUILD
typedef struct ListRec ScrollRec;
#else
typedef struct ScrollRec {
    CGrafPtr port;
    Rect rView;
    Rect dataBounds;
    Point indent;
    Point cellSize;
    ListDrawProcPtr drawProc;
    Boolean hasScrollBar;
    int32_t scroll;
    char **cells;
    short *cellArray;
    short maxIndex;
    Point lastClick;
    int32_t clikTime;
    char unk3c[4];
    Boolean drawIt;
} ScrollRec;
#endif

typedef struct DialogRecord {
    CGrafPtr port;
    Rect gwBounds;
    Rect bounds;
    Rect portRect;
    int32_t itemCount;
    int32_t editItem;
    int32_t defaultItem;
    int32_t cancelItem;
    void *items;
    void *updateProc;
    void *eventProc;
    int32_t needsUpdate;
    int32_t visible;
} DialogRecord;

#if defined(MAC1010) && !TARGET_API_MAC_CARBON /* as SpriteWorld's records */
#pragma options align=mac68k
#endif
typedef struct Particle {
    short lifeRemaining;
    int32_t horizLoc;
    int32_t vertLoc;
    int32_t oldHorizLoc;
    int32_t oldVertLoc;
    int32_t horizSpeed;
    int32_t vertSpeed;
    int32_t drawnHorizLoc;
    int32_t drawnVertLoc;
    int32_t shade;
    uint32_t color;
} Particle, *ParticlePtr;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
#pragma options align=reset
#endif

/* Toolbox GetTime shim — port implementation at 0x004bbb10 */
#ifndef MACBUILD
extern void GetTime(DateTimeRec *dtr);
#endif
extern void IncrementDate(DateTimeRec *date, int32_t daysToAdd);

#ifdef V111
extern void PtoCstr(unsigned char *str);
extern void CtoPstr(char *str);
#else
extern char *PtoCstr(unsigned char *str);
#if defined(MAC1010) && TARGET_API_MAC_CARBON
extern void CtoPstr(char *str);
#else
extern unsigned char *CtoPstr(char *str);
#endif
#endif

#define PORT_PIXMAP(port) ((PixMapPtr)((char *)(port) + 4))
#define kAutoKey       5
#define kBlue555  0x001f
#define kCharCodeMask  0xff
#define kDIBHeaderSize 0x28
#define kDepth16            0x10
#define kEscapeChar    0x1b
#define kGreen555 0x03e0
#define kKeyDown       3
#define kMouseDown     1
#define kNullEvent   0
#define kPictType 0x50494354   /* 'PICT' */
#define kSoundType 0x736e6420  /* 'snd ' */
#define kStringType 0x53545220 /* 'STR ' */
#define kStringListType 0x53545223UL /* 'STR#' */
#define kRed555   0x7c00
#define kReturnChar    0x0d
#define kTabChar       0x09
#ifdef MAC1010 /* the 1.0.10 filters take Enter with Return */
#define IsReturnChar(c) ((c) == kReturnChar || (c) == kEnterChar)
#else
#define IsReturnChar(c) ((c) == kReturnChar)
#endif
#ifdef MACBUILD
/* the Toolbox character codes; the port gives the win virtual keys codes of its own */
#define kDownArrowChar  0x1f
#define kEndChar       0x04
#define kEnterChar     0x03
#define kHomeChar      0x01
#define kLeftArrowChar  0x1c
#define kPageDownChar   0x0c
#define kPageUpChar     0x0b
#define kRightArrowChar 0x1d
#define kUpArrowChar    0x1e
#else
#define kDownArrowChar  0x0a
#define kEndChar       0x93
#define kEnterChar     0x03
#define kHomeChar      0x91
#define kLeftArrowChar  0x96
#define kPageDownChar   0x94
#define kPageUpChar     0x92
#define kRightArrowChar 0x15
#define kUpArrowChar    0x0b
#endif
#define kUpdateEvt     6

#define kScanLShift 0x2a
#define kScanRShift 0x36
#define kScanLCtrl  0x1d
#define kScanRCtrl  0x6b
#define kScanLAlt   0x38
#define kScanRAlt   0x6f

#if V111
/* virtual key codes: the 1.1.1 dialog filters read the key, not the character (mac 1.0.10's
   compare character codes) */
#define kKeyCodeMask      0xff00
#define kAKeyCode         0x00
#define kLKeyCode         0x25
#define kReturnKeyCode    0x24
#define kTabKeyCode       0x30
#define kEscapeKeyCode    0x35
#define kEnterKeyCode     0x4c
#define kDownArrowKeyCode 0x7d
#define kUpArrowKeyCode   0x7e
#define kHKeyCode         0x04
#define kGKeyCode         0x05
#define kWKeyCode         0x0d
#define kEKeyCode         0x0e
#define kRKeyCode         0x0f
#define kNKeyCode         0x2d
#define kSKeyCode         0x01
#define kCKeyCode         0x08
#define kUKeyCode         0x20
#define kDKeyCode         0x02
#define kBKeyCode         0x0b
#define kTKeyCode         0x11
#define kOKeyCode         0x1f
#define kMKeyCode         0x2e
#define kFKeyCode         0x03
#define kSpaceKeyCode     0x31
#define kHomeKeyCode      0x73
#define kPageUpKeyCode    0x74
#define kEndKeyCode       0x77
#define kPageDownKeyCode  0x79
#define kBackslashKeyCode 0x2a
#define kMinusKeyCode     0x1b
#define kEqualKeyCode     0x18
#define kPadMinusKeyCode  0x4e
#define kPadPlusKeyCode   0x45
#define kLeftArrowKeyCode 0x7b
#define kRightArrowKeyCode 0x7c
#endif
#ifdef MAC1010
#define kEscapeKeyCode    MacKey(0x35)
#endif

#ifdef MACBUILD
#define kScanCapsLock MacKey(0x39)
#define kScanOption MacKey(0x3a)
#define kScanControl MacKey(0x3b)
#define CapsLockIsOn() QuickKeyCheck(kScanCapsLock)
#else
#define CapsLockIsOn() (GetKeyState(VK_CAPITAL) & 1)
#endif

#ifndef MACBUILD
#define shiftKey    0x0200
#define optionKey   0x0800
#define controlKey  0x1000
#define diskMask    0x0080
#define everyEvent  0xffff
#endif

/* a byte pointer stepped through a cast lvalue: gcc then keeps the pointer in its stack slot, which 1.1.1's bytes witness */
#define PtrAddBytes(p, n) ((char *)(p) += (n))

/* a registration key's check. `(k) << 4 >> 4` in one expression is an internal error in every MWCPPC held, so 1.0.10 shifts in statements, the expected value too. A revoked key has no top nibble, so want's is K's */
#ifdef MAC1010
#define KeyMismatch(out, local, read, K, Y) { \
    unsigned long long want; \
    unsigned long long got; \
    unsigned long long k; \
    want = (((Y) ^ (K)) & 0x0FFFFFFFFFFFFFFFULL) ^ (K); \
    k = (read); \
    got = k ^ (K); \
    got <<= 4; \
    got >>= 4; \
    want <<= 4; \
    want >>= 4; \
    out = want != got; \
}
#else
#define KeyLow60Bits(k) ((k) << 4 >> 4)
#define KeyMismatch(out, local, read, K, Y) out = KeyLow60Bits(local ^ K) != Y
#endif

/* a mac virtual key code in the engine's numbering of the KeyMap: 1.0.10 tests it 16 bits at a time, 1.1.1 a byte at a time */
#ifdef MAC1010
#define MacKey(k) ((k) ^ 8)
#else
#define MacKey(k) (k)
#endif

/* the key checks' result bytes: tested unsigned in 1.0.10, plain char where 1.1.1's gcc shows the sign */
#ifdef MAC1010
typedef Boolean KeyCheckFlag;
#else
typedef char KeyCheckFlag;
#endif

/* the character in a key event: 1.0.10's filters hold it in a char */
#ifdef MAC1010
typedef char EventChar;
#define EventCharOf(ev) ((char)((ev)->message & kCharCodeMask))
#else
typedef int EventChar;
#define EventCharOf(ev) ((ev)->message & kCharCodeMask)
#endif

/* 1.1.1's filters hold that character in a short (EventKey) or a char (EventKeyByte), and some keep the
   event's what in a short (EventWhat) */
#if V111
#define EventKey short
#define EventKeyByte char
#define EventWhat short
#else
#define EventKey EventChar
#define EventKeyByte EventChar
#define EventWhat int
#endif

#define kRLE8ResType 0x726c9138UL
#define kRLEDResType 0x726c9144UL

extern void AcquireScreenDC(void);
#ifndef MACBUILD
extern short Alert(short alertID, void *filterProc);
#define StopAlert(id, filter) Alert(id, filter)
extern void BackColor(const RGBColor *color);
extern Boolean Button(void);
#endif
extern void ClosePrefs(void);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Rect screenMinusDockRect;
extern int32_t paintBlackFlag;
EXTERN_CPP_END
#pragma cplusplus reset
extern void BusyIdle(void);
#ifndef MACBUILD
extern void Delay(uint32_t numTicks, uint32_t *finalTicks);
extern void DisposeCIcon(Image_t *icon);
#endif
extern void DrawCString(const char *text);
#ifdef MACBUILD
extern short DrawPicHandle(PicHandle pic, Rect *dstRect);
#else
extern void DrawPicHandle(PicHandle pic, Rect *dstRect);
#endif
#ifndef MACBUILD
extern void DrawString(const unsigned char *str);
extern void DrawText(const void *textBuf, int firstByte, int byteCount);
extern void EraseRect(Rect *r);
extern void ExitToShell(void);
#endif
extern void SetupMonitor(void);
#ifndef MACBUILD
extern void DisposePixPat(Image_t *pat);
#endif
extern void InitDataFiles(void);
#ifdef MACBUILD
#ifdef MAC1010
extern void MakeAlias(const FSSpec *target);
#else
extern void MakeAlias(const FSRef *target);
#endif
extern short EVOpenResourceFile(const FSRef *ref, SInt8 permission);
#else
extern void MakeAlias(char *name);
extern short EVOpenResourceFile(void *path);
#endif
extern void SwapEndianByDescriptor(unsigned char *data, unsigned short *desc);
extern void LoadMenuListItem(ControlRecord *rec, int32_t menuID, unsigned char *promptTitle,
                         int32_t resType);
extern void LoadControlResource(ControlRecord *rec, uint32_t resID);
extern int32_t PopUpListSelect(DialogPtr dlg, ControlRecord *rec);
extern void EditTextKey(ControlRecord *rec, int32_t key, int32_t mods);
void DrawBevel(Rect *r, const RGBColor *fill, const RGBColor *light, const RGBColor *dark);
#ifndef MACBUILD
extern void LUpdate(void *list);
#define ListUpdate(rgn, list) LUpdate(list)
#else
#define ListUpdate(rgn, list) LUpdate(rgn, list)
#endif
extern int32_t __stdcall KeyboardHookProc(int code, uint32_t wParam, int32_t lParam);
extern int32_t __stdcall WinGameWndProc(void *hwnd, uint32_t msg,
                         uint32_t wParam, int32_t lParam);
extern void ShutdownWinGame(void);
extern void FillRectWithColor(Rect *r, RGBColor *color);
extern short FindFirstCoLocatedSystem(short sys);
extern short FindPriorLoadedShipSprite(short value, short count, short field);
extern void DisposeClonedSprite(SpritePtr *p);
extern void ShowWindowNoFlash(void *arg);
extern void DisposeDialogNoFlash(void *dlg);
#ifndef MACBUILD
extern void FlushEvents(unsigned short whichMask, unsigned short stopMask);
extern void FrameOval(Rect *r);
extern CWindowPtr FrontWindow(void *window);
extern Image_t *GetCIcon(short iconID);
extern void *GetCWindowVisRgn(void *window);
#endif
extern Rect *GetDialogPortRect(DialogPtr dlg);
extern void *GetFont(short fontID, short size, short style);
#ifndef MACBUILD
extern void GetGlobalMouse(Point *pt);
extern void GetMouse(Point *pt);
extern DialogPtr GetNewDialog(short dlogID, void *updateProc,
                              void *eventProc);
extern PicHandle GetPicture(int picID);
extern Image_t *GetPixPat(short id);
extern void GetPort(CGrafPtr *port);
#endif
#ifndef MAC1010
extern void InitFonts(void);
#endif
extern short InitializeQTML(int32_t flags);
#ifndef MACBUILD
extern void *FindNextComponent(void *prev, ComponentDescription *cd);
extern void InsetRect(Rect *r, int dh, int dv);
extern void InvertRect(Rect *r);
#endif
extern unsigned char KeyCheck(short scanCode);
extern int32_t GetKeyModifiers(void);
extern void KeyboardScanCodeToString(char *buf, uint32_t size, uint32_t code);
extern unsigned int KeyIsDown(unsigned int scanCode);
extern int32_t KeyboardGet2(void);
#ifndef MACBUILD
extern void Line(short dh, short dv);
extern void LineTo(short h, short v);
#endif
extern void *ST_LoadSndResource(short id);
extern short MeasureCString(const char *text);
#ifndef MACBUILD
extern void MoveTo(short h, short v);
extern void OffsetRect(Rect *r, int dh, int dv);
#endif
extern void OpenDataFiles(void);
extern void OpenPrefs(void);
extern void PStr2CStr(char *dst, const unsigned char *src);
#ifndef MACBUILD
extern void PaintOval(Rect *r);
#endif
extern void PaletteSetPtr(unsigned int start, unsigned int count, const unsigned char *pal);
#ifndef MACBUILD
extern void PenSize(short width, short height);
extern void CloseMovieFile(short resRefNum);
#endif
#if defined(__MWERKS__) && !defined(MACBUILD) && (!defined(__cplusplus) || !defined(_MSL_CSTDLIB))
#if __MWERKS__ >= 0x3000
__declspec(noreturn) extern void exit(int status);
#else
extern void exit(int status);   /* mwcc 2.4, AmbrosiaTools' compiler, has no noreturn */
#endif
#endif
#if defined(__MWERKS__) && !defined(MACBUILD) && (!defined(__cplusplus) || !defined(_MSL_CSTRING))
extern char *strrchr(const char *s, int c);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern char *strcat(char *dst, const char *src);
extern int strcmp(const char *a, const char *b);
#endif
#if defined(__MWERKS__) && !defined(MACBUILD) && (!defined(__cplusplus) || !defined(_MSL_CSTDIO))
extern int sprintf(char *dst, const char *fmt, ...);
#endif
#if defined(__MWERKS__) && !defined(MACBUILD) && (!defined(__cplusplus) || !defined(_MSL_CCTYPE))
extern int isprint(int c);
extern int isalpha(int c);
extern int isdigit(int c);
extern int tolower(int c);
extern int toupper(int c);
#endif
#ifndef MACBUILD
extern int PtInRect(int h, int v, Rect *r);
#endif
extern void QDReleaseResource(PicHandle pic);
#ifndef MACBUILD
extern void RGBForeColor(const RGBColor *color);
extern void ForeColor(int32_t color);
extern void *NewPtr(int32_t size);
extern void DisposePtr(void *ptr);
extern uint32_t TickCount(void);
extern CGrafPtr GetWindowPort(CWindowPtr window);
extern void QDFlushPortBuffer(CGrafPtr port, void *region);
extern void PtToAngle(const Rect *r, Point pt, short *angle);
extern Boolean RectInRgn(Rect *r, void *rgn);
#endif
extern void RedrawDialog(DialogPtr dlg);
extern void RedrawDialogs(int a);
extern void ReleaseScreenDC(void);
extern void ResetScreenRect(void);
extern void ScreenEndScene(void);
#ifndef MACBUILD
extern int SectRect(const Rect *a, const Rect *b, Rect *dst);
extern uint32_t Color2Index(RGBColor *color);
extern void SetDialogDefaultItem(DialogPtr dlg, int32_t value);
extern short Random(void);
#endif
extern short SWSetSpriteWorldScreenDrawProc(SpriteWorldRec *spriteWorld,
                                            DrawProcPtr proc);
#ifndef MACBUILD
extern void SetMovieBox(Movie movie, Rect *box);
extern void DisposeMovie(Movie movie);
extern void GetMovieBox(Movie movie, Rect *box);
extern int32_t GetMovieDuration(Movie movie);
extern void GoToBeginningOfMovie(Movie movie);
extern Boolean IsMovieDone(Movie movie);
extern void LoadMovieIntoRam(Movie movie, int32_t time, int32_t duration, int32_t flags);
extern void MoviesTask(Movie movie, int32_t maxMs);
extern void Microseconds(UnsignedWide *now);
extern void StringToNum(const unsigned char *str, int32_t *value);
extern void *GetMovieTimeBase(Movie movie);
extern void SetMovieRate(Movie movie, int32_t rate);
extern void SetMoviePlayHints(Movie movie, int32_t a2, int32_t a3);
extern void *GetMovieIndTrackType(Movie movie, int32_t a, int32_t type, int32_t b);
extern void SetMovieGWorld(Movie movie, void *gworld, int32_t a3);
extern void StartMovie(Movie movie);
extern void StopMovie(Movie movie);
extern void SetPort(CGrafPtr port);
extern void SetPortWindowPort(CWindowPtr window);
extern void SetRect(Rect *r, int left, int top, int right, int bottom);
#else
#define SetPortWindowPort(window) SetPort((GrafPtr)GetCWindowGrafPort(window))
#endif
extern void FatalStartupError(short index);
extern void *ST_LoadSndResourceIndirect(void **handle, int32_t flags);
#ifndef MACBUILD
extern Boolean StillDown(void);
#endif
extern void StrcatOrdinalNumber(short n, char *dst, Boolean capitalize);
#ifndef MACBUILD
extern short StringWidth(const unsigned char *str);
extern void SysBeep(short duration);
#endif
extern void TerminateQTML(void);
#ifndef MACBUILD
extern void TETextBox(const void *text, int32_t length, Rect *box, short just);
extern short FSMakeFSSpec(int32_t vRefNum, int32_t dirID, const void *name, void *spec);
extern short OpenMovieFile(const void *spec, short *refNum, int32_t mode);
extern int32_t NewMovieFromFile(Movie *movie, short refNum, short *resId,
                             int32_t a4, int32_t a5, int32_t a6);
#endif
extern int MeasureWrappedText(const char *text, int byteCount, int width);
#ifndef MACBUILD
extern void NumToString(int32_t value, unsigned char *str);
extern void QDGetPictureBounds(PicHandle pic, Rect *r);
extern void HideWindow(CWindowPtr window);
extern void *GetGWorldDevice(CGrafPtr port);
extern void DisposeGWorld(CGrafPtr port);
extern void DebugStr(const char *message);
extern short MemError(void);
extern void FillCRect(Rect *dstRect, Image_t *pat);
#endif
extern short GetFrameBiggestSize(SWFrameRec *frame);
#ifndef MACBUILD
extern void GetMenuItemText(ControlPtr ctl, int index, unsigned char *text);
#endif
extern void SWSetPostEraseCallBack(SpriteWorldPtr world, WorldProcPtr proc);
extern void SWSetPostDrawCallBack(SpriteWorldPtr world, WorldProcPtr proc);
#ifndef MACBUILD
extern void DisposeWindow(CWindowPtr window);
extern void *GetGDevice(void);
#endif
extern void BuildGammaRamp(float gamma);
extern int32_t ReplaceInHandle(char **h, int32_t offset, const char *find, int32_t findLen,
                         const char *repl, int32_t replLen);
extern void GetListCellRect(Rect *box, Point cell, ScrollRec **h);
#ifndef MACBUILD
extern Boolean LClick(Point where, short modifiers, ScrollRec **h);
extern void ParamText(const unsigned char *p0, const unsigned char *p1,
                      const unsigned char *p2, const unsigned char *p3);
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern char gParamText0[];
extern char gParamText1[];
extern char gParamText2[];
extern char gParamText3[];
EXTERN_CPP_END
#pragma cplusplus reset
extern void FadeGammaTo(uint32_t percent, uint32_t duration);
extern unsigned char SplitTwoLineString(const unsigned char *src, unsigned char *line1,
                                        unsigned char *line2);
#ifndef MACBUILD
extern void TextFace(short face);
extern void TextFont(short font);
extern void TextSize(short size);
extern short TextWidth(const char *text, int byteCount);
#endif
extern void UpdateAndPageFlip(void);
#ifndef MACBUILD
extern RGBColor *blackColor;
#endif
extern int32_t gCaretPhase;
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern int32_t gBlinkTick;
extern SWFrameRec *floatingEscortMenuFrame;
extern SWFrameRec *floatingMapFrame;
extern short gChicagoFont;
extern void *gClickDialog;
extern void *gClickItem;
extern uint32_t gClickTime;
extern int32_t gDialogCount;
extern DialogPtr gCurDialog;
extern unsigned char gGammaRamp[32];
extern void *gKeyboardHook;
extern int32_t gLastModifiers;
extern void *gMacRomanToCP1252;
extern SpriteWorldPtr gSpriteWorld;

extern SpriteRec *masterShipSprite[768];
extern SpriteRec *masterShipGlowSprite[768];
extern SpriteRec *masterShipShieldSprite[768];
extern SpriteRec *masterShipLightSprite[768];
extern SpriteRec *masterShipWeaponSprite[768];
extern SpriteRec *masterShipAltSprite[768];
#ifndef MACBUILD
extern CGrafPort gGamePort;
#endif
extern RGBColor blackRGB;
extern int32_t gRezOpenPending;
extern int32_t gUseKeyboardHook;
EXTERN_CPP_END
#pragma cplusplus reset
extern void nv_BlitImageOr(Rect *dstRect, Image_t *icon);
extern int gDisableUpdatesMode;
#ifndef MACBUILD
extern RGBColor *whiteColor;
#endif

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern RGBColor bkgndGreen;
extern RGBColor brightGreen;
extern RGBColor dimGreen;
extern RGBColor dimmerGreen;
extern RGBColor rgbWhite;
extern RGBColor playerPaint;
extern RGBColor rgbLightGrey;
extern RGBColor rgbMedGrey;
extern RGBColor rgbDarkGrey;
extern RGBColor rgbLessDarkGrey;
extern RGBColor rgbRealDarkGrey;
extern RGBColor rgbRealDarkBlue;
extern RGBColor rgbBlack;
EXTERN_CPP_END
#pragma cplusplus reset

extern short GetSpriteXSize(SpriteRec *spr);
extern short GetSpriteYSize(SpriteRec *spr);
extern void SWSetCurrentFrameIndex(SpritePtr srcSpriteP, short frameIndex);
extern void SWSetSpriteLocation(SpritePtr srcSpriteP, short horizLoc,
                                short vertLoc);
extern void SWSetSpriteVisible(SpritePtr srcSpriteP, unsigned char isVisible);
extern void SWSetPortToWorkArea(SpriteWorldPtr sw);
extern void SWSetSpriteWorldMaxFPS(SpriteWorldPtr world, short fps);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern PrefsFileRec gPrefs;
#ifndef MACBUILD
EXTERN_CPP_END
#pragma cplusplus reset

extern void SetDialogItemText(Handle item, const unsigned char *pstr);
extern void GetDialogItemText(Handle item, unsigned char *pstr);
extern void DisableControl(ControlPtr ctl);
#endif
extern void SetDialogVisible(DialogPtr dlg, unsigned char value);
extern void SizeDialogWindow(DialogPtr dlg, short width, short height,
                         Boolean update);
extern void OffsetDialogWindow(DialogPtr dlg, short dh, short dv, Boolean update);
extern void CreateGameWindow(void *hInstance, int32_t nCmdShow);
extern CGrafPtr OpenGamePort(Rect *bounds, const unsigned char *title);
#ifndef MACBUILD
extern int32_t GetControlValue(ControlPtr ctl);
#endif
extern void InvalDialogRect(DialogPtr dlg, Rect *box);
#ifndef MACBUILD
extern void ModalDialog(void *filterProc, short *itemHitPtr);
extern void DisposeDialog(DialogPtr dlg);
#endif
/* the body is empty, but every caller passes an argument */
extern void ShowMacWindow(void *arg);
#ifndef MACBUILD
extern void GetDialogItem(DialogPtr dlg, int32_t item, short *type, Handle *handle,
                          Rect *box);
#endif
void GetControlText(ControlPtr ctl, char *text);
#ifndef MACBUILD
extern CGrafPtr GetDialogPort(DialogPtr dlg);
#endif
extern void InvalDialogItem(DialogPtr dlg, int32_t item);
void SetControlText(ControlPtr ctl, const char *text);
#ifndef MACBUILD
extern void SetControlValue(ControlPtr ctl, int32_t value);
extern void SetDialogItem(DialogPtr dlg, int32_t item, short type, void *handle,
                          const Rect *box);
extern void SetControlTitle(ControlPtr ctl, const unsigned char *text);

extern void PaintRect(Rect *r);
#endif

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean gameLoaded;
extern Boolean gameInProgress;
extern Boolean inBackground;
extern Boolean creatingPilotFile;
extern Boolean onPlanet;
extern Boolean closeRequested;
EXTERN_CPP_END
#pragma cplusplus reset
/* invalidate the whole game window: the port only raises its redraw flag */
#ifdef MACBUILD
#define InvalWindowPortRect(window) InvalWindowRect(window, GetCWindowPortRect(window))
#else
#define InvalWindowPortRect(window) (GetCWindowPortRect(window), gNeedUpdateFlag = 1)
#endif
extern Boolean gNeedUpdateFlag;

#ifndef MACBUILD
extern void LAutoScroll(ScrollRec **list);
extern void LScroll(int32_t unused, short delta, ScrollRec **list);
extern void LSetSelect(int track, Point pt, ScrollRec **list); /* 0x004d1cd0 */
extern void LSetDrawingMode(Boolean value, ScrollRec **list);          /* 0x004d1b70 */
extern void LSetCell(void *text, int32_t length, Point cell,
                     ScrollRec **list);                          /* 0x004d1b80 */
extern Boolean LGetSelect(int32_t unused, Point *cell, ScrollRec **list); /* 0x004d1d10 */
extern void LDispose(ScrollRec **list);                     /* 0x004d1b40 */
extern ScrollRec **nv_NewDialog(const Rect *rView, const Rect *dataBounds,
                                Point cellSize, ListDrawProcPtr drawProc, CGrafPtr port,
                                unsigned char hasScrollBar);     /* 0x004d1a60 */
#endif

#ifndef MACBUILD
extern TEHandle TENew(const Rect *destRect, const Rect *viewRect);
extern void TESetText(const void *text, int32_t length, TEHandle hTE);
extern void TECalText(TEHandle hTE);
extern void TEPinScroll(int dh, int dv, TEHandle hTE);
extern void TEUpdate(Rect *rUpdate, TEHandle hTE);
extern void TEDispose(TEHandle hTE);
#endif

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern CWindowPtr gWind;
extern GWorldContainer loadPort;
#ifndef MACBUILD
EXTERN_CPP_END
#pragma cplusplus reset

extern void *GetPixBaseAddr(PixMapPtr pm);
extern short GetPixRowBytes(PixMapPtr pm);
extern short GetPixDepth(PixMapPtr pm);
#endif

#ifdef MACBUILD
/* The engine source calls the win port's Toolbox spellings. Over the SDK's prototypes they
   are these wrappers; the parenthesised name, (PtInRect)(pt, r), is the Carbon form. PtInRect
   builds the Point in place: an inline function would widen h and v to int on the way. */
#ifdef MAC1010
static inline Boolean EV_PtInRect(short h, short v, const Rect *r)
{
    Point pt_;

    pt_.h = h;
    pt_.v = v;
    return (PtInRect)(pt_, r);
}
#define PtInRect(ph_, pv_, pr_) EV_PtInRect(ph_, pv_, pr_)
#else
#define PtInRect(ph_, pv_, pr_) ({ Point pt_; pt_.h = (ph_); pt_.v = (pv_); (PtInRect)(pt_, pr_); })
#endif
static inline short EV_NewGWorld(CGrafPtr *out, int depth, const Rect *bounds, void *theGDH)
{
    return NewGWorld(out, depth, bounds, 0, theGDH, 0);
}
#define NewGWorld(out, depth, bounds, gdh) EV_NewGWorld(out, depth, bounds, gdh)
#define FrontWindow(dlg) GetDialogWindow(dlg)
#define ShowMacWindow(window) ShowWindow(window)
#define InvalDialogRect(dlg, box) InvalWindowRect(GetDialogWindow(dlg), box)
#define SetDialogGWorld(dlg) SetPort((GrafPtr)GetDialogPort(dlg))
extern FSRef pluginsFolder;
extern FSRef novaFilesFolder;
/* MoreFilesX (Apple sample code, linked into the mac build) */
union FinderInfo { FileInfo file; FolderInfo folder; };
typedef union FinderInfo FinderInfo;
union ExtendedFinderInfo { ExtendedFileInfo file; ExtendedFolderInfo folder; };
typedef union ExtendedFinderInfo ExtendedFinderInfo;
extern OSErr FSGetFinderInfo(const FSRef *ref, FinderInfo *info, ExtendedFinderInfo *extendedInfo,
                             Boolean *isDirectory);
extern OSErr FSSetFinderInfo(const FSRef *ref, const FinderInfo *info,
                             const ExtendedFinderInfo *extendedInfo);
extern OSErr FSChangeCreatorType(const FSRef *ref, OSType fileCreator, OSType fileType);
extern GDHandle mainScreenDevice;
extern CGrafPtr GetWindowGrafPort(WindowPtr window);
extern OSErr HFSNameGetUnicodeName(ConstStr31Param hfsName, TextEncoding textEncodingHint,
                                   HFSUniStr255 *unicodeName);
/* the port releases a picture through its image cache, the mac through the resource manager */
#define QDReleaseResource(pic) ReleaseResource((Handle)(pic))
#define ReplaceInHandle Munger
#define GetNewDialog(id, updateProc, eventProc) (GetNewDialog)(id, NULL, (WindowRef)-1L)
#define RedrawDialog(dlg) InvalWindowRect(GetDialogWindow(dlg), GetDialogPortRect(dlg))
#define GetListCellRect(box, cell, list) LRect(box, cell, list)
#define SetDialogVisible(dlg, value) \
    ((value) ? ShowWindow(GetDialogWindow(dlg)) : HideWindow(GetDialogWindow(dlg)))
extern RgnHandle GetWindowVisRgn(WindowRef window);
#else
/* the port's blackColor and whiteColor are RGBColor globals, so its ForeColor(c) is RGBForeColor
   (ForeColor itself, parenthesised, takes the QuickDraw numbers) and its BackColor takes the RGBColor */
#define ForeColor(c) RGBForeColor(c)
#define RGBBackColor(c) BackColor(c)
#define GetWindowVisRgn(window) GetCWindowVisRgn(window)
/* the port keeps no window order: SelectWindow and BringToFront keep only their argument */
#define SelectWindow(window) ((void)(window))
#define BringToFront(window) ((void)(window))
#endif

#ifdef __cplusplus
}
#endif

#endif
