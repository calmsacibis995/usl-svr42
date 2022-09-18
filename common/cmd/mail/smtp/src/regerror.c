/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/regerror.c	1.4.2.2"
#ident "@(#)regerror.c	1.4 'attmail mail(1) command'"
#include <string.h>
#include <unistd.h>
#ifdef SVR4_1
#include <stdio.h>
#include <pfmt.h>
#endif

void regerror(s)
	char *s;
{
#ifdef SVR4_1
	pfmt(stderr, MM_ERROR, s);
#else
	/* Skip over SVR4ES message number */
	for (s++; *s != ':'; )
		s++;
	s++;
	write(2, s, strlen(s));
#endif
	exit(1);
}
