/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/retmail.c	1.4.2.3"
#ident "@(#)retmail.c	1.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	retmail - return a mail message

    SYNOPSIS
	void retmail(Msg *pmsg, int wherefrom, int rc, const char *fmt, args)

    DESCRIPTION
	Print an error message. Initialize pmsg->errmsg, if necessary.
	Add appropriate headers describing this error message.
*/

/* VARARGS4 */
void
#ifdef __STDC__
retmail(Msg *pmsg, int wherefrom, int rc, const char *fmt, ...)
#else
# ifdef lint
retmail(Xpmsg, Xwherefrom, Xrc, Xfmt, va_alist)
Msg *Xpmsg;
int Xwherefrom;
int Xrc;
char *Xfmt;
va_dcl
# else
retmail(va_alist)
va_dcl
# endif
#endif
{
    int msgtype;
#ifndef __STDC__
    Msg *pmsg;
    int wherefrom;
    int rc;
    char *fmt;
#endif
    va_list args;

#ifndef __STDC__
# ifdef lint
    pmsg = Xpmsg;
    wherefrom = Xwherefrom;
    rc = Xrc;
    fmt = Xfmt;
# endif
#endif

#ifdef __STDC__
    va_start(args, fmt);
#else
    va_start(args);
    pmsg = va_arg(args, Msg*);
    wherefrom = va_arg(args, int);
    rc = va_arg(args, int);
    fmt = va_arg(args, char*);
#endif

    if (fmt)
	{
	(void) vpfmt (stderr, MM_ERROR, fmt, args);
	(void) fprintf (stderr, "\n");
	msgtype = MM_NOSTD;
	}

    else
	msgtype = MM_ERROR;

    /* is this message coming from postmaster? */
    if (!pmsg->ret_on_error)
	{
	pfmt(stderr, MM_NOSTD, ":62:Cannot return mail.\n");
	return;
	}

    /* check Delivery-Options: */
    if (ckdlivopts(pmsg) & IGNORE)
        return;

    pfmt(stderr, msgtype, ":61:Return to %s\n", s_to_c(pmsg->Rpath));
    if (!pmsg->errmsg)
	{
	pmsg->errmsg = new_Msg();
	init_retmail(pmsg, pmsg->errmsg, rc);
	}

    /* add in the specific information for this message */
    add_retmail(pmsg->errmsg, wherefrom, rc);
}
