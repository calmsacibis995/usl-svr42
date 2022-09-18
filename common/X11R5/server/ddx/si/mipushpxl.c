/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mipushpxl.c	1.4"

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
/* $XConsortium: mipushpxl.c,v 5.1 89/07/26 12:18:19 rws Exp $ */

#include "X.h"
#include "gc.h"			/* SI */
#include "gcstruct.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"		/* SI */
#include "regionstr.h"		/* SI */
#include "miscstruct.h"
#include "simskbits.h"		/* SI */
#include "si.h"			/* SI */
#include "sidep.h"		/* SI */

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

#define NPT 128

/* int siGCPrivateIndex = 0; */	/* SI (for NOW only) */

/* miPushPixels -- squeegees the fill style of pGC through pBitMap
 * into pDrawable.  pBitMap is a stencil (dx by dy of it is used, it may
 * be bigger) which is placed on the drawable at xOrg, yOrg.  Where a 1 bit
 * is set in the bitmap, the fill style is put onto the drawable using
 * the GC's logical function. The drawable is not changed where the bitmap
 * has a zero bit or outside the area covered by the stencil.

WARNING:
    this code works if the 1-bit deep pixmap format returned by GetSpans
is the same as the format defined by the mfb code (i.e. 32-bit padding
per scanline, scanline unit = 32 bits; later, this might mean
bitsizeof(int) padding and sacnline unit == bitsizeof(int).)

 */
void
miPushPixels(pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    int		h, dxDiv32, ibEnd;
    unsigned long *pwLineStart;
    register unsigned long	*pw, *pwEnd;
    register unsigned long msk;
    register int ib, w;
    register int ipt;		/* index into above arrays */
    Bool 	fInBox;
    DDXPointRec	pt[NPT], ptThisLine;
    int		width[NPT];
    /* SI: start */
    RegionPtr	prgnDst;
    BoxPtr	pbox;
    int		nbox;
    int		realclip = 0;
    SIbitmap	tmpBM;
    SIint32	csx, csy, cw, ch, cdx, cdy;
    extern unsigned int siendtab1[];
    /* SI: end */

    ipt = 0;
    dxDiv32 = dx/32;

    /* SI: start */
    PPW = 32 / pDrawable->depth;
    PWSH = si_pix_to_word[PPW];

    if (pDrawable->type == DRAWABLE_WINDOW && si_hasmsstplblt &&
	pGC->fillStyle == FillSolid) {
	/* prgnDst = ((siPrivGC *)(pGC->devPriv))->pCompositeClip;  R3 */
	prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip; 	/* SI (union in R4) */

        nbox = REGION_NUM_RECTS(prgnDst);	/* SI (R4) */
        pbox = REGION_RECTS(prgnDst);		/* SI (R4) */

	tmpBM.Bdepth = 1;
	tmpBM.Bwidth = (pBitMap->drawable.width + 31) & ~0x1F;	/* SI (R4) */
	tmpBM.Bheight = dy;
	tmpBM.BorgX = tmpBM.BorgY = 0;
	tmpBM.Bptr = (SIArray) pBitMap->devPrivate.ptr;		/* SI (R4) */

	si_PrepareGS(pGC);
	while(nbox--) {

#ifdef XWIN_SAVE_UNDERS
            /*
             * Check to see if the rect conflicts with
             * any save-under windows
             */ 
            if (SUCheckDrawable(pDrawable))
            {
		if (SUCheckBox(pDrawable, pbox))
		{
		    siSUScanWindows(pDrawable, pGC->subWindowMode, NULL, pbox);
		}
            }
#endif
	    /* Init blt bounds */
	    csx = 0; csy = 0;
	    cw = dx; ch = dy;
	    cdx = xOrg; cdy = yOrg;
		/* Clip to the rectangle */
	    if (	/* Is it anywhere close? */
			/* rectangle lower than glyph */
		 (pbox->y1 > (yOrg + dy) ) ||
			/* rectangle higher than glyph */
		 (pbox->y2 < yOrg) ||
			/* rectangle starts after glyph */
		 (pbox->x1 > (xOrg + dx) ) ||
			/* rectangle ends before glyph */
		 (pbox->x2 < xOrg)
	       ) {
		pbox++;
		continue;
	    }
	    if ( xOrg < pbox->x1 ) {		/* Clip Left side */
		cw -= csx = pbox->x1 - xOrg;
		cdx = pbox->x1;
	    }
	    if ( (xOrg + cw) > pbox->x2 ) {	/* Clip right side */
		cw = pbox->x2 - cdx;
	    }
	    if ( yOrg < pbox->y1 ) {		/* Clip Top */
		ch -= csy = pbox->y1 - yOrg;
		cdy = pbox->y1;
	    }
	    if ( (yOrg + ch) > pbox->y2 ) {	/* Clip Bottom */
		ch = pbox->y2 - cdy;
	    }
	    if (cw == 0 || ch == 0) {		/* Why Bother ? */
		pbox++;
		continue;
	    }
	    si_MSstplblt(&tmpBM, csx, csy, cdx, cdy, cw, ch, 0, SGStipple);
	    pbox++;
	}
	if(realclip)
	    (*pGC->pScreen->RegionDestroy)(prgnDst);
	return;
    }
    /* SI: end */

    pwLineStart = (unsigned long *)xalloc(PixmapBytePad(dx, 1));
    if (!pwLineStart)
	return;

    for(h = 0, ptThisLine.x = 0, ptThisLine.y = 0; 
	h < dy; 
	h++, ptThisLine.y++)
    {

	(*pBitMap->drawable.pScreen->GetSpans)(pBitMap, dx, &ptThisLine, &dx,
					       1, pwLineStart);

	pw = pwLineStart;
	/* Process all words which are fully in the pixmap */
	
	fInBox = FALSE;
	pwEnd = pwLineStart + dxDiv32;
	while(pw  < pwEnd)
	{
	    w = *pw;
	    msk = siendtab1[1];
	    for(ib = 0; ib < 32; ib++)
	    {
		if(w & msk)
		{
		    if(!fInBox)
		    {
			pt[ipt].x = ((pw - pwLineStart) << 5) + ib + xOrg;
			pt[ipt].y = h + yOrg;
			/* start new box */
			fInBox = TRUE;
		    }
		}
		else
		{
		    if(fInBox)
		    {
			width[ipt] = ((pw - pwLineStart) << 5) + 
				     ib + xOrg - pt[ipt].x;
			if (++ipt >= NPT)
			{
			    (*pGC->ops->FillSpans)(pDrawable, pGC, 
					      NPT, pt, width, TRUE);
			    ipt = 0;
			}
			/* end box */
			fInBox = FALSE;
		    }
		}
		msk = SCRRIGHT(msk, 1);
	    }
	    pw++;
	}
	ibEnd = dx & 0x1F;
	if(ibEnd)
	{
	    /* Process final partial word on line */
	    w = *pw;
	    msk = siendtab1[1];
	    for(ib = 0; ib < ibEnd; ib++)
	    {
		if(w & msk)
		{
		    if(!fInBox)
		    {
			/* start new box */
			pt[ipt].x = ((pw - pwLineStart) << 5) + ib + xOrg;
			pt[ipt].y = h + yOrg;
			fInBox = TRUE;
		    }
		}
		else
		{
		    if(fInBox)
		    {
			/* end box */
			width[ipt] = ((pw - pwLineStart) << 5) + 
				     ib + xOrg - pt[ipt].x;
			if (++ipt >= NPT)
			{
			    (*pGC->ops->FillSpans)(pDrawable, 
					      pGC, NPT, pt, width, TRUE);
			    ipt = 0;
			}
			fInBox = FALSE;
		    }
		}
		msk = SCRRIGHT(msk, 1);
	    }
	}
	/* If scanline ended with last bit set, end the box */
	if(fInBox)
	{
	    width[ipt] = dx + xOrg - pt[ipt].x;
	    if (++ipt >= NPT)
	    {
		(*pGC->ops->FillSpans)(pDrawable, pGC, NPT, pt, width, TRUE);
		ipt = 0;
	    }
	}
    }
    xfree(pwLineStart);
    /* Flush any remaining spans */
    if (ipt)
    {
	(*pGC->ops->FillSpans)(pDrawable, pGC, ipt, pt, width, TRUE);
    }
}
