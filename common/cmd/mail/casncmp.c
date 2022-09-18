/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/casncmp.c	1.7.2.2"
#ident "@(#)casncmp.c	2.9 'attmail mail(1) command'"
#include "libmail.h"
#include <ctype.h>
/*
    NAME
	casncmp - compare strings ignoring case

    SYNOPSIS
	int casncmp(const char *s1, const char *s2, int n)

    DESCRIPTION
	Compare two strings ignoring case differences.
	Stop after n bytes or the trailing NUL.
*/

#define lower(c)        ( isupper(c) ? _tolower(c) : (c) )

int casncmp(s1, s2, n)
register const char *s1, *s2;
register n;
{
	if (s1 == s2)
		return(0);
	while ((--n >= 0) && (lower(*s1) == lower(*s2))) {
		s2++;
		if (*s1++ == '\0')
			return (0);
	}
	return ((n < 0) ? 0 : (lower(*s1) - lower(*s2)));
}
