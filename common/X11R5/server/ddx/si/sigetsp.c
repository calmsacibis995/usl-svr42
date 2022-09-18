/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sigetsp.c	1.4"

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

/*
 * This is a modified siGetSpans routine.  The basic idea is to
 * change the meaning of psrcBase for a DRAWABLE_WINDOW (screen memory).
 * The GetSpans routine grabs the appropriate scan line from the screen,
 * and the proceeds to modify it. as appropriate according to the
 * siGetSpans code.
 */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"

#include "misc.h"
#include "region.h"
#include "gc.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "si.h"
#include "simskbits.h"

#include "sidep.h"

/* GetSpans -- for each span, gets bits from drawable starting at ppt[i]
 * and continuing for pwidth[i] bits
 * Each scanline returned will be server scanline padded, i.e., it will come
 * out to an integral number of words.
 */
void
siGetSpans(pDrawable, wMax, ppt, pwidth, nspans, pdstStart)
    DrawablePtr		pDrawable;	/* drawable from which to get bits */
    int			wMax;		/* largest value of all *pwidths */
    register DDXPointPtr ppt;		/* points to start copying from */
    int			*pwidth;	/* list of number of bits to copy */
    int			nspans;		/* number of scanlines to copy */
    unsigned int	*pdstStart;	/* where to put the bits */ 
{
    register unsigned int	*pdst;		/* where to put the bits */
    register unsigned int	*psrc;		/* where to get the bits */
    register unsigned int	tmpSrc;		/* scratch buffer for bits */
    unsigned int		*psrcBase;	/* start of src bitmap */
    int			widthSrc;	/* width of pixmap in bytes */
    register DDXPointPtr pptLast;	/* one past last point to get */
    int         	xEnd;		/* last pixel to copy from */
    register int	nstart; 
    int	 		nend; 
    int	 		srcStartOver; 
    int	 		startmask, endmask, nlMiddle, nl, srcBit;
    int			w;
    unsigned int	*pdstNext;
    int			isscr = 0;

    PPW = 32 / pDrawable->depth;
    PWSH = si_pix_to_word[PPW];
    PSZ = pDrawable->depth;
    if ((pDrawable->depth == 1) && (pDrawable->type != DRAWABLE_WINDOW)) {
	    mfbGetSpans(pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
	    return;
    }

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	    widthSrc = si_getscanlinelen;
	    isscr++;
#ifndef FLUSH_IN_BH
	    si_Initcache();
#endif
    }
    else
    {
	    psrcBase = (unsigned int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	    widthSrc = (int)(((PixmapPtr)pDrawable)->devKind);
    }

    pdst = pdstStart;
    pptLast = ppt + nspans;
    while(ppt < pptLast)
    {
	xEnd = min(ppt->x + *pwidth, widthSrc << (PWSH-2) );
	w = xEnd - ppt->x;
	srcBit = ppt->x & PIM;
	/* This shouldn't be needed */
	pdstNext = pdst + PixmapWidthInPadUnits(w, pDrawable->depth);

	if (isscr) {
	    psrcBase = (unsigned int *)si_getscanline(ppt->y);
	    psrc = psrcBase + (ppt->x >> PWSH); 
	} else {
	    psrc = psrcBase + (ppt->y * (widthSrc >> 2)) + (ppt->x >> PWSH);
	}

	if (srcBit + w <= PPW) 
	{ 
	    getbits(psrc, srcBit, w, tmpSrc);
/*XXX*/	    putbits(tmpSrc, 0, w, pdst, (unsigned long) -1); 
	    pdst++;
	} 
	else 
	{ 

	    maskbits(ppt->x, w, startmask, endmask, nlMiddle);
	    if (startmask) 
		nstart = PPW - srcBit; 
	    else 
		nstart = 0; 
	    if (endmask) 
		nend = xEnd & PIM; 
	    srcStartOver = srcBit + nstart > PLST;
	    if (startmask) 
	    { 
		getbits(psrc, srcBit, nstart, tmpSrc);
/*XXX*/		putbits(tmpSrc, 0, nstart, pdst, (unsigned long) -1);
		if(srcStartOver)
		    psrc++;
	    } 
	    nl = nlMiddle; 
	    while (nl--) 
	    { 
		tmpSrc = *psrc;
/*XXX*/		putbits(tmpSrc, nstart, PPW, pdst, (unsigned long) -1);
		psrc++;
		pdst++;
	    } 
	    if (endmask) 
	    { 
		getbits(psrc, 0, nend, tmpSrc);
/*XXX*/		putbits(tmpSrc, nstart, nend, pdst, (unsigned long) -1);
		if(nstart + nend >= PPW)
		    pdst++;
	    } 
	    pdst = pdstNext;
	} 
	if (isscr)
	    si_freescanline(psrcBase);
        ppt++;
	pwidth++;
    }
#ifndef FLUSH_IN_BH
    if (isscr)
	si_Flushcache();
#endif
}
