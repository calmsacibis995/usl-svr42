/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2clean.c	1.1.2.2"
#ident "@(#)send2clean.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2clean - clean and move list of recipients from one surrogate entry to another

    SYNOPSIS
	void send2clean(Msg *pmsg, int osurr_num, int nsurr_num)

    DESCRIPTION
	send2clean moves an entire list of recipients from one surrogate list
	to another surrogate list, after deleting the commands strings associated
	with the recipients.	
*/

void send2clean(pmsg, whereexec, whereto)
Msg	*pmsg;
int	whereexec;
int	whereto;
{
    Recip *r;
    /* delete the strings to save space */
    for (r = recips_head(pmsg, whereexec)->next; r != (Recip*) NULL; r = r->next)
	{
	if (r->cmdl) s_delete(r->cmdl);
	if (r->cmdr) s_delete(r->cmdr);
	}

    send2move(pmsg, whereexec, whereto);
}
