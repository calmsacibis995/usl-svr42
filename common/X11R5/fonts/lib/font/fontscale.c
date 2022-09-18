/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/fontscale.c	1.4"
/*

 * $XConsortium: fontscale.c,v 1.7 91/07/22 23:00:50 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include    "fontfilest.h"

Bool
FontFileAddScaledInstance (entry, vals, pFont, bitmapName)
    FontEntryPtr		entry;
    FontScalablePtr		vals;
    FontPtr			pFont;
    char			*bitmapName;
{
    FontScalableEntryPtr    scalable;
    FontScalableExtraPtr    extra;
    FontScaledPtr	    new;
    int			    newsize;

    scalable = &entry->u.scalable;
    extra = scalable->extra;
    if (extra->numScaled == extra->sizeScaled)
    {
	newsize = extra->sizeScaled + 4;
	new = (FontScaledPtr) xrealloc (extra->scaled,
			    newsize * sizeof (FontScaledRec));
	if (!new)
	    return FALSE;
	extra->sizeScaled = newsize;
	extra->scaled = new;
    }
    new = &extra->scaled[extra->numScaled++];
    new->vals = *vals;
    new->pFont = pFont;
    new->bitmap = (FontEntryPtr) bitmapName;
    if (pFont)
	pFont->fpePrivate = (pointer) entry;
    return TRUE;
}

/* Must call this after the directory is sorted */

FontFileSwitchStringsToBitmapPointers (dir)
    FontDirectoryPtr	dir;
{
    int	    s;
    int	    b;
    int	    i;
    FontEntryPtr	    scalable;
    FontEntryPtr	    nonScalable;
    FontScaledPtr	    scaled;
    FontScalableExtraPtr    extra;
    
    scalable = dir->scalable.entries;
    nonScalable = dir->nonScalable.entries;
    for (s = 0; s < dir->scalable.used; s++)
    {
	extra = scalable[s].u.scalable.extra;
	scaled = extra->scaled;
	for (i = 0; i < extra->numScaled; i++)
	    for (b = 0; b < dir->nonScalable.used; b++)
		if (nonScalable[b].name.name == (char *) scaled[i].bitmap)
		    scaled[i].bitmap = &nonScalable[b];
    }
}

void
FontFileRemoveScaledInstance (entry, pFont)
    FontEntryPtr	entry;
    FontPtr		pFont;
{
    FontScalableEntryPtr    scalable;
    FontScalableExtraPtr    extra;
    int			    i;

    scalable = &entry->u.scalable;
    extra = scalable->extra;
    for (i = 0; i < extra->numScaled; i++)
    {
	if (extra->scaled[i].pFont == pFont)
	{
	    extra->numScaled--;
	    for (; i < extra->numScaled; i++)
		extra->scaled[i] = extra->scaled[i+1];
	}
    }
}

Bool
FontFileCompleteXLFD (vals, def)
    FontScalablePtr	vals;
    FontScalablePtr	def;
{
    int		best;
    /*
     * If two of the three vertical scale values are specified, compute the
     * third.  If all three are specified, make sure they are consistent
     * (within a pixel)
     */

#define Close(a,b,d)  ((b) - (d) <= (a) && (a) <= (b) + (d))

    if (vals->point > 0 && vals->y > 0) {
	best = (vals->point * vals->y * 10 + 7227/2) / 7227;
	if (vals->pixel <= 0)
	    vals->pixel = best;
	else if (!Close(vals->pixel, best, 1))
	    return FALSE;
    } else if (vals->pixel > 0 && vals->y > 0) {
	best = (vals->pixel * 7227 + vals->y*5) / (vals->y * 10);
	if (vals->point <= 0)
	    vals->point = best;
	else if (!Close(vals->point, best, 1))
	    return FALSE;
    } else if (vals->point > 0 && vals->pixel > 0) {
	best = (vals->pixel * 7227 + vals->point*5) / (vals->point * 10);
	if (vals->y <= 0)
	    vals->y = best;
	else if (!Close(vals->y, best, 1))
	    return FALSE;
    } else {
	if (vals->y <= 0)
	    vals->y = def->y;
	if (vals->pixel > 0)
	{
	    if (!vals->y)
		vals->point = 0;
	    else
		vals->point = (vals->pixel * 7227 + vals->y*5) / (vals->y * 10);
	}
	else if (vals->point > 0)
	    vals->pixel = (vals->point * vals->y * 10 + 7227/2) / 7227;
	else {
	    vals->point = def->point;
	    vals->pixel = def->pixel;
	}
    }
    if (vals->x <= 0)
	vals->x = vals->y;

    if (vals->width < 0)
	vals->width = 0;

    if ((vals->pixel >MAXPIXEL)|| (vals->point > MAXPOINT)) return  FALSE;
    return TRUE;
}

static Bool
MatchScalable (a, b)
    FontScalablePtr	a, b;
{
    return  a->x == b->x &&
	    a->y == b->y &&
	    (a->width == b->width || a->width == 0 || b->width == 0) &&
	    (a->pixel == b->pixel || a->point == b->point);
}

static Bool
ExactMatchScalable(a,b)
FontScalablePtr a,b;
{
 return (a->x == b->x &&
	a->y == b->y  &&
  	a->width == b->width	 &&
	a->pixel == b->pixel  &&
	a->point == b->point);

}

#define IsAnamorphic(s)	((s)->pFont && (s)->pFont->info.anamorphic)

FontScaledPtr
FontFileFindScaledInstance (entry, vals, noSpecificSize)
    FontEntryPtr	entry;
    FontScalablePtr	vals;
{
    FontScalableEntryPtr    scalable;
    FontScalableExtraPtr    extra;
    FontScalablePtr	    mvals;
    int			    dist, i;
    int			    mindist, mini;

    scalable = &entry->u.scalable;
    extra = scalable->extra;
    if (noSpecificSize && extra->numScaled)
    {
	mini = 0;
	mindist = extra->scaled[0].vals.point - vals->point;
	if (mindist < 0)
	    mindist = -mindist;
	for (i = 1; i < extra->numScaled; i++)
	{
	    mvals = &extra->scaled[i].vals;
	    if (!IsAnamorphic(&extra->scaled[i]) &&
		mvals->x == vals->x && mvals->y == vals->y)
	    {
		dist = mvals->point - vals->point;
		if (dist < 0)
		    dist = -dist;
		if (dist < mindist)
		{
		    mindist = dist;
		    mini = i;
		}
	    }
	}
	return &extra->scaled[mini];
    }
    else
    {
    	/* See if we've scaled to this value yet */
    	for (i = 0; i < extra->numScaled; i++)
    	{
#ifdef DEBUG
	fprintf(stderr,"matched vals->width=%d scaled.width=%d\n",vals->width,extra->scaled[i].vals.width);
#endif
	    if (ExactMatchScalable (&extra->scaled[i].vals, vals))
			return &extra->scaled[i];
	    if (MatchScalable (&extra->scaled[i].vals, vals) &&
		(vals->width || !IsAnamorphic(&extra->scaled[i])))
	    	return &extra->scaled[i];
    	}
    }
#ifdef DEBUG
	fprintf(stderr, "no scaled match\n");
#endif

    return 0;
}
