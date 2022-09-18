/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgabigtile.c	1.3"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
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

/*
 * The internal format for large tile patterns is planar.  Each Z-format
 * scanline of data passed from the SBI is broken into (depth) scanlines
 * of planar data.  They are arranged so that each scanline's worth of
 * planes are together, rather than each planes worth of scanlines.
 * There is one byte of padding at the beginning of each scanline (used
 * later for shifting if needed) and no other padding between scanlines.
 */

#define MAX_TILE_WIDTH 128
#define MAX_TILE_HEIGHT 16


/*
 *	vga_big_tile_setup()	-- convert a large tile pattern into
 *				internal (sort of XY) format so later 
 *				filling doesn't have to do it.  Also, 
 *				expand the tile if it's width is a 
 *				multiple of 8 (since it's easy and can
 *				speed things up quite a bit.)  Also, 
 *				expand the tile's height to be at least
 *				MAX_TILE_HEIGHT.
 */
SIBool
vga_big_tile_setup()
{
	SIbitmapP 	bmap;
	BYTE		*src, *dst, *pdst;
	int		srcinc, dstinc, i, j, h, expand, bytewidth, dstoffset;
	extern		BYTE *malloc();

	if (vt_info.planes == 1)
		return(SI_SUCCEED);

	bmap = &(vga_gs->raw_tile);
	srcinc = (((bmap->Bwidth * bmap->Bdepth) + 31) & ~31) >> 3;
	dstinc = (((bmap->Bwidth + 7) >> 3) + 1) * vt_info.planes;
	src = vga_gs->big_tile;
	pdst = dst = malloc(bmap->Bheight * dstinc);
	if (!dst)
		return(SI_FAIL);
	
	for (i = 0; i < bmap->Bheight; i++) {
		vga_sbtovga(src, pdst, bmap->Bwidth, 1, 0);
		src += srcinc;
		pdst += dstinc;
	}

	free(vga_gs->big_tile);
	vga_gs->big_tile = dst;

	/*
	 * if the pattern is already large, or it's not a multiple of
	 * 8 wide, don't do anything.
	 */
	if ((bmap->Bwidth >= MAX_TILE_WIDTH) || (bmap->Bwidth & 0x7))
		return(SI_SUCCEED);

	/*
	 * expand the pattern to a width >= MAX_TILE_WIDTH
	 */
	expand = (MAX_TILE_WIDTH + bmap->Bwidth - 1) / bmap->Bwidth;
	srcinc = ((bmap->Bwidth + 7) >> 3) + 1;
	dstinc = (((expand * bmap->Bwidth) + 7) >> 3) + 1;
	src = vga_gs->big_tile+1;
	h = bmap->Bheight * vt_info.planes;
	dst = malloc(dstinc * h);
	if (!dst)
		return(SI_FAIL);
	
	bytewidth = bmap->Bwidth >> 3;
	for (i = 0; i < h; i++) {
		pdst = dst + (dstinc * i) + 1;
		for (j = 0; j < expand; j++) {
			bcopy(src, pdst, bytewidth);
			pdst += bytewidth;
		}
		src += srcinc;
	}

	free(vga_gs->big_tile);
	vga_gs->big_tile = dst;
	bmap->Bwidth *= expand;

	/*
	 * if the pattern is already large, don't do anything.
	 */
	if (bmap->Bheight >= MAX_TILE_HEIGHT)
		return(SI_SUCCEED);

	/*
	 * expand the pattern to a height >= MAX_TILE_HEIGHT
	 */
	expand = (MAX_TILE_HEIGHT + bmap->Bheight - 1) / bmap->Bheight;
	dstinc = srcinc = ((bmap->Bwidth + 7) >> 3) + 1;
	src = vga_gs->big_tile+1;
	h = bmap->Bheight * vt_info.planes;
	dstoffset = h * dstinc;
	dst = malloc(expand * dstinc * h);
	if (!dst)
		return(SI_FAIL);
	
	bytewidth = bmap->Bwidth >> 3;
	for (i = 0; i < h; i++) {
		pdst = dst + (dstinc * i) + 1;
		for (j = 0; j < expand; j++) {
			bcopy(src, pdst, bytewidth);
			pdst += dstoffset;
		}
		src += srcinc;
	}

	free(vga_gs->big_tile);
	vga_gs->big_tile = dst;
	bmap->Bheight *= expand;
	return(SI_SUCCEED);
}
	


/*
 * 	vga_big_tile_rect(prect)	-- fill a rectangle with a large
 *						tile.
 *
 *	Input:
 *		SIRectP	prect		-- pointer to rectangle to fill
 */
SIBool
vga_big_tile_rect(prect)
SIRectP prect;
{
	int sh, sy, h, ycnt, xcnt;
	int x1, y1, x2, y2;
	extern SIBool vga_big_tile_rect_copy ();

	x1 = prect->ul.x;
	y1 = prect->ul.y;
	x2 = prect->lr.x - 1;
	y2 = prect->lr.y - 1;

	if ((x1 > x2) || (y1 > y2))
		return(SI_SUCCEED);
		
	/*
	 * Clip points
	 */
	if ((x1 > vga_clip_x2) || (x2 < vga_clip_x1) ||
	    (y1 > vga_clip_y2) || (y2 < vga_clip_y1)) {
		return(SI_SUCCEED);
	}

	if (x1 < vga_clip_x1) x1 = vga_clip_x1;
	if (x2 > vga_clip_x2) x2 = vga_clip_x2;
	if (y1 < vga_clip_y1) y1 = vga_clip_y1;
	if (y2 > vga_clip_y2) y2 = vga_clip_y2;

	if (vga_gs->mode == GXcopy) {
		vga_big_tile_rect_copy(x1, y1, x2, y2);
		return;
	}

	ycnt = y2 - y1 + 1;
	xcnt = x2 - x1 + 1;
	sh = vga_gs->raw_tile.Bheight;
	sy = (y1 - vga_gs->raw_tile.BorgY) % sh;
	if (sy < 0) sy += sh;

	/*
	 * do lines that lead up to the full tile
	 */
	h = sh - sy;
	if (h > ycnt)
		h = ycnt;

	vga_big_hline_tile(x1, y1, xcnt, h);
	ycnt -= h;
	y1 += h;

	/*
	 * do the middle (full) tile
	 */
	while (ycnt > sh) {
		vga_big_hline_tile(x1, y1, xcnt, sh);
		ycnt -= sh;
		y1 += sh;
	}

	/*
	 * do final lines
	 */
	if (ycnt)
		vga_big_hline_tile(x1, y1, xcnt, ycnt);
}



/*
 * 	vga_big_tile_rect_copy(x1, y1, x2, y2)	-- fill a rectangle with a 
 *						large tile. (Rasterop = copy)
 *						This draws an entire row
 *						of the tile pattern then
 *						does a screen to screen blit
 *						for drawing the rest.
 *
 *	Input:
 *	
 *		int	x1		-- x position of upper left corner
 *		int	y1		-- y position of upper left corner
 *		int	x2		-- x position of lower right corner
 *		int	y2		-- y position of lower right corner
 */
SIBool
vga_big_tile_rect_copy(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
	int sh, sy, h, ycnt, xcnt;

	ycnt = y2 - y1 + 1;
	xcnt = x2 - x1 + 1;
	sh = vga_gs->raw_tile.Bheight;
	sy = (y1 - vga_gs->raw_tile.BorgY) % sh;
	if (sy < 0) sy += sh;

	/*
	 * do lines that lead up to the full tile
	 */
	h = sh - sy;
	if (h > ycnt)
		h = ycnt;

	vga_big_hline_tile(x1, y1, xcnt, h);
	ycnt -= h;
	y1 += h;

	/*
	 * If we're filling a large area, fill in enough so we have
	 * a full tile height on the screen, then use ss_bitblt to
	 * draw the rest.
	 */
	if (ycnt) {
		h = sh - h;
		if (h > ycnt)
			h = ycnt;

		vga_big_hline_tile(x1, y1, xcnt, h);
		ycnt -= h;
		y1 += h;

		/*
		 * do the middle (full) tile.  Notice we double the
		 * copy amount each time.
		 */
		while (ycnt > sh) {
			vga_ss_bitblt(x1, y1-sh, x1, y1, xcnt, sh);
			ycnt -= sh;
			y1 += sh;
			sh += sh;
		}

		/*
		 * do final lines
		 */
		if (ycnt)
			vga_ss_bitblt(x1, y1-sh, x1, y1, xcnt, ycnt);
	}
}



/*
 *	vga_big_hline_tile(x, y, xcnt, ycnt)	-- draw xcnt pixels 
 *				horizontally starting at (x, y) using the
 *				current tile pattern.  This works by
 *				blitting the scanlines from the downloaded 
 *				stipple onto the line being filled using
 *				the vga_big_tileblt() routine.  For ycnt > 1, 
 *				blit multiple lines.  This does no check
 *				to make sure there are ycnt lines available 
 *				to be blitted.  Caller beware.
 *
 *	Input:
 *		int		x	-- x position of starting point
 *		int		y	-- y position of starting point
 *		int		xcnt	-- number of pixels to draw
 *		int		ycnt	-- number of lines to draw
 */
vga_big_hline_tile(x, y, xcnt, ycnt)
int		x, y;
register int	xcnt;
int		ycnt;
{
	register int	sw;
	int		sh, sx, sy;
	int		w;
	extern SIBool	vga_big_tileblt ();
	
	sw = vga_gs->raw_tile.Bwidth;
	sh = vga_gs->raw_tile.Bheight;
	sx = (x - vga_gs->raw_tile.BorgX) % sw;
	sy = (y - vga_gs->raw_tile.BorgY) % sh;
	if (sx < 0) sx += sw;
	if (sy < 0) sy += sh;

	/* 
	 * Do leading pixels up to the start of the tile
	 */
	w = sw - sx;				/* starting blit width */
	if (w > xcnt)
		w = xcnt;

	vga_big_tileblt(&(vga_gs->raw_tile), sx, sy, x, y, w, ycnt);
	xcnt -= w;
	if (!xcnt)				/* narrow stipple fill */
		return;
	x += w;

	/*
	 * Do the middle (full) blits
	 */
	while (xcnt > sw) {
		vga_big_tileblt(&(vga_gs->raw_tile), 0, sy, x, y, sw, ycnt);
		xcnt -= sw;
		x += sw;
	}

	/*
	 * Do the last (partial) blit
	 */
	vga_big_tileblt(&(vga_gs->raw_tile), 0, sy, x, y, xcnt, ycnt);
}



/*
 *	vga_big_tileblt(src, sx, sy, dx, dy, w, h) -- Blit from a big tile
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
vga_big_tileblt(src, sx, sy, dx, dy, w, h)
SIbitmapP 	src;
int		sx, sy, dx, dy;
int		w, h;
{
	BYTE    dmask1, dmask2, srcend;
	int	shift, byteshift, srccnt, dstcnt;
	BYTE	*psrc, *pdst;
	int	setflag;
	int     srcinc, dstinc;

	DBENTRY("vga_big_tileblt()");

	if ((w == 0) || (h == 0))
		return(SI_SUCCEED);

	outw(VGA_GRAPH, GR_FUNC | vga_function);	/* set rop */

	/*
	 * Depth 1 leaves the tile unchanged, so it's padded to 32 bits.
	 * Depth 2 and 4 convert the tile to an internal form with one
	 * byte of padding at the beginning of each plane for shifting.
	 */
	switch (src->Bdepth) {
	case 1:
		srcinc = ((src->Bwidth + 31) & ~31) >> 3;
		psrc = (BYTE *)src->Bptr + (srcinc * sy) + (sx >> 3);
		break;

	case 2:
	case 4:
		srcinc = (((src->Bwidth + 7)  >> 3) + 1) * vt_info.planes;
		psrc = (BYTE *)src->Bptr + (srcinc * sy) + (sx >> 3);
		break;
	}

	pdst = vga_fb + vga_byteoffset(dx, dy);
	dmask1 = vga_start_bits[dx & 0x7];
	dmask2 = vga_end_bits[(dx+w-1) & 0x7];

	srcend = (sx+w-1) & 0x7;
	dstinc = vga_slbytes;
	dstcnt = srccnt = ((sx+w-1) >> 3) - (sx >> 3) + 1;

	byteshift = 0;
	shift = (sx & 0x7) - (dx & 0x7);
	if (shift < 0) {                        /* shift crosses bytes? */
		byteshift = 1;
		shift += 8;
		dstcnt++;
	}

	if (shift > (int)srcend)                     /* clear the last byte? */
		dstcnt--;

	if (vga_invertdest)
		memset(vga_tmpsl, 0xff, dstcnt);

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
			vga_slset(vga_tmpsl, pdst, dstcnt, dmask1, dmask2);

	else if (src->Bdepth == 1)
		for (;--h >= 0; psrc+=srcinc, pdst+=dstinc) {
			vga_byteflip(psrc, vga_slbuf, srccnt, byteshift);
			vga_slshift(vga_slbuf, vga_slbuf, shift, 
				    srccnt+byteshift, vga_invertsrc);
			if (vga_invertdest) {
				outw(VGA_GRAPH, GR_FUNC | VGA_XOR);
				vga_slset(vga_tmpsl,pdst,dstcnt,dmask1,dmask2);

				if (vga_gs->mode == GXinvert)
					continue;
				outw(VGA_GRAPH, GR_FUNC | vga_function);
			}

			vga_slset(vga_slbuf, pdst, dstcnt, dmask1, dmask2);
		}

	else if (vga_gs->mode == GXcopy) {
		if (!byteshift)
			psrc++;			/* skip over padding */
		if (h == 1)
			vga_shift_copy(psrc, pdst, srcinc/vt_info.planes,
				       dstcnt, dmask1, dmask2, shift);
		else
			vga_big_tile_copy(psrc, pdst, h, srcinc, dstinc, 
					  dstcnt, dmask1, dmask2, shift);
	}
	else {
		srccnt = srcinc / vt_info.planes;
		if (!byteshift)
			psrc++;			/* skip over padding */
		for (;--h >= 0; psrc+=srcinc, pdst+=dstinc) {
			vga_slshift(psrc, vga_slbuf, shift, 
				    srcinc, vga_invertsrc);
			if (vga_invertdest) {
				outw(VGA_GRAPH, GR_FUNC | VGA_XOR);
				vga_slset(vga_tmpsl,pdst,dstcnt,dmask1,dmask2);

				if (vga_gs->mode == GXinvert)
					continue;
				outw(VGA_GRAPH, GR_FUNC | vga_function);
			}
			vga_slcopyout(pdst, srccnt, dstcnt, dmask1, dmask2);
		}
	}

exit_ok:
	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* restore rop */

	return(SI_SUCCEED);
}



/*
 *	vga_big_tile_copy(psrc, pdst, h, srcinc, dstinc, dstcnt, 
 *			  dmask1, dmask2, shift); 
 *
 *				Tile an area using a "big" tile when
 *				the rasterop is GXcopy.
 *
 *	Input:
 *		BYTE		*psrc	-- pointer so source bits
 *		BYTE		*pdst	-- pointer so destination bits
 *		int		h	-- Height (in pixels) of area to tile
 *		int		srcinc	-- increment between source scanlines
 *		int		dstinc	-- increment between dest scanlines
 *		int		dstcnt	-- number of bytes per scanline
 *		BYTE		dmask1	-- starting mask
 *		BYTE		dmask2	-- ending mask
 *		int		shift	-- amount to shift while writing
 */
vga_big_tile_copy(psrc, pdst, h, srcinc, dstinc, dstcnt, dmask1, dmask2, shift)
register BYTE	*psrc, *pdst;
int		h;
int		srcinc, dstinc, dstcnt;
BYTE		dmask1, dmask2;
int		shift;
{
	int srccnt = srcinc / vt_info.planes;
	register int i;

	dstcnt--;
	if (dmask1 != 0xff) 			/* do the first byte */
		if (dstcnt == 0)		/* only one byte to do */
			dmask2 = dmask2 & dmask1;
		else {
			outw(VGA_GRAPH, BITMASK | (dmask1 << 8));
			for (i = 0; i < vt_info.planes; i++) { 
				outw(VGA_SEQ, MAP_MASK | (vga_write_map[i]<<8));
				vga_shift_out1(psrc+(srccnt*i), pdst, 
					       srcinc, h, shift);
			}
			psrc++;
			pdst++;
			dstcnt--;
		}

	if (dmask2 == 0xff)
		dstcnt++;
	else {
		outw(VGA_GRAPH, BITMASK | (dmask2 << 8));
		for (i = 0; i < vt_info.planes; i++) { 
			outw(VGA_SEQ, MAP_MASK | (vga_write_map[i]<<8));
			vga_shift_out1(psrc+dstcnt+(srccnt*i), pdst+dstcnt,
				       srcinc,h,shift);
		}
	}

	if (dstcnt) {
		outw(VGA_GRAPH, BITMASK | 0xff00);
		for (i = 0; i < vt_info.planes; i++) { /* loop through planes */
			outw(VGA_SEQ, MAP_MASK | (vga_write_map[i] << 8));
			vga_shift_outn(psrc+(srccnt*i), pdst, 
				       srcinc, dstcnt, h, shift);
		}
	}
}
