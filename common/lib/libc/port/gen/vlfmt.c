/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/vlfmt.c	1.2"

/*LINTLIBRARY*/

/* vlfmt() - format, print and log (variable arguments) */

#ifdef __STDC__
	#pragma weak vlfmt = _vlfmt
#endif
#include "synonyms.h"
#include <pfmt.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pfmt_data.h"

int
vlfmt(stream, flag, format, args)
FILE *stream;
long flag;
const char *format;
va_list args;
{
	int ret;
	const char *text, *sev;
	
	if ((ret = __pfmt_print(stream, flag, format, &text, &sev, args)) < 0)
		return ret;

	return __lfmt_log(text, sev, args, flag, ret);
}
