/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256tile.c	1.2"

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
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include "v256spreq.h"

extern BYTE v256_pat_fg;
extern BYTE v256_pat_bg;

extern unsigned long v256_expand[];

v256_hline_tile(pt1, pt2, ycnt)
DDXPointRec	pt1;
DDXPointRec	pt2;
int		ycnt;
{
	register int scroff, scrend, scrincr;
	int xcnt, i, length;
	int y;
	int sw, sh, sx, sy;
	BYTE *tile, *dst;
	unsigned long mask;
	int src_incr;

	sw = v256_gs->raw_tile.Bwidth;
	sh = v256_gs->raw_tile.Bheight;
	sx = (pt1.x - v256_gs->raw_tile.BorgX) % sw;
	sy = (pt1.y - v256_gs->raw_tile.BorgY) % sh;
	if (sx < 0) sx += sw;
	if (sy < 0) sy += sh;
	
	scrend = pt2.x + v256_slbytes * (pt1.y + ycnt - 1);
	scrincr = v256_slbytes * sh;
	xcnt = pt2.x - pt1.x + 1;
	mask = v256_expand[v256_gs->pmask];

	/*
	 * Loop through the lines in the tile.  For each line, expand
	 * the tile into a temporary buffer big enough to tile the width
	 * needed, then tile that buffer onto each scanline on the screen
	 * that would receive that part of the pattern.
	 */
	src_incr = (sw + 3) & ~3;
	tile = v256_gs->raw_tile_data + (sy * src_incr);
	for (i = 0; i < sh; i++) {
		scroff = pt1.x + v256_slbytes * pt1.y;
		if (scroff > scrend)
			return;

		/*
		 * If we're tiling an area that's smaller than the tile,
		 * use the tile directly.  Otherwise, expand the tile
		 * into the scanline buffer and tile from there.
		 */
		if ((sw-sx) >= xcnt)
			dst = tile+sx;
		else {
			dst = v256_slbuf;
			memcpy(dst, tile+sx, sw - sx);
			dst += (sw - sx);
			if (sx) {
				memcpy(dst, tile, sx);
				dst += sx;
			}

			length = xcnt - sw;
			while (length > 0) {
				memcpy(dst, v256_slbuf, sw);
				dst += sw;
				length -= sw;
			}
			dst = v256_slbuf;
		}

		while (scroff <= scrend) {
			vidwrite(scroff, dst, xcnt, mask);
			scroff += scrincr;
		}

		pt1.y++;
		tile += src_incr;
		if (++sy >= sh) {
			sy = 0;
			tile = v256_gs->raw_tile_data;
		}
	}
}

/*
 * modification of the previous routine to do fast tiling.
 * Assumptions : rectangle to be tiled lies in one vga page and 
 * select page has already been done.
 */
v256_fast_hline_tile(VgaRegion	*region_p)
{
	/*
	 * functions external to this file
	 */
	extern 	int	v256_fast_vidwrite();

	/*
	 * temps
	 */
	int		i,j;

	/* 
	 * length of one line of the source tile
	 */
	int 	src_incr;

	/*
	 * length of one line of the destination pixmap in bytes and a temp
	 */
	int 	xcnt, length;

	/*
	 * width and height of the source tile
	 */
	int 	sw, sh, sx, sy;

	/*
	 * expanded planemask,pointer to source tile 
	 */
	unsigned long expanded_mask;
	BYTE 	*tile; 

	/*
	 * temp scanline and pointer to temp scanline
	 */
	BYTE	*dst_p;
	BITS32 	temp_scanline[MAXSCANLINE/4];

	/*
	 * descriptors of the destination region on screen to be tiled
	 */
	register int	x_top_left,y_top_left;
	int				x_bottom_right,y_bottom_right;

	/*
	 * offset of the current line in the destination region
	 */
	int		dest_offset;

	/*
	 * get the destination region bounds
	 */
	x_top_left = region_p->x;
	y_top_left = region_p->y;
	x_bottom_right = x_top_left + region_p->width - 1;
	y_bottom_right = y_top_left + region_p->height - 1;

	/*
	 * compute the bounds of the source tile
	 */
	sw = v256_gs->raw_tile.Bwidth;
	sh = v256_gs->raw_tile.Bheight;
	sx = (x_top_left - v256_gs->raw_tile.BorgX) % sw;
	sy = (y_top_left - v256_gs->raw_tile.BorgY) % sh;
	if (sx < 0) sx += sw;
	if (sy < 0) sy += sh;
	
	expanded_mask = v256_expand[v256_gs->pmask];
	xcnt = x_bottom_right - x_top_left + 1;
	src_incr = (sw + 3) & ~3;
	tile = v256_gs->raw_tile_data + (sy * src_incr);

	/*
	 * Loop through the lines in the tile.  For each line, expand
	 * the tile into a temporary buffer big enough to tile the width
	 * needed, then tile that buffer onto each scanline on the screen
	 * that would receive that part of the pattern.
	 */
	for (i = 0; i < sh; i++) 
	{

		/*
		 * check  if we are out of bounds
		 */
		if ( y_top_left > y_bottom_right )
		{
			break;
		}

		/*
		 * copy the current line of the tile into the temp buffer
		 * If we're tiling an area that's smaller than the tile,
		 * use the tile directly.  Otherwise, expand the tile
		 * into the scanline buffer and tile from there.
		 */
		if ((sw-sx) >= xcnt)
		{
			dst_p = tile+sx;
		}
		else 
		{
			dst_p = (BYTE *)temp_scanline;

			memcpy(dst_p, tile+sx, sw - sx);
			dst_p += (sw - sx);

			if (sx) 
			{
				memcpy(dst_p, tile, sx);
				dst_p += sx;
			}
			length = xcnt - sw;

			while (length > 0) 
			{
				memcpy(dst_p, temp_scanline, sw);
				dst_p += sw;
				length -= sw;
			}
			dst_p = (BYTE *)temp_scanline;
		}

		for ( j = y_top_left; j <= y_bottom_right; j+= sh)
		{
			dest_offset = OFFSET(x_top_left,j) & VIDEO_PAGE_MASK;
			v256_fast_vidwrite(dest_offset,dst_p,xcnt,expanded_mask);
		}

		/*
		 * increment to the next line of the source tile and destination
		 */
		tile += src_incr;
		y_top_left++;

		if (++sy >= sh) 
		{
			sy = 0;
			tile = v256_gs->raw_tile_data;
		}
	}  /*  end of for (i = 0; i < sh; i++) */
}

#define MAX_TILE_WIDTH 32


/*
 *	v256_tile_setup()	-- Set up a tile for later use.
 */
v256_tile_setup()
{
	int		expand;
	register int	w;
	int		h, i, j;
	int		srcinc, dstinc;
	BYTE		*dst;
	register BYTE	*src, *pdst;

	w = v256_gs->raw_tile.Bwidth;
	h = v256_gs->raw_tile.Bheight;
	expand = (MAX_TILE_WIDTH + w - 1) / w;
	if (expand == 1)
		return(SI_TRUE);
	srcinc = (w + 3) & ~3;
	dstinc = ((w * expand) + 3) & ~3;
	src = v256_gs->raw_tile_data;
	if ((dst = (BYTE *)malloc(dstinc * h)) == NULL)
		return(SI_TRUE);
	
	for (i = 0; i < h; i++) {
		pdst = dst + (dstinc * i);
		for (j = 0; j < expand; j++) {
			bcopy(src, pdst, w);
			pdst += w;
		}
		src += srcinc;
	}

	free(v256_gs->raw_tile_data);
	v256_gs->raw_tile_data = dst;
	v256_gs->raw_tile.Bwidth *= expand;
	return(SI_TRUE);
}
