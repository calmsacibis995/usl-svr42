/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/pfmt.c	1.4"

#ifdef __STDC__
	#pragma weak pfmt = _pfmt
#endif
#include "synonyms.h"
#include <pfmt.h>
#include <stdio.h>
#include <ctype.h>

/* pfmt() - format and print */

/*ARGSUSED*/
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
	const char *ptr;
	int status;
	register int length = 0;

#ifdef __STDC__
	va_start(args,);
#else
	va_start(args);
#endif

	if (!(flag & MM_NOGET) && format) {
		ptr = format;
		while(*ptr++ != ':');
		*ptr++;
		while (isdigit(*ptr++));
		
		format = ptr;

	}

	if (stream){
		if ((status = vfprintf(stream, format, args)) < 0)
			return -1;
		length += status;
	}

	return length;
}
