/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/echo.c	1.2.6.3"
#ident  "$Header: $"
#include <stdio.h>
#include <varargs.h>

extern	int	nointeract;
int	quietmode;

/*VARARGS*/
void
echo(va_alist)
va_dcl
{
	va_list ap;
	char	*fmt;
	char	*quiet;

	va_start(ap);
	fmt = va_arg(ap, char *);
	va_end(ap);
		

	/*
	 * If quietmode or nointeract is set
	 * do not produce any output.
	 */
	if(quietmode || nointeract)
		return;

	(void) vfprintf(stderr, fmt, ap);
	(void) putc('\n', stderr);
}
