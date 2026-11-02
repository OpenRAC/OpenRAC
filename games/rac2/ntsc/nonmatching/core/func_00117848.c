/* GPT-6.1 Sol, 28 October 2026: FAILED, 1044/1108 bytes, 0/1 places.
 * Profile: -O2 -G2 -ffunction-sections -mastra-quad-saves.
 * fdlibm fmod from https://github.com/freemint/fdlibm/blob/master/e_fmod.c,
 * public Sun licence retained below. Algorithm and special cases agree with
 * retail, including (x*y)/(x*y), signed zero table at D_0013A3F8 and
 * normalization/subtraction loops. Entry allocation/order differs, frame is
 * 0x50 versus 0x40, and output union writes fold away retail's mask operations.
 * Three output-union spellings tied; next useful experiment is to identify
 * the original public fdlibm word-update macros/profile, not add padding.
 * All declarations and macros needed are below. Retail consumes two doubles;
 * the old cvt draft's third int argument was unsupported by the callee.
 */
#include "common.h"
#define IC(x) x
#define GET_DOUBLE_WORDS(hi,lo,d) { union {double f; s64 i;} u; s64 bits; u.f=(d); bits=u.i; (lo)=(s32)bits; (hi)=(s32)(bits>>32); }
#define INSERT_WORDS(d,hi,lo) { union {double f; u32 i[2];} u; u.f=(d); u.i[1]=(hi); (d)=u.f; u.f=(d); u.i[0]=(lo); (d)=u.f; }
/* @(#)e_fmod.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * __ieee754_fmod(x,y)
 * Return x mod y in exact arithmetic
 * Method: shift and subtract
 */

double func_00117848(double x, double y)
{
	s32 n, hx, hy, hz, ix, iy, sx, i;
	u32 lx, ly, lz;
    s32 rawlx, rawly;

	static const double one = 1.0;
	extern double D_0013A3F8[];
#define Zero D_0013A3F8

	GET_DOUBLE_WORDS(hx, rawlx, x);
    lx = rawlx;
	GET_DOUBLE_WORDS(hy, rawly, y);
    ly = rawly;
	sx = hx & IC(0x80000000);			/* sign of x */
	hx ^= sx;							/* |x| */
	hy &= IC(0x7fffffff);				/* |y| */

	/* purge off exception values */
	if ((hy | ly) == 0 || (hx >= IC(0x7ff00000)) ||	/* y=0,or x not finite */
		((hy | ((ly | -ly) >> 31)) > IC(0x7ff00000)))	/* or y is NaN */
		return (x * y) / (x * y);
	if (hx <= hy)
	{
		if ((hx < hy) || (lx < ly))
			return x;					/* |x|<|y| return x */
		if (lx == ly)
			return Zero[(u32) sx >> 31];	/* |x|=|y| return x*0 */
	}

	/* determine ix = ilogb(x) */
	if (hx < IC(0x00100000))
	{									/* subnormal x */
		if (hx == 0)
		{
			for (ix = -1043, i = rawlx; i > 0; i <<= 1)
				ix -= 1;
		} else
		{
			for (ix = -1022, i = (hx << 11); i > 0; i <<= 1)
				ix -= 1;
		}
	} else
	{
		ix = (hx >> 20) - 1023;
	}

	/* determine iy = ilogb(y) */
	if (hy < IC(0x00100000))
	{									/* subnormal y */
		if (hy == 0)
		{
			for (iy = -1043, i = rawly; i > 0; i <<= 1)
				iy -= 1;
		} else
		{
			for (iy = -1022, i = (hy << 11); i > 0; i <<= 1)
				iy -= 1;
		}
	} else
	{
		iy = (hy >> 20) - 1023;
	}

	/* set up {hx,lx}, {hy,ly} and align y to x */
	if (ix >= -1022)
		hx = IC(0x00100000) | (IC(0x000fffff) & hx);
	else
	{									/* subnormal x, shift x to normal */
		n = -1022 - ix;
		if (n <= 31)
		{
			hx = (hx << n) | (lx >> (32 - n));
			lx <<= n;
		} else
		{
			hx = lx << (n - 32);
			lx = 0;
		}
	}
	if (iy >= -1022)
		hy = IC(0x00100000) | (IC(0x000fffff) & hy);
	else
	{									/* subnormal y, shift y to normal */
		n = -1022 - iy;
		if (n <= 31)
		{
			hy = (hy << n) | (ly >> (32 - n));
			ly <<= n;
		} else
		{
			hy = ly << (n - 32);
			ly = 0;
		}
	}

	/* fix point fmod */
	n = ix - iy;
	while (n--)
	{
		hz = hx - hy;
		lz = lx - ly;
		if (lx < ly)
			hz -= 1;
		if (hz < 0)
		{
			hx = hx + hx + (lx >> 31);
			lx = lx + lx;
		} else
		{
			if ((hz | lz) == 0)			/* return sign(x)*0 */
				return Zero[(u32) sx >> 31];
			hx = hz + hz + (lz >> 31);
			lx = lz + lz;
		}
	}
	hz = hx - hy;
	lz = lx - ly;
	if (lx < ly)
		hz -= 1;
	if (hz >= 0)
	{
		hx = hz;
		lx = lz;
	}

	/* convert back to floating value and restore the sign */
	if ((hx | lx) == 0)					/* return sign(x)*0 */
		return Zero[(u32) sx >> 31];
	while (hx < IC(0x00100000))
	{									/* normalize x */
		hx = hx + hx + (lx >> 31);
		lx = lx + lx;
		iy -= 1;
	}
	if (iy >= -1022)
	{									/* normalize output */
		hx = ((hx - IC(0x00100000)) | ((iy + 1023) << 20));
		INSERT_WORDS(x, hx | sx, lx);
	} else
	{									/* subnormal output */
		n = -1022 - iy;
		if (n <= 20)
		{
			lx = (lx >> n) | ((u32) hx << (32 - n));
			hx >>= n;
		} else if (n <= 31)
		{
			lx = (hx << (32 - n)) | (lx >> n);
			hx = sx;
		} else
		{
			lx = hx >> (n - 32);
			hx = sx;
		}
		INSERT_WORDS(x, hx | sx, lx);
		x *= one;						/* create necessary signal */
	}
	return x;							/* exact output */
}
