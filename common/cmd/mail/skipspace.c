/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/skipspace.c	1.7.2.2"
#ident "@(#)skipspace.c	2.8 'attmail mail(1) command'"
#include "libmail.h"
#include <ctype.h>
/*
    NAME
	skipspace - skip past white space

    SYNOPSIS
	const char *skipspace(const char *p)

    DESCRIPTION
	skipspace() looks through the string for either
	end of the string or a non-space character.
*/

const char *skipspace(p)
register const char *p;
{
    while (*p && isspace(*p))
	p++;
    return (p);
}
