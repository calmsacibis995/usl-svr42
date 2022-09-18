/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgastplblt.c	1.8"

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

extern int vga_slstpl();
extern int ega_slstpl();


/*
 *	vga_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, type) 
 *				-- Stipple the screen using the bitmap in src.
 *
 *	Input:
 *		SIbitmapP	src	-- pointer to source data		
 *		int		sx	-- X position (in pixels) of source
 *		int		sy	-- Y position (in pixels) of source
 *		int		dx	-- X position (in pixels) of destination
 *		int		dy	-- Y position (in pixels) of destination
 *		int		w	-- Width (in pixels) of area to move
 *		int		h	-- Height (in pixels) of area to move
 *		int		plane	-- which source plane
 *		int		type	-- Opaque or regular stipple
 */
SIBool
vga_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, type)
SIbitmapP 	src;
int		sx, sy;
register int	dx;
int		dy;
register int	w;
int		h;
int		plane;
int		type;
{
	BYTE	dmask1, dmask2, srcend;
	int	srcshift, shift, byteshift, srccnt, dstcnt;
	register BYTE	*psrc, *pdst;
	int	srcinc, dstinc;
	register int 	(**stpls)();
	int	(*slstpl)();
	BYTE fg, bg;

	DBENTRY("vga_ms_stplblt()");
	
	if ((w == 0) || (h == 0))
		return(SI_SUCCEED);
	
	if (src->Bdepth != 1)
		return(SI_FAIL);

	if (type == 0)
		type = vga_gs->stp_mode;

	if ((type != SGOPQStipple) &&
	    (vga_gs->mode == GXcopy) && (w <= 16) && (src->Bwidth == w)) {
		outw(VGA_GRAPH, GR_SR | (vga_color_map[vga_gs->fg]<<8));
		outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes);

		if (vt_info.is_vga) {
			outw(VGA_GRAPH, gr_mode | 0x300); /* write mode 3 */
			stpls = vga_stpls;
		}
		else {
			outb(VGA_GRAPH, BITMASK);
			stpls = ega_stpls;
		}
		
		(*stpls[((dx+w-1) >> 3) - (dx >> 3)])(
			(BYTE *)src->Bptr + (sy<<2), 
			vga_fb + vga_byteoffset(dx, dy),
			h, 
			((dx & 0x07) - (sx & 0x7)) & 0x7,
			(1 << w) - 1);

		if (vt_info.is_vga)
			outw(VGA_GRAPH, gr_mode);
		else
			outw(VGA_GRAPH, BITMASK | 0xff00);

		outw(VGA_GRAPH, GR_ENAB_SR);
		return(SI_SUCCEED);
	}

	if (vga_gs->mode == GXnoop)
		return(SI_SUCCEED);

	srcinc = ((src->Bwidth + 31) & ~31) >> 3;
	psrc = (BYTE *)src->Bptr + (srcinc * sy) + (sx >> 3);
	srcshift = sx & 0x7;
	srcend = (sx+w-1) & 0x7;

	dstinc = vga_slbytes;
	pdst = vga_fb + vga_byteoffset(dx, dy);
	dmask1 = vga_start_bits[dx & 0x7];
	dmask2 = vga_end_bits[(dx+w-1) & 0x7];

	shift = srcshift - (dx & 0x7);

	dstcnt = srccnt = ((sx+w-1) >> 3) - (sx >> 3) + 1;
	byteshift = 0;
	if (shift < 0) {			/* shift crosses bytes? */
		byteshift = 1;
		shift += 8;
		dstcnt++;
	}

	if (shift > (int)srcend) /* clear the last byte? */
		dstcnt--;

	/*
	 * Set up colors
	 */
	if (vga_invertsrc) {
		fg = ~vga_gs->fg & (vt_info.colors - 1);
		bg = ~vga_gs->bg & (vt_info.colors - 1);
	}
	else {
		fg = vga_gs->fg;
		bg = vga_gs->bg;
	}

	if (vga_gs->mode == GXset) {
		fg = bg = vt_info.colors - 1;
	}
	else if (vga_gs->mode == GXclear) {
		fg = bg = 0;
	}

	fg = vga_color_map[fg];
	bg = vga_color_map[bg];

	outw(VGA_GRAPH, GR_FUNC | vga_function);	/* set rop */
	if (vt_info.is_vga) {
		outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes);
		outw(VGA_GRAPH, gr_mode | 0x300);	/* write mode 3 */
		slstpl = vga_slstpl;
	} 
	else {
		outw(VGA_GRAPH, gr_mode | 0x200);	/* write mode 2 */
		slstpl = ega_slstpl;
	}

	for (;h--; psrc+=srcinc, pdst+=dstinc) {
		vga_shiftflip(psrc, vga_slbuf, shift, srccnt, byteshift);
		if (vga_invertdest) {
			outw(VGA_GRAPH, GR_FUNC | VGA_XOR);

			(*slstpl)(vga_slbuf,pdst,dstcnt,dmask1,dmask2, 
				vt_info.colors - 1, vt_info.colors - 1, type);

			if (vga_gs->mode == GXinvert) 
				continue;

			outw(VGA_GRAPH, GR_FUNC | vga_function);
		}

		(*slstpl)(vga_slbuf, pdst, dstcnt, dmask1, dmask2, fg,bg,type);
	}

	outw(VGA_GRAPH, gr_mode);
	outw(VGA_GRAPH, BITMASK | 0xff00);		/* reset bit mask */
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);		/* restore rop */
	outw(VGA_GRAPH, GR_ENAB_SR);

	return(SI_SUCCEED);
}



/* 
 *	ega_slstpl(psrc, pdst, dstcnt, startmask, endmask) 
 *					-- Stipple the screen at pdst with
 *					the bitmap in psrc.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to bitmap
 *		BYTE	*pdst		-- pointer to screen memory
 *		int	dstcnt		-- number of bytes to copy
 *		BYTE	startmask	-- mask for first byte
 *		BYTE	endmask		-- mask for last byte
 */
ega_slstpl(psrc, pdst, dstcnt, startmask, endmask, fg, bg, type)
register BYTE	*pdst;
register BYTE	*psrc;
register int	dstcnt;
BYTE	startmask, endmask;
BYTE	fg, bg;
int	type;
{
	VOLATILEBYTE temp;
	BYTE dat;

	dstcnt--;

	outb(VGA_GRAPH, BITMASK);

	if (startmask) 
		if (dstcnt == 0)			/* do only one byte */
			endmask = endmask & startmask;
		else {
			dat = startmask & *psrc++;
			outb(VGA_GRAPH+1, dat);
			temp = *pdst;
			*pdst = fg;
			if (type == SGOPQStipple) {
				outb(VGA_GRAPH+1, startmask & ~dat);
				temp = *pdst;
				*pdst = bg;
			}
			pdst++;
			dstcnt--;
		}

	if (dstcnt) {
		if (type == SGOPQStipple) 
			ega_opqstplblt_middle(psrc, pdst, dstcnt,fg,bg);
		else
			ega_stplblt_middle(psrc, pdst, dstcnt, fg);
	}

	psrc += dstcnt;
	pdst += dstcnt;
							/* last byte */
	dat = endmask & *psrc;
	outb(VGA_GRAPH+1, dat);
	temp = *pdst;
	*pdst = fg;
	if (type == SGOPQStipple) {
		outb(VGA_GRAPH+1, endmask & ~dat);
		temp = *pdst;
		*pdst = bg;
	}
}



/* 
 *	vga_slstpl(psrc, pdst, dstcnt, startmask, endmask) 
 *					-- Stipple the screen at pdst with
 *					the bitmap in psrc.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to bitmap
 *		BYTE	*pdst		-- pointer to screen memory
 *		int	dstcnt		-- number of bytes to copy
 *		BYTE	startmask	-- mask for first byte
 *		BYTE	endmask		-- mask for last byte
 */
vga_slstpl(psrc, pdst, dstcnt, startmask, endmask, fg, bg, type)
register BYTE	*pdst;
register BYTE	*psrc;
register int	dstcnt;
BYTE	startmask, endmask;
BYTE	fg, bg;
int	type;
{
	VOLATILEBYTE temp;
	BYTE dat;
	BYTE *latch;

	dstcnt--;

	outb(VGA_GRAPH, GR_SR);

	if (startmask)
		if (dstcnt == 0)			/* do only one byte */
			endmask &= startmask;
		else {
			dat = startmask & *psrc++;
			outb(VGA_GRAPH+1, fg);
			temp = *pdst;
			*pdst = dat;
			if (type == SGOPQStipple) {
				outb(VGA_GRAPH+1, bg);
				temp = *pdst;
				*pdst = startmask & ~dat;
			}
			pdst++;
			dstcnt--;
		}


	if (dstcnt) {
		if (type == SGOPQStipple) {
			/*
			 * When the mode is copy, we can do an opaque stipple
			 * very fast by latching the background color and 
			 * drawing in the foreground color.  We don't have
			 * to latch each destination address, so it saves
			 * half the accesses to the VGA.
			 */
			if (vga_gs->mode == GXcopy) {
				outb(VGA_GRAPH+1, bg);
				latch = vga_fb + (vt_info.ypix+2)*vga_slbytes;
				*latch = 0xff;
				temp = *latch;
				outb(VGA_GRAPH+1, fg);
				vga_copyopqstplblt_middle(psrc, pdst, dstcnt);
			}
			else
				vga_opqstplblt_middle(psrc,pdst,dstcnt,fg,bg);
		}
		else
			vga_stplblt_middle(psrc, pdst, dstcnt, fg);
	}

	psrc += dstcnt;
	pdst += dstcnt;
							/* last byte */
	dat = endmask & *psrc;
	outb(VGA_GRAPH+1, fg);
	temp = *pdst;
	*pdst = dat;
	if (type == SGOPQStipple) {
		outb(VGA_GRAPH+1, bg);
		temp = *pdst;
		*pdst = endmask & ~dat;
	}
}
