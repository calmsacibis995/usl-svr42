/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgapoints.c	1.2"

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

#include "X.h"
#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "vga.h"
#include "sys/inline.h"


/*
 *	vga_plot_points(cnt, ptsIn)	-- draw a series of pixels based on
 *					points passed in.  Points are drawn
 *					in the current foreground color using
 *					the current ROP.  Since the VGA has 
 *					the four basic ROPs built in, we expand
 *					that by noting that:
 *					       ~dest == dest XOR 0xff
 *
 *	Input:
 *		int		cnt	-- count of number of points
 *		DDXPointRec	*ptsIn	-- Points
 */
SIBool
vga_plot_points(cnt, ptsIn)
int		cnt;
DDXPointRec	*ptsIn;
{
	DBENTRY("vga_poly_points()");
	
	if (vga_gs->mode == GXnoop)
		return(SI_SUCCEED);

	outw(VGA_GRAPH, GR_ENAB_SR | vt_allplanes);

	if (vga_invertdest) {
		outw(VGA_GRAPH, GR_FUNC | VGA_XOR);		
		outw(VGA_GRAPH, GR_SR | 0xff00);
		outb(VGA_GRAPH, BITMASK);
		vga_put_points(ptsIn, vga_fb, cnt);
	}

	if (vga_gs->mode != GXinvert) {
		outw(VGA_GRAPH, GR_FUNC | vga_function);	/* set rop */
		outw(VGA_GRAPH, GR_SR | (vga_color_map[vga_src] << 8));
		outb(VGA_GRAPH, BITMASK);
		vga_put_points(ptsIn, vga_fb, cnt);
	}
	
	outw(VGA_GRAPH, gr_mode);			/* reset regs */
	outw(VGA_GRAPH, BITMASK | 0xff00);
	outw(VGA_GRAPH, GR_FUNC | VGA_COPY);
	outw(VGA_GRAPH, GR_ENAB_SR);
	return(SI_SUCCEED);
}
