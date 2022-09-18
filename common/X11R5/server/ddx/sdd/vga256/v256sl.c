/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256sl.c	1.1"

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
 * The get and set scanline routines actually use video memory directly 
 * whenever the scanline being modified is within one memory page.
 */


/*
 *	v256_getsl(y)		-- Get pixels in a scanline.  This returns
 *				a pointer to a static buffer containing the
 *				scanline data.  Subsequent calls will overwrite
 *				the buffer.
 *
 *	Input:
 *		int	y	-- Index of scanline to fetch
 */
SILine
v256_getsl(y)
int	y;
{
	register BYTE	*paddr;
	register int	i;
	int w;

	DBENTRY("v256_getsl()");

	i = v256_slbytes * y;			/* point to pixels */
	selectpage(i);
	paddr = (BYTE *)(v256_fb + (i & VIDEO_PAGE_MASK));
	if (i + v256_slbytes - 1 > v256_endpage) {
		w = v256_endpage - i + 1;
		memcpy(v256_slbuf, paddr, w);
		selectpage((i & ~VIDEO_PAGE_MASK) + (VIDEO_PAGE_MASK+1));
		memcpy(v256_slbuf+w, v256_fb, v256_slbytes - w);
		return((SILine)v256_slbuf);
	} else
		return((SILine)paddr);
}



/*
 *	v256_setsl(y, psl)	-- Set pixels in a scanline.
 *
 *	Input:
 *		int	y	-- Index of scanline to set
 *		SILine	psl	-- Pointer to scanline pixels
 */
void
v256_setsl(y, psl)
int	y;
SILine	psl;
{
	register BYTE	*paddr;
	register int	i;
	int w;

	DBENTRY("v256_setsl()");

	i = v256_slbytes * y;			/* point to pixels */
	selectpage(i);
	paddr = (BYTE *)(v256_fb + (i & VIDEO_PAGE_MASK));
	if (i + v256_slbytes - 1 > v256_endpage) {
		w = v256_endpage - i + 1;
		memcpy(paddr, v256_slbuf, w);
		selectpage((i & ~VIDEO_PAGE_MASK) + (VIDEO_PAGE_MASK+1));
		memcpy(v256_fb, v256_slbuf+w, v256_slbytes - w);
	} else {
		/* Do Nothing */
	}
}



/*
 *	v256_freesl(psl)		-- Free scanline buffer.  This doesn't
 *				do anything on the V256 because we use
 *				a static buffer for the scanline.
 *
 *	Input:
 *		SILine	psl	-- Pointer previously gotten scanline
 */
void
v256_freesl(psl)
SILine	psl;
{
	DBENTRY("v256_freesl()");
}
