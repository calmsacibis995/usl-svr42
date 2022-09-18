/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mizerline.c	1.3"

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
/* $XConsortium: mizerline.c,v 5.3 89/11/25 12:30:33 rws Exp $ */

#include "X.h"

#include "misc.h"
#include "scrnintstr.h"
#include "regionstr.h"	/* SI */
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"
#include "si.h"
#include "sidep.h"

void
miZeroLine(dst, pgc, mode, nptInit, pptInit)
DrawablePtr dst;
GCPtr pgc;
int mode;
int nptInit;		/* number of points in polyline */
DDXPointRec *pptInit;	/* points in the polyline */
{
    int xorg, yorg;
    register DDXPointRec *ppt;
    int npt;

    DDXPointRec pt1, pt2;
    RegionPtr prgnDst;

    int dx, dy, adx, ady;
    int signdx, signdy, len;
    int du, dv;
    int e, e1, e2;

    int x,y;		/* current point on the line */
    int i;

    DDXPointPtr pspanInit;
    int *pwidthInit;
    int list_len = dst->height;
    Bool local = TRUE;

    /* SI: start */
    if (nptInit <= 1)
	return;

    if (pgc->lineStyle != LineSolid) {
        siZeroDashLine(dst, pgc, mode, nptInit, pptInit);
        return;
    }
    /* SI: end */

    ppt = pptInit;
    npt = nptInit;

    if (pgc->miTranslate)
    {
	xorg = dst->x;
	yorg = dst->y;

        if (mode == CoordModeOrigin) 
        {
         	for (i = npt; --i >= 0;)
		{
                    ppt->x += xorg;
                    ppt++->y += yorg;
		}
        }
        else 
        {
            ppt->x += xorg;
            ppt++->y += yorg;
            for (i = npt-1; --i >= 0;)
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
            for (i = npt-1; --i >= 0;) 	/* SI */
            {
             	ppt->x += (ppt-1)->x;
		ppt->y += (ppt-1)->y;
		ppt++;
            }
	}
    }

    if (dst->type == DRAWABLE_WINDOW && si_haslinedraw &&
	pgc->fillStyle == FillSolid) {

	register BoxPtr pbox;
	register int	nbox;

	ppt = &pptInit[nptInit-1];	/* last point */
	/*
	 * We don't want to paint the last point if the style is CapNotLast
	 * or the first point is the same as the last point.
	 */
	if ((pgc->capStyle == CapNotLast) || 
	    ((ppt->x == pptInit->x) && (ppt->y == pptInit->y))) {
	    /*
	     * Calculate the cap not last point by calculating
	     * 1 pixel using bresenhams algorithm.
	     */
	    dx = (ppt-1)->x - ppt->x;
	    dy = (ppt-1)->y - ppt->y;
	    adx = abs(dx);
	    ady = abs(dy);
	    signdx = sign(dx);
	    signdy = sign(dy);
    
	    x = ppt->x;
	    y = ppt->y;
	    if (adx > ady) {
	        /* X_AXIS */
		e = (ady << 1) - adx;
	        if (adx > 1) {
		    if (((signdx > 0) && (e < 0)) ||
		        ((signdx <=0) && (e <=0))
		       ) {
		        x+= signdx;
		    } else {
		        /* initialize next span */
		        x += signdx;
		        y += signdy;
		    }
	        }
	    }
	    else
	    {
	        /* Y_AXIS */
		e = (adx << 1) - ady;
	        if (ady > 1) {
		    if (((signdx > 0) && (e < 0)) ||
		        ((signdx <=0) && (e <=0))
		       ) {
			;
		    } else {
		        x += signdx;
		    }
		    y += signdy;
	        }
	    }
    
	    /* (x,y) is the last pixel location */
	    ppt->x = x;
	    ppt->y = y;
	}

	si_PrepareGS(pgc);
	if (si_hascliplist(SIavail_line)) {
		if (si_onebitlinedraw(nptInit, pptInit) == SI_FAIL)
		    goto error;
		return;
	}

	prgnDst = ((siPrivGC *)(pgc->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
	nbox = REGION_NUM_RECTS(prgnDst);
	pbox = REGION_RECTS(prgnDst);

	while(nbox--) {
	    CHECKINPUT();
	    si_setlineclip(pbox->x1, pbox->y1, pbox->x2 - 1, pbox->y2 - 1);
	    if (si_onebitlinedraw(nptInit, pptInit) == SI_FAIL) {
		si_setlineclip(0,0,si_getscanlinelen-1,si_getscanlinecnt-1);
		goto error;
	    }
	    pbox++;
	}
	si_setlineclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
	return;
    }

    /*
     * Either the SDD doesn't have line drawing or the line drawing failed.
     * Draw using spans.
     */
error:
    pspanInit = (DDXPointPtr)ALLOCATE_LOCAL(list_len * sizeof(DDXPointRec));
    pwidthInit = (int *)ALLOCATE_LOCAL(list_len * sizeof(int));
    if (!pspanInit || !pwidthInit)
	return;

    ppt = pptInit;
    npt = nptInit;

    while (--npt)
    {
	DDXPointPtr pspan;
 	int *pwidth;
	int width;

	pt1 = *ppt++;
	pt2 = *ppt;
	dx = pt2.x - pt1.x;
	dy = pt2.y - pt1.y;
	adx = abs(dx);
	ady = abs(dy);
	signdx = sign(dx);
	signdy = sign(dy);

	if (adx > ady)
	{
	    du = adx;
	    dv = ady;
	    len = adx;
	}
	else
	{
	    du = ady;
	    dv = adx;
	    len = ady;
	}

	e1 = dv * 2;
	e2 = e1 - 2*du;
	e = e1 - du;

	if (ady >= list_len)
	{
	    DDXPointPtr npspanInit;
	    int *npwidthInit;

	    if (local)
	    {
		DEALLOCATE_LOCAL(pwidthInit);
		pwidthInit = (int *)NULL;
		DEALLOCATE_LOCAL(pspanInit);
		pspanInit = (DDXPointPtr)NULL;
		local = FALSE;
	    }
	    list_len = ady + 1;
	    npspanInit = (DDXPointPtr)xrealloc(pspanInit,
					       sizeof(DDXPointRec) * list_len);
	    if (!npspanInit)
	    {
		list_len = 0;
		continue;
	    }
	    pspanInit = npspanInit;
	    npwidthInit = (int *)xrealloc(pwidthInit, sizeof(int) * list_len);
	    if (!npwidthInit)
	    {
		list_len = 0;
		continue;
	    }
	    pwidthInit = npwidthInit;
	}
	pspan = pspanInit;
	pwidth = pwidthInit;

	x = pt1.x;
	y = pt1.y;
	*pspan = pt1;
	if (adx > ady)
	{
	    /* X_AXIS */
	    width = 0;
	    while(len--)
	    {
		if (((signdx > 0) && (e < 0)) ||
		    ((signdx <=0) && (e <=0))
		   )
		{
		    e += e1;
		    x+= signdx;
		    width++;
		}
		else
		{
		    /* give this span a width */
		    width++;
		    *pwidth++ = width;

		    /* force the span the right way */
		    if (signdx < 0)
			pspan->x -= (width-1);

		    /* initialize next span */
		    x += signdx;
		    y += signdy;
		    e += e2;

		    width = 0;
		    pspan++;
		    pspan->x = x;
		    pspan->y = y;

		}
	    };
	    /* do the last span */
	    *pwidth++ = width;
	    if (signdx < 0)
		pspan->x -= (width-1);
	}
	else
	{
	    /* Y_AXIS */
	    while(len--)
	    {
		if (((signdx > 0) && (e < 0)) ||
		    ((signdx <=0) && (e <=0))
		   )
		{
		    e +=e1;
		}
		else
		{
		    x += signdx;
		    e += e2;
		}
		y += signdy;
		pspan++;
		pspan->x = x;
		pspan->y = y;
		*pwidth++ = 1;
	    };
	}
	(*pgc->ops->FillSpans)(dst, pgc, pwidth-pwidthInit,
			  pspanInit, pwidthInit, FALSE);
    }
    if (local)
    {
	DEALLOCATE_LOCAL(pwidthInit);	/* SI has this after FillSpans above */
	DEALLOCATE_LOCAL(pspanInit);	/* SI has this after FillSpans above */
    }
    else
    {
	xfree(pwidthInit);
	xfree(pspanInit);
    }

    if ((pgc->capStyle != CapNotLast) &&
	((ppt->x != pptInit->x) ||
	 (ppt->y != pptInit->y) ||
	 (ppt == pptInit + 1)))
    {
	int width = 1;
	(*pgc->ops->FillSpans)(dst, pgc, 1, ppt, &width, TRUE);
    }
} 


miZeroDashLine(dst, pgc, mode, nptInit, pptInit)
DrawablePtr dst;
GCPtr pgc;
int mode;
int nptInit;		/* number of points in polyline */
DDXPointRec *pptInit;	/* points in the polyline */
{
    /* XXX kludge until real zero-width dash code is written */
    pgc->lineWidth = 1;
    miWideDash (dst, pgc, mode, nptInit, pptInit);
    pgc->lineWidth = 0;
}
