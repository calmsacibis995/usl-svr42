/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgacurs.c	1.6"

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
 *	The term "blotted" will refer to the area that a cursor "blots" out.
 */
static	vga_cursor current_cur;			/* current cursor */
static	current_cur_index = -1;			/* current cursor index */
static	int	vga_curs_x = -1;		/* current x position */
static	int	vga_curs_y = -1;		/* current y position */
static	int	current_cur_sx;			/* source x offset */
static	int	current_cur_sy;			/* source y offset */
BYTE		*vga_blotted_addr;		/* blotted screen position */
static	BYTE	*vga_blotted_mem;		/* data that gets blotted */
static	int	blotted_size = 0;		/* number of bytes blotted */
static	int	blotted_h;			/* number of lines blotted */
static	int	blotted_w;			/* number of bytes blotted */



/*
 *	vga_hcurs_download(index, cp) 	-- download a cursor.  The bitmaps
 *					for the cursor get translated to
 *					the VGA screen format, and a byte
 *					of padding is inserted at the beginning
 *					for shifting room when needed.
 *
 *	Input:
 *		int		index	-- index of cursor being downloaded
 *		SICursorP	cp	-- pointer to new cursor structure
 */
int
vga_hcurs_download(index, cp)
int		index;
SICursorP	cp;
{
	register BYTE *src_mask, *src_face;
	int w, h, i, size;
	extern    SIBool  vga_hcurs_turnon();

	DBENTRY2("vga_hcurs_download()");

	if (vga_curs_on && (current_cur_index == index))
		vga_restore_blotted();

	vga_cursors[index].fg = vga_color_map[cp->SCfg];
	vga_cursors[index].bg = vga_color_map[cp->SCbg];
	vga_cursors[index].w = w = cp->SCwidth;
	vga_cursors[index].h = h = cp->SCheight;

	if (vga_cursors[index].mask)
		free(vga_cursors[index].mask);
	if (vga_cursors[index].face)
		free(vga_cursors[index].face);

	size = (((w + 31) & ~31) >> 3) * h;
	if (size > blotted_size) {
		if (blotted_size)
			vga_blotted_mem = (BYTE *)realloc(vga_blotted_mem, (size+h)*4);
		else
			vga_blotted_mem = (BYTE *)malloc((size+h) * vt_info.planes);
		if (!vga_blotted_mem)
			return(SI_FAIL);
		blotted_size = size;
	}
	vga_cursors[index].mask = (BYTE *)malloc(size);
	if (!vga_cursors[index].mask) {
		free(vga_blotted_mem);
		return(SI_FAIL);
	}

	vga_cursors[index].face = (BYTE *)malloc(size);
	if (!vga_cursors[index].face) {
		free(vga_blotted_mem);
		free(vga_cursors[index].mask);
		return(SI_FAIL);
	}

	bcopy(cp->SCinvsrc->Bptr, vga_cursors[index].mask, size);
	bcopy(cp->SCsrc->Bptr, vga_cursors[index].face, size);

	src_mask = (BYTE *)cp->SCmask->Bptr;
	src_face = vga_cursors[index].face;

	for (i = 0; i < size; i++)
		*src_face++ &= *src_mask++;

	if (vga_curs_on && (current_cur_index == index))
		vga_hcurs_turnon(index);
	return(SI_SUCCEED);
}



/*
 *	vga_hcurs_turnon(index)	-- turn on a cursor
 *
 *	Input: 
 *		int	index	-- index of cursor to turn on
 */
SIBool
vga_hcurs_turnon(index)
int	index;
{
	DBENTRY2("vga_hcurs_turnon()");

	vga_setup_cursor(index);
	vga_save_blotted();
	vga_paint_cursor(index);
	vga_curs_on = 1;
	current_cur_index = index;
}



/*
 *	vga_hcurs_turnoff(index) -- turn off a cursor
 *
 *	Input: 
 *		int	index	-- index of cursor to turn off
 */
SIBool
vga_hcurs_turnoff(index)
int	index;
{
	DBENTRY2("vga_hcurs_turnoff()");

	if (vga_curs_on)
		vga_restore_blotted();

	vga_curs_on = 0;
	current_cur_index = -1;
}



/*
 *	vga_hcurs_move(index, x, y)	-- move a cursor
 *
 *	Input: 
 *		int	index	-- index of cursor to turn off
 *		int	x	-- x position (in pixels) for cursor
 *		int	y	-- y position (in pixels) for cursor
 */
SIBool
vga_hcurs_move(index, x, y)
int	index;
int	x, y;
{
	DBENTRY2("vga_hcurs_move()");

	vga_curs_x = x;
	vga_curs_y = y;
	if (vga_curs_on) {
		vga_setup_cursor(index);
		vga_restore_blotted();
		vga_save_blotted();
		vga_paint_cursor(index);
	}
}



/*
 *	vga_save_blotted()	-- save the area about to be blotted out
 *				by a new cursor.  
 */
vga_save_blotted()
{
	register BYTE *dst;
	register int plane, incr;

	vga_blotted_addr = vga_curs_addr;
	blotted_h = current_cur.h;
	if (current_cur.w <= 0)
		blotted_w = -1;
	else
		blotted_w = ((vga_curs_x + current_cur.w - 1) >> 3) - 
			    (vga_curs_x >> 3) + 1;

	if ((blotted_w <= 0) || (blotted_h <= 0))
		return;

	dst = vga_blotted_mem;
	incr = blotted_w * blotted_h;
	for (plane = 0; plane < vt_info.planes; plane++) {
		outw(VGA_GRAPH, READ_MASK | (vga_read_map[plane] << 8));
		vga_save_blot(dst, blotted_w, blotted_h);
		dst += incr;
	}
}


/*
 *	vga_restore_blotted()	-- restore the area previously blotted out
 *				a new cursor.
 */
vga_restore_blotted()
{
	register BYTE	*src;
	register int	plane, incr;

	if ((blotted_w <= 0) || (blotted_h <= 0))
		return;

	src = vga_blotted_mem;
	incr = blotted_w * blotted_h;
	for (plane = 0; plane < vt_info.planes; plane++) {
		outw(VGA_SEQ, MAP_MASK | (vga_write_map[plane] << 8));
		vga_restore_blot(src, blotted_w, blotted_h);
		src += incr;
	}

	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */
}



/*
 *	vga_paint_cursor(index)	-- draw the current cursor
 */
vga_paint_cursor(index)
int index;
{
	SIbitmap src;
	int saved_mode, saved_fg, saved_function, saved_isrc, saved_idst;

	if ((current_cur.w <= 0) || (current_cur.h <=0))
		return;

	saved_fg = vga_gs->fg;
	saved_mode = vga_gs->mode;
	saved_function = vga_function;
	saved_isrc = vga_invertsrc;
	saved_idst = vga_invertdest;
	vga_gs->mode = GXcopy;
	vga_function = VGA_COPY;
	vga_invertsrc = SI_FALSE;
	vga_invertdest = SI_FALSE;
	outw(VGA_SEQ, MAP_MASK | vt_allplanes);		/* all planes on */
	
	src.Bwidth = vga_cursors[index].w;
	src.Bheight = vga_cursors[index].h;
	src.Bdepth = 1;

	vga_gs->fg = current_cur.fg;
	src.Bptr = (SIArray)vga_cursors[index].face;
	vga_ms_stplblt(&src, current_cur_sx, current_cur_sy, 
		       vga_curs_x, vga_curs_y, 
		       current_cur.w, current_cur.h, 0, SGStipple);

	vga_gs->fg = current_cur.bg;
	src.Bptr = (SIArray)vga_cursors[index].mask;
	vga_ms_stplblt(&src, current_cur_sx, current_cur_sy, 
		       vga_curs_x, vga_curs_y, 
		       current_cur.w, current_cur.h, 0, SGStipple);

	vga_gs->fg = saved_fg;				/* reset current GS */
	vga_gs->mode = saved_mode;
	vga_function = saved_function;
	vga_invertsrc = saved_isrc;
	vga_invertdest = saved_idst;
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);	/* reset plane mask */
}



/*
 *	vga_setup_cursor(index)	-- setup the internal cursor structure
 *				based on the current cursor index, and
 *				the cursor's x, y position.  Clip as
 *				needed.
 *
 *	Input:
 *		int	index	-- index of cursor to setup
 */
vga_setup_cursor(index)
{
	register vga_cursor	*pcurs;
	int		t;

	pcurs = &(vga_cursors[index]);
	current_cur.w = pcurs->w;
	current_cur.h = pcurs->h;
	current_cur.fg = pcurs->fg;
	current_cur.bg = pcurs->bg;
	current_cur_sx = 0;
	current_cur_sy = 0;

	if (vga_curs_x < 0) {
		current_cur_sx = -vga_curs_x;
		current_cur.w += vga_curs_x;
		vga_curs_x = 0;
	}
	t = (vga_curs_x + current_cur.w) - vt_info.xpix;
	if (t > 0)
		current_cur.w -= t;

	if (vga_curs_y < 0) {
		current_cur_sy = -vga_curs_y;
		current_cur.h += vga_curs_y;
		vga_curs_y = 0;
	}
	t = (vga_curs_y + current_cur.h) - vt_info.ypix;
	if (t > 0)
		current_cur.h -= t;

	vga_curs_addr = vga_fb + vga_byteoffset(vga_curs_x, vga_curs_y);

	if ((current_cur.h == 0) || (current_cur.w <= 0))
		return;

	/*
	 * do any panning needed
	 */
	if (vga_curs_x < vt_screen_x)
		vt_screen_x = vga_curs_x;

	if ((vga_curs_x + current_cur.w) >= (vt_screen_x + vt_screen_w))
		vt_screen_x = vga_curs_x + current_cur.w - vt_screen_w;

	if (vga_curs_y < vt_screen_y)
		vt_screen_y = vga_curs_y;

	if ((vga_curs_y + current_cur.h) >= (vt_screen_y + vt_screen_h))
		vt_screen_y = vga_curs_y + current_cur.h - vt_screen_h;

	vt_set_start_addr();
}
