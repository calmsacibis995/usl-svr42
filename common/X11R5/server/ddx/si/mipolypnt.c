/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mipolypnt.c	1.5"

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
/* $XConsortium: mipolypnt.c,v 5.0 89/06/09 15:08:37 keith Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "windowstr.h"
/* SI: START */
#include "scrnintstr.h"
#include "regionstr.h"
#include "si.h"
#include "sidep.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif
/* SI: END */

void
miPolyPoint(pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr 	pDrawable;
    GCPtr 		pGC;
    int 		mode;		/* Origin or Previous */
    int 		npt;
    xPoint 		*pptInit;
{

    int 		xorg;
    int 		yorg;
    int 		nptTmp;
    unsigned long	fsOld, fsNew;
    int			*pwidthInit, *pwidth;
    int			i;
    register xPoint 	*ppt;
    /* SI: START */
    RegionPtr           prgnDst;
    register BoxPtr     pbox;
    int        		nbox;
    int                 realclip = 0;
    xPoint              *pptCur;
    int                 nptCur;
    register xPoint     *ppt2;
    int			ret;
#ifdef XWIN_SAVE_UNDERS
    static void siTestXPts ();
#endif
    /* SI: END */

    /* make pointlist origin relative */
    if (mode == CoordModePrevious) {
        ppt = pptInit;
        nptTmp = npt;
	nptTmp--;
	if(pGC->miTranslate) {
	    ppt->x += pDrawable->x;
	    ppt->y += pDrawable->y;
	}
	while(nptTmp--) {
	    ppt++;
	    ppt->x += (ppt-1)->x;
	    ppt->y += (ppt-1)->y;
	}
    }
    else {
	if(pGC->miTranslate) {
	    ppt = pptInit;
	    nptTmp = npt;
	    xorg = pDrawable->x;
	    yorg = pDrawable->y;
	    while(nptTmp--) {
		ppt->x += xorg;
		ppt++->y += yorg;
	    }
	}
    }

    /* SI: START */
    if (pDrawable->type == DRAWABLE_WINDOW && si_hasplotpoint ) {
	prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;

#ifdef XWIN_SAVE_UNDERS
	/*
	 * Check to see if the point drawing conflicts with
	 * any save-under windows
	 */ 
	if (SUCheckDrawable(pDrawable))
	{
		siTestXPts(pDrawable, pGC, npt, pptInit);
	}

#endif
	nbox = REGION_NUM_RECTS (prgnDst);
	pbox = REGION_RECTS (prgnDst);
	if(!(ppt2 = pptCur = (xPoint *)ALLOCATE_LOCAL(nbox * npt * sizeof(xPoint))))
	    return;

	nptCur = 0;
	while(nbox--) {
	    ppt = pptInit;
	    nptTmp = npt;
	    while(nptTmp--) {
		if (ppt->x >= pbox->x1 && ppt->x < pbox->x2 &&
		    ppt->y >= pbox->y1 && ppt->y < pbox->y2) {
		    ppt2->x = ppt->x;
		    ppt2->y = ppt->y;
		    ppt2++;
		    nptCur++;
		}
		ppt++;
	    }
	    pbox++;
	}

	ret = SI_SUCCEED;

	if (nptCur) {
		si_PrepareGS(pGC);
		ret = si_plotpoints(nptCur, pptCur);
	}

	DEALLOCATE_LOCAL(pptCur);
	if(ret == SI_SUCCEED)
	   return;
    }

    /*
     * Don't have SDD routine or it's not appropriate here, use spans
     */
    fsOld = pGC->fillStyle;
    fsNew = FillSolid;
    if(pGC->fillStyle != FillSolid) {
	DoChangeGC(pGC, GCFillStyle, &fsNew, 0);
	ValidateGC(pDrawable, pGC);
    }

    if(!(pwidthInit = (int *)ALLOCATE_LOCAL(npt * sizeof(int))))
	return;
    pwidth = pwidthInit;
    for(i = 0; i < npt; i++)
	*pwidth++ = 1;
    (*pGC->ops->FillSpans)(pDrawable, pGC, npt, pptInit, pwidthInit, FALSE);

    if(fsOld != FillSolid) {
	DoChangeGC(pGC, GCFillStyle, &fsOld, 0);
	ValidateGC(pDrawable, pGC);
    }

    DEALLOCATE_LOCAL(pwidthInit);
}

#ifdef XWIN_SAVE_UNDERS
static void
siTestXPts(pDraw, pGC, npts, pPts)
DrawablePtr pDraw;
GCPtr   pGC;
int	npts;
xPoint  *pPts;
{
    register int i, xMin, xMax, yMin, yMax;
    int xorg, yorg;
    BoxRec box;

    xMin = yMin = MAXSHORT; 
    xMax = yMax = MINSHORT;
    for(i = 0; i < npts; i++)
    {
        xMin = min (xMin, pPts[i].x);
	yMin = min (yMin, pPts[i].y);
	xMax = max (xMax, pPts[i].x);
	yMax = max (yMax, pPts[i].y);
    }
    xorg = (int)pDraw->x;
    yorg = (int)pDraw->y;
    box.x1 = xMin + xorg;
    box.y1 = yMin + yorg;
    box.x2 = xMax + xorg;
    box.y2 = yMax + yorg;
    if (SUCheckBox(pDraw, &box))
    {
	siSUScanWindows(pDraw, pGC->subWindowMode, NULL, &box);
    }
}
#endif
