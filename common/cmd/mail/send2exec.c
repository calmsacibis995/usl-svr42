/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2exec.c	1.2.2.2"
#ident "@(#)send2exec.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2exec - send letter to a pipe and report on errors

    SYNOPSIS
	void send2exec(Msg *pmsg, int whereexec, int nowait4process)

    DESCRIPTION
	Execute the command being held for the given surrogate
	via pipeletter(). Report the output via the debugging
	files.
*/

void send2exec(pmsg, whereexec, nowait4process)
Msg	*pmsg;
int	whereexec;
int	nowait4process;
{
    static const char pn[] = "send2exec";
    
    pipeletter(pmsg, whereexec, nowait4process);
    Dout(pn, 0, "Command complete, result %d\n", pmsg->surg_rc);
    if (debug)
	{
	if (pmsg->SURRoutfile)
	    {
	    fprintf(dbgfp, "=============== Start of stdout from surrogate ============\n");
	    rewind (pmsg->SURRoutfile);
	    (void) copystream(pmsg->SURRoutfile, dbgfp);
	    fprintf(dbgfp, "\n=============== End of stdout from surrogate ============\n");
	    }

	else
	    fprintf(dbgfp, "=============== Surrogate stdout unavailable ============\n");

	if (pmsg->SURRerrfile)
	    {
	    fprintf(dbgfp, "=============== Start of stderr from surrogate ============\n");
	    rewind (pmsg->SURRerrfile);
	    (void) copystream(pmsg->SURRerrfile, dbgfp);
	    fprintf(dbgfp, "\n=============== End of stderr from surrogate ============\n");
	    }

	else
	    fprintf(dbgfp, "=============== Surrogate stderr unavailable ============\n");
	}
}
