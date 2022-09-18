/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/fontfilewr.c	1.1"
/*
 * $XConsortium: fontfilewr.c,v 1.1 91/09/07 11:58:12 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 * Author:  Keith Packard, MIT X Consortium
 */

#include <fontfileio.h>

FontFilePtr
FontFileOpenWrite (name)
    char    *name;
{
    int	fd;

    fd = creat (name, 0666);
    if (fd < 0)
	return 0;
    return (FontFilePtr) BufFileOpenWrite (fd);
}

FontFilePtr
FontFileOpenWriteFd (fd)
{
    return (FontFilePtr) BufFileOpenWrite (fd);
}

FontFilePtr
FontFileOpenFd (fd)
    int	fd;
{
    return (FontFilePtr) BufFileOpenRead (fd);
}
