/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgabitblt.c	1.13"

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

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vgaregs.h"
#include "vtio.h"
#include "vga.h"
#include "sys/inline.h"

#define FAST_SAVE_UNDERS 1


/* 
 *	vga_ss_bitblt(sx, sy, dx, dy, w, h)	-- Moves pixels from one screen
 *						position to another using the
 *						ROP from the setdrawmode call.
 *
 *	Input:
 *		int	sx	-- X position (in pixels) of source
 *		int	sy	-- Y position (in pixels) of source
 *		int	dx	-- X position (in pixels) of destination
 *		int	dy	-- Y position (in pixels) of destination
 *		int	w	-- Width (in pixels) of area to move
 *		int	h	-- Height (in pixels) of area to move
 */
SIBool
vga_ss_bitblt(sx, sy, dx, dy, w, h)
int	sx, sy, dx, dy;
int	w, h;
{
	BYTE	dmask1, dmask2;
	int	dstcnt;
	register BYTE	*psrc, *pdst;
	register int	inc;

	DBENTRY("vga_ss_bitblt()");

	if ((w <= 0) || (h <= 0))
		return(SI_SUCCEED);

	/*
	 * Special version for scrolling
	 */
	if ((sx != dx) || (vga_gs->mode != GXcopy)) {
		vga_rop (NullSIbitmap, NullSIbitmap, sx, sy, dx, dy, w, h,
			 vga_gs->mode, vga_gs->pmask>>8);
		outw(VGA_SEQ, MAP_MASK | vga_gs->pmask); /* reset plane mask */
		return (SI_SUCCEED);
	}

	psrc = vga_fb + vga_byteoffset(sx, sy);
	pdst = vga_fb + vga_byteoffset(dx, dy);
	dmask1 = vga_start_bits[dx & 0x7];
	dmask2 = vga_end_bits[(dx+w-1) & 0x7];

	dstcnt = ((sx+w-1) >> 3) - (sx >> 3) + 1;

	if (sy < dy) {				/* copy from bottom to top */
		inc = -vga_slbytes;
		psrc += ((h-1) * vga_slbytes);
		pdst += ((h-1) * vga_slbytes);
	}
	else 					/* copy from top to bottom */
		inc = vga_slbytes;

	vga_aligned_copy(psrc, pdst, dstcnt, h, dmask1, dmask2, inc);

	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */

	return(SI_SUCCEED);
}


/*
 *	vga_ms_bitblt(src, sx, sy, dx, dy, w, h) -- Moves pixels from memory
 *						to the screen using the ROP 
 *						from the setdrawmode call.
 *
 *	Input:
 *		SIbitmapP	src	-- pointer to source data		
 *		int		sx	-- X position (in pixels) of source
 *		int		sy	-- Y position (in pixels) of source
 *		int		dx	-- X position (in pixels) of destination
 *		int		dy	-- Y position (in pixels) of destination
 *		int		w	-- Width (in pixels) of area to move
 *		int		h	-- Height (in pixels) of area to move
 */
SIBool
vga_ms_bitblt(src, sx, sy, dx, dy, w, h)
SIbitmapP 	src;
int		sx, sy, dx, dy;
int		w, h;
{
	char		allocated;
	SIbitmapP	psrc;
	SIbitmap	bogusPixmap;
	int		srcx, srcy, width, bytes;
	BYTE		*buf;
	BYTE		lcl_buf [LCL_BUF_SIZE];

	DBENTRY("vga_ms_bitblt()");

	if ((w == 0) || (h == 0))
		return(SI_SUCCEED);
	/*
	 * check to see if the source pixmap is in the same format as screen
	 *
	 * if (src_pixmap == XY_PIXMAP)
	 *	zzjust copy from XY format pixmap (ie: src) to screen.
	 * else 
	 *	 continue. ie: go through Z->XY conversion....
	 */
	if (src->Btype == XY_PIXMAP) {
		return (vga_XYpix2scr (src, dx, dy, w, h));
	}

	/* Convert the Z format pixmap to XY format */
	allocated = FALSE;
	if (src->Bdepth > 1)
	{
		srcx = sx & 0x1f;
		srcy = 0;
	        width = srcx + w;
		bytes = ((width + 31) >> 5) * vt_info.planes * h;
		bytes <<= 2;
		if (bytes > LCL_BUF_SIZE)
		{
			buf = (BYTE *) malloc (bytes);
			allocated = TRUE;
			if (!buf)
				return (SI_FAIL);
		}
		else
			buf = lcl_buf;

		vga_sbtoxy (src, buf, sx & ~0x1f, sy, width, h);
		bogusPixmap.Bdepth = src->Bdepth;
		bogusPixmap.Bwidth = width;
		bogusPixmap.Bheight = h;
		bogusPixmap.Bptr = (SIArray) buf;
		psrc = &bogusPixmap;
	}
	else
	{
		srcx = sx;
		srcy = sy;
		psrc = src;
	}

	vga_rop (psrc, NullSIbitmap, srcx, srcy, dx, dy, w, h, vga_gs->mode,
		 vga_gs->pmask>>8); 

	if (allocated)
		free (buf);
	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* restore rop */

	return(SI_SUCCEED);
}

/*
 *	vga_sm_bitblt(dst, sx, sy, dx, dy, w, h) -- Moves pixels from the 
 *						screen to memory using the ROP 
 *						from the setdrawmode call.
 *
 *	Input:
 *		SIbitmapP	dst	-- pointer to destination buffer
 *		int		sx	-- X position (in pixels) of source
 *		int		sy	-- Y position (in pixels) of source
 *		int		dx	-- X position (in pixels) of destination
 *		int		dy	-- Y position (in pixels) of destination
 *		int		w	-- Width (in pixels) of area to move
 *		int		h	-- Height (in pixels) of area to move
 */
SIBool
vga_sm_bitblt(dst, sx, sy, dx, dy, w, h)
SIbitmapP	dst;
int		sx, sy, dx, dy;
int		w, h;
{
	BYTE    dmask1;  /* First byte mask */
	BYTE    dmask2;  /* Last byte mask */
	int     shift, byteshift, srccnt, dstcnt;
	int     sftcnt; /* bytes to shift */
	BYTE	*psrc, *pdst;
	int	setflag;
	int     srcinc, dstinc;
	int	byteoffset;
	int	zwidth;

	DBENTRY("vga_sm_bitblt()");

	if ((w == 0) || (h == 0))
		return(SI_SUCCEED);

	/*
	 * check to see if the dest pixmap is in the same format as screen
	 *
	 * if (dest_pixmap == XY_PIXMAP)
	 *	just copy from screen to XY format pixmap (ie: dest).
	 * else 
	 *	 continue. ie: go through XY->Z conversion....
	 */
#ifdef FAST_SAVE_UNDERS
	if (dst->Btype == XY_PIXMAP) {
		return (vga_scr2XYpix (dst, sx, sy, w, h));
	}
#endif

	outw(VGA_GRAPH, GR_FUNC | vga_function);	/* set rop */

	psrc = vga_fb + vga_byteoffset(sx, sy);
	srcinc = vga_slbytes;
	srccnt = ((sx+w-1) >> 3) - (sx >> 3) + 1;

	byteshift = 0;

	switch (dst->Bdepth) {
	case 1:
		dstinc = ((dst->Bwidth + 31) & ~31) >> 3;
		pdst = (BYTE *)dst->Bptr + (dstinc * dy) + (dx >> 3);

		dstcnt= ((dx+w-1) >> 3) - (dx >> 3) + 1;
		sftcnt = srccnt;
		shift =  (sx & 0x7) - (dx & 0x7);

		dmask1 = vga_bitflip[vga_start_bits[dx & 0x7]];
		dmask2 = vga_bitflip[vga_end_bits[(dx+w-1) & 0x7]];

		break;

	case 2:
		dstinc = ((dst->Bwidth + 15) & ~15) >> 2;
		pdst = (BYTE *)dst->Bptr + (dstinc * dy) + (dx >> 2);

		dstcnt = ((dx+w-1) >> 2) - (dx >> 2) + 1;
		sftcnt = (w+(sx&3)+3) >> 2;
		shift = ((sx & 3) - (dx & 3)) << 1;
		byteoffset = (sx & 15) >> 2;
		zwidth = w + (sx & 15);

		dmask1 = vga_bitflip[vga_2_start_bits[dx & 0x3]];
		dmask2 = vga_bitflip[vga_2_end_bits[(dx+w-1) & 0x3]];

		break;

	case 4:
		dstinc = (((dst->Bwidth + 7) & ~7) >> 1);
		pdst = (BYTE *)dst->Bptr + (dstinc * dy) + (dx >> 1);

		dstcnt = ((dx+w-1) >> 1) - (dx >> 1) + 1;
		sftcnt = (w+(sx&1)+1) >> 1;
		shift = ((sx & 1) - (dx & 1)) << 2;
		byteoffset = (sx & 7) >> 1;
		zwidth = w + (sx & 7);

		dmask1 = vga_bitflip[vga_4_start_bits[dx & 0x1]];
		dmask2 = vga_bitflip[vga_4_end_bits[(dx+w-1) & 0x1]];

		break;
	}

	if (shift < 0) {                        /* shift crosses bytes? */
		byteshift = 1;
		shift += 8;
		sftcnt++;
	}

	switch (vga_gs->mode) {
	case GXnoop:
		goto exit_ok;
	case GXclear:
		memset(vga_tmpsl, 0x0, dstcnt);
		setflag = 1;
		break;
	case GXset:
		memset(vga_tmpsl, 0xff, dstcnt);
		setflag = 1;
		break;
	default:
		setflag = 0;
		break;
	}

	if (setflag)
		for (;--h >= 0; pdst+=dstinc)
			vga_sbset(vga_tmpsl, pdst, dstcnt, dmask1, dmask2);

	else if (dst->Bdepth == 1)
		for (;--h >= 0; psrc+=srcinc, pdst+=dstinc) {
			if (vga_invertdest) {
				vga_slsbinvertbm(pdst, dstcnt, dmask1, dmask2);
				if (vga_gs->mode == GXinvert)
					continue;
			}
			memcpy(vga_slbuf+byteshift, psrc, srccnt);
			vga_slshift(vga_slbuf, vga_slbuf, shift,
				    sftcnt, vga_invertsrc);
			vga_byteflip(vga_slbuf, vga_slbuf, dstcnt, 0);
			vga_slsbbltbm(vga_slbuf, pdst, dstcnt, dmask1, dmask2);
		}

	else
		for (;--h >= 0; psrc+=srcinc, pdst+=dstinc) {
			if (vga_invertdest) {
				vga_slsbinvertbm(pdst, dstcnt, dmask1, dmask2);
				if (vga_gs->mode == GXinvert)
					continue;
			}
			vga_slcopyin(psrc, srccnt, 0);
			vga_xytosb(vga_slbuf, zwidth, 1, dst->Bdepth, srccnt,
				   vga_tmpsl+byteshift);
			if (shift || vga_invertsrc)
			    vga_slshiftr(vga_tmpsl+byteoffset,
					 vga_tmpsl+byteoffset, shift,
					sftcnt, vga_invertsrc);
			vga_slsbbltbm(vga_tmpsl+byteoffset, pdst, dstcnt,
				      dmask1, dmask2);
		}

exit_ok:
	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* restore rop */

	return(SI_SUCCEED);
}

vga_scr2XYpix (dst, sx, sy, w, h)
SIbitmapP	dst;
int		sx, sy;
int		w, h;
{
	register BYTE	*psrc, *pdst, *from;
	int     cnt, i, j;			/* number of bytes to copy */

	dst->BorgX = sx;	/* save the current sx and sy */
	dst->BorgY = sy;	/* save the current sx and sy */

	psrc = vga_fb + vga_byteoffset(sx, sy);	/* start of source pixels */
	pdst = (BYTE *)dst->Bptr;		/* pointer to pixmap */
	cnt = ((sx+w-1) >> 3) - (sx >> 3) + 1;

	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* set rop to copy*/

	/* loop through planes */
	for (i = 0; i < vt_info.planes; i++) {
	    outw(VGA_GRAPH, (READ_MASK) | (vga_read_map[i] << 8));
	    from = psrc;
	    j = h;
	    for (;--j >= 0; from+=vga_slbytes, pdst+=cnt)
		memcpy(pdst, from, cnt);
	}

	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* reset rop to copy*/

	return(SI_SUCCEED);
}

vga_XYpix2scr (src, dx, dy, w, h)
SIbitmapP 	src;
int		dx, dy;
int		w, h;
{
	BYTE    lmask, rmask;
	BYTE	*psrc, *pdst;
	register int	cnt;
	register BYTE	*from, *to;
	int	srccnt, dstcnt;
	int     srcpoffset, i; 	/* src plane offset */
	VOLATILEBYTE t;

	dstcnt = ((dx+w-1) >> 3) - (dx >> 3) + 1;
	srccnt = ((src->BorgX + src->Bwidth-1) >> 3) - (src->BorgX >> 3) + 1;
	pdst = vga_fb + vga_byteoffset(dx, dy);
	psrc = (BYTE *)src->Bptr 		/* start of the pixmap */
		+ ((dy - src->BorgY) * srccnt) /* offset by num of lines*/
		+ ((dx>>3) - ((src->BorgX)>>3)); /* offset into the line */ 
	srcpoffset = srccnt * src->Bheight;
	lmask = vga_start_bits[dx & 0x7];
	rmask = vga_end_bits[(dx+w-1) & 0x7];

	if (--dstcnt == 0) {
		rmask = rmask & lmask;
		lmask = 0;
	}

	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* set rop to copy*/

	/* loop through number of lines */
	for (;--h >= 0; psrc+=srccnt, pdst+=vga_slbytes) {

	   /* loop through planes */
	   for (i = 0; i < vt_info.planes; i++) {
		outw(VGA_SEQ, MAP_MASK | (vga_write_map[i] << 8));
		to = pdst;
		cnt = dstcnt; 
		from = psrc + (i*srcpoffset);

		if (lmask)  {			/* first byte */
			outw(VGA_GRAPH, BITMASK | (lmask<<8));
			t = *to;
			*to++ = *from++;
			cnt--;
		}
		
		outw(VGA_GRAPH, 0xff00 | BITMASK);	/* middle bytes */
		while (--cnt >= 0)
			*to++ = *from++;

		outw(VGA_GRAPH, BITMASK | (rmask<<8));	/* last byte */
		t = *to;
		*to = *from;
	   }
	}

	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* reset rop to copy*/

	return(SI_SUCCEED);
}
