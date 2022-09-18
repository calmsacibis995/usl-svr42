/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/fontfile.c	1.7"
/*copyright     "%c%"*/

/*

 * $XConsortium: fontfile.c,v 1.9 91/07/18 22:37:29 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */
/* $NCDId: @(#)fontfile.c,v 1.6 1991/07/02 17:00:46 lemke Exp $ */

#include    "fontfilest.h"
#include 	<stdio.h>

/*
 * Map FPE functions to renderer functions
 */

extern FontRenderersRec renderers;


CountDashes (name, namelen)
    char    *name;
    int	    namelen;
{
    int	ndashes = 0;

    while (namelen--)
	if (*name++ == '\055')	/* avoid non ascii systems */
	    ++ndashes;
    return ndashes;
}


int
FontFileNameCheck (name)
    char    *name;
{
#ifndef NCD
    return *name == '/';
#else
    return ((strcmp(name, "built-ins") == 0) || (*name == '/'));
#endif
}

int
FontFileInitFPE (fpe)
    FontPathElementPtr	fpe;
{
    int			status;
    FontDirectoryPtr	dir;

    status = FontFileReadDirectory (fpe->name, &dir);
    if (status == Successful)
    {
	if (dir->nonScalable.used > 0)
	    if (!FontFileRegisterBitmapSource (fpe))
	    {
		FontFileFreeFPE (fpe);
		return AllocError;
	    }
	fpe->private = (pointer) dir;
    }
    return status;
}

/* ARGSUSED */
int
FontFileResetFPE (fpe)
    FontPathElementPtr	fpe;
{
    FontDirectoryPtr	dir;

    dir = (FontDirectoryPtr) fpe->private;
    if (FontFileDirectoryChanged (dir))
    {
	/* can't do it, so tell the caller to close and re-open */
	return FPEResetFailed;	
    }
    return Successful;
}

int
FontFileFreeFPE (fpe)
    FontPathElementPtr	fpe;
{
    FontFileUnregisterBitmapSource (fpe);
    FontFileFreeDir ((FontDirectoryPtr) fpe->private);
    return Successful;
}

/* ARGSUSED */
int
FontFileOpenFont (client, fpe, flags, name, namelen, format, fmask,
		  id, pFont, aliasName)
    pointer		client;
    FontPathElementPtr	fpe;
    int			flags;
    char		*name;
    int			namelen;
    fsBitmapFormat	format;
    fsBitmapFormatMask	fmask;
    XID			id;
    FontPtr		*pFont;
    char		**aliasName;
{
    FontDirectoryPtr	dir;
    char		lowerName[MAXFONTNAMELEN];
    char		fileName[MAXFONTFILENAMELEN*2 + 1];
    FontNameRec		tmpName;
    FontEntryPtr	entry;
    FontScalableRec	vals;
    FontScalableEntryPtr   scalable;
    FontScaledPtr	scaled;
    FontBitmapEntryPtr	bitmap;
    FontAliasEntryPtr	alias;
    FontBCEntryPtr	bc;
    int			ret;
    Bool		noSpecificSize;
    FontRendererPtr     renderer;
    
    if (namelen >= MAXFONTNAMELEN)
	return AllocError;
    dir = (FontDirectoryPtr) fpe->private;
    CopyISOLatin1Lowered (lowerName, name, namelen);
    lowerName[namelen] = '\0';
    tmpName.name = lowerName;
    tmpName.length = namelen;
    tmpName.ndashes = CountDashes (lowerName, namelen);
    /* Match XLFD patterns */
    if (tmpName.ndashes == 14 &&
	FontParseXLFDName (lowerName, &vals, FONT_XLFD_REPLACE_ZERO))
    {
	tmpName.length = strlen (lowerName);
	entry = FontFileFindNameInDir (&dir->scalable, &tmpName);
	noSpecificSize = vals.point <= 0 && vals.pixel <= 0;
    	if (entry && entry->type == FONT_ENTRY_SCALABLE &&
	    FontFileCompleteXLFD (&vals, &entry->u.scalable.extra->defaults))
	{
	    scalable = &entry->u.scalable;
	    scaled = FontFileFindScaledInstance (entry, &vals, noSpecificSize);
	    /*
	     * A scaled instance can occur one of two ways:
	     *
	     *  Either the font has been scaled to this
	     *   size already, in which case scaled->pFont
	     *   will point at that font.
	     *
	     *  Or a bitmap instance in this size exists,
	     *   which is handled as if we got a pattern
	     *   matching the bitmap font name.
	     */
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
			ret = FontFileOpenBitmap (fpe, pFont, flags, entry,
						  format, fmask);
		    }
		}
		else /* "cannot" happen */
		{
		    ret = BadFontName;
		}
	    }
	    else
	    {
		ret = FontFileMatchBitmapSource (fpe, pFont, flags, entry, &tmpName, &vals, format, fmask, noSpecificSize);
		if (ret != Successful)
		{
		    /* Make a new scaled instance */
	    	    strcpy (fileName, dir->directory);
	    	    strcat (fileName, scalable->fileName);

                    if (entry->u.scalable.renderer->config.loaded != TRUE) {
               		ret = DynamicOpenFontLibrary(entry->u.scalable.renderer, renderers);
	                 if (ret == -1) return(BadFontName);
	                }
                    if (*scalable->renderer->OpenScalable != NULL) {
	    	    ret = (*scalable->renderer->OpenScalable) (fpe, pFont,
				flags, entry, fileName, &vals, format, fmask);
 	    	     }
			/* Save the instance */
		    if (ret == Successful)
		    	if (!FontFileAddScaledInstance (entry, &vals,
						    *pFont, (char *) 0))
			    (*pFont)->fpePrivate = (pointer) 0;
		}
	    }
	    if (ret == Successful)
		(*pFont)->info.cachable = TRUE;
	    return ret;
	}
	CopyISOLatin1Lowered (lowerName, name, namelen);
	tmpName.length = namelen;
    }
    /* Match non XLFD pattern */
    if (entry = FontFileFindNameInDir (&dir->nonScalable, &tmpName))
    {
	switch (entry->type) {
	case FONT_ENTRY_BITMAP:
#ifdef DEBUG
fprintf(stderr,"bitmap Font tmpName=%s\n",tmpName);
#endif
	    bitmap = &entry->u.bitmap;
	    if (bitmap->pFont)
	    {
	    	*pFont = bitmap->pFont;
	    	ret = Successful;
	    }
	    else
	    {
		ret = FontFileOpenBitmap (fpe, pFont, flags, entry, format, fmask);
	    }
	    break;
	case FONT_ENTRY_ALIAS:
	    alias = &entry->u.alias;
	    *aliasName = alias->resolved;
	    ret = FontNameAlias;
	    break;
	case FONT_ENTRY_BC:
	    bc = &entry->u.bc;
	    entry = bc->entry;
            if (entry->u.scalable.renderer->config.loaded != TRUE) {
               ret = DynamicOpenFontLibrary(entry->u.scalable.renderer, &renderers);
               if (ret == -1) return(BadFontName);
               }
	    if (*scalable->renderer->OpenScalable != NULL) {
	    ret = (*scalable->renderer->OpenScalable)
		    (fpe, pFont, flags, entry, &bc->vals, format, fmask);
	    }
            break;
	default:
	    ret = BadFontName;
	}
    }
    else
    {
	ret = BadFontName;
    }
    if (ret == Successful)
	(*pFont)->info.cachable = TRUE;
    return ret;
}

/* ARGSUSED */
FontFileCloseFont (fpe, pFont)
    FontPathElementPtr	fpe;
    FontPtr		pFont;
{
    FontEntryPtr    entry;
    FontRendererPtr currenderer;
    int free_lib=0;
/* check if last font opened for renderer and rendering library
 should be closed on  last close then call to do dlclose on
the renderering library */
    currenderer = pFont->rendererPtr;

    if ((currenderer->fonts_open ==  1) &&
	(currenderer->config.close_when == 1)) { 
#ifdef DEBUG
		fprintf(stderr, "currenderer is closing last font name=%s\n",currenderer->fileSuffix);
#endif
		free_lib = 1;
    }
    if (entry = (FontEntryPtr) pFont->fpePrivate) {
	switch (entry->type) {
	case FONT_ENTRY_SCALABLE:
	    FontFileRemoveScaledInstance (entry, pFont);
	    break;
	case FONT_ENTRY_BITMAP:
	    entry->u.bitmap.pFont = 0;
	    break;
	default:
	    /* "cannot" happen */
	    break;
	}
	pFont->fpePrivate = 0;
    }
    (*pFont->unload_font) (pFont);
    if (free_lib == 1) FontFileCloseLibrary(currenderer);
}

int
FontFileCloseLibrary(thisrenderer)
FontRendererPtr thisrenderer;
{
int ret,inuse, match=0;
char *name = thisrenderer->config.sharedlib_filename;
	match = CheckOtherSuffixes(thisrenderer, name);
	if (match > 0) {
		inuse = CheckUseBeforeLibClose(thisrenderer,name);
		if (inuse > 0) {
			return FALSE;
			}
	}
#ifdef DEBUG
fprintf(stderr,"calling dlcose for renderer=%s\n",thisrenderer->fileSuffix);
#endif
	if (*thisrenderer->FreeRenderer != NULL) {
		ret =(*thisrenderer->FreeRenderer)();
		/* let the renderer do some clean up before closing library */	
	}
	dlclose(thisrenderer->config.handle);
	thisrenderer->config.loaded = FALSE;
	thisrenderer->config.handle = 0;
	thisrenderer->config.initialized = FALSE;
	if (match > 0) ClearMatchingLib(thisrenderer, name);
	return TRUE;
}

FontFileOpenBitmap (fpe, pFont, flags, entry, format, fmask)
    FontPathElementPtr	fpe;
    int			flags;
    FontEntryPtr	entry;
    FontPtr		*pFont;
{
    FontBitmapEntryPtr	bitmap;
    char		fileName[MAXFONTFILENAMELEN*2+1];
    int			ret;
    FontDirectoryPtr	dir;
    FontRendererPtr     renderer;

    if (entry->u.bitmap.renderer->config.loaded != TRUE) {
	ret = DynamicOpenFontLibrary(entry->u.bitmap.renderer, &renderers);
	if (ret == -1) return(BadFontName);
	}
    dir = (FontDirectoryPtr) fpe->private;
    bitmap = &entry->u.bitmap;
    strcpy (fileName, dir->directory);
    strcat (fileName, bitmap->fileName);
    if (*bitmap->renderer->OpenBitmap != NULL) {
    ret = (*bitmap->renderer->OpenBitmap) 
			(fpe, pFont, flags, entry, fileName, format, fmask);
    }
    if (ret == Successful)
    {
	bitmap->pFont = *pFont;
	(*pFont)->fpePrivate = (pointer) entry;
    }
    return ret;
}

FontFileGetInfoBitmap (fpe, pFontInfo, entry)
    FontPathElementPtr	fpe;
    FontInfoPtr		pFontInfo;
    FontEntryPtr	entry;
{
    FontBitmapEntryPtr	bitmap;
    char		fileName[MAXFONTFILENAMELEN*2+1];
    int			ret;
    FontDirectoryPtr	dir;
    FontRendererPtr     renderer;

    if (entry->u.bitmap.renderer->config.loaded != TRUE) {
	ret = DynamicOpenFontLibrary(entry->u.bitmap.renderer, &renderers);
	 if (ret == -1) return(BadFontName);
	}
    dir = (FontDirectoryPtr) fpe->private;
    bitmap = &entry->u.bitmap;
    strcpy (fileName, dir->directory);
    strcat (fileName, bitmap->fileName);
    if (*bitmap->renderer->GetInfoBitmap != NULL) {
    ret = (*bitmap->renderer->GetInfoBitmap) (fpe, pFontInfo, entry, fileName);
    }
    return ret;
}

/* ARGSUSED */
FontFileListFonts (client, fpe, pat, len, max, names)
    pointer     client;
    FontPathElementPtr fpe;
    char       *pat;
    int         len;
    int         max;
    FontNamesPtr names;
{
    FontDirectoryPtr	dir;
    char		lowerChars[MAXFONTNAMELEN], zeroChars[MAXFONTNAMELEN];
    FontNameRec		lowerName;
    FontNameRec		zeroName;
    FontNamesPtr	scaleNames;
    FontScalableRec	vals, zeroVals, tmpVals;
    struct defaultVals {
	CARD16 x B16;
	CARD16 y B16;
	CARD16 point B16;
	} *defaultVals, *GetClientResolutions();
    int			i,num;
    int			oldnnames;

    if (len >= MAXFONTNAMELEN)
	return AllocError;
    dir = (FontDirectoryPtr) fpe->private;
    CopyISOLatin1Lowered (lowerChars, pat, len);
    lowerChars[len] = '\0';
    lowerName.name = lowerChars;
    lowerName.length = len;
    lowerName.ndashes = CountDashes (lowerChars, len);
    /* Match XLFD patterns */
    strcpy (zeroChars, lowerChars);
    if (lowerName.ndashes == 14 &&
	FontParseXLFDName (zeroChars, &vals, FONT_XLFD_REPLACE_ZERO))
    {
	oldnnames = names->nnames;
	scaleNames = MakeFontNamesRecord (0);
	if (!scaleNames)
	    return AllocError;
	zeroName.name = zeroChars;
	zeroName.length = strlen (zeroChars);
	zeroName.ndashes = lowerName.ndashes;
	FontFileFindNamesInDir (&dir->scalable, &zeroName, max, scaleNames);
	for (i = 0; i < scaleNames->nnames; i++)
	{
	    FontParseXLFDName (scaleNames->names[i], &zeroVals, FONT_XLFD_REPLACE_NONE);
	    tmpVals = vals;
	    if (FontFileCompleteXLFD (&tmpVals, &zeroVals))
	    {
		strcpy (zeroChars, scaleNames->names[i]);
		defaultVals = GetClientResolutions(&num);
		if (vals.pixel <= 0)
		    tmpVals.pixel = 0;
		if (vals.point <= 0)
		    tmpVals.point = 0;
		if (vals.width <= 0)
		    tmpVals.width = 0;
		if (vals.x <= 0) 
		   tmpVals.x = defaultVals->x;
		    /*tmpVals.x = 0;*/
		if (vals.y <= 0)
		    /*tmpVals.y = 0;*/
		      tmpVals.y = defaultVals->y;
		FontParseXLFDName (zeroChars, &tmpVals, FONT_XLFD_REPLACE_VALUE);
#ifdef DEBUG
fprintf(stderr,"zeroChars2=%s len=%d\n",zeroChars,strlen(zeroChars));
#endif
		(void) AddFontNamesName (names, zeroChars, strlen (zeroChars));
		/* add derived intstance stuff here */
		/* for each possible one replace vals.point */
	     }
	}
	FreeFontNames (scaleNames);
	max -= names->nnames - oldnnames;
    }
    else
    {
    	oldnnames = names->nnames;
    	FontFileFindNamesInDir (&dir->scalable, &lowerName, max, names);
	max -= names->nnames - oldnnames;
    }
    return FontFileFindNamesInDir (&dir->nonScalable, &lowerName, max, names);
}

typedef struct _LFWIData {
    FontNamesPtr    names;
    int		    current;
} LFWIDataRec, *LFWIDataPtr;

FontFileStartListFontsWithInfo(client, fpe, pat, len, max, privatep)
    pointer     client;
    FontPathElementPtr fpe;
    char       *pat;
    int         len;
    int         max;
    pointer    *privatep;
{
    LFWIDataPtr	data;
    int		ret;

    data = (LFWIDataPtr) xalloc (sizeof *data);
    if (!data)
	return AllocError;
    data->names = MakeFontNamesRecord (0);
    if (!data->names)
    {
	xfree (data);
	return AllocError;
    }
    ret = FontFileListFonts (client, fpe, pat, len, max, data->names);
    if (ret != Successful)
    {
	FreeFontNames (data->names);
	xfree (data);
	return ret;
    }
    data->current = 0;
    *privatep = (pointer) data;
    return Successful;
}

/* ARGSUSED */
static int
FontFileListOneFontWithInfo (client, fpe, namep, namelenp, pFontInfo)
    pointer		client;
    FontPathElementPtr	fpe;
    char		**namep;
    int			*namelenp;
    FontInfoPtr		*pFontInfo;
{
    FontDirectoryPtr	dir;
    char		lowerName[MAXFONTNAMELEN];
    char		fileName[MAXFONTFILENAMELEN*2 + 1];
    FontNameRec		tmpName;
    FontEntryPtr	entry;
    FontScalableRec	vals;
    FontScalableEntryPtr   scalable;
    FontScaledPtr	scaled;
    FontBitmapEntryPtr	bitmap;
    FontAliasEntryPtr	alias;
    int			ret;
    char		*name = *namep;
    int			namelen = *namelenp;
    Bool		noSpecificSize;
    FontRendererPtr	renderer;
    
    if (namelen >= MAXFONTNAMELEN)
	return AllocError;
    dir = (FontDirectoryPtr) fpe->private;
    CopyISOLatin1Lowered (lowerName, name, namelen);
    lowerName[namelen] = '\0';
    tmpName.name = lowerName;
    tmpName.length = namelen;
    tmpName.ndashes = CountDashes (lowerName, namelen);
    /* Match XLFD patterns */
    if (tmpName.ndashes == 14 &&
	FontParseXLFDName (lowerName, &vals, FONT_XLFD_REPLACE_ZERO))
    {
	tmpName.length = strlen (lowerName);
	entry = FontFileFindNameInDir (&dir->scalable, &tmpName);
	noSpecificSize = vals.point <= 0 && vals.pixel <= 0;
    	if (entry && entry->type == FONT_ENTRY_SCALABLE &&
	    FontFileCompleteXLFD (&vals, &entry->u.scalable.extra->defaults))
	{
	    scalable = &entry->u.scalable;
	    scaled = FontFileFindScaledInstance (entry, &vals, noSpecificSize);
	    /*
	     * A scaled instance can occur one of two ways:
	     *
	     *  Either the font has been scaled to this
	     *   size already, in which case scaled->pFont
	     *   will point at that font.
	     *
	     *  Or a bitmap instance in this size exists,
	     *   which is handled as if we got a pattern
	     *   matching the bitmap font name.
	     */
	    if (scaled)
	    {
		if (scaled->pFont)
		{
		    *pFontInfo = &scaled->pFont->info;
		    ret = Successful;
		}
		else if (scaled->bitmap)
		{
		    entry = scaled->bitmap;
		    bitmap = &entry->u.bitmap;
		    if (bitmap->pFont)
		    {
			*pFontInfo = &bitmap->pFont->info;
			ret = Successful;
		    }
		    else
		    {
			ret = FontFileGetInfoBitmap (fpe, *pFontInfo, entry);
		    }
		}
		else /* "cannot" happen */
		{
		    ret = BadFontName;
		}
	    }
	    else
	    {
#ifdef NOTDEF
		/* no special case yet */
		ret = FontFileMatchBitmapSource (fpe, pFont, flags, entry, &vals, format, fmask, noSpecificSize);
		if (ret != Successful)
#endif
		{
		    /* Make a new scaled instance */
	    	    strcpy (fileName, dir->directory);
	    	    strcat (fileName, scalable->fileName);
                    if (entry->u.scalable.renderer->config.loaded != TRUE) {
                         ret = DynamicOpenFontLibrary(entry->u.scalable.renderer, &renderers);
                         if (ret == -1) return(BadFontName);
                    }
		    if (*scalable->renderer->GetInfoScalable != NULL) {
	    	    ret = (*scalable->renderer->GetInfoScalable)
			(fpe, *pFontInfo, entry, &tmpName, fileName, &vals);
			}
		}
	    }
	    return ret;
	}
	CopyISOLatin1Lowered (lowerName, name, namelen);
	tmpName.length = namelen;
    }
    /* Match non XLFD pattern */
    if (entry = FontFileFindNameInDir (&dir->nonScalable, &tmpName))
    {
	switch (entry->type) {
	case FONT_ENTRY_BITMAP:
	    bitmap = &entry->u.bitmap;
	    if (bitmap->pFont)
	    {
	    	*pFontInfo = &bitmap->pFont->info;
	    	ret = Successful;
	    }
	    else
	    {
		ret = FontFileGetInfoBitmap (fpe, *pFontInfo, entry);
	    }
	    break;
	case FONT_ENTRY_ALIAS:
	    alias = &entry->u.alias;
	    *(char **)pFontInfo = name;
	    *namelenp = strlen (*namep = alias->resolved);
	    ret = FontNameAlias;
	    break;
	case FONT_ENTRY_BC:
#ifdef NOTYET
	    /* no LFWI for this yet */
	    bc = &entry->u.bc;
	    entry = bc->entry;
	    /* Make a new scaled instance */
    	    strcpy (fileName, dir->directory);
    	    strcat (fileName, scalable->fileName);
            if (entry->u.scalable.renderer->config.loaded != TRUE) {
                 ret = DynamicOpenFontLibrary(entry->u.scalable.renderer, &renderers);
                 if (ret == -1) return(BadFontName);
            }
 
            if (*scalable->renderer->GetInfoScalable != NULL) {
	    ret = (*scalable->renderer->GetInfoScalable)
		    (fpe, *pFontInfo, entry, tmpName, fileName, &bc->vals);
            }
#endif
	    break;
	default:
	    ret = BadFontName;
	}
    }
    else
    {
	ret = BadFontName;
    }
/* check if last font opened for renderer and rendering library
 should be closed on  last close then call to do dlclose on
the renderering library */

    if ((entry->u.scalable.renderer->config.loaded == TRUE)  &&
	(entry->u.scalable.renderer->config.close_when == 1) && 
	(entry->u.scalable.renderer->fonts_open <= 0)) { 
#ifdef DEBUG
	fprintf(stderr,"calling dlcose for renderer=%s\n",entry->u.scalable.renderer->fileSuffix);
#endif
		FontFileCloseLibrary(entry->u.scalable.renderer);
	}
    return ret;
}

FontFileListNextFontWithInfo(client, fpe, namep, namelenp, pFontInfo,
			     numFonts, private)
    pointer		client;
    FontPathElementPtr	fpe;
    char		**namep;
    int			*namelenp;
    FontInfoPtr		*pFontInfo;
    int			*numFonts;
    pointer		private;
{
    LFWIDataPtr	data = (LFWIDataPtr) private;
    int		ret;
    char	*name;
    int		namelen;

    if (data->current == data->names->nnames)
    {
	FreeFontNames (data->names);
	xfree (data);
	return BadFontName;
    }
    name = data->names->names[data->current];
    namelen = data->names->length[data->current];
    ret = FontFileListOneFontWithInfo (client, fpe, &name, &namelen, pFontInfo);
    if (ret == BadFontName)
	ret = AllocError;
    *namep = name;
    *namelenp = namelen;
    ++data->current;
    *numFonts = data->names->nnames - data->current;
    return ret;
}

typedef int (*IntFunc) ();
static int  font_file_type;


XFontFileRegisterFpeFunctions(callerType)
int callerType;
{
    static Bool Xbeenhere = FALSE;

    if ((callerType == XSERVER_TYPE) && !Xbeenhere) {
		XFontFileRegisterFontFileFunctions(XSERVER_TYPE);
		Xbeenhere = TRUE;

    }
    font_file_type = RegisterFPEFunctions(FontFileNameCheck,
					  FontFileInitFPE,
					  FontFileFreeFPE,
					  FontFileResetFPE,
					  FontFileOpenFont,
					  FontFileCloseFont,
					  FontFileListFonts,
					  FontFileStartListFontsWithInfo,
					  FontFileListNextFontWithInfo,
					  (IntFunc) 0,
					  (IntFunc) 0);
}


FontFileRegisterFpeFunctions()
{
    static Bool beenhere = FALSE;


    if (!beenhere) {
	FontFileRegisterFontFileFunctions ();
	beenhere = TRUE;
    }
    font_file_type = RegisterFPEFunctions(FontFileNameCheck,
					  FontFileInitFPE,
					  FontFileFreeFPE,
					  FontFileResetFPE,
					  FontFileOpenFont,
					  FontFileCloseFont,
					  FontFileListFonts,
					  FontFileStartListFontsWithInfo,
					  FontFileListNextFontWithInfo,
					  (IntFunc) 0,
					  (IntFunc) 0);
}
