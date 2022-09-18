/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:os/fonttype.h	1.1"
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/
/* $XConsortium: fonttype.h,v 1.3 88/10/10 16:48:00 rws Exp $ */

#ifndef FONTTYPE_H
#define FONTTYPE_H

#include "font.h"
#include "misc.h"

/*
 * Parameterize the procedures for internalizing a font dependent upon the
 * extension of the file name. This is somewhat OS dependent, but most
 * OSs support this kind of thing.
 */
typedef FontPtr (*ReadFontProc)(/* FID */);
typedef Bool (*ReadPropertiesProc)(/* FID, FontInfoPtr, DIXFontPropPtr */);
typedef void (*FreeFontProc)(/* FontPtr */);

typedef struct _FontFileReader {
    char *		extension;
    ReadFontProc	loadFont;
    ReadPropertiesProc	loadProperties;
    FreeFontProc	freeFont;
    char **		filter;
} FontFileReaderRec, *FontFileReader;

extern FontFileReaderRec fontFileReaders[];



#endif /* FONTTYPE_H */
