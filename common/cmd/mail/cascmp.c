/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/cascmp.c	1.3.2.2"
#ident "@(#)cascmp.c	1.4 'attmail mail(1) command'"
#include "libmail.h"
#include <ctype.h>
/*
    NAME
	cascmp - compare strings ignoring case

    SYNOPSIS
	int cascmp(const char *s1, const char *s2)

    DESCRIPTION
	Compare two strings ignoring case differences.
	Stop at the trailing NUL.
*/

#define lower(c)        ( isupper(c) ? _tolower(c) : (c) )

int cascmp(s1, s2)
register const char *s1, *s2;
{
	if (s1 == s2)
		return(0);
	while (*s1 && (lower(*s1) == lower(*s2)))
		s1++, s2++;
	return (lower(*s1) - lower(*s2));
}
