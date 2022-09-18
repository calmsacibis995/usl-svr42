/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:os/fonttype.c	1.1"
/************************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



************************************************************************/

/* $XConsortium: fonttype.c,v 1.4 88/10/10 18:22:45 rws Exp $ */

#include "dixfont.h"
#include "fonttype.h"

#ifndef UNCOMPRESSFILT
#define UNCOMPRESSFILT "/usr/ucb/uncompress"
#endif
#ifndef FCFLAGS
#define FCFLAGS "-t"
#endif
#ifndef BDFTOSNFFILT
#define BDFTOSNFFILT "/usr/bin/X11/bdftosnf"
#endif
#ifndef SHELLPATH
#define SHELLPATH "/bin/sh"
#endif
#ifndef ZBDFTOSNFFILT
#define ZBDFTOSNFFILT "/usr/ucb/uncompress | /usr/bin/X11/bdftosnf -t"
#endif

extern FontPtr	ReadSNFFont();
extern Bool	ReadSNFProperties();
extern void	FreeSNFFont();

#ifdef FONT_SNF
#ifdef COMPRESSED_FONTS
static char *
snfZFilter[] = {UNCOMPRESSFILT, NULL};
#endif
#endif

#ifdef FONT_BDF
static char *
bdfFilter[] = {BDFTOSNFFILT, FCFLAGS, NULL};
#ifdef COMPRESSED_FONTS
static char *
bdfZFilter[] = {SHELLPATH, "-c", ZBDFTOSNFFILT, NULL};
#endif
#endif

FontFileReaderRec fontFileReaders[] = {
#ifdef FONT_SNF
    {".snf", ReadSNFFont, ReadSNFProperties, FreeSNFFont, (char **)NULL},
#ifdef COMPRESSED_FONTS
    {".snf.Z", ReadSNFFont, ReadSNFProperties, FreeSNFFont, snfZFilter},
#endif
#endif
#ifdef FONT_BDF
    {".bdf", ReadSNFFont, ReadSNFProperties, FreeSNFFont, bdfFilter},
#ifdef COMPRESSED_FONTS
    {".bdf.Z", ReadSNFFont, ReadSNFProperties, FreeSNFFont, bdfZFilter},
#endif
#endif
    NULL
};

