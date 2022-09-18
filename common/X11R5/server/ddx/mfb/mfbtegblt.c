/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/mfb/mfbtegblt.c	1.4"

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

/* $XConsortium: mfbtegblt.c,v 5.5 89/11/21 15:19:41 keith Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include 	"misc.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"mfb.h"
#include	"maskbits.h"

/*
    this works for fonts with glyphs <= 32 bits wide.

    This should be called only with a terminal-emulator font;
this means that the FIXED_METRICS flag is set, and that
glyphbounds == charbounds.

    in theory, this goes faster; even if it doesn't, it reduces the
flicker caused by writing a string over itself with image text (since
the background gets repainted per character instead of per string.)
this seems to be important for some converted X10 applications.

    Image text looks at the bits in the glyph and the fg and bg in the
GC.  it paints a rectangle, as defined in the protocol dcoument,
and the paints the characters.

   to avoid source proliferation, this file is compiled
two times:
	MFBTEGLYPHBLT		OP
	mfbTEGlyphBltWhite		(white text, black bg )
	mfbTEGlyphBltBlack	~	(black text, white bg )

*/

#if defined(NO_3_60_CG4) && defined(FASTPUTBITS) && defined(FASTGETBITS)
#define FASTCHARS
#endif

/*
 * this macro "knows" that only characters <= 8 bits wide will
 * fit this case (which is why it is independent of GLYPHPADBYTES)
 */

#if (BITMAP_BIT_ORDER == MSBFirst) && (GLYPHPADBYTES != 4)
#if GLYPHPADBYTES == 1
#define ShiftAmnt   24
#else
#define ShiftAmnt   16
#endif

#define GetBits4    c = (*char1++ << ShiftAmnt) | \
			SCRRIGHT (*char2++ << ShiftAmnt, xoff2) | \
			SCRRIGHT (*char3++ << ShiftAmnt, xoff3) | \
			SCRRIGHT (*char4++ << ShiftAmnt, xoff4);
#else
#define GetBits4    c = *char1++ | \
			SCRRIGHT (*char2++, xoff2) | \
			SCRRIGHT (*char3++, xoff3) | \
			SCRRIGHT (*char4++, xoff4);
#endif


#if GLYPHPADBYTES == 1
typedef	unsigned char	*glyphPointer;
#define USE_LEFTBITS
#endif

#if GLYPHPADBYTES == 2
typedef unsigned short	*glyphPointer;
#define USE_LEFTBITS
#endif

#if GLYPHPADBYTES == 4
typedef unsigned int	*glyphPointer;
#endif

#ifdef USE_LEFTBITS
#define GetBits1    getleftbits (char1, widthGlyph, c); \
		    c &= glyphMask; \
		    char1 = (glyphPointer) (((char *) char1) + glyphBytes);
#else
#define GetBits1    c = *char1++;
#endif

void
#ifdef NO_MFBOPTIMIZE
MFBTEGLYPHBLT(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
#endif
mfbTEGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase, fgPixel)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
    unsigned long fgPixel;
{
    FontPtr pfont = pGC->font;
    int widthDst;
    unsigned int *pdstBase;	/* pointer to longword with top row 
				   of current glyph */

    int h;			/* height of glyph and char */
    register int xpos;		/* current x  */
    int ypos;			/* current y */
    int widthGlyph;

    int hTmp;			/* counter for height */
    register int startmask, endmask;
    int nfirst;			/* used if glyphs spans a longword boundary */
    BoxRec bbox;		/* for clipping */
    int	widthGlyphs;
    register unsigned int  *dst;
    register unsigned int  c;
    register int	    xoff1, xoff2, xoff3, xoff4;
    register glyphPointer   char1, char2, char3, char4;

#ifdef USE_LEFTBITS
    register int	    glyphMask;
    register unsigned int  tmpSrc;
    register int	    glyphBytes;
#endif

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (unsigned int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	pdstBase = (unsigned int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    xpos = x + pDrawable->x;
    ypos = y + pDrawable->y;

    widthGlyph = FONTMAXBOUNDS(pfont,characterWidth);
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);

    xpos += FONTMAXBOUNDS(pfont,leftSideBearing);
    ypos -= FONTASCENT(pfont);

    bbox.x1 = xpos;
    bbox.x2 = xpos + (widthGlyph * nglyph);
    bbox.y1 = ypos;
    bbox.y2 = ypos + h;

    switch ((*pGC->pScreen->RectIn)(
                ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip, &bbox))
    {
      case rgnPART:
	/* this is the WRONG thing to do, but it works.
	   calling the non-terminal text is easy, but slow, given
	   what we know about the font.

	   the right thing to do is something like:
	    for each clip rectangle
		compute at which row the glyph starts to be in it,
		   and at which row the glyph ceases to be in it
		compute which is the first glyph inside the left
		    edge, and the last one inside the right edge
		draw a fractional first glyph, using only
		    the rows we know are in
		draw all the whole glyphs, using the appropriate rows
		draw any pieces of the last glyph, using the right rows

	   this way, the code would take advantage of knowing that
	   all glyphs are the same height and don't overlap.

	   one day...
	*/
#ifdef NO_MFBOPTIMIZE
	CLIPTETEXT(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
#endif
	mfbImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase, fgPixel);
      case rgnOUT:
	return;
    }
    pdstBase += widthDst * ypos;
    widthGlyphs = widthGlyph << 2;

#ifdef USE_LEFTBITS
    glyphMask = siendtab1[widthGlyph];
    glyphBytes = GLYPHWIDTHBYTESPADDED(pci);
#endif

    if (nglyph >= 4 && widthGlyphs <= 32)
    {
	while (nglyph >= 4)
	{
	    nglyph -= 4;
	    xoff1 = xpos & 0x1f;
	    xoff2 = widthGlyph;
	    xoff3 = xoff2 + widthGlyph;
	    xoff4 = xoff3 + widthGlyph;
	    char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char2 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char3 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char4 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));

	    hTmp = h;
	    dst = pdstBase + (xpos >> 5);

#ifndef FASTCHARS
	    if (xoff1 + widthGlyphs <= 32)
	    {
		maskpartialbits (xoff1, widthGlyphs, startmask);
#endif
		while (hTmp--)
		{
		    GetBits4
#ifdef FASTCHARS
# if BITMAP_BIT_ORDER == MSBFirst
		    c >>= 32 - widthGlyphs;
# endif
		    if (fgPixel)
		    	FASTPUTBITS((c), xoff1, widthGlyphs, dst);
		    else
		    	FASTPUTBITS(~(c), xoff1, widthGlyphs, dst);
#else
		    if (fgPixel)
		       *(dst) = (*dst) & ~startmask | (SCRRIGHT(c, xoff1)) & startmask;
		    else
		       *(dst) = (*dst) & ~startmask | ~(SCRRIGHT(c, xoff1)) & startmask;
#endif
		    dst += widthDst;
		}
#ifndef FASTCHARS
	    }
	    else
	    {
		mask32bits (xoff1, widthGlyphs, startmask, endmask);
		nfirst = 32 - xoff1;
		if (fgPixel) {
			while (hTmp--)
			{
			    GetBits4
			    dst[0] = dst[0] & ~startmask |
				     (SCRRIGHT(c,xoff1)) & startmask;
			    dst[1] = dst[1] & ~endmask |
				     (SCRLEFT(c,nfirst)) & endmask;
			    dst += widthDst;
			}
		}
		else {
			while (hTmp--)
			{
			    GetBits4
			    dst[0] = dst[0] & ~startmask |
				     ~(SCRRIGHT(c,xoff1)) & startmask;
			    dst[1] = dst[1] & ~endmask |
				     ~(SCRLEFT(c,nfirst)) & endmask;
			    dst += widthDst;
			}
		}
	    }
#endif
	    xpos += widthGlyphs;
	}
    }

    while(nglyph--)
    {
	xoff1 = xpos & 0x1f;
	char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	hTmp = h;
	dst = pdstBase + (xpos >> 5);

#ifndef FASTCHARS
	if (xoff1 + widthGlyph <= 32)
	{
	    maskpartialbits (xoff1, widthGlyph, startmask);
#endif
	    while (hTmp--)
	    {
#ifdef FASTCHARS
#ifdef USE_LEFTBITS
		FASTGETBITS (char1,0,widthGlyph,c);
		char1 = (glyphPointer) (((char *) char1) + glyphBytes);
#else
		c = *char1++;
#if BITMAP_BIT_ORDER == MSBFirst
		c >>= 32 - widthGlyph;
#endif
#endif
		if (fgPixel)
			FASTPUTBITS ((c),xoff1,widthGlyph,dst);
		else
			FASTPUTBITS (~(c),xoff1,widthGlyph,dst);
#else
		GetBits1
		if (fgPixel)
		   (*dst) = (*dst) & ~startmask | (SCRRIGHT(c, xoff1)) & startmask;
		else
		   (*dst) = (*dst) & ~startmask | ~(SCRRIGHT(c, xoff1)) & startmask;
#endif
		dst += widthDst;
	    }
#ifndef FASTCHARS
	}
	else
	{
	    mask32bits (xoff1, widthGlyph, startmask, endmask);
	    nfirst = 32 - xoff1;
	    if (fgPixel) {
		    while (hTmp--)
		    {
			GetBits1
			dst[0] = dst[0] & ~startmask |
				 (SCRRIGHT(c,xoff1)) & startmask;
			dst[1] = dst[1] & ~endmask |
				 (SCRLEFT(c,nfirst)) & endmask;
			dst += widthDst;
		    }
	    }
	    else {
		    while (hTmp--)
		    {
			GetBits1
			dst[0] = dst[0] & ~startmask |
				 ~(SCRRIGHT(c,xoff1)) & startmask;
			dst[1] = dst[1] & ~endmask |
				 ~(SCRLEFT(c,nfirst)) & endmask;
			dst += widthDst;
		    }
	    }
	}
#endif
	xpos += widthGlyph;
    }
}
