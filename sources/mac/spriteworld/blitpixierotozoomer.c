/* blitpixierotozoomer.c -- BlitPixieRotoZoomer, the mac 1.1.1 BlitPixieRotoZoomer.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

static Boolean LogBase2(long x,long *bit);

/*
	trig tables : sintable[360], costable[360] in 16.16 fixed point format

*/

static long trigtable[360 + 90] =
{
          +0,   +1143,   +2287,   +3429,   +4571,   +5711,   +6850,   +7986,
       +9120,  +10251,  +11380,  +12504,  +13625,  +14742,  +15854,  +16961,
      +18063,  +19160,  +20251,  +21336,  +22414,  +23485,  +24549,  +25606,
      +26655,  +27696,  +28728,  +29752,  +30766,  +31771,  +32767,  +33753,
      +34728,  +35692,  +36646,  +37589,  +38520,  +39439,  +40347,  +41242,
      +42125,  +42994,  +43851,  +44694,  +45524,  +46340,  +47141,  +47929,
      +48702,  +49459,  +50202,  +50930,  +51642,  +52338,  +53018,  +53683,
      +54330,  +54962,  +55576,  +56174,  +56754,  +57318,  +57863,  +58392,
      +58902,  +59394,  +59869,  +60325,  +60763,  +61182,  +61582,  +61964,
      +62327,  +62671,  +62996,  +63301,  +63588,  +63855,  +64102,  +64330,
      +64539,  +64728,  +64897,  +65046,  +65175,  +65285,  +65375,  +65445,
      +65495,  +65525,  +65534,  +65525,  +65495,  +65445,  +65375,  +65285,
      +65175,  +65046,  +64897,  +64728,  +64539,  +64330,  +64102,  +63855,
      +63588,  +63301,  +62996,  +62671,  +62327,  +61964,  +61582,  +61182,
      +60762,  +60325,  +59869,  +59394,  +58902,  +58392,  +57863,  +57318,
      +56754,  +56174,  +55576,  +54962,  +54330,  +53683,  +53018,  +52338,
      +51642,  +50930,  +50202,  +49459,  +48702,  +47929,  +47141,  +46340,
      +45524,  +44694,  +43851,  +42994,  +42125,  +41242,  +40347,  +39440,
      +38520,  +37589,  +36646,  +35693,  +34728,  +33753,  +32767,  +31772,
      +30766,  +29752,  +28728,  +27696,  +26655,  +25606,  +24550,  +23485,
      +22414,  +21336,  +20251,  +19160,  +18064,  +16961,  +15854,  +14742,
      +13625,  +12504,  +11380,  +10252,   +9120,   +7986,   +6850,   +5712,
       +4571,   +3430,   +2287,   +1144,      +0,   -1143,   -2286,   -3429,
       -4571,   -5711,   -6849,   -7986,   -9120,  -10251,  -11379,  -12504,
      -13625,  -14741,  -15853,  -16961,  -18063,  -19160,  -20251,  -21335,
      -22413,  -23485,  -24549,  -25606,  -26655,  -27695,  -28728,  -29751,
      -30766,  -31771,  -32767,  -33752,  -34727,  -35692,  -36646,  -37588,
      -38520,  -39439,  -40346,  -41242,  -42124,  -42994,  -43851,  -44694,
      -45523,  -46339,  -47141,  -47928,  -48701,  -49459,  -50202,  -50929,
      -51641,  -52338,  -53018,  -53682,  -54330,  -54961,  -55576,  -56174,
      -56754,  -57317,  -57863,  -58391,  -58902,  -59394,  -59868,  -60324,
      -60762,  -61181,  -61582,  -61964,  -62327,  -62671,  -62996,  -63301,
      -63588,  -63855,  -64102,  -64330,  -64539,  -64728,  -64897,  -65046,
      -65175,  -65285,  -65375,  -65445,  -65495,  -65525,  -65534,  -65525,
      -65495,  -65445,  -65375,  -65285,  -65176,  -65046,  -64897,  -64728,
      -64539,  -64331,  -64103,  -63855,  -63588,  -63302,  -62996,  -62671,
      -62327,  -61964,  -61583,  -61182,  -60763,  -60325,  -59869,  -59395,
      -58902,  -58392,  -57864,  -57318,  -56755,  -56175,  -55577,  -54962,
      -54331,  -53683,  -53019,  -52339,  -51642,  -50930,  -50203,  -49460,
      -48702,  -47930,  -47142,  -46341,  -45525,  -44695,  -43852,  -42995,
      -42126,  -41243,  -40348,  -39440,  -38521,  -37590,  -36647,  -35693,
      -34729,  -33754,  -32768,  -31773,  -30767,  -29753,  -28729,  -27697,
      -26656,  -25607,  -24551,  -23486,  -22415,  -21337,  -20252,  -19161,
      -18065,  -16963,  -15855,  -14743,  -13626,  -12506,  -11381,  -10253,
       -9122,   -7988,   -6851,   -5713,   -4572,   -3431,   -2288,   -1145

			// table repeats, for the costable
          +0,   +1143,   +2287,   +3429,   +4571,   +5711,   +6850,   +7986,
       +9120,  +10251,  +11380,  +12504,  +13625,  +14742,  +15854,  +16961,
      +18063,  +19160,  +20251,  +21336,  +22414,  +23485,  +24549,  +25606,
      +26655,  +27696,  +28728,  +29752,  +30766,  +31771,  +32767,  +33753,
      +34728,  +35692,  +36646,  +37589,  +38520,  +39439,  +40347,  +41242,
      +42125,  +42994,  +43851,  +44694,  +45524,  +46340,  +47141,  +47929,
      +48702,  +49459,  +50202,  +50930,  +51642,  +52338,  +53018,  +53683,
      +54330,  +54962,  +55576,  +56174,  +56754,  +57318,  +57863,  +58392,
      +58902,  +59394,  +59869,  +60325,  +60763,  +61182,  +61582,  +61964,
      +62327,  +62671,  +62996,  +63301,  +63588,  +63855,  +64102,  +64330,
      +64539,  +64728,  +64897,  +65046,  +65175,  +65285,  +65375,  +65445,
      +65495,  +65525
};

static long *sintable = &trigtable[0];
static long *costable = &trigtable[90];

	/* texture delta values */
#define horiz_delta_u	( + cost )
#define horiz_delta_v	( + sint )
#define vert_delta_u	( - sint )
#define vert_delta_v	( + cost )


/*
		calculate the log base 2 of the argument,
		i.e how many bits the number is shifted

		return value: is x a power of 2 at all ?
*/

static Boolean LogBase2(long x,long *bit)
{
	long 			i;
	unsigned long	n;

	if (x <= 0)
		return false;

	n =  x;

	for (i = 0; (n & 1) == 0; i++)
	{
		n >>= 1;
	}

	if (n == 1)
	{
		*bit = i;		// 1 << i == x
		return true;
	}
	else
		return false;
}

/* BlitPixieRotoZoom8Bit (mac _BlitPixieRotoZoom8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRotoZoom8Bit i386 0x000be60f ppc 0x000ca000
void BlitPixieRotoZoom8Bit(
	unsigned char *source,
	unsigned char *dest,
	unsigned long srcBytes,
	unsigned long dstBytes,
	unsigned long width1,
	unsigned long height1,
	unsigned long width2,
	unsigned long height2,
	unsigned long angle,
	unsigned long zoom )
{
	long			x,y;							// loop variables
	register unsigned char	*src,*dst;				// image pointers
	unsigned long	stride;							// add to go to next row
	long			half_width,half_height;			// half of destination
	long			start_u,start_v;				// starting texel
	long			sint,cost;						// sine and cosine
	unsigned long	u,v,ul,vl;						// texel, and fixed point texel
	long			is_shift,shift_x,shift_y;		// shift instead of multiply

		/* normalize angle and zoom */
	while (angle < 0)		angle += 360;
	while (angle >= 360)	angle -= 360;

		/* get look-up values */
	zoom >>= 8;
	sint = (sintable[angle] * (long) zoom) >> 8;
	cost = (costable[angle] * (long) zoom) >> 8;

	half_width = width2 >> 1;
	half_height = height2 >> 1;

		/* get texture starting values (rotate around center) */
	start_u = (width1 >> 1) << 16;
	start_v = (height1 >> 1) << 16;
	start_u -= cost * half_width - sint * half_height;
	start_v -= sint * half_width + cost * half_height;

		/* init pointers & counters */
	src =  source;
	dst =  dest;
	stride = (dstBytes - width2);

		/* is texture a multiple of 2 ? */
	is_shift = LogBase2(width1,&shift_x) && LogBase2(height1,&shift_y);

	y = height2;

	if (is_shift)
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v > 0 && v < height1 )
					*dst++ = src[(v << shift_x) + u];
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}
	else
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v >= 0 && v < height1 )
					*dst++ = src[(v * srcBytes) + u];
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);

	}

}

/* BlitPixieMaskRotoZoom8Bit (mac _BlitPixieMaskRotoZoom8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieMaskRotoZoom8Bit i386 0x000be7ea ppc 0x000ca22c
void BlitPixieMaskRotoZoom8Bit(
	unsigned char *source,
	unsigned char *dest,
	unsigned char *maskAddr,
	unsigned long srcBytes,
	unsigned long dstBytes,
	unsigned long width1,
	unsigned long height1,
	unsigned long width2,
	unsigned long height2,
	unsigned long angle,
	unsigned long zoom )
{
	long			x,y;							// loop variables
	register unsigned char	*src,*dst,*mask;		// image pointers
	unsigned long	stride;							// add to go to next row
	long			half_width,half_height;			// half of destination
	long			start_u,start_v;				// starting texel
	long			sint,cost;						// sine and cosine
	unsigned long	u,v,ul,vl;						// texel, and fixed point texel
	long			is_shift,shift_x,shift_y;		// shift instead of multiply

		/* normalize angle and zoom */
	while (angle < 0)		angle += 360;
	while (angle >= 360)	angle -= 360;

		/* get look-up values */
	zoom >>= 8;
	sint = (sintable[angle] * (long) zoom) >> 8;
	cost = (costable[angle] * (long) zoom) >> 8;

	half_width = width2 >> 1;
	half_height = height2 >> 1;

		/* get texture starting values (rotate around center) */
	start_u = (width1 >> 1) << 16;
	start_v = (height1 >> 1) << 16;
	start_u -= cost * half_width - sint * half_height;
	start_v -= sint * half_width + cost * half_height;

		/* init pointers & counters */
	src =  source;
	mask =  maskAddr;
	dst =  dest;
	stride = (dstBytes - width2);

		/* is texture a multiple of 2 ? */
	is_shift = LogBase2(width1,&shift_x) && LogBase2(height1,&shift_y);

	y = height2;

	if (is_shift)
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v > 0 && v < height1 )
				{
					if ( mask[(v << shift_x) + u] == 0 )
						*dst++ = src[(v << shift_x) + u];
					else
						++dst;
				}
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}
	else
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v >= 0 && v < height1 )
				{
					if ( mask[(v * srcBytes) + u] == 0 )
						*dst++ = src[(v * srcBytes) + u];
					else
						++dst;
				}
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}

}

/* BlitPixieRotoZoom16Bit (mac _BlitPixieRotoZoom16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRotoZoom16Bit i386 0x000be9e9 ppc 0x000ca474
void BlitPixieRotoZoom16Bit(
	unsigned short *source,
	unsigned short *dest,
	unsigned long srcBytes,
	unsigned long dstBytes,
	unsigned long width1,
	unsigned long height1,
	unsigned long width2,
	unsigned long height2,
	unsigned long angle,
	unsigned long zoom )
{
	long			x,y;							// loop variables
	register unsigned short	*src,*dst;				// image pointers
	unsigned long	stride;							// add to go to next row
	long			half_width,half_height;			// half of destination
	long			start_u,start_v;				// starting texel
	long			sint,cost;						// sine and cosine
	unsigned long	u,v,ul,vl;						// texel, and fixed point texel
	long			is_shift,shift_x,shift_y;		// shift instead of multiply

		/* normalize angle and zoom */
	while (angle < 0)		angle += 360;
	while (angle >= 360)	angle -= 360;

		/* get look-up values */
	zoom >>= 8;
	sint = (sintable[angle] * (long) zoom) >> 8;
	cost = (costable[angle] * (long) zoom) >> 8;

	half_width = width2 >> 1;
	half_height = height2 >> 1;

		/* get texture starting values (rotate around center) */
	start_u = (width1 >> 1) << 16;
	start_v = (height1 >> 1) << 16;
	start_u -= cost * half_width - sint * half_height;
	start_v -= sint * half_width + cost * half_height;

		/* init pointers & counters */
	src =  source;
	dst =  dest;
	stride = (dstBytes - width2);

		/* is texture a multiple of 2 ? */
	is_shift = LogBase2(width1,&shift_x) && LogBase2(height1,&shift_y);

	y = height2;

	if (is_shift)
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v > 0 && v < height1 )
					*dst++ = src[(v << shift_x) + u];
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}
	else
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v >= 0 && v < height1 )
					*dst++ = src[(v * srcBytes) + u];
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);

	}

}

/* BlitPixieMaskRotoZoom16Bit (mac _BlitPixieMaskRotoZoom16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieMaskRotoZoom16Bit i386 0x000bebd3 ppc 0x000ca6b0
void BlitPixieMaskRotoZoom16Bit(
	unsigned short *source,
	unsigned short *dest,
	unsigned short *maskAddr,
	unsigned long srcBytes,
	unsigned long dstBytes,
	unsigned long width1,
	unsigned long height1,
	unsigned long width2,
	unsigned long height2,
	unsigned long angle,
	unsigned long zoom )
{
	long			x,y;							// loop variables
	register unsigned short	*src,*dst,*mask;		// image pointers
	unsigned long	stride;							// add to go to next row
	long			half_width,half_height;			// half of destination
	long			start_u,start_v;				// starting texel
	long			sint,cost;						// sine and cosine
	unsigned long	u,v,ul,vl;						// texel, and fixed point texel
	long			is_shift,shift_x,shift_y;		// shift instead of multiply

		/* normalize angle and zoom */
	while (angle < 0)		angle += 360;
	while (angle >= 360)	angle -= 360;

		/* get look-up values */
	zoom >>= 8;
	sint = (sintable[angle] * (long) zoom) >> 8;
	cost = (costable[angle] * (long) zoom) >> 8;

	half_width = width2 >> 1;
	half_height = height2 >> 1;

		/* get texture starting values (rotate around center) */
	start_u = (width1 >> 1) << 16;
	start_v = (height1 >> 1) << 16;
	start_u -= cost * half_width - sint * half_height;
	start_v -= sint * half_width + cost * half_height;

		/* init pointers & counters */
	src =  source;
	mask =  maskAddr;
	dst =  dest;
	stride = (dstBytes - width2);

		/* is texture a multiple of 2 ? */
	is_shift = LogBase2(width1,&shift_x) && LogBase2(height1,&shift_y);

	y = height2;

	if (is_shift)
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v > 0 && v < height1 )
				{
					if ( mask[(v << shift_x) + u] == 0 )
						*dst++ = src[(v << shift_x) + u];
					else
						++dst;
				}
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}
	else
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v >= 0 && v < height1 )
				{
					if ( mask[(v * srcBytes) + u] == 0 )
						*dst++ = src[(v * srcBytes) + u];
					else
						++dst;
				}
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}

}

/* BlitPixieRotoZoom32Bit (mac _BlitPixieRotoZoom32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRotoZoom32Bit i386 0x000bedea ppc 0x000ca908
void BlitPixieRotoZoom32Bit(
	unsigned long *source,
	unsigned long *dest,
	unsigned long srcBytes,
	unsigned long dstBytes,
	unsigned long width1,
	unsigned long height1,
	unsigned long width2,
	unsigned long height2,
	unsigned long angle,
	unsigned long zoom )
{
	long			x,y;							// loop variables
	register unsigned long	*src,*dst;				// image pointers
	unsigned long	stride;							// add to go to next row
	long			half_width,half_height;			// half of destination
	long			start_u,start_v;				// starting texel
	long			sint,cost;						// sine and cosine
	unsigned long	u,v,ul,vl;						// texel, and fixed point texel
	long			is_shift,shift_x,shift_y;		// shift instead of multiply

		/* normalize angle and zoom */
	while (angle < 0)		angle += 360;
	while (angle >= 360)	angle -= 360;

		/* get look-up values */
	zoom >>= 8;
	sint = (sintable[angle] * (long) zoom) >> 8;
	cost = (costable[angle] * (long) zoom) >> 8;

	half_width = width2 >> 1;
	half_height = height2 >> 1;

		/* get texture starting values (rotate around center) */
	start_u = (width1 >> 1) << 16;
	start_v = (height1 >> 1) << 16;
	start_u -= cost * half_width - sint * half_height;
	start_v -= sint * half_width + cost * half_height;

		/* init pointers & counters */
	src =  source;
	dst =  dest;
	stride = (dstBytes - width2);

		/* is texture a multiple of 2 ? */
	is_shift = LogBase2(width1,&shift_x) && LogBase2(height1,&shift_y);

	y = height2;

	if (is_shift)
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v > 0 && v < height1 )
					*dst++ = src[(v << shift_x) + u];
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}
	else
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v >= 0 && v < height1 )
					*dst++ = src[(v * srcBytes) + u];
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);

	}

}

/* BlitPixieMaskRotoZoom32Bit (mac _BlitPixieMaskRotoZoom32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieMaskRotoZoom32Bit i386 0x000befd2 ppc 0x000cab44
void BlitPixieMaskRotoZoom32Bit(
	unsigned long *source,
	unsigned long *dest,
	unsigned long *maskAddr,
	unsigned long srcBytes,
	unsigned long dstBytes,
	unsigned long width1,
	unsigned long height1,
	unsigned long width2,
	unsigned long height2,
	unsigned long angle,
	unsigned long zoom )
{
	long			x,y;							// loop variables
	register unsigned long	*src,*dst,*mask;		// image pointers
	unsigned long	stride;							// add to go to next row
	long			half_width,half_height;			// half of destination
	long			start_u,start_v;				// starting texel
	long			sint,cost;						// sine and cosine
	unsigned long	u,v,ul,vl;						// texel, and fixed point texel
	long			is_shift,shift_x,shift_y;		// shift instead of multiply

		/* normalize angle and zoom */
	while (angle < 0)		angle += 360;
	while (angle >= 360)	angle -= 360;

		/* get look-up values */
	zoom >>= 8;
	sint = (sintable[angle] * (long) zoom) >> 8;
	cost = (costable[angle] * (long) zoom) >> 8;

	half_width = width2 >> 1;
	half_height = height2 >> 1;

		/* get texture starting values (rotate around center) */
	start_u = (width1 >> 1) << 16;
	start_v = (height1 >> 1) << 16;
	start_u -= cost * half_width - sint * half_height;
	start_v -= sint * half_width + cost * half_height;

		/* init pointers & counters */
	src =  source;
	mask =  maskAddr;
	dst =  dest;
	stride = (dstBytes - width2);

		/* is texture a multiple of 2 ? */
	is_shift = LogBase2(width1,&shift_x) && LogBase2(height1,&shift_y);

	y = height2;

	if (is_shift)
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v > 0 && v < height1 )
				{
					if ( mask[(v << shift_x) + u] == 0 )
						*dst++ = src[(v << shift_x) + u];
					else
						++dst;
				}
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}
	else
	{
		do
		{
			ul = start_u;	vl = start_v;

			for (x = 0; x < width2; x++)
			{
				u = ul >> 16;	v = vl >> 16;

				if (u >= 0 && u < width1 && v >= 0 && v < height1 )
				{
					if ( mask[(v * srcBytes) + u] == 0 )
						*dst++ = src[(v * srcBytes) + u];
					else
						++dst;
				}
				else
					++dst;

				ul += horiz_delta_u;	vl += horiz_delta_v;
			}

			start_u += vert_delta_u;	start_v += vert_delta_v;

			dst += stride;

		} while (--y);
	}

}
