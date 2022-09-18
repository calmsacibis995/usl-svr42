/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/egerror.c	1.1.2.2"
#ident "@(#)egerror.c	1.1 'attmail mail(1) command'"
#include	<stdio.h>
#include	<libc.h>
#include	"re.h"

void
re_error(s)
	char *s;
{
	fprintf(stderr, "pattern error: %s\n", s);
	exit(1);
	/* NOTREACHED */
}
