/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mibitblt.c	1.15"

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
/* $XConsortium: mibitblt.c,v 5.13 89/11/22 17:02:56 rws Exp $ */
/* Author: Todd Newman  (aided and abetted by Mr. Drewry) */

#include "X.h"
#include "Xprotostr.h"

#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "regionstr.h"
#include "Xmd.h"
#include "servermd.h"

/* SI: start */
#include "gc.h"
#include "miscstruct.h"
#include "si.h"
#include "simskbits.h"
#include "sidep.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

#define	    SRC_IS_S	1
#define	    SRC_IS_M	0
#define	    DST_IS_S	2
#define	    DST_IS_M	0
/* SI: end */

/* MICOPYAREA -- public entry for the CopyArea request
 * For each rectangle in the source region
 *     get the pixels with GetSpans
 *     set them in the destination with SetSpans
 * We let SetSpans worry about clipping to the destination.
 */
RegionPtr
miCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, xIn, yIn, widthSrc, heightSrc, xOut, yOut)
    register DrawablePtr 	pSrcDrawable;
    register DrawablePtr 	pDstDrawable;
    GCPtr 			pGC;
    int 			xIn, yIn;
    int 			widthSrc, heightSrc;
    int 			xOut, yOut;
{
    DDXPointPtr		ppt, pptFirst;
    unsigned int	*pwidthFirst, *pwidth, *pbits;
    BoxRec 		srcBox, *prect;
    			/* may be a new region, or just a copy */
    RegionPtr 		prgnSrcClip;
    			/* non-0 if we've created a src clip */
    RegionPtr		prgnExposed;
    int 		realSrcClip = 0;
    int			srcx, srcy, dstx, dsty, i, j, y, width, height,
    			xMin, xMax, yMin, yMax;
    unsigned int	*ordering;
    int			numRects;
    BoxPtr		boxes;
    int			careful;
/* SI: start */
    SIbitmap	        tmpBM, tmp2BM;
    int		        hardassist, assisttype;
    RegionPtr	        prgnDst;
    int		        dx, dy, nbox;
    DDXPointPtr	        pptTmp, pptNew1, pptNew2;
    BoxPtr	        pboxTmp, pboxNext, pboxBase;

/* SI: end */


    srcx = xIn + pSrcDrawable->x;
    srcy = yIn + pSrcDrawable->y;

/* SI: start */
	/* Fast ?? out for Memory to Memory operations */
    if (pSrcDrawable->type != DRAWABLE_WINDOW &&
	pDstDrawable->type != DRAWABLE_WINDOW &&
	pSrcDrawable->depth == 1 && pDstDrawable->depth == 1)
    {
	prgnExposed = mfbCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, xIn, yIn, widthSrc, heightSrc, xOut, yOut);
	return prgnExposed;
    }
/* SI: end */

    /* If the destination isn't realized, this is easy */
    if (pDstDrawable->type == DRAWABLE_WINDOW &&
	!((WindowPtr)pDstDrawable)->realized)
	return (RegionPtr)NULL;

    /* clip the source */
    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	BoxRec box;

	box.x1 = pSrcDrawable->x;
	box.y1 = pSrcDrawable->y;
	box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
	box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;

	prgnSrcClip = (*pGC->pScreen->RegionCreate)(&box, 1);
	realSrcClip = 1;
    }
    else
    {
	if (pGC->subWindowMode == IncludeInferiors)
	{
	    prgnSrcClip = NotClippedByChildren ((WindowPtr) pSrcDrawable);
	    realSrcClip = 1;
	} else
	    prgnSrcClip = &((WindowPtr)pSrcDrawable)->clipList;
    }

    /* If the src drawable is a window, we need to translate the srcBox so
     * that we can compare it with the window's clip region later on. */
    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx  + widthSrc;
    srcBox.y2 = srcy  + heightSrc;

    dstx = xOut;
    dsty = yOut;
    if (pGC->miTranslate)
    {
	dstx += pDstDrawable->x;
	dsty += pDstDrawable->y;
    }

/* SI: start */ /* CHECK FIRST FOR HARDWARE ASSISTS */
    hardassist = 0;
    assisttype = 0;
    if (pSrcDrawable->type == DRAWABLE_WINDOW) {	/* SS or SM */
	if (pDstDrawable->type == DRAWABLE_WINDOW) {	/* SS */
            if (si_hasssbitblt) {
		assisttype = SRC_IS_S | DST_IS_S;
		hardassist++;
            }
	} else {					/* SM */
            if (si_hassmbitblt) {
		assisttype = SRC_IS_S;
		hardassist++;
		/*
		 * SI:  Make sure we validate the source rectangle
		 */
		if (pSrcDrawable->pScreen->SourceValidate)
		    (*pSrcDrawable->pScreen->SourceValidate) (pSrcDrawable, 
							      xIn, yIn, 
							      widthSrc, 
							      heightSrc);
            }
	}
    } else { 						/* MS or MM */
	if (pDstDrawable->type == DRAWABLE_WINDOW) {	/* MS */
            if (si_hasmsbitblt) {
		assisttype = DST_IS_S;
		hardassist++;
            }
	}
#ifdef ix86
	else
	    hardassist++;	/* MM case is not really hardware assisted,
				 * but it's convenient to think of it as such.
				 */
#endif
    }

    /* PERFORM HARDWARE ASSISTED CODE */

    if (hardassist) {
      prgnDst = (*pGC->pScreen->RegionCreate) (&srcBox, 1);
      (pGC->pScreen->Intersect) (prgnDst, prgnDst, prgnSrcClip);

      dx = srcx - dstx;
      dy = srcy - dsty;

      /* clip the shape of the dst to the destination composite clip */
      (*pGC->pScreen->TranslateRegion)(prgnDst, -dx, -dy);
      (*pGC->pScreen->Intersect)(prgnDst, prgnDst,
		((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the dest region conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDstDrawable))
        {
            if (SUCheckRegion(pDstDrawable, prgnDst))
            {
	        siSUScanWindows(pDstDrawable, IncludeInferiors, prgnDst, NULL);
            }
        }
#endif
#ifdef OLD
      pptFirst = (DDXPointPtr)ALLOCATE_LOCAL( REGION_NUM_RECTS( prgnDst ) *
						sizeof(DDXPointRec));
      ppt = pptFirst;
      prect = REGION_RECTS( prgnDst );
      for(i = 0; i < REGION_NUM_RECTS( prgnDst ); i++, prect++, ppt++)
      {
	  ppt->x = prect->x1 + dx;
	  ppt->y = prect->y1 + dy;
      }

      prect = REGION_RECTS( prgnDst );
      nbox = REGION_NUM_RECTS( prgnDst );
#else
    /* XXX we have to err on the side of safety when both are windows,
     * because we don't know if IncludeInferiors is being used.
     */
     careful = ((pSrcDrawable == pDstDrawable) ||
	((pSrcDrawable->type == DRAWABLE_WINDOW) &&
	(pDstDrawable->type == DRAWABLE_WINDOW)));

      prect = REGION_RECTS( prgnDst );
      nbox = REGION_NUM_RECTS( prgnDst );
      pptFirst = ppt = (DDXPointPtr)
		ALLOCATE_LOCAL( nbox * sizeof(DDXPointRec));

      for(i = 0; i < nbox; i++, ppt++)
      {
	  ppt->x = prect[i].x1 + dx;
	  ppt->y = prect[i].y1 + dy;
      }
#endif
    
      {
	BoxPtr tpbox = prect;
	BoxPtr pboxNew1, pboxNew2;
    
	pboxNew1 = NULL;
	pptNew1 = NULL;
	pboxNew2 = NULL;
	pptNew2 = NULL;
	if (careful && (pptFirst->y < tpbox->y1) )
	{
	    /* walk source botttom to top */
    	/* ydir = -1; */
    
    	if (nbox > 1)
    	{
    	    /* keep ordering in each band, reverse order of bands */
    	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
    	    if(!pboxNew1)
	    {
    	        DEALLOCATE_LOCAL(pptFirst);
		(*pGC->pScreen->RegionDestroy)(prgnDst);
    		return NULL;
	    }
    	    pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
    	    if(!pptNew1)
    	    {
    	        DEALLOCATE_LOCAL(pptFirst);
    	        DEALLOCATE_LOCAL(pboxNew1);
		(*pGC->pScreen->RegionDestroy)(prgnDst);
    	        return NULL;
    	    }
    	    pboxBase = pboxNext = tpbox+nbox-1;
    	    while (pboxBase >= tpbox)
    	    {
    	        while ((pboxNext >= tpbox) && 
    		       (pboxBase->y1 == pboxNext->y1))
    		    pboxNext--;
    	        pboxTmp = pboxNext+1;
    	        pptTmp = pptFirst + (pboxTmp - tpbox);
    	        while (pboxTmp <= pboxBase)
    	        {
    		    *pboxNew1++ = *pboxTmp++;
    		    *pptNew1++ = *pptTmp++;
    	        }
    	        pboxBase = pboxNext;
    	    }
    	    pboxNew1 -= nbox;
    	    tpbox = pboxNew1;
    	    pptNew1 -= nbox;
    	    pptFirst = pptNew1;
	    }
	}
	else
	{
    	/* walk source top to bottom */
    	/* ydir = 1; */
	}
    
	if (careful && (pptFirst->x < tpbox->x1))
	{
    	/* walk source right to left */
	    /* xdir = -1; */
    
    	if (nbox > 1)
    	{
    	    /* reverse order of rects in each band */
    	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
    	    pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
    	    if(!pboxNew2 || !pptNew2)
    	    {
    		if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
    		if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
    		if (pboxNew1)
    		{
    		    DEALLOCATE_LOCAL(pptNew1);
    		    DEALLOCATE_LOCAL(pboxNew1);
    		}
    	        DEALLOCATE_LOCAL(pptFirst);
		(*pGC->pScreen->RegionDestroy)(prgnDst);
    	        return NULL;
    	    }
    	    pboxBase = pboxNext = tpbox;
    	    while (pboxBase < tpbox+nbox)
    	    {
    	        while ((pboxNext < tpbox+nbox) &&
    		       (pboxNext->y1 == pboxBase->y1))
    		    pboxNext++;
    	        pboxTmp = pboxNext;
    	        pptTmp = pptFirst + (pboxTmp - tpbox);
    	        while (pboxTmp != pboxBase)
    	        {
    		    *pboxNew2++ = *--pboxTmp;
    		    *pptNew2++ = *--pptTmp;
    	        }
    	        pboxBase = pboxNext;
    	    }
    	    pboxNew2 -= nbox;
    	    tpbox = pboxNew2;
    	    pptNew2 -= nbox;
    	    pptFirst = pptNew2;
    	}
	}
	else
	{
    	/* walk source left to right */
	    /* xdir = 1; */
	}
    
	/* free up stuff */
	if (pboxNew2)
	{
    	DEALLOCATE_LOCAL(pptNew2);
    	DEALLOCATE_LOCAL(pboxNew2);
	}
	if (pboxNew1)
	{
    	DEALLOCATE_LOCAL(pptNew1);
    	DEALLOCATE_LOCAL(pboxNew1);
	}
	prect = tpbox;
      }
    
      /* PERFORM THE HARDWARE ASSISTED BITBLT */
      ppt = pptFirst;

      tmpBM.BorgX = tmpBM.BorgY = 0;
      tmpBM.Btype = Z_BITMAP;
      if (assisttype == SRC_IS_S) {
	  si_PrepareGS2(((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GStateidx, 
			&((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GState);
      }
      else {
	  si_PrepareGS(pGC);
      }

#ifndef FLUSH_IN_BH
      si_Initcache();
#endif
      switch(assisttype) {
#ifdef ix86
      case SRC_IS_S | DST_IS_S:			/* SCREEN SCREEN */
#else
      default:					/* SCREEN SCREEN */
#endif
	  while(nbox--) {
	      width = prect->x2 - prect->x1;
	      height = prect->y2 - prect->y1;
	      si_SSbitblt(ppt->x, ppt->y,
			   prect->x1, prect->y1,
			   width, height);
	      prect++;
	      ppt++;
	  }
	  break;
      case SRC_IS_S:		/* SCREEN MEMORY */
	  tmpBM.Bdepth = pDstDrawable->depth;
	  tmpBM.Bwidth = (int)pDstDrawable->width;
	  tmpBM.Bheight = (int)pDstDrawable->height;
	  tmpBM.Bptr = (SIArray)(((PixmapPtr)pDstDrawable)->devPrivate.ptr);
	  while(nbox--) {
	      width = prect->x2 - prect->x1;
	      height = prect->y2 - prect->y1;
	      si_SMbitblt(&tmpBM,
			   ppt->x, ppt->y,
			   prect->x1, prect->y1,
			   width, height);
	      prect++;
	      ppt++;
	  }
	  break;
      case DST_IS_S:		/* MEMORY SCREEN */
	  tmpBM.Btype = Z_PIXMAP;
	  tmpBM.Bdepth = pSrcDrawable->depth;
	  tmpBM.Bwidth = (int)pSrcDrawable->width;
	  tmpBM.Bheight = (int)pSrcDrawable->height;
	  tmpBM.Bptr = (SIArray)(((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
	  while(nbox--) {
	      width = prect->x2 - prect->x1;
	      height = prect->y2 - prect->y1;
	      si_MSbitblt(&tmpBM,
			   ppt->x, ppt->y,
			   prect->x1, prect->y1,
			   width, height);
	      prect++;
	      ppt++;
	  }
	  break;
#ifdef ix86
	/*
	 * For Intel architecture, we use an optimized version of mem->mem copy
	 * routines; These routines have some assembly stuff; so do not compile
	 * these routines on non-Intel architectures.
	 * These routines are in sirop.c 
	 * For non-intel architectures, general (slower) routines are used.
	 */
      case 0:		/* MEMORY MEMORY */
	  tmpBM.Btype = Z_PIXMAP;
	  tmpBM.Bdepth = pSrcDrawable->depth;
	  tmpBM.Bwidth = (int)pSrcDrawable->width;
	  tmpBM.Bheight = (int)pSrcDrawable->height;
	  tmpBM.Bptr = (SIArray)(((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
	  tmp2BM.Btype = Z_PIXMAP;
	  tmp2BM.Bdepth = pDstDrawable->depth;
	  tmp2BM.Bwidth = (int)pDstDrawable->width;
	  tmp2BM.Bheight = (int)pDstDrawable->height;
	  tmp2BM.Bptr = (SIArray)(((PixmapPtr)pDstDrawable)->devPrivate.ptr);
	  while(nbox--) {
	      width = prect->x2 - prect->x1;
	      height = prect->y2 - prect->y1;
	      si_rop(&tmpBM, &tmp2BM,
			   ppt->x, ppt->y,
			   prect->x1, prect->y1,
			   width, height, pGC->alu, pGC->planemask);
	      prect++;
	      ppt++;
	  }
	  break;
#endif 	/* ix86 */
      }

#ifndef FLUSH_IN_BH
      si_Flushcache();
#endif

      /* CLEANUP EXPOSE WINDOWS EXIT */
      if (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->fExpose)
	prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
			  xIn, yIn,
			  widthSrc, heightSrc,
			  xOut, yOut, 0);
      if(realSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
      DEALLOCATE_LOCAL(pptFirst);
      (*pGC->pScreen->RegionDestroy)(prgnDst);
      return prgnExposed;
    }

	/* PERFORM SLOWER CODE */

/* SI: end */

    pptFirst = ppt = (DDXPointPtr)
        ALLOCATE_LOCAL(heightSrc * sizeof(DDXPointRec));
    pwidthFirst = pwidth = (unsigned int *)
        ALLOCATE_LOCAL(heightSrc * sizeof(unsigned int));
    numRects = REGION_NUM_RECTS(prgnSrcClip);
    boxes = REGION_RECTS(prgnSrcClip);
    ordering = (unsigned int *)
        ALLOCATE_LOCAL(numRects * sizeof(unsigned int));
    if(!pptFirst || !pwidthFirst || !ordering)
    {
       if (ordering)
	   DEALLOCATE_LOCAL(ordering);
       if (pwidthFirst)
           DEALLOCATE_LOCAL(pwidthFirst);
       if (pptFirst)
           DEALLOCATE_LOCAL(pptFirst);
       return (RegionPtr)NULL;
    }

    /* If not the same drawable then order of move doesn't matter.
       Following assumes that boxes are sorted from top
       to bottom and left to right.
    */
/* LES: the last three conditions below are NOT in the si stuff
        but since deltas are off by .17 I'm unsure wht to do
*/
    if ((pSrcDrawable != pDstDrawable) &&
	((pGC->subWindowMode != IncludeInferiors) ||
	 (pSrcDrawable->type == DRAWABLE_PIXMAP) ||
	 (pDstDrawable->type == DRAWABLE_PIXMAP)))
      for (i=0; i < numRects; i++)
        ordering[i] = i;
    else { /* within same drawable, must sequence moves carefully! */
      if (dsty <= srcBox.y1) { /* Scroll up or stationary vertical.
                                  Vertical order OK */
        if (dstx <= srcBox.x1) /* Scroll left or stationary horizontal.
                                  Horizontal order OK as well */
          for (i=0; i < numRects; i++)
            ordering[i] = i;
        else { /* scroll right. must reverse horizontal banding of rects. */
          for (i=0, j=1, xMax=0; i < numRects; j=i+1, xMax=i) {
            /* find extent of current horizontal band */
            y=boxes[i].y1; /* band has this y coordinate */
            while ((j < numRects) && (boxes[j].y1 == y))
              j++;
            /* reverse the horizontal band in the output ordering */
            for (j-- ; j >= xMax; j--, i++)
              ordering[i] = j;
          }
        }
      }
      else { /* Scroll down. Must reverse vertical banding. */
        if (dstx < srcBox.x1) { /* Scroll left. Horizontal order OK. */
          for (i=numRects-1, j=i-1, yMin=i, yMax=0;
              i >= 0;
              j=i-1, yMin=i) {
            /* find extent of current horizontal band */
            y=boxes[i].y1; /* band has this y coordinate */
            while ((j >= 0) && (boxes[j].y1 == y))
              j--;
            /* reverse the horizontal band in the output ordering */
            for (j++ ; j <= yMin; j++, i--, yMax++)
              ordering[yMax] = j;
          }
        }
        else /* Scroll right or horizontal stationary.
                Reverse horizontal order as well (if stationary, horizontal
                order can be swapped without penalty and this is faster
                to compute). */
          for (i=0, j=numRects-1; i < numRects; i++, j--)
              ordering[i] = j;
      }
    }
 
#ifdef XWIN_SAVE_UNDERS
    	/*
	 * Check to see if the drawable conflicts with
	 * any save-under windows
	 */ 
	if (SUCheckDrawable(pDstDrawable))
	{
		siTestRects(pDstDrawable, pGC, numRects, prect);
	}
#endif

     for(i = 0; i < numRects; i++)
     {
        prect = &boxes[ordering[i]];
  	xMin = max(prect->x1, srcBox.x1);
  	xMax = min(prect->x2, srcBox.x2);
  	yMin = max(prect->y1, srcBox.y1);
	yMax = min(prect->y2, srcBox.y2);
	/* is there anything visible here? */
	if(xMax <= xMin || yMax <= yMin)
	    continue;

        ppt = pptFirst;
	pwidth = pwidthFirst;
	y = yMin;
	height = yMax - yMin;
	width = xMax - xMin;

	for(j = 0; j < height; j++)
	{
	    /* We must untranslate before calling GetSpans */
	    ppt->x = xMin;
	    ppt++->y = y++;
	    *pwidth++ = width;
	}
	pbits = (unsigned int *)xalloc(height * PixmapBytePad(width,
						     pSrcDrawable->depth));
	if (pbits)
	{
	    (*pSrcDrawable->pScreen->GetSpans)(pSrcDrawable, width, pptFirst,
					       pwidthFirst, height, pbits);
	    ppt = pptFirst;
	    pwidth = pwidthFirst;
	    xMin -= (srcx - dstx);
	    y = yMin - (srcy - dsty);
	    for(j = 0; j < height; j++)
	    {
		ppt->x = xMin;
		ppt++->y = y++;
		*pwidth++ = width;
	    }

	    (*pGC->ops->SetSpans)(pDstDrawable, pGC, pbits, pptFirst,
				  pwidthFirst, height, TRUE);
	    xfree(pbits);
	}
    }
/* SI: old code
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, xIn, yIn,
		      widthSrc, heightSrc, xOut, yOut, (unsigned long)0);
*/
/* SI: start */
    if (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->fExpose)
	prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
			  xIn, yIn,
			  widthSrc, heightSrc,
			  xOut, yOut, 0);
/* SI: end */
    if(realSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
		
    DEALLOCATE_LOCAL(ordering);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(pptFirst);
    return prgnExposed;
}

/* MIGETPLANE -- gets a bitmap representing one plane of pDraw
 * A helper used for CopyPlane and XY format GetImage 
 * No clever strategy here, we grab a scanline at a time, pull out the
 * bits and then stuff them in a 1 bit deep map.
 */
static
unsigned long	*
miGetPlane(pDraw, planeNum, sx, sy, w, h, result)
    DrawablePtr		pDraw;
    int			planeNum;	/* number of the bitPlane */
    int			sx, sy, w, h;
    unsigned long	*result;
{
    int			i, j, k, width, bitsPerPixel, widthInBytes;
    DDXPointRec 	pt;
    unsigned long	pixel;
    unsigned long	bit;
    unsigned char	*pCharsOut;
#if BITMAP_SCANLINE_UNIT == 16
    CARD16		*pShortsOut;
#endif
#if BITMAP_SCANLINE_UNIT == 32
    CARD32		*pLongsOut;
#endif
    int			delta;
/* SI: start */
    PixmapPtr		pPixmap = (PixmapPtr)0;
    unsigned int	tmpSrc, *psrc, *psrcBase;
    int			widthSrc, slowscr = 0;

    PPW = 32 / pDraw->depth;
    PWSH = si_pix_to_word[PPW];
    PSZ = pDraw->depth;

/* LES: changed depth to pDraw->depth below */
    if (pDraw->type == DRAWABLE_WINDOW && si_hassmbitblt) {
	int		idx, ret, srcx, srcy;
	SIbitmap	tmpBM, tmpTile, tmpStpl;
	SIGState	tmpGS;

	/*
	 * SI:  Make sure we validate the source rectangle
	 */
	if (pDraw->pScreen->SourceValidate)
	    (*pDraw->pScreen->SourceValidate) (pDraw, sx, sy, w, h);
	srcx = sx + pDraw->x;
	srcy = sy + pDraw->y;
	pPixmap = (PixmapPtr)(*pDraw->pScreen->CreatePixmap)
                           (pDraw->pScreen, w, h, pDraw->depth);
	if (pPixmap) {
            idx = sisettempgs(&tmpGS, &tmpTile, &tmpStpl);
            tmpGS.SGmode = GXcopy;
            tmpGS.SGpmask = -1;
            tmpBM.Bdepth = pDraw->depth;
            tmpBM.Bwidth = w;
            tmpBM.Bheight = h;
            tmpBM.BorgX = tmpBM.BorgY = 0;
	    tmpBM.Btype = Z_BITMAP;
            tmpBM.Bptr = (SIArray) pPixmap->devPrivate.ptr;
	    si_PrepareGS2(idx, &tmpGS);
            ret = si_SMbitblt( &tmpBM, srcx, srcy, 0, 0, w, h);
            sifreestate(idx, &tmpGS);
            if (ret == SI_SUCCEED) {
		pDraw = (DrawablePtr) pPixmap;
		sx = sy = 0;
            }
	}
    }
/* SI: end */

    sx += pDraw->x;
    sy += pDraw->y;
    if (pDraw->type != DRAWABLE_PIXMAP)
	slowscr++;

    widthInBytes = PixmapBytePad(w, 1);
    if(!result)
        result = (unsigned long *)xalloc(h * widthInBytes);
    if (!result)
	return (unsigned long *)NULL;
    bitsPerPixel = pDraw->bitsPerPixel;
    bzero((char *)result, h * widthInBytes);
#if BITMAP_SCANLINE_UNIT == 8
	pCharsOut = (unsigned char *) result;
#endif
#if BITMAP_SCANLINE_UNIT == 16
	pShortsOut = (CARD16 *) result;
#endif
#if BITMAP_SCANLINE_UNIT == 32
	pLongsOut = (CARD32 *) result;
#endif
    if(bitsPerPixel == 1)
    {
	pCharsOut = (unsigned char *) result;
	width = w;
    }
    else
    {
	delta = (widthInBytes / (BITMAP_SCANLINE_UNIT / 8)) -
	    (w / BITMAP_SCANLINE_UNIT);
	width = 1;
#if IMAGE_BYTE_ORDER == MSBFirst
	planeNum += (32 - bitsPerPixel);
#endif
    }
/* SI: start */
    if (!slowscr) {
	psrcBase = (unsigned int *)(((PixmapPtr)pDraw)->devPrivate.ptr);
	widthSrc = (int)(((PixmapPtr)pDraw)->devKind);
    }
/* SI: end */
    pt.y = sy;
    for (i = h; --i >= 0; pt.y++)
    {
	pt.x = sx;
	if(bitsPerPixel == 1)
	{
	    (*pDraw->pScreen->GetSpans)(pDraw, width, &pt, &width, 1,
					(unsigned long *)pCharsOut);
	    pCharsOut += widthInBytes;
	}
	else
	{
	    k = 0;
	    for(j = w; --j >= 0; pt.x++)
	    {
		/* Fetch the next pixel */
/* SI: start */
		if (slowscr)
		{
/* SI: end */
		    (*pDraw->pScreen->GetSpans)(pDraw, width, &pt, &width, 1,
					        &pixel);
/* SI: start */
		}
		else
		{
                    psrc = psrcBase + (pt.y * (widthSrc >> 2)) + (pt.x >> PWSH);
		    getbits(psrc, pt.x & PIM, 1, tmpSrc);
                    putbits(tmpSrc, 0, 1, &pixel, (unsigned long) -1);
		}
/* SI: end */
		/*
		 * Now get the bit and insert into a bitmap in XY format.
		 */
		bit = (pixel >> planeNum) & 1;
		/* XXX assuming bit order == byte order */
#if BITMAP_BIT_ORDER == LSBFirst
		bit <<= k;
#else
		bit <<= ((BITMAP_SCANLINE_UNIT - 1) - k);
#endif
#if BITMAP_SCANLINE_UNIT == 8
		*pCharsOut |= (unsigned char) bit;
		k++;
		if (k == 8)
		{
		    pCharsOut++;
		    k = 0;
		}
#endif
#if BITMAP_SCANLINE_UNIT == 16
		*pShortsOut |= (CARD16) bit;
		k++;
		if (k == 16)
		{
		    pShortsOut++;
		    k = 0;
		}
#endif
#if BITMAP_SCANLINE_UNIT == 32
		*pLongsOut |= (CARD32) bit;
		k++;
		if (k == 32)
		{
		    pLongsOut++;
		    k = 0;
		}
#endif
	    }
#if BITMAP_SCANLINE_UNIT == 8
	    pCharsOut += delta;
#endif
#if BITMAP_SCANLINE_UNIT == 16
	    pShortsOut += delta;
#endif
#if BITMAP_SCANLINE_UNIT == 32
	    pLongsOut += delta;
#endif
	}
    }
/* SI: start */
    if (pPixmap)
	(*pDraw->pScreen->DestroyPixmap)(pPixmap);
/* SI: end */
    return(result);    

}

/* MIOPQSTIPDRAWABLE -- use pbits as an opaque stipple for pDraw.
 * Drawing through the clip mask we SetSpans() the bits into a 
 * bitmap and stipple those bits onto the destination drawable by doing a
 * PolyFillRect over the whole drawable, 
 * then we invert the bitmap by copying it onto itself with an alu of
 * GXinvert, invert the foreground/background colors of the gc, and draw
 * the background bits.
 * Note how the clipped out bits of the bitmap are always the background
 * color so that the stipple never causes FillRect to draw them.
 */
void
miOpqStipDrawable(pDraw, pGC, prgnSrc, pbits, srcx, w, h, dstx, dsty)
    DrawablePtr pDraw;
    GCPtr	pGC;
    RegionPtr	prgnSrc;
    unsigned long	*pbits;
    int		srcx, w, h, dstx, dsty;
{
    int		oldfill, i;
    unsigned long oldfg;
    int		*pwidth, *pwidthFirst;
    XID		gcv[6];
    PixmapPtr	pStipple, pPixmap;
    DDXPointRec	oldOrg;
    GCPtr	pGCT;
    DDXPointPtr ppt, pptFirst;
    xRectangle rect;
    RegionPtr	prgnSrcClip;
/* SI: start */

    if (pDraw->type == DRAWABLE_WINDOW && !((WindowPtr)pDraw)->realized) {
	return;
    }
    if (pDraw->type == DRAWABLE_WINDOW && si_hasmsstplblt) {
	RegionPtr	prgnDst;
	register BoxPtr pbox;
	register int	nbox;
	BoxRec		bbox, clip;
	SIbitmap	tmpBM;
	unsigned long	*pinvbits;

	dstx += pDraw->x;
	dsty += pDraw->y;
	/* First create a destroyable destination region */
	prgnDst = (*pGC->pScreen->RegionCreate)(NULL, 0);
	(*pGC->pScreen->RegionCopy)(prgnDst, prgnSrc);
	(*pGC->pScreen->TranslateRegion)(prgnDst, dstx, dsty);
	(*pGC->pScreen->Intersect)(prgnDst, prgnDst,
			((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the rect conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDraw))
        {
            if (SUCheckRegion(pDraw, prgnDst))
            {
	        siSUScanWindows(pDraw, IncludeInferiors, prgnDst, NULL);
            }
        }
#endif

	/*
	 * If we don't have Opaque Stippleblt capability, make a copy
	 * of the bitmap with the bits inverted so we can stipple both
	 * the foreground and the background
	 */
	if (!si_hasopqstipple(SIavail_stplblt)) {
		int longs_per_line, i, j;
		unsigned long *psrc, *pdst;

		longs_per_line = (srcx + w + 31) >> 5;
		pinvbits = (unsigned long *) ALLOCATE_LOCAL(longs_per_line*4*h);

		psrc = pbits;
		pdst = pinvbits;
		for (j = 0; j < h; j++)
			for (i = 0; i < longs_per_line; i++)
                                *pdst++ = ~(*psrc++);
	}

	/* Then perform the Opaque Stippleblt */
	tmpBM.BorgX = tmpBM.BorgY = 0;
	tmpBM.Btype = Z_BITMAP;
	tmpBM.Bdepth = 1;
	tmpBM.Bwidth = w + srcx;
	tmpBM.Bheight = h;
	tmpBM.Bptr = (SIArray) pbits;
	si_PrepareGS(pGC);
#ifndef FLUSH_IN_BH
	si_Initcache();
#endif

	nbox = REGION_NUM_RECTS( prgnDst );
	pbox = REGION_RECTS( prgnDst );
	while(nbox--) {
            CHECKINPUT();
            bbox.x1 = dstx;
            bbox.y1 = dsty;
            bbox.x2 = dstx + w;
            bbox.y2 = dsty + h;
            clip.x1 = max(bbox.x1, pbox->x1);
            clip.y1 = max(bbox.y1, pbox->y1);
            clip.x2 = min(bbox.x2, pbox->x2);
            clip.y2 = min(bbox.y2, pbox->y2);
            if ((clip.x2<=clip.x1) || (clip.y2<=clip.y1)) {
		pbox++;
		continue;
            }
            if ((clip.x2 - clip.x1) == 0 || (clip.y2 - clip.y1) == 0) {
		pbox++;
		continue;
            }

            if (si_hasopqstipple(SIavail_stplblt))
		/*
		 * Stipple the foreground and background at once.
		 */
		si_MSstplblt( &tmpBM, clip.x1 - dstx + srcx, clip.y1 - dsty,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1,
                    0, SGOPQStipple);
            else {
		/*
		 * Stipple the foreground
		 */
		si_MSstplblt( &tmpBM, clip.x1 - dstx + srcx, clip.y1 - dsty,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1,
                    0, SGStipple);
	
		/*
		 * Swap the foreground and background in the GC
		 */
		oldfg = pGC->fgPixel;
		gcv[0] = (long) pGC->bgPixel;
		gcv[1] = (long) oldfg;
		DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);
		ValidateGC(pDraw, pGC);
		si_PrepareGS(pGC);
		tmpBM.Bptr = (SIArray) pinvbits;

		/*
		 * Stipple the background
		 */
		si_MSstplblt( &tmpBM, clip.x1 - dstx + srcx, clip.y1 - dsty,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1,
                    0, SGStipple);

		/*
		 * Put everything back the way it belongs
		 */
		gcv[0] = (long) oldfg;
		gcv[1] = (long) pGC->fgPixel;
		DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);
		ValidateGC(pDraw, pGC);
		si_PrepareGS(pGC);
		tmpBM.Bptr = (SIArray) pbits;
		DEALLOCATE_LOCAL(pinvbits);
            }

            pbox++;
	}

#ifndef FLUSH_IN_BH
	si_Flushcache();
#endif
	(*pGC->pScreen->RegionDestroy)(prgnDst);
	return;
    }

/* SI: end */

    pPixmap = (*pDraw->pScreen->CreatePixmap)
			   (pDraw->pScreen, w + srcx, h, 1);
    if (!pPixmap)
	return;

    /* Put the image into a 1 bit deep pixmap */
    pGCT = GetScratchGC(1, pDraw->pScreen);
    if (!pGCT)
    {
	(*pDraw->pScreen->DestroyPixmap)(pPixmap);
	return;
    }
    /* First set the whole pixmap to 0 */
    gcv[0] = 0;
    DoChangeGC(pGCT, GCBackground, gcv, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);
    miClearDrawable((DrawablePtr)pPixmap, pGCT);
    ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
    if(!pptFirst || !pwidthFirst)
    {
	if (pwidthFirst) DEALLOCATE_LOCAL(pwidthFirst);
	if (pptFirst) DEALLOCATE_LOCAL(pptFirst);
	FreeScratchGC(pGCT);
	return;
    }

    /* we need a temporary region because ChangeClip must be assumed
       to destroy what it's sent.  note that this means we don't
       have to free prgnSrcClip ourselves.
    */
    prgnSrcClip = (*pGCT->pScreen->RegionCreate)(NULL, 0);
    (*pGCT->pScreen->RegionCopy)(prgnSrcClip, prgnSrc);
    (*pGCT->pScreen->TranslateRegion) (prgnSrcClip, srcx, 0);
    (*pGCT->funcs->ChangeClip)(pGCT, CT_REGION, prgnSrcClip, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the region conflicts with
         * any save-under windows
	 *
	 * Don't bother, because the lowest call here is to setspans and
	 * FillRect and save-under checks are done there anyway.....
         */ 
#endif

    /* Since we know pDraw is always a pixmap, we never need to think
     * about translation here */
    for(i = 0; i < h; i++)
    {
	ppt->x = 0;
	*pwidth++ = w + srcx;
	ppt++->y = i;
    }

    (*pGCT->ops->SetSpans)(pPixmap, pGCT, pbits, pptFirst, pwidthFirst, h, TRUE);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(pptFirst);


    /* Save current values from the client GC */
    oldfill = pGC->fillStyle;
    pStipple = pGC->stipple;
    if(pStipple)
        pStipple->refcnt++;
    oldOrg = pGC->patOrg;

    /* Set a new stipple in the drawable */
    gcv[0] = FillStippled;
    gcv[1] = (long) pPixmap;
    gcv[2] = dstx - srcx;
    gcv[3] = dsty;

    DoChangeGC(pGC,
             GCFillStyle | GCStipple | GCTileStipXOrigin | GCTileStipYOrigin,
	     gcv, 1);
    ValidateGC(pDraw, pGC);

    /* Fill the drawable with the stipple.  This will draw the
     * foreground color whereever 1 bits are set, leaving everything
     * with 0 bits untouched.  Note that the part outside the clip
     * region is all 0s.  */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Invert the tiling pixmap. This sets 0s for 1s and 1s for 0s, only
     * within the clipping region, the part outside is still all 0s */
    gcv[0] = GXinvert;
    DoChangeGC(pGCT, GCFunction, gcv, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);
    (*pGCT->ops->CopyArea)(pPixmap, pPixmap, pGCT, 0, 0, w + srcx, h, 0, 0);

    /* Swap foreground and background colors on the GC for the drawable.
     * Now when we fill the drawable, we will fill in the "Background"
     * values */
    oldfg = pGC->fgPixel;
    gcv[0] = (long) pGC->bgPixel;
    gcv[1] = (long) oldfg;
    gcv[2] = (long) pPixmap;
    DoChangeGC(pGC, GCForeground | GCBackground | GCStipple, gcv, 1); 
    ValidateGC(pDraw, pGC);
    /* PolyFillRect might have bashed the rectangle */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Now put things back */
    if(pStipple)
        pStipple->refcnt--;
    gcv[0] = (long) oldfg;
    gcv[1] = pGC->fgPixel;
    gcv[2] = oldfill;
    gcv[3] = (long) pStipple;
    gcv[4] = oldOrg.x;
    gcv[5] = oldOrg.y;
    DoChangeGC(pGC, 
        GCForeground | GCBackground | GCFillStyle | GCStipple | 
	GCTileStipXOrigin | GCTileStipYOrigin, gcv, 1);

    ValidateGC(pDraw, pGC);
    /* put what we hope is a smaller clip region back in the scratch gc */
    (*pGCT->funcs->ChangeClip)(pGCT, CT_NONE, NULL, 0);
    FreeScratchGC(pGCT);
    (*pDraw->pScreen->DestroyPixmap)(pPixmap);
}

/* MICOPYPLANE -- public entry for the CopyPlane request.
 * strategy: 
 * First build up a bitmap out of the bits requested 
 * build a source clip
 * Use the bitmap we've built up as a Stipple for the destination 
 */
RegionPtr
miCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane)
    DrawablePtr 	pSrcDrawable;
    DrawablePtr		pDstDrawable;
    GCPtr		pGC;
    int 		srcx, srcy;
    int 		width, height;
    int 		dstx, dsty;
    unsigned long	bitPlane;
{
    unsigned long	*ptile;
    BoxRec 		box;
    RegionPtr		prgnSrc, prgnExposed;

    /* incorporate the source clip */
/* SI: start */
    if (pSrcDrawable->type != DRAWABLE_WINDOW &&
	pDstDrawable->type != DRAWABLE_WINDOW &&
	pSrcDrawable->depth == 1 && pDstDrawable->depth == 1)
    {
	prgnExposed = mfbCopyPlane(pSrcDrawable, pDstDrawable,
            pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
	return prgnExposed;
    }
/* SI: end */

    box.x1 = srcx + pSrcDrawable->x;
    box.y1 = srcy + pSrcDrawable->y;
    box.x2 = box.x1 + width;
    box.y2 = box.y1 + height;
    /* clip to visible drawable */
    if (box.x1 < pSrcDrawable->x)
	box.x1 = pSrcDrawable->x;
    if (box.y1 < pSrcDrawable->y)
	box.y1 = pSrcDrawable->y;
    if (box.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
	box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
    if (box.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
	box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
    if (box.x1 > box.x2)
	box.x2 = box.x1;
    if (box.y1 > box.y2)
	box.y2 = box.y1;
    prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);

    if (pSrcDrawable->type != DRAWABLE_PIXMAP) {
	/* clip to visible drawable */

	if (pGC->subWindowMode == IncludeInferiors)
	{
	    RegionPtr	clipList = NotClippedByChildren ((WindowPtr) pSrcDrawable);
	    (*pGC->pScreen->Intersect) (prgnSrc, prgnSrc, clipList);
	    (*pGC->pScreen->RegionDestroy) (clipList);
	} else
	    (*pGC->pScreen->Intersect)
		    (prgnSrc, prgnSrc, &((WindowPtr)pSrcDrawable)->clipList);
    }

    box = *(*pGC->pScreen->RegionExtents)(prgnSrc);
    (*pGC->pScreen->TranslateRegion)(prgnSrc, -box.x1, -box.y1);

    if ((box.x2 > box.x1) && (box.y2 > box.y1))
    {
	/* minimize the size of the data extracted */
	/* note that we convert the plane mask bitPlane into a plane number */
	box.x1 -= pSrcDrawable->x;
	box.x2 -= pSrcDrawable->x;
	box.y1 -= pSrcDrawable->y;
	box.y2 -= pSrcDrawable->y;
	ptile = miGetPlane(pSrcDrawable, ffs(bitPlane) - 1,
			   box.x1, box.y1,
			   box.x2 - box.x1, box.y2 - box.y1,
			   (unsigned long *) NULL);
	if (ptile)
	{
	    miOpqStipDrawable(pDstDrawable, pGC, prgnSrc, ptile, 0,
			      box.x2 - box.x1, box.y2 - box.y1,
			      dstx + box.x1 - srcx, dsty + box.y1 - srcy);
	    xfree(ptile);
	}
    }
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
		      width, height, dstx, dsty, bitPlane);
    (*pGC->pScreen->RegionDestroy)(prgnSrc);
    return prgnExposed;
}

/* MIGETIMAGE -- public entry for the GetImage Request
 * We're getting the image into a memory buffer. While we have to use GetSpans
 * to read a line from the device (since we don't know what that looks like),
 * we can just write into the destination buffer
 *
 * two different strategies are used, depending on whether we're getting the
 * image in Z format or XY format
 * Z format:
 * Line at a time, GetSpans a line into the destination buffer, then if the
 * planemask is not all ones, we do a SetSpans into a temporary buffer (to get
 * bits turned off) and then another GetSpans to get stuff back (because
 * pixmaps are opaque, and we are passed in the memory to write into).  This is
 * pretty ugly and slow but works.  Life is hard.
 * XY format:
 * get the single plane specified in planemask
 */
void
miGetImage(pDraw, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr 	pDraw;
    int			sx, sy, w, h;
    unsigned int 	format;
    unsigned long 	planeMask;
    pointer             pdstLine;
{
    unsigned char	depth;
    int			i, linelength, width, srcx, srcy;
    DDXPointRec		pt;
    XID			gcv[2];
    PixmapPtr		pPixmap = (PixmapPtr)NULL;
/* SI: old code
    GCPtr		pGC;
    pointer		pDst = pdstLine;
*/
/* SI: start */
    GCPtr		pGC = (GCPtr)0;
    unsigned long *	pDst = (unsigned long *)pdstLine;
/* SI: end */

/* SI: start */
    if (pDraw->depth == 1 && pDraw->type == DRAWABLE_PIXMAP) {
	mfbGetImage(pDraw, sx, sy, w, h, format, planeMask, pdstLine);
	return;
    }
/* SI: end */

    depth = pDraw->depth;
    if(format == ZPixmap)
    {
/* SI: start */
	linelength = PixmapBytePad(w, depth);
	srcx = sx;
	srcy = sy;
	if(pDraw->type == DRAWABLE_WINDOW)
	{
	    srcx += pDraw->x;
	    srcy += pDraw->y;
	}
        if (pDraw->type == DRAWABLE_WINDOW && si_hassmbitblt) {
            int		idx, ret;
            SIbitmap	tmpBM, tmpTile, tmpStpl;
            SIGState	tmpGS;

	    /*
	     * SI:  Make sure we validate the source rectangle
	     */
	    if (pDraw->pScreen->SourceValidate)
	        (*pDraw->pScreen->SourceValidate) (pDraw, sx, sy, w, h);
            idx = sisettempgs(&tmpGS, &tmpTile, &tmpStpl);
            tmpGS.SGmode = GXcopy;
            tmpGS.SGpmask = planeMask;
            tmpBM.Bdepth = depth;
            tmpBM.Bwidth = w;
            tmpBM.Bheight = h;
            tmpBM.BorgX = tmpBM.BorgY = 0;
	    tmpBM.Btype = Z_BITMAP;
            tmpBM.Bptr = (SIArray) pdstLine;
	    si_PrepareGS2(idx, &tmpGS);
            ret = si_SMbitblt( &tmpBM, srcx, srcy, 0, 0, w, h);
            sifreestate(idx, &tmpGS);
            if (ret == SI_SUCCEED)
		return;
	}

/* SI: end */
	if ( (((1<<depth)-1)&planeMask) != (1<<depth)-1 )
	{
	    pGC = GetScratchGC(depth, pDraw->pScreen);
	    if (!pGC)
		return;
            pPixmap = (*pDraw->pScreen->CreatePixmap)
			       (pDraw->pScreen, w, h, depth);
	    if (!pPixmap)
	    {
		FreeScratchGC(pGC);
		return;
	    }
	    gcv[0] = GXcopy;
	    gcv[1] = planeMask;
	    DoChangeGC(pGC, GCPlaneMask | GCFunction, gcv, 0);
	    ValidateGC((DrawablePtr)pPixmap, pGC);
	}

	for(i = 0; i < h; i++)
	{
	    pt.x = srcx;
	    pt.y = srcy + i;
	    width = w;
	    (*pDraw->pScreen->GetSpans)(pDraw, w, &pt, &width, 1,
					(unsigned long *)pDst);
	    if (pPixmap)
	    {
	       pt.x = 0;
	       pt.y = 0;
	       width = w;
	       (*pGC->ops->SetSpans)(pPixmap, pGC, (unsigned long *)pDst,
				     &pt, &width, 1, TRUE);
	       (*pDraw->pScreen->GetSpans)(pPixmap, w, &pt, &width, 1,
					   (unsigned long *)pDst);
	    }
/* SI: old code
	    pDst += linelength;
*/
/* SI: start */
            pDst += linelength / sizeof(long);
/* SI: end */
	}
	if (pPixmap)
	{
	    (*pGC->pScreen->DestroyPixmap)(pPixmap);
	    FreeScratchGC(pGC);
	}
    }
    else
    {
	(void) miGetPlane(pDraw, ffs(planeMask) - 1, sx, sy, w, h,
			  (unsigned long *)pDst);
    }
}


/* MIPUTIMAGE -- public entry for the PutImage request
 * Here we benefit from knowing the format of the bits pointed to by pImage,
 * even if we don't know how pDraw represents them.  
 * Three different strategies are used depending on the format 
 * XYBitmap Format:
 * 	we just use the Opaque Stipple helper function to cover the destination
 * 	Note that this covers all the planes of the drawable with the 
 *	foreground color (masked with the GC planemask) where there are 1 bits
 *	and the background color (masked with the GC planemask) where there are
 *	0 bits
 * XYPixmap format:
 *	what we're called with is a series of XYBitmaps, but we only want 
 *	each XYPixmap to update 1 plane, instead of updating all of them.
 * 	we set the foreground color to be all 1s and the background to all 0s
 *	then for each plane, we set the plane mask to only effect that one
 *	plane and recursive call ourself with the format set to XYBitmap
 *	(This clever idea courtesy of RGD.)
 * ZPixmap format:
 *	This part is simple, just call SetSpans
 */
void
miPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr		pDraw;
    GCPtr		pGC;
    int 		depth, x, y, w, h, leftPad;
    unsigned int	format;
    unsigned char	*pImage;
{
    DDXPointPtr		pptFirst, ppt;
    int			*pwidthFirst, *pwidth;
    RegionPtr		prgnSrc;
    BoxRec		box;
    unsigned long	oldFg, oldBg, gcv[3];
    unsigned long	oldPlanemask;
    unsigned long	i;
    long		bytesPer;

    if (!w || !h)
	return;

/* SI: start */
    if (pDraw->depth == 1 && pDraw->type == DRAWABLE_PIXMAP) {
	mfbPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage);
	return;
    }
/* SI: end */

    switch(format)
    {
      case XYBitmap:

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = w;
	box.y2 = h;
	prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);

        miOpqStipDrawable(pDraw, pGC, prgnSrc, (unsigned long *) pImage,
			  leftPad, w, h, x, y);
	(*pGC->pScreen->RegionDestroy)(prgnSrc);
	break;

      case XYPixmap:
	depth = pGC->depth;
	oldPlanemask = pGC->planemask;
	oldFg = pGC->fgPixel;
	oldBg = pGC->bgPixel;
	gcv[0] = ~0L;
	gcv[1] = 0;
	DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);
	bytesPer = (long)h * PixmapBytePad(w + leftPad, 1);

	for (i = 1 << (depth-1); i != 0; i >>= 1, pImage += bytesPer)
	{
	    if (i & oldPlanemask)
	    {
	        gcv[0] = i;
	        DoChangeGC(pGC, GCPlaneMask, gcv, 0);
	        ValidateGC(pDraw, pGC);
	        (*pGC->ops->PutImage)(pDraw, pGC, 1, x, y, w, h, leftPad,
			         XYBitmap, pImage);
	    }
	}
	gcv[0] = oldPlanemask;
	gcv[1] = oldFg;
	gcv[2] = oldBg;
	DoChangeGC(pGC, GCPlaneMask | GCForeground | GCBackground, gcv, 0);
	break;

      case ZPixmap:
/* SI: start */
	if ((pDraw->type == DRAWABLE_WINDOW) &&
            (pGC->miTranslate))
	{
            x += pDraw->x;
            y += pDraw->y;
	}
	if (pDraw->type == DRAWABLE_WINDOW && si_hasmsbitblt) {
            RegionPtr	prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
            PixmapPtr	pPixmap = (PixmapPtr)NULL;
            BoxRec	bbox, clip;
            BoxPtr	pbox;
            int		nbox;
            SIbitmap	tmpBM;

            tmpBM.Btype = Z_PIXMAP;
            tmpBM.Bdepth = depth;
            tmpBM.Bwidth = w;
            tmpBM.Bheight = h;
            tmpBM.BorgX = tmpBM.BorgY = 0;
	    tmpBM.Btype = Z_BITMAP;
            tmpBM.Bptr = (SIArray) pImage;
            si_PrepareGS(pGC);
            nbox = REGION_NUM_RECTS( prgnDst );
            pbox = REGION_RECTS( prgnDst );

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the region conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDraw))
        {
            if (SUCheckRegion(pDraw, prgnDst))
            {
	        siSUScanWindows(pDraw, IncludeInferiors, prgnDst, NULL);
            }
        }
#endif

            while(nbox--) {
		CHECKINPUT();
		bbox.x1 = x;
		bbox.y1 = y;
		bbox.x2 = x + w;
		bbox.y2 = y + h;
		clip.x1 = max(bbox.x1, pbox->x1);
		clip.y1 = max(bbox.y1, pbox->y1);
		clip.x2 = min(bbox.x2, pbox->x2);
		clip.y2 = min(bbox.y2, pbox->y2);
		if ((clip.x2<=clip.x1) || (clip.y2<=clip.y1)) {
                    pbox++;
                    continue;
		}
		if ( (clip.x2 - clip.x1) == 0 ||
                     (clip.y2 - clip.y1) == 0) {
                    pbox++;
                    continue;
		}

		si_MSbitblt( &tmpBM, clip.x1 - x, clip.y1 - y,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1);
		pbox++;
            }
            return;
	}
/* SI: end */
    	ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    	pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
	if(!pptFirst || !pwidthFirst)
        {
	   if (pwidthFirst)
               DEALLOCATE_LOCAL(pwidthFirst);
           if (pptFirst)
               DEALLOCATE_LOCAL(pptFirst);
           return;
        }
/* SI: old code
	if (pGC->miTranslate)
	{
	    x += pDraw->x;
	    y += pDraw->y;
	}
*/

	for(i = 0; i < h; i++)
	{
	    ppt->x = x;
	    ppt->y = y + i;
	    ppt++;
	    *pwidth++ = w;
	}

	(*pGC->ops->SetSpans)(pDraw, pGC, pImage, pptFirst, pwidthFirst, h, TRUE);
	DEALLOCATE_LOCAL(pwidthFirst);
	DEALLOCATE_LOCAL(pptFirst);
	break;
    }
}
/* LES: miClearDRawable has been removed for R4 */
/* SI: start */
/* DoBitblt() does multiple rectangle moves into the rectangles
   DISCLAIMER:
   this code can be made much faster; this implementation is
designed to be independent of byte/bit order, processor
instruction set, and the like.	it could probably be done
in a similarly device independent way using mask tables instead
of the getbits/putbits macros.	the narrow case (w<32) can be
subdivided into a case that crosses word boundaries and one that
doesn't.

   we have to cope with the dircetion on a per band basis,
rather than a per rectangle basis.  moving bottom to top
means we have to invert the order of the bands; moving right
to left requires reversing the order of the rectangles in
each band.

   if src or dst is a window, the points have already been
translated.
   This bitblt routine ONLY does GXcopy, and is intended only
for internal use for specific simple cases such as backing store
updating.
*/

siCopyBitblt(pWin, pSrcDrawable, pDstDrawable, prgnDst, pptSrc)
WindowPtr pWin;
DrawablePtr pSrcDrawable;
DrawablePtr pDstDrawable;
RegionPtr prgnDst;
DDXPointPtr pptSrc;
{
    int *psrcBase, *pdstBase;	/* start of src and dst bitmaps */
    int widthSrc, widthDst;	/* add to get to same position in next line */
    DDXPointPtr		ppt, pptFirst;
    unsigned int	*pwidthFirst, *pwidth, *pbits;
    RegionPtr 		prgnDstClip;
    XID subWindowMode = IncludeInferiors;
    int			x, y, j;

    register BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNewX, pboxNewY;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNewX, pptNewY; /* shuffling boxes entails shuffling the
					     source points too */
    int w, h;
    int xdir;			/* 1 = left right, -1 = right left/ */
    int ydir;			/* 1 = top down, -1 = bottom up */

    int *psrcLine, *pdstLine;	/* pointers to line with current src and dst */
    register int *psrc;		/* pointer to current src longword */
    register int *pdst;		/* pointer to current dst longword */

				/* following used for looping through a line */
    int startmask, endmask;	/* masks for writing ends of dst */
    int nlMiddle;		/* whole longwords in dst */
    register int nl;		/* temp copy of nlMiddle */
    register int tmpSrc;	/* place to store full source word */
    GCPtr	tmpGC = (GCPtr) 0;
    long	gcvals[3];
    SIGState	tmpGS;
    SIint32	index;
    SIbitmap	tmpBM;
    int		hardassist;
    int		src_is_s, dst_is_s;

    /* CHECK FIRST FOR HARDWARE ASSISTS */
    hardassist = 0;
    src_is_s = 0;
    dst_is_s = 0;
    if (pSrcDrawable->type == DRAWABLE_WINDOW) {	/* SS or SM */
	if (pDstDrawable->type == DRAWABLE_WINDOW) {	/* SS */
	    src_is_s = dst_is_s = 1;
	    if (si_hasssbitblt)
		hardassist++;
	} else {					/* SM */
	    src_is_s = 1;
	    if (si_hassmbitblt)
		hardassist++;
	}
    } else {						/* MS or MM */
	if (pDstDrawable->type == DRAWABLE_WINDOW) {	/* MS */
	    dst_is_s = 1;
	    if (si_hasmsbitblt)
		hardassist++;
	}
    }
    if (src_is_s)
	widthSrc = si_getscanlinelen;
    else {
	psrcBase = (int *) (((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
	widthSrc = (int) ((PixmapPtr)pSrcDrawable)->devKind >> 2;
    }

    if (dst_is_s)
	widthDst = si_getscanlinelen;
    else {
	pdstBase = (int *) (((PixmapPtr)pDstDrawable)->devPrivate.ptr);
	widthDst = (int) ((PixmapPtr)pDstDrawable)->devKind >> 2;
    }

    pbox = REGION_RECTS( prgnDst );
    nbox = REGION_NUM_RECTS( prgnDst );

    pboxNewX = NULL;
    pboxNewY = NULL;
    pptNewX = NULL;
    pptNewY = NULL;
    if (pptSrc->y < pbox->y1) 
    {
        /* walk source botttom to top */
	ydir = -1;
	widthSrc = -widthSrc;
	widthDst = -widthDst;

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNewY = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNewY)
		return;
	    pptNewY = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pptNewY)
	    {
		DEALLOCATE_LOCAL(pboxNewY);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox)
	    {
	        while ((pboxNext >= pbox) && 
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase)
	        {
		    *pboxNewY++ = *pboxTmp++;
		    *pptNewY++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNewY -= nbox;
	    pbox = pboxNewY;
	    pptNewY -= nbox;
	    pptSrc = pptNewY;
        }
    }
    else
    {
	/* walk source top to bottom */
	ydir = 1;
    }

    if (pptSrc->x < pbox->x1)
    {
	/* walk source right to left */
        xdir = -1;

	if (nbox > 1)
	{
	    /* reverse order of rects ineach band */
	    pboxNewX = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNewX = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNewX || !pptNewX)
	    {
		if (pptNewX) DEALLOCATE_LOCAL(pptNewX);
		if (pboxNewX) DEALLOCATE_LOCAL(pboxNewX);
		if (pboxNewY)
		{
		    DEALLOCATE_LOCAL(pptNewY);
		    DEALLOCATE_LOCAL(pboxNewY);
		}
	        return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox+nbox)
	    {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase)
	        {
		    *pboxNewX++ = *--pboxTmp;
		    *pptNewX++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNewX -= nbox;
	    pbox = pboxNewX;
	    pptNewX -= nbox;
	    pptSrc = pptNewX;
	}
    }
    else
    {
	/* walk source left to right */
        xdir = 1;
    }

    if (hardassist) {
	index = sisettempgs(&tmpGS, &tmpBM, &tmpBM);
	tmpGS.SGmode = GXcopy;
	tmpBM.BorgX = tmpBM.BorgY = 0;
	si_PrepareGS2(index, &tmpGS);

#ifndef FLUSH_IN_BH
	si_Initcache();
#endif
    }
    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

	if (hardassist) {
	    if (src_is_s && dst_is_s)		/* SCREEN to SCREEN */
		si_SSbitblt(pptSrc->x, pptSrc->y, pbox->x1, pbox->y1, w, h);

	    else if (src_is_s) {		/* SCREEN to MEMORY */
		if (pSrcDrawable->pScreen->SourceValidate)
		    (*pSrcDrawable->pScreen->SourceValidate) (pSrcDrawable, 
							      pptSrc->x, 
							      pptSrc->y, 
							      w, h);
		tmpBM.Bdepth = pDstDrawable->depth;
		tmpBM.Bwidth = (int)pDstDrawable->width;
		tmpBM.Bheight = (int)pDstDrawable->height;
		tmpBM.Bptr = (SIArray)(((PixmapPtr)pDstDrawable)->devPrivate.ptr);
		si_SMbitblt(&tmpBM, pptSrc->x, pptSrc->y,
			     pbox->x1, pbox->y1, w, h);
	    }

	    else if (dst_is_s) {		/* MEMORY to SCREEN */
		tmpBM.Btype = Z_PIXMAP;
		tmpBM.Bdepth = pSrcDrawable->depth;
		tmpBM.Bwidth = (int)pSrcDrawable->width;
		tmpBM.Bheight = (int)pSrcDrawable->height;
		tmpBM.Bptr = (SIArray)(((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
		si_MSbitblt(&tmpBM,
			     pptSrc->x, pptSrc->y,
			     pbox->x1, pbox->y1, w, h);
	    }
	    pbox++;
	    pptSrc++;
	    continue;
	}

	if (!tmpGC) {
		tmpGC = GetScratchGC(pDstDrawable->depth,pDstDrawable->pScreen);
		gcvals[0] = GXcopy;
		DoChangeGC(tmpGC, GCFunction, gcvals, 0);

		/* 
		 * we need a temporary region because ChangeClip must be 
		 * assumed to destroy what it's sent.  note that this means 
		 * we don't have to free prgnDstClip ourselves.
		 */
		ChangeGC (tmpGC, GCSubwindowMode, &subWindowMode);
/*
		prgnDstClip = (*tmpGC->pScreen->RegionCreate)(NULL, 0);
		(*tmpGC->pScreen->RegionCopy)(prgnDstClip, prgnDst);
		(*tmpGC->pScreen->TranslateRegion) (prgnDstClip, srcx, 0);
		(*tmpGC->funcs->ChangeClip)(tmpGC, CT_REGION, prgnDstClip, 0);
*/
		ValidateGC(pDstDrawable, tmpGC);
	}

	ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
	pwidth = pwidthFirst = (unsigned int *)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
	pbits = (unsigned int *)ALLOCATE_LOCAL(h * PixmapBytePad(w, pSrcDrawable->depth));
	x = pptSrc->x;
	y = pptSrc->y;
	
	for (j = 0; j < h; j++) {
		ppt->x = x;
		ppt->y = y++;
		*pwidth++ = w;
		ppt++;
	}

	(*pSrcDrawable->pScreen->GetSpans)(pSrcDrawable, w, pptFirst,
					   pwidthFirst, h, pbits);

	ppt = pptFirst;
	pwidth = pwidthFirst;
	x = pbox->x1;
	y = pbox->y1;
	for (j = 0; j < h; j++) {
		ppt->x = x;
		ppt->y = y++;
		*pwidth++ = w;
		ppt++;
	}

	(*tmpGC->ops->SetSpans)(pDstDrawable, tmpGC, pbits, pptFirst,
					   pwidthFirst, h, TRUE);
	
	DEALLOCATE_LOCAL(pptFirst);
	DEALLOCATE_LOCAL(pwidthFirst);
	DEALLOCATE_LOCAL(pbits);
	pbox++;
	pptSrc++;
    } /* while (nbox--) */

#ifndef FLUSH_IN_BH
    if (si_hasssbitblt) {
	si_Flushcache();
    }
#endif

    if (pboxNewX)
    {
	DEALLOCATE_LOCAL(pptNewX);
	DEALLOCATE_LOCAL(pboxNewX);
    }
    if (pboxNewY)
    {
	DEALLOCATE_LOCAL(pptNewY);
	DEALLOCATE_LOCAL(pboxNewY);
    }
    if (tmpGC) {
	FreeScratchGC(tmpGC);
    }
}
