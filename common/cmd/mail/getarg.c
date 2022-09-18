/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/getarg.c	1.5.2.2"
#ident "@(#)getarg.c	2.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	getarg - get next token

    SYNOPSIS
	char *getarg(char *s, char *p)

    DESCRIPTION
	Copies the next token from the string into a buffer.

		p	-> string to be searched
		s	-> area to return token

	returns:
		p	-> updated string pointer
		s	-> token
		NULL	-> no token
*/

char *getarg(s, p)	
register char *s, *p;
{
    p = (char*)skipspace(p);
    if (*p == '\n' || *p == '\0')
	return(NULL);
    while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
	*s++ = *p++;
    *s = '\0';
    return(p);
}
