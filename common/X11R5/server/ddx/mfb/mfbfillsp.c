/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/mfb/mfbfillsp.c	1.2"

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
/* $XConsortium: mfbfillsp.c,v 5.6 89/11/24 18:02:25 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "mfb.h"
#include "maskbits.h"

#include "servermd.h"

/* scanline filling for monochrome frame buffer
   written by drewry, oct 1986

   these routines all clip.  they assume that anything that has called
them has already translated the points (i.e. pGC->miTranslate is
non-zero, which is howit gets set in mfbCreateGC().)

   the number of new scnalines created by clipping ==
MaxRectsPerBand * nSpans.

    FillSolid is overloaded to be used for OpaqueStipple as well,
if fgPixel == bgPixel.  


    FillTiled is overloaded to be used for OpaqueStipple, if
fgPixel != bgPixel.  based on the fill style, it uses
{RotatedPixmap, gc.alu} or {RotatedPixmap, PrivGC.ropOpStip}
*/


void mfbBlackSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int *addrlBase;		/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register int *addrl;	/* pointer to current longword in bitmap */
    register int nlmiddle;
    register int startmask;
    register int endmask;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    if (!(pGC->planemask & 1))
	return;

    n = nInit * miFindMaxBand(((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip);
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans(((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip,
		    pptInit, pwidthInit, nInit,
		    ppt, pwidth, fSorted);

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    while (n--)
    {
        addrl = addrlBase + (ppt->y * nlwidth) + (ppt->x >> 5);

	if (*pwidth)
	{
	    if ( ((ppt->x & 0x1f) + *pwidth) < 32)
	    {
		/* all bits inside same longword */
		maskpartialbits(ppt->x, *pwidth, startmask);
		    *addrl &= ~startmask;
	    }
	    else
	    {
		maskbits(ppt->x, *pwidth, startmask, endmask, nlmiddle);
		if (startmask)
		    *addrl++ &= ~startmask;
		Duff (nlmiddle, *addrl++ = 0x0);
		if (endmask)
		    *addrl &= ~endmask;
	    }
	}
	pwidth++;
	ppt++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}



void mfbWhiteSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int *addrlBase;		/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register int *addrl;	/* pointer to current longword in bitmap */
    register int nlmiddle;
    register int startmask;
    register int endmask;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    if (!(pGC->planemask & 1))
	return;

    n = nInit * miFindMaxBand(((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip);
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans(((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip,
		    pptInit, pwidthInit, nInit,
		    ppt, pwidth, fSorted);

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	nlwidth = (int)
		 (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    while (n--)
    {
        addrl = addrlBase + (ppt->y * nlwidth) + (ppt->x >> 5);

	if (*pwidth)
	{
	    if ( ((ppt->x & 0x1f) + *pwidth) < 32)
	    {
		/* all bits inside same longword */
		maskpartialbits(ppt->x, *pwidth, startmask);
		*addrl |= startmask;
	    }
	    else
	    {
		maskbits(ppt->x, *pwidth, startmask, endmask, nlmiddle);
		if (startmask)
		    *addrl++ |= startmask;
		Duff (nlmiddle, *addrl++ = 0xffffffff);
		if (endmask)
		    *addrl |= endmask;
	    }
	}
	pwidth++;
	ppt++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}



void mfbInvertSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int *addrlBase;		/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register int *addrl;	/* pointer to current longword in bitmap */
    register int nlmiddle;
    register int startmask;
    register int endmask;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    if (!(pGC->planemask & 1))
	return;

    n = nInit * miFindMaxBand(((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip);
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans(((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip,
		    pptInit, pwidthInit, nInit,
		    ppt, pwidth, fSorted);

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    while (n--)
    {
        addrl = addrlBase + (ppt->y * nlwidth) + (ppt->x >> 5);

	if (*pwidth)
	{
	    if ( ((ppt->x & 0x1f) + *pwidth) < 32)
	    {
		/* all bits inside same longword */
		maskpartialbits(ppt->x, *pwidth, startmask);
		*addrl ^= startmask;
	    }
	    else
	    {
		maskbits(ppt->x, *pwidth, startmask, endmask, nlmiddle);
		if (startmask)
		    *addrl++ ^= startmask;
		Duff (nlmiddle, *addrl++ ^= 0xffffffff);
		if (endmask)
		    *addrl ^= endmask;
	    }
	}
	pwidth++;
	ppt++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}
