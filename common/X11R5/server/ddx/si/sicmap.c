/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sicmap.c	1.11"

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
/* $XConsortium: mfbcmap.c,v 5.3 89/07/19 15:48:00 rws Exp $ */

#include "X.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"
/* SI: start */
#include "sidep.h"
#include "Xproto.h"
/* SI: end */

extern int	TellLostMap(), TellGainedMap();
static ColormapPtr InstalledMaps[MAXSCREENS];

void    siUninstallColormap ();
void    siInstallColormap ();
Bool    siInitializeColormap ();
static  Bool    siGetStaticGray ();
static  Bool    siGetStaticColor ();
static  Bool    siGetTrueColor ();
static	void	siFakeStaticMap();
static	unsigned short *siGenerateCmap();
static Bool	siGetStaticMap();

/* SI: end */

int
siListInstalledColormaps(pScreen, pmaps)
    ScreenPtr	pScreen;
    Colormap	*pmaps;
{
    /* By the time we are processing requests, we can guarantee that there
     * is always a colormap installed */
    *pmaps = InstalledMaps[pScreen->myNum]->mid;
    return (1);
}

void
siInstallColormap(pmap)
    ColormapPtr	pmap;
{
    /* SI: start */
    register    long        count;
    register    Entry       *colorptr;
    register    SIColor     *colchgptr;
    register    int         num_colors;
    register    SIColor     *ColorChanges;
    /* SI: end */

    int index = pmap->pScreen->myNum;
    ColormapPtr oldpmap = InstalledMaps[index];
    VisualPtr pVisual;

    if(pmap == oldpmap)
	return;

    /* Uninstall pInstalledMap. No hardware changes required, just
     * notify all interested parties. */
    if(oldpmap != (ColormapPtr)None)
	WalkTree(pmap->pScreen, TellLostMap, (pointer)&oldpmap->mid);

    /* SI: start */
    /* 
     * Generate a  list  of pixel  color  changes and call the  device 
     * dependent routine to change the pixel colors.  Some of these 
     * pixel color changes may be superfluous.  It is up to the device 
     * dependent part to weed these out.
     */

    pVisual = pmap->pVisual;
    num_colors = pVisual->ColormapEntries;
    ColorChanges = (SIColor *)ALLOCATE_LOCAL(sizeof (SIColor) * num_colors);

    if ((pmap->pVisual->class | DynamicClass) == DirectColor) {
	for (count = 0, colchgptr = ColorChanges;
	     count < num_colors; count++, colchgptr++) {

	    colchgptr -> SCpindex = count;
	    colorptr = &pmap->red[(count & pVisual->redMask) >> 
				  pVisual->offsetRed];
	    colchgptr->SCred   = colorptr->co.local.red;
	    colorptr = &pmap->green[(count & pVisual->greenMask) >> 
				  pVisual->offsetGreen];
	    colchgptr->SCgreen = colorptr->co.local.green;
	    colorptr = &pmap->blue[(count & pVisual->blueMask) >> 
				  pVisual->offsetBlue];
	    colchgptr->SCblue  = colorptr->co.local.blue;
	}
    }
    else {
	for (count = 0, colchgptr = ColorChanges, colorptr = pmap->red;
	     count < num_colors; count++, colchgptr++, colorptr++) {

	    colchgptr -> SCpindex = count;
	    if (colorptr->fShared) {
		colchgptr->SCred   = colorptr->co.shco.red->color;
		colchgptr->SCgreen = colorptr->co.shco.green->color;
		colchgptr->SCblue  = colorptr->co.shco.blue->color;
	    }
	    else {
		colchgptr->SCred   = colorptr->co.local.red;
		colchgptr->SCgreen = colorptr->co.local.green;
		colchgptr->SCblue  = colorptr->co.local.blue;
	    }
	}
    }

/*
 * Scott:  FIX THIS:  multiple visual/colormap
 */
    /* if (pmap->pVisual->class & DynamicClass) */
	si_setcolormap(0, 0, ColorChanges, num_colors);
    DEALLOCATE_LOCAL(ColorChanges);
    /* SI: end */

    /* Install pmap */
    InstalledMaps[index] = pmap;
    WalkTree(pmap->pScreen, TellGainedMap, (pointer)&pmap->mid);
}

#ifdef OLD
void
siUninstallColormap(pmap)
    ColormapPtr	pmap;
{
    /* SI: start */
    register	ScreenPtr   pscreen;
    register	ColormapPtr newpmap;

    pscreen = pmap -> pScreen;

    if (pmap != InstalledMaps[pscreen -> myNum])
	return;
    newpmap = (ColormapPtr) LookupIDByType (pscreen -> defColormap, RT_COLORMAP );
    if (newpmap == 0) {
	ErrorF ("Can't find default Colormap\n");
	return;
    }

    if (pmap != newpmap)
	siInstallColormap (newpmap);
    /* SI: end */
}
#else /* OLD */
void
siUninstallColormap(pmap)
    ColormapPtr	pmap;
{
    int index = pmap->pScreen->myNum;
    ColormapPtr curpmap = InstalledMaps[index];

    if(pmap == curpmap)
    {
	if (pmap->mid != pmap->pScreen->defColormap)
	{
	    curpmap = (ColormapPtr) LookupIDByType(pmap->pScreen->defColormap,
						   RT_COLORMAP);
	    (*pmap->pScreen->InstallColormap)(curpmap);
	}
    }
}
#endif /* OLD */

void
siResolveColor(pred, pgreen, pblue, pVisual)
    unsigned short	*pred, *pgreen, *pblue;
    register VisualPtr	pVisual;
{
    int shift = 16 - pVisual->bitsPerRGBValue;
    unsigned lim = (1 << pVisual->bitsPerRGBValue) - 1;

    if ((pVisual->class == PseudoColor) || (pVisual->class == DirectColor))
    {
	/* rescale to rgb bits */
	*pred = ((*pred >> shift) * 65535) / lim;
	*pgreen = ((*pgreen >> shift) * 65535) / lim;
	*pblue = ((*pblue >> shift) * 65535) / lim;
    }
    else if (pVisual->class == GrayScale)
    {
	/* rescale to gray then rgb bits */
	*pred = (unsigned short)((30L * *pred + 59L * *pgreen + 11L * *pblue) / 100);
	*pblue = *pgreen = *pred = ((*pred >> shift) * 65535) / lim;
    }
}

void
siStoreColors (pmap, num_pixels, pdefs)
ColormapPtr     pmap;
int     num_pixels;
xColorItem      *pdefs;
{
    register    long        count;
    register    SIColor     *colchgptr;
    register    SIColor     *ColorChanges;

    /*
     * If we're not using a visual that's dynamic, there's nothing to do
     */
    if (!(pmap->pVisual->class & DynamicClass)) 
	return;

    /* If we are not currently running with this color  map,  ignore */
    /* the  Changes.   This  means  that we cannot currently support */
    /* multiple colormaps in hardware.                               */

    if (pmap != InstalledMaps[pmap -> pScreen -> myNum])
	return;

    /* 
     * Generate a list of pixel color changes, and then call the
     * device dependent routine to change the pixel colors.      
     */

    ColorChanges = (SIColor *)ALLOCATE_LOCAL(sizeof (SIColor) * num_pixels);

    for (count = 0, colchgptr = ColorChanges;
	 count < num_pixels; count++, colchgptr++) {

	colchgptr->SCpindex = pdefs->pixel;
	colchgptr->SCred   = pdefs->red;
	colchgptr->SCgreen = pdefs->green;
	colchgptr->SCblue  = pdefs->blue;
    }

/*
 * Scott:  FIX THIS:  multiple visual/colormap
 */
    si_setcolormap(0, 0, ColorChanges, num_pixels);
    DEALLOCATE_LOCAL(ColorChanges);
}

Bool
siInitializeColormap (pmap)
register    ColormapPtr pmap;
{
	int index = pmap->pScreen->myNum;
	ColormapPtr oldpmap = InstalledMaps[index];
	SIVisualP pVisual;
	unsigned short red, green, blue;
	Bool status = TRUE;

	switch (pmap->pVisual->class) {
	    case StaticGray:
	    case StaticColor:
		/*
		 * If the SDD's real visual is dynamic, load it
		 * with a colormap and return the loaded map.
		 */
		pVisual = si_GetInfoVal(SIvisuals);
		if ((pVisual->SVtype == PSEUDOCOLOR_AVAIL) ||
		    (pVisual->SVtype == STATICCOLOR_AVAIL) ||
		    (pVisual->SVtype == GRAYSCALE_AVAIL)) 
			siFakeStaticMap(pmap);
		else
			siGetStaticMap (pmap);
		break;

   	    case PseudoColor:
		    if (pmap->flags & IsDefault) {
		      red = green = blue = 0;
		      if (AllocColor (pmap, &red, &green, &blue,
				      &(pmap->pScreen->blackPixel), 0)
			  != Success)
			status = FALSE;
		      red = green = blue = ~0;
		      if (AllocColor (pmap, &red, &green, &blue,
				      &(pmap->pScreen->whitePixel), 0)
			  != Success)
			status = FALSE;
		    }
		    break;

	    case TrueColor:
		siGetTrueColor (pmap);
		break;
	}
	return status;
}

static Bool
siGetInstalledMap(pmap)
ColormapPtr pmap;
{
	int index = pmap->pScreen->myNum;
	ColormapPtr oldpmap = InstalledMaps[index];
	register    Entry       *colorptr1, *colorptr2;
	register    int         num_colors;

	num_colors = pmap->pVisual->ColormapEntries;

	colorptr1 = pmap->red;
	colorptr2 = oldpmap->red;
	while (num_colors--) {
		colorptr1->co.local.red   = colorptr2->co.local.red;
		colorptr1->co.local.green = colorptr2->co.local.green;
		colorptr1->co.local.blue  = colorptr2->co.local.blue;
	}

	return (SI_SUCCEED);
}

void
siFakeStaticMap(pmap)
register    ColormapPtr pmap;
{
	extern siCmap si_colormap;
	int index = pmap->pScreen->myNum;
	ColormapPtr oldpmap = InstalledMaps[index];
	Entry       *colorptr;
	SIColor     *colchgptr;
	SIColor     *ColorChanges;
	SIVisualP pVisual;
	int i, j;
	unsigned short *pshort;

	/*
	 * If we don't already have a colormap of the right size set up, 
	 * we need to generate one.
	 */
	if (si_colormap.sz != pmap->pVisual->ColormapEntries) {
		si_colormap.sz = pmap->pVisual->ColormapEntries;
		if (si_colormap.colors)
			Xfree(si_colormap.colors);
		si_colormap.colors = siGenerateCmap(pmap->pVisual);
		if (!si_colormap.colors)
			return;
	}

	ColorChanges=(SIColor *)ALLOCATE_LOCAL(sizeof (SIColor)*si_colormap.sz);
	colorptr = pmap->red;
	colchgptr = ColorChanges;
	pshort = si_colormap.colors;
	for (i = 0; i < si_colormap.sz; i++) {
		colchgptr->SCpindex = i;
		colorptr->co.local.red   = colchgptr->SCred   = *pshort++;
		colorptr->co.local.green = colchgptr->SCgreen = *pshort++;
		colorptr->co.local.blue  = colchgptr->SCblue  = *pshort++;
		colorptr++;
		colchgptr++;
	}

	si_setcolormap(0, 0, ColorChanges, si_colormap.sz);
	DEALLOCATE_LOCAL(ColorChanges);
}

unsigned short *
siGenerateCmap(visual)
VisualRec *visual;
{
	unsigned short *pshort, *cmap;
	int total_bits, bits_red, bits_green, bits_blue;
	unsigned lim, maxent, shift, i;
	unsigned rmask, gmask, bmask, roff, goff, boff, limr, limg, limb;

	lim = (1 << visual->bitsPerRGBValue) - 1;
	shift = 16 - visual->bitsPerRGBValue;
	maxent = visual->ColormapEntries - 1;

	if (!(cmap = (unsigned short *)Xalloc((maxent+1) * sizeof(short) * 3)))
		return (cmap);
	pshort = cmap;

	/*
	 * if there are only two colors, default them to black and white
	 */
	if (visual->ColormapEntries == 2) {
		*pshort++ = 0;
		*pshort++ = 0;
		*pshort++ = 0;
		*pshort++ = 0xffff;
		*pshort++ = 0xffff;
		*pshort = 0xffff;

		return (cmap);
	}
	
	switch(visual->class) {
	case StaticGray:
		roff = goff = boff = 0;
		rmask = gmask = bmask = maxent;
		limr = limg = limb = maxent;
		break;
	case StaticColor:
		total_bits = visual->nplanes;
		bits_blue  = total_bits / 3;
		bits_green = (total_bits - bits_blue) / 2;
		bits_red   = total_bits - (bits_blue + bits_green);

		roff = bits_green + bits_blue;
		goff = bits_blue;
		boff = 0;
		limr = (1 << bits_red) - 1;
		limg = (1 << bits_green) - 1;
		limb = (1 << bits_blue) - 1;
		rmask = limr << (bits_green + bits_blue);
		gmask = limg << bits_blue;
		bmask = limb;
		
		break;
	}

	for(i = 0; i <= maxent; i++) {
		/* rescale to [0..65535] then rgb bits */
		*pshort++ = ((((((i & rmask) >> roff) * 65535) / limr) >> 
				shift) * 65535) / lim;
		*pshort++ = ((((((i & gmask) >> goff) * 65535) / limg) >> 
				shift) * 65535) / lim;
		*pshort++ = ((((((i & bmask) >> boff) * 65535) / limb) >> 
				shift) * 65535) / lim;
	}

	return(cmap);
}
	

static Bool
siGetStaticMap (pmap)
ColormapPtr pmap;
{
	register    long        count;
	register    Entry       *colorptr;
	register    SIColor     *colchgptr;
	register    int         num_colors;
	register    SIColor     *ColorChanges;

	num_colors = pmap->pVisual->ColormapEntries;
	ColorChanges = (SIColor *)ALLOCATE_LOCAL(sizeof (SIColor) * num_colors);

	for (count = 0, colchgptr = ColorChanges; count < num_colors;
	  count++, colchgptr++) {

	    colchgptr -> SCpindex = count;
	    colchgptr -> SCred = 0;
	    colchgptr -> SCgreen = 0;
	    colchgptr -> SCblue = 0;
	}

	if (si_getcolormap(0, 0, ColorChanges, num_colors) != SI_SUCCEED) {
	    DEALLOCATE_LOCAL (ColorChanges);
	    return (SI_FAIL);
	}

	for (count = 0, colchgptr = ColorChanges, colorptr = pmap->red;
	  count < num_colors; count++, colchgptr++, colorptr++) {
	    colorptr -> co.local.red = colchgptr -> SCred;
	    colorptr -> co.local.green = colchgptr -> SCgreen;
	    colorptr -> co.local.blue = colchgptr -> SCblue;
	}

	DEALLOCATE_LOCAL (ColorChanges);
	return (SI_SUCCEED);
}


static Bool
siGetTrueColor (pmap)
ColormapPtr pmap;
{
	register    long        count;
	register    Entry       *colorptr;
	register    SIColor     *colchgptr;
	register    VisualRec   *visual;
	register    int         num_reds;
	register    int         num_greens;
	register    int         num_blues;
	register    SIColor     *ColorChanges;

	/* Read in the TrueColor colormap. */

	visual = pmap -> pVisual;

	/* Compute the number of  reds,  greens,  and  blues.   This */
	/* presumes  that the masks in the visual are contiguous and */
	/* begin  at  the  offsets  in  the  visual.    This   seems */
	/* reasonable,  but might have to be changed is some strange */
	/* hardware comes along.                                     */

	num_reds = (visual -> redMask >> visual -> offsetRed) + 1;
	num_greens = (visual -> greenMask >> visual -> offsetGreen) + 1;
	num_blues = (visual -> blueMask >> visual -> offsetBlue) + 1;

	ColorChanges = (SIColor *) ALLOCATE_LOCAL (sizeof (SIColor) *
	  (num_reds + num_greens + num_blues));

	colchgptr = ColorChanges;

	for (count = 0; count < num_reds;
	  count++, colchgptr++) {
	    colchgptr -> SCpindex = count;
	    colchgptr -> SCred = 0;
	    colchgptr -> SCgreen = 0;
	    colchgptr -> SCblue = 0;
	}

	for (count = 0; count < num_greens;
	  count++, colchgptr++) {
	    colchgptr -> SCpindex = count + num_reds;
	    colchgptr -> SCred = 0;
	    colchgptr -> SCgreen = 0;
	    colchgptr -> SCblue = 0;
	}

	for (count = 0; count < num_blues;
	  count++, colchgptr++) {
	    colchgptr -> SCpindex = count + num_reds + num_greens;
	    colchgptr -> SCred = 0;
	    colchgptr -> SCgreen = 0;
	    colchgptr -> SCblue = 0;
	}

	if (si_getcolormap(0, 0, ColorChanges, num_blues) != SI_SUCCEED) {
	    DEALLOCATE_LOCAL (ColorChanges);
	    return (SI_FAIL);
	}

	colchgptr = ColorChanges;

	for (count = 0, colorptr = pmap->red; count < num_reds;
	  count++, colorptr++, colchgptr++) {
	    colorptr -> co.local.red = colchgptr -> SCred;
	    colorptr -> co.local.green = 0;
	    colorptr -> co.local.blue = 0;
	}

	for (count = 0, colorptr = pmap->green; count < num_greens;
	  count++, colorptr++, colchgptr++) {
	    colorptr -> co.local.red = colchgptr -> SCgreen;
	    colorptr -> co.local.green = 0;
	    colorptr -> co.local.blue = 0;
	}

	for (count = 0, colorptr = pmap->blue; count < num_blues;
	  count++, colorptr++, colchgptr++) {
	    colorptr -> co.local.red = colchgptr -> SCblue;
	    colorptr -> co.local.green = 0;
	    colorptr -> co.local.blue = 0;
	}

	DEALLOCATE_LOCAL (ColorChanges);
	return (SI_SUCCEED);
}
/* SI: end */

Bool
siCreateDefColormap(pScreen)
    ScreenPtr pScreen;
{
    unsigned short	zero = 0, ones = (unsigned short)~0; 
    VisualPtr	pVisual;
    ColormapPtr	cmap;
    
    for (pVisual = pScreen->visuals;
	 pVisual->vid != pScreen->rootVisual;
	 pVisual++)
	;

    if (CreateColormap(pScreen->defColormap, pScreen, pVisual, &cmap,
		       (pVisual->class & DynamicClass) ? AllocNone : AllocAll,
		       0)
	!= Success)
	return FALSE;
    if ((AllocColor(cmap, &ones, &ones, &ones, &(pScreen->whitePixel), 0) !=
       	   Success) ||
    	(AllocColor(cmap, &zero, &zero, &zero, &(pScreen->blackPixel), 0) !=
       	   Success))
    	return FALSE;
    (*pScreen->InstallColormap)(cmap);
    return TRUE;
}

