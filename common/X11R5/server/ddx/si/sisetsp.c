/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sisetsp.c	1.6"

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
/* $XConsortium: mfbsetsp.c,v 5.3 89/09/13 18:58:28 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"

#include "misc.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "si.h"			/* SI */
#include "simskbits.h"		/* SI */
#include "sidep.h"		/* SI */

extern unsigned int si_pfill();

/* siSetScanline -- copies the bits from psrc to the drawable starting at
 * (xStart, y) and continuing to (xEnd, y).  xOrigin tells us where psrc 
 * starts on the scanline. (I.e., if this scanline passes through multiple
 * boxes, we may not want to start grabbing bits at psrc but at some offset
 * further on.) 
 */
siSetScanline(y, xOrigin, xStart, xEnd, psrc, alu, pdstBase, widthDst, planemask)
    int			y;
    int			xOrigin;	/* where this scanline starts */
    int			xStart;		/* first bit to use from scanline */
    int			xEnd;		/* last bit to use from scanline + 1 */
    register int	*psrc;
    register int	alu;		/* raster op */
    int			*pdstBase;	/* start of the drawable */
    int			widthDst;	/* width of drawable in words */
    long		planemask;	/* SI */
{
    int			w;		/* width of scanline in bits */
    register int	*pdst;		/* where to put the bits */
    register int	tmpSrc;		/* scratch buffer to collect bits in */
    int			dstBit;		/* offset in bits from beginning of 
					 * word */
    register int	nstart; 	/* number of bits from first partial */
    register int	nend; 		/* " " last partial word */
    int			offSrc;
    int		startmask, endmask, nlMiddle, nl;

    pdst = pdstBase + (y * widthDst) + (xStart >> PWSH);
    psrc += (xStart - xOrigin) >> PWSH;
    offSrc = (xStart - xOrigin) & PIM;
    w = xEnd - xStart;
    dstBit = xStart & PIM;

    if (dstBit + w <= PPW)
    { 
     	getbits(psrc, offSrc, w, tmpSrc);
	putbitsrop((unsigned int) tmpSrc, dstBit, w, pdst, planemask, alu);
    } 
    else 
    { 
	maskbits(xStart, w, startmask, endmask, nlMiddle);
	if (startmask) 
	    nstart = PPW - dstBit;
	else 
	    nstart = 0; 
	if (endmask) 
	    nend = xEnd & PIM;
	else 
	    nend = 0; 
	if (startmask) 
	{ 
            getbits(psrc, offSrc, nstart, tmpSrc);
            putbitsrop((unsigned int) tmpSrc, dstBit, nstart, pdst, planemask, alu);
	    pdst++; 
	    offSrc += nstart;
	    if (offSrc > PLST)
	    {
		psrc++;
		offSrc -= PPW;
	    }
	} 
	nl = nlMiddle; 
	while (nl--) 
	{ 
            getbits(psrc, offSrc, PPW, tmpSrc);
            putbitsrop((unsigned int) tmpSrc, 0, PPW, pdst, planemask, alu );
	    pdst++; 
	    psrc++; 
	} 
	if (endmask) 
	{ 
            getbits(psrc, offSrc, nend, tmpSrc);
            putbitsrop((unsigned int) tmpSrc, 0, nend, pdst, planemask, alu);
	} 
	 
    } 
}

/* SetSpans -- for each span copy pwidth[i] bits from psrc to pDrawable at
 * ppt[i] using the raster op from the GC.  If fSorted is TRUE, the scanlines
 * are in increasing Y order.
 * Source bit lines are server scanline padded so that they always begin
 * on a word boundary.
 */ 
void
siSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    int			*psrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    int 		*pdstBase;	/* start of dst bitmap */
    int			*pdst;		/* SI (destination bitmap) */
    int 		widthDst;	/* width of bitmap in words */
    register BoxPtr 	pbox, pboxLast, pboxTest;
    register DDXPointPtr pptLast;
    int 		alu;
    RegionPtr 		prgnDst;
    int			xStart, xEnd;
    int			lStart, lEnd;	/* SI */
    int			yMax;
    int			isscr = 0;	/* SI */
    int			msbblt = 0;	/* SI */
    SIbitmap		tmpBmap;	/* SI */
    unsigned int	planemask;

    /* SI: start */
    PPW = 32 / pDrawable->depth;
    PWSH = si_pix_to_word[PPW];
    PSZ = pDrawable->depth;
    planemask = si_pfill(pDrawable->depth, pGC->planemask);
    if ((pDrawable->depth == 1) && (pDrawable->type != DRAWABLE_WINDOW)) {
            mfbSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
            return;
    }
    /* SI: end */

    alu = pGC->alu;
    prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;		/* SI (mfbGCPrivateIndex to siGCPrivateIndex) */

    pptLast = ppt + nspans;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	/* SI 
	pdstBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		   ((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind
		    >> 2;
	SI */
	/* SI: start */
        widthDst = si_getscanlinelen;
        yMax = si_getscanlinecnt;
        isscr++;
        if (si_hasmsbitblt) {
	    SIVisualP pVisuals;			/* SI */

       	    msbblt++;
	    pVisuals = si_GetInfoVal(SIvisuals);
	    tmpBmap.Bdepth = pVisuals->SVdepth;
	    tmpBmap.Bheight = 1;
	}
	/* SI: end */
    }
    else
    {
	pdstBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
	yMax = pDrawable->y + (int) pDrawable->height;	/* SI */
    }

    pbox =  REGION_RECTS(prgnDst);
    pboxLast = pbox + REGION_NUM_RECTS(prgnDst);

    if(fSorted)
    {
    /* scan lines sorted in ascending order. Because they are sorted, we
     * don't have to check each scanline against each clip box.  We can be
     * sure that this scanline only has to be clipped to boxes at or after the
     * beginning of this y-band 
     */
	pboxTest = pbox;
	while(ppt < pptLast)
	{
	    pbox = pboxTest;
	    if(ppt->y >= yMax)
		break;
	    /* SI: start */
            if (isscr)
		pdst = (int *)si_getscanline(ppt->y);
            if (msbblt) {
		lStart = si_getscanlinelen;
		lEnd = 0;
            }
	    /* SI: end */
	    while(pbox < pboxLast)
	    {
		if(pbox->y1 > ppt->y)
		{
		    /* scanline is before clip box */
		    break;
		}
		else if(pbox->y2 <= ppt->y)
		{
		    /* clip box is before scanline */
		    pboxTest = ++pbox;
		    continue;
		}
		else if(pbox->x1 > ppt->x + *pwidth) 
		{
		    /* clip box is to right of scanline */
		    break;
		}
		else if(pbox->x2 <= ppt->x)
		{
		    /* scanline is to right of clip box */
		    pbox++;
		    continue;
		}

		/* at least some of the scanline is in the current clip box */
		xStart = max(pbox->x1, ppt->x);
		lStart = min( lStart, xStart);		/* SI */
		xEnd = min(ppt->x + *pwidth, pbox->x2);
		/* SI: start */
		lEnd = max( lEnd, xEnd );
		if (isscr)
                    siSetScanline(0, ppt->x, xStart, xEnd, psrc, alu,
                    pdst, 0, planemask);
		else
                    siSetScanline(ppt->y, ppt->x, xStart, xEnd, psrc, alu,
                    pdstBase, widthDst, planemask);
		/* SI: end */
		/* SI
		mfbSetScanline(ppt->y, ppt->x, xStart, xEnd, psrc, alu,
			       pdstBase, widthDst);
		*/	

		if(ppt->x + *pwidth <= pbox->x2)
		{
		    /* End of the line, as it were */
		    break;
		}
		else
		    pbox++;
	    }
	    /* We've tried this line against every box; it must be outside them
	     * all.  move on to the next point */
	    /* SI: start */
            if (msbblt) {
		tmpBmap.Bwidth = lEnd;
		tmpBmap.Bptr = (SIArray) pdst;
		si_MSbitblt(&tmpBmap,
                             lStart, lEnd, lStart, ppt->y, lEnd - lStart, 1);
            } else if (isscr)
		si_setscanline(ppt->y, pdst);
	    /* SI: end */
	    ppt++;
	    psrc += PixmapWidthInPadUnits(*pwidth, pDrawable->depth);
	    pwidth++;
	}
    }
    else
    {
    /* scan lines not sorted. We must clip each line against all the boxes */
	while(ppt < pptLast)
	{
	    if(ppt->y >= 0 && ppt->y < yMax)
	    {
		
		for(pbox = REGION_RECTS(prgnDst); pbox< pboxLast; pbox++)
		{
		    if(pbox->y1 > ppt->y)
		    {
			/* rest of clip region is above this scanline,
			 * skip it */
			break;
		    }
		    if(pbox->y2 <= ppt->y)
		    {
			/* clip box is below scanline */
			pbox++;
			break;
		    }
		    if(pbox->x1 <= ppt->x + *pwidth &&
		       pbox->x2 > ppt->x)
		    {
			xStart = max(pbox->x1, ppt->x);
			xEnd = min(pbox->x2, ppt->x + *pwidth);
			siSetScanline(ppt->y, ppt->x, xStart, xEnd, psrc, alu, 
				      pdstBase, widthDst, planemask);
		    }

		}
	    }
	psrc += PixmapWidthInPadUnits(*pwidth, pDrawable->depth);
	ppt++;
	pwidth++;
	}
    }
}

