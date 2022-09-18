/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xau:AuFileName.c	1.2"
/*
 * Xau - X Authorization Database Library
 *
 * $XConsortium: AuFileName.c,v 1.2 91/01/08 15:09:00 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xauth.h>

char *
XauFileName ()
{
    char    *name, *malloc (), *getenv ();
    char    *strcat (), *strcpy ();
    static char	*buf;
    static int	bsize;
    int	    size;

    if (name = getenv ("XAUTHORITY")) {
	return name;
    } else if (name = getenv ("HOME")) {
	size = strlen (name) + strlen(".Xauthority") + 2;
	if (size > bsize) {
	    if (buf)
		free (buf);
	    buf = malloc ((unsigned) size);
	    if (!buf)
		return 0;
	    bsize = size;
	}
	strcpy (buf, name);
	strcat (buf, "/.Xauthority" + (name[1] == '\0' ? 1 : 0));
	return buf;
    }
    return 0;
}
