/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/trimnl.c	1.4.2.2"
#ident "@(#)trimnl.c	1.5 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	trimnl - trim trailing newlines from string

    SYNOPSIS
	void trimnl(char *s)

    DESCRIPTION
	trimnl() goes to the end of the string and
	removes an trailing newlines.
*/

void trimnl(s)
register char 	*s;
{
    register char	*p;

    p = s + strlen(s) - 1;
    while ((*p == '\n') && (p >= s))
	*p-- = '\0';
}
