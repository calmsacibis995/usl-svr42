/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgadlfont.c	1.4"

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
#include "vga.h"
#include "sys/inline.h"

static	int	left_x, right_x, top_y, bottom_y;

/*
 * For downloaded fonts, we just copy the font data into an internal 
 * area, leaving the padding in place.  The end result is significantly
 * faster text painting because we eliminate extra function calls, tests, 
 * etc that the SBI must do for it's general cases.
 */


/*
 *	vga_check_dlfont(num, info) 	-- Check to see if we can download
 *					a new font.
 *
 *	Input:
 *		int		num	-- index of the font to be downloaded
 *		SIFontInfoP	info	-- basic info about font
 */
SIBool
vga_check_dlfont(num, info)
int		num;
SIFontInfoP	info;
{
	int height;

	DBENTRY("vga_check_dlfont()");

	if ((info->SFflag & SFTerminalFont) == 0)
		return(SI_FAIL);

	if ((info->SFnumglyph < 0) || (info->SFnumglyph > VGA_NUMDLGLYPHS))
		return(SI_FAIL);

	if ((info->SFmax.SFwidth < 1) || (info->SFmax.SFwidth > VGA_DL_FONT_W))
		return(SI_FAIL);
		
	height = info->SFmax.SFascent + info->SFmax.SFdescent;
	if ((height < 1) || (height > VGA_DL_FONT_H))
		return(SI_FAIL);
	
	return(SI_SUCCEED);
}



/*
 *	vga_dl_font(num, info, glyphs) 	-- download the glyphs for a font
 *
 *	Input:
 *		int		num	-- the index for the downloaded font
 *		SIFontInfoP	info	-- basic info about font
 *		SIGlyphP	glyphs	-- the glyphs themselves
 */
SIBool
vga_dl_font(num, info, glyphs)
int		num;
SIFontInfoP	info;
SIGlyphP	glyphs;
{
	register BITS32	*font_dst, *font_src;
	register int	i, chars;
	int		h;

	DBENTRY("vga_dl_font()");

	vga_fonts[num].w = info->SFmax.SFwidth;
	vga_fonts[num].h = h = info->SFmax.SFascent + info->SFmax.SFdescent;
	vga_fonts[num].ascent = info->SFmax.SFascent;
	chars = info->SFnumglyph;

	if (!(vga_fonts[num].glyphs = (BYTE *) malloc(chars * 4 * h)))
		return(SI_FAIL);

	font_dst = (BITS32 *) vga_fonts[num].glyphs;

	for (; chars; chars--, glyphs++) {
		i = h;					/* number of lines */
		font_src = (BITS32 *)glyphs->SFglyph.Bptr;
		while (i--)
			*font_dst++ = *font_src++;
	}
	
	return(SI_SUCCEED);
}



/*
 *	vga_stpl_font(num, x_start, y, cnt_start, glyphs_start, type)
 *				-- stipple glyphs in a downloaded font.
 *
 *	Input:
 *		int	num	-- font index to stipple from
 *		int	x_start	-- x position of baseline to stipple to
 *		int 	y	-- y position of baseline to stipple to
 *		int	cnt_start	-- number of glyphs to stipple
 *		BITS16	*glyphs_start	-- list of glyph indices to stipple
 *		int	type	-- Opaque or regular stipple
 */
SIBool
vga_stpl_font(num, x_start, y, cnt_start, glyphs_start, type)
int	num;
int	x_start;
int	y;
int	cnt_start;
BITS16	*glyphs_start;
int	type;
{
	SIbitmap	src;
	register int x;
	register BITS16	*glyphs;
	BITS32	*glyph_data;
	int	w;
	int	(**stpls)();
	int	sy, char_height, h;
	int	old_mode = -1;

	DBENTRY("vga_stpl_font()");

	y -= vga_fonts[num].ascent;
	src.Bdepth = 1;
	src.Bwidth = w = vga_fonts[num].w;
	char_height = vga_fonts[num].h;
	glyph_data = (BITS32 *)vga_fonts[num].glyphs;

	/*
	 * Figure out all the clipping bounds
	 */
	left_x   = x_start;
	right_x  = x_start + (w * cnt_start) - 1;
	top_y    = y;
	bottom_y = y + char_height - 1;

	if ((left_x > vga_clip_x2) || (right_x < vga_clip_x1) ||
	    (top_y > vga_clip_y2) || (bottom_y < vga_clip_y1)) 
		return(SI_SUCCEED);

	if (left_x < vga_clip_x1) left_x = vga_clip_x1;
	if (right_x > vga_clip_x2) right_x = vga_clip_x2;
	if (top_y < vga_clip_y1) top_y = vga_clip_y1;
	if (bottom_y > vga_clip_y2) bottom_y = vga_clip_y2;

	h = bottom_y - top_y + 1;

	/*
	 * fill in the background if we're doing an opaque stipple
	 */
	if ((type == SGOPQStipple) || 
	    ((type == 0) && (vga_gs->stp_mode == SGOPQStipple))) {
		DDXPointRec	pt1, pt2;

		outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes); /* enable SR reg */
		outw(VGA_GRAPH, GR_SR | (vga_color_map[vga_gs->bg] << 8));
		old_mode = vga_gs->mode;
		vga_gs->mode = GXcopy;

		pt1.x = left_x;
		pt2.x = right_x;
		pt2.y = pt1.y = top_y;

		vga_line_horiz(pt1, pt2, h);

		outw(VGA_GRAPH, BITMASK | 0xff00);
	}

	sy = top_y - y;
	if (x_start < left_x)			/* handle chars off left */
		vga_stpl_left_clip(num, &x_start, top_y, h, &glyphs_start, sy);
	
	x = x_start;
	right_x -= (w - 1);
	glyphs = glyphs_start;

	if (vga_gs->mode == GXcopy) {
		outw(VGA_GRAPH, GR_SR | (vga_color_map[vga_gs->fg] << 8));
		outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes);

		if (vt_info.is_vga) {
			outw(VGA_GRAPH, gr_mode | 0x300); /* write mode 3 */
			stpls = vga_stpls;
		}
		else {
			outb(VGA_GRAPH, BITMASK);
			stpls = ega_stpls;
		}

		while (x <= right_x) {
			(*stpls[((x+w-1) >> 3) - (x >> 3)])(
				glyph_data + (*glyphs * char_height) + sy,
				vga_fb + vga_byteoffset(x, top_y),
				h,
				x & 0x07,
				(1 << w) - 1);

			x += w;
			glyphs++;
		}

		if (vt_info.is_vga)
			outw(VGA_GRAPH, gr_mode);
		else
			outw(VGA_GRAPH, BITMASK | 0xff00);

		outw(VGA_GRAPH, GR_ENAB_SR);
	}
	else {
		while (x <= right_x) {
			src.Bptr = (SIArray) (glyph_data+(*glyphs*char_height));
			vga_ms_stplblt(&src, 0, sy, x, top_y, w, h,0,SGStipple);

			x += w;
			glyphs++;
		}
	}

	if (x < (right_x + w)) {	/* have a final clipped glyph to do */
		src.Bptr = (SIArray) (glyph_data+(*glyphs*char_height));
		vga_ms_stplblt(&src, 0, sy,x,top_y,(right_x+w-x),h,0,SGStipple);
	}

	/*
	 * Put back the rasterop mode if we had to change it
	 */
	if (old_mode != -1)
		vga_gs->mode = old_mode;

	return(SI_SUCCEED);
}



/*
 *	vga_stpl_left_clip(num, x, y, h, glyphs, sy)
 *				-- stipple the glyphs in a downloaded font
 *				that are fully or partially off the left
 *				edge of the clip rectangle.
 *
 *	Input:
 *		int	num	-- index of font being drawn
 *		int	*x	-- x position of baseline to stipple to
 *		int 	y	-- y position of baseline to stipple to
 *		int	h	-- height to stipple
 *		BITS16	**glyphs-- list of glyph indices to stipple
 *		int	sy	-- starting y position
 */
vga_stpl_left_clip(num, x, y, h, glyphs, sy)
int		num;
register int	*x;
int		y;
int		h;
register BITS16	**glyphs;
int		sy;
{
	int char_height, max_w, width;
	BITS32 *glyph_data;
	SIbitmap	src;

	src.Bwidth = max_w = vga_fonts[num].w;
	src.Bdepth = 1;
	char_height = vga_fonts[num].h;
	glyph_data = (BITS32 *)vga_fonts[num].glyphs;

	while ((*x + max_w) < left_x) {
		*x += max_w;
		(*glyphs)++;
	}

	width = max_w - (left_x - *x) - 1;
	if ((left_x + width) > right_x)
		width = right_x - left_x;
	width++;

	src.Bwidth = max_w;
	src.Bptr = (SIArray) (glyph_data+(**glyphs*char_height));
	vga_ms_stplblt(&src, (left_x - *x), sy, left_x, y, 
		       width, h, 0, SGStipple);

	*x += max_w;
	(*glyphs)++;
}



/*
 *	vga_font_free(num)	-- Free data structures associated with a
 *				downloaded font.
 *
 *	Input:
 *		int	num	-- index of font
 */
SIBool
vga_font_free(num)
int	num;
{
	DBENTRY("vga_font_free()");

	free(vga_fonts[num].glyphs);
	return(SI_SUCCEED);
}
