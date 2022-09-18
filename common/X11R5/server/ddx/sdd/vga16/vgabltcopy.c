/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgabltcopy.c	1.2"

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
 *	vga_shift_copy(psrc, pdst, srccnt, dstcnt, startmask, endmask, shift)
 *		-- Special case of bitblt where the ROP is copy.  We start
 *		with a scanline of pixels, then do the shifts and writes
 *		all at once for speed.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to source
 *		BYTE	*pdst		-- pointer to destination
 *		int	srccnt		-- number of bytes in buf per scanline
 *		int	dstcnt		-- number of bytes to copy 
 *		BYTE	startmask	-- mask for first byte of each line
 *		BYTE	endmask		-- mask for first byte of each line
 *		int	shift		-- number of bits to shift left by
 */
vga_shift_copy(psrc, pdst, srccnt, dstcnt, startmask, endmask, shift)
register BYTE	*psrc;
register BYTE	*pdst;
int	srccnt, dstcnt;
BYTE	startmask, endmask;
{
	int	i;

	if (--dstcnt == 0) {
		endmask = endmask & startmask;
		startmask = 0;
	}

	for (i = 0; i < vt_info.planes; i++) {	/* loop through planes */
		outw(VGA_SEQ, MAP_MASK | (vga_write_map[i] << 8));
		vga_shift_out(psrc, pdst, dstcnt, startmask, endmask, shift);
		psrc += srccnt;
	}
}



/*
 *	vga_aligned_copy(psrc, pdst, xcnt, ycnt, start_mask, end_mask, incr)
 *		-- Special case of bitblt where no shifting is needed 
 *		and the ROP is copy.  For the interior bytes (those 
 *		that are complete 8 bit copies), we can do a fast copy 
 *		using the VGA latches instead of having to deal with
 *		multiple planes.
 *
 *	Input:
 *		BYTE	*psrc		-- pointer to source data
 *		BYTE	*pdst		-- pointer to destination
 *		int	xcnt		-- number of bytes per scanline to copy
 *		int	ycnt		-- number of scanlines to copy
 *		BYTE	start_mask	-- mask for first byte of each line
 *		BYTE	end_mask	-- mask for first byte of each line
 *		int	incr		-- amount to bump psrc, and pdst by
 */
vga_aligned_copy(psrc, pdst, xcnt, ycnt, start_mask, end_mask, incr)
BYTE	*psrc, *pdst;
int	xcnt, ycnt;
BYTE	start_mask, end_mask;
int	incr;
{
	int	do_start = 0;

	xcnt--;
	if (start_mask) {
		if (xcnt == 0)
			end_mask &= start_mask;
		else {
			do_start = 1;
			psrc++;
			pdst++;
			xcnt--;
		}
	}

	if (xcnt) {
		outw(VGA_GRAPH, gr_mode | 0x100);	/* write mode 1 */
		if (incr < 0)
			vga_alcpy_middle_down(psrc, pdst, xcnt, ycnt);
		else
			vga_alcpy_middle_up(psrc, pdst, xcnt, ycnt);
		outw(VGA_GRAPH, gr_mode);	/* restore write mode 0 */
	}

	if (do_start) 
		vga_alcpy_one(psrc-1, pdst-1, ycnt, start_mask, 
			      incr, vt_info.planes);

	vga_alcpy_one(psrc+xcnt, pdst+xcnt, ycnt, end_mask,incr,vt_info.planes);

	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
	outw(VGA_GRAPH, BITMASK | 0xff00);
}
