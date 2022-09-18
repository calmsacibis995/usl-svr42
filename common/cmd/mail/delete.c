/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/delete.c	1.9.2.2"
#ident "@(#)delete.c	2.11 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	delete - catch the interrupt key and other signals

    SYNOPSIS
	void delete(i)

    DESCRIPTION
	Delete() resets signals on quits and interupts
	and then does a long jump back to the middle of main()
	(which will immediately exit via done())
	or to the middle of printmail().
	If -q is specified, it exits.
	Delete() exits on other signals.

		i	-> signal #
*/

void delete(i)
register int i;
{
    static const char pn[] = "delete";

    setsig(i, delete);

    if ((i==SIGINT) || (i==SIGQUIT))
	{
	fprintf(stderr, "\n");
	if (!flgq)
	    longjmp(sjbuf, 1);
	}

    else
	{
	pfmt(stderr, MM_ERROR, ":41:Signal %d\n", i);
	Dout(pn, 0, "caught signal %d\n", i);
	}

    done(0);
}
