/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/fontfileio.c	1.1"
/*
 * $XConsortium: fontfileio.c,v 1.1 91/09/07 11:58:03 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 * Author:  Keith Packard, MIT X Consortium
 */

#include <fontfileio.h>

FontFilePtr
FontFileOpen (name)
    char    *name;
{
    int		fd;
    int		len;
    BufFilePtr	raw, cooked;

    fd = open (name, 0);
    if (fd < 0)
	return 0;
    raw = BufFileOpenRead (fd);
    if (!raw)
    {
	close (fd);
	return 0;
    }
    len = strlen (name);
    if (len > 2 && !strcmp (name + len - 2, ".Z")) {
	cooked = BufFilePushCompressed (raw);
	if (!cooked) {
	    BufFileClose (raw, TRUE);
	    return 0;
	}
	raw = cooked;
    }
    return (FontFilePtr) raw;
}

FontFileClose (f)
    FontFilePtr	f;
{
    BufFileClose ((BufFilePtr) f, TRUE);
}

