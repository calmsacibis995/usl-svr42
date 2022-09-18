/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/mfb/mfbfillrct.c	1.3"

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

/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: mfbfillrct.c,v 5.5 89/11/24 18:04:31 rws Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "miscstruct.h"
#include "regionstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"

#define MODEQ(a, b) ((a) %= (b))
void mfbPaintOddSize();

/* 
    filled rectangles.
    translate the rectangles, clip them, and call the
helper function in the GC.
*/

void
mfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    int xorg, yorg;
    register int n;		/* spare counter */
    register xRectangle *prect; /* temporary */
    RegionPtr prgnClip;
    register BoxPtr pbox;	/* used to clip with */
    register BoxPtr pboxClipped;
    BoxPtr pboxClippedBase;
    register BoxPtr pextent;
    mfbPrivGC	*priv;
    int alu;
    void (* pfn) ();
    PixmapPtr ppix;
    int numRects;

    priv = (mfbPrivGC *) pGC->devPrivates[mfbGCPrivateIndex].ptr;
    alu = priv->ropFillArea;
    pfn = priv->FillArea;
    ppix = priv->pRotatedPixmap;
    prgnClip = priv->pCompositeClip;

    numRects = REGION_NUM_RECTS(prgnClip);
    pboxClippedBase = (BoxPtr)ALLOCATE_LOCAL(numRects * sizeof(BoxRec));

    if (!pboxClippedBase)
	return;

    prect = prectInit;
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    if (xorg || yorg)
    {
        prect = prectInit;
	n = nrectFill;
	Duff (n, prect->x += xorg; prect->y += yorg; prect++);
    }

    prect = prectInit;

    pextent = (*pGC->pScreen->RegionExtents)(prgnClip);

    while (nrectFill--)
    {
	BoxRec box;
	int	x2, y2;

	/*
	 * clip the box to the extent of the region --
	 * avoids overflowing shorts and minimizes other
	 * computations
	 */

	box.x1 = prect->x;
	if (box.x1 < pextent->x1)
		box.x1 = pextent->x1;

	box.y1 = prect->y;
	if (box.y1 < pextent->y1)
		box.y1 = pextent->y1;

	x2 = (int) prect->x + (int) prect->width;
	if (x2 > pextent->x2)
		x2 = pextent->x2;
	box.x2 = x2;

	y2 = (int) prect->y + (int) prect->height;
	if (y2 > pextent->y2)
		y2 = pextent->y2;
	box.y2 = y2;

	prect++;

	if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    continue;

	switch((*pGC->pScreen->RectIn)(prgnClip, &box))
	{
	  case rgnOUT:
	    break;
	  case rgnIN:
	    (*pfn)(pDrawable, 1, &box, alu, ppix);
	    break;
	  case rgnPART:
	    pboxClipped = pboxClippedBase;
	    pbox = REGION_RECTS(prgnClip);
	    n = numRects;

	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
	        pboxClipped->x1 = max(box.x1, pbox->x1);
	        pboxClipped->y1 = max(box.y1, pbox->y1);
	        pboxClipped->x2 = min(box.x2, pbox->x2);
	        pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

	        /* see if clipping left anything */
	        if(pboxClipped->x1 < pboxClipped->x2 && 
	           pboxClipped->y1 < pboxClipped->y2)
	        {
		    pboxClipped++;
	        }
	    }
	    (*pfn)(pDrawable, pboxClipped-pboxClippedBase, 
		   pboxClippedBase, alu, ppix);
	    break;
	}
    }
    DEALLOCATE_LOCAL(pboxClippedBase);
}
