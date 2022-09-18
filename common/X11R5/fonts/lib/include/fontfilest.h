/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontinc:lib/include/fontfilest.h	1.2"


/*
 * $XConsortium: fontfilest.h,v 1.3 91/07/16 20:15:16 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef _FONTFILEST_H_
#define _FONTFILEST_H_

#include <stdio.h>
#include    <X11/Xos.h>
#include    "fontmisc.h"
#include    "font.h"
#include    "fontstruct.h"
#include    "fontfile.h"

typedef struct _FontName {
    char	*name;
    short	length;
    short	ndashes;
} FontNameRec;

typedef struct _FontScaled {
    FontScalableRec	vals;
    FontEntryPtr	bitmap;
    FontPtr		pFont;
} FontScaledRec;

typedef struct _FontScalableExtra {
    FontScalableRec	defaults;
    int			numScaled;
    int			sizeScaled;
    FontScaledPtr	scaled;
    pointer		private;
} FontScalableExtraRec;

typedef struct _FontScalableEntry {
    FontRendererPtr	    renderer;
    char		    *fileName;
    FontScalableExtraPtr   extra;
} FontScalableEntryRec;

/*
 * This "can't" work yet - the returned alias string must be permanent,
 * but this layer would need to generate the appropriate name from the
 * resolved scalable + the XLFD values passed in.  XXX
 */

typedef struct _FontScaleAliasEntry {
    char		*resolved;
} FontScaleAliasEntryRec;

typedef struct _FontBitmapEntry {
    FontRendererPtr	renderer;
    char		*fileName;
    FontPtr		pFont;
} FontBitmapEntryRec;

typedef struct _FontAliasEntry {
    char	*resolved;
} FontAliasEntryRec;

typedef struct _FontBCEntry {
    FontScalableRec	    vals;
    FontEntryPtr	    entry;
} FontBCEntryRec;

typedef struct _FontEntry {
    FontNameRec	name;
    int		type;
    union _FontEntryParts {
	FontScalableEntryRec	scalable;
	FontBitmapEntryRec	bitmap;
	FontAliasEntryRec	alias;
	FontBCEntryRec		bc;
    } u;
} FontEntryRec;

typedef struct _FontTable {
    int		    used;
    int		    size;
    FontEntryPtr    entries;
    Bool	    sorted;
} FontTableRec;

typedef struct _FontDirectory {
    char	    *directory;
    unsigned long   dir_mtime;
    unsigned long   alias_mtime;
    FontTableRec    scalable;
    FontTableRec    nonScalable;
} FontDirectoryRec;


typedef struct _BitmapInstance {
    FontScalableRec	vals;
    FontBitmapEntryPtr	bitmap;
} BitmapInstanceRec, *BitmapInstancePtr;

typedef struct _BitmapScalablePrivate {
    int			numInstances;
    BitmapInstancePtr	instances;
} BitmapScalablePrivateRec, *BitmapScalablePrivatePtr;

typedef struct _BitmapSources {
    FontPathElementPtr	*fpe;
    int			size;
    int			count;
} BitmapSourcesRec, *BitmapSourcesPtr;

extern BitmapSourcesRec	FontFileBitmapSources;


#endif /* _FONTFILEST_H_ */

