/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bitfontlib:bitmap/snffuncs.c	1.8"
/*copyright     "%c%"*/

/*

 * $XConsortium: bitmapfuncs.c,v 1.3 91/06/12 14:35:17 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */
#include   "fontstruct.h"
#include    "fontfilest.h"
#include    "bitmap.h"
#include    "Xrenderer.h"

FontRendererPtr renderer= 0;
RendererConfigPtr rendererInfo=0;



extern int  snfReadFont(), snfReadFontInfo();
int	    snfOpenBitmap ();
int	    snfGetInfoBitmap ();

/*
 * these two arrays must be in the same order
 */
/*
static BitmapFileFunctionsRec readers[] = {
    snfReadFont, snfReadFontInfo
};

*/

int snfFontRendererInit();

int
snfFontRendererInit(prenderer)
FontRendererPtr prenderer;
{

        int ret;

	prenderer->OpenBitmap = snfOpenBitmap;
	prenderer->OpenScalable = 0; /* BitmapOpenScalable */
	prenderer->GetInfoBitmap = snfGetInfoBitmap;
	prenderer->GetInfoScalable = 0; /* BitmapGetInfoScalable */
	prenderer->FreeRenderer = 0;
	prenderer->fonts_open = 0;
        ret = ParseRendererPublic(prenderer);
			/* get font configuration options set */

	return(1);
}


snfOpenBitmap (fpe, ppFont, flags, entry, fileName, format, fmask)
    FontPathElementPtr	fpe;
    FontPtr		*ppFont;
    int			flags;
    FontEntryPtr	entry;
    char		*fileName;
    fsBitmapFormat	format;
    fsBitmapFormatMask	fmask;
{
    FILE       *file;
    FontPtr     pFont;
    int         i;
    int         ret;
    int         bit,
                byte,
                glyph,
                scan,
		image;
     short	callerType;
    /*
     * compute offset into renderers array - same offset is
     * useful in the file functions array
     */
    file = fopen(fileName, "r");
    if (!file)
	return BadFontName;
#ifdef DEBUG
	fprintf(stderr,"snfOpenBitmap file=%s\n",fileName);
#endif
    pFont = (FontPtr) xalloc(sizeof(FontRec));
    if (!pFont) {
	fclose(file);
	return AllocError;
    }
    memset(pFont, 0 , sizeof(FontRec));
    /* set up default values */
    FontDefaultFormat(&bit, &byte, &glyph, &scan);

    /* get any changes made from above */

    ret = CheckFSFormat(format, fmask, &bit, &byte, &scan, &glyph, &image); 

    /* Fill in font record. Data format filled in by reader. */
    pFont->refcnt = 0;
    pFont->maxsiPrivate = MAXSISCREENS;
    pFont->rendererPtr = entry->u.bitmap.renderer;
    pFont->cachestats = 0;
    pFont->maxPrivate = -1;
    pFont->devPrivates = (pointer *) 0;
    pFont->svrPrivate = 0;

    callerType = entry->u.bitmap.renderer->caller;
    ret = snfReadFont (pFont, file, fileName, bit, byte, glyph, scan);

    fclose(file);
    if (ret != Successful) {
	xfree(pFont);
	return ret;
	}

    entry->u.bitmap.renderer->fonts_open = entry->u.bitmap.renderer->fonts_open+1;

    *ppFont = pFont;
    return ret;
}

snfGetInfoBitmap (fpe, pFontInfo, entry, fileName)
    FontPathElementPtr	fpe;
    FontInfoPtr		pFontInfo;
    FontEntryPtr	entry;
    char		*fileName;
{
    FILE    *file;
    int	    i;
    int	    ret;
    FontRendererPtr renderer;

#ifdef DEBUG
	fprintf(stderr,"snfGetInfoBitmap file=%s\n",fileName);
#endif
    file = fopen (fileName, "r");
    if (!file)
	return BadFontName;
    ret = snfReadFontInfo(pFontInfo, file);
    fclose (file);
    return ret;
}

