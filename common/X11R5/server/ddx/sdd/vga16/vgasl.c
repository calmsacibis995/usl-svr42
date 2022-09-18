/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgasl.c	1.2"

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



/*
 * A scanline is a string of pixels.  A pixel is usually 4 bits on the VGA.
 * Therefore, there are two pixels to a byte.
 *
 * (Or, at least, that's what we'll get eventually..  right now it's 1
 * pixel to a byte.  Pixels are in the least significant bits.)
 */

/*
 *	vga_getsl(y)		-- Get pixels in a scanline.  This returns
 *				a pointer to a static buffer containing the
 *				scanline data.  Subsequent calls will overwrite
 *				the buffer.
 *
 *	Input:
 *		int	y	-- Index of scanline to fetch
 */
SILine
vga_getsl(y)
int	y;
{
	register BYTE	*paddr, *to;
	register int	i;

	DBENTRY("vga_getsl()");

	paddr = (BYTE *)(vga_fb + (vga_slbytes * y));	/* point to pixels */
	to = vga_tmpsl;

	if (vt_info.planes == 1) {
		outw(VGA_GRAPH, READ_MASK | vga_read_map[0] << 8);	
		vga_byteflip(paddr, to, vt_info.xpix>>3, 0);
		return((SILine)vga_tmpsl);
	}
		
	for (i = 0; i < vt_info.planes; i++) {	/* loop through planes */
		outw(VGA_GRAPH, READ_MASK | vga_read_map[i] << 8);	
		memcpy(to, paddr, vga_slbytes);
		to += vga_slbytes;
	}

	vga_vgatosb(vga_tmpsl, vga_slbuf, ((vt_info.xpix+7)>>3), vt_info.xpix, 0);/* cvt to sb format */
	return((SILine)vga_slbuf);
}



/*
 *	vga_setsl(y, psl)	-- Set pixels in a scanline.
 *
 *	Input:
 *		int	y	-- Index of scanline to set
 *		SILine	psl	-- Pointer to scanline pixels
 */
void
vga_setsl(y, psl)
int	y;
SILine	psl;
{
	register BYTE	*paddr, *from;
	register int	i;

	DBENTRY("vga_setsl()");

	paddr = (BYTE *)(vga_fb + (vga_slbytes * y));	/* point to pixels */

	if (vt_info.planes == 1) {
		outw(VGA_SEQ, MAP_MASK | (vga_write_map[0] << 8));
		vga_byteflip(psl, paddr, vt_info.xpix>>3, 0);
		outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
		return;
	}
		
	vga_sbtovga((BYTE *)psl,vga_tmpsl,vt_info.xpix, 0, 0);/* cvt to VGA */

	from = vga_tmpsl;
	for (i = 0; i < vt_info.planes; i++) {	/* loop through planes */
		outw(VGA_SEQ, MAP_MASK | (vga_write_map[i] << 8));
		memcpy(paddr, from, vga_slbytes);
		from += vga_slbytes;
	}
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
}



/*
 *	vga_freesl(psl)		-- Free scanline buffer.  This doesn't
 *				do anything on the VGA because we use
 *				a static buffer for the scanline.
 *
 *	Input:
 *		SILine	psl	-- Pointer previously gotten scanline
 */
void
vga_freesl(psl)
SILine	psl;
{
	DBENTRY("vga_freesl()");
}
