/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/savdead.c	1.2.2.2"
#ident "@(#)savdead.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	savdead - handle interrupts when sending mail

    SYNOPSIS
	void delete(i)

    DESCRIPTION
	Savdead() catches the interrupt key when sending mail.
	It remembers that the interrupt has been typed and lets
	the original delete() function take care of things.
	It also sets a flag indicating that this message should
	not be returned and that a dead.letter file should be
	created if another interrupt key is pressed.
*/

void savdead()
{
	static const char pn[] = "savdead";
	(void) setsig(SIGINT, saveint);
	interrupted = 1;
	topmsg->ret_on_error = 0;	/* do not send back letter on interrupt */
	Dout(pn, 0, "ret_on_error set to 0\n");
	if (!error) {
		error = E_REMOTE;
		Dout(pn, 0, "error set to %d\n", error);
	}
	maxerr = error;
	Dout(pn, 0, "maxerr set to %d\n", maxerr);
}
