/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bitfontlib:bitmap/bitmap.c	1.11"
/*copyright     "%c%"*/

/*

 * $XConsortium: bitmap.c,v 1.3 91/05/30 19:06:55 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include	<stdio.h>
#include    "fontfilest.h"
#include    "bitmap.h"

int         bitmapGetGlyphs(), bitmapGetMetrics();
#ifdef FONTSERVER
int         bitmapGetBitmaps(), bitmapGetExtents();
#endif
#ifdef BITSOURCE
void	    bitmapComputeFontBounds ();
void	    bitmapComputeFontInkBounds ();
#endif

int
bitmapGetGlyphs(pFont, count, chars, charEncoding, glyphCount, glyphs, flag, glist)

    FontPtr    pFont;
    unsigned long count;
    register unsigned char *chars;
    FontEncoding charEncoding;
    unsigned long *glyphCount;	/* RETURN */
    CharInfoPtr *glyphs;	/* RETURN */
    int flag;
    INT16 glist[];		/* RETURN */
{
    BitmapFontPtr  bitmapFont;
    unsigned int firstCol;
    register unsigned int numCols;
    unsigned int firstRow;
    unsigned int cDef;
    unsigned int numRows;
    CharInfoPtr *glyphsBase;
    register unsigned int c;
    register unsigned long i;
    unsigned long n;
    register CharInfoPtr pci;
    unsigned int row,col;
    CharInfoPtr *encoding;
    CharInfoPtr pDefault;
    int ret;
    FILE *file;
    int rerender = 0;

    n = i =0;
    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
#ifdef DEBUG 
	fprintf(stderr,"GetGlyphs fonts=%s\n",bitmapFont->filename);
#endif
#ifdef DEBUG1
	fprintf(stderr,"pFont->rendererPtr->config.prerender=%d pFont->info.rerender=%d\n",pFont->rendererPtr->config.prerender, pFont->info.rerender);
#endif

    if ((pFont->info.rerender == TRUE) || (pFont->rendererPtr->config.prerender != 1)) 
	 rerender = 1;
    if (rerender) {
	file = (FILE *)reopen_fontfile(pFont);
	if (!file)  {
		fprintf(stderr,"Error Reopening Fontfile: %s\n",bitmapFont->filename);
		return BadFontName;
	}
    	ret = recreate_glyphs(file,pFont, chars, count, charEncoding);
		/* make sure all the glyphs we need are allocated and cached */
    	if (ret == -1) {
		fprintf(stderr,"Error Recreating Glyphs: fontfile=%s\n",bitmapFont->filename);
		FreePerFontCache(pFont);
		snfFreeGlyphs(pFont);
		fclose(file);
		return BadFontName;
	}	
    }
    encoding = bitmapFont->encoding;
    pDefault = bitmapFont->pDefault;
    firstCol = pFont->info.firstCol;
    cDef = pFont->info.defaultCh - firstCol;
    numCols = pFont->info.lastCol - firstCol + 1;
    glyphsBase = glyphs;
    switch (charEncoding) {

    case Linear8Bit:
    case TwoD8Bit:
#ifdef DEBUG1
	fprintf(stderr,"Linear8Bit for file=%s\n",bitmapFont->filename);
#endif
	if (pFont->info.firstRow > 0)
	    break;
	if (pFont->info.allExist && pDefault) {
	    while (count--) {
		c = (*chars++) - firstCol;
		if (c < numCols) {
		    if (flag) glist[i++] = c;
		    *glyphs++ = encoding[c];
		}
		else {
   		    if (flag) glist[i++] = cDef;
		    *glyphs++ = pDefault;
		}
	    }
	} else {
	    while (count--) {
		c = (*chars++) - firstCol;
		if (c < numCols && (pci = encoding[c])) {
		    *glyphs++ = pci;
		    if (flag) glist[i++] = c;
		}
		else if (pDefault) {
   		    if (flag) glist[i++] = cDef;
		    *glyphs++ = pDefault;
   		}
		}
	}
	break;
    case Linear16Bit:
#ifdef DEBUG1
	fprintf(stderr,"Linear16Bit for file=%s\n",bitmapFont->filename);
#endif
	if (pFont->info.allExist && pDefault) {
	    while (count--) {
		c = *chars++ << 8;
		c = (c | *chars++) - firstCol;
		if (c < numCols) {
  
		    *glyphs++ = encoding[c];
		    if (flag) glist[i++] = c;
		    }
		else
		    {
   		    if (flag) glist[i++] = cDef;
		    *glyphs++ = pDefault;
		    }
	    }
	} else {
	    while (count--) {
		c = *chars++ << 8;
		c = (c | *chars++) - firstCol;
		if (c < numCols && (pci = encoding[c])) {
		    *glyphs++ = pci;
		    if (flag) glist[i++] = c;
		    }
		else if (pDefault) {
		    *glyphs++ = pDefault;
   		    if (flag) glist[i++] = cDef;
		    }
	    }
	}
	break;

    case TwoD16Bit:
#ifdef DEBUG1
	fprintf(stderr,"TwoD16Bit for file=%s\n",bitmapFont->filename);
#endif
	firstRow = pFont->info.firstRow;
	numRows = pFont->info.lastRow - firstRow + 1;
	while (count--) {
	    row = (*chars++) - firstRow;
	    col = (*chars++) - firstCol;
	    c = row*numCols + col;	
	    if (row < numRows && col < numCols &&
		    (pci = encoding[c])) {
		   *glyphs++ = pci;
		    if (flag) glist[i++] = c;
		}
	    else if (pDefault) {
		if (flag) glist[i++] = cDef;
		*glyphs++ = pDefault;
		}
	}
	break;
    }
    *glyphCount = glyphs - glyphsBase;
    if (rerender) fclose(file);
    return Successful;
}

static CharInfoRec nonExistantChar;

int
bitmapGetMetrics(pFont, count, chars, charEncoding, glyphCount, glyphs )
    FontPtr     pFont;
    unsigned long count;
    register unsigned char *chars;
    FontEncoding charEncoding;
    unsigned long *glyphCount;	/* RETURN */
    xCharInfo **glyphs;		/* RETURN */
{
    int         ret;
    xCharInfo  *ink_metrics;
    CharInfoPtr metrics;
    BitmapFontPtr  bitmapFont;
    CharInfoPtr	oldDefault;
    int         i;

    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
    oldDefault = bitmapFont->pDefault;
    bitmapFont->pDefault = &nonExistantChar;
    ret = bitmapGetGlyphs(pFont, count, chars, charEncoding, glyphCount, (CharInfoPtr *) glyphs, 0, 0);
    if (ret == Successful) {
	if (bitmapFont->ink_metrics) {
	    metrics = bitmapFont->metrics;
	    ink_metrics = bitmapFont->ink_metrics;
	    for (i = 0; i < *glyphCount; i++) {
		if (glyphs[i] != (xCharInfo *) & nonExistantChar)
		    glyphs[i] = ink_metrics + (((CharInfoPtr) glyphs[i]) - metrics);
	    }
	}
    }
    bitmapFont->pDefault = oldDefault;
    return ret;
}

