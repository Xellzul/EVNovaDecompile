// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierlecollision.c -- BlitPixieRLECollision (win 0x00472190), the two-stream RLE overlap test. Notes: Research/notes/blitpixierlecollision.md */
#include "macport.h"
#include "shipobj.h"

#define ALIGN_PTR(p) ( -((long)(p)) & 3)

/* BlitPixieRLECollision (mac _BlitPixieRLECollision). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00472190
char BlitPixieRLECollision(
	unsigned char *srcTokens,
	unsigned char *dstTokens,
	Rect *srcRect,
	Rect *dstRect)
{
	unsigned long	token1,op1,count1;
	unsigned long	token2,op2,count2;
	unsigned char	*nextRow1,*nextRow2;
	long			x,width;
	long			y,height;
	long			x1,x2;
	int				solid1,solid2;
	
		// advance source to first row
	for ( y = 0; y < srcRect->top; y++ )
	{ 
		token1 = *((unsigned long *) srcTokens);
		srcTokens += 4;

		op1 = token1 >> 24;
		count1 = token1 & 0xffffff;

		if ( op1 == kLineStartToken )
			srcTokens += count1;	
		else if ( op1 == kEndShapeToken )
			return 0;
	}
	
		// advance dest to first row
	for ( y = 0; y < dstRect->top; y++ )
	{ 
		token2 = *((unsigned long *) dstTokens);
		dstTokens += 4;

		op2 = token2 >> 24;
		count2 = token2 & 0xffffff;

		if ( op2 == kLineStartToken )
			dstTokens += count2;	
		else if ( op2 == kEndShapeToken )
			return 0;
	}	
	
	height = srcRect->bottom - srcRect->top;
	width = srcRect->right - srcRect->left;
	
		// check the intersecting rows for span overlap
	for ( y = 0; y < height; y++ )
	{
		token1 = *((unsigned long *) srcTokens);
		srcTokens += 4;
		
		op1 = token1 >> 24;
		count1 = token1 & 0xffffff;

		token2 = *((unsigned long *) dstTokens);
		dstTokens += 4;
		
		op2 = token2 >> 24;
		count2 = token2 & 0xffffff;

		if ( op1 == kEndShapeToken || op2 == kEndShapeToken )
			return 0;

		nextRow1 = srcTokens + count1;
		nextRow2 = dstTokens + count2;

		op1 = -1;
		op2 = -1;
		solid1 = 0;
		solid2 = 0;
		x1 = - srcRect->left;
		x2 = - dstRect->left;
		
		for ( x = x1 < x2 ? x1 : x2 ; x < width; x = x1 < x2 ? x1 : x2  )
		{
			if ( x1 < x2 )
			{
				token1 = *((unsigned long *) srcTokens);
				srcTokens += 4;
		
				op1 = token1 >> 24;
				count1 = token1 & 0xffffff;
			
				switch ( op1 )
				{
					case kDrawPixelsToken:
						solid1 = 1;
						x1 += count1;
						srcTokens += count1;
						srcTokens += ALIGN_PTR(srcTokens);
						break;
					case kSkipPixelsToken:
						solid1 = 0;
						x1 += count1;
						break;
					case kLineStartToken:
						solid1 = 0;
						x1 = width;
						break;
					case kEndShapeToken:
						return 0;
				}
			}
			else
			{
				token2 = *((unsigned long *) dstTokens);
				dstTokens += 4;
		
				op2 = token2 >> 24;
				count2 = token2 & 0xffffff;
			
				switch ( op2 )
				{
					case kDrawPixelsToken:
						solid2 = 1;
						x2 += count2;
						dstTokens += count2;
						dstTokens += ALIGN_PTR(dstTokens);
						break;
					case kSkipPixelsToken:
						solid2 = 0;
						x2 += count2;
						break;
					case kLineStartToken:
						solid2 = 0;
						x2 = width;
						break;
					case kEndShapeToken:
						return 0;
				}
			}
			
			if ( solid1 && solid2 )
				return 1;

			if ( op1 == kLineStartToken || op2 == kLineStartToken )
				break;
		}
		
		srcTokens = nextRow1;
		dstTokens = nextRow2;
	}
	
	return 0;
}
