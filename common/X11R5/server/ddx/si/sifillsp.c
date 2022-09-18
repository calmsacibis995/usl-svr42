/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sifillsp.c	1.8"

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

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.
                    All Rights Reserved
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

/* $XConsortium: cfbfillsp.c,v 5.7 89/11/24 18:09:00 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "regionstr.h"

#include "si.h"
#include "sidep.h"
#include "simskbits.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

extern SIBool si_fill_spans();
extern SIBool si_linespans();
extern SIBool si_rectspans();
extern unsigned int si_pfill();

/*
 * Since some objects are commonly filled with spans, but can be 
 * clipped at the object level instead of the spans level, we set
 * the global flag si_noclip if no clipping needs to be done in the
 * spans routines.  Filled arcs are a good example of where this is
 * used.
 */
int si_noclip = 0;

extern void mfbInvertSolidFS(), mfbBlackSolidFS(), mfbWhiteSolidFS();

extern	void siTestDDXPts ();

/* scanline filling for color frame buffer
   written by drewry, oct 1986 modified by smarks
   changes for compatibility with Little-endian systems Jul 1987; MIT:yba.

   these routines all clip.  they assume that anything that has called
them has already translated the points (i.e. pGC->miTranslate is
non-zero, which is howit gets set in cfbCreateGC().)

   the number of new scnalines created by clipping ==
MaxRectsPerBand * nSpans.

    FillSolid is overloaded to be used for OpaqueStipple as well,
if fgPixel == bgPixel.  
Note that for solids, PrivGC.rop == PrivGC.ropOpStip


    FillTiled is overloaded to be used for OpaqueStipple, if
fgPixel != bgPixel.  based on the fill style, it uses
{RotatedTile, gc.alu} or {RotatedStipple, PrivGC.ropOpStip}
*/

#ifdef	notdef
#include	<stdio.h>
static
dumpspans(n, ppt, pwidth)
    int	n;
    DDXPointPtr ppt;
    int *pwidth;
{
    fprintf(stderr,"%d spans\n", n);
    while (n--) {
	fprintf(stderr, "[%d,%d] %d\n", ppt->x, ppt->y, *pwidth);
	ppt++;
	pwidth++;
    }
    fprintf(stderr, "\n");
}
#endif

void
siSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */
    int *addrlBase;		/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register int *addrl;	/* pointer to current longword in bitmap */
    register int width;		/* current span width */
    register int nlmiddle;
    register int fill;
    register int x;
    register int startmask;
    register int endmask;
    int rop;			/* rasterop */
    int planemask;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    int rrop;
    int isscr = 0;

    if ((!(planemask = pGC->planemask)) || (!nInit) )
	return;

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

    if ((pDrawable->depth == 1) && (pDrawable->type != DRAWABLE_WINDOW)) {
	    rrop = ReduceRop(pGC->alu, pGC->fgPixel);
	    switch (rrop) {
		case RROP_BLACK:
		    mfbBlackSolidFS(pDrawable, pGC, nInit, pptInit,
			pwidthInit, fSorted);
		    return;
		case RROP_WHITE:
		    mfbWhiteSolidFS(pDrawable, pGC, nInit, pptInit,
			pwidthInit, fSorted);
		    return;
		case RROP_NOP:
		    return;
		case RROP_INVERT:
		    mfbInvertSolidFS(pDrawable, pGC, nInit, pptInit,
			pwidthInit, fSorted);
		    return;
	    }
    }

    if (!si_noclip) {
	n = nInit * miFindMaxBand(((siPrivGC *)(pGC->devPrivates[
				    siGCPrivateIndex].ptr))->pCompositeClip);
	pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!pptFree || !pwidthFree) {
	    if (pptFree) DEALLOCATE_LOCAL(pptFree);
	    if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	    return;
	}
	pwidth = pwidthFree;
	ppt = pptFree;
	n = miClipSpans(((siPrivGC *)(pGC->devPrivates[
				      siGCPrivateIndex].ptr))->pCompositeClip,
		         pptInit, pwidthInit, nInit,
		         ppt, pwidth, fSorted);
    }
    else {
	n = nInit;
	ppt = pptInit;
	pwidth = pwidthInit;
    }

    /*
     * If we can fill the spans using an SDD routine, do it here.
     */
    if (si_canspansfill && pDrawable->type == DRAWABLE_WINDOW)
	if (si_fill_spans(pDrawable,pGC,n,ppt,pwidth) == SI_SUCCEED)
            goto siSolidFS_done;

    /*
     * See if we can do the span filling with an sdd line drawing
     * function.
     */
    if (si_linespans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
        goto siSolidFS_done;

#ifdef	notdef
    dumpspans(n, ppt, pwidth);
#endif
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
        nlwidth = si_getscanlinelen;
        isscr++;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    PPW = 32 / pDrawable->depth;
    PWSH = si_pix_to_word[PPW];
    PMSK = (1 << pDrawable->depth) - 1;
    PSZ = pDrawable->depth;
    rop = pGC->alu;
    fill = si_pfill(pDrawable->depth, pGC->fgPixel);
    planemask = si_pfill(pDrawable->depth, planemask);

    if (rop == GXcopy && (planemask & PMSK) == PMSK)
    {
	while (n--)
	{
	    x = ppt->x;
	    if (isscr) {
	        addrl = addrlBase = (int *)si_getscanline(ppt->y);
	    } else
	        addrl = addrlBase + (ppt->y * nlwidth);
	    width = *pwidth++;
	    if (!width)
		continue;

	    if ( ((x & PIM) + width) <= PPW)
	    {
		addrl += x >> PWSH;
		maskpartialbits(x, width, startmask);
		*addrl = (*addrl & ~startmask) | (fill & startmask);
	    }
	    else
	    {
		addrl += x >> PWSH;
		maskbits(x, width, startmask, endmask, nlmiddle);
		if ( startmask ) {
		    *addrl = *addrl & ~startmask | fill & startmask;
		    ++addrl;
		}
		while ( nlmiddle-- )
		    *addrl++ = fill;
		if ( endmask )
		    *addrl = *addrl & ~endmask | fill & endmask;
	    }
            if (isscr)
                si_setscanline(ppt->y, addrlBase);
	    ++ppt;
	}
    }
    else if ((rop == GXxor && (planemask & PMSK) == PMSK) || rop == GXinvert)
    {
	if (rop == GXinvert)
	    fill = planemask;
	while (n--)
	{
	    x = ppt->x;
	    if (isscr) {
	        addrl = addrlBase = (int *)si_getscanline(ppt->y);
	    } else
	        addrl = addrlBase + (ppt->y * nlwidth);
	    width = *pwidth++;
	    if (!width)
		continue;

	    if ( ((x & PIM) + width) <= PPW)
	    {
		addrl += x >> PWSH;
		maskpartialbits(x, width, startmask);
		*addrl ^= (fill & startmask);
	    }
	    else
	    {
		addrl += x >> PWSH;
		maskbits(x, width, startmask, endmask, nlmiddle);
		if ( startmask )
		    *addrl++ ^= (fill & startmask);
		while ( nlmiddle-- )
		    *addrl++ ^= fill;
		if ( endmask )
		    *addrl ^= (fill & endmask);
	    }
            if (isscr)
                si_setscanline(ppt->y, addrlBase);
	    ++ppt;
	}
    }
    else
    {
    	while (n--)
    	{
	    x = ppt->x;
	    if (isscr) {
	        addrlBase = (int *)si_getscanline(ppt->y);
	        addrl = addrlBase + (ppt->x >> PWSH);
	    } else
	        addrl = addrlBase + (ppt->y * nlwidth) + (x >> PWSH);
	    width = *pwidth++;
	    if (width)
	    {
	    	if ( ((x & PIM) + width) <= PPW)
	    	{
		    maskpartialbits(x, width, startmask);
		    *addrl = *addrl & ~(planemask & startmask) |
			     DoRop(rop, fill, *addrl) & (planemask & startmask);
	    	}
	    	else
	    	{
		    maskbits(x, width, startmask, endmask, nlmiddle);
		    if ( startmask ) {
			*addrl = *addrl & ~(planemask & startmask) |
			         DoRop (rop, fill, *addrl) & (planemask & startmask);
		    	++addrl;
		    }
		    while ( nlmiddle-- ) {
			*addrl = (*addrl & ~planemask) |
				 DoRop (rop, fill, *addrl) & planemask;
		    	++addrl;
		    }
		    if ( endmask ) {
			*addrl = *addrl & ~(planemask & endmask) |
			         DoRop (rop, fill, *addrl) & (planemask & endmask);
		    }
	    	}
	    }
            if (isscr)
                si_setscanline(ppt->y, addrlBase);
	    ++ppt;
    	}
    }
siSolidFS_done:

    if (!si_noclip) {
    	DEALLOCATE_LOCAL(pptFree);
    	DEALLOCATE_LOCAL(pwidthFree);
    }
}


/* Fill spans with tiles that aren't 32 bits wide */
void
siUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
    int		iline;		/* first line of tile to use */
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int		*addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register int *pdst;		/* pointer to current word in bitmap */
    register int *psrc;		/* pointer to current word in tile */
    register int nlMiddle;
    register int startmask;
    PixmapPtr	pTile;		/* pointer to tile we want to fill with */
    int		w, width, x, tmpSrc, srcStartOver, nstart, nend;
    int		endinc; 
    int		xSrc, ySrc;
    int 	endmask, tlwidth, rem, tileWidth, *psrcT, rop;
    int		tileHeight;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    unsigned int planemask;

    int		isscr = 0;

    if ((!(planemask = pGC->planemask)) || (!nInit) )
	return;

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

    if (!si_noclip) {
	n = nInit * miFindMaxBand(((siPrivGC *)(pGC->devPrivates[
				    siGCPrivateIndex].ptr))->pCompositeClip);
	pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!pptFree || !pwidthFree) {
	    if (pptFree) DEALLOCATE_LOCAL(pptFree);
	    if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	    return;
	}
	pwidth = pwidthFree;
	ppt = pptFree;
	n = miClipSpans(((siPrivGC *)(pGC->devPrivates[
				      siGCPrivateIndex].ptr))->pCompositeClip,
		         pptInit, pwidthInit, nInit,
		         ppt, pwidth, fSorted);
    }
    else {
	n = nInit;
	ppt = pptInit;
	pwidth = pwidthInit;
    }

    /*
     * If we can fill the spans using an SDD routine, do it here.
     */
    if (si_canspansfill && si_hastile(SIavail_spans) &&
	pDrawable->type == DRAWABLE_WINDOW)
	if (si_fill_spans(pDrawable,pGC,n,ppt,pwidth) == SI_SUCCEED)
	    goto siUnnaturalTileFS_done;

    /*
     * See if we can do the span filling with an sdd line drawing
     * function.
     */
    if (si_rectspans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
	goto siUnnaturalTileFS_done;

    PPW = 32 / pDrawable->depth;
    PWSH = si_pix_to_word[PPW];
    PSZ = pDrawable->depth;
    planemask = si_pfill(pDrawable->depth, planemask);
    if (pGC->fillStyle == FillTiled)
    {
	pTile = pGC->tile.pixmap;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }
    else
    {
	pTile = pGC->stipple;
	pTile = pGC->stipple;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }

    xSrc = pDrawable->x;
    ySrc = pDrawable->y;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
        nlwidth = si_getscanlinelen;
        isscr++;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    tileWidth = pTile->drawable.width;
    tileHeight = pTile->drawable.height;

    /* this replaces rotating the tile. Instead we just adjust the offset
     * at which we start grabbing bits from the tile.
     * Ensure that ppt->x - xSrc >= 0 and ppt->y - ySrc >= 0,
     * so that iline and xrem always stay within the tile bounds.
     */
    xSrc += (pGC->patOrg.x % tileWidth) - tileWidth;
    ySrc += (pGC->patOrg.y % tileHeight) - tileHeight;

    while (n--)
    {
	iline = (ppt->y - ySrc) % tileHeight;
	if (isscr) {
            addrlBase = (int *)si_getscanline(ppt->y);
            pdst = addrlBase + (ppt->x >> PWSH);
	} else
            pdst = addrlBase + (ppt->y * nlwidth) + (ppt->x >> PWSH);
        psrcT = (int *) pTile->devPrivate.ptr + (iline * tlwidth);
	x = ppt->x;

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		rem = (x - xSrc) % tileWidth;
		psrc = psrcT + rem / PPW;
	        w = min(tileWidth, width);
		w = min(w,tileWidth-rem);
#ifdef notdef
		if((rem = x % tileWidth) != 0)
		{
		    w = min(min(tileWidth - rem, width), PPW);
		    /* we want to grab from the end of the tile.  Figure
		     * out where that is.  In general, the start of the last
		     * word of data on this scanline is tlwidth -1 words 
		     * away. But if we want to grab more bits than we'll
		     * find on that line, we have to back up 1 word further.
		     * On the other hand, if the whole tile fits in 1 word,
		     * let's skip the work */ 
		    endinc = tlwidth - 1 - (tileWidth-rem) / PPW;

		    if(endinc)
		    {
			if((rem & PIM) + w > tileWidth % PPW)
			    endinc--;
		    }

		    getbits(psrc + endinc, rem & PIM, w, tmpSrc);
		    putbitsrop(tmpSrc, (x & PIM), w, pdst, planemask, rop);
		    if((x & PIM) + w >= PPW)
			pdst++;
		}
		else
#endif /* notdef */
		if(((x & PIM) + w) <= PPW)
		{
		    getbits(psrc, (rem & PIM), w, tmpSrc);
		    putbitsrop((unsigned int) tmpSrc, x & PIM, w, pdst, planemask, rop);
		    if ((x & PIM) + w == PPW) ++pdst;
		}
		else
		{
		    maskbits(x, w, startmask, endmask, nlMiddle);

	            if (startmask)
		        nstart = PPW - (x & PIM);
	            else
		        nstart = 0;
	            if (endmask)
	                nend = (x + w)  & PIM;
	            else
		        nend = 0;

	            srcStartOver = nstart + (rem & PIM) > PLST;

		    if(startmask)
		    {
			getbits(psrc, rem & PIM, nstart, tmpSrc);
			putbitsrop((unsigned int) tmpSrc, x & PIM, nstart, pdst, 
			    planemask, rop);
			pdst++;
			if(srcStartOver)
			    psrc++;
		    }
		    nstart = (nstart + rem) & PIM;
		    while(nlMiddle--)
		    {
			    getbits(psrc, nstart, PPW, tmpSrc);
			    putbitsrop( (unsigned int) tmpSrc, 0, PPW, pdst, planemask, rop );
			    pdst++;
			    psrc++;
		    }
		    if(endmask)
		    {
			getbits(psrc, nstart, nend, tmpSrc);
			putbitsrop((unsigned int) tmpSrc, 0, nend, pdst, planemask, rop);
		    }
		 }
		 x += w;
		 width -= w;
	    }
            if (isscr)
		si_setscanline(ppt->y, addrlBase);
	}
	ppt++;
	pwidth++;
    }
siUnnaturalTileFS_done:
    if (!si_noclip) {
    	DEALLOCATE_LOCAL(pptFree);
    	DEALLOCATE_LOCAL(pwidthFree);
    }
}

/*
 * getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix )
 *
 * Converts bits to pixels in a reasonable way.	 Takes w (1 <= w <= 4)
 * bits from *psrcstip, starting at bit x; call this a quartet of bits.
 * Then, takes the pixels from *psrcpix corresponding to the one-bits (if
 * ones is TRUE) or the zero-bits (if ones is FALSE) of the quartet
 * and puts these pixels into destpix.
 *
 * Example:
 *
 *	getstipplepixels( &(0x08192A3B), 17, 4, 1, &(0x4C5D6E7F), dest )
 *
 * 0x08192A3B = 0000 1000 0001 1001 0010 1010 0011 1011
 *
 * This will take 4 bits starting at bit 17, so the quartet is 0x5 = 0101.
 * It will take pixels from 0x4C5D6E7F corresponding to the one-bits in this
 * quartet, so dest = 0x005D007F.
 *
 * XXX This should be turned into a macro after it is debugged.
 * XXX Has byte order dependencies.
 * XXX This works for all values of x and w within a doubleword, depending
 *     on the compiler to generate proper code for negative shifts.
 */

void getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix )
unsigned int *psrcstip, *psrcpix, *destpix;
int x, w, ones;
{
    unsigned int QuartetMask, tstpixel, i;
    unsigned int q;

#if (BITMAP_BIT_ORDER == MSBFirst)
    q = ((*psrcstip) >> ((32-PPW)-x)) & ((1 << PPW) - 1);
    if ( x+w > 32 )
	q |= *(psrcstip+1) >> (64-x-w); /* & 0xF ? ****XXX*/
#else
    q = (*psrcstip) >> x;
    if ( x+w > 32 )
	q |= *(psrcstip+1) << (32-x);
    if (PPW < 32)
	q &= ((1 << PPW) - 1);
#endif
    q = QuartetBitsTable[w] & (ones ? q : ~q);
    QuartetMask = 0;
    tstpixel = 1 << (PPW-1);
    for(i = 0; i < PPW; i++) {
	QuartetMask <<= PSZ;
	if (q & tstpixel)
            QuartetMask |= PMSK;
	q <<= 1;
    }
    *destpix = (*(psrcpix)) & QuartetMask;
}

/* Fill spans with stipples that aren't 32 bits wide */
void
siUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int		iline;		/* first line of tile to use */
    int		*addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register int *pdst;		/* pointer to current word in bitmap */
    PixmapPtr	pStipple;	/* pointer to stipple we want to fill with */
    register int w;
    int		width,  x, xrem, xSrc, ySrc;
    unsigned int tmpSrc, tmpDst1, tmpDst2;
    int 	stwidth, stippleWidth, *psrcS, rop, stiprop;
    int		stippleHeight;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    unsigned int fgfill, bgfill;
    unsigned int planemask;
    int isscr = 0;

    if (!(planemask = pGC->planemask))
	return;

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

    if (!si_noclip) {
	n = nInit * miFindMaxBand(((siPrivGC *)(pGC->devPrivates[
				    siGCPrivateIndex].ptr))->pCompositeClip);
	pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!pptFree || !pwidthFree) {
	    if (pptFree) DEALLOCATE_LOCAL(pptFree);
	    if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	    return;
	}
	pwidth = pwidthFree;
	ppt = pptFree;
	n = miClipSpans(((siPrivGC *)(pGC->devPrivates[
				      siGCPrivateIndex].ptr))->pCompositeClip,
		         pptInit, pwidthInit, nInit,
		         ppt, pwidth, fSorted);
    }
    else {
	n = nInit;
	ppt = pptInit;
	pwidth = pwidthInit;
    }

    /*
     * If we can fill the spans using an SDD routine, do it here.
     */
    if (si_canspansfill && si_hasstipple(SIavail_spans) &&
	pDrawable->type == DRAWABLE_WINDOW)
	if (si_fill_spans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
	    goto siUnnaturalStippleFS_done;

    /*
     * See if we can do the span filling with an sdd line drawing
     * function.
     */
    if (si_rectspans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
	goto siUnnaturalStippleFS_done;

    PSZ = pDrawable->depth;
    PPW = 32 / pDrawable->depth;
    PWSH = si_pix_to_word[PPW];
    PMSK = (1 << pDrawable->depth) - 1;
    rop = pGC->alu;

    if (pGC->fillStyle == FillStippled) {
	switch (rop) {
	case GXand:
	case GXcopy:
	case GXnoop:
	case GXor:
	    stiprop = rop;
	    break;
	default:
	    stiprop = rop;
	    rop = GXcopy;
	}
    }

    fgfill = si_pfill(pDrawable->depth, pGC->fgPixel);
    bgfill = si_pfill(pDrawable->depth, pGC->bgPixel);
    planemask = si_pfill(pDrawable->depth, planemask);

    /*
     *  OK,  so what's going on here?  We have two Drawables:
     *
     *  The Stipple:
     *		Depth = 1
     *		Width = stippleWidth
     *		Words per scanline = stwidth
     *		Pointer to pixels = pStipple->devPrivate.ptr
     */
    pStipple = pGC->stipple;

    if (pStipple->drawable.depth != 1) {
	FatalError( "Stipple depth not equal to 1!\n" );
    }

    stwidth = pStipple->devKind >> 2;
    stippleWidth = pStipple->drawable.width;
    stippleHeight = pStipple->drawable.height;

    /*
     *	The Target:
     *		Depth = PSZ
     *		Width = determined from *pwidth
     *		Words per scanline = nlwidth
     *		Pointer to pixels = addrlBase
     */
    xSrc = pDrawable->x;
    ySrc = pDrawable->y;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
        nlwidth = si_getscanlinelen;
        isscr++;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    /* this replaces rotating the stipple. Instead we just adjust the offset
     * at which we start grabbing bits from the stipple.
     * Ensure that ppt->x - xSrc >= 0 and ppt->y - ySrc >= 0,
     * so that iline and xrem always stay within the stipple bounds.
     */
    xSrc += (pGC->patOrg.x % stippleWidth) - stippleWidth;
    ySrc += (pGC->patOrg.y % stippleHeight) - stippleHeight;

    while (n--)
    {
	iline = (ppt->y - ySrc) % stippleHeight;
	x = ppt->x;
	if (isscr)
            pdst = addrlBase = (int *)si_getscanline(ppt->y);
	else
            pdst = addrlBase + (ppt->y * nlwidth);
        psrcS = (int *) pStipple->devPrivate.ptr + (iline * stwidth);

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		int xtemp, tmpx;
		register unsigned int *ptemp;
		register int *pdsttmp;
		/*
		 *  Do a stripe through the stipple & destination w pixels
		 *  wide.  w is not more than:
		 *	-	the width of the destination
		 *	-	the width of the stipple
		 *	-	the distance between x and the next word 
		 *		boundary in the destination
		 *	-	the distance between x and the next word
		 *		boundary in the stipple
		 */

		/* width of dest/stipple */
                xrem = (x - xSrc) % stippleWidth;
	        w = min((stippleWidth - xrem), width);
		/* dist to word bound in dest */
		w = min(w, PPW - (x & PIM));
		/* dist to word bound in stip */
		w = min(w, 32 - (x & 0x1f));

		xtemp = (xrem & 0x1f);
		ptemp = (unsigned int *)(psrcS + (xrem >> 5));
		tmpx = x & PIM;
		pdsttmp = pdst + (x>>PWSH);

		switch ( pGC->fillStyle ) {
		    case FillOpaqueStippled:
			getstipplepixels(ptemp, xtemp, w, 0, &bgfill, &tmpDst1);
			getstipplepixels(ptemp, xtemp, w, 1, &fgfill, &tmpDst2);
			break;
		    case FillStippled:
			/* Fill tmpSrc with the source pixels */
			getbits(pdsttmp, tmpx, w, tmpSrc);
			getstipplepixels(ptemp, xtemp, w, 0, &tmpSrc, &tmpDst1);
			if (rop != stiprop) {
			    putbitsrop(fgfill, 0, w, &tmpSrc, pGC->planemask, stiprop);
			} else {
			    tmpSrc = fgfill;
			}
			getstipplepixels(ptemp, xtemp, w, 1, &tmpSrc, &tmpDst2);
			break;
		}
		tmpDst2 |= tmpDst1;
		putbitsrop(tmpDst2, tmpx, w, pdsttmp, planemask, rop);
		x += w;
		width -= w;
	    }
	}
	if (isscr)
            si_setscanline(ppt->y, addrlBase);
	ppt++;
	pwidth++;
    }
siUnnaturalStippleFS_done:
    if (!si_noclip) {
    	DEALLOCATE_LOCAL(pptFree);
    	DEALLOCATE_LOCAL(pwidthFree);
    }
}


/*
 * si_fill_spans()	-- Try to do a fill spans operation 
 *			using an sdd's span filling capability.
 */
SIBool
si_fill_spans(pDraw, pGC, nInit, pptInit, pwidthInit)
DrawablePtr	pDraw;
GCPtr		pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr	pptInit;	/* pointer to list of start points */
int		*pwidthInit;	/* point to list of n widths */
{
	/*
	 * Firewall for opaque stipple filling since that isn't tested for
	 * earlier (it comes through the tile fill spans routine).
	 */
	if ((pGC->fillStyle == FillOpaqueStippled) && 
	    !(si_canspansfill && si_hasopqstipple(SIavail_spans)))
		return(SI_FAIL);

#ifdef XWIN_SAVE_UNDERS
	    /*
	     * Check to see if the line drawing conflicts with
	     * any save-under windows
	     */
	    if (SUCheckDrawable(pDraw))
	    {
		siTestDDXPts(pDraw, pGC, nInit, pptInit);
	    }
#endif

	si_PrepareGS(pGC);
	return(si_Fillspans(nInit, pptInit, pwidthInit));
}


/*
 * si_linespans()	-- Try to do a solid fill spans operation 
 *			using an sdd's line drawing capability.
 */
SIBool
si_linespans(pDraw, pGC, nInit, pptInit, pwidthInit)
DrawablePtr		pDraw;
GCPtr			pGC;
int			nInit;		/* number of spans to fill */
register DDXPointPtr	pptInit;	/* pointer to list of start points */
int			*pwidthInit;	/* point to list of n widths */
{
	register DDXPointPtr	ppt;
	DDXPointPtr 		pptTmp;
	register int 		i;
	int			nlines;
	int			lsOld, lsNew;
	int			retval;

	/* 
	 * See if we can fill the spans using an sdd line drawing
	 * function.  If we can, set things up, otherwise, return
	 * failure.
	 */
	if ((si_haslineseg || si_haslinedraw) && pDraw->type==DRAWABLE_WINDOW) {
		ppt = pptTmp = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) *
							   (nInit<<1));

		for (i = 0, nlines = 0; i < nInit; i++) {
			if (*pwidthInit) {
				*ppt++ = *pptInit;
				ppt->x = pptInit->x + *pwidthInit - 1;
				ppt->y = pptInit->y;
				ppt++;
				nlines++;
			}
			pptInit++;
			pwidthInit++;
		}
		
		lsOld = pGC->lineStyle;
		lsNew = LineSolid;
		if (pGC->lineStyle != LineSolid) {
			DoChangeGC(pGC, GCLineStyle, &lsNew, 0);
			ValidateGC(pDraw, pGC);
		}
		si_PrepareGS(pGC);
	}
	else
		return(SI_FAIL);

	si_setlineclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);

#ifdef XWIN_SAVE_UNDERS
	    /*
	     * Check to see if the line drawing conflicts with
	     * any save-under windows
	     */
	    if (SUCheckDrawable(pDraw))
	    {
		siTestDDXPts(pDraw, pGC, nInit, pptInit);
	    }
#endif
	if (si_haslineseg) {
		if ((retval = si_onebitlineseg(nlines << 1, pptTmp)) == SI_FAIL)
			goto linespans_done;
	} else {
		ppt = pptTmp;
		for (i=nlines; --i >= 0; ppt += 2)
			if ((retval = si_onebitlinedraw(2, ppt)) == SI_FAIL)
				goto linespans_done;
	}

linespans_done:
	if (lsOld != LineSolid) {
		DoChangeGC(pGC, GCLineStyle, &lsOld, 0);
		ValidateGC(pDraw, pGC);
	}
	DEALLOCATE_LOCAL(pptTmp);
	return(retval);
}


/*
 * si_rectspans()	-- Try to do a fill spans operation 
 *			using an sdd's rectangle filling capability.
 */
SIBool
si_rectspans(pDraw, pGC, nInit, pptInit, pwidthInit)
DrawablePtr		pDraw;
GCPtr			pGC;
int			nInit;		/* number of spans to fill */
register DDXPointPtr	pptInit;	/* pointer to list of start points */
int			*pwidthInit;	/* point to list of n widths */
{
	register SIRectP	prect;
	SIRectP			prectInit;
	int			nrect, retval;
	register int 		i;

	/* 
	 * See if we can fill the spans using an sdd rectagle drawing
	 * function.  If we can, set things up, otherwise, return
	 * failure.
	 */
	if ((si_canpolyfill || si_hasfillrectangle) && 
	     pDraw->type==DRAWABLE_WINDOW) {
		prect=prectInit = (SIRectP)ALLOCATE_LOCAL(sizeof(SIRect)*nInit);

		for (i = 0, nrect = 0; i < nInit; i++) {
			if (*pwidthInit) {
				prect->ul.x = pptInit->x;
				prect->ul.y = pptInit->y;
				prect->lr.x = pptInit->x + *pwidthInit;
				prect->lr.y = pptInit->y + 1;
				prect++;
				nrect++;
			}
			pptInit++;
			pwidthInit++;
		}
		
		si_PrepareGS(pGC);
	}
	else 
		return(SI_FAIL);

	if (!si_hascliplist(SIavail_fpoly))
		si_setpolyclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);

#ifdef XWIN_SAVE_UNDERS
	    /*
	     * Check to see if the line drawing conflicts with
	     * any save-under windows
	     */
	    if (SUCheckDrawable(pDraw))
	    {
		siTestDDXPts(pDraw, pGC, nInit, pptInit);
	    }
#endif
	retval = si_fillrectangle(nrect, prectInit);

	DEALLOCATE_LOCAL(prectInit);
	return(retval);
}

#ifdef XWIN_SAVE_UNDERS
void
siTestDDXPts(pDraw, pGC, npts, pPts)
DrawablePtr pDraw;
GCPtr       pGC;
int         npts;
DDXPointPtr pPts;
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
