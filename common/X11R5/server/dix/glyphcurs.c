/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:dix/glyphcurs.c	1.4"
/*copyright	"%c%*/
/************************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

************************************************************************/

/* $XConsortium: glyphcurs.c,v 5.7 91/06/26 14:04:24 rws Exp $ */

#include <stdio.h>
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "misc.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "resource.h"
#include "dix.h"
#include "cursorstr.h"
#include "opaque.h"
#include "servermd.h"


/*
    get the bits out of the font in a portable way.  to avoid
dealing with padding and such-like, we draw the glyph into
a bitmap, then read the bits out with GetImage, which
uses server-natural format.
    since all screens return the same bitmap format, we'll just use
the first one we find.
    the character origin lines up with the hotspot in the
cursor metrics.
*/

int
ServerBitsFromGlyph(pfont, ch, cm, ppbits)
    FontPtr	pfont;
    unsigned short ch;
    register CursorMetricPtr cm;
    unsigned char **ppbits;
{
    register ScreenPtr pScreen;
    register GCPtr pGC;
    xRectangle rect;
    PixmapPtr ppix;
    long nby;
    unsigned char *pbits;
    XID gcval[3];
    unsigned char char2b[2];

    /* turn glyph index into a protocol-format char2b */
    char2b[0] = (unsigned char)(ch >> 8);
    char2b[1] = (unsigned char)(ch & 0xff);

    pScreen = screenInfo.screens[0];
    nby = PixmapBytePad(cm->width, 1) * (long)cm->height;
    pbits = (unsigned char *)xalloc(nby);
    if (!pbits)
	return BadAlloc;
    /* zeroing the (pad) bits seems to help some ddx cursor handling */
    bzero((char *)pbits, nby);

    ppix = (PixmapPtr)(*pScreen->CreatePixmap)(pScreen, cm->width,
					       cm->height, 1);
    pGC = GetScratchGC(1, pScreen);
    if (!ppix || !pGC)
    {
	if (ppix)
	    (*pScreen->DestroyPixmap)(ppix);
	if (pGC)
	    FreeScratchGC(pGC);
	xfree(pbits);
	return BadAlloc;
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = cm->width;
    rect.height = cm->height;

    /* fill the pixmap with 0 */
    gcval[0] = GXcopy;
    gcval[1] = 0;
    gcval[2] = (XID)pfont;
    DoChangeGC(pGC, GCFunction | GCForeground | GCFont, gcval, 1);
    ValidateGC((DrawablePtr)ppix, pGC);
    (*pGC->ops->PolyFillRect)(ppix, pGC, 1, &rect);

    /* draw the glyph */
    gcval[0] = 1;
    DoChangeGC(pGC, GCForeground, gcval, 0);
    ValidateGC((DrawablePtr)ppix, pGC);
    (*pGC->ops->PolyText16)(ppix, pGC, cm->xhot, cm->yhot, 1, char2b);
    (*pScreen->GetImage)(ppix, 0, 0, cm->width, cm->height,
			 ZPixmap, 1, pbits);
    *ppbits = pbits;
    FreeScratchGC(pGC);
    (*pScreen->DestroyPixmap)(ppix);
    return Success;
}


Bool
CursorMetricsFromGlyph( pfont, ch, cm)
    register FontPtr 	pfont;
    unsigned		ch;
    register CursorMetricPtr cm;
{
    CharInfoPtr 	pci;
    /*int			nglyphs;*/
	unsigned long nglyphs;
    CARD8		chs[2];
    FontEncoding	encoding;
    INT16 		*glist;
    int			ret;

    chs[0] = ch >> 8;
    chs[1] = ch;
    pfont->info.freeWhen = FONT_DONT_FREE;
		/* set so cache will not free the cursor */
#ifdef DEBUG
	fprintf(stderr,"CursorMetricsFromGlyph \n");
#endif
    encoding = (FONTLASTROW(pfont) == 0) ? Linear16Bit : TwoD16Bit;
    if (encoding == Linear16Bit)
    {

	if (ch < pfont->info.firstCol || pfont->info.lastCol < ch)
	    return FALSE;
    }
    else
    {
	if (chs[0] < pfont->info.firstRow || pfont->info.lastRow < chs[0])
	    return FALSE;
	if (chs[1] < pfont->info.firstCol || pfont->info.lastCol < chs[1])
	    return FALSE;
    }
    ret = (*pfont->get_glyphs) (pfont, 1, chs, encoding, &nglyphs, &pci, 1,&glist);
    if (ret != Successful) return FALSE;
    if (nglyphs == 0)
	return FALSE;
    cm->width = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
    cm->height = pci->metrics.descent + pci->metrics.ascent;
    if (pci->metrics.leftSideBearing > 0)
    {
	cm->width += pci->metrics.leftSideBearing;
	cm->xhot = 0;
    }
    else
    {
	cm->xhot = -pci->metrics.leftSideBearing;
	if (pci->metrics.rightSideBearing < 0)
	    cm->width -= pci->metrics.rightSideBearing;
    }
    if (pci->metrics.ascent < 0)
    {
	cm->height -= pci->metrics.ascent;
	cm->yhot = 0;
    }
    else
    {
	cm->yhot = pci->metrics.ascent;
	if (pci->metrics.descent < 0)
	    cm->height -= pci->metrics.descent;
    }
    return TRUE;
}
