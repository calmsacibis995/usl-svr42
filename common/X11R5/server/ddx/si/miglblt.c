/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/miglblt.c	1.7"

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
/* $XConsortium: miglblt.c,v 5.2 89/08/25 14:57:38 rws Exp $ */

#include        "X.h"
#include        "Xmd.h"
#include        "Xproto.h"
#include 	"misc.h"
#include        "fontstruct.h"
#include        "dixfontstr.h"
#include        "gcstruct.h"
#include        "windowstr.h"
#include        "scrnintstr.h"
#include        "pixmap.h"
#include        "servermd.h"

/* SI: START */
#include	"miscstruct.h"
#include	"regionstr.h"
#include	"si.h"
#include	"sidep.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

/* SI: END */
/* NOTE: THIS FILE IS HEAVILY MODIFIED */

	/* USED TO SAVE GC's between calls */
extern GC *GetScratchGC();

/*
    Open Display Interface glyph blt.
    assumes that glyph bits in snf are written in longwords,
    have same bit order as the server's bitmap format,
    and are longword aligned.

    get a scratch GC.
    in the scratch GC set alu = GXcopy, fg = 1, bg = 0
    allocate a bitmap big enough to hold the largest glyph in the font
    validate the scratch gc with the bitmap
    for each glyph
	carefully put the bits of the glyph in a buffer,
	    padded to the server pixmap scanline padding rules
	fake a call to PutImage from the buffer into the bitmap
	use the bitmap in a call to PushPixels
*/

void
miPolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    ExtentInfoRec info;			/* used by QueryGlyphExtents */
    int width, height;
    PixmapPtr pPixmap = (PixmapPtr)0;
    int nbyLine;			/* bytes per line of padded pixmap */
    FontRec *pfont;
    GCPtr pGCtmp = (GCPtr)0;
    int i;
    register int j;
    unsigned char *pbits;		/* buffer for PutImage */
    register unsigned char *pb;		/* temp pointer into buffer */
    CharInfoPtr pci;			/* currect char info */
    register unsigned char *pglyph;	/* pointer bits in glyph */
    int gWidth, gHeight;		/* width and height of glyph */
    int nbyGlyphWidth;			/* bytes per scanline of glyph */
    int nbyPadGlyph;			/* server padded line of glyph */
    int size;
    int gcvals[3];
    SIbitmap tmpBM;
    BoxRec bbox;
    int	dwind = 0;			/* Set if a drawable window */
    int rrop;

    if (!nglyph)
	return;

    if (pDrawable->type == DRAWABLE_WINDOW)
	dwind++;
    else if (pDrawable->depth == 1) {
	if ((pGC->font) &&
	    FONTMAXBOUNDS(pGC->font,rightSideBearing) -
	     FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32)
	{
	    if (pGC->fillStyle == FillSolid ||
		(pGC->fillStyle == FillOpaqueStippled &&
		 pGC->fgPixel == pGC->bgPixel
		)
	       )
	    {
		rrop = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->rop;

#ifdef NO_MFBOPTIMIZE
		if (rrop == RROP_WHITE)
		    mfbPolyGlyphBltWhite(pDrawable, pGC, x, y,
					 nglyph, ppci, pglyphBase);
		else if (rrop == RROP_BLACK)
		    mfbPolyGlyphBltBlack(pDrawable, pGC, x, y,
					 nglyph, ppci, pglyphBase);
		else if (rrop == RROP_INVERT)
		    mfbPolyGlyphBltInvert(pDrawable, pGC, x, y,
					 nglyph, ppci, pglyphBase);
#else
		if ( (rrop==RROP_WHITE) || (rrop==RROP_BLACK) || (rrop==RROP_INVERT) )
		    mfbPolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase,rrop);
#endif
		return;
	    }
	}
    }

    if (pGC->miTranslate)
    {
	x += pDrawable->x;
	y += pDrawable->y;
    }

    QueryGlyphExtents(pGC->font, ppci, nglyph, &info);

    bbox.x1 = x + info.overallLeft;
    bbox.x2 = x + info.overallRight;
    bbox.y1 = y - info.overallAscent;
    bbox.y2 = y + info.overallDescent;

    pfont = pGC->font;
    width = FONTMAXBOUNDS(pfont,rightSideBearing) - 
	FONTMINBOUNDS(pfont,leftSideBearing);
    height = FONTMAXBOUNDS(pfont,ascent) + FONTMAXBOUNDS(pfont,descent);


    if (!dwind || !si_hasmsstplblt || pGC->fillStyle != FillSolid) {
	pPixmap = (PixmapPtr)(*pDrawable->pScreen->CreatePixmap)
	     (pDrawable->pScreen, width, height, 1, XYBitmap);

	if (!pPixmap)
	    return;
    }

    nbyLine = PixmapBytePad(width, 1);
    size = height*nbyLine;
    pbits = (unsigned char *)ALLOCATE_LOCAL(size);
    if (!pbits)
        return ;

    if (dwind) {
#ifndef FLUSH_IN_BH
	si_Initcache();
#endif
	if (si_hasmsstplblt && pGC->fillStyle == FillSolid)
	    si_PrepareGS(pGC);
    }

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the rect conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDrawable))
        {
            if (SUCheckBox(pDrawable, &bbox))
            {
	        siSUScanWindows(pDrawable, pGC->subWindowMode, NULL, &bbox);
            }
        }
#endif

    switch ((*pGC->pScreen->RectIn)(
	     ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip, &bbox))
    {
      case rgnOUT:
	break;
      case rgnIN:
	tmpBM.Bdepth = 1;
	tmpBM.BorgX = tmpBM.BorgY = 0;
	while(nglyph--)
	{
	    pci = *ppci++;
	    gWidth = GLYPHWIDTHPIXELS(pci);
	    gHeight = GLYPHHEIGHTPIXELS(pci);
	    nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(pci);


	    if (dwind) {
		/*
		 * This Stippleblt save a large amount of code execution time.
		 * Including at least 4 malloc's and frees for each character
		 * (Among other things).
		 */
		if (si_hasmsstplblt && si_hasstipple(SIavail_stplblt) &&
		    pGC->fillStyle == FillSolid) {
		    tmpBM.Bwidth = gWidth;
		    tmpBM.Bheight = gHeight;
		    tmpBM.Bptr = (SIArray) FONTGLYPHBITS(pglyphBase ,pci);
		    si_MSstplblt(&tmpBM, 0, 0, 
			   x + pci->metrics.leftSideBearing,
			   y - pci->metrics.ascent,
			   gWidth, gHeight, 0,
			   SGStipple);
		    x += pci->metrics.characterWidth;
		    continue;
		}
	    }

	    /*
	     * Real slow code.
	     */

	    if (!pGCtmp) {
		pGCtmp = GetScratchGC(1, pDrawable->pScreen);
		gcvals[0] = GXcopy;
		gcvals[1] = 1;
		gcvals[2] = 0;
		DoChangeGC(pGCtmp,GCFunction|GCForeground|GCBackground,gcvals,0);
	    }

	    if (!pPixmap) {
		pPixmap = (PixmapPtr)(*pDrawable->pScreen->CreatePixmap)
			    (pDrawable->pScreen, width, height, 1, XYBitmap);
		if (!pPixmap)
		    return;
	    }

	    pglyph = FONTGLYPHBITS(pglyphBase, pci);
	    nbyPadGlyph = PixmapBytePad(gWidth, 1);

	    if (nbyGlyphWidth == nbyPadGlyph)
            {
                pb = pglyph;
            }
            else
            {
                for (i=0, pb = pbits; i<gHeight; i++, pb = pbits+(i*nbyPadGlyph))
                    for (j = 0; j < nbyGlyphWidth; j++)
                        *pb++ = *pglyph++;
                pb = pbits;
            }
    	    /* SI: END */

	    if ((pGCtmp->serialNumber) != (pPixmap->drawable.serialNumber))
		ValidateGC(pPixmap, pGCtmp);
	    (*pGCtmp->ops->PutImage)(pPixmap, pGCtmp, pPixmap->drawable.depth,
			    0, 0, gWidth, gHeight, 
			    0, XYBitmap, pb);

	    if ((pGC->serialNumber) != (pDrawable->serialNumber))
		ValidateGC(pDrawable, pGC);
	    (*pGC->ops->PushPixels)(pGC, pPixmap, pDrawable,
			   gWidth, gHeight,
			   x + pci->metrics.leftSideBearing,
			   y - pci->metrics.ascent);
	    x += pci->metrics.characterWidth;
	}
	break;
      case rgnPART:
	{
	  TEXTPOS *ppos;
	  int xpos, nbox;
	  int w;			/* width of glyph in bits */
	  int h;			/* height of glyph in bits */
	  int leftEdge, rightEdge;
	  int topEdge, bottomEdge;
	  int glyphRow;
	  int widthGlyph;
	  BoxPtr pbox;
	  BoxRec clip;

	  if (!(ppos = (TEXTPOS *)ALLOCATE_LOCAL(nglyph * sizeof(TEXTPOS))))
		return;

		/* Check for slow code here */
	  if (!dwind || (!(si_hasmsstplblt&&si_hasstipple(SIavail_stplblt))) ||
	      pGC->fillStyle != FillSolid) {
	    if (!pGCtmp) {
		pGCtmp = GetScratchGC(1, pDrawable->pScreen);
		gcvals[0] = GXcopy;
		gcvals[1] = 1;
		gcvals[2] = 0;
		DoChangeGC(pGCtmp,GCFunction|GCForeground|GCBackground,gcvals,0);
	    }

	    if (!pPixmap) {
		pPixmap = (PixmapPtr)(*pDrawable->pScreen->CreatePixmap)
			    (pDrawable->pScreen, width, height, 1, XYBitmap);
		if (!pPixmap)
		    return;
	    }

	    while(nglyph--)
	    {
	      pci = *ppci++;
	      pglyph = FONTGLYPHBITS(pglyphBase , pci);
	      gWidth = GLYPHWIDTHPIXELS(pci);
	      gHeight = GLYPHHEIGHTPIXELS(pci);
	      nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(pci);
	      nbyPadGlyph = PixmapBytePad(gWidth, 1);

	    if (nbyGlyphWidth == nbyPadGlyph)
            {
                pb = pglyph;
            }
            else
            {
                for (i=0, pb = pbits; i<gHeight; i++, pb = pbits+(i*nbyPadGlyph))
                    for (j = 0; j < nbyGlyphWidth; j++)
                        *pb++ = *pglyph++;
                pb = pbits;
            }
    	    /* SI: END */
	      if ((pGCtmp->serialNumber) != (pPixmap->drawable.serialNumber))
		  ValidateGC(pPixmap, pGCtmp);
	      (*pGCtmp->ops->PutImage)(pPixmap, pGCtmp, pPixmap->drawable.depth,
				  0, 0, gWidth, gHeight, 
				  0, XYBitmap, pb);

	      if ((pGC->serialNumber) != (pDrawable->serialNumber))
		  ValidateGC(pDrawable, pGC);
	      (*pGC->ops->PushPixels)(pGC, pPixmap, pDrawable,
				 gWidth, gHeight,
				 x + pci->metrics.leftSideBearing,
				 y - pci->metrics.ascent);
	      x += pci->metrics.characterWidth;
	    }
	    break;
	  }

	  xpos = x;

	  for(j = 0; j < nglyph; j++)
	  {
	      pci = ppci[j];
	      ppos[j].xpos = xpos;
	      ppos[j].leftEdge = xpos + pci->metrics.leftSideBearing;
	      ppos[j].rightEdge = xpos + pci->metrics.rightSideBearing;
	      ppos[j].topEdge = y - pci->metrics.ascent;
	      ppos[j].bottomEdge = y + pci->metrics.descent;
	      ppos[j].widthGlyph = GLYPHWIDTHPIXELS(pci);
	      xpos += pci->metrics.characterWidth;
	  }

	  pbox = REGION_RECTS (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);
	  nbox = REGION_NUM_RECTS (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);

	  tmpBM.Bdepth = 1;
	  tmpBM.BorgX = tmpBM.BorgY = 0;

	  pbox--;
	  while(nbox--)
	  {
	      pbox++;
	      clip.x1 = max(bbox.x1, pbox->x1);
	      clip.y1 = max(bbox.y1, pbox->y1);
	      clip.x2 = min(bbox.x2, pbox->x2);
	      clip.y2 = min(bbox.y2, pbox->y2);
	      if ((clip.x2<=clip.x1) || (clip.y2<=clip.y1))
		 continue;

	      for(j = 0; j < nglyph; j++)
	      {
		  pci = ppci[j];

		  /* clip the left and right edges */
		  if (ppos[j].leftEdge < clip.x1)
			leftEdge = clip.x1;
		  else
			leftEdge = ppos[j].leftEdge;
		  if (ppos[j].rightEdge > clip.x2)
			rightEdge = clip.x2;
		  else
			rightEdge = ppos[j].rightEdge;

		  w = rightEdge - leftEdge;
		  if (w <= 0)
		      continue;

		  /* clip the top and bottom edges */
		  if (ppos[j].topEdge < clip.y1)
			topEdge = clip.y1;
		  else
			topEdge = ppos[j].topEdge;
		  if (ppos[j].bottomEdge > clip.y2)
			bottomEdge = clip.y2;
		  else
			bottomEdge = ppos[j].bottomEdge;

		  h = bottomEdge - topEdge;
		  if (h <= 0)
		      continue;

		  glyphRow = (topEdge - y) + pci->metrics.ascent;
		  widthGlyph = ppos[j].widthGlyph;
		  tmpBM.Bwidth = widthGlyph;
		  tmpBM.Bheight = h;
		  pglyph = FONTGLYPHBITS(pglyphBase,pci);
		  pglyph += (glyphRow * GLYPHWIDTHBYTESPADDED(pci));
		  tmpBM.Bptr = (SIArray) pglyph;
		  si_MSstplblt(&tmpBM,
				leftEdge - ppos[j].leftEdge, 0,
				leftEdge, topEdge,
				w, h, 0,
				SGStipple);
	      }
	  }
	  DEALLOCATE_LOCAL(ppos);
	  break;
	}
    default:
	break;
    }

#ifndef FLUSH_IN_BH
    if (dwind)
	si_Flushcache();
#endif

    if (pPixmap)
	(*pDrawable->pScreen->DestroyPixmap)(pPixmap);
    if (pGCtmp)
	FreeScratchGC(pGCtmp);
    DEALLOCATE_LOCAL(pbits);
}


void
miImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    ExtentInfoRec info;			/* used by QueryGlyphExtents */
    long gcvals[3];
    int oldAlu, oldFS;
    unsigned long oldFG;
    xRectangle backrect;
    int width, height;
    PixmapPtr pPixmap = (PixmapPtr)0;
    int nbyLine;			/* bytes per line of padded pixmap */
    FontRec *pfont;
    GCPtr pGCtmp = (GCPtr) 0;
    int i;
    register int j;
    unsigned char *pbits;		/* buffer for PutImage */
    register unsigned char *pb;		/* temp pointer into buffer */
    CharInfoPtr pci;			/* currect char info */
    register unsigned char *pglyph;	/* pointer bits in glyph */
    int gWidth, gHeight;		/* width and height of glyph */
    int nbyGlyphWidth;			/* bytes per scanline of glyph */
    int nbyPadGlyph;			/* server padded line of glyph */
    int size;
    SIbitmap tmpBM;
    BoxRec bbox;
    SIint32 stipplekind;
    int	dwind = 0;			/* Set if a drawable window */
    int backfill = 0;

    if (pDrawable->type == DRAWABLE_WINDOW)
	dwind++;
    else if (pDrawable->depth == 1) {
	if ((pGC->font) &&
		FONTMAXBOUNDS(pGC->font, rightSideBearing) -
	     FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32)
	{
	    /* special case ImageGlyphBlt for terminal emulator fonts */
	    if ((pGC->font) &&
		TERMINALFONT(pGC->font) &&
		(pGC->fgPixel != pGC->bgPixel))
	    {
#ifdef NO_MFBOPTIMIZE
		if (pGC->fgPixel)
		    mfbTEGlyphBltWhite(pDrawable, pGC, x, y,
				       nglyph, ppci, pglyphBase);
		else
		    mfbTEGlyphBltBlack(pDrawable, pGC, x, y,
				       nglyph, ppci, pglyphBase);
#else
		    mfbTEGlyphBlt(pDrawable, pGC, x, y,
				       nglyph, ppci, pglyphBase, pGC->fgPixel);
#endif
	    }
	    else
	    {
#ifdef NO_MFBOPTIMIZE
	        if (pGC->fgPixel == 0)
		    mfbImageGlyphBltBlack(pDrawable, pGC, x, y,
					  nglyph, ppci, pglyphBase);
	        else
		    mfbImageGlyphBltWhite(pDrawable, pGC, x, y,
					  nglyph, ppci, pglyphBase);
#else
		    mfbImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase, pGC->fgPixel);
#endif
	    }
	    return;
	}
    }

    stipplekind = SGOPQStipple;

    QueryGlyphExtents(pGC->font, ppci, nglyph, &info);

    if (!dwind || !TERMINALFONT(pGC->font) ||
	(TERMINALFONT(pGC->font) && !si_hasopqstipple(SIavail_stplblt)) ||
	pGC->fillStyle != FillSolid) {
	stipplekind = SGStipple;
	backrect.x = x;
	backrect.y = y - FONTASCENT(pGC->font);
	backrect.width = info.overallWidth;
	backrect.height = FONTASCENT(pGC->font) + 
		      FONTDESCENT(pGC->font);
	oldAlu = pGC->alu;
	oldFG = pGC->fgPixel;
	oldFS = pGC->fillStyle;

	/* fill in the background */
	gcvals[0] = (long) GXcopy;
	gcvals[1] = (long) pGC->bgPixel;
	gcvals[2] = (long) FillSolid;
	DoChangeGC(pGC, GCFunction|GCForeground|GCFillStyle, gcvals, 0);
	ValidateGC(pDrawable, pGC);
	(*pGC->ops->PolyFillRect)(pDrawable, pGC, 1, &backrect);

	/* put down the glyphs */
	gcvals[0] = (long) oldFG;
	DoChangeGC(pGC, GCForeground, gcvals, 0);
	ValidateGC(pDrawable, pGC);
	backfill++;
    }

    if (dwind && (pGC->miTranslate))
    {
	x += pDrawable->x;
	y += pDrawable->y;
    }

    bbox.x1 = x + info.overallLeft;
    bbox.x2 = x + info.overallRight;
    bbox.y1 = y - info.overallAscent;
    bbox.y2 = y + info.overallDescent;


    pfont = pGC->font;
    width = FONTMAXBOUNDS(pfont,rightSideBearing) - 
	FONTMINBOUNDS(pfont, leftSideBearing);
    height = FONTMAXBOUNDS(pfont,ascent) + 
	FONTMAXBOUNDS(pfont, descent);


    if (!dwind || !si_hasmsstplblt || pGC->fillStyle != FillSolid) {
	pPixmap = (PixmapPtr)(*pDrawable->pScreen->CreatePixmap)
	    (pDrawable->pScreen, width, height, 1, XYBitmap);
	if (!pPixmap)
	    return;
    }

    nbyLine = PixmapBytePad(width, 1);
    size = height*nbyLine;
    pbits = (unsigned char *)ALLOCATE_LOCAL(size);
    if (!pbits)
        return ;

    if (dwind) {
#ifndef FLUSH_IN_BH
	si_Initcache();
#endif
	if (si_hasmsstplblt && pGC->fillStyle == FillSolid)
	    si_PrepareGS(pGC);
    }

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the rect conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDrawable))
        {
            if (SUCheckBox(pDrawable, &bbox))
            {
	        siSUScanWindows(pDrawable, pGC->subWindowMode, NULL, &bbox);
            }
        }
#endif

    switch ((*pGC->pScreen->RectIn)(
	((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip, &bbox))
    {
      case rgnOUT:
	break;
      case rgnIN:
	tmpBM.Bdepth = 1;
	tmpBM.BorgX = tmpBM.BorgY = 0;
	while(nglyph--)
	{
	    pci = *ppci++;
	    gWidth = GLYPHWIDTHPIXELS(pci);
	    gHeight = GLYPHHEIGHTPIXELS(pci);
	    nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(pci);

	    /*
	     * This Stippleblt save a large amount of code execution time.
	     * Including at least 4 malloc's and frees for each character
	     * (Among other things).
	     */
	    if (dwind) {
		if (si_hasmsstplblt && pGC->fillStyle == FillSolid) {
		    tmpBM.Bwidth = gWidth;
		    tmpBM.Bheight = gHeight;
		    /*tmpBM.Bptr = (SIArray) (pglyphBase + byteOffset);*/
		    tmpBM.Bptr = (SIArray) FONTGLYPHBITS(pglyphBase, pci);
		    si_MSstplblt(&tmpBM, 0, 0, 
			   x + pci->metrics.leftSideBearing,
			   y - pci->metrics.ascent,
			   gWidth, gHeight, 0,
			   stipplekind);
		    x += pci->metrics.characterWidth;
		    continue;
		}
	    }

	    /*
	     * Real slow code.
	     */

	    if (!pGCtmp) {
		pGCtmp = GetScratchGC(1, pDrawable->pScreen);
		gcvals[0] = GXcopy;
		gcvals[1] = 1;
		gcvals[2] = 0;
		DoChangeGC(pGCtmp,GCFunction|GCForeground|GCBackground,gcvals,0);
	    }

	    if (!pPixmap) {
		pPixmap = (PixmapPtr)(*pDrawable->pScreen->CreatePixmap)
			    (pDrawable->pScreen, width, height, 1, XYBitmap);
		if (!pPixmap)
		    return;
	    }

	    pglyph = FONTGLYPHBITS(pglyphBase, pci);
	    nbyPadGlyph = PixmapBytePad(gWidth, 1);

	    if (nbyGlyphWidth == nbyPadGlyph)
            {
                pb = pglyph;
            }
            else
            {
                for (i=0, pb = pbits; i<gHeight; i++, pb = pbits+(i*nbyPadGlyph))
                    for (j = 0; j < nbyGlyphWidth; j++)
                        *pb++ = *pglyph++;
                pb = pbits;
            }
    	    /* SI: END */

	    if ((pGCtmp->serialNumber) != (pPixmap->drawable.serialNumber))
		ValidateGC(pPixmap, pGCtmp);
	    (*pGCtmp->ops->PutImage)(pPixmap, pGCtmp, pPixmap->drawable.depth,
			    0, 0, gWidth, gHeight, 
			    0, XYBitmap, pb);

	    if ((pGC->serialNumber) != (pDrawable->serialNumber))
		ValidateGC(pDrawable, pGC);
	    (*pGC->ops->PushPixels)(pGC, pPixmap, pDrawable,
			   gWidth, gHeight,
			   x + pci->metrics.leftSideBearing,
			   y - pci->metrics.ascent);
	    x += pci->metrics.characterWidth;
	}
	break;
      case rgnPART:
	{
	  TEXTPOS *ppos;
	  int xpos, nbox;
	  int w;			/* width of glyph in bits */
	  int h;			/* height of glyph in bits */
	  int leftEdge, rightEdge;
	  int topEdge, bottomEdge;
	  int glyphRow;
	  int widthGlyph;
	  BoxPtr pbox;
	  BoxRec clip;

	  if (!(ppos = (TEXTPOS *)ALLOCATE_LOCAL(nglyph * sizeof(TEXTPOS))))
		return;

		/* Check for slow code here */
	  if (!dwind || !si_hasmsstplblt || pGC->fillStyle != FillSolid) {
	    if (!pGCtmp) {
		pGCtmp = GetScratchGC(1, pDrawable->pScreen);
		gcvals[0] = GXcopy;
		gcvals[1] = 1;
		gcvals[2] = 0;
		DoChangeGC(pGCtmp,GCFunction|GCForeground|GCBackground,gcvals,0);
	    }

	    if (!pPixmap) {
		pPixmap = (PixmapPtr)(*pDrawable->pScreen->CreatePixmap)
			    (pDrawable->pScreen, width, height, 1, XYBitmap);
		if (!pPixmap)
		    return;
	    }

	    while(nglyph--)
	    {
	      pci = *ppci++;
	      pglyph = FONTGLYPHBITS(pglyphBase, pci);
	      gWidth = GLYPHWIDTHPIXELS(pci);
	      gHeight = GLYPHHEIGHTPIXELS(pci);
	      nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(pci);
	      nbyPadGlyph = PixmapBytePad(gWidth, 1);

	    if (nbyGlyphWidth == nbyPadGlyph)
            {
                pb = pglyph;
            }
            else
            {
                for (i=0, pb = pbits; i<gHeight; i++, pb = pbits+(i*nbyPadGlyph))
                    for (j = 0; j < nbyGlyphWidth; j++)
                        *pb++ = *pglyph++;
                pb = pbits;
            }
    	    /* SI: END */

	      if ((pGCtmp->serialNumber) != (pPixmap->drawable.serialNumber))
		  ValidateGC(pPixmap, pGCtmp);
	      (*pGCtmp->ops->PutImage)(pPixmap, pGCtmp, pPixmap->drawable.depth,
				  0, 0, gWidth, gHeight, 
				  0, XYBitmap, pb);

	      if ((pGC->serialNumber) != (pDrawable->serialNumber))
		  ValidateGC(pDrawable, pGC);
	      (*pGC->ops->PushPixels)(pGC, pPixmap, pDrawable,
				 gWidth, gHeight,
				 x + pci->metrics.leftSideBearing,
				 y - pci->metrics.ascent);
	      x += pci->metrics.characterWidth;
	    }
	    break;
	  }

	  xpos = x;

	  for(j = 0; j < nglyph; j++)
	  {
	      pci = ppci[j];
	      ppos[j].xpos = xpos;
	      ppos[j].leftEdge = xpos + pci->metrics.leftSideBearing;
	      ppos[j].rightEdge = xpos + pci->metrics.rightSideBearing;
	      ppos[j].topEdge = y - pci->metrics.ascent;
	      ppos[j].bottomEdge = y + pci->metrics.descent;
	      ppos[j].widthGlyph = GLYPHWIDTHPIXELS(pci);
	      xpos += pci->metrics.characterWidth;
	  }

	  pbox = REGION_RECTS (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);
	  nbox = REGION_NUM_RECTS (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);

	  tmpBM.Bdepth = 1;
	  tmpBM.BorgX = tmpBM.BorgY = 0;

	  pbox--;
	  while(nbox--)
	  {
	      pbox++;
	      clip.x1 = max(bbox.x1, pbox->x1);
	      clip.y1 = max(bbox.y1, pbox->y1);
	      clip.x2 = min(bbox.x2, pbox->x2);
	      clip.y2 = min(bbox.y2, pbox->y2);
	      if ((clip.x2<=clip.x1) || (clip.y2<=clip.y1))
		 continue;

	      for(j = 0; j < nglyph; j++)
	      {
		  pci = ppci[j];

		  /* clip the left and right edges */
		  if (ppos[j].leftEdge < clip.x1)
			leftEdge = clip.x1;
		  else
			leftEdge = ppos[j].leftEdge;
		  if (ppos[j].rightEdge > clip.x2)
			rightEdge = clip.x2;
		  else
			rightEdge = ppos[j].rightEdge;

		  w = rightEdge - leftEdge;
		  if (w <= 0)
		      continue;

		  /* clip the top and bottom edges */
		  if (ppos[j].topEdge < clip.y1)
			topEdge = clip.y1;
		  else
			topEdge = ppos[j].topEdge;
		  if (ppos[j].bottomEdge > clip.y2)
			bottomEdge = clip.y2;
		  else
			bottomEdge = ppos[j].bottomEdge;

		  h = bottomEdge - topEdge;
		  if (h <= 0)
		      continue;

		  glyphRow = (topEdge - y) + pci->metrics.ascent;
		  widthGlyph = ppos[j].widthGlyph;
		  tmpBM.Bwidth = widthGlyph;
		  tmpBM.Bheight = h;
		  pglyph = FONTGLYPHBITS(pglyphBase, pci);
		  pglyph += (glyphRow * GLYPHWIDTHBYTESPADDED(pci));
		  tmpBM.Bptr = (SIArray) pglyph;
		  si_MSstplblt(&tmpBM,
				leftEdge - ppos[j].leftEdge, 0,
				leftEdge, topEdge,
				w, h, 0,
				stipplekind);
	      }
	  }
	  DEALLOCATE_LOCAL(ppos);
	  break;
	}
    default:
	break;
    }

#ifndef FLUSH_IN_BH
    if (dwind)
	si_Flushcache();
#endif

    if (pPixmap)
	(*pDrawable->pScreen->DestroyPixmap)(pPixmap);
    if (pGCtmp)
	FreeScratchGC(pGCtmp);
    DEALLOCATE_LOCAL(pbits);
    if (backfill) {
	/* put all the toys away when done playing */
	gcvals[0] = (long) oldAlu;
	gcvals[1] = (long) oldFG;
	gcvals[2] = (long) oldFS;
	DoChangeGC(pGC, GCFunction|GCForeground|GCFillStyle, gcvals, 0);
    }
}

