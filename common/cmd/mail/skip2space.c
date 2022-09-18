/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/skip2space.c	1.2.2.2"
#ident "@(#)skip2space.c	1.2 'attmail mail(1) command'"
#include "libmail.h"
#include <ctype.h>
/*
    NAME
	skiptospace - skip up to white space

    SYNOPSIS
	const char *skiptospace(const char *p)

    DESCRIPTION
	skiptospace() looks through the string for either
	end of the string or a space character.
*/

const char *skiptospace(p)
register const char	*p;
{
    while (*p && !isspace(*p))
	p++;
    return (p);
}
