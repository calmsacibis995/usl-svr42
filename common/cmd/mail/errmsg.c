/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/errmsg.c	1.9.2.3"
#ident "@(#)errmsg.c	2.12 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	errmsg - print error message

    SYNOPSIS
	void errmsg(int error_value, const char *error_message, ...)

    DESCRIPTION
	Errmsg() prints error messages. If error_message is supplied,
	that is taken as the text for the message, otherwise the
	text for the err_val message is gotten from the errlist[] array.
*/

/* PRINTFLIKE2 */
void
#ifdef __STDC__
errmsg(int err_val, const char *err_txt, ...)
#else
# ifdef lint
errmsg(Xerr_val, Xerr_txt, va_alist)
int	Xerr_val;
char	*Xerr_txt;
va_dcl
# else
errmsg(va_alist)
va_dcl
# endif
#endif
{
    static const char pn[] = "errmsg";
#ifndef __STDC__
    int err_val;
    char *err_txt;
#endif
    va_list args;

#ifndef __STDC__
# ifdef lint
    err_val = Xerr_val;
    err_txt = Xerr_txt;
# endif
#endif

#ifdef __STDC__
    va_start(args, err_txt);
#else
    va_start(args);
    err_val = va_arg(args, int);
    err_txt = va_arg(args, char*);
#endif

    error = err_val;
    if (err_txt && *err_txt)
	{
	vpfmt(stderr, MM_ERROR, err_txt, args);
	vDout(pn, 0, err_txt, args);
	}

    else
	{
	pfmt(stderr, MM_ERROR, errlist[err_val]);
	Dout(pn, 0, "%s", errlist[err_val]);
	}

    Dout(pn, 0,"error set to %d\n", error);
    va_end(args);
}
