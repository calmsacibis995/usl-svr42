/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/done.c	1.7.2.2"
#ident "@(#)done.c	2.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	done - clean up lock files and exit

    SYNOPSIS
	int done(int needtmp)

    DESCRIPTION
	Do whatever cleanup processing is necessary
	to exit, such as cleaning up lock files
	and tmp files.
*/

void done(needtmp)
int	needtmp;
{
	static const char pn[] = "done";
	unlock();

	if (!maxerr) {
		maxerr = error;
		Dout(pn, 0, "maxerr set to %d\n", maxerr);
		if (flgx > 0)
			unlink(dbgfname);
	}

	if (maxerr)
		mkdead(topmsg);

	if (!needtmp) {
		Tmpfile *p;
		for (p = toptmpfile; p; p = p->next)
			if (p->lettmp)
				unlink(p->lettmp);
	}

	exit(maxerr);
	/* NOTREACHED */
}
