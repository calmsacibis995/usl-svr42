/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256curs.c	1.1"

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


/*
 *	The term "blotted" will refer to the area that a cursor "blots" out.
 */
static	int	v256_curs_on = 0;	/* true when cursor is on screen */
static	int	v256_curs_x = -1;	/* current x position */
static	int	v256_curs_y = -1;	/* current y position */
static	int	current_height;		/* current (clipped) cursor height */
static	int	current_width;		/* current (clipped) cursor width */
static	int	current_cur_sx;		/* source x offset */
static	int	current_cur_sy;		/* source y offset */
static	int	v256_blotted_addr;	/* blotted screen position */
static	BYTE	*v256_blotted_mem;	/* data that gets blotted */
static	int	v256_curs_addr;	/* position for cursor to be drawn */
static	int	blotted_size = 0;	/* number of bytes blotted */
static	int	blotted_h;		/* number of lines blotted */
static	int	blotted_w;		/* number of bytes blotted */

struct {				/* internal cursor format */
	int	w;			/* width of cursor */
	int	h;			/* height of cursor */
	BYTE	valid;			/* indicates the cursor is valid */
	BYTE	fg;			/* foreground color */
	BYTE	bg;			/* background color */
	BYTE	*mask;			/* cursor mask */
	BYTE	*face;			/* cursor face */
} v256_cursor;


extern SIBool v256_hcurs_turnon();


/*
 *	v256_hcurs_download(index, cp) 	-- download a cursor.  The bitmaps
 *					for the cursor get translated to
 *					the V256 screen format, and a byte
 *					of padding is inserted at the beginning
 *					for shifting room when needed.
 *
 *	Input:
 *		int		index	-- index of cursor being downloaded
 *		SICursorP	cp	-- pointer to new cursor structure
 *
 *	Note:
 *		There are hardcoded numbers in here because the routine is
 *		written is such a way that simply changing the Cursor width
 *		and height constants in v256.h would not work.
 */
int
v256_hcurs_download(index, cp)
int		index;
SICursorP	cp;
{
	int w, h, size;
	int was_on;

	DBENTRY2("v256_hcurs_download()");

	was_on = v256_curs_on;
	if (v256_curs_on)
		v256_restore_blotted();

	v256_cursor.fg = cp->SCfg;
	v256_cursor.bg = cp->SCbg;
	v256_cursor.w = w = cp->SCwidth;
	v256_cursor.h = h = cp->SCheight;

	if (v256_cursor.mask)
		free(v256_cursor.mask);
	if (v256_cursor.face)
		free(v256_cursor.face);

	size = (((cp->SCwidth + 31) & ~31) >> 3) * h;
	if (size > blotted_size) {
		if (blotted_size)
			v256_blotted_mem = (BYTE *)realloc(v256_blotted_mem, 
							   size*8);
		else
			v256_blotted_mem = (BYTE *)malloc(size*8);
		if (!v256_blotted_mem)
			return(SI_FAIL);
		blotted_size = size;
	}

	v256_cursor.mask = (BYTE *)malloc(size);
	if (!v256_cursor.mask)
		return(SI_FAIL);

	v256_cursor.face = (BYTE *)malloc(size);
	if (!v256_cursor.face) {
		free(v256_blotted_mem);
		free(v256_cursor.mask);
		return(SI_FAIL);
	}

	bcopy(cp->SCmask->Bptr, v256_cursor.mask, size);
	bcopy(cp->SCsrc->Bptr, v256_cursor.face, size);

	if (was_on)
		v256_hcurs_turnon();
	return(SI_SUCCEED);
}



/*
 *	v256_hcurs_turnon(index)	-- turn on a cursor
 *
 *	Input: 
 *		int	index	-- index of cursor to turn on
 */
SIBool
v256_hcurs_turnon(index)
int	index;
{
	DBENTRY2("v256_hcurs_turnon()");

	v256_setup_cursor();
	v256_save_blotted();
	v256_paint_cursor();
	v256_curs_on = 1;
}



/*
 *	v256_hcurs_turnoff(index) -- turn off a cursor
 *
 *	Input: 
 *		int	index	-- index of cursor to turn off
 */
SIBool
v256_hcurs_turnoff(index)
int	index;
{
	DBENTRY2("v256_hcurs_turnoff()");
	if (v256_curs_on)
		v256_restore_blotted();
	v256_curs_on = 0;
}



/*
 *	v256_hcurs_move(index, x, y)	-- move a cursor
 *
 *	Input: 
 *		int	index	-- index of cursor to turn off
 *		int	x	-- x position (in pixels) for cursor
 *		int	y	-- y position (in pixels) for cursor
 */
SIBool
v256_hcurs_move(index, x, y)
int	index;
int	x, y;
{
	DBENTRY2("v256_hcurs_move()");

	v256_curs_x = x;
	v256_curs_y = y;
	if (v256_curs_on) {
		v256_restore_blotted();
		v256_setup_cursor();
		v256_save_blotted();
		v256_paint_cursor();
	}
}



/*
 *	v256_save_blotted()	-- save the area about to be blotted out
 *				by a new cursor.  
 */
v256_save_blotted()
{
	register BYTE *dst;
	register int i, k;

	v256_blotted_addr = k = v256_curs_addr;
	blotted_h = current_height;
	blotted_w = current_width;
	dst = (BYTE *)v256_blotted_mem;

	if ((blotted_h <= 0) || (blotted_w <= 0))
		return;

	for (i = 0; i < blotted_h; i++) {
		vidread(dst, k, blotted_w);
		dst += blotted_w;
		k += v256_slbytes;
	}
}



/*
 *	v256_restore_blotted()	-- restore the area previously blotted out
 *				a new cursor.
 */
v256_restore_blotted()
{
	register BYTE	*src;
	register int	i, k;
	int saved_function;

	if ((blotted_h <= 0) || (blotted_w <= 0))
		return;

	src = (BYTE *)v256_blotted_mem;
	k = v256_blotted_addr;
	saved_function = v256_function;
	v256_function = V256_COPY;

	for (i = 0; i < blotted_h; i++) {
		vidwrite(k, src, blotted_w, 0xffffffff);
		src += blotted_w;
		k += v256_slbytes;
	}

	v256_function = saved_function;
}



/*
 *	v256_paint_cursor()	-- draw the current cursor
 */
v256_paint_cursor()
{
	BYTE *mask_ptr, *face_ptr;
	register BYTE mask;
	BYTE src;
	register BYTE *dst;
	register int  bit;
	int i, j, k, w, incr, src_incr;
	BYTE fg, bg;

	if (current_height <= 0 || current_width <= 0)
		return;

	fg = v256_cursor.fg;
	bg = v256_cursor.bg;
	k = v256_curs_addr;
	selectpage(k);
	dst = v256_fb + (k & 0xffff);
	src_incr = ((v256_cursor.w + 31) & ~31) >> 3;
	face_ptr = v256_cursor.face + (current_cur_sy * src_incr);
	mask_ptr = v256_cursor.mask + (current_cur_sy * src_incr);
	src_incr -= ((current_width+current_cur_sx) >> 3) + 1;
	incr = v256_slbytes - current_width;

	for (i = 0; i < current_height; i++) {
		src = *face_ptr++;
		mask = *mask_ptr++;
		bit = 1;

		j = current_cur_sx;
		while (--j >= 0) {
			bit <<= 1;
			if (bit == 0x100) {
				src = *face_ptr++;
				mask = *mask_ptr++;
				bit = 1;
			}
		}

		j = current_width;
		while (--j >= 0) {
			if (k > v256_endpage) {
				selectpage(k);
				dst -= 0x10000;
			}

			if (mask & bit) 
				*dst = (src & bit)? fg: bg;
			dst++;
			k++;
		
			bit <<= 1;
			if (bit == 0x100) {
				src = *face_ptr++;
				mask = *mask_ptr++;
				bit = 1;
			}
		}

		k += incr;
		dst += incr;
		face_ptr += src_incr;
		mask_ptr += src_incr;
	}
}



/*
 *	v256_setup_cursor()	-- setup the internal cursor structure
 *				based on the current cursor index, and
 *				the cursor's x, y position.  Clip as
 *				needed.
 */
v256_setup_cursor()
{
	int	t;

	current_width = v256_cursor.w;
	current_height = v256_cursor.h;
	current_cur_sx = 0;
	current_cur_sy = 0;

	t = (v256_curs_x + current_width) - vt_info.xpix;
	if (t > 0)
		current_width -= t;
	if (v256_curs_x < 0) {
		current_cur_sx = (-v256_curs_x);
		current_width += v256_curs_x;
		v256_curs_x = 0;
	}

	t = (v256_curs_y + current_height) - vt_info.ypix;
	if (t > 0)
		current_height -= t;
	if (v256_curs_y < 0) {
		current_cur_sy = (-v256_curs_y);
		current_height += v256_curs_y;
		v256_curs_y = 0;
	}

	if ((current_height <= 0) || (current_width <= 0))
		return;

	v256_curs_addr = v256_curs_x + v256_slbytes * v256_curs_y;
}
