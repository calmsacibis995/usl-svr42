/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sibstore.c	1.2"

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

/*-
 * sbbstore.c --
 *	Functions required by the backing-store implementation in MI.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$XConsortium: cfbbstore.c,v 5.4 89/09/14 17:04:20 rws Exp $ SPRITE (Berkeley)";
#endif

/* SI: old code
#include    "cfb.h"
*/
#include    "X.h"
#include    "regionstr.h"
#include    "scrnintstr.h"
#include    "pixmapstr.h"
#include    "windowstr.h"
#include    "mibstore.h"

extern WindowRec WindowTable[];

/*-
 *-----------------------------------------------------------------------
 * sbSaveAreas --
 *	Function called by miSaveAreas to actually fetch the areas to be
 *	saved into the backing pixmap. This is very simple to do, since
 *	sbDoBitblt is designed for this very thing. The region to save is
 *	already destination-relative and we're given the offset to the
 *	window origin, so we have only to create an array of points of the
 *	u.l. corners of the boxes in the region translated to the screen
 *	coordinate system and fetch the screen pixmap out of its devPrivate
 *	field....
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Data are copied from the screen into the pixmap.
 *
 *-----------------------------------------------------------------------
 */
void
sbSaveAreas(pWin, pPixmap, prgnSave, xorg, yorg)
    WindowPtr		pWin;		/* SI: added arg */
    PixmapPtr	  	pPixmap;  	/* Backing pixmap */
    RegionPtr	  	prgnSave; 	/* Region to save (pixmap-relative) */
    int	    	  	xorg;	    	/* X origin of region */
    int	    	  	yorg;	    	/* Y origin of region */
{
    register DDXPointPtr pPt;
    DDXPointPtr		pPtsInit;
    register BoxPtr	pBox;
    register int	i;
    
    i = REGION_NUM_RECTS(prgnSave);
    pPtsInit = (DDXPointPtr)ALLOCATE_LOCAL(i * sizeof(DDXPointRec));

    pBox = REGION_RECTS(prgnSave);
    pPt = pPtsInit;
    while (--i >= 0) {
	pPt->x = pBox->x1 + xorg;
	pPt->y = pBox->y1 + yorg;
	pPt++;
	pBox++;
    }

    /* SI: start */
    sbCopyBitblt(pWin,
	        &WindowTable[pPixmap->drawable.pScreen->myNum],
		pPixmap,
		prgnSave,
		pPtsInit, ~0L); /* LES: verify last arg new to r4 */
    /* SI: end */
    DEALLOCATE_LOCAL (pPtsInit);
}

/*-
 *-----------------------------------------------------------------------
 * sbRestoreAreas --
 *	Function called by miRestoreAreas to actually fetch the areas to be
 *	restored from the backing pixmap. This is very simple to do, since
 *	sbDoBitblt is designed for this very thing. The region to restore is
 *	already destination-relative and we're given the offset to the
 *	window origin, so we have only to create an array of points of the
 *	u.l. corners of the boxes in the region translated to the pixmap
 *	coordinate system and fetch the screen pixmap out of its devPrivate
 *	field....
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Data are copied from the pixmap into the screen.
 *
 *-----------------------------------------------------------------------
 */
void
sbRestoreAreas(pWin, pPixmap, prgnRestore, xorg, yorg)
    WindowPtr		pWin;		/* SI: added arg */
    PixmapPtr	  	pPixmap;  	/* Backing pixmap */
    RegionPtr	  	prgnRestore; 	/* Region to restore (screen-relative)*/
    int	    	  	xorg;	    	/* X origin of window */
    int	    	  	yorg;	    	/* Y origin of window */
{
    register DDXPointPtr pPt;
    DDXPointPtr		pPtsInit;
    register BoxPtr	pBox;
    register int	i;
    
    i = REGION_NUM_RECTS(prgnRestore);
    pPtsInit = (DDXPointPtr)ALLOCATE_LOCAL(i*sizeof(DDXPointRec));

    pBox = REGION_RECTS(prgnRestore);
    pPt = pPtsInit;
    while (--i >= 0) {
	pPt->x = pBox->x1 - xorg;
	pPt->y = pBox->y1 - yorg;
	pPt++;
	pBox++;
    }

    /* SI: start */
    sbCopyBitblt(pWin,
		pPixmap,
		&WindowTable[pPixmap->drawable.pScreen->myNum],
		prgnRestore,
		pPtsInit, ~0L); /* LES: verify last arg new to r4 */
    /* SI: end */

    DEALLOCATE_LOCAL (pPtsInit);
}
