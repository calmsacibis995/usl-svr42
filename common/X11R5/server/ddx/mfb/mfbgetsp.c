/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/mfb/mfbgetsp.c	1.2"

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
/* $XConsortium: mfbgetsp.c,v 5.5 89/09/14 16:26:46 rws Exp $ */

#include "X.h"
#include "Xmd.h"

#include "misc.h"
#include "region.h"
#include "gc.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"

#include "servermd.h"

/* GetSpans -- for each span, gets bits from drawable starting at ppt[i]
 * and continuing for pwidth[i] bits
 * Each scanline returned will be server scanline padded, i.e., it will come
 * out to an integral number of words.
 */
/*ARGSUSED*/
void
mfbGetSpans(pDrawable, wMax, ppt, pwidth, nspans, pdstStart)
    DrawablePtr		pDrawable;	/* drawable from which to get bits */
    int			wMax;		/* largest value of all *pwidths */
    register DDXPointPtr ppt;		/* points to start copying from */
    int			*pwidth;	/* list of number of bits to copy */
    int			nspans;		/* number of scanlines to copy */
    unsigned int	*pdstStart;	/* where to put the bits */
{
    register unsigned int	*pdst;	/* where to put the bits */
    register unsigned int	*psrc;	/* where to get the bits */
    register unsigned int	tmpSrc;	/* scratch buffer for bits */
    unsigned int		*psrcBase;	/* start of src bitmap */
    int			widthSrc;	/* width of pixmap in bytes */
    register DDXPointPtr pptLast;	/* one past last point to get */
    int         	xEnd;		/* last pixel to copy from */
    register int	nstart; 
    int	 		nend; 
    int	 		srcStartOver; 
    int	 		startmask, endmask, nlMiddle, nl, srcBit;
    int			w;
  
    pptLast = ppt + nspans;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	psrcBase = (unsigned int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthSrc = (int)
		   ((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind;
    }
    else
    {
	psrcBase = (unsigned int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthSrc = (int)(((PixmapPtr)pDrawable)->devKind);
    }
    pdst = pdstStart;

    while(ppt < pptLast)
    {
	xEnd = min(ppt->x + *pwidth, widthSrc << 3);
	pwidth++;
	psrc = psrcBase + (ppt->y * (widthSrc >> 2)) + (ppt->x >> 5); 
	w = xEnd - ppt->x;
	srcBit = ppt->x & 0x1f;

	if (srcBit + w <= 32) 
	{ 
	    getandputbits0(psrc, srcBit, w, pdst);
	    pdst++;
	} 
	else 
	{ 

	    maskbits(ppt->x, w, startmask, endmask, nlMiddle);
	    if (startmask) 
		nstart = 32 - srcBit; 
	    else 
		nstart = 0; 
	    if (endmask) 
		nend = xEnd & 0x1f; 
	    srcStartOver = srcBit + nstart > 31;
	    if (startmask) 
	    { 
		getandputbits0(psrc, srcBit, nstart, pdst);
		if(srcStartOver)
		    psrc++;
	    } 
	    nl = nlMiddle; 
#ifdef FASTPUTBITS
	    Duff(nl, putbits(*psrc, nstart, 32, pdst); psrc++; pdst++;);
#else
	    while (nl--) 
	    { 
		tmpSrc = *psrc;
		putbits(tmpSrc, nstart, 32, pdst);
		psrc++;
		pdst++;
	    } 
#endif
	    if (endmask) 
	    { 
		putbits(*psrc, nstart, nend, pdst);
		if(nstart + nend > 32)
		    pdst++;
	    } 
	    if (startmask || endmask)
		pdst++; 
	} 
        ppt++;
    }
}

