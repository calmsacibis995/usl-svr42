/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/errexit.c	1.6.2.3"
#ident "@(#)errexit.c	1.8 'attmail mail(1) command'"
#include "libmail.h"

/*
    NAME
	errexit - print error message and exit

    SYNOPSIS
	void errexit(int exitval, int errno, char *fmt, ...)

    DESCRIPTION
	Errexit() prints the given error message and exits with
	the given exit value. If errno is non-zero, then
	perror(3) is called for that error number. Newlines
	are assumed to be part of the string.
*/

/* PRINTFLIKE3 */
#ifdef __STDC__
void errexit(int exitval, int sverrno, char *fmt, ...)
#else
# ifdef lint
void errexit(Xexitval, Xsverrno, Xfmt, va_alist)
int Xexitval;
int Xsverrno;
char *Xfmt;
va_dcl
# else
void errexit(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
    int exitval, sverrno;
    char *fmt;
#endif
    va_list ap;

#ifndef __STDC__
# ifdef lint
    exitval = Xexitval;
    sverrno = Xsverrno;
    fmt = Xfmt;
# endif
#endif

#ifdef __STDC__
    va_start(ap, fmt);
#else
    va_start(ap);
    exitval = va_arg(ap, int);
    sverrno = va_arg(ap, int);
    fmt = va_arg(ap, char*);
#endif

    (void) vpfmt(stderr, MM_ERROR, fmt, ap);
    va_end(ap);

    if (sverrno != 0)
	pfmt(stderr, MM_INFO, ":340:%s\n", strerror(sverrno));

    exit(exitval);
    /* NOTREACHED */
}
