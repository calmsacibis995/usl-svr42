/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)iconv:loc_msg.c	1.2"
#ident "$Header: loc_msg.c 1.1 91/07/02 $"

/* Dummy functions to emulate SVR4ES messaging */

#include <pfmt.h>
#include <stdio.h>
#include <ctype.h>

char *
gettxt(msgid, dflt)
char *msgid, *dflt;
{
	return(dflt);
}

extern char* sys_errlist[];
extern int sys_nerr;

char *
strerror(errnum)
int errnum;
{
	if (errnum < sys_nerr && errnum >= 0)
		return(sys_errlist[errnum]);
	else
		return(NULL);
}

char *
setlocale(cat, loc)
int cat;
char *loc;
{
	return (char *)NULL;
}

int
#ifdef __STDC__
pfmt(FILE *stream, long flag, char *format, ...)
#else
pfmt(stream, flag, format, va_alist)
FILE *stream;
long flag;
char *format;
va_dcl
#endif
{
	va_list args;
	char *ptr;
	int status;
	register int length = 0;

#ifdef __STDC__
	va_start(args,);
#else
	va_start(args);
#endif

	if (format) {
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

char *
setcat(cat)
char *cat;
{
	return NULL;
}
int
setlabel(label)
char *label;
{
	return 0;
}
