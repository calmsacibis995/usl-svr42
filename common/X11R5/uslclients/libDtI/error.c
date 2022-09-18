/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libDtI:error.c	1.2"
#endif

#ifdef NOT_USE
#include <stdio.h>
#include <stdarg.h>

void
Dm__vaprtwarning(char *format, ... )
{
	va_list ap;

	va_start(ap, format);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, format, ap);
	va_end(ap);
}

void
Dm__vaprterror(char *format, ... )
{
	va_list ap;

	va_start(ap, format);
	fprintf(stderr, "error: ");
	vfprintf(stderr, format, ap);
	va_end(ap);
}
#endif
