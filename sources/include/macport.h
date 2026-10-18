/* macport.h -- Mac OS Toolbox types and shims for the Windows port. Notes: Research/notes/macport.md */
#include "gametypes.h"
#include "win32.h"
#include "burgertypes.h"

#ifndef MACPORT_H
#define MACPORT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char *StringPtr;

/* QuickDraw basics (partial) */
typedef struct Point {
    short v;
    short h;
} Point;

#pragma options align=packed
typedef struct EventRecord {
    unsigned short what;
    long message;
    long when;
    Point where;
    short modifiers;
} EventRecord;
#pragma options align=reset

typedef struct FSSpec {
    short vRefNum;
    long parID;
    Str255 name;
} FSSpec;

typedef struct CGrafPort *CWindowPtr;
typedef struct ControlRecord {
    long kind;
    long itemNo;
    long value;
    long minValue;
    long maxValue;
    Rect box;
    void *data;
    DialogPtr owner;
    char enabled;
} ControlRecord;
typedef struct ControlRecord *ControlPtr;
typedef void *Movie;
typedef struct ComponentDescription {
    long componentType;
    long componentSubType;
    long componentManufacturer;
    unsigned long componentFlags;
    unsigned long componentFlagsMask;
} ComponentDescription;

typedef struct PixMap {
    Rect bounds;
    long pixelSize;
    void *baseAddr;
    unsigned char *ctab;
    long originV;
    long width;
    long height;
    long rowBytes;
} PixMap, *PixMapPtr;
typedef struct CGrafPort {
    char offscreen;
    char filler1[3];
    PixMap pix;
    long depth;
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
typedef struct BitMap *BitMapPtr;

typedef Image_t *PicHandle;

typedef struct SrcBitMap {
    Rect bounds;
    unsigned long rowBytes;
    Image_t image;
    long unk28[2];
} SrcBitMap;

typedef struct GWorldContainer {
    CGrafPtr world;
    Rect bounds;
} GWorldContainer;

extern void GetIndString(unsigned char *str, int strListID, int index);
extern void GetIniIndString(char *out, long listID, long index);
extern void nv_UpdateDialogWindow(int all);
#pragma cplusplus on
extern "C++" {
extern GWorldContainer pictCachePort;
}
#pragma cplusplus reset
extern void SelectDialogItemText(DialogPtr dlg, long item, long start, long end);
extern void FrameRect(const Rect *r);
extern void GetFNum(const unsigned char *name, short *familyID);
extern void DrawTextRun(const void *text, unsigned long byteCount, Rect *box,
                        char clipped, int height);
extern CGrafPtr CurrentGrafPort;
extern void CopyBits(PixMapPtr srcBits, BitMapPtr dstBits,
                     const Rect *srcRect, const Rect *dstRect,
                     long mode, void *maskRgn);
extern BitMapPtr GetPortBitMapForCopyBits(CGrafPtr port);
extern void BlockMove(const void *src, void *dst, unsigned long len);
extern void CopyMask(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
                     Rect *srcRect, Rect *maskRect, Rect *dstRect);
extern void *GetPortVisRgn(void *port);
extern short NewGWorld(CGrafPtr *out, int depth, const Rect *bounds,
                       void *theGDH);
extern PixMapPtr GetGWorldPixMap(CGrafPtr world);
extern PixMapPtr GetWindowPixMap(CWindowPtr window);
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
        long xStep, long yStep, long xStart, long yStart);
extern void CopyBitsScale16Average(PixMapPtr srcBits, PixMapPtr dstBits, Rect *srcRect,
        Rect *dstRect, long xStep, long yStep, long xFrac,
        long yFrac);
extern void CopyBitsScale24(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
        long xStep, long yStep, long xFrac, long yFrac);
extern void CopyMaskNone(PixMapPtr src, PixMapPtr mask, PixMapPtr dst,
        Rect *srcRect, Rect *maskRect, Rect *dstRect);
extern void CopyMask16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
        Rect *srcRect, Rect *maskRect, Rect *dstRect);
extern void CopyMaskScale16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
        Rect *srcRect, Rect *maskRect, Rect *dstRect,
        long xStep, long yStep, long xFrac, long yFrac);

extern CGrafPtr GetCWindowGrafPort(CWindowPtr window);
extern Rect *GetWindowPortRect(CWindowPtr window);
extern Rect *GetCWindowPortRect(CWindowPtr window);
extern PixMapPtr GetCWindowPixMap(CWindowPtr window);
extern PixMapPtr GetGWCPixMap(GWorldContainer *gwc);

typedef struct SWFrameRec {
    CGrafPtr framePort;
    void *frameDevice;
    PixMapPtr framePixHndl;
    PixMapPtr framePix;
    char *frameBaseAddr;
    unsigned long frameRowBytes;
    short leftAlignFactor;
    short rightAlignFactor;
    char isFrameLocked;
    char isWindowFrame;
    char interlacingIsOn;
    char skipOddLines;
    short frameDepth;
    Rect frameRect;
    short hotSpotH;
    short hotSpotV;
    Rect collisionInset;
    Point offsetPoint;
    void *maskRgn;
    CGrafPtr maskPort;
    void *maskDevice;
    PixMapPtr maskPixHndl;
    PixMapPtr maskPix;
    char *maskBaseAddr;
    char tileMaskIsSolid;
    char sharesGWorld;
    char usesCollisionInset;
    char framePad57;
    unsigned short useCount;
    short worldRectOffset;
    unsigned short numScanLines;
    unsigned short pixelShift;
    unsigned long *scanLinePtrArray;
    void *rleDataH;
    void *rleTokenStart;
    long userData;
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
    DrawProcPtr doubleRectDrawProc;
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
    WorldProcPtr worldMoveProc;
    char swFiller80[0x84 - 0x80];
    void *tileLayerArray;
    char swFiller88[2];
    char tilingIsInitialized;
    char tilingIsOn;
    void *tilingCache;
    char swFiller90[0xa4 - 0x90];
    long numTilesChanged;
    void *changedTiles;
    char swFillerAC[4];
    void *tileChangeProc;
    void *tileRectDrawProc;
    void *tileMaskDrawProc;
    DrawProcPtr partialMaskDrawProc;
    void *mainSWGDH;
    short pixelDepth;
    short frameInterval;
    unsigned long runningTimeCount;
    char swFillerCC[4];
    unsigned long long lastMicroseconds;
    unsigned long timeOfLastFrame;
    char frameHasOccurred;
    char thereAreNonScrollingLayers;
    char swFillerDE[0xe8 - 0xde];
    long backgroundValue;
    char fastBlit;
    char swFillerED[0xf8 - 0xed];
} SpriteWorldRec, *SpriteWorldPtr;

extern void SWSetPortToWindow(SpriteWorldPtr spriteWorld);

typedef struct SpriteLayerRec {
    struct SpriteLayerRec *nextSpriteLayerP;
    struct SpriteLayerRec *prevSpriteLayerP;
    struct SpriteRec *headSpriteP;
    struct SpriteRec *tailSpriteP;
    void (*layerCallBack)(struct SpriteWorldRec *);
    long userData;
    short tileLayer;
    char isPaused;
    char layerIsNonScrolling;
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

#define noErr           0
#define memFullErr      (-108)
#define resNotFound     (-192)
#define srcCopy         0
#define noNewDevice     2               /* QuickTime GWorldFlags */
#define k16BE555PixelFormat 0x00000010  /* QuickTime pixel formats */
#define k16LE555PixelFormat 0x4c353535  /* 'L555' */

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
    ((unsigned long  *) ( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + ((x) << 2) ))

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
                             long *frameIndex);
typedef void (*MoveProcPtr)(SpritePtr srcSpriteP);
typedef void (*CollideProcPtr)(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                               Rect *sectRect);

struct SpriteRec {
    SpriteLayerRec *parentSpriteLayerP;
    SpritePtr nextSpriteP;
    SpritePtr prevSpriteP;
    SpritePtr nextActiveSpriteP;
    SpritePtr nextIdleSpriteP;
    unsigned char isVisible;
    char needsToBeDrawn;
    char needsToBeErased;
    char onTop;
    char justBecameVisible;
    char pad1b;
    Rect destFrameRect;
    Rect oldFrameRect;
    Rect deltaFrameRect;
    DrawProcPtr frameDrawProc;
    short tileDepth;
    char destRectIsVisible;
    char oldRectIsVisible;
    Rect clippedSourceRect;
    Rect destOffscreenRect;
    SWFrameRec **frameArray;
    SWFrameRec *curFrameP;
    long numFrames;
    long maxFrames;
    long frameTimeInterval;
    unsigned long timeOfLastFrameChange;
    long frameAdvance;
    long curFrameIndex;
    long firstFrameIndex;
    long lastFrameIndex;
    FrameProcPtr frameChangeProc;
    long moveTimeInterval;
    unsigned long timeOfLastMove;
    short horizMoveDelta;
    short vertMoveDelta;
    Rect moveBoundsRect;
    MoveProcPtr spriteMoveProc;
    MoveProcPtr realSpriteMoveProc;
    CollideProcPtr spriteCollideProc;
    CGrafPtr sharedPictGWorld;
    CGrafPtr sharedMaskGWorld;
    char spriteRemoval;
    char frameAdvanceMode;
    short translucency;
    short tintRed;
    short tintGreen;
    short tintBlue;
    short tintMode;
    unsigned long tintColor;
    unsigned long rotation;
    short scaledWidth;
    short scaledHeight;
    unsigned long colorValue;
    RGBColor colorRGB;
    char flippedHorizontal;
    char flippedVertical;
    short pad2;
    long userData;
    char **rleDataH;
    char collisionsDisabled;
    char ownsRLEData;
    char isEmptySprite;
    char srD3;
};

typedef struct TERec {
    char                  *text;
    long                   textLength;
    short                  txFont;
    short                  txSize;
    Rect                   destRect;
    Rect                   viewRect;
    long                   height;
    CGrafPtr               port;
} TERec, **TEHandle;

struct ScrollRec;
typedef void (*ListDrawProcPtr)(short lMessage, unsigned char lSelect, Rect *lRect,
                                Point *lCell, short lDataOffset, short lDataLen,
                                struct ScrollRec **lHandle);
typedef struct ScrollRec {
    CGrafPtr port;
    Rect rView;
    Rect dataBounds;
    Point indent;
    Point cellSize;
    ListDrawProcPtr drawProc;
    char hasScrollBar;
    long scroll;
    char **cells;
    short *cellArray;
    short maxIndex;
    Point lastClick;
    long clikTime;
    char unk3c[4];
    char drawIt;
} ScrollRec;

typedef struct DialogRecord {
    CGrafPtr port;
    Rect gwBounds;
    Rect bounds;
    Rect portRect;
    long itemCount;
    long editItem;
    long defaultItem;
    long cancelItem;
    void *items;
    void *updateProc;
    void *eventProc;
    long needsUpdate;
    long visible;
} DialogRecord, *DialogPtr;

typedef struct Particle {
    short lifeRemaining;
    char pFiller02[0x04 - 0x02];
    long horizLoc;
    long vertLoc;
    long oldHorizLoc;
    long oldVertLoc;
    long horizSpeed;
    long vertSpeed;
    long drawnHorizLoc;
    long drawnVertLoc;
    long shade;
    unsigned long color;
} Particle, *ParticlePtr;

/* Toolbox GetTime shim — port implementation at 0x004bbb10 */
extern void GetTime(DateTimeRec *dtr);
extern void AdvanceDate(DateTimeRec *date, long daysToAdd);

extern char *PtoCstr(unsigned char *str);
extern unsigned char *CtoPstr(char *str);

#define PORT_PIXMAP(port) ((PixMapPtr)((char *)(port) + 4))
#define kAutoKey       5
#define kBlue555  0x001f
#define kCharCodeMask  0xff
#define kDIBHeaderSize 0x28
#define kDepth16            0x10
#define kDownArrowChar  0x0a
#define kEndChar       0x93
#define kEscapeChar    0x1b
#define kGreen555 0x03e0
#define kHomeChar      0x91
#define kKeyDown       3
#define kLeftArrowChar  0x96
#define kMouseDown     1
#define kNullEvent   0
#define kPageDownChar   0x94
#define kPageUpChar     0x92
#define kPictType 0x50494354   /* 'PICT' */
#define kSoundType 0x736e6420  /* 'snd ' */
#define kStringType 0x53545220 /* 'STR ' */
#define kStringListType 0x53545223UL /* 'STR#' */
#define kRed555   0x7c00
#define kReturnChar    0x0d
#define kRightArrowChar 0x15
#define kTabChar       0x09
#define kUpArrowChar    0x0b
#define kUpdateEvt     6

#define kScanLShift 0x2a
#define kScanRShift 0x36
#define kScanLCtrl  0x1d
#define kScanRCtrl  0x6b
#define kScanLAlt   0x38
#define kScanRAlt   0x6f

#define shiftKey    0x0200
#define optionKey   0x0800
#define controlKey  0x1000
#define diskMask    0x0080
#define everyEvent  0xffff

#define kRLE8ResType 0x726c9138UL
#define kRLEDResType 0x726c9144UL

extern void AcquireScreenDC(void);
extern short Alert(short alertID, void *filterProc);
extern void BackColor(const RGBColor *color);
extern char Button(void);
extern void ClosePrefs(void);
#pragma cplusplus on
extern "C++" {
extern Rect screenMinusDockRect;
extern long paintBlackFlag;
}
#pragma cplusplus reset
extern void BusyIdle(void);
extern void Delay(unsigned long numTicks, unsigned long *finalTicks);
extern void DisposeCIcon(Image_t *icon);
extern void DrawCString(const char *text);
extern void DrawPicHandle(PicHandle pic, Rect *dstRect);
extern void DrawString(const unsigned char *str);
extern void DrawText(const void *textBuf, int firstByte, int byteCount);
extern void EraseRect(Rect *r);
extern void ExitToShell(void);
extern void SetupMonitor(void);
extern void DisposePixPat(Image_t *pat);
extern void InitDataFiles(void);
extern void MakeAlias(char *name);
extern short EVOpenResourceFile(void *path);
extern void SwapEndianByDescriptor(unsigned char *data, unsigned short *desc);
extern void LoadMenuListItem(ControlRecord *rec, long menuID, unsigned char *promptTitle,
                         long resType);
extern void LoadControlResource(ControlRecord *rec, unsigned long resID);
extern long PopUpListSelect(DialogPtr dlg, ControlRecord *rec);
extern void EditTextKey(ControlRecord *rec, long key, long mods);
void DrawBevel(Rect *r, const RGBColor *fill, const RGBColor *light, const RGBColor *dark);
extern void LUpdate(void *list);
extern long __stdcall KeyboardHookProc(int code, unsigned long wParam, long lParam);
extern long __stdcall WinGameWndProc(void *hwnd, unsigned long msg,
                         unsigned long wParam, long lParam);
extern void ShutdownWinGame(void);
extern void FillRectWithColor(Rect *r, RGBColor *color);
extern short FindFirstCoLocatedSystem(short sys);
extern short FindSlotEntry(short value, short count, short field);
extern void DisposeClonedSprite(void *p);
extern void ShowWindowNoFlash(void *arg);
extern void DisposeDialogNoFlash(void *dlg);
extern void FlushEvents(unsigned short whichMask, unsigned short stopMask);
extern void FrameOval(Rect *r);
extern CWindowPtr FrontWindow(void *window);
extern Image_t *GetCIcon(short iconID);
extern void *GetCWindowVisRgn(void *window);
extern Rect *GetDialogPortRect(DialogPtr dlg);
extern void *GetFont(short fontID, short size, short style);
extern void GetGlobalMouse(Point *pt);
extern void GetMouse(Point *pt);
extern DialogPtr GetNewDialog(short dlogID, void *updateProc,
                              void *eventProc);
extern PicHandle GetPicture(int picID);
extern Image_t *GetPixPat(short id);
extern void GetPort(CGrafPtr *port);
extern void InitFonts(void);
extern short InitializeQTML(long flags);
extern void *FindNextComponent(void *prev, ComponentDescription *cd);
extern void InsetRect(Rect *r, int dh, int dv);
extern void InvertRect(Rect *r);
extern unsigned char KeyCheck(short scanCode);
extern long GetKeyModifiers(void);
extern void KeyboardScanCodeToString(char *buf, unsigned long size, unsigned long code);
extern unsigned int KeyIsDown(unsigned int scanCode);
extern long KeyboardGet2(void);
extern void Line(short dh, short dv);
extern void LineTo(short h, short v);
extern void *LoadSoundResource(short id);
extern short MeasureCString(const char *text);
extern void MoveTo(short h, short v);
extern void OffsetRect(Rect *r, int dh, int dv);
extern void OpenDataFiles(void);
extern void OpenPrefs(void);
extern void PStr2CStr(char *dst, const unsigned char *src);
extern void PaintOval(Rect *r);
extern void PaletteSetPtr(unsigned int start, unsigned int count, const unsigned char *pal);
extern void PenSize(short width, short height);
extern void CloseMovieFile(short resRefNum);
#if !defined(__cplusplus) || !defined(_MSL_CSTDLIB)
#if __MWERKS__ >= 0x3000
__declspec(noreturn) extern void exit(int status);
#else
extern void exit(int status);   /* mwcc 2.4, AmbrosiaTools' compiler, has no noreturn */
#endif
#endif
#if !defined(__cplusplus) || !defined(_MSL_CSTRING)
extern char *strrchr(const char *s, int c);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern char *strcat(char *dst, const char *src);
extern int strcmp(const char *a, const char *b);
#endif
#if !defined(__cplusplus) || !defined(_MSL_CSTDIO)
extern int sprintf(char *dst, const char *fmt, ...);
#endif
#if !defined(__cplusplus) || !defined(_MSL_CCTYPE)
extern int isprint(int c);
extern int isalpha(int c);
extern int isdigit(int c);
extern int tolower(int c);
extern int toupper(int c);
#endif
extern int PtInRect(int h, int v, Rect *r);
extern void QDReleaseResource(PicHandle pic);
extern void RGBForeColor(const RGBColor *color);
extern void ForeColor(long color);
extern void *NewPtr(long size);
extern void DisposePtr(void *ptr);
extern unsigned long TickCount(void);
extern CGrafPtr GetWindowPort(CWindowPtr window);
extern void QDFlushPortBuffer(CGrafPtr port, void *region);
extern void PtToAngle(const Rect *r, Point pt, short *angle);
extern char RectInRgn(Rect *r, void *rgn);
extern void RedrawDialog(DialogPtr dlg);
extern void RedrawDialogs(int a);
extern void ReleaseScreenDC(void);
extern void ResetScreenRect(void);
extern void ScreenEndScene(void);
extern int SectRect(const Rect *a, const Rect *b, Rect *dst);
extern unsigned long Color2Index(RGBColor *color);
extern void SetDialogDefaultItem(DialogPtr dlg, long value);
extern short Random(void);
extern short SWSetSpriteWorldScreenDrawProc(SpriteWorldRec *spriteWorld,
                                            DrawProcPtr proc);
extern void SetMovieBox(Movie movie, Rect *box);
extern void DisposeMovie(Movie movie);
extern void GetMovieBox(Movie movie, Rect *box);
extern long GetMovieDuration(Movie movie);
extern void GoToBeginningOfMovie(Movie movie);
extern char IsMovieDone(Movie movie);
extern void LoadMovieIntoRam(Movie movie, long time, long duration, long flags);
extern void MoviesTask(Movie movie, long maxMs);
extern void Microseconds(UnsignedWide *now);
extern void StringToNum(const unsigned char *str, long *value);
extern void *GetMovieTimeBase(Movie movie);
extern void SetMovieRate(Movie movie, long rate);
extern void SetMoviePlayHints(Movie movie, long a2, long a3);
extern void *GetMovieIndTrackType(Movie movie, long a, long type, long b);
extern void SetMovieGWorld(Movie movie, void *gworld, long a3);
extern void StartMovie(Movie movie);
extern void StopMovie(Movie movie);
extern void SetPort(CGrafPtr port);
extern void SetPortWindowPort(CWindowPtr window);
extern void SetRect(Rect *r, int left, int top, int right, int bottom);
extern void ShowIndAlert(short index);
extern void *ST_LoadSndResourceIndirect(void **handle, long flags);
extern char StillDown(void);
extern void StrcatOrdinalNumber(short n, char *dst, char capitalize);
extern short StringWidth(const unsigned char *str);
extern void SysBeep(short duration);
extern void TerminateQTML(void);
extern void TETextBox(const void *text, long length, Rect *box, short just);
extern short FSMakeFSSpec(long vRefNum, long dirID, const void *name, void *spec);
extern short OpenMovieFile(const void *spec, short *refNum, long mode);
extern long NewMovieFromFile(Movie *movie, short refNum, short *resId,
                             long a4, long a5, long a6);
extern int MeasureWrappedText(const char *text, int byteCount, int width);
extern void NumToString(long value, unsigned char *str);
extern void QDGetPictureBounds(PicHandle pic, Rect *r);
extern void HideWindow(CWindowPtr window);
extern void *GetGWorldDevice(CGrafPtr port);
extern void DisposeGWorld(CGrafPtr port);
extern void DebugStr(const char *message);
extern short MemError(void);
extern void FillCRect(Rect *dstRect, Image_t *pat);
extern short GetFrameBiggestSize(SWFrameRec *frame);
extern void GetMenuItemText(ControlPtr ctl, int index, unsigned char *text);
extern void SWSetPostEraseCallBack(SpriteWorldPtr world, WorldProcPtr proc);
extern void SWSetPostDrawCallBack(SpriteWorldPtr world, WorldProcPtr proc);
extern void DisposeWindow(CWindowPtr window);
extern void *GetGDevice(void);
extern void BuildGammaRamp(float gamma);
extern long ReplaceInHandle(char **h, long offset, const char *find, long findLen,
                         const char *repl, long replLen);
extern void GetListCellRect(Rect *box, Point cell, ScrollRec **h);
extern char LClick(Point where, short modifiers, ScrollRec **h);
extern void ParamText(const unsigned char *p0, const unsigned char *p1,
                      const unsigned char *p2, const unsigned char *p3);
#pragma cplusplus on
extern "C++" {
extern char gParamText0[];
extern char gParamText1[];
extern char gParamText2[];
extern char gParamText3[];
}
#pragma cplusplus reset
extern void FadeGammaTo(unsigned long percent, unsigned long duration);
extern char SplitTwoLineString(const unsigned char *src, unsigned char *line1,
                               unsigned char *line2);
extern void TextFace(short face);
extern void TextFont(short font);
extern void TextSize(short size);
extern short TextWidth(const char *text, int byteCount);
extern void UpdateAndPageFlip(void);
extern RGBColor *blackColor;
extern long gCaretPhase;
#pragma cplusplus on
extern "C++" {
extern long gBlinkTick;
extern SWFrameRec *floatingEscortMenuFrame;
extern SWFrameRec *floatingMapFrame;
extern short gChicagoFont;
extern void *gClickDialog;
extern void *gClickItem;
extern unsigned long gClickTime;
extern long gDialogCount;
extern DialogPtr gCurDialog;
extern unsigned char gGammaRamp[32];
extern void *gKeyboardHook;
extern long gLastModifiers;
extern void *gMacRomanToCP1252;
extern SpriteWorldPtr gSpriteWorld;

extern SpriteRec *masterShipSprite[768];
extern SpriteRec *masterShipGlowSprite[768];
extern SpriteRec *masterShipShieldSprite[768];
extern SpriteRec *masterShipLightSprite[768];
extern SpriteRec *masterShipWeaponSprite[768];
extern SpriteRec *masterShipAltSprite[768];
extern CGrafPort gGamePort;
extern RGBColor blackRGB;
extern long gRezOpenPending;
extern long gUseKeyboardHook;
}
#pragma cplusplus reset
extern void nv_BlitImageOr(Rect *dstRect, Image_t *icon);
extern int gDisableUpdatesMode;
extern RGBColor *whiteColor;

#pragma cplusplus on
extern "C++" {
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
}
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
extern "C++" {
extern PrefsFileRec gPrefs;
}
#pragma cplusplus reset

extern void SetDialogItemText(ControlPtr ctl, const unsigned char *pstr);
extern void GetDialogItemText(ControlPtr ctl, unsigned char *pstr);
extern void DisableControl(ControlPtr ctl);
extern void SetDialogVisible(DialogPtr dlg, unsigned char value);
extern void SizeDialogWindow(DialogPtr dlg, short width, short height,
                         char update);
extern void OffsetDialogWindow(DialogPtr dlg, short dh, short dv, char update);
extern void CreateGameWindow(void *hInstance, long nCmdShow);
extern CGrafPtr OpenGamePort(Rect *bounds, const unsigned char *title);
extern long GetControlValue(ControlPtr ctl);
extern void InvalDialogRect(DialogPtr dlg, Rect *box);
extern void ModalDialog(void *filterProc, short *itemHitPtr);
extern void DisposeDialog(DialogPtr dlg);
/* the body is empty, but every caller passes an argument */
extern void ShowMacWindow(void *arg);
extern void GetDialogItem(DialogPtr dlg, long item, short *type, void **handle,
                          Rect *box);
void GetControlText(ControlPtr ctl, char *text);
extern CGrafPtr GetDialogPort(DialogPtr dlg);
extern void InvalDialogItem(DialogPtr dlg, long item);
void SetControlText(ControlPtr ctl, const char *text);
extern void SetControlValue(ControlPtr ctl, long value);
extern void SetDialogItem(DialogPtr dlg, long item, short type, void *handle,
                          const Rect *box);
extern void SetControlTitle(ControlPtr ctl, const unsigned char *text);

extern void PaintRect(Rect *r);

#pragma cplusplus on
extern "C++" {
extern char gameLoaded;
extern char gameInProgress;
extern char inBackground;
extern char creatingPilotFile;
extern char onPlanet;
extern char closeRequested;
}
#pragma cplusplus reset
extern char gNeedUpdateFlag;

extern void LAutoScroll(ScrollRec **list);
extern void LScroll(long unused, short delta, ScrollRec **list);
extern void LSetSelect(int track, Point pt, ScrollRec **list); /* 0x004d1cd0 */
extern void LSetDrawingMode(char value, ScrollRec **list);          /* 0x004d1b70 */
extern void LSetCell(void *text, long length, Point cell,
                     ScrollRec **list);                          /* 0x004d1b80 */
extern char LGetSelect(long unused, Point *cell, ScrollRec **list); /* 0x004d1d10 */
extern void LDispose(ScrollRec **list);                     /* 0x004d1b40 */
extern ScrollRec **nv_NewDialog(const Rect *rView, const Rect *dataBounds,
                                Point cellSize, ListDrawProcPtr drawProc, CGrafPtr port,
                                unsigned char hasScrollBar);     /* 0x004d1a60 */

extern TEHandle TENew(const Rect *destRect, const Rect *viewRect);
extern void TESetText(const void *text, long length, TEHandle hTE);
extern void TECalText(TEHandle hTE);
extern void TEPinScroll(int dh, int dv, TEHandle hTE);
extern void TEUpdate(Rect *rUpdate, TEHandle hTE);
extern void TEDispose(TEHandle hTE);

#pragma cplusplus on
extern "C++" {
extern CWindowPtr gWind;
extern GWorldContainer loadPort;
}
#pragma cplusplus reset

extern void *GetPixBaseAddr(PixMapPtr pm);
extern short GetPixRowBytes(PixMapPtr pm);
extern short GetPixDepth(PixMapPtr pm);

#ifdef __cplusplus
}
#endif

#endif
