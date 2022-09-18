/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgafill.c	1.4"

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

static	BYTE *pat_bitmap;
static	BYTE pat_fg;
static	BYTE pat_bg;
extern	SIBool vga_big_tile_setup();
extern	SIBool vga_big_stpl_setup();

/*
 * If we're tiling but have a rasterop of GXset or GXclear, we don't really
 * tile, we just draw with 0 or 1.  We do this by temporarily setting things
 * up as if it's doing a solid fill.  This variables is used to restore
 * the real setting.
 */
static	int  saved_fill_mode;

/*
 * Tile and stipple patterns get set up in a global array that is already
 * alligned as needed based on the pattern's origin.  The pattern is set up
 * as if it's origin were 0, 0 (the upper left corner of the screen.
 * This way it's simple for us to pattern a region because we don't have to
 * do any allignment calculations at fill time.   We also build up the
 * pattern to be exactly 16 bits wide, which makes the code here simpler.
 * The pattern is set up at GS download time.
 *
 * Patterns are 16 bits wide.
 */



/*
 *	vga_fill_rect(cnt, prect) -- draw a series of filled rectangles.
 *				The current fill style, foreground and 
 *				background colors, and current ROP are used.
 *
 *	Input:
 *		int	cnt		-- number of rectangles to fill
 *		SIRectP	prect		-- pointer to list of rectangles
 */
SIBool
vga_fill_rect(cnt, prect)
register int		cnt;
register SIRectP	prect;
{
	DBENTRY("vga_fill_rect()");

	if (vga_start_fill() == SI_FAIL)
		return(SI_FAIL);

	if ((vga_gs->fill_mode == SGFillTile) && (vga_gs->big_tile))
		while (--cnt >= 0)
			vga_big_tile_rect(prect++);
	else if ((vga_gs->fill_mode == SGFillStipple) && (vga_gs->big_stpl))
		while (--cnt >= 0)
			vga_big_stpl_rect(prect++);
	else
		while (--cnt >= 0) {
			if ((prect->lr.x > prect->ul.x) &&
			    (prect->lr.y > prect->ul.y))
				vga_fill_lines(prect->ul.x, prect->lr.x-1,
					       prect->ul.y, 
					       prect->lr.y - prect->ul.y);
			
			prect++;
		}

	vga_finish_fill();
	return(SI_SUCCEED);
}



/*
 *	vga_fill_spans(count, pts, widths)	-- fill a series of scanlines.
 *				The current fill style, foreground and 
 *				background colors, and current ROP are used.
 *
 *	Input:
 *		int		count		-- number of scanlines to fill
 *		SIPointP 	pts		-- list of start points.
 *		int		*widths		-- list of widths to fill
 */
SIBool
vga_fill_spans(count, pts, widths)
register int	count;
register SIPointP pts;
register int	*widths;
{
	DDXPointRec pt1, pt2;

	DBENTRY("vga_fill_spans()");

	if (vga_gs->mode == GXnoop)
		return(SI_SUCCEED);

	if (vga_start_fill() == SI_FAIL)
		return(SI_FAIL);

	if (vga_invertdest) {
		while (count--) {
			if (*widths)
				vga_fill_lines(pts->x, pts->x+*widths-1,
					       pts->y, 1);
			widths++;
			pts++;
		}
	}
	else {
		while (count--) {
			if (!*widths) {
				widths++;
				pts++;
				continue;
			}
			pt1.x = pts->x;
			pt2.x = pts->x + *widths++ - 1;
			pt1.y = pt2.y = pts->y;
			pts++;

			switch(vga_gs->fill_mode) {
			case SGFillSolidFG:
			case SGFillSolidBG:
				vga_line_horiz1(pt1, pt2);
				break;
					
			case SGFillStipple:
				if (vga_gs->big_stpl) {
					vga_big_hline_stpl(pt1.x, pt1.y,
							   pt2.x-pt1.x+1, 1);
					break;
				}
				if (vga_gs->stp_mode == SGOPQStipple) {
					outw(VGA_GRAPH, GR_SR | (pat_bg << 8));
					pat_bitmap = vga_gs->inv_stpl;
					vga_hline_stpl(pt1, pt2, 1);
				}

				outw(VGA_GRAPH, GR_SR | (pat_fg << 8));
				pat_bitmap = cur_pat;
				vga_hline_stpl(pt1, pt2, 1);
				break;

			case SGFillTile:
				if (vga_gs->big_tile) {
					vga_big_hline_tile(pt1.x, pt1.y,
							   pt2.x-pt1.x+1, 1);
					break;
				}
				outw(VGA_GRAPH, GR_ENAB_SR);
				pat_bitmap = cur_pat;
				vga_hline_tile(pt1, pt2, 1);
				outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes); 
				break;
			}
		}
	}

	vga_finish_fill();
	return(SI_SUCCEED);
}



/*
 *	vga_start_fill()	-- do all the setup before a fill operation
 */
vga_start_fill()
{
	register int i, j;
	register BYTE *addr, *s, *p;
	SIbitmapP bmap;
	BYTE	  *pat;
	int	  h, height, width;

	saved_fill_mode = -1;
	if ((vga_gs->fill_mode == SGFillStipple) && (!vga_gs->stpl_valid)) {
		if (vga_gs->big_stpl) {
			if (!vga_gs->raw_stipple.Bptr) {
				if (vga_big_stpl_setup() == SI_FAIL)
					return(SI_FAIL);
				vga_gs->raw_stipple.Bptr = (SIArray)
							   vga_gs->big_stpl;
			}
			return(SI_TRUE);
		}
		bmap = &(vga_gs->raw_stipple);
		pat = vga_gs->stpl;
		height = bmap->Bheight;
		width = bmap->Bwidth;
		if ((width > 16) || (height > 16)) {
			vga_gs->stpl_h = -1;
			return(SI_FAIL);
		}

		vga_byteflip(vga_gs->raw_stpl_data, vga_gs->raw_stpl_data,
			     height * 4, 0);

		memset(pat, 0, VGA_PATBYTES);
		vga_gs->stpl_h = height;
		vga_gs->stpl_one = vga_pat_setup(vga_gs->raw_stpl_data, pat, 
					   bmap->Bwidth, height, 
					   bmap->BorgX, bmap->BorgY);
		if (vga_gs->stpl_one == -1)
			vga_gs->stpl_h = -1;

		/*
		 * now set up the inverted stipple pattern used for
		 * opaque stipple fills.
		 */
		p = vga_gs->inv_stpl;
		s = vga_gs->stpl;
		for (i = 0; i < VGA_PATBYTES; i++)
			*p++ = ~(*s++);

		cur_pat = vga_gs->stpl;
		if ((cur_pat_h = vga_gs->stpl_h) == -1)
			return(SI_FAIL);
		vga_gs->stpl_valid = 1;
	}

	if ((vga_gs->fill_mode == SGFillTile) && (!vga_gs->tile_valid)) {
		if (vga_gs->big_tile) {
			if (!vga_gs->raw_tile.Bptr) {
				if (vga_big_tile_setup() == SI_FAIL)
					return(SI_FAIL);
				vga_gs->raw_tile.Bptr = (SIArray) 
							vga_gs->big_tile;
			}
			return(SI_TRUE);
		}
		bmap = &(vga_gs->raw_tile);
		pat = vga_gs->tile;
		height = bmap->Bheight;
		width = bmap->Bwidth;
		if ((width > 16) || (height > 16)) {
			vga_gs->tile_h = -1;
			return(SI_FAIL);
		}

		memset(vga_slbuf, 0, VGA_PATBYTES*4);
		vga_tiletopat(vga_gs->raw_tile_data, vga_slbuf, width, height);
	
		for (h = 0; h < vt_info.planes*4; h+=4) {
			vga_gs->tile_h = height;
			vga_gs->tile_one=vga_pat_setup(vga_slbuf+(h*height),
					   pat + VGA_PATBYTES*(h>>2), 
					   width, height, 
					   bmap->BorgX, bmap->BorgY);
			if (vga_gs->tile_one == -1)
				vga_gs->tile_h = -1;
		}

		cur_pat = vga_gs->tile;
		if ((cur_pat_h = vga_gs->tile_h) == -1)
			return(SI_FAIL);
		vga_gs->tile_valid = 1;
	}

	if (vga_gs->fill_mode == SGFillStipple) {
		if (vga_invertsrc) {
			pat_fg = ~vga_gs->fg & (vt_info.colors - 1);
			pat_bg = ~vga_gs->bg & (vt_info.colors - 1);
		}
		else {
			pat_fg = vga_gs->fg;
			pat_bg = vga_gs->bg;
		}
		if (vga_gs->mode == GXset) {
			pat_fg = pat_bg = vt_info.colors - 1;
		}
		else if (vga_gs->mode == GXclear) {
			pat_fg = pat_bg = 0;
		}
		pat_fg = vga_color_map[pat_fg];
		pat_bg = vga_color_map[pat_bg];
	}

	if (vga_gs->fill_mode == SGFillTile) {
		if (vga_gs->mode == GXcopy) {		/* set up off screen */
			for (i = 0; i < vt_info.planes; i++) {	/* tile */
				pat_bitmap = cur_pat + (i * VGA_PATBYTES);
				addr = vga_fb + (vt_info.ypix+1) * vga_slbytes;
				outw(VGA_SEQ, MAP_MASK | (vga_write_map[i]<<8));
				for (j = 0; j < cur_pat_h; j++) {
					*addr++ = *pat_bitmap++;
					*addr++ = *pat_bitmap++;
				}
			}
			outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
		}

		else if ((vga_gs->mode == GXclear) || (vga_gs->mode == GXset)) {
			saved_fill_mode = vga_gs->fill_mode;
			vga_gs->fill_mode = SGFillSolidFG;
		}

		/*
		 * If we need to invert the tile, do it now, then invalidate
		 * it when we're done filling.
		 */
		if (vga_invertsrc) {
			p = s = cur_pat;
			for (i = 0; i < VGA_PATBYTES*4; i++)
				*p++ = ~(*s++);
		}

	}

	outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes); /* enable SR reg */
	outw(VGA_GRAPH, GR_SR | (vga_color_map[vga_src] << 8));  
	outw(VGA_GRAPH, GR_FUNC | vga_function);		/* set rop */

	return(SI_SUCCEED);
}



/*
 *	vga_finish_fill()	-- do the last little cleanup after having
 *				filled something.
 */
vga_finish_fill()
{
	outw(VGA_GRAPH, GR_ENAB_SR);			/* disable SR reg */
	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* restore rop */
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);

	/*
	 * Restore the fill mode if we had to save it away previously
	 */
	if (saved_fill_mode != -1)
		vga_gs->fill_mode = saved_fill_mode;

	/*
	 * If we were tile filling and had to invert the tile, it's not
	 * valid anymore.
	 */
	if ((vga_gs->fill_mode == SGFillTile) && vga_invertsrc &&
	    !vga_gs->big_tile)
		vga_gs->tile_valid = 0;
}



/*
 *	vga_fill_lines(xl, xr, y, h,)	-- fill in lines
 *
 *	Input:
 *		int		xl	-- left edge of line to fill
 *		int		xr	-- right edge of line to fill
 *		int		y	-- y position of line to fill
 *		int		h	-- number of lines to draw
 */
vga_fill_lines(xl, xr, y, h)
register int xl, xr, y, h;
{
	DDXPointRec pt1, pt2;
	int y2;

	if ((vga_gs->mode == GXnoop) || (h == 0))
		return(SI_SUCCEED);
	
	y2 = y + h - 1;

	/*
	 * Clip points
	 */
	if ((xl > vga_clip_x2) || (xr < vga_clip_x1) ||
	    (y > vga_clip_y2) || (y2 < vga_clip_y1)) {
		return(SI_SUCCEED);
	}

	if (xl < vga_clip_x1) xl = vga_clip_x1;
	if (xr > vga_clip_x2) xr = vga_clip_x2;
	if (y  < vga_clip_y1) y  = vga_clip_y1;
	if (y2 > vga_clip_y2) y2 = vga_clip_y2;

	pt1.x = xl;
	pt1.y = y;
	pt2.x = xr;
	pt2.y = y;
	h = y2 - y + 1;

	/*
	 * To invert the destination for everything *except* an normal
	 * non-opaque stipple fill, all we need to do is fill the whole
	 * area with the XOR's plane mask.  Stipple fill can't touch the
	 * background pixels, so we do that later.
	 */
	if (vga_invertdest && !((vga_gs->fill_mode == SGFillStipple) && 
	      (vga_gs->stp_mode == SGStipple))) {
		outw(VGA_GRAPH, GR_FUNC | VGA_XOR);	/* drawing mode XOR */
		outw(VGA_GRAPH, GR_SR | vga_gs->pmask);/* reset color */

		vga_line_horiz(pt1, pt2, h);

		if (vga_gs->mode == GXinvert)
			return;

		outw(VGA_GRAPH, GR_SR | (vga_color_map[vga_src] << 8));
		outw(VGA_GRAPH, GR_FUNC | vga_function);/* reset function reg */
	}

	switch(vga_gs->fill_mode) {
	case SGFillSolidFG:
	case SGFillSolidBG:
		vga_line_horiz(pt1, pt2, h);
		break;
			
	case SGFillStipple:
		if ( (cur_pat_h = vga_gs->stpl_h) == 0) {
			return (SI_FAIL);
		}

		if (vga_gs->stp_mode == SGOPQStipple) {
			outw(VGA_GRAPH, GR_SR | (pat_bg << 8));
			pat_bitmap = vga_gs->inv_stpl;
			vga_hline_stpl(pt1, pt2, h);
		}
		else if (vga_invertdest) {
			outw(VGA_GRAPH, GR_FUNC | VGA_XOR);
			outw(VGA_GRAPH, GR_SR | vga_gs->pmask);

			pat_bitmap = cur_pat;
			vga_hline_stpl(pt1, pt2, h);

			if (vga_gs->mode == GXinvert)
				break;

			outw(VGA_GRAPH, GR_FUNC | vga_function);
		}

		outw(VGA_GRAPH, GR_SR | (pat_fg << 8));
		pat_bitmap = cur_pat;
		vga_hline_stpl(pt1, pt2, h);
		break;

	case SGFillTile:
		if ( (cur_pat_h = vga_gs->tile_h) == 0) {
			return (SI_FAIL);
		}

		outw(VGA_GRAPH, GR_ENAB_SR);		/* disable SR reg */
		pat_bitmap = cur_pat;
		vga_hline_tile(pt1, pt2, h);
		outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes); /* enable SR reg */
		break;
	}
}



/*
 *	vga_hline_tile(pt1, pt2, ycnt)	-- draw ycnt horizontal lines from 
 *					pt1 to pt2 using the current pattern
 *					as a tile.  For ycnt > 1, draw each
 *					successive line one scanline down from
 *					the preceeding line.
 *
 *	Input:
 *		DDXPointRec	pt1	-- first point in line
 *		DDXPointRec	pt2	-- last point in line
 *		int		ycnt	-- number of lines to draw
 */
vga_hline_tile(pt1, pt2, ycnt)
DDXPointRec	pt1;
DDXPointRec	pt2;
int		ycnt;
{
	BYTE	startmask, endmask, *paddr;
	int	xcnt, xincr, pat_ycnt, i;
	BYTE	*pat_start;

	startmask = vga_start_bits[pt1.x & 0x7];
	endmask = vga_end_bits[pt2.x & 0x7];
	xcnt = (pt2.x >> 0x3) - (pt1.x >> 0x3);

	/*
	 * Do the first byte
	 */
	if (xcnt == 0) {			/* only one byte to do */
		endmask = endmask & startmask;
		startmask = 0;
	}
	else if (startmask == 0xff)
		startmask = 0;

	if (endmask == 0xff) {
		xcnt++;
		endmask = 0;
	}

	if (startmask) {
		xcnt--;
		outw(VGA_GRAPH, BITMASK | (startmask << 8));
		vga_tile_one(pt1, ycnt);
		pt1.x = (pt1.x + 7) & ~7;
	}

	if (endmask) {
		pt2.x &= ~7;
		outw(VGA_GRAPH, BITMASK | (endmask << 8));
		vga_tile_one(pt2, ycnt);
	}

	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	if ((vga_gs->mode == GXcopy) && xcnt) {
		outw(VGA_GRAPH, gr_mode | 0x100);/* write mode 1 */
		vga_tile_copy(pt1, xcnt, ycnt);
		outw(VGA_GRAPH, gr_mode);
	}
	else if (xcnt) {
		if ((pt1.x >> 3) & 1)
			xincr = -1;
		else
			xincr = 1;

		paddr = vga_fb + vga_byteoffset(pt1.x, pt1.y);
		pat_start = pat_bitmap + (xincr == -1);
		pat_ycnt = cur_pat_h - (pt1.y % cur_pat_h);
			
		for (i = 0; i < vt_info.planes; i++) { /* loop through planes */
			outw(VGA_SEQ, MAP_MASK | (vga_write_map[i] << 8));
			vga_tile_middle(paddr, pat_ycnt, pat_start, 
					xincr, xcnt, ycnt);
			pat_start += VGA_PATBYTES;
		}

		outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
	}
}



/*
 *	vga_tile_one(pt, ycnt)	-- draw ycnt lines for the current tile 
 *				pattern.  One byte per line.
 *
 *	Input:
 *		DDXPointRec	pt		-- first point in line
 *		int		ycnt		-- number of lines to draw
 */
vga_tile_one(pt, ycnt)
DDXPointRec	pt;
int		ycnt;
{
	BYTE	*scrend;
	int	scrincr;
	int	i, j, pat_y;
	int	pat_y_start;
	BYTE	*dst_start, *tile_start;
	register BYTE	*dst, *tile;
	 
	ycnt--;
	scrend = vga_fb + vga_byteoffset(pt.x + 8, pt.y + ycnt);
	scrincr = (vga_slbytes * cur_pat_h);
	pat_y_start = pt.y % cur_pat_h;
	tile_start = cur_pat + (pat_y_start << 1) + ((pt.x >> 3) & 1);
	dst_start = vga_fb + vga_byteoffset(pt.x, pt.y);
	outb(VGA_SEQ, MAP_MASK);

	for (i = 0; i < vt_info.planes; i++) {	/* loop through planes */
		outb(VGA_SEQ+1, vga_write_map[i]);

		pat_y = pat_y_start;
		tile = tile_start;
		dst = dst_start;

		/*
		 * Loop through the lines in the tile.  For each line, draw 
		 * all the the bytes on the screen that come from that line.
		 */
		for (j = 0; j < cur_pat_h; j++) {
			if (dst >= scrend)
				break;

			vga_write1(*tile, dst, scrend, scrincr);
		
			dst += vga_slbytes;
			tile += 2;
			if (++pat_y >= cur_pat_h) {
				pat_y = 0;
				tile = cur_pat + (i * VGA_PATBYTES) +
					((pt.x >> 3) & 1);
			}
		}
		tile_start += VGA_PATBYTES;
	}
}



/*
 *	vga_tile_middle(addr, pat_ycnt, pat_start, xincr, xcnt, ycnt)	
 *			-- draw ycnt bytes for the current tile pattern.  
 *			Xcnt bytes are drawn on each scanline.
 *
 *	Input:
 *		BYTE		*addr		-- first point in line
 *		int		pat_ycnt	-- num of lines before restart
 *		BYTE		*pat_start	-- where to restart the pattern
 *		int		xincr		-- direction to increment X
 *		int		xcnt		-- number of bytes per line
 *		int		ycnt		-- number of lines to draw
 */
vga_tile_middle(addr, pat_ycnt, pat_start, xincr, xcnt, ycnt)
register BYTE	*addr;
int	pat_ycnt;
BYTE	*pat_start;
register int xincr;
int	xcnt;
int	ycnt;
{
	register BYTE 	*pattern, *taddr;
	BYTE	*pat_restart;
	VOLATILEBYTE	t;
	int	j;
	int	xincr_start;
	 
	pat_restart = pat_start+(2*(cur_pat_h-pat_ycnt));
	xincr_start = xincr;
	while (--ycnt >= 0) {
		taddr = addr;
		addr += vga_slbytes;
		pattern = pat_restart;
		xincr = xincr_start;
		j = xcnt;
		while (--j >= 0) {
			t = *taddr;
			*taddr++ = *pattern;
			pattern += xincr;
			xincr *= -1;
		}

		if (--pat_ycnt == 0) {	/* get next pattern */
			pat_ycnt = cur_pat_h;
			pat_restart = pat_start;
		}
		else
			pat_restart += 2;
	}
}



/*
 *	vga_tile_copy(pt, xcnt, ycnt) -- draw ycnt lines for the current 
 *					tile pattern.  Xcnt bytes are drawn on 
 *					each scanline.
 *
 *	Input:
 *		DDXPointRec	pt		-- Upper left point
 *		int		xcnt		-- number of bytes per line
 *		int		ycnt		-- number of lines to draw
 */
vga_tile_copy(pt, xcnt, ycnt)
DDXPointRec	pt;
int		xcnt;
int		ycnt;
{
	BYTE	*scrend;
	int	scrincr;
	int	i, pat_y, off1, off2;
	int	xcnt1, xcnt2;
	register BYTE	*dst, *tile;
	VOLATILEBYTE	t;
	 
	ycnt--;
	scrend = vga_fb + vga_byteoffset(pt.x + (xcnt<<3), pt.y + ycnt);
	scrincr = (vga_slbytes * cur_pat_h);
	pat_y = pt.y % cur_pat_h;
	off1 = (pt.x & 8) >> 3;
	off2 = !off1;
	tile = vga_fb + ((vt_info.ypix+1) * vga_slbytes) + (pat_y<<1);
	dst = vga_fb + vga_byteoffset(pt.x, pt.y);
	xcnt1 = xcnt2 = xcnt >> 1;
	if (xcnt & 1)
		if (off1)
			xcnt2++;
		else
			xcnt1++;

	/*
	 * Loop through the lines in the tile.  For each line, draw all the
	 * the bytes on the screen that come from that line.
	 */
	for (i = 0; i < cur_pat_h; i++) {
		if (dst >= scrend)
			return;

		if (vga_gs->tile_one) {
			t = *tile;			/* latch tile data */
			vga_stosb(dst, scrend, xcnt, scrincr-xcnt);
			tile += 2;
		}
		else {
			t = *tile++;			/* latch tile data */
			if (xcnt1)
				vga_copy_tile(dst + off1, scrend, xcnt1,
					      scrincr - (xcnt1 << 1));
		
			t = *tile++;			/* latch tile data */
			if (xcnt2)
				vga_copy_tile(dst + off2, scrend, xcnt2,
					      scrincr - (xcnt2 << 1));
		}
	
		dst += vga_slbytes;
		if (++pat_y >= cur_pat_h) {
			pat_y = 0;
			tile = vga_fb + ((vt_info.ypix+1) * vga_slbytes);
		}
	}
}



/*
 *	vga_hline_stpl(pt1, pt2, ycnt)	-- draw ycnt horizontal lines from 
 *					pt1 to pt2 using the current fill
 *					style.  For ycnt > 1, draw each 
 *					successive line one scanline down from
 *					the preceeding line.
 *
 *	Input:
 *		DDXPointRec	pt1	-- first point in line
 *		DDXPointRec	pt2	-- last point in line
 *		int		ycnt	-- number of lines to draw
 */
vga_hline_stpl(pt1, pt2, ycnt)
DDXPointRec	pt1;
DDXPointRec	pt2;
int		ycnt;
{
	BYTE startmask, endmask, *paddr;
	int xcnt, xincr_start, pat_ycnt, i, j;
	BYTE *pat_start, *pat_restart;
	register BYTE *taddr, *pattern;
	register int xincr;

	paddr = vga_fb + vga_byteoffset(pt1.x, pt1.y);
	startmask = vga_start_bits[pt1.x & 0x7];
	endmask = vga_end_bits[pt2.x & 0x7];
	xcnt = (pt2.x >> 0x3) - (pt1.x >> 0x3);

	if ((pt1.x >> 3) & 1)
		xincr = -1;
	else
		xincr = 1;
	pat_start = pat_bitmap + (xincr == -1);
		
	/*
	 * Do the first byte
	 */
	if (startmask) {
		if (xcnt == 0)			/* only one byte to do */
			endmask = endmask & startmask;
		else {
			taddr = paddr++;
			pat_ycnt = cur_pat_h - (pt1.y % cur_pat_h);
			pattern = pat_start+(2*(cur_pat_h-pat_ycnt));
			for (i = 0; i < ycnt; i++) {
				outw(VGA_GRAPH, ((startmask & *pattern) << 8) |
				     BITMASK); 
				*taddr |= 1;
				taddr += vga_slbytes;

				if (--pat_ycnt == 0) {	/* get next pattern */
					pat_ycnt = cur_pat_h;
					pattern = pat_start;
				}
				else
					pattern += 2;
			}
			pat_start += xincr;
			xincr *= -1;
			xcnt--;
		}
	}

	/*
	 * Do middle bytes
	 */
	if (xcnt) {
		pat_ycnt = cur_pat_h - (pt1.y % cur_pat_h);
		pat_restart = pat_start+(2*(cur_pat_h-pat_ycnt));
		xincr_start = xincr;
		if (vga_gs->stpl_one) {
			register int cnt;

			for (i = 0; i < ycnt; i++) {
				taddr = paddr + (i * vga_slbytes);
				outw(VGA_GRAPH, BITMASK | (*pat_restart << 8));
				for (cnt = xcnt; cnt; cnt--)
					*taddr++ |= 1;

				if (--pat_ycnt == 0) {	/* get next patten */
					pat_ycnt = cur_pat_h;
					pat_restart = pat_start;
				}
				else
					pat_restart += 2;
			}
		}
		else {
			for (i = 0; i < ycnt; i++) {
				taddr = paddr + (i * vga_slbytes);
				pattern = pat_restart;
				xincr = xincr_start;
				for (j = 0; j < xcnt; j++) {
					outw(VGA_GRAPH,BITMASK|(*pattern << 8));
					*taddr++ |= 1;
					pattern += xincr;
					xincr *= -1;
				}

				if (--pat_ycnt == 0) {	/* get next patten */
					pat_ycnt = cur_pat_h;
					pat_restart = pat_start;
				}
				else
					pat_restart += 2;
			}
		}
	}

	taddr = paddr + xcnt;

	/*
	 * Do final byte
	 */
	pat_start = &pat_bitmap[(pt2.x >> 3) & 1];
	pat_ycnt = cur_pat_h - (pt1.y % cur_pat_h);
	pattern = pat_start+(2*(cur_pat_h-pat_ycnt));
	for (i = 0; i < ycnt; i++) {
		outw(VGA_GRAPH, BITMASK | ((endmask & *pattern) << 8));
		*taddr |= 1;
		taddr += vga_slbytes;

		if (--pat_ycnt == 0) {	/* get next patten */
			pat_ycnt = cur_pat_h;
			pattern = pat_start;
		}
		else
			pattern += 2;
	}
}
