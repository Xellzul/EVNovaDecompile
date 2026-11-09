/* swfastline.h -- SpriteWorld 2.3.1's SWFastLine.h as the mac 1.1.1 build saw it (the 68k assembler entry points dropped). */
#ifndef __SWFASTLINE__
#define __SWFASTLINE__
typedef void (*BlitLineProcPtr)(FramePtr dstFrameP, Point srcPoint, Point dstPoint, Byte color);
typedef void (*CopyLineProcPtr)(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint);

extern void SWDrawLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint, Byte color);
extern void SWCopyLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint);
extern void SWDrawFrameRect(FramePtr dstFrameP, Rect *destRect, Byte color);
extern void SWCopyFrameRect(FramePtr srcFrameP, FramePtr dstFrameP, Rect *destRect);
extern void SWDrawLineInScrollingWorld(SpriteWorldPtr spriteWorldP, FramePtr dstFrameP,
	Point srcPoint, Point dstPoint, Byte color);
extern void SWCopyLineInScrollingWorld(SpriteWorldPtr spriteWorldP, FramePtr srcFrameP,
	FramePtr dstFrameP, Point srcPoint, Point dstPoint, Rect *clipRectP);
extern void SWDrawFrameRectInScrollingWorld(SpriteWorldPtr spriteWorldP, FramePtr dstFrameP,
	Rect *dstRectP, Byte color);
extern void SWCopyFrameRectInScrollingWorld(SpriteWorldPtr spriteWorldP, FramePtr srcFrameP,
	FramePtr dstFrameP, Rect *dstRectP, Rect *clipRectP);
extern Boolean SWClipLineWithRect(Point *srcPoint, Point *dstPoint, Rect *clipRect);

extern void BlitLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint, Byte color);
extern void BPCopyLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint);
extern void BlitWrappingLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint, Byte color);
extern void BPCopyWrappingLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint);
extern void BlitStraightLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint, Byte color);
extern void BPCopyStraightLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint);
extern void BlitStraightWrappingLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint, Byte color);
extern void BPCopyStraightWrappingLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint);
#endif
