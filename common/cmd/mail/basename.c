/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/basename.c	1.5.2.2"
#ident "@(#)basename.c	1.6 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	basename - return base from pathname

    SYNOPSIS
	char *basename(const char *path)

    DESCRIPTION
	basename() returns a pointer to the base
	component of a pathname.

	Like strchr() and family, this function
	returns a "char*" instead of a "const char*".
	This is somewhat questionable, but works for us.
*/

char *
basename(path)
	const char *path;
{
	char *cp;

	cp = strrchr(path, '/');
	return cp==NULL ? (char*)path : cp+1;
}
