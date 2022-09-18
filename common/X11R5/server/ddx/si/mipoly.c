/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mipoly.c	1.3"

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
/* $XConsortium: mipoly.c,v 5.0 89/06/09 15:08:32 keith Exp $ */

/*
 *  mipoly.c
 *
 *  Written by Brian Kelleher; June 1986
 *
 *  Draw polygons.  This routine translates the point by the
 *  origin if pGC->miTranslate is non-zero, and calls
 *  to the appropriate routine to actually scan convert the
 *  polygon.
 */
#include "X.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "mi.h"
#include "miscstruct.h"
/* SI: START */
#include "scrnintstr.h"
#include "regionstr.h"
#include "si.h"
#include "sidep.h"

/* SI: END */

void
miFillPolygon(dst, pgc, shape, mode, count, pPts)
    DrawablePtr		dst;
    register GCPtr	pgc;
    int			shape, mode;
    register int	count;
    DDXPointPtr		pPts;
{
    int			i;
    register int	xorg, yorg;
    register DDXPointPtr ppt;
    /* SI: START */
    RegionPtr           prgnDst;
    BoxPtr              pbox;
    int                 nbox;
    /* SI: END */

    if (count == 0)
	return;

    ppt = pPts;
    if (pgc->miTranslate)
    {
	xorg = dst->x;
	yorg = dst->y;

        if (mode == CoordModeOrigin) 
        {
	        for (i = 0; i<count; i++) 
                {    
	            ppt->x += xorg;
	            ppt++->y += yorg;
	        }
        }
        else 
        {
	    ppt->x += xorg;
	    ppt++->y += yorg;
	    for (i = 1; i<count; i++) 
            {
	        ppt->x += (ppt-1)->x;
	        ppt->y += (ppt-1)->y;
	        ppt++;
	    }
        }
    }
    else
    {
	if (mode == CoordModePrevious)
        {
	    ppt++;
	    for (i = 1; i<count; i++) 
            {
	        ppt->x += (ppt-1)->x;
	        ppt->y += (ppt-1)->y;
	        ppt++;
	    }
        }
    }

    /* SI: START */
    if (!si_hascliplist(SIavail_fpoly)) {
	prgnDst = ((siPrivGC *)(pgc->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
	pbox = REGION_RECTS (prgnDst);
	nbox = REGION_NUM_RECTS (prgnDst);
    }

    if (shape == Convex) {
	if (dst->type == DRAWABLE_WINDOW && si_hasconvexfpolygon) {
	    si_PrepareGS(pgc);
	    if (si_hascliplist(SIavail_fpoly)) {
		if (si_fillconvexpoly(count, pPts) == SI_SUCCEED)
		    return;
	    }
	    else {
		while(nbox--) {
		    CHECKINPUT();
		    si_setpolyclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
		    if (si_fillconvexpoly(count, pPts) == SI_FAIL)
			break;
		    pbox++;
		}
		si_setpolyclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
		return;
	    }
	}
	/*
	 * No SDD routines, do it the hard way
	 */
	miFillConvexPoly(dst, pgc, count, pPts);
    } 
    else {
	if ((dst->type == DRAWABLE_WINDOW && si_hasgeneralfpolygon) &&
	    ((pgc->fillRule == EvenOddRule && si_canevenoddfill) ||
	    (pgc->fillRule == WindingRule && si_canwindingfill))) {

	    si_PrepareGS(pgc);
	    if (si_hascliplist(SIavail_fpoly)) {
		if (si_fillgeneralpoly(count, pPts) == SI_SUCCEED)
		    return;
	    }
	    else {
		while(nbox--) {
		    CHECKINPUT();
		    si_setpolyclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
		    if (si_fillgeneralpoly(count, pPts) == SI_FAIL)
			break;
		    pbox++;
		}
		si_setpolyclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
		return;
	    }
	}
	/*
	 * No SDD routines, do it the hard way
	 */
	miFillGeneralPoly(dst, pgc, count, pPts);
    }
}
