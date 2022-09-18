/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/pfmt.c	1.5"

/*LINTLIBRARY*/

#ifdef __STDC__
	#pragma weak pfmt = _pfmt
#endif
#include "synonyms.h"
#include <pfmt.h>
#include <stdio.h>
#include <stdarg.h>

/* pfmt() - format and print */

int
#ifdef __STDC__
pfmt(FILE *stream, long flag, const char *format, ...)
#else
pfmt(stream, flag, format, va_alist)
FILE *stream;
long flag;
const char *format;
va_dcl
#endif
{
	va_list args;

#ifdef __STDC__
	va_start(args,);
#else
	va_start(args);
#endif
	return __pfmt_print(stream, flag, format, NULL, NULL, args);
}
