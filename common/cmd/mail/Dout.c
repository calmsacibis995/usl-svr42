/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/Dout.c	1.5.2.3"
#ident "@(#)Dout.c	1.7 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	Dout - Print debug output

    SYNOPSIS
	void Dout(char *subname, int level, const char *msg, ...)

    DESCRIPTION
	Dout prints debugging output if debugging is turned
	on (-x specified) and the level of this message is
	lower than the value of the global variable debug.
	The subroutine name is printed if it is not a null
	string.
*/

/* PRINTFLIKE3 */
void
#ifdef __STDC__
Dout(const char *subname, int level, const char *fmt, ...)
#else
# ifdef lint
Dout(Xsubname, Xlevel, Xfmt, va_alist)
char *Xsubname;
int Xlevel;
char *Xfmt;
va_dcl
# else
Dout(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
	char    *subname;
	int	level;
	char    *fmt;
#endif
	va_list args;

#ifndef __STDC__
# ifdef lint
	subname = Xsubname;
	level = Xlevel;
	fmt = Xfmt;
# endif
#endif

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	subname = va_arg(args, char *);
	level = va_arg(args, int);
	fmt = va_arg(args, char *);
#endif

	if (debug > level)
		vDout(subname, level, fmt, args);
	va_end(args);
}

void vDout(subname, level, fmt, args)
const char *subname;
int level;
const char *fmt;
va_list args;
{
	if (debug > level) {
		if (subname && *subname) {
			fprintf(dbgfp,"%s(): ", subname);
		}
		vfprintf(dbgfp, fmt, args);
	}
}
