/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/nqtext.c	1.1"
#ident  "$Header: $"
#include <stdio.h>
#include <varargs.h>

extern int	puttext();

extern char	*prog;

/*VARARGS*/
void
nqtext(va_alist)
va_dcl
{
	va_list ap;
	FILE	*fp;
	char	*fmt;
	char	buffer[2048];


	va_start(ap);
	fp = va_arg(ap, FILE *);
	fmt = va_arg(ap, char *);
	va_end(ap);
		
	(void) vsprintf(buffer, fmt, ap);

	(void) puttext(fp, buffer, 0, 70);
	(void) putc('\n', fp);
}
