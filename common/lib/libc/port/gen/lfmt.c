/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/lfmt.c	1.3"

/* lfmt() - format, print and log */

#ifdef __STDC__
	#pragma weak lfmt = _lfmt
#endif
#include "synonyms.h"
#include <pfmt.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pfmt_data.h"

int
#ifdef __STDC__
lfmt(FILE *stream, long flag, const char *format, ...)
#else
lfmt(stream, flag, format, va_alist)
FILE *stream;
long flag;
const char *format;
va_dcl
#endif
{
	int ret;
	va_list args;
	const char *text, *sev;
	
#ifdef __STDC__
	va_start(args,);
#else
	va_start(args);
#endif

	if ((ret = __pfmt_print(stream, flag, format, &text, &sev, args)) < 0)
		return ret;

	return __lfmt_log(text, sev, args, flag, ret);
}
