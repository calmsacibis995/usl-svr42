/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgags.c	1.6"

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
#include "sys/inline.h"
#include "vtio.h"
#include "vga.h"
/*
 * macros like htonl are defined in different files for SVR4 and SVR3.2
 * SVR4:    sys/byteorder.h
 * SVR3.2 : defined in libtcp.a or libnet.a; but since we cannot depend
 *	    on these libraries, we define our own.
 */

/*
 *	unsigned long htonl( hl )
 *	long hl;
 *	reverses the byte order of 'long hl'
 */

asm unsigned long htonl( hl )
{
%mem	hl;	
	movl	hl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned long ntohl( nl )
 *	unsigned long nl;
 *	reverses the byte order of 'ulong nl'
 */

asm unsigned long ntohl( nl )
{
%mem	nl;
	movl	nl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

static SIint32 psstate[] = {		/* Used in downloadstate */
	SetSGpmask,
	SetSGmode,
	SetSGstplmode,
	SetSGfillmode,
	SetSGfg,
	SetSGbg,
	SetSGtile,
	SetSGstipple,
	SetSGline,
	SetSGfillrule,
	0
};

static SIint32 pgstate[] = {		/* used in get state */
	GetSGpmask,
	GetSGmode,
	GetSGstplmode,
	GetSGfillmode,
	GetSGfg,
	GetSGbg,
	GetSGfillrule,
	0
};



/*
 *	vga_download_state(indx, flag, state)	-- set the graphics state
 *						specified by indx to *state.
 *
 *	Input:
 *		int		indx	-- index into graphics states
 *		int		flag	-- mask of state elements to change
 *		SIGStateP	statep	-- pointer to new state structure
 */
SIBool
vga_download_state(indx, flag, statep)
int		indx;
int		flag;
SIGStateP	statep;
{
	SIbitmapP bmap;
	int i, w, h, size;
	vga_state *gs;

	DBENTRY1("vga_download_state()");
	gs = &vga_gstates[indx];
	for (i = 0; psstate[i]; i++) {
		switch (flag & psstate[i]) {
		case SetSGpmask:
			gs->pmask = (statep->SGpmask & (vt_info.colors-1)) << 8;
			if (vt_info.is_chained) {
				if (gs->pmask == 0x100)
					gs->pmask = 0x300;
				else
					gs->pmask = 0xf00;
			}
			break;
		case SetSGmode:
			gs->mode = statep->SGmode;
			break;
		case SetSGstplmode:
			gs->stp_mode = statep->SGstplmode;
			break;
		case SetSGfillmode:
			gs->fill_mode = statep->SGfillmode;
			break;
		case SetSGfillrule:
			gs->fill_rule = statep->SGfillrule;
			break;
		case SetSGfg:
			gs->fg = statep->SGfg & (vt_info.colors-1);
			break;
		case SetSGbg:
			gs->bg = statep->SGbg & (vt_info.colors-1);
			break;
		case SetSGstipple:
			bmap = statep->SGstipple;
			gs->raw_stipple = *bmap;
			gs->raw_stipple.Bptr = NULL;
			h = gs->raw_stipple.Bheight;
			w = gs->raw_stipple.Bwidth;
			gs->stpl_valid = 0;
			/*
			 * if we've got a large stipple, we allocate
			 * space for it and save it, otherwise, we
			 * use pre-allocated space.
			 */
			if ((w > 16) || (h > 16) || (w & (w-1))) {
				size = h * (((w + 31) & ~31) >> 3);
				if (gs->big_stpl)
					free(gs->big_stpl);
				gs->big_stpl = (BYTE *)malloc(size);
				if (gs->big_stpl != NULL)
					bcopy(bmap->Bptr, gs->big_stpl, size);
			}
			else {
				if (gs->big_stpl)
					free(gs->big_stpl);
				gs->big_stpl = NULL;
				bcopy(bmap->Bptr,gs->raw_stpl_data, h * 4);
			}
			break;
		case SetSGtile:
			bmap = statep->SGtile;
			gs->raw_tile = *bmap;
			gs->raw_tile.Bptr = NULL;
			h = gs->raw_tile.Bheight;
			w = gs->raw_tile.Bwidth;
			gs->tile_valid = 0;
			/*
			 * if we've got a large tile, we allocate
			 * space for it and save it, otherwise, we
			 * use pre-allocated space.
			 */
			size = h * ((((w*bmap->Bdepth)+31) & ~31) >> 3);
			if ((w > 16) || (h > 16) || (w & (w-1))) {
				if (gs->big_tile)
					free(gs->big_tile);
				gs->big_tile = (BYTE *)malloc(size);
				if (gs->big_tile != NULL)
					bcopy(bmap->Bptr, gs->big_tile, size);
			}
			else {
				if (gs->big_tile)
					free(gs->big_tile);
				gs->big_tile = NULL;
				bcopy(bmap->Bptr,gs->raw_tile_data, size);
			}
			break;
		case SetSGline:
			gs->line_count = statep->SGlineCNT;
			break;
			
		default:
			break;
		}
	}
	return(SI_SUCCEED);
}



/*
 *	vga_get_state(indx, flag, state)	-- get the graphics state
 *						specified by indx to *state.
 *
 *	Input:
 *		int		indx	-- index into graphics states
 *		int		flag	-- mask of state elements to change
 *		SIGStateP	statep	-- pointer to new state structure
 */
SIBool
vga_get_state(indx, flag, statep)
int		indx;
int		flag;
SIGStateP	statep;
{
	int i;
	vga_state *gs;

	DBENTRY1("vga_get_state()");
	gs = &vga_gstates[indx];
	for (i = 0; pgstate[i]; i++) {
		switch (flag & pgstate[i]) {
		case GetSGpmask:
			statep->SGpmask = (gs->pmask >> 8) & (vt_info.colors-1);
			if (vt_info.is_chained) {
				if (gs->pmask == 0x300)
					statep->SGpmask = 0x1;
				else
					statep->SGpmask = 0x3;
			}
			break;
		case GetSGmode:
			statep->SGmode = gs->mode;
			break;
		case GetSGstplmode:
			statep->SGstplmode = gs->stp_mode;
			break;
		case GetSGfillmode:
			statep->SGfillmode = gs->fill_mode;
			break;
		case GetSGfillrule:
			statep->SGfillrule = gs->fill_rule;
			break;
		case GetSGfg:
			statep->SGfg = gs->fg;
			break;
		case GetSGbg:
			statep->SGbg = gs->bg;
			break;
		default:
			break;
		}
	}
	return(SI_SUCCEED);
}



/*
 *	vga_select_state(indx, flag, state)	-- set the current state
 *						to that specified by indx.
 *
 *	Input:
 *		int		indx	-- index into graphics states
 */
SIBool
vga_select_state(indx)
int indx;
{
	DBENTRY1("vga_select_state()");

	vga_gs = &vga_gstates[indx];
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);

	/* 
 	 * now set up all the internal data structures
	 * to be used with the VGA adapter based on what is in the GS.  
	 */
	switch (vga_gs->fill_mode) {
	case SGFillSolidFG:
		vga_src = vga_gs->fg;
		break;
	case SGFillSolidBG:
		vga_src = vga_gs->bg;
		break;
	case SGFillStipple:
		cur_pat = vga_gs->stpl;
		cur_pat_h = vga_gs->stpl_h;
		break;
	case SGFillTile:
		cur_pat = vga_gs->tile;
		cur_pat_h = vga_gs->tile_h;
		break;
	}

	vga_invertsrc = SI_FALSE;
	vga_invertdest = SI_FALSE;

	switch (vga_gs->mode) {		/* set up source and dest */
		case GXclear:
			vga_src = 0;
			break;
		case GXset:
			vga_src = vt_info.colors - 1;
			break;
		case GXandInverted:
		case GXorInverted:
		case GXequiv:
		case GXcopyInverted:
			vga_src = ~vga_src & (vt_info.colors-1);
			vga_invertsrc = SI_TRUE;
			break;
		case GXnor:
		case GXnand:
			vga_src = ~vga_src & (vt_info.colors-1);
			vga_invertsrc = SI_TRUE;
		case GXandReverse:
		case GXorReverse:
		case GXinvert:
			vga_invertdest = SI_TRUE;
			break;
		case GXnoop:
			break;
	}

	switch (vga_gs->mode) {		/* determine the function */
		case GXand:
		case GXandInverted:
		case GXnor:
		case GXandReverse:
			vga_function = VGA_AND;
			break;
		case GXor:
		case GXorInverted:
		case GXorReverse:
		case GXnand:
			vga_function = VGA_OR;
			break;
		case GXequiv:
		case GXxor:
			vga_function = VGA_XOR;
			break;
		default:
			vga_function = VGA_COPY;
	}

	return(SI_SUCCEED);
}



/*
 *	vga_clip(x1, y1, x2, y2)	-- set out clipping rectangle to
 *					the coordinates specified.
 *
 *	Input:
 *		int	x1, y1		-- upper left corner of rectangle
 *		int	x2, y2		-- bottom right corner of rectangle
 */
SIBool
vga_clip(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
	DBENTRY1("vga_clip()");

	vga_clip_x1 = x1;
	vga_clip_y1 = y1;
	vga_clip_x2 = x2;
	vga_clip_y2 = y2;
	return(SI_SUCCEED);
}



/*
 *	vga_pat_setup(src, dst, w, h, x, y)	-- Given a bitmap in src, 
 *						pad it to exactly 16 bits
 *						of width, align it at 0, 0
 *						and store it in dst.  
 *
 *	Input:
 *		BYTE	*src	-- source bitmap
 *		BYTE	*dst	-- destination
 *		int	w	-- width of source bitmap
 *		int	h	-- height of source bitmap
 *		int	x	-- x origin (in bits) of source bitmap
 *		int	y	-- y origin (in bits) of source bitmap
 *
 *	Returns:
 *		0 for normal patterns
 *		1 if pattern is really 8 bits wide
 *		-1 if the pattern has a bad width
 */
vga_pat_setup(src, dst, w, h, x, y)
BYTE	*src, *dst;
int	w, h, x, y;
{
	BITS16 *pat_word, *dst_word, mask;
	BITS32 *pat_long;
	unsigned long t;
	int	i, hcnt;
	int	shift;
	int	tile_one;
	
	tile_one = 0;

	while (w < 16)
		/* 
		 * We want to end up with a pattern exactly 16 bits wide
		 * so for any power of two less than that, we double the
		 * pattern until we've got 16 bits worth.
		 */
		switch (w) {
		case 1:
		case 2:
		case 4:
			mask = (~(0xff >> w)) & 0xff;
			pat_word = (BITS16 *)src;
			for (i = 0; i < h; i++) {
				*pat_word &= mask;
				*pat_word |= *pat_word >> w;
				pat_word +=2;
			}
			w *= 2;
			break;
		case 8:
			pat_word = (BITS16 *)src;
			for (i = 0; i < h; i++) {
				*pat_word &= 0xff;
				*pat_word |= *pat_word << w;
				pat_word +=2;
			}
			w *= 2;
			tile_one = 1;
			break;
		default:
			return(-1);		/* bad pattern */
		}

	/* 
	 * now that we've got a 16 bit wide pattern, double it again so it 
	 * can easily be shifted over to get the aligned pattern.
	 */
	pat_word = (BITS16 *)src;
	for (i = 0; i < h; i++) {
		*(pat_word+1) = *pat_word;
		pat_word += 2;
	}

	/*
	 * Shift the pattern as needed and store the result in the dst.
	 * Note that we also may need to start in the middle if the
	 * pattern is mis-aligned on the y axis.
	 */
	y %= h;
	hcnt = (y ? (h - y) : 0);
	pat_long = ((BITS32 *)src) + hcnt;
	dst_word = (BITS16 *)dst;
	shift = 16 - (x % 16);
	for (i = 0; i < h; i++, hcnt++) {
		if (hcnt == h) {
			hcnt = 0;
			pat_long = (BITS32 *)src;
		}
		t = (unsigned)htonl(*pat_long++) << shift;
		*dst_word++ = (BITS16)ntohl(t);
	}

	return(tile_one);
}
