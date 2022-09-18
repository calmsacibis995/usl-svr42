/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/nwsendlist.c	1.2.2.3"
#ident "@(#)nwsendlist.c	1.4 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	nw_sendlist - optionally fork to send message

    SYNOPSIS
	int nw_sendlist(Msg *pmsg)

    DESCRIPTION
	nw_sendlist() is a "no-wait" version of sendlist().
	nw_sendlist() calls sendlist() to send a message.
	If flgw is set, the processing will be done within
	the background. It returns 1 if the sending fails, 0 otherwise.
*/

int nw_sendlist(pmsg)
Msg	*pmsg;
{
	static const char pn[] = "nw_sendlist";
	pid_t pid;

	Dout(pn, 0, "entered\n");
	if (flgw) {
		Dout(pn, 3, "-w\n");
		if ((pid = loopfork()) == 0) {
			Dout(pn, 3, "fork succeeded\n");
			setpgrp(); /* prevent foreground signals from affecting background */
			(void) sendlist(pmsg, 0);
			_exit(0);
			/* NOTREACHED */
		} else if (pid == CERROR) {
			Dout(pn, 3, "fork failed\n");
			return sendlist(pmsg, 0);
		} else
			return 0;
	} else {
		Dout(pn, 3, "!-w\n");
		return sendlist(pmsg, 0);
	}
}
