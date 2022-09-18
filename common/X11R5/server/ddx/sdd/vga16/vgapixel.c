/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgapixel.c	1.3"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

/*
 * vgapixel.c	-- routines to convert between the standard server kit 
 *		pixel format to the VGA format.
 *
 *		A pixel on a color display is represented by more than one
 *		bit.   The two formats used to represent this are:
 *
 *			1)  all the bits in a pixel are contained in one
 *			    nibble, byte, etc.
 *			2)  Memory is divided into "planes", where the 
 *			    first bit in a pixel is in plane 1, the second
 *			    bit is in plane 2, etc.
 *
 *		The server kit describes pixels as bits in a byte.  The
 *		VGA adapters have multiple planes of memory.  A pixel
 *		on the screen is described by stacking together the bits
 *		in a given position in each of the 4 planes.  We represent
 *		this in memory with 4 bit buffers, one for each plane.
 *
 *		The first pixel in a series of server kit pixels is the left
 *		most pixel in the series.  On the VGA, the bits in each
 *		byte are set up so the high order bit in a byte will display
 *		on the left.
 */

#include "Xmd.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "sidep.h"
#include "vga.h"

extern struct at_disp_info vt_info;



/*
 *      vga_sbtovga(src, dst, cnt, pad, pix) -- convert the pixels in src from
 *					server kit format to vga format and
 *					put the result in dst.
 *					NOTE:  this can't be called with 
 *					a depth of 1.
 *
 *	Input:
 *		BYTE	*src		-- pointer to source pixels
 *		BYTE	*dst		-- pointer to buffer for dst pixels
 *		int	cnt		-- number of pixels to convert
 *		int	pad		-- padding to add to each plane
 *              int     pix             -- starting pixel offset
 */
vga_sbtovga(src, dst, cnt, pad, pix)
register BYTE	*src;
BYTE	*dst;
int     cnt, pad, pix;
{
	int	offset;
	int	leftover;
	register int	bit;
	register BYTE srcdat;
	BYTE	p1dat, p2dat;
	BYTE	p3dat, p4dat;
	BYTE	*p1, *p2, *p3, *p4;

	/*
	 * figure out number of bytes per plane, set up pointers to planes
	 */
	leftover = cnt & 7;
	offset = (cnt / 8);			/* get number of bytes needed */
	if (leftover) {
		offset++;			/* need one more (incomplete) */
		leftover = 8 - leftover;	/* shift amount for later */
	}

	bit = 0;
	offset += pad;
	p1dat = p2dat = p3dat = p4dat = 0;

	if (vt_info.planes == 2) {		/* 2 plane display */
		p1 = dst + pad;
		p2 = p1 + offset;
		srcdat = *src++;
		srcdat >>= (2 * pix);
		while (--cnt >= 0) {
			p1dat = (p1dat << 1) + (srcdat & 1); srcdat >>= 1;
			p2dat = (p2dat << 1) + (srcdat & 1); srcdat >>= 1;

			if (++pix > 3) {
				pix = 0;
				srcdat = *src++;
			}

			if (++bit == 8) {	/* hit a byte boundry */
				bit = 0;
				*p1++ = p1dat;		/* set buffers */
				*p2++ = p2dat;
			}
		}
	}

	else {					/* 4 plane display */
		p1 = dst + pad;
		p2 = p1 + offset;
		p3 = p2 + offset;
		p4 = p3 + offset;
		srcdat = *src++;
		if (pix)
		    srcdat >>= 4;

		while (--cnt >= 0) {
			p1dat = (p1dat << 1) + (srcdat & 1); srcdat >>= 1;
			p2dat = (p2dat << 1) + (srcdat & 1); srcdat >>= 1;
			p3dat = (p3dat << 1) + (srcdat & 1); srcdat >>= 1;
			p4dat = (p4dat << 1) + (srcdat & 1); srcdat >>= 1;

			if (!(pix ^= 1))
				srcdat = *src++;

			if (++bit == 8) {	/* hit a byte boundry */
				bit = 0;
				*p1++ = p1dat;		/* set buffers */
				*p2++ = p2dat;
				*p3++ = p3dat;
				*p4++ = p4dat;
			}
		}
	}

	if(leftover) {
		*p1 = p1dat << leftover;              /* store leftover bits */
		*p2 = p2dat << leftover;
		if(vt_info.planes == 4) {
			*p3 = p3dat << leftover;
			*p4 = p4dat << leftover;
		}
	}
}


/*
 *      vga_vgatosb(src, dst, off, cnt, pix) -- convert the pixels in src from
 *                                      vga format to server kit format and
 *                                      put the result in dst.
 *					NOTE:  this can't be called with 
 *					a depth of 1.
 *
 *	Input:
 *		BYTE *src		-- pointer to source pixels
 *		BYTE *dst		-- pointer to buffer for dst pixels
 *              int off                 -- plane offset
 *              int cnt                 -- number of pixels to convert
 *              int pix                 -- pixel offset in first byte
 */
vga_vgatosb(src, dst, off, cnt, pix)
BYTE		*src;
register BYTE 	*dst;
int     off;
int     cnt;
int     pix;
{
	int     firsttime = 1;
	register int bit;
	BYTE	mask;
	BYTE	p1dat, p2dat, p3dat, p4dat;
	BYTE	*p1, *p2, *p3, *p4;
	/*
	 * figure out number of bytes per plane, set up pointers to planes
	 */
	p1 = src;
	p2 = p1 + off;
	if(vt_info.planes >= 4) {
		p3 = p2 + off;
		p4 = p3 + off;
	}

	mask = 0x80;
	pix &= 0x7;         /* must be between 0-7 */
	while (--pix >= 0)  /* set mask to the correct first pixel */
	    mask >>= 1;

	bit = -1;
	while (--cnt >= 0) {
		bit++;
		if(mask == 0x0)  /* reset mask */
			mask = 0x80;

		if ((mask == 0x80) || firsttime) { /* hit a byte boundry */
			p1dat = *p1++;             /* get next source data */
			p2dat = *p2++;
			if(vt_info.planes >= 4) {
				p3dat = *p3++;
				p4dat = *p4++;
			}
			firsttime = 0;
		}

		switch (vt_info.planes) {
		case 2:
			switch (bit & 0x3) {
			case 3:
				if (p1dat & mask) *dst |= 0x40;
				if (p2dat & mask) *dst |= 0x80;
				dst++;
				break;
			case 2:
				if (p1dat & mask) *dst |= 0x10;
				if (p2dat & mask) *dst |= 0x20;
				break;
			case 1:
				if (p1dat & mask) *dst |= 0x4;
				if (p2dat & mask) *dst |= 0x8;
				break;
			case 0:
				*dst = 0;
				if (p1dat & mask) *dst |= 0x1;
				if (p2dat & mask) *dst |= 0x2;
				break;
			}
			break;
		case 4:
			if (bit & 1) {
				if (p1dat & mask) *dst |= 0x10;
				if (p2dat & mask) *dst |= 0x20;
				if (p3dat & mask) *dst |= 0x40;
				if (p4dat & mask) *dst |= 0x80;
				dst++;
			} else {
				*dst = 0;
				if (p1dat & mask) *dst |= 0x1;
				if (p2dat & mask) *dst |= 0x2;
				if (p3dat & mask) *dst |= 0x4;
				if (p4dat & mask) *dst |= 0x8;
			}
			break;
		}
		mask >>= 1;
	}
}



/*
 *	vga_tiletopat(src, dst, w, h)	-- convert the pixels in the src tile
 *					from server kit format to vga pattern
 *					format and put the result in dst.
 *
 *	Input:
 *		BYTE	*src		-- pointer to source pixels
 *		BYTE	*dst		-- pointer to buffer for dst pixels
 *		int	w		-- width of tile
 *		int	h		-- height of tile
 */
vga_tiletopat(src, dst, w, h)
BYTE		*src;
BYTE 		*dst;
register int	w, h;
{
	int		leftover;
	int		bytes_per_line;
	int		hcnt, wcnt;
	BYTE		p1dat, p2dat, p3dat, p4dat, mask;
	BYTE		*p1, *p2, *p3, *p4;
	register BYTE	*psrc;

	if (vt_info.planes == 1) {
		vga_tiletopat1(src, dst, h);
		return;
	}

	/*
	 * figure out number of bytes per plane, set up pointers to planes
	 */
	leftover = w & 7;
	if (leftover) 
		leftover = 8 - leftover;	/* shift amount for later */

	/*
	 * calculate the number of bytes per line in the tile.  This
 	 * is done by figuring the number of bytes to contain the pixels, 
	 * then rounding this up to a 4 byte boundry.
	 */
	if (vt_info.planes == 2) 
		bytes_per_line = (((w + 3) >> 2) + 3) & ~3;
	else
		bytes_per_line = (((w + 1) >> 1) + 3) & ~3;

	for (hcnt = 0; hcnt < h; hcnt++) {
		psrc = src + hcnt * bytes_per_line;
		p1 = dst + (4 * hcnt);
		p2 = p1 + (4 * h);
		if (vt_info.planes >= 4) {
			p3 = p2 + (4 * h);
			p4 = p3 + (4 * h);
		}

		p1dat = p2dat = p3dat = p4dat = 0;

		for (wcnt = 0; wcnt < w;) {
			p1dat <<= 1; p2dat <<= 1; p3dat <<= 1; p4dat <<= 1;
			switch (vt_info.planes) {
			case 2:
				mask = 0x2 << 2*(wcnt & 0x3);
				if (*psrc & (mask >> 1)) p1dat++;
				if (*psrc & mask)        p2dat++;
		
				break;

			case 4:
				mask = 0x8 << 4*(wcnt & 0x1);
				if (*psrc & (mask >> 3)) p1dat++;
				if (*psrc & (mask >> 2)) p2dat++;
				if (*psrc & (mask >> 1)) p3dat++;
				if (*psrc & mask)        p4dat++;
	
				break;
			}

			if (mask == 0x80) psrc++;

			if ((++wcnt & 0x7) == 0) {      /* hit a byte boundry */
				*p1++ = p1dat;          /* set buffers */
				if(vt_info.planes >= 2) /* clear data bytes */
					*p2++ = p2dat;
				if(vt_info.planes >= 4) {
					*p3++ = p3dat;
					*p4++ = p4dat;
				}
				p1dat = p2dat = p3dat = p4dat = 0;
			}
		}

		if(leftover) {
		    *p1 = p1dat << leftover;   /* store leftover bits */
		    if(vt_info.planes >= 2)
			*p2 = p2dat << leftover;
		    if(vt_info.planes >= 4) {
			*p3 = p3dat << leftover;
			*p4 = p4dat << leftover;
		    }
		}
	}
}



/*
 *	vga_tiletopat1(src, dst, h)
 *					-- convert the pixels in the src tile
 *					from server kit format to vga pattern
 *					format and put the result in dst.
 *					This version works when the depth
 *					of the display is 1.  We assume the
 *					tile size is < 32.
 *
 *	Input:
 *		BYTE	*src		-- pointer to source pixels
 *		BYTE	*dst		-- pointer to buffer for dst pixels
 *		int	h		-- height of tile
 */
vga_tiletopat1(src, dst, h)
register BYTE	*src;
register BYTE 	*dst;
register int	h;
{
	while (h--) {
		*dst++ = vga_bitflip[*src++];
		*dst++ = vga_bitflip[*src++];
		src += 2;
		dst += 2;
	}
}



/*	Copyright (c) 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Convert pixmaps from XY format to Z format and vice-versa */

/* WARNING--This code assumes that there are 4 bytes per word,
 * and that the scanline pad unit is words.
 */

#include <memory.h>

static unsigned XYtoZintense4 [256];

static unsigned ZtoXY1_4 [256];

#ifdef MULTIDEPTHS
static unsigned short XYtoZintense2 [256];

static unsigned short ZtoXY1_2 [256];
#endif

static char	initialized = 0;

static void
InitConversionTbls ()
{
	register i;

	for (i=0; i<256; i++)
	{
		/* The table for converting XY data to Z data is indexed
		 * by 8 bits of data from a single plane.  The table entry
		 * is the 8 bits in the intensity positions in Z format.  Data
		 * for the other planes can be found by simply shifting
		 * the intensity data left by the proper number of bits (1-3).
		 * This table is used for 4 bits/pixel data (depth 4).
		 * Since Z format is really an array of bytes (not words), be
		 * careful about byte ordering when writing out words.  Yuck.
		 */

		XYtoZintense4 [i] = (i & 0x80) >> 7 | (i & 0x40) >> 2 |
			(i & 0x20) << 3 | (i & 0x10) << 8 |
			(i & 0x08) << 13 | (i & 0x04) << 18 |
			(i & 0x02) << 23 | (i & 0x01) << 28;

		/* A similar idea is used to 2 bits/pixel data (depth 2),
		 * but now, the table consistes of shorts, not ints.
		 */

#ifdef MULTIDEPTHS
		XYtoZintense2 [i] = (i & 0x80) >> 7 | (i & 0x40) >> 4 |
			(i & 0x20) >> 1 | (i & 0x10) << 2 |
			(i & 0x08) << 5 | (i & 0x04) << 8 |
			(i & 0x02) << 11 | (i & 0x01) << 14;
#endif

		/* The table for converting Z data to XY data is indexed
		 * by 8 bits of Z data (irgbirgb).  The table entry is
		 * the two bits from each plane collected in the low
		 * order bits of a byte in the word.  Thus, the high order
		 * byte describes the I plane, the next byte describes
		 * the R plane, and so forth.  By collecting the bits
		 * of 4 Z format bytes in this way, the result is 8 pixels
		 * of XY data.  Confusing, isn't it?
		 */

		ZtoXY1_4 [i] = (i & 0x80) << 17 | (i & 0x08) << 22 |
			(i & 0x40) << 10 | (i & 0x04) << 15 |
			(i & 0x20) << 3 | (i & 0x02) << 8 |
			(i & 0x10) >> 4 | (i & 0x01) << 1;

		/* A similar notion for depth 2 pixmaps. */

#ifdef MULTIDEPTHS
		ZtoXY1_2 [i] = (i & 0x80) << 1 | (i & 0x20) << 4 |
			(i & 0x08) << 7 | (i & 0x02) << 10 |
			(i & 0x40) >> 6 | (i & 0x10) >> 3 |
			(i & 0x04) | (i & 0x01) << 3;
#endif
	}
}

/* Convert XY data to Z data */

void
vga_xytosb (XYbits, width, height, depth, offset, Zbits)
    register BYTE 	*XYbits;
    int			width, height, depth;
    int			offset;
    register BYTE	*Zbits;
{
	unsigned	Bbyte, Gbyte, Rbyte, Ibyte, Zword;
	int	ZwordCnt;
	int	RplaneOffset, GplaneOffset, BplaneOffset;
	int	i, j;

	if (!initialized)
	{
		initialized = 1;
		InitConversionTbls ();
	}

	RplaneOffset = offset * height;
	GplaneOffset = RplaneOffset * 2;
	BplaneOffset = RplaneOffset * 3;
	ZwordCnt = ((width * depth + 0x1f) & ~0x1f) >> 5;

	switch (depth) {  /* we don't want the switch in the inner loop */

#ifdef MULTIDEPTHS
	case 2:
		for (i=0; i<height; i++)
		{
			for (j=0; j<ZwordCnt; j++)
			{
			    Ibyte = *XYbits;
			    Rbyte = *(XYbits + RplaneOffset);
			    Gbyte = *++XYbits;
			    Bbyte = *(XYbits + RplaneOffset);
			    XYbits++;

			    Zword = (XYtoZintense2 [Ibyte] |
				     XYtoZintense2 [Rbyte] << 1) |
				     (XYtoZintense2 [Gbyte] |
				      XYtoZintense2 [Rbyte] << 1) << 16;

			    *Zbits++ = Zword & 0xff;
			    *Zbits++ = (Zword >> 8) & 0xff;
			    *Zbits++ = (Zword >> 16) & 0xff;
			    *Zbits++ = (Zword >> 24) & 0xff;
			}
		}
		break;
#endif

	case 4:

		for (i=0; i<height; i++)
		{
			for (j=0; j<ZwordCnt; j++)
			{
			    Ibyte = *XYbits;
			    Rbyte = *(XYbits + RplaneOffset);
			    Gbyte = *(XYbits + GplaneOffset);
			    Bbyte = *(XYbits + BplaneOffset);
			    XYbits++;

			    Zword = XYtoZintense4 [Bbyte] << 3 |
				    XYtoZintense4 [Gbyte] << 2 |
				    XYtoZintense4 [Rbyte] << 1 |
				    XYtoZintense4 [Ibyte];

			    *Zbits++ = Zword & 0xff;
			    *Zbits++ = (Zword >> 8) & 0xff;
			    *Zbits++ = (Zword >> 16) & 0xff;
			    *Zbits++ = (Zword >> 24) & 0xff;
			}
		}
		break;

	}	/* end of switch */
}

/* Convert Z data to XY data
 *
 * Both source and destination must be in memory.  srcx must be a multiple
 * of 32.  Don't call this function if the depth is 1.
 */

void
vga_sbtoxy (src, dst, srcx, srcy, width, height)
	register unsigned	*dst;
	int			width, height;
	register SIbitmapP	src;
	int			srcx, srcy;
{
	register BYTE		*pSrc;
	unsigned	XYword, Bword, Gword, Rword, Iword;
	unsigned short	XYshort;
	int		ZwordCnt, byteCnt, lineCnt;
	int		RplaneOffset, GplaneOffset, BplaneOffset;
	int		i, j;

	if (!initialized)
	{
		initialized = 1;
		InitConversionTbls ();
	}

	RplaneOffset = ((width + 31) >> 5) * height;
	GplaneOffset = RplaneOffset * 2;
	BplaneOffset = RplaneOffset * 3;

	switch (src->Bdepth) {
#ifdef MULTIDEPTHS
	case 2:
		ZwordCnt = (width + 15) >> 4;
		lineCnt = ((src->Bwidth + 15) & ~15) >> 2;
		for (i=0; i<height; i++)
		{
			pSrc = (BYTE *) src->Bptr + lineCnt*(srcy+i) +
				(srcx>>2);
			byteCnt = 0;
			for (j=0; j<ZwordCnt; j++)
			{
				XYshort = ZtoXY1_2 [*pSrc++] << 4 |
					ZtoXY1_2 [*pSrc++];

				Bword = Bword << 8 | XYshort & 0xff;
				Gword = Gword << 8 | XYshort >> 8;

				XYshort = ZtoXY1_2 [*pSrc++] << 4 |
					ZtoXY1_2 [*pSrc];

				Bword = Bword << 8 | XYshort & 0xff;
				Gword = Gword << 8 | XYshort >> 8;

				if (++byteCnt > 1)
				{
					byteCnt = 0;
					*dst = Gword;
					*(dst + RplaneOffset) = Bword;
					dst++;
				}
			}

			if (byteCnt != 0)
			{
				*dst = Gword << 16;
				*(dst + RplaneOffset) = Bword << 16;
				dst++;
			}
		}
		break;
#endif

	case 4:
		ZwordCnt = (width + 7) >> 3;
		lineCnt = ((src->Bwidth + 7) & ~7) >> 1;
		for (i=0; i<height; i++)
		{
			pSrc = (BYTE *) src->Bptr + lineCnt*(srcy+i) +
				(srcx>>1);
			byteCnt = 0;
			for (j=0; j<ZwordCnt; j++)
			{
				XYword = ZtoXY1_4 [*pSrc++] << 6 |
					ZtoXY1_4 [*pSrc++] << 4 |
					ZtoXY1_4 [*pSrc++] << 2 |
					ZtoXY1_4 [*pSrc++];

				Bword = Bword << 8 | XYword & 0xff;
				Gword = Gword << 8 | (XYword >> 8) & 0xff;
				Rword = Rword << 8 | (XYword >> 16) & 0xff;
				Iword = Iword << 8 | XYword >> 24;

				if (++byteCnt > 3)
				{
					byteCnt = 0;
					*dst = Iword;
					*(dst + RplaneOffset) = Rword;
					*(dst + GplaneOffset) = Gword;
					*(dst + BplaneOffset) = Bword;
					dst++;
				}
			}

			if (byteCnt != 0)
			{
				byteCnt = (4 - byteCnt) << 3;
				*dst = Iword << byteCnt;
				*(dst + RplaneOffset) = Rword << byteCnt;
				*(dst + GplaneOffset) = Gword << byteCnt;
				*(dst + BplaneOffset) = Bword << byteCnt;
				dst++;
			}
		}
		break;
	}
}
