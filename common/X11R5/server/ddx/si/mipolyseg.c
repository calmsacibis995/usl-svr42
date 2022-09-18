/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mipolyseg.c	1.3"

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
/* $XConsortium: mipolyseg.c,v 5.0 89/06/09 15:08:39 keith Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmap.h"
/* SI: START */
#include "scrnintstr.h"
#include "windowstr.h"
#include "regionstr.h"
#include "si.h"
#include "sidep.h"
/* SI: END */


/*****************************************************************
 * miPolySegment
 *
 *    For each segment, draws a line between (x1, y1) and (x2, y2).  The
 *    lines are drawn in the order listed.
 *
 *    Walks the segments, compressing them into format for PolyLines.
 *    
 *****************************************************************/


void
miPolySegment(pDraw, pGC, nseg, pSegs)
    DrawablePtr pDraw;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    /* int i; SI: commented this line */
    /* SI: START */
    xSegment *pS = pSegs;
    register DDXPointPtr ppt;
    DDXPointPtr pptTmp;
    register int i, nbox;
    int xorg, yorg, segcnt;
    int dx, dy, adx, ady, e;
    int signdx, signdy;
    int abort = 0;
    BoxPtr pbox;
    RegionPtr prgnDst;

    if ((si_haslineseg || si_haslinedraw) && pGC->lineWidth == 0 &&
	pDraw->type == DRAWABLE_WINDOW && pGC->fillStyle == FillSolid &&
	pGC->lineStyle == LineSolid) {

	xorg = pDraw->x;
	yorg = pDraw->y;

	if (!si_hascliplist(SIavail_line)) {
	    prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
	}

	pptTmp = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * (nseg<<1));

	ppt = pptTmp;
	segcnt = 0;
	for (i = nseg; --i >= 0; pS++) {
	    if (pS->x1 == pS->x2 && pS->y1 == pS->y2)
		continue;
	    ppt->x = pS->x1 + xorg;
	    ppt->y = pS->y1 + yorg;
	    ppt++;
	    ppt->x = pS->x2 + xorg;
	    ppt->y = pS->y2 + yorg;
	    if (pGC->capStyle == CapNotLast) {
		dx = pS->x1 - pS->x2;
		dy = pS->y1 - pS->y2;
		adx = abs(dx);
		ady = abs(dy);
		signdx = sign(dx);
		signdy = sign(dy);

		if (adx > ady) {
		    /* X AXIS */
		    e = (ady << 1) - adx;
		    if (adx > 1) {
			if (((signdx > 0) && (e < 0)) ||
			    ((signdx <=0) && (e <=0))
			   ) {
			    ppt->x += signdx;
			   } else {
			    ppt->x += signdx;
			    ppt->y += signdy;
			   }
		    }
		} else {
		    /* Y AXIS */
		    e = (adx << 1) - ady;
		    if (ady > 1) {
			if (((signdx > 0) && (e < 0)) ||
			    ((signdx <=0) && (e <=0))
			   ) {
			    ;
			   } else {
			    ppt->x += signdx;
			   }
		        ppt->y += signdy;
		    }
		}
	    }
	    ppt++;
	    segcnt++;
	}

	si_PrepareGS(pGC);
	if (si_hascliplist(SIavail_line)) {
	    if (si_haslineseg) {
		if (si_onebitlineseg(segcnt << 1, pptTmp) == SI_FAIL) {
		    abort = 1;
		    goto error;
		}
	    } else {
		ppt = pptTmp;
		for (i=segcnt; --i >= 0; ppt += 2) {
		    if (si_onebitlinedraw(2, ppt) == SI_FAIL) {
			abort = 1;
			goto error;
		    }
		}
	    }
	    DEALLOCATE_LOCAL(pptTmp);
	    return;
	}

	nbox = REGION_NUM_RECTS (prgnDst);
	pbox = REGION_RECTS (prgnDst);

	while(--nbox >= 0) {
	    CHECKINPUT();
	    si_setlineclip(pbox->x1, pbox->y1, pbox->x2 - 1, pbox->y2 - 1);
	    if (si_haslineseg) {
		if (si_onebitlineseg(segcnt << 1, pptTmp) == SI_FAIL) {
		    abort = 1;
		    goto error;
		}
	    } else {
		ppt = pptTmp;
		for (i=segcnt; --i >= 0; ppt += 2) {
		    if (si_onebitlinedraw(2, ppt) == SI_FAIL) {
			abort = 1;
			goto error;
		    }
		}
	    }
	    pbox++;
	}
	if (!si_hascliplist(SIavail_line))
	    si_setlineclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
	DEALLOCATE_LOCAL(pptTmp);
	return;
    }

error:
    if (abort) {
	if (!si_hascliplist(SIavail_line))
	    si_setlineclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
	DEALLOCATE_LOCAL(pptTmp);
    }
    /* SI: END */

    for (i=0; i<nseg; i++)
    {
        (*pGC->ops->Polylines)(pDraw, pGC, CoordModeOrigin, 2,(DDXPointPtr)pSegs);
    	pSegs++;
    }
}
