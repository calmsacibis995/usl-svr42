/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/bitsource.c	1.1"
/* $NCDId: @(#)bitsource.c,v 1.4 1991/07/02 17:00:59 lemke Exp $ */

/*
 * $XConsortium: bitsource.c,v 1.5 91/07/17 14:29:54 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include    "fontfilest.h"

BitmapSourcesRec	FontFileBitmapSources;

Bool
FontFileRegisterBitmapSource (fpe)
    FontPathElementPtr	fpe;
{
    FontPathElementPtr	*new;
    int			i;
    int			newsize;

    for (i = 0; i < FontFileBitmapSources.count; i++)
	if (FontFileBitmapSources.fpe[i] == fpe)
	    return TRUE;
    if (FontFileBitmapSources.count == FontFileBitmapSources.size)
    {
	newsize = FontFileBitmapSources.size + 4;
	new = (FontPathElementPtr *) xrealloc (FontFileBitmapSources.fpe, newsize * sizeof *new);
	if (!new)
	    return FALSE;
	FontFileBitmapSources.size = newsize;
	FontFileBitmapSources.fpe = new;
    }
    FontFileBitmapSources.fpe[FontFileBitmapSources.count++] = fpe;
    return TRUE;
}

void
FontFileUnregisterBitmapSource (fpe)
    FontPathElementPtr	fpe;
{
    int	    i;

    for (i = 0; i < FontFileBitmapSources.count; i++)
	if (FontFileBitmapSources.fpe[i] == fpe)
	{
	    FontFileBitmapSources.count--;
	    if (FontFileBitmapSources.count == 0)
	    {
		FontFileBitmapSources.size = 0;
		xfree (FontFileBitmapSources.fpe);
		FontFileBitmapSources.fpe = 0;
	    }
	    else
	    {
	    	for (; i < FontFileBitmapSources.count; i++)
		    FontFileBitmapSources.fpe[i] = FontFileBitmapSources.fpe[i+1];
	    }
	    break;
	}
}

FontFileMatchBitmapSource (fpe, pFont, flags, entry, zeroPat, vals, format, fmask, noSpecificSize)
    FontPathElementPtr	fpe;
    FontPtr		*pFont;
    int			flags;
    FontEntryPtr	entry;
    FontNamePtr		zeroPat;
    FontScalablePtr	vals;
    fsBitmapFormat	format;
    fsBitmapFormatMask	fmask;
    Bool		noSpecificSize;
{
    int			source;
    FontEntryPtr	zero;
    FontBitmapEntryPtr	bitmap;
    int			ret;
    FontDirectoryPtr	dir;
    FontScaledPtr	scaled;

    /*
     * Look through all the registered bitmap sources for
     * the same zero name as ours; entries along that one
     * can be scaled as desired.
     */
    ret = BadFontName;
    for (source = 0; source < FontFileBitmapSources.count; source++)
    {
    	if (FontFileBitmapSources.fpe[source] == fpe)
	    continue;
	dir = (FontDirectoryPtr) FontFileBitmapSources.fpe[source]->private;
	zero = FontFileFindNameInDir (&dir->scalable, zeroPat);
	if (!zero)
	    continue;
    	scaled = FontFileFindScaledInstance (zero, vals, noSpecificSize);
    	if (scaled)
    	{
	    if (scaled->pFont)
	    {
		*pFont = scaled->pFont;
		ret = Successful;
	    }
	    else if (scaled->bitmap)
	    {
		entry = scaled->bitmap;
		bitmap = &entry->u.bitmap;
		if (bitmap->pFont)
		{
		    *pFont = bitmap->pFont;
		    ret = Successful;
		}
		else
		{
		    ret = FontFileOpenBitmap (
				FontFileBitmapSources.fpe[source],
				pFont, flags, entry, format, fmask);
		}
	    }
	    else /* "cannot" happen */
	    {
		ret = BadFontName;
	    }
	    break;
    	}
    }
    return ret;
}
