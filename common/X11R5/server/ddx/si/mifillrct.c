/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mifillrct.c	1.5"

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

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: mifillrct.c,v 5.0 89/06/09 15:08:22 keith Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "gcstruct.h"
/* SI: start */
#include "scrnintstr.h"
#include "regionstr.h"
#include "mi.h"
#include "miscstruct.h"
#include "si.h"
#include "sidep.h"
/* SI: end */
#include "windowstr.h"
#include "pixmap.h"

#include "misc.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

/* mi rectangles
   written by newman, with debts to all and sundry
*/

/* MIPOLYFILLRECT -- public entry for PolyFillRect request
 * very straight forward: translate rectangles if necessary
 * then call FillSpans to fill each rectangle.  We let FillSpans worry about
 * clipping to the destination
 */
void
miPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
DrawablePtr	pDrawable;
GCPtr	pGC;
int		nrectFill; 	/* number of rectangles to fill */
xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
	int 			i, width, height;
	register xRectangle 	*prect; 
	int			xorg, yorg;
	int			maxheight;
	DDXPointPtr		ppt, pptFirst;
	int			*pw, *pwFirst;
	SIRectP			sirectFirst = 0;
	register SIRectP	sirect;
	DDXPointRec	        tmpRect[4];
	RegionPtr	        prgnDst;
	BoxPtr	     		pbox;
	BoxRec	        	bbox, clip;
	int		     	nbox;

	if (!nrectFill)
		return;

#ifdef XWIN_SAVE_UNDERS
    	/*
	 * Check to see if the drawable conflicts with
	 * any save-under windows
	 */ 
	if (SUCheckDrawable(pDrawable))
	{
		siTestRects(pDrawable, pGC, nrectFill, prectInit);
	}
#endif

	if (pGC->miTranslate) {
		xorg = pDrawable->x;
		yorg = pDrawable->y;
		if ((xorg != 0) || (yorg != 0)) {
			prect = prectInit;
			for (i = 0; i<nrectFill; i++, prect++) {
				prect->x += xorg;
				prect->y += yorg;
			}
		}
	}

	/* SI: start */
	if (pDrawable->type == DRAWABLE_WINDOW && si_hasfillrectangle) {
        	prect = prectInit;
		if ((sirect=sirectFirst=(SIRectP)ALLOCATE_LOCAL(nrectFill * 
						       sizeof(SIRect))) == 0)
			return;

		for (i = 0; i < nrectFill; i++, sirect++, prect++) {
			sirect->ul.x = prect->x;
			sirect->ul.y = prect->y;
			sirect->lr.x = prect->x + prect->width;
			sirect->lr.y = prect->y + prect->height;
		}

		si_PrepareGS(pGC);
#ifndef FLUSH_IN_BH
		si_Initcache();
#endif
		if (si_hascliplist(SIavail_fpoly)) {
			if (si_fillrectangle(nrectFill, 
					     sirectFirst) == SI_SUCCEED)
				goto done;
		}
		else {
			prgnDst = ((siPrivGC *)(pGC->devPrivates[
				   siGCPrivateIndex].ptr))->pCompositeClip;
			nbox = REGION_NUM_RECTS(prgnDst);
			pbox = REGION_RECTS(prgnDst);

			while (--nbox >= 0) {
				CHECKINPUT();
				si_setpolyclip(pbox->x1, pbox->y1,
					       pbox->x2-1, pbox->y2-1);
				if (si_fillrectangle(nrectFill, 
						     sirectFirst) == SI_FAIL)
					goto error;
				pbox++;
			}
			si_setpolyclip(0, 0, si_getscanlinelen-1, 
				       si_getscanlinecnt-1);
			goto done;
		}
	}

	if (pDrawable->type == DRAWABLE_WINDOW && si_canpolyfill) {
        	prect = prectInit;
		si_PrepareGS(pGC);
#ifndef FLUSH_IN_BH
		si_Initcache();
#endif
		for (i = 0; i<nrectFill; i++, prect++) {
			tmpRect[0].x = tmpRect[3].x = prect->x;
			tmpRect[0].y = tmpRect[1].y = prect->y;
			tmpRect[1].x = tmpRect[2].x = prect->x + prect->width;

			if (si_hascliplist(SIavail_fpoly)) {
				if (si_hasconvexfpolygon) {
					if (si_fillconvexpoly(4, tmpRect) == 
								SI_FAIL) 
						goto error;
				} 
				else if (si_hasgeneralfpolygon) {
					if (si_fillgeneralpoly(4, tmpRect) == 
								SI_FAIL)
						goto error;
				}
				continue;
			}

			prgnDst = ((siPrivGC *)(pGC->devPrivates[
				   siGCPrivateIndex].ptr))->pCompositeClip;
			nbox = REGION_NUM_RECTS(prgnDst);
			pbox = REGION_RECTS(prgnDst);

			while (--nbox >= 0) {
				CHECKINPUT();
				si_setpolyclip(pbox->x1,pbox->y1,
					       pbox->x2-1,pbox->y2-1);
				if (si_hasconvexfpolygon) {
					if (si_fillconvexpoly(4, tmpRect) == 
								SI_FAIL) 
						goto error;
				} 
				else if (si_hasgeneralfpolygon) {
					if (si_fillgeneralpoly(4, tmpRect) == 
								SI_FAIL)
						goto error;
				}
				pbox++;
			}
		}
		if (!si_hascliplist(SIavail_fpoly))
			si_setpolyclip(0, 0, si_getscanlinelen-1, 
				       si_getscanlinecnt-1);
		goto done;
	}

error:
    /* Can't poly fill or rectangle fill failed */
    /* SI: end */

    prect = prectInit;
    maxheight = 0;
    for (i = 0; i<nrectFill; i++, prect++)
	maxheight = max(maxheight, (int)prect->height);

    pptFirst = (DDXPointPtr) ALLOCATE_LOCAL(maxheight * sizeof(DDXPointRec));
    pwFirst = (int *) ALLOCATE_LOCAL(maxheight * sizeof(int));
    if(!pptFirst || !pwFirst)
    {
	if (pwFirst) DEALLOCATE_LOCAL(pwFirst);
	if (pptFirst) DEALLOCATE_LOCAL(pptFirst);
	return;
    }

    prect = prectInit;
    while(nrectFill--)
    {
	ppt = pptFirst;
	pw = pwFirst;
	height = prect->height;
	width = prect->width;
	xorg = prect->x;
	yorg = prect->y;
	while(height--)
	{
	    *pw++ = width;
	    ppt->x = xorg;
	    ppt->y = yorg;
	    ppt++;
	    yorg++;
	}

	CHECKINPUT();

	(* pGC->ops->FillSpans)(pDrawable, pGC, 
			   prect->height, pptFirst, pwFirst,
			   1);
	prect++;
    }
    DEALLOCATE_LOCAL(pwFirst);
    DEALLOCATE_LOCAL(pptFirst);
    return;

done:
	if (sirectFirst)
		DEALLOCATE_LOCAL(sirectFirst);
#ifndef FLUSH_IN_BH
	si_Flushcache();
#endif
	return;

}
